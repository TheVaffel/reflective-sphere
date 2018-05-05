#version 450 core

layout(location = 0) in vec2 texdir;
layout(location = 1) in vec3 dist;

layout(location = 0) out vec4 col;

uniform float texture_size;

layout(location = 0) uniform sampler2D original_texture;

void main(){
  const float maxdist = 0.3;

  vec2 coord = gl_FragCoord.xy/texture_size;
  vec4 sampled_value = texture(original_texture, coord);
  
  if(length(dist) < maxdist){
    vec3 orig_val = normalize(sampled_value.xyz * 2 - 1);
    
    float weight = pow((maxdist - length(dist))/maxdist, 2);
    vec3 new_val = normalize(vec3(-weight * texdir, 0.1));

    
    vec3 combined_val = normalize(mix(orig_val, new_val, weight));
    col = vec4((combined_val + vec3(1)) / 2, 1.);
  }else{
    col = sampled_value;
  }
}
