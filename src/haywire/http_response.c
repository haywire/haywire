#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "http_response.h"
#include "http_response_cache.h"
#include "haywire.h"
#include "trie/khash.h"
#include "helpers.h"

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
    free(resp->status_code);
    free(resp->body);
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
    resp->status_code = status_code;
}

void hw_set_response_header(hw_http_response* response, hw_string* name, hw_string* value)
{
    http_response* resp = (http_response*)response;
    http_header* header = &resp->headers[resp->number_of_headers];
    header->name = name;
    header->value = value;
    resp->headers[resp->number_of_headers] = *header;
    resp->number_of_headers++;
}

void hw_set_body(hw_http_response* response, hw_string* body)
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
    char* append_buffer = buffer;
    hw_string* cached_entry = get_cached_request(HTTP_STATUS_200);
    
    /*
    strcat(buffer, "HTTP/");
    strcat(buffer, itoa(resp->http_major, 10));
    strcat(buffer, ".");
    strcat(buffer, itoa(resp->http_minor, 10));
    strcat(buffer, " ");
    strcat(buffer, resp->status_code);
    strcat(buffer, CRLF);
    */
    
    //buff = memcpy(buffer, cached_entry->value, cached_entry->length);
    //append_buffer = (char*)memcpy(append_buffer, cached_entry->value, cached_entry->length) + cached_entry->length;
    //buffer[len] = '\0';
    append_buffer = memcpy_append(append_buffer, cached_entry->value, cached_entry->length);
    
    for (int i=0; i< resp->number_of_headers; i++)
    {
        /*
        strcat(buffer, resp->headers[i].name);
        strcat(buffer, ": ");
        strcat(buffer, resp->headers[i].value);
        strcat(buffer, CRLF);
        */
        
        /*
        append_buffer = (char*)memcpy(append_buffer, resp->headers[i].name->value, resp->headers[i].name->length) + resp->headers[i].name->length;
        append_buffer = (char*)memcpy(append_buffer, ": ", 2) + 2;
        append_buffer = (char*)memcpy(append_buffer, resp->headers[i].value->value, resp->headers[i].value->length) + resp->headers[i].value->length;
        append_buffer = (char*)memcpy(append_buffer, CRLF, 2) + 2;
        */
        
        append_buffer = memcpy_append(append_buffer, resp->headers[i].name->value, resp->headers[i].name->length);
        append_buffer = memcpy_append(append_buffer, ": ", 2);
        append_buffer = memcpy_append(append_buffer, resp->headers[i].value->value, resp->headers[i].value->length);
        append_buffer = memcpy_append(append_buffer, CRLF, 2);
    }
    
    /* Add the body */
    
    /*
    strcat(buffer, "Content-Length: ");
    strcat(buffer, itoa(body_len + 3, 10));
    strcat(buffer, CRLF CRLF);
    strcat(buffer, resp->body);
    strcat(buffer, CRLF);
    */
    
    append_buffer = memcpy_append(append_buffer, "Content-Length: ", 16);
    char* content_length = itoa(resp->body->length + 3, 10);
    size_t content_length_size = strlen(content_length);
    append_buffer = memcpy_append(append_buffer, content_length, content_length_size);
    append_buffer = memcpy_append(append_buffer, CRLF CRLF, 4);
    append_buffer = memcpy_append(append_buffer, resp->body->value, resp->body->length);
    append_buffer = memcpy_append(append_buffer, CRLF, 2);
    return buffer;
}
