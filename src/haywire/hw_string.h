#pragma once

hw_string* create_string(const char* value);
void append_string(hw_string* destination, hw_string* source);
char* dupstr(const char *s);
void string_from_int(hw_string* str, int val, int base);
int string_equals(hw_string* a, hw_string* b);
