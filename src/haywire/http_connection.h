#include <stddef.h>

#pragma once
#include "uv.h"
#include "http_parser.h"
#include "http_request.h"
#include "http_request_buffers.h"

typedef struct
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
} http_connection;
