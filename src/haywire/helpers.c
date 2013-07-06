#include <stdio.h>
#include <string.h>

void* memcpy_append(void* destination, const void* source, size_t num)
{
    memcpy(destination, source, num);
    return destination + num;
}
