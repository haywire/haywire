#include <stdio.h>
#include "haywire.h"

#define CRLF "\r\n"
static const char response[] =
    "HTTP/1.0 200 OK" CRLF
    "Server: Haywire/master" CRLF
    "Date: Fri, 26 Aug 2011 00:31:53 GMT" CRLF
    "Content-Type: text/html" CRLF
    "Content-Length: 14" CRLF
    CRLF
    "hello world" CRLF;

static const char response_keep_alive[] =
    "HTTP/1.1 200 OK" CRLF
    "Server: Haywire/master" CRLF
    "Date: Fri, 26 Aug 2011 00:31:53 GMT" CRLF
    "Connection: Keep-Alive" CRLF
    "Content-Type: text/html" CRLF
    "Content-Length: 14" CRLF
    CRLF
    "hello world" CRLF;

char *get_root(http_request *request)
{
    char* value = hw_get_header(request, "user-agent");
    /* printf("HEADER: %s\n", value); */
    
    if (request->keep_alive)
        return (char *)response_keep_alive;
    else
        return (char *)response;
}

int main()
{
    char route[] = "/";

    hw_init_from_config("hello_world.conf");
    hw_http_add_route(route, get_root);
    hw_http_open();
}
