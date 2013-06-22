#pragma once
/* Request Methods */
#define HW_HTTP_METHOD_MAP(XX)       \
XX(0,  DELETE,      DELETE)       \
XX(1,  GET,         GET)          \
XX(2,  HEAD,        HEAD)         \
XX(3,  POST,        POST)         \
XX(4,  PUT,         PUT)          \
/* pathological */                \
XX(5,  CONNECT,     CONNECT)      \
XX(6,  OPTIONS,     OPTIONS)      \
XX(7,  TRACE,       TRACE)        \
/* webdav */                      \
XX(8,  COPY,        COPY)         \
XX(9,  LOCK,        LOCK)         \
XX(10, MKCOL,       MKCOL)        \
XX(11, MOVE,        MOVE)         \
XX(12, PROPFIND,    PROPFIND)     \
XX(13, PROPPATCH,   PROPPATCH)    \
XX(14, SEARCH,      SEARCH)       \
XX(15, UNLOCK,      UNLOCK)       \
/* subversion */                  \
XX(16, REPORT,      REPORT)       \
XX(17, MKACTIVITY,  MKACTIVITY)   \
XX(18, CHECKOUT,    CHECKOUT)     \
XX(19, MERGE,       MERGE)        \
/* upnp */                        \
XX(20, MSEARCH,     M-SEARCH)     \
XX(21, NOTIFY,      NOTIFY)       \
XX(22, SUBSCRIBE,   SUBSCRIBE)    \
XX(23, UNSUBSCRIBE, UNSUBSCRIBE)  \
/* RFC-5789 */                    \
XX(24, PATCH,       PATCH)        \
XX(25, PURGE,       PURGE)        \

enum hw_http_method
{
#define XX(num, name, string) HW_HTTP_##name = num,
    HW_HTTP_METHOD_MAP(XX)
#undef XX
};

typedef struct
{
    unsigned short http_major;
    unsigned short http_minor;
    unsigned char method;
    int keep_alive;
	char *url;
    void *headers;
    char *body;
} http_request;

typedef char* (*http_request_callback)(http_request *request);
extern http_request_callback http_req_callback;

int hw_http_open(char *ipaddress, int port);
void hw_http_add_route(char *route, http_request_callback callback);
char * hw_get_header(http_request *request, char *key);
