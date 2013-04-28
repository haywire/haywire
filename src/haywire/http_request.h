#include "http_parser.h"
#include "uv.h"

typedef struct
{
    uv_tcp_t stream;
    http_parser parser;
    uv_write_t write_req;
	char *url;
} http_request;

int http_request_on_message_begin(http_parser* parser);
int http_request_on_url(http_parser *parser, const char *at, size_t length);
int http_request_on_headers_complete(http_parser* parser);
int http_request_on_message_complete(http_parser* parser);