#pragma comment (lib, "libuv.lib")
#pragma comment (lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Iphlpapi.lib")

/** @file http_server.c */
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
#include "http_connection.h"
#include "http_response_cache.h"
#include "server_stats.h"
#include "route_compare_method.h"
#include "configuration/configuration.h"

/** Macro for print out Libuv errors to the stderr stream */
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

/**
 * Initialise a new http_connection structure
 *
 * @note If DEBUG is defined - will increment the number of created connections in server stats
 *
 * @return A pointer to the http_connection; otherwise NULL
 */
http_connection* create_http_connection()
{
    http_connection* connection = malloc(sizeof(http_connection));
    if(connection == NULL)
    {
        return NULL;
    }

    connection->request = NULL;
    INCREMENT_STAT(stat_connections_created_total);
    return connection;
}

/**
 * Free the memory of an allocated http_connection structure
 *
 * @note If DEBUG is defined - will increment the number of 
 *       destroyed connections in server stats
 *
 * @param connection A pointer to an http_connection structure
 */
void free_http_connection(http_connection* connection)
{
    if (connection->request != NULL)
    {
        free_http_request(connection->request);
    }
    
    free(connection);
    INCREMENT_STAT(stat_connections_destroyed_total);
}

/**
 * Adds an initialised route entry to the hashtable
 *
 * @param hashmap A pointer to the route table hashmap
 * @param name The string URL representation of the route
 * @param route_entry The initialised route_entry
 */
void set_route(void* hashmap, char* name, hw_route_entry* route_entry)
{
    int ret;
    khiter_t k;
    khash_t(string_hashmap) *h = hashmap;
    k = kh_put(string_hashmap, h, dupstr(name), &ret);
    kh_value(h, k) = route_entry;
}

/**
 * Create and add a new route to the server
 *
 * @param route The server URL of the route to add
 * @param callback The callback function for when the root is requested
 * @param user_data The user data to include on a request of the route; can be NULL
 */
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
    printf("Added route %s\n", route); /* TODO: Replace with logging instead. */
}

/**
 * Initialise the server from a configuration file
 *
 * @param configuration_filename The path and name to the configuration file
 * @return 0 on success; otherwise 1
 */
int hw_init_from_config(char* configuration_filename)
{
    configuration* config = load_configuration(configuration_filename);
    if (config == NULL)
    {
        return 1;
    }
    return hw_init_with_config(config);
}

/**
 * Initialise with a configuration loaded in memory
 *
 * @note  if DEBUG is defined - will add a new route '/stats'
 * @param c A configuration data structure
 */
int hw_init_with_config(configuration* c)
{
    int http_listen_address_length;
#ifdef DEBUG
    char route[] = "/stats";
    hw_http_add_route(route, get_server_stats, NULL);
#endif /* DEBUG */
    /* Copy the configuration to the extern */
    config = malloc(sizeof(configuration));
    config->http_listen_address = dupstr(c->http_listen_address);
    config->http_listen_port = c->http_listen_port;
    
    http_v1_0 = create_string("HTTP/1.0 ");
    http_v1_1 = create_string("HTTP/1.1 ");
    server_name = create_string("Server: Haywire/master");
    return 0;
}

/**
 * Free the http route table
 *
 * @todo Shutdown any incoming requests
 */
void free_http_server()
{
    /* TODO: Shut down accepting incoming requests */
    khash_t(string_hashmap) *h = routes;
    const char* k;
    const char* v;
    kh_foreach(h, k, v, { free((char*)k); free((char*)v); });
    kh_destroy(string_hashmap, routes);
}

/**
 * Start the http server in single or multi-threaded mode
 *
 * @note
 *    If threads are >0 - it will fire up the threads using libuv
 *    and begin dispatching connection requests using an IPC server
 *    that is started via the function start_connection_dispatching()
 *
 * @param threads The number of worker threads to start in the pool- 0 for single thread; otherwise >0
 */
int hw_http_open(int threads)
{
    uv_async_t* service_handle = 0;

    /* set the callbacks for the http parser */
    parser_settings.on_header_field = http_request_on_header_field;
    parser_settings.on_header_value = http_request_on_header_value;
    parser_settings.on_headers_complete = http_request_on_headers_complete;
    parser_settings.on_body = http_request_on_body;
    parser_settings.on_message_begin = http_request_on_message_begin;
    parser_settings.on_message_complete = http_request_on_message_complete;
    parser_settings.on_url = http_request_on_url;
    
#ifdef PLATFORM_POSIX
    signal(SIGPIPE, SIG_IGN);
#endif // PLATFORM_POSIX
    
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
        
        uv_ip4_addr(config->http_listen_address, config->http_listen_port, &listen_address);
        uv_tcp_bind(&server, (const struct sockaddr*)&listen_address, 0);
        uv_listen((uv_stream_t*)&server, 128, http_stream_on_connect);
        printf("Listening on %s:%d\n", config->http_listen_address, config->http_listen_port);
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
        
        start_connection_dispatching(UV_TCP, threads, servers, config->http_listen_address, config->http_listen_port);
    }
    
    return 0;
}

/**
 * Callback for accepting and reading a connection request on a socket
 *
 * @param stream A pointer to the stream handle
 * @param status The status of the request- 0 on success; otherwise <0
 */
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

/**
 * Callback for handling the allocation of memory for a buffer
 *
 * @param client A pointer to a client handle
 * @param suggested_size The suggested size of allocation
 * @param buf A pointer to a libuv buffer
 */
void http_stream_on_alloc(uv_handle_t* client, size_t suggested_size, uv_buf_t* buf)
{
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

/**
 * A callback for handling the closing a stream
 *
 * This function frees the connection via free_http_connection
 *
 * @handle The handle to the stream
*/
void http_stream_on_close(uv_handle_t* handle)
{
    http_connection* connection = (http_connection*)handle->data;
    free_http_connection(connection);
}

/**
 * Callback for handling read operations on a stream
 *
 * @param tcp Pointer to the stream handle
 * @param nread >0 there is data; 0 done reading; <0 an error occured
 * @param buf The buffer data type
 */
void http_stream_on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
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

/**
 * Write a single response buffer to a stream
 *
 * @todo Use return values from ub_write
 *
 * @param write_context A pointer to the hw_write_context to write to
 * @param response A pointer to the hw_string response buffer to write to the stream
 *
 */
int http_server_write_response(hw_write_context* write_context, hw_string* response)
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

/**
 * Callback for after data has been written to a stream
 *
 * @param req A pointer to write request
 * @param status The status of the request- 0 on success; otherwise 1
 */
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
    
    free(write_context);
    free(resbuf->base);
    free(req);
}
