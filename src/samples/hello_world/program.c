#include <stdio.h>
#include <stdlib.h>
#include "haywire.h"

#define CRLF "\r\n"

hw_http_response* get_root(http_request *request)
{
    hw_http_response* response = hw_create_http_response();
    hw_set_response_status_code(response, HTTP_STATUS_200);
    hw_set_response_header(response, "Server", "Haywire/master");
    hw_set_response_header(response, "Date", "Fri, 31 Aug 2011 00:31:53 GMT");
    hw_set_response_header(response, "Content-Type", "text/html");
    hw_set_body(response, "hello world");
    
    if (request->keep_alive)
    {
        hw_set_response_header(response, "Connection", "Keep-Alive");
    }
    else
    {
        hw_set_http_version(response, 1, 0);
    }
    return response;
}

int main(int args, char** argsv)
{
    char route[] = "/";
    configuration config;
    config.http_listen_address = "0.0.0.0";
    if (args > 1)
    {
        config.http_listen_port = atoi(argsv[1]);
    }
    else
    {
        config.http_listen_port = 8000;
    }

    /* hw_init_from_config("hello_world.conf"); */
    hw_init_with_config(&config);
    hw_http_add_route(route, get_root);
    hw_http_open();
    return 0;
}
