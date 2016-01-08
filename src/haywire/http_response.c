#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
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

void hw_add_response_header(hw_http_response* response, hw_string* name, hw_string* value)
{
    http_response* resp = (http_response*)response;
    http_header* header = &resp->headers[resp->number_of_headers];
    header->name = *name;
    header->value = *value;
    resp->headers[resp->number_of_headers] = *header;
    resp->number_of_headers++;
}

void hw_set_response_header(hw_http_response* response, hw_string* name, hw_string* value)
{
    http_response* resp = (http_response*)response;
    int i = 0;

    for (; i < resp->number_of_headers; i++)
    {
        http_header* header = &resp->headers[i];
        if (string_equals(&(header->name), name))
        {
            header->value = *value;
        }
    }

    hw_add_response_header(response, name, value);
}

void hw_set_body(hw_http_response* response, hw_string* body)
{
    http_response* resp = (http_response*)response;
    resp->body = *body;
}

int num_chars(int n) {
    int r = 1;
    if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
    while (n > 9) {
        n /= 10;
        r++;
    }
    return r;
}

hw_string* create_response_buffer(hw_http_response* response)
{
    http_response* resp = (http_response*)response;
    hw_string* response_string = malloc(sizeof(hw_string));
    hw_string* cached_entry = get_cached_request(resp->status_code.value);
    hw_string content_length;

    int i = 0;

    char length_header[] = "Content-Length: ";
    int line_sep_size = sizeof(CRLF);

    int header_buffer_incr = 512;
    int body_size = resp->body.length + line_sep_size;
    int header_size_remaining = header_buffer_incr;
    int response_size = header_size_remaining + sizeof(length_header) + num_chars(resp->body.length) + 2 * line_sep_size + body_size + line_sep_size;

    response_string->value = malloc(response_size);

    response_string->length = 0;
    append_string(response_string, cached_entry);
    
    for (i=0; i< resp->number_of_headers; i++)
    {
        http_header header = resp->headers[i];

        header_size_remaining -= header.name.length + 2 + header.value.length + line_sep_size;
        if (header_size_remaining < 0) {
            header_size_remaining += header_buffer_incr * ((-header_size_remaining/header_buffer_incr) + 1);
            response_size += header_size_remaining;
            response_string->value = realloc(response_string->value, response_size);
        }

        append_string(response_string, &header.name);
        APPENDSTRING(response_string, ": ");
        append_string(response_string, &header.value);
        APPENDSTRING(response_string, CRLF);
    }
    
    /* Add the body */
    APPENDSTRING(response_string, length_header);

    string_from_int(&content_length, body_size, 10);
    append_string(response_string, &content_length);
    APPENDSTRING(response_string, CRLF CRLF);
    
    if (resp->body.length > 0)
    {
        append_string(response_string, &resp->body);
    }
    APPENDSTRING(response_string, CRLF);
    response_string->value[response_string->length] = '\0';
    return response_string;
}
