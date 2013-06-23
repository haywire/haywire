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

char* get_server_stats(http_request* request)
{
    printf("connections_created_total: %d\nconnections_destroyed_total: %d\nrequests_created_total: %d\nrequests_destroyed_total: %d\n\n",
        stat_connections_created_total,
        stat_connections_destroyed_total,
        stat_requests_created_total,
        stat_requests_destroyed_total);
    
    return (char *)stats_response;
}
