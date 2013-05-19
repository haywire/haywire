#pragma comment (lib, "libuv.lib")
#pragma comment (lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Iphlpapi.lib")

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "uv.h"
#include "haywire.h"
#include "http_server.h"
#include "http_request.h"
#include "http_parser.h"
#include "http_request_context.h"
#include "trie/radix.h"
#include "trie/route_compare_method.h"

#define UVERR(err, msg) fprintf(stderr, "%s: %s\n", msg, uv_strerror(err))
#define CHECK(r, msg) \
  if (r) { \
    uv_err_t err = uv_last_error(uv_loop); \
    UVERR(err, msg); \
    exit(1); \
  }

static uv_loop_t* uv_loop;
static uv_tcp_t server;
static http_parser_settings parser_settings;
static uv_buf_t resbuf;

rxt_node *routes = NULL;

void hw_http_add_route(char *route, http_request_callback callback)
{
    int i;
    char *value;

    if (routes == NULL)
    {
        routes = rxt_init();
    }
    rxt_put(route, callback, routes);
    printf("Added route %s\n", route); /* TODO: Replace with logging instead. */
}

int hw_http_open(char *ipaddress, int port)
{
    int r;

    /*
    rxt_node *node;
    void *values[3] = {NULL, NULL, NULL};
    rxt_node *headers = rxt_init();
    rxt_node *current;
    rxt_put("Server", "Haywire/master", headers);
    rxt_put("Date", "Fri, 26 Aug 2011 00:31:53 GMT", headers);
    rxt_put("Connection", "Keep-Alive", headers);

    rxt_print_in_order(headers);
    printf("\n\n\n\n");
    */

    parser_settings.on_header_field = http_request_on_header_field;
    parser_settings.on_header_value = http_request_on_header_value;
    parser_settings.on_headers_complete = http_request_on_headers_complete;
    parser_settings.on_body = http_request_on_body;
    parser_settings.on_message_begin = http_request_on_message_begin;
    parser_settings.on_message_complete = http_request_on_message_complete;
    parser_settings.on_url = http_request_on_url;
  
    uv_loop = uv_default_loop();
    r = uv_tcp_init(uv_loop, &server);

    r = uv_tcp_bind(&server, uv_ip4_addr(ipaddress, port));
    uv_listen((uv_stream_t*)&server, 128, http_stream_on_connect);

    printf("Listening on 0.0.0.0:8000\n");

    uv_run(uv_loop, UV_RUN_DEFAULT);
    return 0;
}

void http_stream_on_connect(uv_stream_t* stream, int status)
{
    int r;

    http_request_context *context = (http_request_context *)malloc(sizeof(http_request_context));

    uv_tcp_init(uv_loop, &context->stream);
    http_parser_init(&context->parser, HTTP_REQUEST);

    context->parser.data = context;
    context->stream.data = context;
    context->request = NULL;

    r = uv_accept(stream, (uv_stream_t*)&context->stream);
    r = uv_read_start((uv_stream_t*)&context->stream, http_stream_on_alloc, http_stream_on_read);
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
    if (context->request != NULL)
    {
        free(context->request->url);
        free(context->request->body);
        rxt_free((rxt_node *)context->request->headers);
        free(context->request);
    }
    free(context);
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
            /* uv_close((uv_handle_t*) &client->handle, http_stream_on_close); */
        }
    } 
    else 
    {
        uv_err_t err = uv_last_error(uv_loop);
        if (err.code != UV_EOF) 
        {
            UVERR(err, "read");
        }
        uv_close((uv_handle_t*) &context->stream, http_stream_on_close);
    }
    free(buf.base);
}

int http_server_write_response(http_parser *parser, http_response *response) 
{
    int r;
    http_request_context *context = (http_request_context *)parser->data;
    uv_write_t* write_req = (uv_write_t *)malloc(sizeof(*write_req));

    resbuf.base = response->buffer;
    resbuf.len = strlen(response->buffer) + 1;

    write_req->data = response;

    r = uv_write(write_req, (uv_stream_t*)&context->stream, &resbuf, 1, http_server_after_write);	

    return 0;
}

void http_server_after_write(uv_write_t* write, int status)
{
    /* uv_close((uv_handle_t*)req->handle, on_close); */
    http_response *response = (http_response *)write->data;
    
    /*
    http_request_context *context = (http_request_context *)parser->data;
    
    free(response->body);
    free(response->buffer);
    free(response->headers);
    free(response);
    */
    
    /* if (write->data != NULL) */
    if (response != NULL)
    {
        hw_free_response(response);
    }
    free(write);
}