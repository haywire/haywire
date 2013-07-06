#pragma once
#include "haywire.h"

void initialize_http_request_cache();
hw_string* get_cached_request(char* http_status);
