#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <haywire.h>
#include "hw_string.h"
#include "khash.h"
#include "http_request.h"
#include "http_response.h"
#include "http_server.h"
#include "server_stats.h"
#include "route_compare_method.h"

extern char* uv__strndup(const char* s, size_t n);

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
KHASH_MAP_INIT_INT64(offset_hashmap, int)

void hw_print_request_headers(http_request* request)
{
    hw_string* k;
    hw_string* v;

    khash_t(hw_string_hashmap) *h = request->headers;
    kh_foreach(h, k, v, {
        char* key = uv__strndup(k->value, k->length + 1);
        char* value = uv__strndup(v->value, v->length + 1);
        printf("KEY: %s VALUE: %s\n", key, value);
        free(key);
        free(value);
    });
}

void hw_print_body(http_request* request)
{
    if (request->body->length > 0) {
        char* body = uv__strndup(request->body->value, request->body->length);
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
    int ret, i;
    khiter_t k;
    khash_t(hw_string_hashmap) *h = request->headers;

    for (i = 0; i < name->length; i++)
    {
        name->value[i] = tolower(name->value[i]);
    }

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
    request->state = OK;
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
    if (connection->request) {
        /* We're seeing a new request on the same connection, so it's time to free up the old one
         * and create a new one.
         *
         * Note: This assumes that the request callback is synchronous. If we're ever to support async callbacks,
         * we either need to copy all required data and pass it into the async callback or free on write instead. */
        free_http_request(connection->request);
    }
    connection->request = create_http_request(connection);
    return 0;
}

int http_request_on_url(http_parser *parser, const char *at, size_t length)
{
    http_connection* connection = (http_connection*)parser->data;
    http_request* request = connection->request;
    hw_string* url = request->url;

    if (url->length) {
        /* This is not the first URL chunk that has been seen in the buffer. Since we've already captured the initial pointer in the branch below,
           so we can recover the URL later on, we only increment the URL length and ignore the pointer passed in as a parameter. */
        url->length += length;
    } else {
        url->value = at;
        url->length = length;
        /* We've seen the URL start, so we need to pin it to recover it later */
        http_request_buffer_pin(connection->buffer, url, url->value);
    }

    return 0;
}

void http_request_save_current_header(http_connection* connection) {
    hw_string* header_key_copy = hw_strdup(&connection->current_header_key);
    hw_string* header_value_copy = hw_strdup(&connection->current_header_value);

    /* We duplicate the current header key/value, so we actually need to use the new pointers as pin ids, as
     * those will be the IDs we'll use later on to locate the headers */
    http_request_buffer_reassign_pin(connection->buffer, &connection->current_header_key, header_key_copy);
    http_request_buffer_reassign_pin(connection->buffer, &connection->current_header_value, header_value_copy);

    /* Set headers is going to need to have the values of header key/value, so we need to make sure we get
     * the pointers pointing at the right place in the buffer */
    header_key_copy->value = http_request_buffer_locate(connection->buffer, header_key_copy,
                                                        connection->current_header_key.value);

    header_value_copy->value = http_request_buffer_locate(connection->buffer, header_value_copy,
                                                          connection->current_header_value.value);

    /* Save last header key/value pair that was read */
    set_header(connection->request, header_key_copy, header_value_copy);
}


int http_request_on_header_field(http_parser *parser, const char *at, size_t length)
{
    http_connection* connection = (http_connection*)parser->data;

    if (connection->last_was_value && connection->current_header_key.length > 0) {
        http_request_save_current_header(connection);

        connection->current_header_key.length = 0;
        connection->current_header_value.length = 0;
    }

    if (connection->current_header_key.length > 0) {
        /* This is not the first header chunk that has been seen in the buffer. Since we've already captured the initial pointer in the branch below,
           so we can recover the header later on, we only increment the header length and ignore the pointer passed in as a parameter. */
        connection->current_header_key.length += length;
    } else {
        /* Start of a new header key */
        connection->current_header_key.value = at;
        connection->current_header_key.length = length;
        /* Pin the header key */
        http_request_buffer_pin(connection->buffer, &connection->current_header_key, at);
    }

    connection->last_was_value = 0;
    return 0;
}

int http_request_on_header_value(http_parser *parser, const char *at, size_t length)
{
    http_connection* connection = (http_connection*)parser->data;

    if (!connection->last_was_value) {
        connection->current_header_value.value = at;
        connection->current_header_value.length = length;
        connection->last_was_value = 1;
        /* Pin the header value */
        http_request_buffer_pin(connection->buffer, &connection->current_header_value, at);
    } else {
        /* Another header value chunk, not necessarily contiguous to the other chunks seen before, so we only increment
         * the length. When the header value is located later on, it will be guaranteedly contiguous. */
        connection->current_header_value.length += length;
    }

    return 0;
}

int http_request_on_headers_complete(http_parser* parser)
{
    http_connection* connection = (http_connection*)parser->data;

    if (connection->current_header_key.length > 0 && connection->current_header_value.length > 0) {
        http_request_save_current_header(connection);
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
    http_request* request = connection->request;
    hw_string* body = request->body;
    if (length != 0)
    {
        if (body->length == 0) {
            body->value = at;
            body->length = length;
            /* Let's pin the body so we can recover it later, even if the underlying buffers change */
            http_request_buffer_pin(connection->buffer, body, body->value);
        } else {
            body->length += length;
        }
        
        request->body_length = body->length;
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

void send_error_response(http_request* request, http_response* response, const char* error_code,
                         const char* error_message)
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

    hw_http_response_send(response, NULL, 0);
}

/**
 * Ensures that the URL, body and header pointers are correct.
 * This is because the underlying buffers might have been reallocated/resized,
 * so we can't use the pointers of where data chunks (e.g. URL start) were originally seen.
 */
void http_request_locate_members(http_connection* connection)
{
    hw_request_buffer* buffer = connection->buffer;
    http_request* request = connection->request;
    request->url->value = http_request_buffer_locate(buffer, request->url, request->url->value);
    request->body->value = http_request_buffer_locate(buffer, request->body, request->body->value);

    hw_string* header_name;
    hw_string* header_value;

    khash_t(hw_string_hashmap) *h = request->headers;
    kh_foreach(h, header_name, header_value, {
        header_name->value = http_request_buffer_locate(buffer, header_name, header_name->value);
        header_value->value = http_request_buffer_locate(buffer, header_value, header_value->value);
    });
}

int http_request_on_message_complete(http_parser* parser)
{
    http_connection* connection = (http_connection*)parser->data;
    return http_request_complete_request(connection);
}

int http_request_complete_request(http_connection* connection)
{
    http_request* request = connection->request;
    hw_http_response* response = hw_create_http_response(connection);

    char* error = NULL;

    if (connection->request->state == SIZE_EXCEEDED) {
        // 413 Request entity too large
        error = HTTP_STATUS_413;
    } else if (connection->request->state == BAD_REQUEST) {
        // 400 Bad Request
        error = HTTP_STATUS_400;
    } else if (connection->request->state == INTERNAL_ERROR) {
        // 500 Internal Server Error
    } else {
        http_request_locate_members(connection);

        hw_route_entry* route_entry = request != NULL ? get_route_callback(request->url) : NULL;

        if (route_entry != NULL)
        {
            route_entry->callback(request, response, route_entry->user_data);
        }
        else
        {
            // 404 Not Found.
            error = HTTP_STATUS_404;
        }

        /* Let's tell the buffer that we don't care about the data that was read before that is not currently
         * being processed, so it can be swept later on.
         *
         * Since the HTTP parser doesn't tell us exactly where the request ends, we can be exact when creating a mark,
         * otherwise we could discard everything up to, and including, the end of the current request.
         *
         * Instead, we create a mark that will be set at the end of the last chunk that was read before the one
         * currently being processed and on which callback fired. */
        http_request_buffer_mark(connection->buffer);
    }
    
    if (error) {
        send_error_response(request, (http_response*)response, error, error);
    }

    return 0;
}
