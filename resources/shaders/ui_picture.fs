#version 330 core

in vec2 f_uv;

out vec4 FragColor;
uniform vec4 background_color;
uniform vec4 color;
uniform sampler2D texture0;

void main()
{
    vec4 sampled = texture(texture0, f_uv);
    vec4 out_color = sampled*color;
    out_color = mix(background_color,out_color,out_color.w);
    FragColor = out_color;
}