#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <haywire.h>
#include "route_compare_method.h"

typedef struct hw_route_token_st {
    hw_string string;
    int start;
} hw_route_token;

inline void update_route_token(hw_route_token* result, hw_string* value, int length, int start) {
    result->string.value = value;
    result->string.length = length;
    result->start = start;
}

void hw_route_next_token(hw_string* url, int start, hw_route_token* result) {
    // when url is /
    if (url->value[start] == '/' && url->length == 1){
        update_route_token(result, url->value + start, 1, start);
    }
    else if (url->value[start] == '?') {
        update_route_token(result, NULL, 0, -1);
    }
    else {
        while (start < url->length && url->value[start] == '/') {
            start++;
        }

        int end = start;

        while (end < url->length && url->value[end] != '/' && url->value[end] != '?') {
            end++;
        }

        if (end != start) {
            update_route_token(result, url->value + start, end - start - 1, start);
        }
        else {
            update_route_token(result, NULL, 0, -1);
        }
    }
}

int hw_route_compare_method(hw_string* url, char* route)
{
    int equal = 0;
    int match = 0;

    // TODO route should probably be a hw_string* too
    hw_string hw_route;
    hw_route.value = route;
    hw_route.length = strlen(route);

    hw_route_token route_token;
    hw_route_token request_token;

    hw_route_next_token(url, 0, &request_token);
    hw_route_next_token(&hw_route, 0, &route_token);

    while (route_token.string.value != NULL && request_token.string.value != NULL)
    {
        if (route_token.string.value[0] == '*') {
            // wildcard support: any route fragment marked with '*' matches the corresponding url fragment
            equal = 1;
        }
        else
        {
            match = hw_strcmp(&route_token.string, &request_token.string);
            if (!match)
            {
                equal = 1;
            }
            else
            {
                equal = 0;
                break;
            }
        }

        hw_route_next_token(url, request_token.start + request_token.string.length + 1, &request_token);
        hw_route_next_token(&hw_route, route_token.start + route_token.string.length + 1, &route_token);
    }

    if (!equal)
    {
        match = hw_strcmp(url, &hw_route);
        if (!match)
        {
            equal = 1;
        }
    }

    if ((route_token.string.value == NULL && request_token.string.value != NULL) || (route_token.string.value != NULL && request_token.string.value == NULL))
    {
        equal = 0;
    }

    return equal;
}
