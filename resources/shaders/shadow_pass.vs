layout (location = 0) in vec3 v_pos;

layout(location=4) in ivec4 v_bone_ids;
layout(location=5) in vec4 v_weights;

uniform mat4 model;
uniform int sel_map;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 bone_matrices[MAX_BONES];

void main()
{       
    vec4 total_position = vec4(0.0);
    vec3 total_normal = vec3(0.0);
    int null_ids = 0;
    for(int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        if(v_bone_ids[i] == -1) 
        {
            null_ids++;
            continue;
        }
        if(v_bone_ids[i] >= MAX_BONES) 
        {
            total_position = vec4(v_pos,1.0f);
            //total_normal = v_normal;
            break;
        }
        vec4 local_position = bone_matrices[v_bone_ids[i]] * vec4(v_pos,1.0);
        total_position += local_position * v_weights[i];
        //vec3 local_normal = mat3(bone_matrices[v_bone_ids[i]]) * v_normal;
        //total_normal += local_normal * v_weights[i];
    }
    if(null_ids == 4)
        total_position = vec4(v_pos,1.0);
        
    if(sel_map == 0)
        gl_Position = lsm * model * total_position;
    else if(sel_map == 1)
        gl_Position = lsm_far * model * total_position;
}  