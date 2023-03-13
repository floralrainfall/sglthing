layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

out vec2 texcoord;

uniform vec2 offset;
uniform vec2 size;

void main() 
{
    vec2 a_size;
    if(length(size) == 0.0)
        a_size = vec2(1.0, 1.0);
    else
        a_size = size;
    gl_Position = vec4(offset.x+a_size.x*1.0, offset.y+a_size.y*1.0, 0.5, 1.0 );
    texcoord = vec2( 1.0, 1.0 );
    EmitVertex();

    gl_Position = vec4(offset.x+a_size.x*-1.0,offset.y+a_size.y* 1.0, 0.5, 1.0 );
    texcoord = vec2( 0.0, 1.0 ); 
    EmitVertex();

    gl_Position = vec4(offset.x+a_size.x* 1.0,offset.y+a_size.y*-1.0, 0.5, 1.0 );
    texcoord = vec2( 1.0, 0.0 ); 
    EmitVertex();

    gl_Position = vec4(offset.x+a_size.x*-1.0,offset.y+a_size.y*-1.0, 0.5, 1.0 );
    texcoord = vec2( 0.0, 0.0 ); 
    EmitVertex();

    EndPrimitive(); 
}