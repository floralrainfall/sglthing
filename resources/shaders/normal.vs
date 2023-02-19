#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 viewport;
uniform float time;
uniform vec3 camera_position;

layout(location=0) in vec3 v_pos;
layout(location=1) in vec3 v_normal;
layout(location=2) in vec4 v_color;
layout(location=3) in vec2 v_uv;
layout(location=4) in ivec4 v_bone_ids;
layout(location=5) in vec4 v_weights;

out vec3 f_pos;
out vec3 f_m_pos;
out vec3 f_normal;
out vec4 f_color;
out vec2 f_uv;
out float f_affine;

vec4 snap(vec4 vertex, vec2 resolution)
{
//  float dist = distance(camera_position, f_pos) * 0.01;
//  vertex.x += sin(v_pos.y + dist + time) * 0.025;
//  vertex.y += cos(v_pos.x + dist +time) * 0.025;
//  vertex.z += sin(v_pos.y + dist +time) * 0.025;
    vec4 snappedPos = vertex;
    snappedPos.xyz = vertex.xyz / vertex.w; // convert to normalised device coordinates (NDC)
    snappedPos.xy = floor(resolution * snappedPos.xy) / resolution; // snap the vertex to the lower-resolution grid
    snappedPos.xyz *= vertex.w; // convert back to projection-space
    return snappedPos;
}

void main()
{
    f_normal = mat3(transpose(inverse(model))) * v_normal;  
    //f_normal = v_normal;
    f_color = v_color;
    f_pos = (model * vec4(v_pos, 1.0)).xyz;
    f_m_pos = v_pos;
    gl_Position = snap(projection * view * model * vec4(v_pos, 1.0),vec2(320,240));

    vec4 vertex_view = view * model * vec4(v_pos, 1.0);
    float dist = length(vertex_view);
    float affine = dist + ((gl_Position.w * 8.0) / dist) * 0.25;
    f_uv = v_uv * affine;
    f_affine = affine;
}
