#pragma once

typedef struct
{
	char *url;
} http_request;

typedef char* (*http_request_callback)(http_request *request);
extern http_request_callback http_req_callback;

int hw_http_open(char *ipaddress, int port);
void hw_http_add_route(char *route, http_request_callback callback);
