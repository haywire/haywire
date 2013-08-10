#include "configuration.h"
#include "../hw_string.h"
#include "../khash.h"
#include "ini.h"

KHASH_MAP_INIT_STR(route_hashes, char*)

int configuration_handler(void* user, const char* section, const char* name, const char* value)
{
    configuration* config = (configuration*)user;
    
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("http", "listen_address"))
    {
        config->http_listen_address = dupstr(value);
    }
    else if (MATCH("http", "listen_port"))
    {
        config->http_listen_port = atoi(value);
    }
    else
    {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

configuration* load_configuration(const char* filename)
{
    configuration* config = malloc(sizeof(configuration));
    if (ini_parse(filename, configuration_handler, config) < 0)
    {
        printf("Can't load configuration\n");
        return NULL;
    }
    return config;
}
