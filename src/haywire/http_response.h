#include "haywire.h"

typedef struct
{
    hw_string* name;
    hw_string* value;
} http_header;

typedef struct
{
    unsigned short http_major;
    unsigned short http_minor;
    hw_string* status_code;
    http_header* headers;
    int number_of_headers;
    hw_string* body;
} http_response;
