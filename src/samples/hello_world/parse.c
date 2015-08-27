#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opt.h"

static void expect_next_arg(struct opt_config *conf, int argc, char **argv,
                            int cur, char *flag_name, char *msg);

struct opt_args *
opt_config_parse(struct opt_config *conf, int argc, char **argv)
{
    struct opt_args *args;
    char *cur;
    int i = 1; /* arg offset. don't include executable name. */
    int j = 0;
    bool flags_done = false;
    bool argmatched = false;

    assert(!conf->parsed);

    args = malloc(sizeof(*args));
    assert(args);
    args->nargs = 0;

    while (i < argc) {
        if (flags_done) {
            if (args->nargs == 0)
                args->args = argv + i;

            args->nargs++;
            i++;
            continue;
        }
        if (strlen(argv[i]) < 3 || argv[i][0] != '-' || argv[i][1] != '-') {
            flags_done = true;
            /* don't increment i: the current argument is the first non-flag */
            continue;
        }

        /* Find the flag in the config that matches the current flag. */
        cur = argv[i] + 2; /* chomp off '--' */
        argmatched = false;
        for (j = 0; j < conf->length; j++) {
            int args_consumed = 0;

            if (0 != strcmp(cur, conf->flags[j]->name))
                continue;

            argmatched = true;
            switch (conf->flags[j]->type) {
            case OPT_BOOL:
                *conf->flags[j]->storage.b = true;
                conf->flags[j]->parsed = true;
                args_consumed = 1;
                break;
            case OPT_INT:
                expect_next_arg(conf, argc, argv, i, conf->flags[j]->name,
                                "Flag '--%s' requires an integer argument.\n");
                *conf->flags[j]->storage.i = atoi(argv[i+1]);
                conf->flags[j]->parsed = true;
                args_consumed = 2;
                break;
            case OPT_DOUBLE:
                expect_next_arg(conf, argc, argv, i, conf->flags[j]->name,
                                "Flag '--%s' requires a double argument.\n");
                *conf->flags[j]->storage.d = atof(argv[i+1]);
                conf->flags[j]->parsed = true;
                args_consumed = 2;
                break;
            case OPT_STRING:
                expect_next_arg(conf, argc, argv, i, conf->flags[j]->name,
                                "Flag '--%s' requires a string argument.\n");
                *conf->flags[j]->storage.s = argv[i+1];
                conf->flags[j]->parsed = true;
                args_consumed = 2;
                break;
            default:
                assert(false);
                break;
            }

            i += args_consumed;
            break;
        }

        if (!argmatched) {
            fprintf(stderr, "Unrecognized flag '--%s'.\n", cur);
            opt_config_print_usage(conf);
            exit(1);
        }
    }

    /* Go back over every flag that hasn't been parsed, and assign
     * a default value. */
    for (i = 0; i < conf->length; i++)
        if (!conf->flags[i]->parsed) {
            switch (conf->flags[i]->type) {
            case OPT_BOOL:
                *conf->flags[i]->storage.b = conf->flags[i]->defval.b;
                break;
            case OPT_INT:
                *conf->flags[i]->storage.i = conf->flags[i]->defval.i;
                break;
            case OPT_DOUBLE:
                *conf->flags[i]->storage.d = conf->flags[i]->defval.d;
                break;
            case OPT_STRING:
                *conf->flags[i]->storage.s = conf->flags[i]->defval.s;
                break;
            default:
                assert(false);
                break;
            }
        }

    conf->parsed = true;

    /* If 'help' has been found, echo usage and exit. */
    if (conf->help) {
        opt_config_print_usage(conf);
        exit(1);
    }

    return args;
}

static void expect_next_arg(struct opt_config *conf, int argc, char **argv,
                            int cur, char *flag_name, char *msg)
{
    if (argc == cur + 1 || (argv[cur+1][0] == '-' && argv[cur+1][0] == '-')) {
        fprintf(stderr, msg, flag_name);
        opt_config_print_usage(conf);
        exit(1);
    }
}
