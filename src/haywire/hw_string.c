#include <stdio.h>
#include <string.h>
#include "haywire.h"

void append_string(hw_string* destination, hw_string* source)
{
    void* location = (char*)destination->value + destination->length;
    memcpy(location, source->value, source->length);
    destination->length += source->length;
}
