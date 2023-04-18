#include "texture.h"
#include <glad/glad.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "graphic.h"
#include "io.h"

#define MAX_TEXTURES 512

struct texture
{
    char file[64];
    int texture_handle;
};

struct texture textures[MAX_TEXTURES];
int loaded_textures = 0;

struct texture_load_info load_texture(char* file)
{   
    char path[256];
    file_get_path(path, 256, file);
    int existing_texture = get_texture2(path);
    if(existing_texture)
        return;
    int image_width;
    int image_height;
    int image_channels;
    char* data = (char*)stbi_load(path, &image_width, &image_height, &image_channels, 4);
    struct texture_load_info tex_load_info;
    if(data)
    {
        struct texture* texture = &textures[loaded_textures];
        loaded_textures++;

        tex_load_info.texture_width = image_width;
        tex_load_info.texture_height = image_height;

        sglc(glGenTextures(1, &texture->texture_handle));
        sglc(glBindTexture(GL_TEXTURE_2D, texture->texture_handle));
        sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
        sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

        sglc(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
        sglc(glGenerateMipmap(GL_TEXTURE_2D));
        strncpy(texture->file,file,64);
        printf("sglthing: loaded texture %s, %ix%i, %i channels (%i)\n", file, image_width, image_height, image_channels, texture->texture_handle);
        stbi_image_free(data);
    }
    else
    {
        printf("sglthing: could not load texture %s\n", file);
    }
    return tex_load_info;
}

int get_texture(char* file)
{
    for(int i = 0; i < MAX_TEXTURES; i++)
    {
        if(strncmp(textures[i].file,file,64)==0)
        {
            return textures[i].texture_handle;
        }
    }
    return get_texture2("pink_checkerboard.png");
}

int get_texture2(char* file)
{
    for(int i = 0; i < MAX_TEXTURES; i++)
    {
        if(strncmp(textures[i].file,file,64)==0)
        {
            return textures[i].texture_handle;
        }
    }
    return 0;
}