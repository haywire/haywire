typedef struct
{
    char* name;
    char* value;
} http_header;

typedef struct
{
    unsigned short http_major;
    unsigned short http_minor;
    const char* status_code;
    http_header* headers;
    int number_of_headers;
    char* body;
} http_response;
