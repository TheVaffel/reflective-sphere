#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

layout(location = 0) out vec2 texdir; // Direction from collision point, in uv space
layout(location = 1) out vec3 dist;

uniform vec3 collisionPoint;

void main()
{
  
  gl_Position = vec4(2 * tex.x - 1, 2 * tex.y - 1, 0., 1);
  dist = position - collisionPoint;

  float a = dot(dist, tangent);
  float b = dot(dist, bitangent);

  texdir = vec2(a, b);

}
