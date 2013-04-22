#ifndef HAYWIRE_H_
#define HAYWIRE_H_

#include <stdarg.h>
#include <stdio.h>

#ifndef DEBUG
# ifdef NDEBUG
#  define DEBUG 0
# else
#  define DEBUG 1
# endif
#endif

void dbg_print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

#define DPUTS(msg) do { if (DEBUG) fputs(msg, stderr); } while (0)
#define DPRINT(x) do { if (DEBUG) dbg_print x; } while (0)

#endif /* HAYWIRE_H_ */
