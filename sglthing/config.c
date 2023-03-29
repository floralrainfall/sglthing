#include "config.h"
#include "io.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

void config_load(struct config_file* config, char* file)
{
    char rscpath[255];
    file_get_path(&rscpath, 255, file);
    if(rscpath)
    {
        config->key_file = g_key_file_new();
        GError* error = NULL; 
        if(g_key_file_load_from_file(config->key_file, rscpath, 0, &error))
        {

        }
        else
        {
            printf("sglthing: glib could not open file %s (%s)\n", rscpath, error->message);
        }
    }
    else
    {
        printf("sglthing: could not find config file %s\n", file);
    }
}

char* config_string_get(struct config_file* config, char* name)
{
    return g_key_file_get_string(config->key_file, "sglthing", name, NULL);
}

float config_number_get(struct config_file* config, char* name)
{
    return g_key_file_get_double(config->key_file, "sglthing", name, NULL);
}