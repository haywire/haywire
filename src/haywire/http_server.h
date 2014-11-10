#pragma once
#include "uv.h"
#include "haywire.h"
#include "http_connection.h"
#include "http_parser.h"
/** @file http_server.h */

/**
 * A datatype structure used to describe
 * the handling of a single route entry in
 * the route table
 */
typedef struct
{
    http_request_callback callback; /**< The callback for handling the route request */
    void* user_data; /**< Any data to bundle with the request handling */
} hw_route_entry;

/**
 * A datatype structure to hold a stream context for writing
 */
typedef struct
{
    http_connection* connection; /**< The http_connection description data structure*/
    void* user_data; /**< Any user data bundled into the request */
    http_response_complete_callback callback; /**< Callback on write completion */
} hw_write_context;

/**
 * A union that holds the handle to a stream - TCP socket or Pipe
 */
union stream_handle
{
    uv_pipe_t pipe;
    uv_tcp_t tcp;
};

/** The hashtable of routes (route table) */
extern void* routes;

extern uv_loop_t* uv_loop;
extern hw_string* http_v1_0;
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
