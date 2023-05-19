layout(location=0) in vec4 v_pos;

out vec2 f_uv;
uniform vec3 offset;
uniform float waviness;
uniform float time;
uniform float depth;
uniform mat4 projection;

void main()
{
    f_uv = v_pos.zw;

    vec2 moved_pos = v_pos.xy;
    
    moved_pos = floor(moved_pos);

    moved_pos.x += sin((v_pos.x+v_pos.y+time*1.1))*waviness;
    moved_pos.y += cos((v_pos.x+v_pos.y+time*1.1))*waviness;
    if(depth < 0.1)
        gl_Position = vec4(0,0,0,0);
        
    gl_Position = (projection * vec4(vec3(moved_pos.xy, depth) + offset, 1.0));
}