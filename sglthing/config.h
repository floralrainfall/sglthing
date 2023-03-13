#ifndef CONFIG_H
#define CONFIG_H

struct config_value {
    char name[32];

    enum {
        VT_STRING,
        VT_NUMBER
    } type;

    float number_value;
    char string_value[64];
    int string_length;
};

struct config_file
{
    struct config_value config[128];
    int config_values;
};

void config_load(struct config_file* config, char* file);
void config_add(struct config_file* config, char** argv, int argc);
void config_add_entry(struct config_file* config, char* a, char* b);

struct config_value* config_value_get(struct config_file* config, char* name);
char* config_string_get(struct config_file* config, char* name);
float config_number_get(struct config_file* config, char* name);

#endif