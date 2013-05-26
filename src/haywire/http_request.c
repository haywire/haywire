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

char * hw_get_request_header(http_request *request, char *key)
{
    return (char *)rxt_get(key, (rxt_node *)request->headers);
}

int http_request_on_message_begin(http_parser* parser)
{
    http_request_context *context = (http_request_context *)parser->data;
    if (context->request != NULL)
    {
        free(context->request->url); /* CRASH: Why does this cause a crash? */
        free(context->request->body);
        rxt_free((rxt_node *)context->request->headers);
        free(context->request);
    }

    context->request = malloc(sizeof(http_request));
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
    http_request_context *context = (http_request_context *)parser->data;
    char *data = (char *)malloc(sizeof(char) * length + 1);

    strncpy(data, at, length);
    data[length] = '\0';

    rxt_put("$CURRENT_HEADER", data, (rxt_node *)context->request->headers);

    return 0;
}

int http_request_on_header_value(http_parser *parser, const char *at, size_t length)
{
    http_request_context *context = (http_request_context *)parser->data;
    char *header = (char *)rxt_get("$CURRENT_HEADER", (rxt_node *)context->request->headers);
    char *data = (char *)malloc(sizeof(char) * length + 1);

    strncpy(data, at, length);
    data[length] = '\0';

    rxt_put(header, data, (rxt_node *)context->request->headers);
    return 0;
}

int http_request_on_headers_complete(http_parser* parser)
{
    return 0;
}

int http_request_on_body(http_parser *parser, const char *at, size_t length)
{
    return 0;
}

int bytes_added(int result_of_sprintf)
{
    return (result_of_sprintf > 0) ? result_of_sprintf : 0;
}

int http_request_on_message_complete(http_parser* parser)
{
    int length = 0;

    http_response *response;
    http_request_context *context = (http_request_context *)parser->data;
    http_request_callback callback = (http_request_callback)rxt_get_custom(context->request->url, routes, hw_route_compare_method);
    if (callback != NULL)
    {
        response = callback(context->request);
        
        length += bytes_added(sprintf(response->buffer + length, "HTTP/1.1 200 OK" CRLF));
        length += bytes_added(sprintf(response->buffer + length, "Server: Haywire/master" CRLF));
        length += bytes_added(sprintf(response->buffer + length, "Date: Fri, 26 Aug 2011 00:31:53 GMT" CRLF));
        length += bytes_added(sprintf(response->buffer + length, "Connection: Keep-Alive" CRLF));
        length += bytes_added(sprintf(response->buffer + length, "Content-Type: Haywire/master" CRLF));
        length += bytes_added(sprintf(response->buffer + length, "Content-Length: %lu" CRLF CRLF, strlen(response->body) + 3));
        length += bytes_added(sprintf(response->buffer + length, "%s" CRLF, response->body));

        http_server_write_response(parser, response);
    }
    else
    {
        /* 404 Not Found. */
        /* http_server_write_response(parser, (char *)response_404); */
    }

    return 0;
}