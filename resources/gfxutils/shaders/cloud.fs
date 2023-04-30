in vec3 f_m_pos;
out vec4 FragColor;
uniform float time;
uniform int banding_effect;
uniform vec3 camera_position;

#define M_PI 3.14159265358979323846

void main()
{   
    vec3 pos;
    pos.x = f_m_pos.x*5.0+time/100.0+camera_position.x/1000.0;
    pos.y = f_m_pos.z*5.0+time/1000.0+camera_position.z/1000.0;
    pos.z = time/1000.0;

    float v = snoise(pos*(255.0-camera_position.y/10.0)/29.0)+0.5;

    float s = clamp(snoise(f_m_pos/10),0.2,1.0);

    pos.x = f_m_pos.x/10.0+time/10000.0+1000;
    pos.y = f_m_pos.z/10.0+1000;
    pos.z = time/100000.0;
    float v2 = snoise(pos*30.0)+0.5;

    v = v * v2 * s;

    float dist = atan(distance(vec2(0,0),f_m_pos.xz)*25);

    vec4 out_color = vec4(255.0/255.0, 220.0/255.0, 110.0/255.0, v/clamp(dist*dist,0.2,0.5));

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

    FragColor = out_color;
}