uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 bone_matrices[MAX_BONES];
uniform vec4 viewport;
uniform float time;
uniform vec3 camera_position;

layout(location=0) in vec3 v_pos;
layout(location=1) in vec3 v_normal;
layout(location=2) in vec4 v_color;
layout(location=3) in vec2 v_uv;
layout(location=4) in ivec4 v_bone_ids;
layout(location=5) in vec4 v_weights;

out vec3 f_pos;
out vec3 f_m_pos;
out vec3 f_normal;
out vec4 f_color;
out vec2 f_uv;
out vec4 f_pos_light;
out vec4 f_pos_light_far;
out float f_affine;

float near = 0.1; 
float far  = 1000.0; 

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
            total_normal = v_normal;
            break;
        }
        vec4 local_position = bone_matrices[v_bone_ids[i]] * vec4(v_pos,1.0);
        total_position += local_position * v_weights[i];
        vec3 local_normal = mat3(bone_matrices[v_bone_ids[i]]) * v_normal;
        total_normal += local_normal * v_weights[i];
    }
    if(null_ids == 4)
        total_position = vec4(v_pos,1.0);
        
    f_normal = mat3(transpose(inverse(model))) * total_normal;  
    //f_normal = v_normal;
    f_color = v_color;
    f_pos = (model * total_position).xyz;
    f_pos_light = lsm * vec4(f_pos, 1.0);
    f_pos_light_far = lsm_far * vec4(f_pos, 1.0);
    f_m_pos = v_pos;
    gl_Position = snap(projection * view * model * total_position,vec2(320,240));

    vec4 vertex_view = view * model * vec4(v_pos, 1.0);
    float dist = length(vertex_view);
    float affine = dist + ((gl_Position.w * 8.0) / dist) * 0.25;
    f_uv = v_uv * affine;
    f_affine = affine;
}
