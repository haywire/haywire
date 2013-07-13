#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "http_response.h"
#include "http_response_cache.h"
#include "haywire.h"
#include "trie/khash.h"
#include "hw_string.h"

#define CRLF "\r\n"
KHASH_MAP_INIT_STR(string_hashmap, char*)

hw_http_response hw_create_http_response()
{
    http_response* response = malloc(sizeof(http_response));
    response->http_major = 1;
    response->http_minor = 1;
    response->number_of_headers = 0;
    return response;
}

void hw_free_http_response(hw_http_response* response)
{
    http_response* resp = (http_response*)response;
    free(resp);
}

void hw_set_http_version(hw_http_response* response, unsigned short major, unsigned short minor)
{
    http_response* resp = (http_response*)response;
    resp->http_major = major;
    resp->http_minor = minor;
}

void hw_set_response_status_code(hw_http_response* response, hw_string* status_code)
{
    http_response* resp = (http_response*)response;
    resp->status_code = *status_code;
}

void hw_set_response_header(hw_http_response* response, hw_string* name, hw_string* value)
{
    http_response* resp = (http_response*)response;
    http_header* header = &resp->headers[resp->number_of_headers];
    header->name = *name;
    header->value = *value;
    resp->headers[resp->number_of_headers] = *header;
    resp->number_of_headers++;
}

void hw_set_body(hw_http_response* response, hw_string* body)
{
    http_response* resp = (http_response*)response;
    resp->body = *body;
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

hw_string* create_response_buffer(hw_http_response* response)
{
    http_response* resp = (http_response*)response;
    hw_string* response_string = malloc(sizeof(hw_string));
    response_string->value = malloc(1024);
    response_string->length = 0;
    
    hw_string* cached_entry = get_cached_request(HTTP_STATUS_200);
    append_string(response_string, cached_entry);
    
    for (int i=0; i< resp->number_of_headers; i++)
    {
        http_header header = resp->headers[i];
        append_string(response_string, &header.name);
        APPENDSTRING(response_string, ": ");
        append_string(response_string, &header.value);
        APPENDSTRING(response_string, CRLF);
    }
    
    /* Add the body */
    APPENDSTRING(response_string, "Content-Length: ");
    
    hw_string content_length;
    content_length.value = itoa(resp->body.length + 3, 10);
    content_length.length = strlen(content_length.value);
    append_string(response_string, &content_length);
    APPENDSTRING(response_string, CRLF CRLF);
    
    append_string(response_string, &resp->body);
    APPENDSTRING(response_string, CRLF);
    return response_string;
}
