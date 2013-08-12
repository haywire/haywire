#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "uv.h"
#include "haywire.h"
#include "hw_string.h"
#include "khash.h"
#include "http_response_cache.h"
#include "http_server.h"

#define CRLF "\r\n"
KHASH_MAP_INIT_STR(string_hashmap, hw_string*)

static khash_t(string_hashmap)* http_request_cache;
static uv_timer_t cache_invalidation_timer;
static uv_rwlock_t http_response_cache_lock;

void initialize_http_request_cache();
void free_http_request_cache();
void http_request_cache_timer(uv_timer_t* handle, int status);
void create_cached_http_request(char* http_status);
void set_cached_request(char* http_status, hw_string* cache_entry);
hw_string* get_cached_request(char* http_status);

void initialize_http_request_cache()
{
    http_request_cache = kh_init(string_hashmap);
    uv_rwlock_init(&http_response_cache_lock);
    uv_timer_init(uv_loop, &cache_invalidation_timer);
    uv_timer_start(&cache_invalidation_timer, http_request_cache_timer, 500, 500);
}

void free_http_request_cache()
{
    const char* k;
    const hw_string* v;
    kh_foreach(http_request_cache, k, v,
               {
                   free((char*)k);
                   free(v->value);
                   free((char*)v);
               });
    kh_destroy(string_hashmap, http_request_cache);
    http_request_cache = kh_init(string_hashmap);
}

void http_request_cache_timer(uv_timer_t* handle, int status)
{
    // TODO: Re-enable response cache when reader/writer lock is fixed.
    //free_http_request_cache();
}

void create_cached_http_request(char* http_status)
{
    char* buffer = calloc(1024, 1);
    int length;
    hw_string* cache_entry;
    
    strcpy(buffer, "HTTP/1.1 ");
    strcat(buffer, http_status);
    strcat(buffer, CRLF);
    strcat(buffer, "Server: Haywire/master");
    strcat(buffer, CRLF);
    strcat(buffer, "Date: Fri, 31 Aug 2011 00:31:53 GMT");
    strcat(buffer, CRLF);
    
    length = strlen(buffer);
    
    cache_entry = malloc(sizeof(hw_string));
    cache_entry->value = buffer;
    cache_entry->length = length;
    set_cached_request(http_status, cache_entry);
}


void set_cached_request(char* http_status, hw_string* cache_entry)
{
    int ret;
    khiter_t key;
    
    // TODO: Fix read/write lock.
    //uv_rwlock_wrlock(&http_response_cache_lock);
    
    key = kh_put(string_hashmap, http_request_cache, dupstr(http_status), &ret);
    kh_value(http_request_cache, key) = cache_entry;
    
    //uv_rwlock_wrunlock(&http_response_cache_lock);
}

hw_string* get_cached_request(char* http_status)
{
    int is_missing;
    void* val;
    
    uv_rwlock_rdlock(&http_response_cache_lock);
    khiter_t key = kh_get(string_hashmap, http_request_cache, http_status);
    if (key == 0)
    {
        is_missing = 1;
    }
    else
    {
        val = kh_value(http_request_cache, key);
        is_missing = (key == kh_end(http_request_cache));
    }
    if (is_missing)
    {
        create_cached_http_request(http_status);
    }
    
    key = kh_get(string_hashmap, http_request_cache, http_status);
    val = kh_value(http_request_cache, key);
    is_missing = (key == kh_end(http_request_cache));
    
    uv_rwlock_rdunlock(&http_response_cache_lock);
    
    if (is_missing)
    {
        return NULL;
    }

    return val;
}

