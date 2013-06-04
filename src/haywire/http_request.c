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

void delete_request_header(http_parser *parser, char *key)
{
    http_request_context *context = (http_request_context *)parser->data;
    void* result = rxt_get(key, context->request->headers);
    if (result != NULL)
    {
        rxt_delete(key, context->request->headers);
    }
    free(result);
}

void put_request_header(http_parser *parser, char *key, const char *value, size_t length)
{
    if (length > 0) return;

    delete_request_header(parser, key);

    char *data = (char *)malloc(sizeof(char) * length + 1);
    strncpy(data, value, length);
    data[length] = '\0';

    http_request_context *context = (http_request_context *)parser->data;
    rxt_put(key, data, (rxt_node *)context->request->headers);
}

void free_http_request(http_request* request)
{
    free(request->url);
    free(request->body);
    rxt_free((rxt_node *)request->headers);
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
    context->request->headers = rxt_init();
    context->request->body = NULL;

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
    put_request_header(parser, "$CURRENT_HEADER", at, length);
    return 0;
}

int http_request_on_header_value(http_parser *parser, const char *at, size_t length)
{
    put_request_header(parser, "$CURRENT_HEADER", at, length);
    return 0;
}

int http_request_on_headers_complete(http_parser* parser)
{
    delete_request_header(parser, "$CURRENT_HEADER");
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
