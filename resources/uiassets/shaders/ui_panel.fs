in vec2 f_uv;

out vec4 FragColor;
uniform vec4 top_color = vec4(0.0,0.325,1.0,1.0);
uniform vec4 bottom_color = vec4(0.016,0.0,0.812,1.0);
uniform sampler2D texture0;

void main()
{
    vec4 sampled = vec4(0.016,0.016,0.016,0.9);
    //out_color = mix(background_color,out_color,out_color.w);
    FragColor = sampled;
}