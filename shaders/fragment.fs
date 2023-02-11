#version 330 core
#define PI 3.1415926538

out vec4 FragColor;

uniform vec3 camera_position;
uniform float time;

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

void main()
{   
    // calculate diffuse lighting
    vec3 norm = normalize(f_normal);
    vec3 light_dir = -normalize(vec3(0.5,0.75,0.5) - f_pos);
    float diff = max(dot(norm, light_dir),0.1);
    vec3 diffuse = diff * vec3(0.77,0.8,0.9);

    // calculate specular lighting
    vec3 view_dir = normalize(camera_position - f_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);  
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 4);
    vec3 specular = 0.5 * spec * vec3(0.9,0.95,1.0);  

    ivec2 coord = ivec2(gl_FragCoord.xy - 0.5);

    vec2 affine_tex_coords = f_uv / f_affine;

    vec4 out_color = texture(texture1, affine_tex_coords);
    out_color *= vec4(diffuse + specular,1.0);
    
    vec4 out_color_raw = out_color;
    out_color_raw *= 255.0;
    ivec4 i_out_color_raw = ivec4(out_color_raw);
    i_out_color_raw.r = i_out_color_raw.r & 0xff8;
    i_out_color_raw.g = i_out_color_raw.g & 0xff8;
    i_out_color_raw.b = i_out_color_raw.b & 0xff8;
    out_color_raw = vec4(i_out_color_raw);
    out_color_raw /= 255.0;

    FragColor = vec4(out_color_raw.xyz, 1.0);
}
