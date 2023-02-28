in vec3 f_m_pos;
out vec4 FragColor;
uniform float time;
uniform int banding_effect;
uniform vec3 camera_position;

#define M_PI 3.14159265358979323846

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