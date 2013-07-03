typedef struct
{
    unsigned short http_major;
    unsigned short http_minor;
    const char* status_code;
    void* headers;
    char* body;
} http_response;
