#pragma once
#include "uv.h"
#include "haywire.h"
#include "http_parser.h"
#include "http_request_buffers.h"

typedef struct http_connection
{
    uv_tcp_t stream;
    http_parser parser;
    uv_write_t write_req;
    http_request* request;
    hw_string current_header_key;
    hw_string current_header_value;
    int keep_alive;
    int last_was_value;
    enum {OPEN, CLOSING, CLOSED} state;
    hw_request_buffer* buffer;
    unsigned int prevbuflen;
} http_connection;

http_connection* create_http_connection();
int http_request_complete_request(struct http_connection* connection);