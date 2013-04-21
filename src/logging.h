#define LEVEL_TRACE 0
#define LEVEL_DEBUG 1
#define LEVEL_INFO  2
#define LEVEL_WARN  3
#define LEVEL_ERROR 4
#define LEVEL_FATAL 5
#define LEVEL_NONE  6

#define CONSOLE printf

#define LOG(s, ...) LOGTO(s "\n", __VA_ARGS__)
#define LOG_LEVEL(LEVEL, s, ...) LOGTO(s "\n", __VA_ARGS__)

#if LOGGING_LEVEL <= LEVEL_TRACE
#define TRACE(s, ...) LOG_LEVEL(LEVEL_TRACE, s, __VA_ARGS__)
#define ONTRACE(s) s
#else
#define TRACE(...)
#define ONTRACE(s)
#endif

#if LOGGING_LEVEL <= LEVEL_DEBUG
#define DEBUG(s, ...) LOG_LEVEL(LEVEL_DEBUG, s, __VA_ARGS__)
#define ONDEBUG(s) s
#else
#define DEBUG(...)
#define ONDEBUG(s)
#endif

#if LOGGING_LEVEL <= LEVEL_INFO
#define INFO(s, ...) LOG_LEVEL(LEVEL_INFO, s, __VA_ARGS__)
#define ONINFO(s) s
#else
#define INFO(...)
#define ONINFO(s)
#endif

#if LOGGING_LEVEL <= LEVEL_WARN
#define WARN(s, ...) LOG_LEVEL(LEVEL_WARN, s, __VA_ARGS__)
#define ONWARN(s) s
#else
#define WARN(...)
#define ONWARN(s)
#endif

#if LOGGING_LEVEL <= LEVEL_ERROR
#define ERR(s, ...) LOG_LEVEL(LEVEL_ERROR, s, __VA_ARGS__)
#define ONERROR(s) s
#else
#define ERR(...)
#define ONERROR(s)
#endif

#if LOGGING_LEVEL <= LEVEL_FATAL
#define FATAL(s, ...) LOG_LEVEL(LEVEL_FATAL, s, __VA_ARGS__)
#define ONFATAL(s) s
#else
#define FATAL(...)
#define ONFATAL(s)
#endif