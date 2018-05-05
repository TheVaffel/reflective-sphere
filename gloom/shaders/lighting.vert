#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec2 coord;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);
    coord = tex;
    out_normal = ( view * model * vec4(normal, 0.0)).xyz;
    out_position = (view * model * vec4(position, 1.0)).xyz;
}