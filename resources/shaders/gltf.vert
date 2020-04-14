#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform Ubo {
  mat4 model;
  mat4 view;
  mat4 projection;
  vec4 cameraPosition;
} ubo;

struct Data {
  vec3 worldPosition;
  vec3 N;
	vec3 T;
  vec2 UV;
	mat3 model;
};

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec4 in_tangent;
layout (location = 3) in vec2 in_texCoord;
layout (location = 0) out Data outData;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
  outData.worldPosition = (ubo.model * vec4(in_position, 1.0)).xyz;
	outData.N = mat3(ubo.model) * in_normal;
	outData.T = mat3(ubo.model) * in_tangent.xyz;
	outData.UV = in_texCoord;

	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(in_position, 1.0);
}

