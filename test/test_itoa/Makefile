CFLAGS?=-std=c99 -I../../include -Wall -Wextra -Wno-unused-parameter -g

test: test_itoa
	./$^
.PHONY: test

test_itoa: test_itoa.o ../../src/haywire/http_response.o
