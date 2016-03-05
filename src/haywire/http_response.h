#pragma once
#include "haywire.h"
#include "http_connection.h"

typedef struct
{
    hw_string name;
    hw_string value;
} http_header;

#define MAX_HEADERS 64
typedef struct
{
    http_connection* connection;
    http_request* request;
    unsigned short http_major;
    unsigned short http_minor;
    hw_string status_code;
    http_header headers[MAX_HEADERS];
    int number_of_headers;
    hw_string body;
    int sent;
} http_response;

typedef struct
{
    http_connection* connection;
    http_request* request;
    void* user_data;
    http_response_complete_callback callback;
} hw_write_context;

hw_http_response hw_create_http_response(http_connection* connection);
hw_string* create_response_buffer(hw_http_response* response);
