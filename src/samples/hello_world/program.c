#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <haywire.h>
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
    hw_string route_matched_name;
    hw_string route_matched_value;
    
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
    
    SETSTRING(route_matched_name, "Route-Matched");
    route_matched_value.value = (char *) user_data;
    route_matched_value.length = strlen((char *) user_data);
    hw_set_response_header(response, &route_matched_name, &route_matched_value);
    
    hw_http_response_send(response, "user_data", response_complete);
}

int main(int args, char** argsv)
{
    char route[] = "/";
    char sub_route[] = "/*";
    configuration config;
    config.http_listen_address = "0.0.0.0";

    struct opt_config *conf;
    conf = opt_config_init();
    opt_flag_int(conf, &config.http_listen_port, "port", 8000, "Port to listen on.");
    opt_flag_int(conf, &config.thread_count, "threads", 0, "Number of threads to use.");
    opt_flag_string(conf, &config.parser, "parser", "http_parser", "HTTP parser to use");
    opt_flag_bool(conf, &config.tcp_nodelay, "tcp_nodelay", "If present, enables tcp_nodelay (i.e. disables Nagle's algorithm).");
    opt_flag_int(conf, &config.listen_backlog, "listen_backlog", 0, "Maximum size of the backlog when accepting connection. Defaults to SOMAXCONN.");
    args = opt_config_parse(conf, args, argsv);

    hw_init_with_config(&config);
    
    hw_http_add_route(route, get_root, route);
    hw_http_add_route(sub_route, get_root, sub_route);
    
    hw_http_open();

    opt_config_free(conf);
    return 0;
}
