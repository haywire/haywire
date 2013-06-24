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
#include "http_server.h"
#include "http_request.h"
#include "http_parser.h"
#include "http_request_context.h"
#include "server_stats.h"
#include "configuration/configuration.h"
#include "trie/radix.h"
#include "trie/khash.h"
#include "trie/route_compare_method.h"

#define UVERR(err, msg) fprintf(stderr, "%s: %s\n", msg, uv_strerror(err))
#define CHECK(r, msg) \
  if (r) { \
    uv_err_t err = uv_last_error(uv_loop); \
    UVERR(err, msg); \
    exit(1); \
  }

KHASH_MAP_INIT_STR(string_hashmap, char*)

static configuration* config;
static uv_loop_t* uv_loop;
static uv_tcp_t server;
static http_parser_settings parser_settings;

//rxt_node *routes = NULL;
void* routes;

http_request_context* create_http_context()
{
    http_request_context* context = (http_request_context *)malloc(sizeof(http_request_context));
    context->request = NULL;
    INCREMENT_STAT(stat_connections_created_total);
    return context;
}

void free_http_context(http_request_context* context)
{
    if (context->request != NULL)
    {
        free_http_request(context->request);
    }
    
    free(context);
    INCREMENT_STAT(stat_connections_destroyed_total);
}

void set_route(void* hashmap, char* name, http_request_callback callback)
{
    int ret;
    khiter_t k;
    khash_t(string_hashmap) *h = hashmap;
    k = kh_put(string_hashmap, h, strdup(name), &ret);
    kh_value(h, k) = callback;
}

void hw_http_add_route(char *route, http_request_callback callback)
{
    if (routes == NULL)
    {
        //routes = rxt_init();
        routes = kh_init(string_hashmap);
    }
    //rxt_put(route, callback, routes);
    set_route(routes, route, callback);
    printf("Added route %s\n", route); // TODO: Replace with logging instead.
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

int hw_init_with_config(configuration* configuration)
{
#ifdef DEBUG
    char route[] = "/stats";
    hw_http_add_route(route, get_server_stats);
#endif /* DEBUG */

    config = configuration;
    
    return 0;
}

int hw_http_open()
{
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
  
    /* TODO: Use the return values from uv_tcp_init() and uv_tcp_bind() */
    uv_loop = uv_default_loop();
    uv_tcp_init(uv_loop, &server);
    uv_tcp_bind(&server, uv_ip4_addr(config->http_listen_address, config->http_listen_port));
    uv_listen((uv_stream_t*)&server, 128, http_stream_on_connect);

    printf("Listening on %s:%d\n", config->http_listen_address, config->http_listen_port);

    uv_run(uv_loop, UV_RUN_DEFAULT);
    return 0;
}

void http_stream_on_connect(uv_stream_t* stream, int status)
{
    http_request_context* context = create_http_context();
    uv_tcp_init(uv_loop, &context->stream);
    http_parser_init(&context->parser, HTTP_REQUEST);

    context->parser.data = context;
    context->stream.data = context;

    /* TODO: Use the return values from uv_accept() and uv_read_start() */
    uv_accept(stream, (uv_stream_t*)&context->stream);
    uv_read_start((uv_stream_t*)&context->stream, http_stream_on_alloc, http_stream_on_read);
}

uv_buf_t http_stream_on_alloc(uv_handle_t* client, size_t suggested_size)
{
    uv_buf_t buf;
    buf.base = (char *)malloc(suggested_size);
    buf.len = suggested_size;
    return buf;
}

void http_stream_on_close(uv_handle_t* handle)
{
    http_request_context* context = (http_request_context*)handle->data;
    free_http_context(context);
}

void http_stream_on_read(uv_stream_t* tcp, ssize_t nread, uv_buf_t buf)
{
    size_t parsed;
    http_request_context *context = (http_request_context *)tcp->data;

    if (nread >= 0) 
    {
        parsed = http_parser_execute(&context->parser, &parser_settings, buf.base, nread);
        if (parsed < nread) 
        {
            //uv_close((uv_handle_t*) &client->handle, http_stream_on_close);
        }
    } 
    else 
    {
        uv_err_t err = uv_last_error(uv_loop);
        if (err.code != UV_EOF) 
        {
            //UVERR(err, "read");
            if (context->request != NULL)
            {
                free_http_request(context->request);
            }
        }
        uv_close((uv_handle_t*) &context->stream, http_stream_on_close);
    }
    free(buf.base);
}

int http_server_write_response(http_parser *parser, char *response) 
{
    http_request_context *context = (http_request_context *)parser->data;    
    uv_write_t* write_req = (uv_write_t *)malloc(sizeof(*write_req) + sizeof(uv_buf_t));
    uv_buf_t *resbuf = (uv_buf_t *)(write_req+1);

    resbuf->base = response;
    resbuf->len = strlen(response) + 1;

    write_req->data = parser->data;

    /* TODO: Use the return values from uv_write() */
    uv_write(write_req, (uv_stream_t*)&context->stream, resbuf, 1, http_server_after_write);

    return 0;
}

void http_server_after_write(uv_write_t* req, int status)
{
    http_request_context *context = (http_request_context *)req->data;
    if (!context->keep_alive)
    {
        uv_close((uv_handle_t*)req->handle, http_stream_on_close);
    }
    free(req);
}
