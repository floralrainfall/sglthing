#include "animator.h"
#include "sglthing.h"
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
}

void animation_get_bones(struct animation* anim, struct aiAnimation* anim_2, struct mesh* model)
{
    int size = anim_2->mNumChannels;
    int bone_count = model->bone_infos;
    for(int i = 0; i < size; i++)
    {
        struct aiNodeAnim* anim_3 = anim_2->mChannels;
        char* node_name = anim_3->mNodeName.data;
        struct model_bone_info _bone_info;
        model_find_bone_info(model, node_name, &_bone_info);
        if(_bone_info.id == bone_count)
        {
            
        }
    }
}

int animation_create(char* animation_path, struct mesh* model, struct animation* anim)
{
    char path[256];
    int f = file_get_path(path, 256, animation_path);
    if(f)
    {
        const struct aiScene* scene = aiImportFile(path, aiProcess_Triangulate);
        ASSERT(scene && scene->mRootNode);
        struct aiAnimation* anim_2 = scene->mAnimations[0];
        anim->duration = anim_2->mDuration;
        anim->ticks_per_second = anim_2->mTicksPerSecond;
        anim->node = (struct assimp_node_data*)malloc(sizeof(struct assimp_node_data));
        animation_read_hierarchy(anim->node, scene->mRootNode);
        animation_get_bones(anim, anim_2, model);
    }
    return -1;
}

struct bone* animation_get_bone(struct animation* anim, char* name)
{

}

void animator_create(struct animator* animate)
{
    animate->animation = NULL;
}

void animator_calc_bone_mat()
{

}

void animator_update(struct animator* animate, float delta_time)
{
    animate->delta_time = delta_time;
    if(animate->animation)
    {
        animate->current_time += animate->animation->ticks_per_second * delta_time;
        animate->current_time = fmod(animate->current_time, animate->animation->duration);
        //animator_calc_bone_mat(animate->root_node, mat4(1.0f));
    }
}

void animator_set_animation(struct animator* animate, struct animation* anim)
{
    animate->animation = anim;
    animate->current_time = 0.f;
}

void animator_set_bone_uniform_matrices(struct animator* animate, int shader_program)
{

}