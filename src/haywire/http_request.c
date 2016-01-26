#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "haywire.h"
#include "hw_string.h"
#include "khash.h"
#include "http_request.h"
#include "http_response.h"
#include "http_parser.h"
#include "http_server.h"
#include "http_connection.h"
#include "server_stats.h"
#include "route_compare_method.h"

#define CRLF "\r\n"
static const char response_404[] =
  "HTTP/1.1 404 Not Found" CRLF
  "Server: Haywire/master" CRLF
  "Date: Fri, 26 Aug 2011 00:31:53 GMT" CRLF
  "Connection: Keep-Alive" CRLF
  "Content-Type: text/html" CRLF
  "Content-Length: 16" CRLF
  CRLF
  "404 Not Found" CRLF
  ;

static kh_inline khint_t hw_string_hash_func(hw_string* s)
{
    khint_t h = s->length > 0 ? (khint_t)*s->value : 0;
    if (h) for (int i = 0; i < s->length; i++) h = (h << 5) - h + (khint_t)*(s->value + i);
    return h;
}

#define hw_string_hash_equal(a, b) (hw_strcmp(a, b) == 0)

KHASH_INIT(hw_string_hashmap, hw_string*,  hw_string*, 1, hw_string_hash_func, hw_string_hash_equal)
KHASH_INIT(offset_hashmap, hw_string*, int*, 1, hw_string_hash_func, hw_string_hash_equal)

KHASH_MAP_INIT_STR(string_hashmap, char*)

void hw_print_request_headers(http_request* request)
{
    hw_string* k;
    hw_string* v;

    khash_t(hw_string_hashmap) *h = request->headers;
    kh_foreach(h, k, v, {
        char* key = strndup(k->value, k->length + 1);
        char* value = strndup(v->value, v->length + 1);
        printf("KEY: %s VALUE: %s\n", key, value);
        free(key);
        free(value);
    });
}

void hw_print_body(http_request* request)
{
    if (request->body->length > 0) {
        char* body = strndup(request->body->value, request->body->length + 1);
        printf("BODY: %s\n", body);
        free(body);
    }
    else {
        printf("BODY is empty!\n");
    }
}

void* get_header(http_request* request, hw_string* name)
{
    khash_t(hw_string_hashmap) *h = request->headers;
    khiter_t k = kh_get(hw_string_hashmap, h, name);

    void* val;
    
    int is_missing = (k == kh_end(h));
    if (is_missing) {
        val = NULL;
    } else {
        val = kh_value(h, k);
    }
    return val;
}

void set_header(http_request* request, hw_string* name, hw_string* value)
{
    int ret;
    khiter_t k;
    khash_t(hw_string_hashmap) *h = request->headers;
    
    void* prev = get_header(request, name);
    if (prev) {
        free(prev);
    }
    
    k = kh_put(hw_string_hashmap, h, name, &ret);
    kh_value(h, k) = value;
}

http_request* create_http_request(http_connection* connection)
{
    http_request* request = calloc(1, sizeof(http_request));
    request->headers = kh_init(hw_string_hashmap);
    request->url = calloc(1, sizeof(hw_string));
    request->body = calloc(1, sizeof(hw_string));
    INCREMENT_STAT(stat_requests_created_total);
    return request;
}

void free_http_request(http_request* request)
{
    khash_t(hw_string_hashmap) *h = request->headers;

    hw_string* k;
    hw_string* v;
    kh_foreach(h, k, v,
    {
        free((hw_string*)k);
        free((hw_string*)v);
    });
    kh_destroy(hw_string_hashmap, h);
    
    free(request->url);
    free(request->body);
    free(request);
    INCREMENT_STAT(stat_requests_destroyed_total);
}

hw_string* hw_get_header(http_request* request, hw_string* key)
{
    void* value = get_header(request, key);
    return value;
}

int http_request_on_message_begin(http_parser* parser)
{
    http_connection* connection = (http_connection*)parser->data;
    connection->request = create_http_request(connection);
    return 0;
}

int http_request_on_url(http_parser *parser, const char *at, size_t length)
{
    http_connection* connection = (http_connection*)parser->data;
    connection->request->url->value = at;
    connection->request->url->length = length;
    
    return 0;
}

int http_request_on_header_field(http_parser *parser, const char *at, size_t length)
{
    http_connection* connection = (http_connection*)parser->data;
    int i = 0;

    if (connection->last_was_value && connection->current_header_key.length > 0)
    {
        // Save last read header key/value pair.
        for (i = 0; i < connection->current_header_key.length; i++)
        {
            connection->current_header_key.value[i] = tolower(connection->current_header_key.value[i]);
        }

        set_header(connection->request, hw_strdup(&connection->current_header_key), hw_strdup(&connection->current_header_value));

        /* Start of a new header */
        connection->current_header_key.length = 0;
    }
    connection->current_header_key.value = at;
    connection->current_header_key.length = length;
    connection->last_was_value = 0;
    return 0;
}

int http_request_on_header_value(http_parser *parser, const char *at, size_t length)
{
    http_connection* connection = (http_connection*)parser->data;

    if (!connection->last_was_value && connection->current_header_value.length > 0)
    {
        /* Start of a new header */
        connection->current_header_value.length = 0;
    }
    connection->current_header_value.value = at;
    connection->current_header_value.length = length;
    connection->last_was_value = 1;
    return 0;
}

int http_request_on_headers_complete(http_parser* parser)
{
    http_connection* connection = (http_connection*)parser->data;
    int i = 0;

    if (connection->current_header_key.length > 0)
    {
        if (connection->current_header_value.length > 0)
        {
            /* Store last header */
            for (i = 0; i < connection->current_header_key.length; i++)
            {
                connection->current_header_key.value[i] = tolower(connection->current_header_key.value[i]);
            }
            set_header(connection->request, hw_strdup(&connection->current_header_key), hw_strdup(&connection->current_header_value));
        }
    }
    connection->current_header_key.length = 0;
    connection->current_header_value.length = 0;
    
    connection->request->http_major = parser->http_major;
    connection->request->http_minor = parser->http_minor;
    connection->request->method = parser->method;
    connection->keep_alive = http_should_keep_alive(parser);
    connection->request->keep_alive = connection->keep_alive;
    return 0;
}

int http_request_on_body(http_parser *parser, const char *at, size_t length)
{
    http_connection* connection = (http_connection*)parser->data;
    if (length != 0)
    {
        if (connection->request->body->length == 0) {
            connection->request->body->value = at;
            connection->request->body->length = length;
        } else {
            connection->request->body->length += length;
        }
    }
    
    return 0;
}

hw_route_entry* get_route_callback(hw_string* url)
{
    hw_route_entry* route_entry = NULL;
    
    const char* k;
    const char* v;
     
    khash_t(string_hashmap) *h = routes;
     
    kh_foreach(h, k, v,
    {
        int found = hw_route_compare_method(url, k);
        if (found)
        {
            route_entry = (hw_route_entry*)v;
        }
    });
     
    return route_entry;
}

void get_error_response(http_request* request, http_response* response, const char* error_code, const char* error_message)
{
    hw_string status_code;
    hw_string content_type_name;
    hw_string content_type_value;
    hw_string body;
    hw_string keep_alive_name;
    hw_string keep_alive_value;
    
    status_code.value = error_code;
    status_code.length = strlen(error_code);
    hw_set_response_status_code(response, &status_code);
    
    SETSTRING(content_type_name, "Content-Type");
    
    SETSTRING(content_type_value, "text/html");
    hw_set_response_header(response, &content_type_name, &content_type_value);
    
    body.value = error_message;
    body.length = strlen(error_message);
    hw_set_body(response, &body);
    
    if (request->keep_alive)
    {
        SETSTRING(keep_alive_name, "Connection");
        
        SETSTRING(keep_alive_value, "Keep-Alive");
        hw_set_response_header(response, &keep_alive_name, &keep_alive_value);
    }
    else
    {
        hw_set_http_version(response, 1, 0);
    }
}


int http_request_on_message_complete(http_parser* parser)
{
    http_connection* connection = (http_connection*)parser->data;
    hw_string* response_buffer;
    hw_write_context* write_context;
    hw_http_response* response = hw_create_http_response(connection);
    char* error = NULL;
    
    if (connection->request->size_exceeded) {
        // 413 Request entity too large
        error = HTTP_STATUS_413;
    } else {
        http_request_commit_offsets(connection->buffer, &connection->offsets, connection->request);
        hw_route_entry* route_entry = connection->request != NULL ? get_route_callback(connection->request->url) : NULL;
        
        if (route_entry != NULL)
        {
            route_entry->callback(connection->request, response, route_entry->user_data);
        }
        else
        {
            // 404 Not Found.
            error = HTTP_STATUS_404;
        }
    }
    
    if (error) {
        write_context = malloc(sizeof(hw_write_context));
        write_context->connection = connection;
        write_context->request = connection->request;
        write_context->callback = 0;
        get_error_response(connection->request, (http_response*)response, error, error);
        response_buffer = create_response_buffer(response);
        http_server_write_response(write_context, response_buffer);
        free(response_buffer);
        hw_free_http_response(response);
    }

    if (connection->keep_alive) {
        http_request_reset_offsets(&connection->offsets);
    }

    return 0;
}

void hw_http_response_send(hw_http_response* response, void* user_data, http_response_complete_callback callback)
{
    hw_write_context* write_context = malloc(sizeof(hw_write_context));
    http_response* resp = (http_response*)response;
    hw_string* response_buffer = create_response_buffer(response);
    
    write_context->connection = resp->connection;
    write_context->request = resp->connection->request;
    write_context->user_data = user_data;
    write_context->callback = callback;
    http_server_write_response(write_context, response_buffer);
    resp->sent = 1;
    
    free(response_buffer);
    hw_free_http_response(response);
}

void free_http_request_header_offsets_values(http_request_offsets* offsets) {
    khash_t(offset_hashmap) *name_offsets = offsets->header_name_offsets;
    khash_t(offset_hashmap) *value_offsets = offsets->header_value_offsets;
    hw_string* name;
    hw_string* value;
    int* offset;
    
    if (name_offsets) {
        kh_foreach(name_offsets, name, offset,
        {
            /* the keys are not freed as they'll be used in the headers map */
            free(offset);
        });
    }
    

    if (value_offsets) {
        kh_foreach(value_offsets, value, offset,
        {
            /* the keys are not freed as they'll be used in the headers map */
            free(offset);
        });
    }
}
void free_http_request_header_offsets(http_request_offsets* offsets) {
    free_http_request_header_offsets_values(offsets);
    kh_destroy(offset_hashmap, offsets->header_name_offsets);
    kh_destroy(offset_hashmap, offsets->header_value_offsets);
}

void http_request_reset_offsets(http_request_offsets* offsets) {
    offsets->url_offset = 0;
    offsets->body_offset = 0;
    free_http_request_header_offsets_values(offsets);
    offsets->in_use = 0;
}

void http_request_update_offsets(void* old_buffer, void* new_buffer, http_request_offsets* offsets, http_request* request) {
    hw_string* header_name;
    hw_string* header_value;
    
    if (!request) {
        /* it's possible that this is called even before the parser has had the chance to detect the beginning of the request */
        return;
    }
    
    if (!offsets->in_use) {
        /* first time initialization */
        offsets->header_name_offsets = kh_init(offset_hashmap);
        offsets->header_value_offsets = kh_init(offset_hashmap);
        offsets->in_use = 1;
    }
    
    if (offsets->url_offset == 0 && request->url && request->url->length > 0) {
        offsets->url_offset = (void*) request->url->value - old_buffer;
    }
    
    if (offsets->body_offset == 0 && request->body && request->body->length > 0) {
        offsets->body_offset = (void*) request->body->value - old_buffer;
    }
    
    khash_t(hw_string_hashmap) *header_map = request->headers;
    khash_t(offset_hashmap) *name_offsets = offsets->header_name_offsets;
    khash_t(offset_hashmap) *value_offsets = offsets->header_value_offsets;
    
    if (header_map) {
        kh_foreach(header_map, header_name, header_value, {
            khiter_t name_offset_key = kh_get(offset_hashmap, name_offsets, header_name);
            khiter_t value_offset_key = kh_get(offset_hashmap, value_offsets, header_name);
            
            int* offset;
            int ret;
            khint_t k;

            int is_missing = (name_offset_key == kh_end(name_offsets));
            if (is_missing) {
                offset = malloc(sizeof(int));
                *offset = (void*) header_name->value - old_buffer + new_buffer;
                k = kh_put(offset_hashmap, name_offsets, header_name, &ret);
                kh_value(name_offsets, k) = offset;
            }
        
            is_missing = (value_offset_key == kh_end(value_offsets));
            if (is_missing) {
                offset = malloc(sizeof(int));
                *offset = (void*) header_value->value - old_buffer + new_buffer;
                k = kh_put(offset_hashmap, value_offsets, header_name, &ret);
                kh_value(value_offsets, k) = offset;
            }
        });
    }
}

void http_request_commit_offsets(void* buffer, http_request_offsets* offsets, http_request* request) {
    if (offsets != NULL && offsets->in_use) {
        hw_string* header_name;
        hw_string* header_value;
        
        request->body->value = buffer + offsets->body_offset;
        request->url->value = buffer + offsets->url_offset;
        
        khash_t(hw_string_hashmap) *header_map = request->headers;
        khash_t(offset_hashmap) *name_offsets = offsets->header_name_offsets;
        khash_t(offset_hashmap) *value_offsets = offsets->header_value_offsets;

        kh_foreach(header_map, header_name, header_value, {
            khiter_t name_offset_key = kh_get(offset_hashmap, name_offsets, header_name);
            khiter_t value_offset_key = kh_get(offset_hashmap, value_offsets, header_name);
            
            int offset;
            
            offset = *kh_value(name_offsets, name_offset_key);
            header_name->value = buffer + offset;
            
            offset = *kh_value(value_offsets, value_offset_key);
            header_value->value = buffer + offset;
        });
    }
}
