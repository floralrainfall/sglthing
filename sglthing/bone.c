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
        data.quat[3] = ai_quaternion.w;
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

    bone->id = id;

    strncpy(bone->name, name, 64);
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
    glm_mat4_identity(t);
    if (bone->num_key_translations == 1)
    {
        struct key_translate t_k = g_array_index(bone->key_translations, struct key_translate, 0); 
        glm_translate(t,t_k.position);
        glm_mat4_copy(t,out);
        return;
    }
    int p0_index = bone_get_translation_idx(bone, animation_time);
    int p1_index = p0_index + 1;
    float scale_factor = get_scale_factor(g_array_index(bone->key_translations, struct key_translate, p0_index).timestamp, 
                                          g_array_index(bone->key_translations, struct key_translate, p1_index).timestamp, animation_time);
    vec3 final_position;
    glm_vec3_mix(g_array_index(bone->key_translations, struct key_translate, p0_index).position,
                 g_array_index(bone->key_translations, struct key_translate, p1_index).position,
                 scale_factor, final_position);
    glm_translate(t,final_position);
    glm_mat4_copy(t, out);
}

void bone_interp_rotation(struct bone* bone, mat4 out, float animation_time)
{    
    mat4 t;
    glm_mat4_identity(t);
    if (bone->num_key_rotations == 1)
    {
        struct key_rotate t_k = g_array_index(bone->key_rotations, struct key_rotate, 0); 
        glm_quat_mat4(t_k.quat,t);
        glm_mat4_copy(t,out);
        return;
    }
    int p0_index = bone_get_rotation_idx(bone, animation_time);
    int p1_index = p0_index + 1;
    float scale_factor = get_scale_factor(g_array_index(bone->key_rotations, struct key_rotate, p0_index).timestamp, 
                                          g_array_index(bone->key_rotations, struct key_rotate, p1_index).timestamp, animation_time);
    versor final_quat;
    glm_quat_slerp(g_array_index(bone->key_rotations, struct key_rotate, p0_index).quat,
                   g_array_index(bone->key_rotations, struct key_rotate, p1_index).quat,
                   scale_factor, final_quat);
    glm_quat_mat4(final_quat,t);
    glm_mat4_copy(t, out);
}

void bone_interp_scale(struct bone* bone, mat4 out, float animation_time)
{
    mat4 t;
    glm_mat4_identity(t);
    if (bone->num_key_scales == 1)
    {
        struct key_scale t_k = g_array_index(bone->key_scales, struct key_scale, 0); 
        glm_translate(t,t_k.scale);
        glm_mat4_copy(t,out);
        return;
    }
    int p0_index = bone_get_scale_idx(bone, animation_time);
    int p1_index = p0_index + 1;
    float scale_factor = get_scale_factor(g_array_index(bone->key_scales, struct key_scale, p0_index).timestamp, 
                                          g_array_index(bone->key_scales, struct key_scale, p1_index).timestamp, animation_time);
    vec3 final_scale;
    glm_vec3_mix(g_array_index(bone->key_scales, struct key_scale, p0_index).scale,
                 g_array_index(bone->key_scales, struct key_scale, p1_index).scale,
                 scale_factor, final_scale);
    glm_scale(t,final_scale);
    glm_mat4_copy(t, out);
}

void bone_update(struct bone* bone, float animation_time)
{
    mat4 model_matrix;
    glm_mat4_identity(model_matrix);
    mat4 t, r, s;
    bone_interp_position(bone, t, animation_time);
    bone_interp_rotation(bone, r, animation_time);
    bone_interp_scale(bone, s, animation_time);
    glm_mul(model_matrix, t, model_matrix);
    glm_mul(model_matrix, r, model_matrix);
    glm_mul(model_matrix, s, model_matrix);
    glm_mat4_copy(model_matrix, bone->local_transform);
}

int bone_get_translation_idx(struct bone* bone, float animation_time)
{
    for (int i = 0; i < bone->num_key_translations - 1; ++i)
    {
        if (animation_time < g_array_index(bone->key_translations, struct key_translate, i + 1).timestamp)
            return i;
    }
    ASSERT(0);
}

int bone_get_rotation_idx(struct bone* bone, float animation_time)
{
    for (int i = 0; i < bone->num_key_rotations - 1; ++i)
    {
        if (animation_time < g_array_index(bone->key_rotations, struct key_rotate, i + 1).timestamp)
            return i;
    }
    ASSERT(0);
}

int bone_get_scale_idx(struct bone* bone, float animation_time)
{
    for (int i = 0; i < bone->num_key_scales - 1; ++i)
    {
        if (animation_time < g_array_index(bone->key_scales, struct key_scale, i + 1).timestamp)
            return i;
    }
    ASSERT(0);
}