#version 450
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

@vert
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	mat4 model;
	mat4 mNormal;
} ubo;

layout (location = 0) out vec3 outWorldViewPosition;
layout (location = 1) out vec3 outNormal;

void main()  {
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(position, 1.0);
	outWorldViewPosition = (ubo.view * ubo.model * vec4(position, 1.0)).xyz;
	outNormal = mat3(ubo.view) * normal;
}

@frag
#include "utils.hglsl"

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec2 outNormal;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out vec4 outWordViewPosition;

layout(push_constant) uniform Material {
	vec4 diffuse;
} material;

void main() {
	outNormal = cartesianToSpherical(normalize(inNormal));
	outAlbedo = material.diffuse;
	outWordViewPosition = vec4(inWorldPos, 1);
}