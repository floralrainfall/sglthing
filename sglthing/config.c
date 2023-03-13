#include "config.h"
#include "io.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void config_load(struct config_file* config, char* file)
{
    FILE* config_file = file_open(file, "r");
    if(config_file)
    {
        printf("sglthing: loading config file %s\n", file);
        char* file_data = malloc(65535);
        memset(file_data,0,65535);
        fread(file_data, 1, 65535, config_file);
        for(int i = 0; i < 65535; i++)
            if(file_data[i] == '\n')
                file_data[i] = ' ';
        char* token = strtok(file_data, " ");
        int odd = 1;
        char val_name[32];
        while(token != NULL)
        {
            if(odd)
                strncpy(val_name,token, 32);
            else
            {
                config_add_entry(config, val_name, token);
            }
            odd = !odd;
            token = strtok(NULL, " ");
        }
        printf("sglthing: loaded %i configuration entries\n", config->config_values);
        fclose(config_file);
        free(file_data);
    }
    else
    {
        printf("sglthing: unable to load config file %s\n", file);
    }
}

void config_add(struct config_file* config, char** argv, int argc)
{
    int odd = 1;
    char val_name[32];
    for(int i = 1; i < argc; i++)
    {
        if(odd)
            strncpy(val_name,argv[i],32);
        else
        {
            config_add_entry(config, val_name, argv[i]);
        }
        odd = !odd;
    }
}

void config_add_entry(struct config_file* config, char* a, char* b)
{
    if(strncmp(a,"load_filesystem",32)==0)
    {
        fs_add_directory(b);
    }
    else
    {
        struct config_value* config_entry = config_value_get(config, a);
        if(!config_entry)
            config_entry = &config->config[config->config_values];
        strncpy(config_entry->name, a, 32);
        strncpy(config_entry->string_value, b, 64);
        int token_len = strlen(b);
        config_entry->string_length = token_len;
        
        strncpy(config_entry->string_value,b,64);
        config_entry->type = VT_STRING;
        int f_r = sscanf(b,"%f",&config_entry->number_value);
        if(f_r)
            config_entry->type = VT_NUMBER;

        config->config_values++;
        printf("sglthing: arg \t%s\t\t=%s\n",config_entry->name,config_entry->string_value);
    }
}

struct config_value* config_value_get(struct config_file* config, char* name)
{
    for(int i = 0; i < config->config_values; i++)
    {
        if(strncmp(config->config[i].name,name,32)==0)
        {
            return &config->config[i];
        }
    }
    return NULL;
}

char* config_string_get(struct config_file* config, char* name)
{
    struct config_value* v = config_value_get(config, name);
    if(!v)
        return "(null)";
    return &v->string_value[0];
}

float config_number_get(struct config_file* config, char* name)
{
    struct config_value* v = config_value_get(config, name);
    if(!v)
        return 0.0;
    return v->number_value;
}