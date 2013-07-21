#pragma once
#include "uv.h"
#include "haywire.h"
#include "http_parser.h"

extern void* routes;
extern uv_loop_t* uv_loop;

void http_stream_on_connect(uv_stream_t* stream, int status);
uv_buf_t http_stream_on_alloc(uv_handle_t* client, size_t suggested_size);
void http_stream_on_close(uv_handle_t* handle);
int http_server_write_response(http_parser *parser, hw_string* response);
void http_server_after_write(uv_write_t* req, int status);
void http_stream_on_read(uv_stream_t* tcp, ssize_t nread, uv_buf_t buf);
