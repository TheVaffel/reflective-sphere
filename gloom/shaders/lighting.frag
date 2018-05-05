#version 450 core

layout(location = 0) in vec2 coord;
layout(location = 1) in vec3 out_normal;
layout(location = 2) in vec3 out_position;

out vec4 color;

uniform vec3 lightPosition;
uniform mat4 view;

layout(binding = 0) uniform sampler2D sampler;

void main()
{
        vec3 lightPosition2 = (view * vec4(lightPosition, 1)).xyz;
        vec3 dp = out_position - lightPosition2;
        float l = length(dp);

        vec3 norm_normal = normalize(out_normal);
        vec3 norm_dp = normalize(dp);

        float light_strength = max(0, -dot(norm_dp, norm_normal));
        //vec3 reflecvec = norm_dp - 2 * dot(norm_dp, norm_normal) * norm_normal;
	vec3 reflecvec = reflect(norm_dp, norm_normal);
	float deviation = max(0, dot(reflecvec, -normalize(out_position.xyz)));
        float specular_str = pow(deviation, 4);

        color =
	// texture(sampler, coord);
         texture(sampler, coord) * light_strength // diffuse
         + vec4(1, 1, 1, 0) * specular_str * light_strength * 0.4 // specular
	// vec4(1.0, 0, 0, 1.0);
	;
        color.w = 1.0;
}