out vec4 FragColor;
  
in vec2 texcoord;

uniform sampler2D scene;
uniform sampler2D bloom;
uniform sampler2D depth;
uniform float exposure = 1.0;
uniform float lsd;
uniform float time;
uniform vec3 camera_position;
uniform int banding_effect = 0xf8;
uniform vec4 viewport;

#define FXAA_SPAN_MAX 8.0
#define FXAA_REDUCE_MUL   (1.0/FXAA_SPAN_MAX)
#define FXAA_REDUCE_MIN   (1.0/128.0)
#define FXAA_SUBPIX_SHIFT (1.0/4.0)

vec3 FxaaPixelShader( vec4 uv, sampler2D tex, vec2 rcpFrame) {
    
    vec3 rgbNW = textureLod(tex, uv.zw, 0.0).xyz;
    vec3 rgbNE = textureLod(tex, uv.zw + vec2(1,0)*rcpFrame.xy, 0.0).xyz;
    vec3 rgbSW = textureLod(tex, uv.zw + vec2(0,1)*rcpFrame.xy, 0.0).xyz;
    vec3 rgbSE = textureLod(tex, uv.zw + vec2(1,1)*rcpFrame.xy, 0.0).xyz;
    vec3 rgbM  = textureLod(tex, uv.xy, 0.0).xyz;

    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
          dir * rcpDirMin)) * rcpFrame.xy;

    vec3 rgbA = (1.0/2.0) * (
        textureLod(tex, uv.xy + dir * (1.0/3.0 - 0.5), 0.0).xyz +
        textureLod(tex, uv.xy + dir * (2.0/3.0 - 0.5), 0.0).xyz);
    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
        textureLod(tex, uv.xy + dir * (0.0/3.0 - 0.5), 0.0).xyz +
        textureLod(tex, uv.xy + dir * (3.0/3.0 - 0.5), 0.0).xyz);
    
    float lumaB = dot(rgbB, luma);

    if((lumaB < lumaMin) || (lumaB > lumaMax)) return rgbA;
    
    return rgbB; 
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{          
    vec2 res = 1.0/viewport.zw;
    vec2 off = vec2(
        sin((time/1.5)+sin(texcoord.x*20.0))*1.5,
        cos((time/2.0)+cos(texcoord.y*15.0))*1.2
    );
    vec2 texcoord_t = (off * lsd) + texcoord;
    //float depth = texture(depth, texcoord_t).r * (500.0 + sin(time/100.0)) + camera_position.y + camera_position.x;
    vec4 uv_i = vec4(texcoord, texcoord - (res * (0.5 + FXAA_SUBPIX_SHIFT)));
    float depth = FxaaPixelShader(uv_i, depth, res).r;

    float lsdPerlinA = snoise(vec3(sin(texcoord.x*1.5)*1.5+sin(time/1.5) + camera_position.x,cos(texcoord.y*2.0)*2.0+cos(time/2.5) + camera_position.y,depth*5.0+time/100.0)) / 2.0;
    float lsdPerlinB = snoise(vec3(sin(texcoord.x*2.0)*2.5+cos(time/1.5) + camera_position.x,cos(texcoord.y*1.5)*1.5+sin(time/2.5) + camera_position.y,depth*5.0+time/100.0)) / 3.0;

    const float gamma = 1.2;
    vec3 hdrColor = FxaaPixelShader(uv_i, scene, res).rgb;      

    vec3 lsdColor = hsv2rgb(vec3(
        sin(depth+(depth+texcoord.x*1.5)+(time/1.5))+lsdPerlinA*2.0+
        cos(depth+(depth+texcoord.y*1.3)+(time/2.0))+lsdPerlinB*1.5
        ,1.0,1.0)) * (lsd * 100.0) * hdrColor;

    // color banding effect
    vec3 lsdColorRaw = lsdColor;
    lsdColorRaw *= 64.0;
    ivec3 i_lsdColorRaw = ivec3(lsdColorRaw);
    i_lsdColorRaw.r = i_lsdColorRaw.r & banding_effect;
    i_lsdColorRaw.g = i_lsdColorRaw.g & banding_effect;
    i_lsdColorRaw.b = i_lsdColorRaw.b & banding_effect;
    lsdColorRaw = vec3(i_lsdColorRaw);
    lsdColorRaw /= 64.0;

    vec3 bloomColor = FxaaPixelShader(uv_i, bloom, res).rgb;
    hdrColor += bloomColor; // additive blending
    hdrColor += lsdColorRaw + (lsdColor * 0.5);

    vec3 result;

    // reinhard tone mapping
    result = hdrColor / (hdrColor + vec3(1.0));

    // tone mapping
    result = vec3(1.0) - exp(-hdrColor * exposure);

    FragColor = vec4(result, 1.0);
}  