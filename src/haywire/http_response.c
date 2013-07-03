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
    response->headers = malloc(sizeof(http_header) * 64);
    response->http_major = 1;
    response->http_minor = 1;
    response->number_of_headers = 0;
    return response;
}

void hw_free_http_response(hw_http_response* response)
{
    http_response* resp = (http_response*)response;
    for (int i=0; i< resp->number_of_headers; i++)
    {
        free(resp->headers[i].name);
        free(resp->headers[i].value);
    }
    free(resp->headers);
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
    http_header* header = &resp->headers[resp->number_of_headers];
    header->name = strdup(name);
    header->value = strdup(value);
    resp->headers[resp->number_of_headers] = *header;
    resp->number_of_headers++;
}

void hw_set_body(hw_http_response* response, char* body)
{
    http_response* resp = (http_response*)response;
    resp->body = body;
}

char* itoa(int val, int base)
{	
	static char buf[32] = {0};
	int i = 30;
	
	for(; val && i ; --i, val /= base)
    {
        
		buf[i] = "0123456789abcdef"[val % base];
    }
	return &buf[i+1];
}

char* create_response_buffer(hw_http_response* response)
{
    http_response* resp = (http_response*)response;
    char* buffer = malloc(1024);
    int body_len = strlen(resp->body);
    
    strcat(buffer, "HTTP/");
    strcat(buffer, itoa(resp->http_major, 10));
    strcat(buffer, ".");
    strcat(buffer, itoa(resp->http_minor, 10));
    strcat(buffer, " ");
    strcat(buffer, resp->status_code);
    strcat(buffer, CRLF);
    
    for (int i=0; i< resp->number_of_headers; i++)
    {
        strcat(buffer, resp->headers[i].name);
        strcat(buffer, ": ");
        strcat(buffer, resp->headers[i].value);
        strcat(buffer, CRLF);
    }
    
    /* Add the body */
    strcat(buffer, "Content-Length: ");
    strcat(buffer, itoa(body_len + 3, 10));
    strcat(buffer, CRLF CRLF);
    strcat(buffer, resp->body);
    strcat(buffer, CRLF);
    
    return buffer;
}
