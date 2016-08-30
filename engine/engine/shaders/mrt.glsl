#version 450

#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

@vert

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	mat4 model;
	mat4 mNormal;
} ubo;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;

void main()  {
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(position, 1.0);
	gl_Position.y = -gl_Position.y;	

	outWorldPos = (ubo.view *  ubo.model * vec4(position, 1.0)).xyz;
	//mat3 mNormal = transpose(inverse(mat3(ubo.view * ubo.model)));
	outNormal = mat3(ubo.mNormal) * normal;	
}

@frag
#include "utils.glsl"

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec2 outNormal;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out float outDepth;

layout(push_constant) uniform Material {
	vec4 diffuse;
} material;


void main() {
	outNormal = cartesianToSpherical(normalize(inNormal));
	outAlbedo = material.diffuse;
	outDepth = length(inWorldPos);
}