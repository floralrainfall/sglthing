uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout(location=0) in vec3 v_pos;
layout(location=1) in vec3 v_normal;
layout(location=3) in vec2 v_uv;

out vec3 f_pos;
out vec2 f_uv;
out vec3 f_normal;
out vec3 f_m_pos;

void main()
{
    f_pos = (model * vec4(v_pos, 1.0)).xyz;
    f_m_pos = v_pos;
    f_normal = v_normal;
    f_uv = v_uv;
    gl_Position = projection * view * model * vec4(v_pos, 1.0);
}
