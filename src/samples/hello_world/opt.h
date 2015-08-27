#ifndef __LIBOPT_OPT_H__
#define __LIBOPT_OPT_H__

#include <stdbool.h>
#include <stdint.h>

/* The design of this package is greatly inspired by Go's standard library
 * package 'flag'. It doesn't support complex argument parsing, but it's simple
 * to use. */

struct opt_config {
    /* Array of all flags specified by the user. */
    struct opt_flag **flags;

    /* The total number of flags specified. */
    int32_t length;

    /* Whether this flag configuration has been parsed or not. */
    bool parsed;

    /* Whether the '--help' flag was used. */
    bool help;

    /* A 'usage' function that is called whenever opt_config_print_usage
     * is called. It can be replaced with your own usage function. */
    void (*usage)(struct opt_config *conf);
};

/* A value with type 'struct opt_args *' is returned from 'opt_config_parse'
 * will all arguments that are not flags.
 *
 * N.B. All flags precede the first argument. As soon as the first non-flag
 * argument is found, all subsequent arguments are automatically non-flags. */
struct opt_args {
    /* Number of arguments proceeding flags. */
    int nargs;

    /* Array of all arguments. This is a simple pointer offset from the
     * original 'argv' passed into the program. */
    char **args;
};

/* Initialize a command line argument parsing configuration. The value
 * returned can be used to specify the different kinds of flags expected. */
struct opt_config *
opt_config_init();

void
opt_config_free(struct opt_config *conf);

void
opt_args_free(struct opt_args *args);

/* Call this after all flags have been specified. It will parse the arguments
 * passed in through 'main' ('argc' and 'argv'), and return all non-flag
 * arguments.
 *
 * N.B. All flags precede the first argument. As soon as the first non-flag
 * argument is found, all subsequent arguments are automatically non-flags. */
struct opt_args *
opt_config_parse(struct opt_config *conf, int argc, char **argv);

/* Prints the usage information by calling 'conf->usage()', which is by
 * default set to 'opt_config_print_defaults'. */
void
opt_config_print_usage(struct opt_config *conf);

/* Prints a kind-of-nicely formatted list of all flags, their default values
 * and the usage information for each flag. */
void
opt_config_print_defaults(struct opt_config *conf);

/* All possible argument types. */
enum opt_flag_types {
    OPT_BOOL,
    OPT_INT,
    OPT_DOUBLE,
    OPT_STRING
};

/* An opaque type representing a flag specified by the user. */
struct opt_flag {
    /* Whether this flag has been parsed or not. After parsing is complete,
     * all flags that have this value set to 'false' will automatically
     * have 'storage' loaded with 'defval'. */
    bool parsed;

    /* The type of this flag. */
    enum opt_flag_types type;

    /* The name of this flag, not including the '--' prefix. */
    char *name;

    /* Usage information for this flag. */
    char *usage;

    /* A pointer to where the value passed in by the user (or the default
     * value of the flag isn't specified) will be stored. */
    union {
        bool *b;
        int32_t *i;
        double *d;
        char **s;
    } storage;

    /* The default value of the flag. For boolean flags, this is always
     * 'false'. */
    union {
        bool b;
        int32_t i;
        double d;
        char *s;
    } defval;
};

/* Add a new bool flag. All bool flags have a default value of 'false'. */
void
opt_flag_bool(struct opt_config *conf,
              bool *storage, char *name, char *usage);

/* Add a new int flag. */
void
opt_flag_int(struct opt_config *conf,
             int32_t *storage, char *name, int32_t defval, char *usage);

/* Add a new double flag. */
void
opt_flag_double(struct opt_config *conf,
                double *storage, char *name, double defval, char *usage);

/* Add a new string flag. */
void
opt_flag_string(struct opt_config *conf,
                char **storage, char *name, char *defval, char *usage);

#endif
