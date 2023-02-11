#version 330 core

in vec2 f_uv;

out vec4 FragColor;
uniform sampler2D texture0;

void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texture0, f_uv).r);
    FragColor = vec4(1.0,1.0,1.0,1.0)*sampled;
}