#pragma once
#include "uv.h"
#include "http_parser.h"

int http_request_on_message_begin(http_parser *parser);
int http_request_on_url(http_parser *parser, const char *at, size_t length);
int http_request_on_header_field(http_parser *parser, const char *at, size_t length);
int http_request_on_header_value(http_parser *parser, const char *at, size_t length);
int http_request_on_body(http_parser *parser, const char *at, size_t length);
int http_request_on_headers_complete(http_parser *parser);
int http_request_on_message_complete(http_parser *parser);
