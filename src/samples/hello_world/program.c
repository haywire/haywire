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

char *on_http_request()
{
	return (char *)response;
}

int main()
{
	hw_http_register_request_callback(on_http_request);
    hw_http_open("0.0.0.0", 8000);
}