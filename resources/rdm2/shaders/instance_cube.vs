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

layout(location=6) in vec3 v_offset;
layout(location=7) in uint v_obscure;
layout(location=8) in uint v_color_id;

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

vec3 color_id_color(float color)
{
    return hsv2rgb(vec3(color/255,1.0,1.0));
}

void main()
{
    vec4 position_model = (model * (vec4(v_pos + v_offset, 1.0)));

    f_normal = v_normal;
    f_color = vec4(color_id_color(float(v_color_id%uint(255))),1.0);
    f_pos = position_model.xyz;
    f_pos_light = lsm * vec4(f_pos, 1.0);
    f_pos_light_far = lsm_far * vec4(f_pos, 1.0);
    f_m_pos = v_pos;

    if(v_obscure != uint(0))
        gl_Position = snap(projection * view * position_model,vec2(viewport.z/4,viewport.w/4));
    else
        gl_Position = vec4(0.0,0.0,0.0,0.0);

    vec4 vertex_view = view * model * vec4(v_pos, 1.0);
    float dist = length(vertex_view);
    float affine = dist + ((gl_Position.w * 8.0) / dist) * 0.25;
    f_uv = v_uv * affine;
    f_affine = affine;
}
