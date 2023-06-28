layout (location = 0) in vec3 v_pos;
layout(location=3) in vec2 v_uv;

layout(location=6) in vec3 v_offset;
layout(location=7) in float v_obscure;
layout(location=8) in float v_block_id;

uniform mat4 model;
uniform int sel_map;

out vec2 f_uv;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 bone_matrices[MAX_BONES];

void main()
{       
    vec4 total_position = vec4(0.0);
    vec3 total_normal = vec3(0.0);
    total_position = vec4(v_pos + v_offset,1.0);
    if(v_obscure == 0)
    {
        gl_Position = vec4(0.0,0.0,0.0,0.0);
        return;
    }
    if(sel_map == 0)
        gl_Position = lsm * model * total_position;
    else if(sel_map == 1)
        gl_Position = lsm_far * model * total_position;
    int i_block_id = int(floor(v_block_id));
    float spr_x = i_block_id % 64;
    float spr_y = i_block_id / 64;
    float i_pctg = 0.03125;
    vec2 uv_2 = vec2(
        (spr_x * i_pctg) + (v_uv.x * i_pctg),
        (spr_y * i_pctg) + (v_uv.y * i_pctg)
    );
    f_uv = uv_2;
}  