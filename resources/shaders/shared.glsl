#version 330 core

uniform vec3 sun_direction;
uniform vec3 sun_position;

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
    float dist;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float present;
};

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 calc_point_light(light i, vec3 normal, vec3 frag_pos, vec3 view_dir)
{
    if(i.present != 0.0)
    {
        vec3 light_dir = normalize(i.position - frag_pos);
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

#define MAX_LIGHTS 32
uniform light lights[MAX_LIGHTS];
uniform vec3 ambient = vec3(30.0/255.0,26.0/255.0,117.0/255.0);
uniform vec3 diffuse = vec3(227.0/255.0,168.0/255.0,87.0/255.0);
uniform vec3 specular = vec3(255.0/255.0,204.0/255.0,51.0/255.0); 

vec3 calc_light(vec3 normal, vec3 frag_pos, vec3 camera_position, vec4 f_pos_light, vec4 f_pos_light_far)
{
        // ambient lighting
    vec3 ambient = 0.25 * ambient;

    // calculate diffuse lighting
    vec3 norm = normalize(normal);
    vec3 light_dir = normalize((frag_pos + sun_direction * 100.0) - frag_pos);
    float diff = max(dot(norm, light_dir),0.1);
    vec3 diffuse = diff * diffuse;

    // calculate specular lighting
    vec3 view_dir = normalize(camera_position - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);  
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 4);
    vec3 specular = 0.5 * spec * specular;

    // calculate shadow lighting
    float shadow = shadow_calculate(camera_position, 0.05, 0.0002, depth_map, frag_pos, f_pos_light, normal, (sun_direction*50.0));   
    if(shadow == -1)   
        shadow = shadow_calculate(camera_position, 0.5, 0.005, depth_map_far, frag_pos, f_pos_light_far, normal, (sun_direction*100.0));      
    if(shadow == -1)
        shadow = 0.0;
    //shadow = mix(shadow,shadow_far,distance(frag_pos, camera_position)/64.0);

    vec3 combined_light_result = vec3(0,0,0);
    for(int i = 0; i < MAX_LIGHTS; i++)
        combined_light_result += calc_point_light(lights[i], normal, frag_pos, view_dir);

    //return vec3(1.0,0.0,1.0) * 1.0-shadow;
    return ambient + (1.5 - shadow) * (diffuse + specular) + combined_light_result;
}

//	Simplex 3D Noise 
//	by Ian McEwan, Ashima Arts
//
vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}

float snoise(vec3 v){ 
    const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
    const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

    // First corner
    vec3 i  = floor(v + dot(v, C.yyy) );
    vec3 x0 =   v - i + dot(i, C.xxx) ;

    // Other corners
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min( g.xyz, l.zxy );
    vec3 i2 = max( g.xyz, l.zxy );

    //  x0 = x0 - 0. + 0.0 * C 
    vec3 x1 = x0 - i1 + 1.0 * C.xxx;
    vec3 x2 = x0 - i2 + 2.0 * C.xxx;
    vec3 x3 = x0 - 1. + 3.0 * C.xxx;

    // Permutations
    i = mod(i, 289.0 ); 
    vec4 p = permute( permute( permute( 
              i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
            + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
            + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

    // Gradients
    // ( N*N points uniformly over a square, mapped onto an octahedron.)
    float n_ = 1.0/7.0; // N=7
    vec3  ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z *ns.z);  //  mod(p,N*N)

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

    vec4 x = x_ *ns.x + ns.yyyy;
    vec4 y = y_ *ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4( x.xy, y.xy );
    vec4 b1 = vec4( x.zw, y.zw );

    vec4 s0 = floor(b0)*2.0 + 1.0;
    vec4 s1 = floor(b1)*2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
    vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

    vec3 p0 = vec3(a0.xy,h.x);
    vec3 p1 = vec3(a0.zw,h.y);
    vec3 p2 = vec3(a1.xy,h.z);
    vec3 p3 = vec3(a1.zw,h.w);

    //Normalise gradients
    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Mix final noise value
    vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                  dot(p2,x2), dot(p3,x3) ) );
}
