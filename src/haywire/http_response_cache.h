#pragma once
#include "uv.h"
#include "haywire.h"

void initialize_http_request_cache();
hw_string* get_cached_request(const char* http_status);
void http_request_cache_configure_listener(uv_loop_t* loop, uv_async_t* handle);
