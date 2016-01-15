#pragma comment (lib, "libuv.lib")
#pragma comment (lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Iphlpapi.lib")

#ifdef PLATFORM_POSIX
#include <signal.h>
#endif // PLATFORM_POSIX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <haywire.h>
#include "uv.h"
#include "hw_string.h"
#include "khash.h"
#include "http_server.h"
#include "connection_consumer.h"
#include "connection_dispatcher.h"
#include "http_response_cache.h"
#include "server_stats.h"
#include "configuration/configuration.h"
#include "http_connection.h"

#define UVERR(err, msg) fprintf(stderr, "%s: %s\n", msg, uv_strerror(err))
#define CHECK(r, msg) \
if (r) { \
uv_err_t err = uv_last_error(uv_loop); \
UVERR(err, msg); \
exit(1); \
}

KHASH_MAP_INIT_STR(string_hashmap, hw_route_entry*)

configuration* config;
static uv_tcp_t server;
static http_parser_settings parser_settings;
static struct sockaddr_in listen_address;

uv_loop_t* uv_loop;
void* routes;
hw_string* http_v1_0;
hw_string* http_v1_1;
hw_string* server_name;
int listener_count;
uv_async_t* listener_async_handles;
uv_loop_t* listener_event_loops;
uv_barrier_t* listeners_created_barrier;

int hw_init_with_config(configuration* c)
{
#ifdef DEBUG
    char route[] = "/stats";
    hw_http_add_route(route, get_server_stats, NULL);
#endif /* DEBUG */
    /* Copy the configuration */
    config = malloc(sizeof(configuration));
    config->http_listen_address = dupstr(c->http_listen_address);
    config->http_listen_port = c->http_listen_port;
    config->thread_count = c->thread_count;
    config->tcp_nodelay = c->tcp_nodelay;
    config->listen_backlog = c->listen_backlog? c->listen_backlog : SOMAXCONN;
    config->parser = dupstr(c->parser);
    config->max_request_size = c->max_request_size;

    http_v1_0 = create_string("HTTP/1.0 ");
    http_v1_1 = create_string("HTTP/1.1 ");
    server_name = create_string("Server: Haywire/master");

    if (strcmp(config->parser, "http_parser") == 0)
    {
        http_stream_on_read = &http_stream_on_read_http_parser;
    }
    http_server_write_response = &http_server_write_response_single;
    return 0;
}

int hw_init_from_config(char* configuration_filename)
{
    configuration* config = load_configuration(configuration_filename);
    if (config == NULL)
    {
        return 1;
    }
    return hw_init_with_config(config);
}

void print_configuration()
{
    printf("Address: %s\nPort: %d\nThreads: %d\nParser: %s\nTCP No Delay: %s\nListen backlog: %d\nMaximum request size: %d\n",
           config->http_listen_address,
           config->http_listen_port,
           config->thread_count,
           config->parser,
           config->tcp_nodelay? "on": "off",
           config->listen_backlog,
           config->max_request_size);
}

http_connection* create_http_connection()
{
    http_connection* connection = calloc(1, sizeof(http_connection));
    connection->buffer = http_request_buffer_init(config->max_request_size);
    INCREMENT_STAT(stat_connections_created_total);
    return connection;
}

void free_http_connection(http_connection* connection)
{
    if (connection->request)
    {
        free_http_request(connection->request);
    }
    http_request_buffer_destroy(connection->buffer);
    free(connection);
    INCREMENT_STAT(stat_connections_destroyed_total);
}

void set_route(void* hashmap, char* name, hw_route_entry* route_entry)
{
    int ret;
    khiter_t k;
    khash_t(string_hashmap) *h = hashmap;
    k = kh_put(string_hashmap, h, dupstr(name), &ret);
    kh_value(h, k) = route_entry;
}

void hw_http_add_route(char *route, http_request_callback callback, void* user_data)
{
    hw_route_entry* route_entry = malloc(sizeof(hw_route_entry));
    route_entry->callback = callback;
    route_entry->user_data = user_data;
    
    if (routes == NULL)
    {
        routes = kh_init(string_hashmap);
    }
    set_route(routes, route, route_entry);
    printf("Added route %s\n", route); // TODO: Replace with logging instead.
}

void free_http_server()
{
    /* TODO: Shut down accepting incoming requests */
    khash_t(string_hashmap) *h = routes;
    const char* k;
    const char* v;
    kh_foreach(h, k, v, { free((char*)k); free((char*)v); });
    kh_destroy(string_hashmap, routes);
}

int hw_http_open()
{
    int threads = config->thread_count;
    uv_async_t* service_handle = 0;

    parser_settings.on_header_field = http_request_on_header_field;
    parser_settings.on_header_value = http_request_on_header_value;
    parser_settings.on_headers_complete = http_request_on_headers_complete;
    parser_settings.on_body = http_request_on_body;
    parser_settings.on_message_begin = http_request_on_message_begin;
    parser_settings.on_message_complete = http_request_on_message_complete;
    parser_settings.on_url = http_request_on_url;
    
#ifdef UNIX
    signal(SIGPIPE, SIG_IGN);
#endif // UNIX
    
    listener_count = threads;
    
    /* TODO: Use the return values from uv_tcp_init() and uv_tcp_bind() */
    uv_loop = uv_default_loop();
    uv_tcp_init(uv_loop, &server);
    
    listener_async_handles = calloc(listener_count, sizeof(uv_async_t));
    listener_event_loops = calloc(listener_count, sizeof(uv_loop_t));
    
    listeners_created_barrier = malloc(sizeof(uv_barrier_t));
    uv_barrier_init(listeners_created_barrier, listener_count + 1);
    
    service_handle = malloc(sizeof(uv_async_t));
    uv_async_init(uv_loop, service_handle, NULL);
    
    if (listener_count == 0)
    {
        /* If running single threaded there is no need to use the IPC pipe
         to distribute requests between threads so lets avoid the IPC overhead */
        
        initialize_http_request_cache();
        http_request_cache_configure_listener(uv_loop, NULL);
        
        uv_ip4_addr(config->http_listen_address, config->http_listen_port, &listen_address);
        uv_tcp_bind(&server, (const struct sockaddr*)&listen_address, 0);
        
        if (config->tcp_nodelay) {
            uv_tcp_nodelay(&server, 1);
        }
        
        uv_listen((uv_stream_t*)&server, config->listen_backlog, http_stream_on_connect);
        print_configuration();
        printf("Listening...\n");
        uv_run(uv_loop, UV_RUN_DEFAULT);
    }
    else
    {
        int i = 0;

        /* If we are running multi-threaded spin up the dispatcher that uses
         an IPC pipe to send socket connection requests to listening threads */
        struct server_ctx* servers;
        servers = calloc(threads, sizeof(servers[0]));
        for (i = 0; i < threads; i++)
        {
            int rc = 0;
            struct server_ctx* ctx = servers + i;
            ctx->index = i;
            
            rc = uv_sem_init(&ctx->semaphore, 0);
            rc = uv_thread_create(&ctx->thread_id, connection_consumer_start, ctx);
        }
        
        uv_barrier_wait(listeners_created_barrier);
        initialize_http_request_cache();
        
        start_connection_dispatching(UV_TCP, threads, servers, config->http_listen_address, config->http_listen_port, config->tcp_nodelay, config->listen_backlog);
    }
    
    return 0;
}

void http_stream_on_connect(uv_stream_t* stream, int status)
{
    http_connection* connection = create_http_connection();
    uv_tcp_init(uv_loop, &connection->stream);
    http_parser_init(&connection->parser, HTTP_REQUEST);
    
    connection->parser.data = connection;
    connection->stream.data = connection;

    /* TODO: Use the return values from uv_accept() and uv_read_start() */
    uv_accept(stream, (uv_stream_t*)&connection->stream);
    connection->state = OPEN;
    uv_read_start((uv_stream_t*)&connection->stream, http_stream_on_alloc, http_stream_on_read);
}

void http_stream_on_alloc(uv_handle_t* client, size_t suggested_size, uv_buf_t* buf)
{
    http_connection* connection = (http_connection*)client->data;

    bool success = http_request_buffer_alloc(connection->buffer, suggested_size);
    hw_request_buffer_chunk chunk;
    chunk.size = 0;
    chunk.buffer = NULL;

    if (success) {
        http_request_buffer_chunk(connection->buffer, &chunk);
    } else {
        /* TODO out of memory event - we should hook up an application callback to this */
    }

    *buf = uv_buf_init(chunk.buffer, chunk.size);
}

void http_stream_on_close(uv_handle_t* handle)
{
    uv_handle_t* stream = handle;
    http_connection* connection = stream->data;

    if (connection->state != CLOSED) {
        connection->state = CLOSED;
        http_connection* connection = (http_connection*)handle->data;
        free_http_connection(connection);
    }
}

void http_stream_close_connection(http_connection* connection) {
    if (connection->state == OPEN) {
        connection->state = CLOSING;
        uv_close(&connection->stream, http_stream_on_close);
    }
}

void handle_request_error(http_connection* connection)
{
    uv_handle_t* stream = &connection->stream;

    if (connection->state == OPEN) {
        uv_read_stop(stream);
    }

    connection->keep_alive = false;

    if (connection->request) {
        if (connection->state == OPEN) {
            /* Send the error message back. */
            http_request_on_message_complete(&connection->parser);
        }
    } else {
        http_stream_close_connection(connection);
    }
}

void handle_bad_request(http_connection* connection)
{
    if (connection->request) {
        connection->request->state = BAD_REQUEST;
    }

    handle_request_error(connection);
}

void handle_buffer_exceeded_error(http_connection* connection)
{
    if (connection->request) {
        connection->request->state = SIZE_EXCEEDED;
    }

    handle_request_error(connection);
}

void handle_internal_error(http_connection* connection)
{
    if (connection->request) {
        connection->request->state = INTERNAL_ERROR;
    }

    handle_request_error(connection);
}

void http_stream_on_shutdown(uv_shutdown_t* req, int status)
{
    http_connection* connection = req->data;
    uv_handle_t* stream = &connection->stream;
    if (connection->state == OPEN) {
        http_stream_close_connection(connection);
    }
    free(req);
}

void http_stream_on_read_http_parser(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
{
    http_connection* connection = (http_connection*)tcp->data;

    if (nread > 0) {
        /* Need to tell the buffer that we care about the next nread bytes */
        http_request_buffer_consume(connection->buffer, nread);

        http_parser_execute(&connection->parser, &parser_settings, (const char*) buf->base, nread);

        if (connection->parser.http_errno) {
            handle_bad_request(connection);
        } else {
            /* We finished processing this chunk of data, therefore we can't get rid of any chunks that were read before
             * the current one we're reading.
             *
             * We can't get rid of the one we're currently processing as it may contain a partial request that will
             * only be complete with the next chunk coming into a subsequent call of this function. */
            http_request_buffer_sweep(connection->buffer);
        }
    } else if (nread == 0) {
        /* no-op - there's no data to be read, but there might be later */
    }
    else if (nread == UV_ENOBUFS) {
        handle_buffer_exceeded_error(connection);
    }
    else if (nread == UV_EOF){
        uv_shutdown_t* req = malloc(sizeof(uv_shutdown_t));
        req->data = connection;
        uv_shutdown(req, &connection->stream, http_stream_on_shutdown);
    }
    else if (nread == UV_ECONNRESET || nread == UV_ECONNABORTED) {
        /* Let's close the connection as the other peer just disappeared */
        http_stream_close_connection(connection);
    } else {
        /* We didn't see this coming, but an unexpected UV error code was passed in, so we'll
         * respond with a blanket 500 error if we can */
        handle_internal_error(connection);
    }
}

void http_server_cleanup_write(char* response_string, hw_write_context* write_context, uv_write_t* write_req)
{
    free(response_string);
    free(write_context);
    free(write_req);
}

int http_server_write_response_single(hw_write_context* write_context, hw_string* response)
{
    http_connection* connection = write_context->connection;

    if (connection->state == OPEN) {
        uv_write_t *write_req = (uv_write_t *) malloc(sizeof(*write_req) + sizeof(uv_buf_t));
        uv_buf_t *resbuf = (uv_buf_t *) (write_req + 1);

        resbuf->base = response->value;
        resbuf->len = response->length;

        write_req->data = write_context;

        uv_stream_t *stream = (uv_stream_t *) &write_context->connection->stream;

        if (uv_is_writable(stream)) {
            /* Ensuring that the the response can still be written. */
            uv_write(write_req, stream, resbuf, 1, http_server_after_write);
            /* TODO: Use the return values from uv_write() */
        } else {
            /* The connection was closed, so we can write the response back, but we still need to free up things */
            http_server_cleanup_write(resbuf->base, write_context, write_req);
        }
    }

    return 0;
}

void http_server_after_write(uv_write_t* req, int status)
{
    hw_write_context* write_context = (hw_write_context*)req->data;
    uv_buf_t *resbuf = (uv_buf_t *)(req+1);
    uv_handle_t* stream = (uv_handle_t*) req->handle;

    http_connection* connection = write_context->connection;

    if (!connection->keep_alive && connection->state == OPEN) {
        http_stream_close_connection(connection);
    }
    
    if (write_context->callback)
    {
        write_context->callback(write_context->user_data);
    }

    http_server_cleanup_write(resbuf->base, write_context, req);
}

