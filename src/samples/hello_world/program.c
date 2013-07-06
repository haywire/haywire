#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "haywire.h"

#define CRLF "\r\n"

hw_http_response* get_root(http_request *request)
{
    hw_http_response* response = hw_create_http_response();
    
    hw_string* status_code = malloc(sizeof(hw_string));
    status_code->value = HTTP_STATUS_200;
    status_code->length = 6;
    hw_set_response_status_code(response, status_code);
    
    hw_string* content_type_name = malloc(sizeof(hw_string));
    content_type_name->value = "Content-Type";
    content_type_name->length = 12;

    hw_string* content_type_value = malloc(sizeof(hw_string));
    content_type_value->value = "text/html";
    content_type_value->length = 9;
    hw_set_response_header(response, content_type_name, content_type_value);
    
    hw_string* body = malloc(sizeof(hw_string));
    body->value = "hello world";
    body->length = 11;
    hw_set_body(response, body);
    
    if (request->keep_alive)
    {
        hw_string* keep_alive_name = malloc(sizeof(hw_string));
        keep_alive_name->value = "Connection";
        keep_alive_name->length = 10;

        hw_string* keep_alive_value = malloc(sizeof(hw_string));
        keep_alive_value->value = "Keep-Alive";
        keep_alive_value->length = 10;
        
        hw_set_response_header(response, keep_alive_name, keep_alive_value);
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
