#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "http_response.h"
#include "haywire.h"
#include "trie/khash.h"

#define CRLF "\r\n"
KHASH_MAP_INIT_STR(string_hashmap, char*)

hw_http_response hw_create_http_response()
{
    http_response* response = malloc(sizeof(http_response));
    response->headers = kh_init(string_hashmap);
    response->http_major = 1;
    response->http_minor = 1;
    return response;
}

void hw_free_http_response(hw_http_response* response)
{
    http_response* resp = (http_response*)response;
    khash_t(string_hashmap) *h = resp->headers;
    const char* k;
    const char* v;
    kh_foreach(h, k, v, { free((char*)k); free((char*)v); });
    kh_destroy(string_hashmap, resp->headers);
    free(resp);
}

void hw_set_http_version(hw_http_response* response, unsigned short major, unsigned short minor)
{
    http_response* resp = (http_response*)response;
    resp->http_major = major;
    resp->http_minor = minor;
}

void hw_set_response_status_code(hw_http_response* response, const char* status_code)
{
    http_response* resp = (http_response*)response;
    resp->status_code = status_code;
}

void hw_set_response_header(hw_http_response* response, char* name, char* value)
{
    http_response* resp = (http_response*)response;
    int ret;
    khiter_t key;
    khash_t(string_hashmap) *hash = resp->headers;
    key = kh_put(string_hashmap, hash, strdup(name), &ret);
    kh_value(hash, key) = strdup(value);
}

void hw_set_body(hw_http_response* response, char* body)
{
    http_response* resp = (http_response*)response;
    resp->body = body;
}

char* create_response_buffer(hw_http_response* response)
{
    http_response* resp = (http_response*)response;
    char* buffer = malloc(1024);
    char* header_name;
    char* header_value;
    khash_t(string_hashmap) *hash = resp->headers;
    
    sprintf(buffer, "HTTP/%d.%d %s", resp->http_major, resp->http_minor, resp->status_code);
    strcpy(buffer + strlen(buffer), CRLF);
    
    /* Loop and add the headers */
    kh_foreach(hash, header_name, header_value,
               {
                   sprintf(buffer + strlen(buffer), "%s: %s" CRLF, header_name, header_value);
               });
    
    /* Add the body */
    sprintf(buffer + strlen(buffer), "Content-Length: %ld" CRLF CRLF, strlen(resp->body) + 3);
    strcpy(buffer + strlen(buffer), resp->body);
    strcpy(buffer + strlen(buffer), CRLF);
    
    return buffer;
}
