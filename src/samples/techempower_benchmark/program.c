#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "haywire.h"

#define CRLF "\r\n"

void response_complete(void* user_data)
{
}

void get_plaintext(http_request* request, hw_http_response* response, void* user_data)
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
    
    SETSTRING(content_type_value, "text/plain");
    hw_set_response_header(response, &content_type_name, &content_type_value);
    
    SETSTRING(body, "Hello, World!");
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
    
    hw_http_response_send(response, NULL, response_complete);
}

int main(int args, char** argsv)
{
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

    hw_init_with_config(&config);
    hw_http_add_route("/plaintext", get_plaintext, NULL);
    hw_http_open(48);
    return 0;
}
