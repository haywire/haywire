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
#include <assert.h>
#include "uv.h"
#include "haywire.h"
#include "hw_string.h"
#include "khash.h"
#include "http_server.h"
#include "connection_consumer.h"
#include "connection_dispatcher.h"
#include "http_request.h"
#include "http_parser.h"
#include "picohttpparser.h"
#include "http_connection.h"
#include "http_response_cache.h"
#include "server_stats.h"
#include "route_compare_method.h"
#include "configuration/configuration.h"

#define UVERR(err, msg) fprintf(stderr, "%s: %s\n", msg, uv_strerror(err))
#define CHECK(r, msg) \
if (r) { \
uv_err_t err = uv_last_error(uv_loop); \
UVERR(err, msg); \
exit(1); \
}

KHASH_MAP_INIT_STR(string_hashmap, hw_route_entry*)

static configuration* config;
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

void signal_handler(int signal)
{
    // TODO: We should clean up initialization leak gracefully.
    // https://github.com/kellabyte/Haywire/pull/83
    exit(EXIT_SUCCESS);
}

int hw_init_with_config(configuration* c)
{
    int http_listen_address_length;
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
    config->parser = dupstr(c->parser);

    http_v1_0 = create_string("HTTP/1.0 ");
    http_v1_1 = create_string("HTTP/1.1 ");
    server_name = create_string("Server: Haywire/master");

    if (strcmp(config->parser, "http_parser") == 0)
    {
        http_stream_on_read = &http_stream_on_read_http_parser;
    }
    else if (strcmp(config->parser, "pico") == 0)
    {
        http_stream_on_read = &http_stream_on_read_pico;
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
    printf("Address: %s\nPort: %d\nThreads: %d\nParser: %s\nTCP No Delay: %s\n",
           config->http_listen_address,
           config->http_listen_port,
           config->thread_count,
           config->parser,
           config->tcp_nodelay? "on": "off");
}

http_connection* create_http_connection()
{
    http_connection* connection = malloc(sizeof(http_connection));
    connection->request = NULL;
    connection->request_buffer = calloc(32768, sizeof(char));
    connection->request_buffer_length = 0;
    connection->prevbuflen = 0;
    connection->current_header_key.length = 0;
    connection->current_header_value.length = 0;
    connection->last_was_value = 0;
    INCREMENT_STAT(stat_connections_created_total);
    return connection;
}

void free_http_connection(http_connection* connection)
{
    if (connection->request != NULL)
    {
        free_http_request(connection->request);
    }
    
    free(connection->request_buffer);
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
    signal(SIGINT, signal_handler);
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
        
        uv_listen((uv_stream_t*)&server, 128, http_stream_on_connect);
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
        
        start_connection_dispatching(UV_TCP, threads, servers, config->http_listen_address, config->http_listen_port, config->tcp_nodelay);
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
    uv_read_start((uv_stream_t*)&connection->stream, http_stream_on_alloc, http_stream_on_read);
}

void http_stream_on_alloc(uv_handle_t* client, size_t suggested_size, uv_buf_t* buf)
{
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

void http_stream_on_close(uv_handle_t* handle)
{
    http_connection* connection = (http_connection*)handle->data;
    free_http_connection(connection);
}

void http_stream_on_read_http_parser(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
{
    size_t parsed;
    http_connection* connection = (http_connection*)tcp->data;
    
    if (nread >= 0)
    {
        parsed = http_parser_execute(&connection->parser, &parser_settings, buf->base, nread);
        if (parsed < nread)
        {
            /* uv_close((uv_handle_t*) &client->handle, http_stream_on_close); */
        }
    }
    else
    {
        uv_close((uv_handle_t*) &connection->stream, http_stream_on_close);
    }
    free(buf->base);
}

void http_stream_on_read_pico(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
{
    http_connection* connection = (http_connection*)tcp->data;
    
    // TODO: nread == 0 should mean the socket closed. We should handle this properly.
    
    if (nread == UV_EOF)
    {
        uv_close((uv_handle_t*) &connection->stream, http_stream_on_close);
    }
    else if (nread >= 0)
    {
        memcpy(&connection->request_buffer[connection->request_buffer_length], buf->base, nread);
        connection->request_buffer_length += nread;
        int counter = 0;
        
        while (connection->prevbuflen < connection->request_buffer_length)
        {
            counter++;
            
            int parsed;
            char *method;
            char *path;
            int minor_version;
            struct phr_header headers[100];
            size_t method_len;
            size_t path_len;
            size_t num_headers;
            num_headers = sizeof(headers) / sizeof(headers[0]);
            
            parsed = phr_parse_request(&connection->request_buffer[connection->prevbuflen], connection->request_buffer_length - connection->prevbuflen,
                                       &method, &method_len, &path, &path_len, &minor_version, headers,
                                       &num_headers, 0);
            
            if (parsed > 0)
            {
                // Successfully parsed the request.
                connection->prevbuflen += parsed;
                
                connection->keep_alive = 1;
                http_request* request = create_http_request(connection);
                connection->request = request;
                
                connection->request->url->value = (char *)malloc(path_len + 1);
                request->url->length = path_len;
                strncpy(connection->request->url->value, path, path_len);
                path[path_len] = 0x00;
                
                // TODO: Zero-copy headers parsed by pico. Need this PR to implement.
                // https://github.com/kellabyte/Haywire/pull/79
                
                http_request_complete_request(connection);
            }
            else if (parsed == -1)
            {
                memmove(connection->request_buffer, &connection->request_buffer[connection->prevbuflen], connection->request_buffer_length - connection->prevbuflen);
                connection->request_buffer_length = connection->request_buffer_length - connection->prevbuflen;
                connection->prevbuflen = 0;
                break;
            }
            else if (parsed == -2)
            {
                break;
            }
        }
        
        if (connection->prevbuflen == connection->request_buffer_length)
        {
            connection->request_buffer_length = 0;
            connection->prevbuflen = 0;
        }
    }
    else
    {
        uv_close((uv_handle_t*) &connection->stream, http_stream_on_close);
    }
    free(buf->base);
    connection->prevbuflen = 0;
}

int http_server_write_response_single(hw_write_context* write_context, hw_string* response)
{
    uv_write_t* write_req = (uv_write_t *)malloc(sizeof(*write_req) + sizeof(uv_buf_t));
    uv_buf_t* resbuf = (uv_buf_t *)(write_req+1);
    
    resbuf->base = response->value;
    resbuf->len = response->length + 1;
    
    write_req->data = write_context;
    
    /* TODO: Use the return values from uv_write() */
    uv_write(write_req, (uv_stream_t*)&write_context->connection->stream, resbuf, 1, http_server_after_write);
    return 0;
}

void http_server_after_write(uv_write_t* req, int status)
{
    hw_write_context* write_context = (hw_write_context*)req->data;
    uv_buf_t *resbuf = (uv_buf_t *)(req+1);
    
    if (!write_context->connection->keep_alive)
    {
        uv_close((uv_handle_t*)req->handle, http_stream_on_close);
    }
    
    if (write_context->callback != 0)
    {
        write_context->callback(write_context->user_data);
    }
    
    write_context->request = NULL;

    free(write_context);
    free(resbuf->base);
    free(req);    
}
