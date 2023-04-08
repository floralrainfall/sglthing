#ifndef MODEL_H
#define MODEL_H
#include <cglm/cglm.h>

#define MAX_BONE_WEIGHTS 4
void load_model(char* file);
struct model* get_model(char* file);

struct model_vertex
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec2 tex_coords;
    int bone_ids[MAX_BONE_WEIGHTS];
    float weights[MAX_BONE_WEIGHTS];
};

struct model_bone_info
{
    char name[64];
    mat4 offset;
    int id;
};

struct mesh
{
    struct model_vertex* vtx_data;
    int vtx_data_count;

    int vertex_array;
    int element_buffer;
    int vertex_buffer;
    int element_count;
    
    int diffuse_texture[3];
    int diffuse_textures;
    int specular_texture[3];
    int specular_textures;

    struct model_bone_info bone_info[128];
    int bone_infos;
    int bone_counter;

    char* path;
};

struct model
{
    char name[64];
    struct mesh meshes[64];
    int mesh_count;
    struct aiScene *scene;

    char* path;
};

void model_find_bone_info(struct mesh* mesh, char* name, struct model_bone_info* info_out);

#endif