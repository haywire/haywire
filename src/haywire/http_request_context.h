#pragma once
#include "uv.h"
#include "http_parser.h"
#include "http_request.h"

typedef struct
{
    uv_tcp_t stream;
    http_parser parser;
    uv_write_t write_req;
    http_request *request;
} http_request_context;