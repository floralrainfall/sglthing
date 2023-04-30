out vec4 FragColor;
  
in vec2 texcoord;

uniform sampler2D scene;
uniform sampler2D bloom;
uniform sampler2D depth;
uniform float exposure = 1.0;
uniform float lsd;
uniform float time;

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{          
    vec2 off = vec2(
        sin((time/1.5)+sin(texcoord.x*20.0))*1.5,
        cos((time/2.0)+cos(texcoord.y*15.0))*1.2
    );
    vec2 texcoord_t = (off * lsd) + texcoord;
    float depth = texture(depth, texcoord_t).r * (500.0 + sin(time/100.0));

    float lsdPerlinA = snoise(vec3(sin(texcoord.x*1.5)*1.5+sin(time/1.5),cos(texcoord.y*2.0)*2.0+cos(time/2.5),depth*5.0+time/100.0)) / 2.0;
    float lsdPerlinB = snoise(vec3(sin(texcoord.x*2.0)*2.5+cos(time/1.5),cos(texcoord.y*1.5)*1.5+sin(time/2.5),depth*5.0+time/100.0)) / 3.0;


    const float gamma = 1.2;
    vec3 hdrColor = texture(scene, texcoord_t).rgb;      

    vec3 lsdColor = hsv2rgb(vec3(
        sin(depth+(depth+texcoord.x*1.5)+(time/1.5))+lsdPerlinA*2.0+
        cos(depth+(depth+texcoord.y*1.3)+(time/2.0))+lsdPerlinB*1.5
        ,1.0,1.0)) * (lsd * 100.0) * hdrColor;

    vec3 bloomColor = texture(bloom, texcoord_t).rgb;
    hdrColor += bloomColor; // additive blending
    hdrColor += lsdColor;

    vec3 result;

    // reinhard tone mapping
    result = hdrColor / (hdrColor + vec3(1.0));

    // tone mapping
    result = vec3(1.0) - exp(-hdrColor * exposure);

    FragColor = vec4(result, 1.0);
}  