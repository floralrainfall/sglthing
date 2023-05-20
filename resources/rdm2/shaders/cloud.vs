uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout(location=0) in vec3 v_pos;

out vec3 f_pos;
out vec3 f_m_pos;

void main()
{
    f_pos = (model * vec4(v_pos, 1.0)).xyz;
    f_m_pos = v_pos;
    gl_Position = projection * view * model * vec4(v_pos, 1.0);
}
