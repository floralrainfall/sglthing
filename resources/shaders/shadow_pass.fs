
in vec2 f_uv;
uniform sampler2D diffuse0;

void main()
{             
    if(texture(diffuse0, f_uv).a == 0)
        discard;
}  
