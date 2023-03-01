layout (location = 0) in vec3 a_pos;

uniform mat4 model;

void main()
{
    gl_Position = lsm * model * vec4(a_pos, 1.0),vec2(320,240);
}  