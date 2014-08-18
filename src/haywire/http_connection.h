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
    char current_header_key[1024];
    int current_header_key_length;
    char current_header_value[1024];
    int current_header_value_length;
    int keep_alive;
    int last_was_value;
} http_connection;
