layout (location = 0) in vec2 v_pos; 
layout (location = 1) in vec2 v_uv; 

out vec2 f_uv;
out vec4 f_color;
out vec3 f_pos;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform vec3 camera_position;
uniform vec4 color;

void main()
{
    float scale = 1.0f;
    f_uv = v_uv;
    f_color = color;

    vec4 pos = model * vec4(v_pos, 0.0, 1.0); 
    
    f_pos = pos.xyz;
    gl_Position = projection * view * pos;
}
