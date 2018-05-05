#version 450 core

// layout(location = 0) in vec2 coord;
layout(location = 0) in vec3 rotated_position;
layout(location = 1) in vec3 original_position;
layout(location = 2) in vec2 uv;
layout(location = 3) in mat3 TBN;

out vec4 color;

uniform vec3 lightPosition;
uniform mat4 view;

layout(binding = 0) uniform samplerCube sampler;
layout(binding = 1) uniform sampler2D normalMap;


void main()
{
  vec3 sampled_normal = texture(normalMap, uv).xyz * 2. - vec3(1.);
  vec3 out_normal = normalize(TBN * sampled_normal);
  
  vec3 world_cam_pos = -transpose(mat3(view)) * view[3].xyz;
  vec3 world_view_vec_norm = normalize(original_position - world_cam_pos);
  vec3 world_reflecvec = reflect(world_view_vec_norm, transpose(mat3(view)) * out_normal);
  
  vec3 lightPosition2 = (view * vec4(lightPosition, 1)).xyz;
  vec3 dp = rotated_position - lightPosition2;

  vec3 norm_normal = normalize(out_normal);
  vec3 norm_dp = normalize(dp);
    
  vec3 reflecvec = reflect(norm_dp, norm_normal);
  float deviation = max(0, dot(reflecvec, -normalize(rotated_position.xyz)));
  float specular_str = pow(deviation, 6);
    
  vec4 col = texture(sampler, world_reflecvec);

  if(dot(vec3(0, 0, 1), sampled_normal) < .1){
    color = vec4(1.0, 0, 0, 1.0);
    return;
  }
  
  float weighting = min(1, 0.2 + 2 * (1 -  dot(vec3(0, 0, 1), sampled_normal)));
  
  
  color =
    mix(col * 0.9, vec4(0.5, 0.5, 0.5, 1.0), weighting)
    + vec4(1, 1, 1, 0) * specular_str * 0.6;
  color.w = col.w;
  
}
