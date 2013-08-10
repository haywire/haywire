#include <stdio.h>
#include "server_stats.h"
#include "haywire.h"

int stat_connections_created_total;
int stat_connections_destroyed_total;
int stat_requests_created_total;
int stat_requests_destroyed_total;

#define CRLF "\r\n"

static const char stats_response[] =
  "HTTP/1.1 200 OK" CRLF
  "Server: Haywire/master" CRLF
  "Date: Fri, 26 Aug 2011 00:31:53 GMT" CRLF
  "Connection: Keep-Alive" CRLF
  "Content-Type: text/html" CRLF
  "Content-Length: 16" CRLF
  CRLF
  "stats printed" CRLF;

void get_server_stats(http_request* request, hw_http_response* response, void* user_data)
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
    
    SETSTRING(body, "stats printed");
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
    
    hw_http_response_send(response, NULL, NULL);
    
    printf("connections_created_total: %d\nconnections_destroyed_total: %d\nrequests_created_total: %d\nrequests_destroyed_total: %d\n\n",
        stat_connections_created_total,
        stat_connections_destroyed_total,
        stat_requests_created_total,
        stat_requests_destroyed_total);
}
