in vec3 f_pos;
in vec3 f_m_pos;
in vec3 f_normal;
out vec4 FragColor;
uniform float time;
uniform int banding_effect;
uniform vec3 camera_position;

#define M_PI 3.14159265358979323846

void main()
{   
    vec4 out_color = vec4(255.0/255.0,204.0/255.0,51.0/255.0,1);

    out_color.w = f_m_pos.y + 0.2;

    vec3 sun = vec3(-0.5,0.5,0.5);

    // color banding effect
    vec4 out_color_raw = clamp(out_color,0,1.0);
    out_color_raw *= 16.0;
    ivec4 i_out_color_raw = ivec4(out_color_raw);
    i_out_color_raw.r = i_out_color_raw.r & banding_effect;
    i_out_color_raw.g = i_out_color_raw.g & banding_effect;
    i_out_color_raw.b = i_out_color_raw.b & banding_effect;
    out_color_raw = vec4(i_out_color_raw);
    out_color_raw /= 16.0;
    out_color = out_color_raw;

    out_color = mix(vec4(1,1,1,1),out_color,distance(sun,f_m_pos)-0.15);
    if(distance(sun,f_m_pos)<0.3)
        out_color = vec4(1,1,1,1);

    FragColor = out_color;
}