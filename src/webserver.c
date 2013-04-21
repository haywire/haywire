#pragma comment (lib, "libuv.lib")
#pragma comment (lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Iphlpapi.lib")

#define LOGGING_LEVEL LEVEL_INFO
#define LOGTO CONSOLE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "uv.h"
#include "http_parser.h"
#include "Logging.h"

#define UVERR(err, msg) fprintf(stderr, "%s: %s\n", msg, uv_strerror(err))
#define CHECK(r, msg) \
  if (r) { \
    uv_err_t err = uv_last_error(uv_loop); \
    UVERR(err, msg); \
    exit(1); \
  }

//"Connection: Keep-Alive\r\n" \

#define RESPONSE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: text/html\r\n" \
  "Content-Length: 13\r\n" \
  "Server: Haywire/master\r\n" \
  "Connection: Keep-Alive\r\n" \
  "\r\n" \
  "hello world\n"

static uv_loop_t* uv_loop;
static uv_tcp_t server;
static http_parser_settings parser_settings;

static uv_buf_t resbuf;


typedef struct 
{
    uv_tcp_t handle;
    http_parser parser;
    uv_write_t write_req;
    int request_num;
} client_t;

ONTRACE(static int connection_number = 0; static int request_number = 0;)

void on_close(uv_handle_t* handle)
{
	client_t* client;

    TRACE("    on_close()");

    client = (client_t*) handle->data;
    free(client);
}

uv_buf_t on_alloc(uv_handle_t* client, size_t suggested_size) 
{
    uv_buf_t buf;
    buf.base = (char *)malloc(suggested_size * 10);
    buf.len = suggested_size * 10;
    return buf;
}

void on_read(uv_stream_t* tcp, ssize_t nread, uv_buf_t buf) 
{
    size_t parsed;
    client_t* client;

    TRACE("    on_read()");

    client = (client_t*) tcp->data;

    if (nread >= 0) 
    {
        parsed = http_parser_execute(&client->parser, &parser_settings, buf.base, nread);
        if (parsed < nread) 
        {
            TRACE("    ERROR on_read() parsed < nread");
            //uv_close((uv_handle_t*) &client->handle, on_close);
        }
    } 
    else 
    {
        //printf("    ERROR on_read() nread < 0\n");
        uv_err_t err = uv_last_error(uv_loop);
        if (err.code != UV_EOF) 
        {
            UVERR(err, "read");
        }
        uv_close((uv_handle_t*) &client->handle, on_close);
    }
    free(buf.base);
}

static int request_num = 1;

void on_connect(uv_stream_t* server_handle, int status) 
{
    int r;
    client_t* client;

    TRACE("BEGIN CONNECTION #%d", connection_number);
    TRACE("    on_connect() status:%d", status);

    client = (client_t *)malloc(sizeof(client_t));
    client->request_num = request_num;

    uv_tcp_init(uv_loop, &client->handle);
    http_parser_init(&client->parser, HTTP_REQUEST);

    client->parser.data = client;
    client->handle.data = client;

    r = uv_accept(server_handle, (uv_stream_t*)&client->handle);
    
    TRACE("    uv_accept() returned:%d", r);

    r = uv_read_start((uv_stream_t*)&client->handle, on_alloc, on_read);

    TRACE("    uv_read_start() returned:%d", r);
}

void after_write(uv_write_t* req, int status) 
{
    TRACE("after_write() status:%d", status);

    //uv_close((uv_handle_t*)req->handle, on_close);
    free(req);
}

int write_response(http_parser* parser) 
{
    client_t* client = (client_t*) parser->data;
    uv_write_t* write_req;
	int r;

    TRACE("    write_response()");

    write_req = (uv_write_t*)malloc(sizeof(*write_req));
  
    r = uv_write(
        //&client->write_req,
        write_req,
        (uv_stream_t*)&client->handle,
        &resbuf,
        1,
        after_write);
	
    TRACE("    uv_write() returned:%d", r);

    return 0;
}

int on_headers_complete(http_parser* parser) 
{
    TRACE("    on_headers_complete()");
	TRACE("        Method: %d", parser->method);

    write_response(parser);
    
    return 0;
}

int on_message_begin(http_parser* parser)
{
    TRACE("BEGIN REQUEST #%d", request_number);
    TRACE("    on_message_begin()");

    return 0;
}

int on_message_complete(http_parser* parser)
{
    TRACE("    on_message_complete()");
    TRACE("END REQUEST #%d", request_number);

    return 0;
}

int start_server()
{
    int r;

    parser_settings.on_headers_complete = on_headers_complete;
    parser_settings.on_message_begin = on_message_begin;
    parser_settings.on_message_complete = on_message_complete;
  
    resbuf.base = RESPONSE;
    resbuf.len = sizeof(RESPONSE);

    uv_loop = uv_default_loop();
    r = uv_tcp_init(uv_loop, &server);

    r = uv_tcp_bind(&server, uv_ip4_addr("0.0.0.0", 8000));
    uv_listen((uv_stream_t*)&server, 128, on_connect);

    LOG("Listening on 0.0.0.0:8000");

    uv_run(uv_loop, UV_RUN_DEFAULT);
    return 0;
}