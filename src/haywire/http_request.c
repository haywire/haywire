#include <stdio.h>
#include <stdlib.h>
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

int http_request_on_message_begin(http_parser* parser)
{
    http_request_context *context = (http_request_context *)parser->data;
    if (context->request != NULL)
    {
        // TODO Review: generates the following warning:
        // warning: implicit declaration of function
        // 'free' is invalid in C99
        free(context->request->url);
        free(context->request);
    }

    // TODO Review: generates the following warning:
    // warning: implicitly declaring library function
    //  'malloc' with type 'void *(unsigned long)'
    //
    // The definition of malloc should probably be included via a header file.
    context->request = (http_request *)malloc(sizeof(http_request));

    return 0;
}

int http_request_on_url(http_parser *parser, const char *at, size_t length)
{
    http_request_context *context = (http_request_context *)parser->data;

    char *url = (char *)malloc(sizeof(char) * length + 1);

    strncpy(url, at, length);
    url[length] = '\0';

    context->request->url = url;

    return 0;
}

int http_request_on_headers_complete(http_parser* parser)
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

    return 0;
}
