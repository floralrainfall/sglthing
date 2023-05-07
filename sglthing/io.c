#include "io.h"
#include <string.h>

struct filesystem_archive archives[64];
int archives_loaded = 0;

int file_get_path(char* dest, int n, char* resource_path)
{    
    {
        FILE* early_test = fopen(resource_path, "r");
        if(early_test)
        {
            fclose(early_test);
            strncpy(dest, resource_path, n);
            return 1;
        }        
    }
    for(int i = archives_loaded; i >= 0; i--)
    {
        char path[256];
        snprintf(path,256,"%s/%s",archives[i].directory,resource_path);
        FILE* test = fopen(path, "r");
        if(test)
        {
            fclose(test);
            strncpy(dest, path, n);
            return i;
        }
    }
    strncpy(dest, resource_path, n);
    return -1;
}

FILE* file_open(char* resource_path, char* modes)
{
    {
        FILE* early_test = fopen(resource_path, "r");
        if(early_test)
            return early_test;
    }
    for(int i = 0; i < archives_loaded; i++)
    {
        char path[256];
        snprintf(path,256,"%s/%s",archives[i].directory,resource_path);
        FILE* test = fopen(path, modes);

        if(test)
            return test;
    }

    return NULL;
}

void fs_add_directory(char* directory)
{
    strncpy(archives[archives_loaded].directory, directory, 64);
    archives_loaded++;
}