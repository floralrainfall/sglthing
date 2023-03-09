uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
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
    f_normal = mat3(transpose(inverse(model))) * v_normal;  
    //f_normal = v_normal;
    f_color = v_color;
    f_pos = (model * vec4(v_pos, 1.0)).xyz;
    f_pos_light = lsm * vec4(f_pos, 1.0);
    f_pos_light_far = lsm_far * vec4(f_pos, 1.0);
    f_m_pos = v_pos;
    gl_Position = snap(projection * view * model * vec4(v_pos, 1.0),vec2(320/2,240/2));

    vec4 vertex_view = view * model * vec4(v_pos, 1.0);
    float dist = length(vertex_view);
    float affine = dist + ((gl_Position.w * 8.0) / dist) * 0.25;
    f_uv = v_uv * affine;
    f_affine = affine;
}
