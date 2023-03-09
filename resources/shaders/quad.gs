layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

out vec2 texcoord;

uniform vec2 offset;
uniform vec2 size;

void main() 
{
    gl_Position = vec4(offset.x+size.x*1.0, offset.y+size.y*1.0, 0.5, 1.0 );
    texcoord = vec2( 1.0, 1.0 );
    EmitVertex();

    gl_Position = vec4(offset.x+size.x*-1.0,offset.y+size.y* 1.0, 0.5, 1.0 );
    texcoord = vec2( 0.0, 1.0 ); 
    EmitVertex();

    gl_Position = vec4(offset.x+size.x* 1.0,offset.y+size.y*-1.0, 0.5, 1.0 );
    texcoord = vec2( 1.0, 0.0 ); 
    EmitVertex();

    gl_Position = vec4(offset.x+size.x*-1.0,offset.y+size.y*-1.0, 0.5, 1.0 );
    texcoord = vec2( 0.0, 0.0 ); 
    EmitVertex();

    EndPrimitive(); 
}