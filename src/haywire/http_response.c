#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "haywire.h"
#include "http_server.h"
#include "trie/radix.h"

#define CRLF "\r\n"

http_response* hw_create_response()
{
    http_response *response = (http_response *)malloc(sizeof(http_response));
    response->buffer = malloc(sizeof(char) * 1024);
    /* response->headers = rxt_init(); */
    return response;
}

void hw_free_response(http_response *response)
{
    if (response->buffer != NULL)
    {
        free(response->buffer);
    }
    if (response->body != NULL)
    {
        free(response->body);
    }
    /* rxt_free(response->headers); */
    free(response);
}
int hw_set_response_header(http_response *response, char *key, void *value)
{
    return rxt_put(key, value, (rxt_node *)response->headers);
}

int hw_send_response(http_response *response)
{
    /* TODO */
    return 0;
}
