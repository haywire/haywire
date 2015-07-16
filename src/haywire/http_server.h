#pragma once
#include "uv.h"
#include "haywire.h"
#include "http_connection.h"
#include "http_parser.h"

typedef struct
{
    http_request_callback callback;
    void* user_data;
} hw_route_entry;

typedef struct
{
    http_connection* connection;
    void* user_data;
    http_response_complete_callback callback;
} hw_write_context;

union stream_handle
{
    uv_pipe_t pipe;
    uv_tcp_t tcp;
};

extern void* routes;
extern uv_loop_t* uv_loop;
extern hw_string* http_v1_1;
extern hw_string* server_name;
extern int listener_count;
extern uv_async_t* listener_async_handles;
extern uv_loop_t* listener_event_loops;
extern uv_barrier_t* listeners_created_barrier;

http_connection* create_http_connection();
void http_stream_on_connect(uv_stream_t* stream, int status);
void http_stream_on_alloc(uv_handle_t* client, size_t suggested_size, uv_buf_t* buf);
void http_stream_on_close(uv_handle_t* handle);
int http_server_write_response(hw_write_context* write_context, hw_string* response);
void http_server_after_write(uv_write_t* req, int status);
void http_stream_on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf);
