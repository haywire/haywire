#pragma once
#include "uv.h"
#include "http_parser.h"
#include "http_request.h"

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
} http_connection;
