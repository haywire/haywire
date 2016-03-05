#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "haywire.h"

hw_string* create_string(const char* value)
{
    int length = strlen(value);
    hw_string* str = malloc(sizeof(hw_string));
    str->value = malloc(length);
    memcpy(str->value, value, length);
    str->length = length;
    return str;
}

hw_string* hw_strdup(hw_string* tocopy)
{
    hw_string* str = malloc(sizeof(hw_string));
    str->value = tocopy->value;
    str->length = tocopy->length;
    return str;
}

int hw_strcmp(hw_string* a, hw_string* b) {
    int ret;
    
    if (a->length > b->length) {
        ret = strncmp(a->value, b->value, b->length);
        if (!ret) {
            ret = 1;
        }
    } else if (a->length == b->length) {
        ret = strncmp(a->value, b->value, a->length);
    } else {
        ret = strncmp(a->value, b->value, a->length);
        if (!ret) {
            ret = -1;
        }
    }
    
    return ret;
}

void append_string(hw_string* destination, hw_string* source)
{
    void* location = (void*) (destination->value + destination->length);
    memcpy(location, source->value, source->length);
    destination->length += source->length;
}

/* Added because of http://stackoverflow.com/questions/8359966/strdup-returning-address-out-of-bounds */
char* dupstr(const char *s)
{
    char* result = malloc(strlen(s) + 1);
    if (result != NULL)
    {
        strcpy(result, s);
    }
    return result;
}

void string_from_int(hw_string* str, int val, int base)
{
	static char buf[32] = {0};
	int i = 30;
	int length = 0;
    
	for(; val && i ; --i, val /= base)
    {
		buf[i] = "0123456789abcdef"[val % base];
        length++;
    }
    
    str->value = &buf[i+1];
    str->length = length;
}