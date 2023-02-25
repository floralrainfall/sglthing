#version 330 core
#define PI 3.1415926538
#define MAX_LIGHTS 4

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;  

struct light {
    vec3 position;

    float constant;
    float linear;
    float quadratic;
    float intensity;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float present;
};
uniform light lights[MAX_LIGHTS];

uniform vec3 camera_position;
uniform float time;
uniform float fog_maxdist;
uniform float fog_mindist;
uniform vec4 fog_color;
uniform int banding_effect;

in vec3 f_pos;
in vec3 f_normal;
in vec4 f_color;
in vec2 f_uv;
in float f_affine;

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform sampler2D texture6;

vec3 calc_point_light(light i, vec3 normal, vec3 frag_pos, vec3 view_dir)
{
    if(i.present != 0.0)
    {
        vec3 light_dir = -normalize(i.position - f_pos);
        float diff = max(dot(normal, light_dir), 0.0); // diffuse
        // specular
        vec3 reflect_dir = reflect(-light_dir, normal);
        float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 4);
        // attenuation
        float dist = distance(i.position, f_pos);
        float attenuation = 1.0 / (i.constant + (i.linear * dist) +
                                    (i.quadratic * (dist * dist)));
        vec3 ambient = (i.ambient * i.intensity) * attenuation;
        vec3 diffuse = (i.diffuse * diff * i.intensity) * attenuation;
        vec3 specular = (i.specular * spec * i.intensity) * attenuation;
        return (ambient + diffuse + specular);
    }
    else
        return vec3(0,0,0);
}

void main()
{   
    // calculate diffuse lighting
    vec3 norm = normalize(f_normal);
    vec3 light_dir = -normalize(vec3(1.,-0.75,-0.5)*100000.0 - f_pos);
    float diff = max(dot(norm, light_dir),0.1);
    vec3 diffuse = diff * vec3(0.77,0.8,0.9);

    // calculate specular lighting
    vec3 view_dir = normalize(camera_position - f_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);  
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 4);
    vec3 specular = 0.5 * spec * vec3(0.9,0.95,1.0);  

    ivec2 coord = ivec2(gl_FragCoord.xy - 0.5);

    // affine thing
    vec2 affine_tex_coords = f_uv / f_affine;

    vec4 out_color = texture(texture1, affine_tex_coords);

    // lighting

    vec3 combined_light_result = vec3(0,0,0);
    for(int i = 0; i < MAX_LIGHTS; i++)
        combined_light_result += calc_point_light(lights[i], f_normal, f_pos, view_dir);

    out_color *= vec4(diffuse + specular + combined_light_result,1.0);

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
