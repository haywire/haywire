#include "haywire.h"

typedef struct
{
    hw_string name;
    hw_string value;
} http_header;

#define MAX_HEADERS 64
typedef struct
{
    unsigned short http_major;
    unsigned short http_minor;
    hw_string status_code;
    http_header headers[MAX_HEADERS];
    int number_of_headers;
    hw_string body;
} http_response;

hw_string* create_response_buffer(hw_http_response* response);
