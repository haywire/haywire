#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "haywire.h"
#include "../../../lib/libuv/include/uv.h"

#define CRLF "\r\n"

void response_complete(void* user_data)
{
}

void do_work(uv_work_t *req)
{
    hw_http_response* response = (hw_http_response*)req->data;
    hw_http_response_send(response, "user_data", response_complete);
}

void after_work(uv_work_t *req, int status)
{
    free(req);
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

void get_root2(http_request* request, hw_http_response* response, void* user_data)
{
    hw_string* status_code = malloc(sizeof(hw_string));
    status_code->value = calloc(1024, 1);
    status_code->length = 0;
    
    hw_string* content_type_name = malloc(sizeof(hw_string));
    content_type_name->value = calloc(1024, 1);
    content_type_name->length = 0;
    
    hw_string* content_type_value = malloc(sizeof(hw_string));
    content_type_value->value = calloc(1024, 1);
    content_type_value->length = 0;
    
    hw_string* body = malloc(sizeof(hw_string));
    body->value = calloc(1024, 1);
    body->length = 0;
    
    hw_string* keep_alive_name = malloc(sizeof(hw_string));
    keep_alive_name->value = calloc(1024, 1);
    keep_alive_name->length = 0;

    hw_string* keep_alive_value = malloc(sizeof(hw_string));
    keep_alive_value->value = calloc(1024, 1);
    keep_alive_value->length = 0;
    
    APPENDSTRING(status_code, HTTP_STATUS_200);
    hw_set_response_status_code(response, status_code);
    
    APPENDSTRING(content_type_name, "Content-Type");
    APPENDSTRING(content_type_value, "text/html");
    
    hw_set_response_header(response, content_type_name, content_type_value);
    
    APPENDSTRING(body, "hello world");

    hw_set_body(response, body);
    
    if (request->keep_alive)
    {
        APPENDSTRING(keep_alive_name, "Connection");
        APPENDSTRING(keep_alive_value, "Keep-Alive");
        hw_set_response_header(response, keep_alive_name, keep_alive_value);
    }
    else
    {
        hw_set_http_version(response, 1, 0);
    }
    
    // Used to send off the request immediately...
    //hw_http_response_send(response, "user_data", response_complete);
    
    uv_work_t* context = malloc(sizeof(uv_work_t));
    context->data = response;
    
    // But now testing the request being completed on a background thread.
    uv_queue_work(hw_get_eventloop(), context, do_work, after_work);
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
    hw_http_add_route(route, get_root2, NULL);
    hw_http_open(0);
    return 0;
}
