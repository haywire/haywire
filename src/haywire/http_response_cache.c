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

static uv_timer_t cache_invalidation_timer;
static uv_key_t thread_cache_key;

void initialize_http_request_cache();
void free_http_request_cache();
void http_request_cache_timer(uv_timer_t* handle);
void create_cached_http_request(khash_t(string_hashmap)* http_request_cache, char* http_status);
void set_cached_request(khash_t(string_hashmap)* http_request_cache, char* http_status, hw_string* cache_entry);
hw_string* get_cached_request(char* http_status);

void initialize_http_request_cache()
{
    uv_key_create(&thread_cache_key);
}

void http_request_cache_configure_listener(uv_loop_t* loop, uv_async_t* handle)
{
    uv_timer_t* cache_invalidation_timer = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, cache_invalidation_timer);
    uv_timer_start(cache_invalidation_timer, http_request_cache_timer, 500, 500);
    
    if (handle != NULL)
    {
        uv_unref((uv_handle_t*) cache_invalidation_timer);
    }
}

void http_request_cache_timer(uv_timer_t* timer)
{
    khash_t(string_hashmap)* http_request_cache = uv_key_get(&thread_cache_key);
    if (http_request_cache != NULL)
    {
        free_http_request_cache(http_request_cache);
    }
}

void free_http_request_cache(khash_t(string_hashmap)* http_request_cache)
{
    const char* k;
    hw_string* v;
    
    kh_foreach(http_request_cache, k, v,
               {
                   free((char*)k);
                   free(v->value);
                   free(v);
               });
    
    kh_destroy_string_hashmap(http_request_cache);
    uv_key_set(&thread_cache_key, NULL);
}

void create_cached_http_request(khash_t(string_hashmap)* http_request_cache, char* http_status)
{
    hw_string* cache_entry = malloc(sizeof(hw_string));
    cache_entry->value = calloc(1024, 1);
    cache_entry->length = 0;
    
    append_string(cache_entry, http_v1_1);
    APPENDSTRING(cache_entry, http_status);
    APPENDSTRING(cache_entry, CRLF);
    append_string(cache_entry, server_name);
    APPENDSTRING(cache_entry, CRLF);

    // Add the current time.
    time_t curtime;
    time(&curtime);
    char* current_time = ctime(&curtime);
    hw_string current_datetime;
    current_datetime.value = current_time;
    current_datetime.length = strlen(current_time);
    APPENDSTRING(cache_entry, "Date: ");
    append_string(cache_entry, &current_datetime);
    
    set_cached_request(http_request_cache, http_status, cache_entry);
}

void set_cached_request(khash_t(string_hashmap)* http_request_cache, char* http_status, hw_string* cache_entry)
{
    int ret;
    int is_missing;
    khiter_t key;
    hw_string* val;
    
    key = kh_get(string_hashmap, http_request_cache, http_status);
    if (key != 0)
    {
        /* cache entry already exists and another thread beat us. Ignore. */
        val = kh_value(http_request_cache, key);
        is_missing = (key == kh_end(http_request_cache));
        
        if (!is_missing)
        {
            printf("FOUND\n");
            //kh_del(string_hashmap, http_request_cache, key);
        }
    }
    
    key = kh_put(string_hashmap, http_request_cache, dupstr(http_status), &ret);
    kh_value(http_request_cache, key) = cache_entry;
}

hw_string* get_cached_request(char* http_status)
{
    int is_missing;
    void* val;
    khash_t(string_hashmap)* http_request_cache;
    khiter_t key = 0;
    
    /* This thread hasn't created a response cache so create one */
    http_request_cache = uv_key_get(&thread_cache_key);
    if (http_request_cache == NULL)
    {
        http_request_cache = kh_init(string_hashmap);
        uv_key_set(&thread_cache_key, http_request_cache);
        //printf("CREATED CACHE\n");
    }
    
    key = kh_get(string_hashmap, http_request_cache, http_status);
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
        create_cached_http_request(http_request_cache, http_status);
    }
    
    key = kh_get(string_hashmap, http_request_cache, http_status);
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
        return NULL;
    }
    return val;
}

