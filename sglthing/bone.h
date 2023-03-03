#ifndef BONE_H
#define BONE_H
#include <glib.h>
#include <cglm/cglm.h>
#include <assimp/scene.h>

struct key_translate
{
    vec3 position;
    float timestamp;
};

struct key_rotate
{
    vec4 quat;
    float timestamp;
};

struct key_scale
{
    vec3 scale;
    float timestamp;
};

struct bone {
    GArray* key_translations;
    int num_key_translations;
    GArray* key_rotations;
    int num_key_rotations;
    GArray* key_scales;
    int num_key_scales;

    mat4 local_transform;
    int id;
};

void bone_get(struct bone* bone, char* name, int id, const struct aiNodeAnim* channel);
void bone_free(struct bone* bone);
void bone_update(struct bone* bone, float animation_time);
int bone_get_translation_idx(struct bone* bone, float animation_time);
int bone_get_rotation_idx(struct bone* bone, float animation_time);
int bone_get_scale_idx(struct bone* bone, float animation_time);

#endif