layout (location = 0) in vec3 v_pos;

layout(location=6) in vec3 v_offset;
layout(location=7) in uint v_obscure;
layout(location=8) in uint v_color_id;

uniform mat4 model;
uniform int sel_map;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 bone_matrices[MAX_BONES];

void main()
{       
    vec4 total_position = vec4(0.0);
    vec3 total_normal = vec3(0.0);
    total_position = vec4(v_pos + v_offset,1.0);
    if(v_obscure == uint(0))
    {
        gl_Position = vec4(0.0,0.0,0.0,0.0);
        return;
    }
    if(sel_map == 0)
        gl_Position = lsm * model * total_position;
    else if(sel_map == 1)
        gl_Position = lsm_far * model * total_position;
}  