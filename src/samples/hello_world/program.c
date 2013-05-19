#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "haywire.h"

char *body = "hello world";

/*
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
*/

http_response *get_root(http_request *request)
{
    keyvalue_enumerator* enumerator;
    keyvalue_pair* pair;





    http_response *resp = hw_create_response();
    /*
    hw_set_response_header(resp, "Server", "Haywire/master");
    hw_set_response_header(resp, "Date", "Fri, 26 Aug 2011 00:31:53 GMT");
    hw_set_response_header(resp, "Connection", "Keep-Alive");
    hw_set_response_header(resp, "Content-Type", "text/html");
    hw_set_response_header(resp, "Content-Length", "14");
    resp->body = malloc(12);
    char body[] = "hello world";
    char *body = (char *)malloc(12);
    body = "hello world";
    resp->body = strdup("hello world");
    */

    resp->body = strdup(body);
    /* hw_send_response(resp); */

    /*
    enumerator = keyvalue_pair_enumerator_init(request);    
    while ((pair = keyvalue_pair_next_pair(enumerator)))
    {
        printf("%s: %s\n", pair->key, pair->value);
    }
    free_keyvalue_pair_enumerator(enumerator);
    */
    
    return resp;
}

int main()
{
    char route[] = "/";

    hw_http_add_route(route, get_root);
    hw_http_open("0.0.0.0", 8000);
}
