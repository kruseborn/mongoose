#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, set = 0, binding = 0) uniform Ubo {
	mat4 mvp;
	mat4 worldToBox;
} ubo;


layout (location = 0) in vec3 positions;
layout (location = 0) out vec4 outPosition;

void main(void) {
  outPosition = vec4(positions, 1.0);
  gl_Position = ubo.mvp * vec4(positions, 1.0);;
}

