#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "http_response.h"
#include "http_response_cache.h"
#include "haywire.h"
#include "khash.h"
#include "hw_string.h"

#define CRLF "\r\n"
KHASH_MAP_INIT_STR(string_hashmap, char*)

hw_http_response hw_create_http_response(http_connection* connection)
{
    http_response* response = malloc(sizeof(http_response));
    response->connection = connection;
    response->http_major = 1;
    response->http_minor = 1;
    response->body.value = NULL;
    response->body.length = 0;
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

hw_string* create_response_buffer(hw_http_response* response)
{
    http_response* resp = (http_response*)response;
    hw_string* response_string = malloc(sizeof(hw_string));
    hw_string* cached_entry = get_cached_request(resp->status_code.value);
    hw_string content_length;

    int i = 0;

    response_string->value = calloc(1024, 1);
    response_string->length = 0;
    append_string(response_string, cached_entry);
    
    for (i=0; i< resp->number_of_headers; i++)
    {
        http_header header = resp->headers[i];
        append_string(response_string, &header.name);
        APPENDSTRING(response_string, ": ");
        append_string(response_string, &header.value);
        APPENDSTRING(response_string, CRLF);
    }
    
    /* Add the body */
    APPENDSTRING(response_string, "Content-Length: ");
    
    string_from_int(&content_length, resp->body.length + 3, 10);
    append_string(response_string, &content_length);
    APPENDSTRING(response_string, CRLF CRLF);
    
    if (resp->body.length > 0)
    {
        append_string(response_string, &resp->body);
    }
    APPENDSTRING(response_string, CRLF);
    return response_string;
}
