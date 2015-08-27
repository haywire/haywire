#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "opt.h"

static void opt_flag_add(struct opt_config *conf, struct opt_flag *flag);

static void opt_flag_free(struct opt_flag *flag);

static void opt_flag_print_usage(struct opt_flag *flag);

struct opt_config *
opt_config_init()
{
    struct opt_config *conf;

    conf = malloc(sizeof(*conf));
    assert(conf);

    conf->length = 0;
    conf->flags = NULL;
    conf->parsed = false;
    conf->usage = opt_config_print_defaults;
    conf->help = false;

    opt_flag_bool(conf, &conf->help, "help",
                  "Shows this usage information.");

    return conf;
}

void
opt_config_free(struct opt_config *conf)
{
    int32_t i;

    for (i = 0; i < conf->length; i++)
        opt_flag_free(conf->flags[i]);
    free(conf->flags);
    free(conf);
}

void
opt_args_free(struct opt_args *args)
{
    free(args);
}

void
opt_config_print_usage(struct opt_config *conf)
{
    (*conf->usage)(conf);
}

void
opt_config_print_defaults(struct opt_config *conf)
{
    int i;

    for (i = 0; i < conf->length; i++)
        opt_flag_print_usage(conf->flags[i]);
}

static void
opt_flag_add(struct opt_config *conf, struct opt_flag *flag)
{
    if (conf->flags == NULL)
        conf->flags = malloc(sizeof(*conf->flags));
    else
        conf->flags = realloc(conf->flags,
                              (conf->length + 1) * sizeof(*conf->flags));
    assert(conf->flags);
    conf->flags[conf->length++] = flag;
}

static void
opt_flag_free(struct opt_flag *flag)
{
    free(flag);
}

static void
opt_flag_print_usage(struct opt_flag *flag)
{
    switch (flag->type) {
    case OPT_BOOL:
        printf("--%s\n", flag->name);
        break;
    case OPT_INT:
        printf("--%s %d\n", flag->name, flag->defval.i);
        break;
    case OPT_DOUBLE:
        printf("--%s %f\n", flag->name, flag->defval.d);
        break;
    case OPT_STRING:
        printf("--%s %s\n", flag->name, flag->defval.s);
        break;
    default:
        assert(false);
        break;
    }
    printf("\t%s\n", flag->usage);
}

void
opt_flag_bool(struct opt_config *conf,
              bool *storage, char *name, char *usage)
{
    struct opt_flag *flag;

    flag = malloc(sizeof(*flag));
    assert(flag);
    flag->parsed = false;
    flag->type = OPT_BOOL;
    flag->name = name;
    flag->usage = usage;
    flag->storage.b = storage;
    flag->defval.b = false;

    opt_flag_add(conf, flag);
}

void
opt_flag_int(struct opt_config *conf,
             int32_t *storage, char *name, int32_t defval, char *usage)
{
    struct opt_flag *flag;

    flag = malloc(sizeof(*flag));
    assert(flag);
    flag->parsed = false;
    flag->type = OPT_INT;
    flag->name = name;
    flag->usage = usage;
    flag->storage.i = storage;
    flag->defval.i = defval;

    opt_flag_add(conf, flag);
}

void
opt_flag_double(struct opt_config *conf,
                double *storage, char *name, double defval, char *usage)
{
    struct opt_flag *flag;

    flag = malloc(sizeof(*flag));
    assert(flag);
    flag->parsed = false;
    flag->type = OPT_DOUBLE;
    flag->name = name;
    flag->usage = usage;
    flag->storage.d = storage;
    flag->defval.d = defval;

    opt_flag_add(conf, flag);
}

void
opt_flag_string(struct opt_config *conf,
                char **storage, char *name, char *defval, char *usage)
{
    struct opt_flag *flag;

    flag = malloc(sizeof(*flag));
    assert(flag);
    flag->parsed = false;
    flag->type = OPT_STRING;
    flag->name = name;
    flag->usage = usage;
    flag->storage.s = storage;
    flag->defval.s = defval;

    opt_flag_add(conf, flag);
}
