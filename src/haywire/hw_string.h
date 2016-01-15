#pragma once

hw_string* create_string(const char* value);
int hw_strcmp(hw_string* a, hw_string* b);
hw_string* hw_strdup(hw_string* tocopy);
void append_string(hw_string* destination, hw_string* source);
char* dupstr(const char *s);
void string_from_int(hw_string* str, int val, int base);
int hw_strcmp(hw_string* a, hw_string* b);
