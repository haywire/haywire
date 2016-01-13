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
    char* body = strndup(request->body->value, request->body->length + 1);
    printf("BODY: %s\n", body);
    free(body);
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
    http_request* request = malloc(sizeof(http_request));
    request->url = NULL;
    request->headers = kh_init(hw_string_hashmap);
    request->url = malloc(sizeof(hw_string));
    request->url->length = 0;
    request->url->value = NULL;
    request->body_length = 0;
    request->body = malloc(sizeof(hw_string));
    request->body->value = NULL;
    request->body->length = 0;
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
    kh_destroy(hw_string_hashmap, request->headers);
    
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
    
    // TODO: This should be zero-copy to remove the malloc/strncpy.
    int buffer_length = sizeof(char) * length;
    connection->request->url->value = (char *)malloc(buffer_length);
    connection->request->url->length = buffer_length;
    strncpy(connection->request->url->value, at, length);
    
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

void get_404_response(http_request* request, http_response* response)
{
    hw_string status_code;
    hw_string content_type_name;
    hw_string content_type_value;
    hw_string body;
    hw_string keep_alive_name;
    hw_string keep_alive_value;
    
    SETSTRING(status_code, HTTP_STATUS_404);
    hw_set_response_status_code(response, &status_code);
    
    SETSTRING(content_type_name, "Content-Type");
    
    SETSTRING(content_type_value, "text/html");
    hw_set_response_header(response, &content_type_name, &content_type_value);
    
    SETSTRING(body, "404 Not Found");
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
    hw_route_entry* route_entry = get_route_callback(connection->request->url);
    hw_string* response_buffer;
    hw_write_context* write_context;
    hw_http_response* response = hw_create_http_response(connection);
    
    if (route_entry != NULL)
    {
        route_entry->callback(connection->request, response, route_entry->user_data);
    }
    else
    {
        // 404 Not Found.
        write_context = malloc(sizeof(hw_write_context));
        write_context->connection = connection;
        write_context->request = connection->request;
        write_context->callback = 0;
        get_404_response(connection->request, (http_response*)response);
        response_buffer = create_response_buffer(response);
        http_server_write_response(write_context, response_buffer);
        free(response_buffer);
        hw_free_http_response(response);
    }
    
    free_http_request(connection->request);
    connection->request = NULL;
    
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
