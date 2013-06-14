#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "haywire.h"
#include "http_request.h"
#include "http_parser.h"
#include "http_server.h"
#include "http_request_context.h"
#include "trie/radix.h"
#include "trie/route_compare_method.h"
#include "trie/bftree_map.h"

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

void free_http_request(http_request* request)
{
    free(request->url);
    free(request->body);
    bftmap_free(request->headers);
    free(request);
}

char * hw_get_header(http_request *request, char *key)
{
    return (char *)rxt_get(key, (rxt_node *)request->headers);
}

int http_request_on_message_begin(http_parser* parser)
{
    http_request_context *context = (http_request_context *)parser->data;
    context->request = (http_request *)malloc(sizeof(http_request));
    context->request->url = NULL;
    context->request->headers = bftmap_create();
    context->request->body = NULL;
    context->request->current_header_key_length = 0;
    context->request->current_header_value_length = 0;

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

    if (last_was_value && context->request->current_header_key_length > 0)
    {
        // Save last read header key/value pair.
        bftmap_put(context->request->headers, context->request->current_header_key, context->request->current_header_key_length, strdup(context->request->current_header_value));
        
        /* Start of a new header */
        context->request->current_header_key_length = 0;  
    }
    memcpy((char *)&context->request->current_header_key[context->request->current_header_key_length], at, length);
    context->request->current_header_key_length += length;
    context->request->current_header_key[context->request->current_header_key_length] = '\0';
    last_was_value = 0;
    return 0;
}

int http_request_on_header_value(http_parser *parser, const char *at, size_t length)
{
    http_request_context *context = (http_request_context *)parser->data;
    
    if (!last_was_value && context->request->current_header_value_length > 0)
    {
        /* Start of a new header */
        context->request->current_header_value_length = 0;
    }
    memcpy((char *)&context->request->current_header_value[context->request->current_header_value_length], at, length);
    context->request->current_header_value_length += length;
    context->request->current_header_value[context->request->current_header_value_length] = '\0';
    last_was_value = 1;
    return 0;
}

int http_request_on_headers_complete(http_parser* parser)
{
    http_request_context *context = (http_request_context *)parser->data;
    
    if (context->request->current_header_key_length > 0)
    {
        if (context->request->current_header_value_length > 0)
        {
            bftmap_put(context->request->headers, context->request->current_header_key, context->request->current_header_key_length, strdup(context->request->current_header_value));
        }
        context->request->current_header_key[context->request->current_header_key_length] = '\0';
        context->request->current_header_value[context->request->current_header_value_length] = '\0';
    }
    context->request->current_header_key_length = 0;
    context->request->current_header_value_length = 0;
    return 0;
}

int http_request_on_body(http_parser *parser, const char *at, size_t length)
{
    return 0;
}

int http_request_on_message_complete(http_parser* parser)
{
    char *response;
    http_request_context *context = (http_request_context *)parser->data;
    http_request_callback callback = (http_request_callback)rxt_get_custom(context->request->url, routes, hw_route_compare_method);
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
