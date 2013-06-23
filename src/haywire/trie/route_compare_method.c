#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "route_compare_method.h"
#include "radix.h"

#if defined(_WIN32) || defined(_WIN64) 
    #define strcasecmp _stricmp 
    #define strncasecmp _strnicmp 
    #define strtok_r strtok_s
#endif

static char route_key[2048];

int hw_route_compare_method(char *url, char* route)
{
    int equal = 0;
    char *route_token;
    char *route_token_ptr;
    char *request_token;
    char *request_token_ptr;
    char prefix;
    
    strcpy(route_key, route);
    
    route_token = strtok_r(route_key, "/", &route_token_ptr);
    request_token = strtok_r(url, "/", &request_token_ptr);
    
    while (route_token != NULL && request_token != NULL)
    {
        if (route_token == NULL || request_token == NULL)
            break;
        
        prefix = *route_token;
        if (prefix == '$')
        {
            // TODO: Do request URL variable substitution.
        }
        else
        {
            if (!strncasecmp(route_token, request_token, 0))
            {
                equal = 1;
                //break;
            }
            else
            {
                equal = 0;
                break;
            }
        }
        
        //printf ("ROUTE:%s\tREQUEST:%s\n", route_token, request_token);
        route_token = strtok_r(NULL, "/", &route_token_ptr);
        request_token = strtok_r(NULL, "/", &request_token_ptr);
    }
    
    if (!equal)
    {
        if (!strncasecmp(route_key, url, 0))
        {
            equal = 1;
        }
    }
    
    if ((route_token == NULL && request_token != NULL) || (route_token != NULL && request_token == NULL))
    {
        equal = 0;
    }
    return equal;
}