#include "animator.h"
#include "sglthing.h"
#include "graphic.h"
#include "io.h"

struct assimp_node_data
{
    mat4 transformation;
    char name[64];
    
    int children_count;
    struct assimp_node_data* children[128];
};

void animation_read_hierarchy(struct assimp_node_data* dest, const struct aiNode* src)
{
    ASSERT(src);

    strncpy(dest->name,src->mName.data,64);
    ASSIMP_TO_GLM(src->mTransformation, dest->transformation);
    dest->children_count = 0;
    for (int i = 0; i < src->mNumChildren; i++)
    {
        struct assimp_node_data* new_data = (struct assimp_node_data*)malloc(sizeof(struct assimp_node_data));
        animation_read_hierarchy(new_data, src->mChildren[i]);
        dest->children[i] = new_data;
        dest->children_count++;
    }
}

void animation_get_bone_info(struct animation* anim, char* name, struct model_bone_info* info_out)
{
    for(int i = 0; i < anim->bone_infos; i++)
    {
        if(strncmp(anim->bone_info[i].name,name,64)==0)
        {
            *info_out = anim->bone_info[i];
            return;
        }
    }
    info_out->id = -1;
}

void animation_get_bones(struct animation* anim, struct aiAnimation* anim_2, struct mesh* model)
{
    int size = anim_2->mNumChannels;
    int bones_not_found = 0;

    for(int i = 0; i < size; i++)
    {
        struct aiNodeAnim* anim_3 = anim_2->mChannels[i];
        char* node_name = anim_3->mNodeName.data;

        struct model_bone_info _bone_info;
        model_find_bone_info(model, node_name, &_bone_info);
        if(_bone_info.id == -1)
        {
            printf("sglthing: adding xtra bone %s (%i)\n", node_name, model->bone_infos);
            strncpy(model->bone_info[model->bone_infos].name, node_name, 64);
            model->bone_info[model->bone_infos].id = model->bone_infos;
            model->bone_infos++;
            _bone_info.id = model->bone_info[model->bone_infos].id;
            bones_not_found++;
        }

        struct bone bone;
        bone_get(&bone, anim_3->mNodeName.data, _bone_info.id, anim_3);
        g_array_append_vals(anim->bones, &bone, 1);
    }

    anim->bone_info = model->bone_info;
    anim->bone_infos = model->bone_infos;

    if(bones_not_found)
        printf("sglthing: WARN: %i bones not found\n", bones_not_found);
}

static void __animation_create2(struct animation* anim, struct mesh* model, int id, const struct aiScene* scene)
{
    struct aiAnimation* anim_2 = scene->mAnimations[id];
    anim->duration = anim_2->mDuration;
    anim->ticks_per_second = anim_2->mTicksPerSecond;
    anim->node = (struct assimp_node_data*)malloc(sizeof(struct assimp_node_data));
    anim->bones = g_array_new(false,true,sizeof(struct bone));
    animation_read_hierarchy(anim->node, scene->mRootNode);
    animation_get_bones(anim, anim_2, model);
}

int animation_create(char* animation_path, struct mesh* model, int id, struct animation* anim)
{
    char path[256];
    int f = file_get_path(path, 256, animation_path);
    if(f != -1)
    {
        printf("sglthing: loading anim %i from %s\n", id, animation_path);
        const struct aiScene* scene = aiImportFile(path, aiProcess_Triangulate);
        ASSERT(scene && scene->mRootNode && scene->mAnimations);
        __animation_create2(anim, model, id, scene);
        printf("sglthing: loaded anim %s duration %0.2f\n", animation_path, anim->duration/anim->ticks_per_second);
        return 0;
    }
    return -1;
}

struct bone* animation_get_bone(struct animation* anim, char* name)
{
    for(int i = 0; i < anim->bones->len; i++)
    {
        struct bone b = g_array_index(anim->bones, struct bone, i);
        if(strncmp(b.name,name,64)==0)
        {
            return &g_array_index(anim->bones, struct bone, i);
        }
    }
    return NULL;
}

void animator_create(struct animator* animate)
{
    animate->animation = NULL;
    for(int i = 0; i < 100; i++)
        glm_mat4_identity(animate->final_bone_matrices[i]);
}

void animator_update(struct animator* animate, float delta_time)
{
    animate->delta_time = delta_time;
    if(animate->animation)
    {
        animate->current_time += animate->animation->ticks_per_second * delta_time * animate->animation_speed;
        animate->current_time = fmod(animate->current_time, animate->animation->duration);
        mat4 i;
        glm_mat4_identity(i);
        animator_calc_bone_transform(animate, animate->animation->node, i);
    }
}

void animator_set_animation(struct animator* animate, struct animation* anim)
{
    animate->animation = anim;
    animate->current_time = 0.f;
    animate->animation_speed = 1.f;
}

void animator_calc_bone_transform(struct animator* animate, struct assimp_node_data* node, mat4 parent_transform)
{
    char* node_name = node->name;
    mat4 node_transformation;
    glm_mat4_copy(node->transformation, node_transformation);
    struct bone* bone = animation_get_bone(animate->animation, node_name);
    if(bone)
    {
        bone_update(bone, animate->current_time);
        glm_mat4_copy(bone->local_transform, node_transformation);
    }
    mat4 global_transform;
    glm_mat4_mul(parent_transform, node_transformation, global_transform);
    struct model_bone_info info;
    animation_get_bone_info(animate->animation, node_name, &info); 
    if(info.id != -1)
        glm_mat4_mul(global_transform, info.offset, animate->final_bone_matrices[info.id]);
    for (int i = 0; i < node->children_count; i++)
        animator_calc_bone_transform(animate,node->children[i], global_transform);
}

void animator_set_bone_uniform_matrices(struct animator* animate, int shader_program)
{
    if(animate->animation)
    {
        sglc(glUseProgram(shader_program));
        for(int i = 0; i < 100; i++)
        {
            char uniform_name[64];
            snprintf(uniform_name, 64, "bone_matrices[%i]", i);
            int unf = glGetUniformLocation(shader_program, uniform_name);
            ASSERT(glGetError()==0);
            if(unf)        
                sglc(glUniformMatrix4fv(unf, 1, GL_FALSE, animate->final_bone_matrices[i][0]));
        }
    }
}

int animation_bundle_create(char* animation_path, struct mesh* model, struct animation_bundle* anim_bundle)
{
    char path[256];
    int f = file_get_path(path, 256, animation_path);
    if(f != -1)
    {
        anim_bundle->animations = g_array_new(false, true, sizeof(struct animation));
        const struct aiScene* scene = aiImportFile(path, aiProcess_Triangulate);
        ASSERT(scene && scene->mRootNode && scene->mAnimations);
        for(int i = 0; i < scene->mNumAnimations; i++)
        {
            struct animation anim;
            __animation_create2(&anim, model, i, scene);
            g_array_append_vals(anim_bundle->animations, &anim, 1);
        }
        printf("sglthing: loaded anim %s count %i\n", animation_path, scene->mNumAnimations);
        return 0;
    }
    return -1;
}

struct animation* animation_bundle_get(struct animation_bundle* anim_bundle, int id)
{
    return &g_array_index(anim_bundle->animations, struct animation, id);
}