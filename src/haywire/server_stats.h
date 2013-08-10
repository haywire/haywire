#pragma once
#include "haywire.h"

#ifdef DEBUG
#define INCREMENT_STAT(stat) stat++
#else
#define INCREMENT_STAT(stat)
#endif /* DEBUG */


extern int stat_connections_created_total;
extern int stat_connections_destroyed_total;
extern int stat_requests_created_total;
extern int stat_requests_destroyed_total;

void get_server_stats(http_request* request, hw_http_response* response, void* user_data);
