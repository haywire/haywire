#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "uv.h"
#include "haywire.h"
#include "http_request_cache.h"
#include "http_server.h"
#include "trie/khash.h"

#define CRLF "\r\n"
KHASH_MAP_INIT_STR(string_hashmap, http_request_cache_entry*)

static khash_t(string_hashmap)* http_request_cache;
static uv_timer_t cache_invalidation_timer;

http_request_cache_entry* get_cached_request(char* http_status)
{
    khiter_t key = kh_get(string_hashmap, http_request_cache, http_status);
    void* val = kh_value(http_request_cache, key);
    int is_missing = (key == kh_end(http_request_cache));
    if (is_missing)
    {
        val = NULL;
    }
    return val;
}

void set_cached_request(char* http_status, http_request_cache_entry* cache_entry)
{
    int ret;
    khiter_t key;
    key = kh_put(string_hashmap, http_request_cache, strdup(http_status), &ret);
    kh_value(http_request_cache, key) = cache_entry;
}

void free_http_request_cache()
{
    const char* k;
    const http_request_cache_entry* v;
    kh_foreach(http_request_cache, k, v,
    {
        free((char*)k);
        free(v->value);
        free((char*)v);
    });
    kh_destroy(string_hashmap, http_request_cache);
    http_request_cache = kh_init(string_hashmap);
}

void create_cached_http_request(char* http_status)
{
    char* buffer = malloc(1024);
    strcat(buffer, "HTTP/1.1 ");
    strcat(buffer, http_status);
    strcat(buffer, CRLF);
    strcat(buffer, "Server: Haywire/master");
    strcat(buffer, CRLF);
    strcat(buffer, "Date: Fri, 31 Aug 2011 00:31:53 GMT");
    strcat(buffer, CRLF);
    
    int length = strlen(buffer);
    buffer[length] = '\0';
    length++;
    
    http_request_cache_entry* cache_entry = malloc(sizeof(http_request_cache_entry));
    cache_entry->value = buffer;
    cache_entry->length = length;
    set_cached_request(http_status, cache_entry);
}

void create_cached_http_requests()
{
    create_cached_http_request(HTTP_STATUS_100);
    create_cached_http_request(HTTP_STATUS_101);
    create_cached_http_request(HTTP_STATUS_102);
    create_cached_http_request(HTTP_STATUS_200);
    create_cached_http_request(HTTP_STATUS_201);
    create_cached_http_request(HTTP_STATUS_202);
    create_cached_http_request(HTTP_STATUS_203);
    create_cached_http_request(HTTP_STATUS_204);
    create_cached_http_request(HTTP_STATUS_205);
    create_cached_http_request(HTTP_STATUS_206);
    create_cached_http_request(HTTP_STATUS_207);
    create_cached_http_request(HTTP_STATUS_300);
    create_cached_http_request(HTTP_STATUS_301);
    create_cached_http_request(HTTP_STATUS_302);
    create_cached_http_request(HTTP_STATUS_303);
    create_cached_http_request(HTTP_STATUS_304);
    create_cached_http_request(HTTP_STATUS_305);
    create_cached_http_request(HTTP_STATUS_307);
    create_cached_http_request(HTTP_STATUS_400);
    create_cached_http_request(HTTP_STATUS_401);
    create_cached_http_request(HTTP_STATUS_402);
    create_cached_http_request(HTTP_STATUS_403);
    create_cached_http_request(HTTP_STATUS_404);
    create_cached_http_request(HTTP_STATUS_405);
    create_cached_http_request(HTTP_STATUS_406);
    create_cached_http_request(HTTP_STATUS_407);
    create_cached_http_request(HTTP_STATUS_408);
    create_cached_http_request(HTTP_STATUS_409);
    create_cached_http_request(HTTP_STATUS_410);
    create_cached_http_request(HTTP_STATUS_411);
    create_cached_http_request(HTTP_STATUS_412);
    create_cached_http_request(HTTP_STATUS_413);
    create_cached_http_request(HTTP_STATUS_414);
    create_cached_http_request(HTTP_STATUS_415);
    create_cached_http_request(HTTP_STATUS_416);
    create_cached_http_request(HTTP_STATUS_417);
    create_cached_http_request(HTTP_STATUS_418);
    create_cached_http_request(HTTP_STATUS_422);
    create_cached_http_request(HTTP_STATUS_423);
    create_cached_http_request(HTTP_STATUS_424);
    create_cached_http_request(HTTP_STATUS_425);
    create_cached_http_request(HTTP_STATUS_426);
    create_cached_http_request(HTTP_STATUS_428);
    create_cached_http_request(HTTP_STATUS_429);
    create_cached_http_request(HTTP_STATUS_431);
    create_cached_http_request(HTTP_STATUS_500);
    create_cached_http_request(HTTP_STATUS_501);
    create_cached_http_request(HTTP_STATUS_502);
    create_cached_http_request(HTTP_STATUS_503);
    create_cached_http_request(HTTP_STATUS_504);
    create_cached_http_request(HTTP_STATUS_505);
    create_cached_http_request(HTTP_STATUS_506);
    create_cached_http_request(HTTP_STATUS_507);
    create_cached_http_request(HTTP_STATUS_509);
    create_cached_http_request(HTTP_STATUS_510);
    create_cached_http_request(HTTP_STATUS_511);
}

void http_request_cache_timer(uv_timer_t* handle, int status)
{
    free_http_request_cache();
    create_cached_http_requests();
}

void initialize_http_request_cache()
{
    http_request_cache = kh_init(string_hashmap);
    uv_timer_init(uv_loop, &cache_invalidation_timer);
    uv_timer_start(&cache_invalidation_timer, http_request_cache_timer, 500, 500);
    
    create_cached_http_requests();
}

