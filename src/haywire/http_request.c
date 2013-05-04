#include <stdio.h>
#include "haywire.h"
#include "http_request.h"
#include "http_parser.h"
#include "http_server.h"
#include "http_request_context.h"

int http_request_on_message_begin(http_parser* parser)
{
    http_request_context *context = (http_request_context *)parser->data;
    if (context->request != NULL)
    {
        free(context->request->url);
        free(context->request);
    }

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
    http_request_context *context = (http_request_context *)parser->data;
    char *response = http_req_callback(context->request);
    http_server_write_response(parser, response);	

    return 0;
}