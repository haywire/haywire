#pragma once

typedef struct
{
    char* url;
    void* headers;
    char* body;
} http_request;

typedef struct
{
    void* headers;
    char* body;
    char* buffer;
} http_response;

typedef struct keyvalue_enumerator keyvalue_enumerator;

typedef struct
{
    char* key;
    void* value;
} keyvalue_pair;

typedef http_response* (*http_request_callback)(http_request *request);
extern http_request_callback http_req_callback;

int hw_http_open(char *ipaddress, int port);
void hw_http_add_route(char *route, http_request_callback callback);
char * hw_get_request_header(http_request *request, char *key);
http_response * hw_create_response();
void hw_free_response(http_response *response);
int hw_set_response_header(http_response *response, char *key, void *value);
int hw_send_response(http_response *response);
keyvalue_enumerator* keyvalue_pair_enumerator_init(http_request* request);
keyvalue_pair* keyvalue_pair_next_pair(keyvalue_enumerator* enumerator);
void free_keyvalue_pair_enumerator(keyvalue_enumerator* enumerator);
