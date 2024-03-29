#ifndef CONFIG_H
#define CONFIG_H

#include <glib.h>

// BUTCHERED to use glib's GKeyFile

struct config_file
{
    GKeyFile* key_file;    
};

void config_load(struct config_file* config, char* file);

//[[deprecated("Use GKeyFile rather then config_string_get")]]
char* config_string_get(struct config_file* config, char* name);
//[[deprecated("Use GKeyFile rather then config_number_get")]]
float config_number_get(struct config_file* config, char* name);

#endif