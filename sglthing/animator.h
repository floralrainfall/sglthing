#ifndef ANIMATOR_H
#define ANIMATOR_H
#include <cglm/cglm.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glib.h>
#include "model.h"
#include "bone.h"

struct assimp_node_data;

struct animation {
    float duration;
    int ticks_per_second;
    GArray* bones;

    struct model_bone_info* bone_info;
    int bone_infos;

    struct assimp_node_data* node;
};

struct animator {
    float current_time;
    float delta_time;
    float animation_speed;
    mat4 final_bone_matrices[100];
    struct animation* animation;
};

void animator_create(struct animator* animate);
void animator_update(struct animator* animate, float delta_time);
void animator_set_animation(struct animator* animate, struct animation* anim);
void animator_calc_bone_transform(struct animator* animate, struct assimp_node_data* node, mat4 parent_transform);
void animator_set_bone_uniform_matrices(struct animator* animate, int shader_program);

int animation_create(char* animation_path, struct mesh* model, struct animation* anim);
void animation_get_bone_info(struct animation* anim, char* name, struct model_bone_info* info_out);
struct bone* animation_get_bone(struct animation* anim, char* name);

#endif