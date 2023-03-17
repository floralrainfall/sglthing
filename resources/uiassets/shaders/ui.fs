in vec2 f_uv;


out vec4 FragColor;
uniform vec4 background_color;
uniform vec4 foreground_color;
uniform sampler2D texture0;

void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texture0, f_uv).r);
    vec4 out_color = vec4(1.0,1.0,1.0,1.0)*sampled;
    out_color = mix(background_color,out_color,sampled.w) * foreground_color;
    if(out_color == background_color && background_color.w == 0.0)
        discard;
    FragColor = out_color;
}