#pragma once
#include "uv.h"
#include "http_parser.h"
#include "http_connection.h"

extern int last_was_value;

void set_request_header(http_request* request, hw_string* name, hw_string* value);