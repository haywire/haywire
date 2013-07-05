#pragma once

typedef struct
{
    char* value;
    int length;
} http_request_cache_entry;

void initialize_http_request_cache();
http_request_cache_entry* get_cached_request(char* http_status);
