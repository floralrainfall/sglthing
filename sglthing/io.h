#ifndef IO_H
#define IO_H

#include <stdio.h>

FILE* file_open(char* resource_path, char* modes);
int file_get_path(char* dest, int n, char* resource_path);
void fs_add_directory(char* directory);

struct filesystem_archive
{
    enum {FA_DIRECTORY, FA_ZIP} archive_type;
    char directory[64];
};

extern struct filesystem_archive archives[64];
extern int archives_loaded;

#endif