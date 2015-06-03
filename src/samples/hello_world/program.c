#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "haywire.h"

#define CRLF "\r\n"

void response_complete(void* user_data)
{
}

void get_root(http_request* request, hw_http_response* response, void* user_data)
{
    hw_string status_code;
    hw_string content_type_name;
    hw_string content_type_value;
    hw_string body;
    hw_string keep_alive_name;
    hw_string keep_alive_value;
    
    SETSTRING(status_code, HTTP_STATUS_200);
    hw_set_response_status_code(response, &status_code);
    
    SETSTRING(content_type_name, "Content-Type");
    
    SETSTRING(content_type_value, "text/html");
    hw_set_response_header(response, &content_type_name, &content_type_value);
    
    SETSTRING(body, "hello world");
    hw_set_body(response, &body);
    
    if (request->keep_alive)
    {
        SETSTRING(keep_alive_name, "Connection");
        
        SETSTRING(keep_alive_value, "Keep-Alive");
        hw_set_response_header(response, &keep_alive_name, &keep_alive_value);
    }
    else
    {
        hw_set_http_version(response, 1, 0);
    }
    
    hw_http_response_send(response, "user_data", response_complete);
}

int main(int args, char** argsv)
{
    char route[] = "/";
    configuration config;

    if (args > 1)
    {
        config.http_listen_port = atoi(argsv[1]);
    }
    else
    {
        config.http_listen_port = 8000;
    }
    
    if (args > 2)
    {
        config.thread_count = atoi(argsv[2]);
    }
    else
    {
        config.thread_count = 0;
    }

    if (args > 3)
    {
        config.response_batch_size = atoi(argsv[3]);
    }
    else
    {
        config.response_batch_size = 1;
    }

    /* hw_init_from_config("hello_world.conf"); */
    config.http_listen_address = "0.0.0.0";
    hw_init_with_config(&config);
    hw_http_add_route(route, get_root, NULL);
    hw_http_open();
    return 0;
}
