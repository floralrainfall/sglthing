#define PI 3.1415926538
#define MAX_LIGHTS 4

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;  

uniform vec3 camera_position;
uniform float time;
uniform float fog_maxdist;
uniform float fog_mindist;
uniform vec4 fog_color;
uniform vec4 color;
uniform int banding_effect;

in vec3 f_pos;
in vec3 f_normal;
in vec4 f_color;
in vec2 f_uv;
in vec4 f_pos_light;
in float f_affine;

void main()
{   
    vec3 lighting = calc_light(f_normal, f_pos, camera_position, f_pos_light);

    // affine thing
    vec2 affine_tex_coords = f_uv / f_affine;

    vec4 out_color = color;

    // lighting

    out_color *= vec4(lighting,1.0);

    // color banding effect
    vec4 out_color_raw = out_color;
    out_color_raw *= 255.0;
    ivec4 i_out_color_raw = ivec4(out_color_raw);
    i_out_color_raw.r = i_out_color_raw.r & banding_effect;
    i_out_color_raw.g = i_out_color_raw.g & banding_effect;
    i_out_color_raw.b = i_out_color_raw.b & banding_effect;
    out_color_raw = vec4(i_out_color_raw);
    out_color_raw /= 255.0;
    out_color = out_color_raw;
    
    // fog
    float dist = distance(camera_position.xyz, f_pos.xyz);
    float fog_factor = (fog_maxdist - dist) /
                    (fog_maxdist - fog_mindist);
    fog_factor = clamp(fog_factor, 0.0, 1.0);

    out_color = mix(fog_color, out_color, fog_factor);

    FragColor = vec4(out_color.xyz, min(out_color.w,fog_factor));

    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 0.1)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
