#include <stddef.h>

#pragma once
#include "uv.h"
#include "http_parser.h"

extern int last_was_value;

typedef struct
{
    void* header_name_offsets;
    void* header_value_offsets;
    unsigned int body_offset;
    unsigned int url_offset;
    int in_use;
} http_request_offsets;

void free_http_request(http_request* request);
int http_request_on_message_begin(http_parser *parser);
int http_request_on_url(http_parser *parser, const char *at, size_t length);
int http_request_on_header_field(http_parser *parser, const char *at, size_t length);
int http_request_on_header_value(http_parser *parser, const char *at, size_t length);
int http_request_on_body(http_parser *parser, const char *at, size_t length);
int http_request_on_headers_complete(http_parser *parser);
int http_request_on_message_complete(http_parser *parser);
void http_request_update_offsets(void* old_buffer, void* new_buffer, http_request_offsets* offsets, http_request* request);
void http_request_commit_offsets(void* buffer, http_request_offsets* offsets, http_request* request);
void http_request_reset_offsets(http_request_offsets* offsets);
void free_http_request_offsets(http_request_offsets* offsets);