#version 450

#extension GL_ARB_shading_language_420pack : enable

@vert
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture;


layout (set = 0, binding = 0) uniform UBO {
	mat4 projection, view, model;
	vec4 lightPosition;
} ubo;
layout (location = 0) out vec3 _N;
layout (location = 1) out vec3 _L;
layout (location = 2) out vec3 _V;

void main(void) {
	mat4 mv = ubo.view * ubo.model;
	mat4 mvp = ubo.projection * mv;
	gl_Position = mvp * vec4(position, 1.0);

	vec4 P = ubo.model * vec4(position, 1.0);
	_N = mat3(mv) * normal;
	vec3 L = (ubo.model * vec4(ubo.lightPosition.xyz, 1.0)).xyz;
	_L = L - P.xyz;
	_V = -P.xyz;
}

@frag
layout (location = 0) in vec3 _N;
layout (location = 1) in vec3 _L;
layout (location = 2) in vec3 _V;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform Material {
	vec4 diffuse;
	vec4 specular;
} material;

void main(void) {
	vec3 N = normalize(_N);
  	vec3 L = normalize(_L);
  	vec3 V = normalize(_V);
  	vec3 R = reflect(-L, N);

  	vec3 diffuse = max(dot(N,L), 0.0) + material.diffuse.rgb;
  	vec3 specular = pow(max(dot(R,V), 0.0), 16.0) *  material.specular.xyz;

  	outFragColor = vec4(diffuse + specular, 1.0);
  }