#version 330 core

uniform vec3 sun_direction;

vec4 snap(vec4 vertex, vec2 resolution)
{
    vec4 snappedPos = vertex;
    snappedPos.xyz = vertex.xyz / vertex.w; // convert to normalised device coordinates (NDC)
    snappedPos.xy = floor(resolution * snappedPos.xy) / resolution; // snap the vertex to the lower-resolution grid
    snappedPos.xyz *= vertex.w; // convert back to projection-space
    return snappedPos;
}

uniform sampler2D depth_map;
uniform sampler2D depth_map_far;
uniform mat4 lsm;
uniform mat4 lsm_far;

float shadow_calculate(vec3 camera, float b1, float b2, sampler2D map, vec3 pos, vec4 pos_light, vec3 normal, vec3 light_dir)
{    
    vec3 proj_coords = pos_light.xyz / pos_light.w;
    proj_coords = proj_coords * 0.5 + 0.5;

    if(proj_coords.x <= 0.01 || proj_coords.x >= 0.999)
        return -1;
    if(proj_coords.y <= 0.01 || proj_coords.y >= 0.999)
        return -1;
    if(proj_coords.z <= 0.01 || proj_coords.z >= 0.999)
        return -1;
    //float closest_depth = texture(map, proj_coords.xy).r;
    float current_depth = proj_coords.z;  
    float bias = max(b1 * (1.0 - dot(normal, light_dir)), b2);  
    float shadow = 0.0;
    vec2 texel_size = 1.0 / textureSize(map, 0);
    
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcf_depth = texture(map, proj_coords.xy + vec2(x, y) * texel_size).r; 
            shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

    return shadow;
}

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

vec3 calc_point_light(light i, vec3 normal, vec3 frag_pos, vec3 view_dir)
{
    if(i.present != 0.0)
    {
        vec3 light_dir = -normalize(i.position - frag_pos);
        float diff = max(dot(normal, light_dir), 0.0); // diffuse
        // specular
        vec3 reflect_dir = reflect(-light_dir, normal);
        float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 4);
        // attenuation
        float dist = distance(i.position, frag_pos);
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

#define MAX_LIGHTS 4
uniform light lights[MAX_LIGHTS];

vec3 calc_light(vec3 normal, vec3 frag_pos, vec3 camera_position, vec4 f_pos_light, vec4 f_pos_light_far)
{
        // ambient lighting
    vec3 ambient = 0.25 * vec3(30.0/255.0,26.0/255.0,117.0/255.0);

    // calculate diffuse lighting
    vec3 norm = normalize(normal);
    vec3 light_dir = normalize((frag_pos + sun_direction * 100.0) - frag_pos);
    float diff = max(dot(norm, light_dir),0.1);
    vec3 diffuse = diff * vec3(227.0/255.0,168.0/255.0,87.0/255.0);

    // calculate specular lighting
    vec3 view_dir = normalize(camera_position - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);  
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 4);
    vec3 specular = 0.5 * spec * vec3(255.0/255.0,204.0/255.0,51.0/255.0);  

    // calculate shadow lighting
    float shadow = shadow_calculate(camera_position, 0.5, 0.05, depth_map, frag_pos, f_pos_light, normal, camera_position + (sun_direction*50.0));   
    if(shadow == -1)   
        shadow = shadow_calculate(camera_position, 0.05, 0.005, depth_map_far, frag_pos, f_pos_light_far, normal, camera_position + (sun_direction*100.0));      
    if(shadow == -1)
        shadow = 0.0;
    //shadow = mix(shadow,shadow_far,distance(frag_pos, camera_position)/64.0);

    vec3 combined_light_result = vec3(0,0,0);
    for(int i = 0; i < MAX_LIGHTS; i++)
        combined_light_result += calc_point_light(lights[i], normal, frag_pos, view_dir);

    //return vec3(1.0,0.0,1.0) * 1.0-shadow;
    return ambient + (1.0 - shadow) * (diffuse + specular) + combined_light_result;
}
