#pragma once
#include "uv.h"
#include "http_parser.h"
#include "http_request.h"

typedef struct http_connection
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
    uv_buf_t response_buffers[1024];
    int response_buffers_count;
    int prevbuflen;
    char* request_buffer;
    int request_buffer_length;
} http_connection;

http_request* create_http_request(http_connection* connection);
void free_http_request(http_request* request);
int http_request_on_message_begin(http_parser *parser);
int http_request_on_url(http_parser *parser, const char *at, size_t length);
int http_request_on_header_field(http_parser *parser, const char *at, size_t length);
int http_request_on_header_value(http_parser *parser, const char *at, size_t length);
int http_request_on_body(http_parser *parser, const char *at, size_t length);
int http_request_on_headers_complete(http_parser *parser);
int http_request_on_message_complete(http_parser *parser);
int http_request_complete_request(struct http_connection* connection);
