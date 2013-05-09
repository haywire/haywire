#include <stdio.h>
#include "haywire.h"

#define CRLF "\r\n"
static const char response[] =
  "HTTP/1.1 200 OK" CRLF
  "Server: Haywire/master" CRLF
  "Date: Fri, 26 Aug 2011 00:31:53 GMT" CRLF
  "Connection: Keep-Alive" CRLF
  "Content-Type: text/html" CRLF
  "Content-Length: 14" CRLF
  CRLF
  "hello world" CRLF
  ;

char *get_root(http_request *request)
{
	return (char *)response;
}

int main()
{
    char route[] = "/";

	hw_http_add_route(route, get_root);
    hw_http_open("0.0.0.0", 8000);
}
