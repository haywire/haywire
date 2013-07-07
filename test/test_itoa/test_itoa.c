#include <stdbool.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

char* itoa(int val, int base);
static bool test_itoa(int val, int base, const char *expected);

#define INT_NIBBLES(h, g, f, e, d, c, b, a) (h##g##f##e##d##c##b##a)
#define STR_NIBBLES(h, g, f, e, d, c, b, a) #h #g #f #e #d #c #b #a

int main(int argc, char *argv[]) {
	int status = EXIT_SUCCESS;
	status |= !test_itoa(42, 10, "42");
	//2**31
	status |= !test_itoa(INT_NIBBLES(0b1000,0000,0000,0000,0000,0000,0000,0000), 2, STR_NIBBLES(1000,0000,0000,0000,0000,0000,0000,0000));

	return status;
}

static bool test_itoa(int val, int base, const char *expected) {
	const char *itoa_output = itoa(val, base);

	if (strcmp(itoa_output, expected) != 0) {
		fprintf(stderr, "itoa(%d, %d) failed: expected \"%s\"; got \"%s\"\n", val, base, expected, itoa_output);
		return false;
	}

	return true;
}
