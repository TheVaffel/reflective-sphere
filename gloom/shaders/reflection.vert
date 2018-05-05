#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

layout(location = 0) out vec3 out_rotated_position;
layout(location = 1) out vec3 out_origin_position;
layout(location = 2) out vec2 coord;
layout(location = 3) out mat3 TBN; // Tangent, bitangent, normal

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);
    coord = tex;
    
    out_origin_position = (model * vec4(position, 1.0)).xyz;
    out_rotated_position = (view * model * vec4(position, 1.0)).xyz;

    // NB: Assumes that tangent and bitangent are normalized
    vec3 T = (view * model * vec4(tangent, 0.0)).xyz;
    vec3 B = (view * model * vec4(bitangent, 0.0)).xyz;
    vec3 N = normalize( (view * model * vec4(normal, 0.0)).xyz);

    TBN = mat3(T, B, N);
}
