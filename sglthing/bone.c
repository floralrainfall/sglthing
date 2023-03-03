#include "bone.h"
#include "sglthing.h"

void bone_get(struct bone* bone, char* name, int id, const struct aiNodeAnim* channel)
{
    bone->key_translations = g_array_new(false, true, sizeof(struct key_translate));
    bone->key_rotations = g_array_new(false, true, sizeof(struct key_rotate));
    bone->key_scales = g_array_new(false, true, sizeof(struct key_scale));

    bone->num_key_translations = channel->mNumPositionKeys;
    for(int i = 0; i < channel->mNumPositionKeys; i++)
    {            
        struct aiVector3D ai_position = channel->mPositionKeys[i].mValue;
        float time_stamp = channel->mPositionKeys[i].mTime;
        struct key_translate data;
        data.position[0] = ai_position.x;
        data.position[1] = ai_position.y;
        data.position[2] = ai_position.z;
        data.timestamp = time_stamp;
        g_array_append_vals(bone->key_translations, &data, 1);
    }

    bone->num_key_rotations = channel->mNumRotationKeys;
    for(int i = 0; i < channel->mNumRotationKeys; i++)
    {            
        struct aiQuaternion ai_quaternion = channel->mRotationKeys[i].mValue;
        float time_stamp = channel->mRotationKeys[i].mTime;
        struct key_rotate data;
        data.quat[0] = ai_quaternion.x;
        data.quat[1] = ai_quaternion.y;
        data.quat[2] = ai_quaternion.z;
        data.quat[2] = ai_quaternion.w;
        data.timestamp = time_stamp;
        g_array_append_vals(bone->key_rotations, &data, 1);
    }

    bone->num_key_scales = channel->mNumScalingKeys;
    for(int i = 0; i < channel->mNumScalingKeys; i++)
    {            
        struct aiVector3D ai_scale = channel->mScalingKeys[i].mValue;
        float time_stamp = channel->mScalingKeys[i].mTime;
        struct key_scale data;
        data.scale[0] = ai_scale.x;
        data.scale[1] = ai_scale.y;
        data.scale[2] = ai_scale.z;
        data.timestamp = time_stamp;
        g_array_append_vals(bone->key_scales, &data, 1);
    }
}

void bone_free(struct bone* bone)
{
    g_array_free(bone->key_translations, TRUE);
    g_array_free(bone->key_rotations, TRUE);
    g_array_free(bone->key_scales, TRUE);
    free(bone);
}

float get_scale_factor(float last_time_stamp, float next_time_stamp, float animation_time)
{
    float scale_factor;
    float m = animation_time - last_time_stamp;
    float diff = next_time_stamp - last_time_stamp;
    scale_factor = m / diff;
    return scale_factor;
}

void bone_interp_position(struct bone* bone, mat4 out, float animation_time)
{
    mat4 t;
    if (bone->num_key_translations == 1)
    {

    }

}

void bone_interp_rotation(struct bone* bone, mat4 out, float animation_time)
{
    
}

void bone_interp_scale(struct bone* bone, mat4 out, float animation_time)
{
    
}

void bone_update(struct bone* bone, float animation_time)
{
    mat4 t, r, s, x;
    bone_interp_position(bone, t, animation_time);
    bone_interp_rotation(bone, r, animation_time);
    bone_interp_scale(bone, s, animation_time);
    glm_mat4_mul(t, r, x);
    glm_mat4_mul(x, s, x);
}

int bone_get_translation_idx(struct bone* bone, float animation_time)
{
    for(int i = 0; i < bone->num_key_translations - 1; ++i)
    {

    }
    ASSERT(0);
}

int bone_get_rotation_idx(struct bone* bone, float animation_time)
{

    ASSERT(0);
}

int bone_get_scale_idx(struct bone* bone, float animation_time)
{

    ASSERT(0);
}