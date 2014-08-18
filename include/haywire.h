#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#ifdef _WIN32
    /* Windows - set up dll import/export decorators. */
    #ifdef BUILDING_HAYWIRE_SHARED
        /* Building shared library. */
        #define HAYWIRE_EXTERN __declspec(dllexport)
    #else
        #ifdef USING_HAYWIRE_SHARED
            /* Using shared library. */
            #define HAYWIRE_EXTERN __declspec(dllimport)
        #else
            /* Building static library. */
            #define HAYWIRE_EXTERN /* nothing */
        #endif
    #endif

    #define HAYWIRE_CALLING_CONVENTION __cdecl
#else
    /* Building static library. */
    #define HAYWIRE_EXTERN /* nothing */
    #define HAYWIRE_CALLING_CONVENTION /* nothing */
#endif

/* Informational 1xx */
#define HTTP_STATUS_100 "100 Continue"
#define HTTP_STATUS_101 "101 Switching Protocols"
#define HTTP_STATUS_102 "102 Processing"
/* Successful 2xx */
#define HTTP_STATUS_200 "200 OK"
#define HTTP_STATUS_201 "201 Created"
#define HTTP_STATUS_202 "202 Accepted"
#define HTTP_STATUS_203 "203 Non-Authoritative Information"
#define HTTP_STATUS_204 "204 No Content"
#define HTTP_STATUS_205 "205 Reset Content"
#define HTTP_STATUS_206 "206 Partial Content"
#define HTTP_STATUS_207 "207 Multi-Status"
/* Redirection 3xx */
#define HTTP_STATUS_300 "300 Multiple Choices"
#define HTTP_STATUS_301 "301 Moved Permanently"
#define HTTP_STATUS_302 "302 Moved Temporarily"
#define HTTP_STATUS_303 "303 See Other"
#define HTTP_STATUS_304 "304 Not Modified"
#define HTTP_STATUS_305 "305 Use Proxy"
#define HTTP_STATUS_307 "307 Temporary Redirect"
/* Client Error 4xx */
#define HTTP_STATUS_400 "400 Bad Request"
#define HTTP_STATUS_401 "401 Unauthorized"
#define HTTP_STATUS_402 "402 Payment Required"
#define HTTP_STATUS_403 "403 Forbidden"
#define HTTP_STATUS_404 "404 Not Found"
#define HTTP_STATUS_405 "405 Method Not Allowed"
#define HTTP_STATUS_406 "406 Not Acceptable"
#define HTTP_STATUS_407 "407 Proxy Authentication Required"
#define HTTP_STATUS_408 "408 Request Time-out"
#define HTTP_STATUS_409 "409 Conflict"
#define HTTP_STATUS_410 "410 Gone"
#define HTTP_STATUS_411 "411 Length Required"
#define HTTP_STATUS_412 "412 Precondition Failed"
#define HTTP_STATUS_413 "413 Request Entity Too Large"
#define HTTP_STATUS_414 "414 Request-URI Too Large"
#define HTTP_STATUS_415 "415 Unsupported Media Type"
#define HTTP_STATUS_416 "416 Requested Range Not Satisfiable"
#define HTTP_STATUS_417 "417 Expectation Failed"
#define HTTP_STATUS_418 "418 I'm a teapot"
#define HTTP_STATUS_422 "422 Unprocessable Entity"
#define HTTP_STATUS_423 "423 Locked"
#define HTTP_STATUS_424 "424 Failed Dependency"
#define HTTP_STATUS_425 "425 Unordered Collection"
#define HTTP_STATUS_426 "426 Upgrade Required"
#define HTTP_STATUS_428 "428 Precondition Required"
#define HTTP_STATUS_429 "429 Too Many Requests"
#define HTTP_STATUS_431 "431 Request Header Fields Too Large"
/* Server Error 5xx */
#define HTTP_STATUS_500 "500 Internal Server Error"
#define HTTP_STATUS_501 "501 Not Implemented"
#define HTTP_STATUS_502 "502 Bad Gateway"
#define HTTP_STATUS_503 "503 Service Unavailable"
#define HTTP_STATUS_504 "504 Gateway Time-out"
#define HTTP_STATUS_505 "505 HTTP Version Not Supported"
#define HTTP_STATUS_506 "506 Variant Also Negotiates"
#define HTTP_STATUS_507 "507 Insufficient Storage"
#define HTTP_STATUS_509 "509 Bandwidth Limit Exceeded"
#define HTTP_STATUS_510 "510 Not Extended"
#define HTTP_STATUS_511 "511 Network Authentication Required"

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

#define STRLENOF(s) sizeof(s)-1
#define SETSTRING(s,val) s.value=val; s.length=STRLENOF(val)
#define APPENDSTRING(s,val) memcpy((char*)s->value + s->length, val, STRLENOF(val)); s->length+=STRLENOF(val)

typedef struct
{
    char* value;
    size_t length;
} hw_string;

typedef struct
{
    char* http_listen_address;
    int http_listen_port;
} configuration;

typedef struct
{
    unsigned short http_major;
    unsigned short http_minor;
    unsigned char method;
    int keep_alive;
	char* url;
    void* headers;
    hw_string* body;
    int body_length;
} http_request;

typedef	void* hw_http_response;

typedef void (HAYWIRE_CALLING_CONVENTION *http_request_callback)(http_request* request, hw_http_response* response, void* user_data);
typedef void (HAYWIRE_CALLING_CONVENTION *http_response_complete_callback)(void* user_data);

HAYWIRE_EXTERN int hw_init_from_config(char* configuration_filename);
HAYWIRE_EXTERN int hw_init_with_config(configuration* config);
HAYWIRE_EXTERN int hw_http_open(int threads);
HAYWIRE_EXTERN void hw_http_add_route(char* route, http_request_callback callback, void* user_data);
HAYWIRE_EXTERN char* hw_get_header(http_request* request, char* key);

HAYWIRE_EXTERN void hw_free_http_response(hw_http_response* response);
HAYWIRE_EXTERN void hw_set_http_version(hw_http_response* response, unsigned short major, unsigned short minor);
HAYWIRE_EXTERN void hw_set_response_status_code(hw_http_response* response, hw_string* status_code);
HAYWIRE_EXTERN void hw_set_response_header(hw_http_response* response, hw_string* name, hw_string* value);
HAYWIRE_EXTERN void hw_set_body(hw_http_response* response, hw_string* body);
HAYWIRE_EXTERN void hw_http_response_send(hw_http_response* response, void* user_data, http_response_complete_callback callback);
    
HAYWIRE_EXTERN void hw_print_request_headers(http_request* request);

#ifdef __cplusplus
}
#endif
