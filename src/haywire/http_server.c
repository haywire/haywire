#pragma comment (lib, "libuv.lib")
#pragma comment (lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Iphlpapi.lib")

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "uv.h"
#include "http_server.h"
#include "../../include/haywire.h"
#include "http_request.h"
#include "http_parser.h"

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

static int request_num = 1;

http_request_callback http_req_callback;

void hw_http_register_request_callback(http_request_callback callback)
{
	http_req_callback = callback;
}

int hw_http_open(char *ipaddress, int port)
{
    int r;

    parser_settings.on_headers_complete = http_request_on_headers_complete;
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

    http_request *request = (http_request *)malloc(sizeof(http_request));

    uv_tcp_init(uv_loop, &request->stream);
    http_parser_init(&request->parser, HTTP_REQUEST);

    request->parser.data = request;
    request->stream.data = request;

    r = uv_accept(stream, (uv_stream_t*)&request->stream);
    r = uv_read_start((uv_stream_t*)&request->stream, http_stream_on_alloc, http_stream_on_read);
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
    http_request* request = (http_request*) handle->data;
    free(request);
}

void http_stream_on_read(uv_stream_t* tcp, ssize_t nread, uv_buf_t buf)
{
    size_t parsed;
    http_request *request = (http_request *)tcp->data;

    if (nread >= 0) 
    {
        parsed = http_parser_execute(&request->parser, &parser_settings, buf.base, nread);
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
            UVERR(err, "read");
        }
        uv_close((uv_handle_t*) &request->stream, http_stream_on_close);
    }
    free(buf.base);
}

int http_server_write_response(http_parser *parser, char *response) 
{
    http_request *request = (http_request *)parser->data;
    uv_write_t* write_req = (uv_write_t *)malloc(sizeof(*write_req));
	int r;
	
	resbuf.base = response;
	resbuf.len = strlen(response) + 1;

    r = uv_write(write_req, (uv_stream_t*)&request->stream, &resbuf, 1, http_server_after_write);	

    return 0;
}

void http_server_after_write(uv_write_t* req, int status)
{
    //uv_close((uv_handle_t*)req->handle, on_close);
    free(req);
}