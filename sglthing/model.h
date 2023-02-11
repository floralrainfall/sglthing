#ifndef MODEL_H
#define MODEL_H

void load_model(char* file);
struct model* get_model(char* file);

struct mesh
{
    int vertex_array;
    int element_buffer;
    int vertex_buffer;
    int element_count;
    
    int diffuse_texture[3];
    int diffuse_textures;
    int specular_texture[3];
    int specular_textures;
};

struct model
{
    char name[64];
    struct mesh meshes[64];
    int mesh_count;
};

#endif