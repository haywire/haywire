#pragma once

typedef struct
{
	char *url;
    void *headers;
    char *body;
    char current_header_key[10000024];
    int current_header_key_length;
    char current_header_value[10000024];
    int current_header_value_length;
} http_request;

typedef char* (*http_request_callback)(http_request *request);
extern http_request_callback http_req_callback;

int hw_http_open(char *ipaddress, int port);
void hw_http_add_route(char *route, http_request_callback callback);
char * hw_get_header(http_request *request, char *key);
