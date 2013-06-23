#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "haywire.h"
#include "http_request.h"
#include "http_parser.h"
#include "http_server.h"
#include "http_request_context.h"
#include "server_stats.h"
#include "trie/radix.h"
#include "trie/route_compare_method.h"
#include "trie/khash.h"

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

int last_was_value;

KHASH_MAP_INIT_STR(string_hashmap, char*)

void print_headers(http_request* request)
{
    char* k;
    char* v;

    khash_t(string_hashmap) *h = request->headers;
    kh_foreach(h, k, v, { printf("KEY: %s VALUE: %s\n", k, v); });
}

void set_header(http_request* request, char* name, char* value)
{
    int ret;
    khiter_t k;
    khash_t(string_hashmap) *h = request->headers;
    k = kh_put(string_hashmap, h, strdup(name), &ret);
    kh_value(h, k) = strdup(value);
}

void* get_header(http_request* request, char* name)
{
    khash_t(string_hashmap) *h = request->headers;
    khiter_t k = kh_get(string_hashmap, h, name);
    void* val = kh_value(h, k);
    int is_missing = (k == kh_end(h));
    if (is_missing)
    {
        val = NULL;
    }
    return val;
}

http_request* create_http_request(http_request_context* context)
{
    http_request* request = malloc(sizeof(http_request));
    request->url = NULL;
    request->headers = kh_init(string_hashmap);
    request->body = NULL;
    context->current_header_key_length = 0;
    context->current_header_value_length = 0;
    INCREMENT_STAT(stat_requests_created_total);
    return request;
}

void free_http_request(http_request* request)
{
    khash_t(string_hashmap) *h = request->headers;    
    char* k;
    char* v;
    kh_foreach(h, k, v, { free(k); free(v); });
    kh_destroy(string_hashmap, request->headers);
    free(request->url);
    free(request->body);    
    free(request);
    INCREMENT_STAT(stat_requests_destroyed_total);
}

char* hw_get_header(http_request* request, char* key)
{
    void* value = get_header(request, key);
    return value;
}

int http_request_on_message_begin(http_parser* parser)
{
    http_request_context *context = (http_request_context *)parser->data;
    context->request = create_http_request(context);
    return 0;
}

int http_request_on_url(http_parser *parser, const char *at, size_t length)
{
    http_request_context *context = (http_request_context *)parser->data;
    char *data = (char *)malloc(sizeof(char) * length + 1);

    strncpy(data, at, length);
    data[length] = '\0';

    context->request->url = data;

    return 0;
}

int http_request_on_header_field(http_parser *parser, const char *at, size_t length)
{
    http_request_context *context = (http_request_context *)parser->data;
    int i = 0;

    if (last_was_value && context->current_header_key_length > 0)
    {
        char *key;

        // Save last read header key/value pair.
        for (i = 0; context->current_header_key[i]; i++)
        {
            context->current_header_key[i] = tolower(context->current_header_key[i]);
        }

        set_header(context->request, context->current_header_key, context->current_header_value);

        /* Start of a new header */
        context->current_header_key_length = 0;
    }
    memcpy((char *)&context->current_header_key[context->current_header_key_length], at, length);
    context->current_header_key_length += length;
    context->current_header_key[context->current_header_key_length] = '\0';
    last_was_value = 0;
    return 0;
}

int http_request_on_header_value(http_parser *parser, const char *at, size_t length)
{
    http_request_context *context = (http_request_context *)parser->data;

    if (!last_was_value && context->current_header_value_length > 0)
    {
        /* Start of a new header */
        context->current_header_value_length = 0;
    }
    memcpy((char *)&context->current_header_value[context->current_header_value_length], at, length);
    context->current_header_value_length += length;
    context->current_header_value[context->current_header_value_length] = '\0';
    last_was_value = 1;
    return 0;
}

int http_request_on_headers_complete(http_parser* parser)
{
    http_request_context *context = (http_request_context *)parser->data;
    int i = 0;

    if (context->current_header_key_length > 0)
    {
        if (context->current_header_value_length > 0)
        {
            /* Store last header */
            for (i = 0; context->current_header_key[i]; i++)
            {
                context->current_header_key[i] = tolower(context->current_header_key[i]);
            }
            set_header(context->request, context->current_header_key, context->current_header_value);
        }
        context->current_header_key[context->current_header_key_length] = '\0';
        context->current_header_value[context->current_header_value_length] = '\0';
    }
    context->current_header_key_length = 0;
    context->current_header_value_length = 0;
    
    context->request->http_major = parser->http_major;
    context->request->http_minor = parser->http_minor;
    context->request->method = parser->method;
    context->keep_alive = http_should_keep_alive(parser);
    context->request->keep_alive = context->keep_alive;
    return 0;
}

int http_request_on_body(http_parser *parser, const char *at, size_t length)
{
    return 0;
}

http_request_callback get_route_callback(char* url)
{
    http_request_callback callback = NULL;
    //callback = (http_request_callback)rxt_get_custom(url, routes, hw_route_compare_method);
    
    char* k;
    char* v;
     
    khash_t(string_hashmap) *h = routes;
     
    kh_foreach(h, k, v,
    {
        //printf("KEY: %s VALUE: %s\n", k, v);
        int found = hw_route_compare_method(url, k);
        if (found)
        {
            callback = (http_request_callback)v;
        }
    });
     
    return callback;
}

int http_request_on_message_complete(http_parser* parser)
{
    char *response;
    http_request_context *context = (http_request_context *)parser->data;
    http_request_callback callback = get_route_callback(context->request->url);
    
    if (callback != NULL)
    {
        response = callback(context->request);
        http_server_write_response(parser, response);
    }
    else
    {
        // 404 Not Found.
        http_server_write_response(parser, (char *)response_404);
    }

    free_http_request(context->request);
    context->request = NULL;
    return 0;
}
