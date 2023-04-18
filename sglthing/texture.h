#ifndef TEXTURE_H
#define TEXTURE_H

struct texture_load_info
{
    int texture_width;
    int texture_height;
};

struct texture_load_info load_texture(char* file);
// void load_textured(void* data, int length);
int get_texture(char* file);
int get_texture2(char* file);

#endif