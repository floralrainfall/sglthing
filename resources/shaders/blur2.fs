out vec4 FragColor;
  
in vec2 texcoord;

uniform sampler2D scene;
uniform sampler2D bloom;
uniform float exposure = 1.0;

void main()
{             
    const float gamma = 1.2;
    vec3 hdrColor = texture(scene, texcoord).rgb;      
    vec3 bloomColor = texture(bloom, texcoord).rgb;
    hdrColor += bloomColor; // additive blending
    vec3 result;

    // reinhard tone mapping
    result = hdrColor / (hdrColor + vec3(1.0));

    // tone mapping
    result = vec3(1.0) - exp(-hdrColor * exposure);

    FragColor = vec4(result, 1.0);
}  