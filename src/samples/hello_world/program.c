#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opt.h"
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
    config.http_listen_address = "0.0.0.0";

    struct opt_config *conf;
    struct opt_args *arguments;
    conf = opt_config_init();
    opt_flag_int(conf, &config.http_listen_port, "port", 8000, "Port to listen on.");
    opt_flag_int(conf, &config.thread_count, "threads", 0, "Number of threads to use.");
    opt_flag_string(conf, &config.parser, "parser", "http_parser", "HTTP parser to use");
    opt_flag_int(conf, &config.response_batch_size, "batch", 0, "Size to batch before sending responses.");
    opt_flag_bool(conf, &config.tcp_nodelay, "tcp_nodelay", "If present, enables tcp_nodelay (i.e. disables Nagle's algorithm).");
    args = opt_config_parse(conf, args, argsv);

    //hw_init_from_config("hello_world.conf");
    hw_init_with_config(&config);
    hw_http_add_route(route, get_root, NULL);
    hw_http_open();

    opt_config_free(conf);
    opt_args_free(arguments);
    return 0;
}
