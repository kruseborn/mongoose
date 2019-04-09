#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform UBO  {
	mat4 mvp;
	vec2 nearAndFar;
} ubo;

@vert
layout (location = 0) in vec4 positions;
layout (location = 0) out vec2 outUV;


out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = ubo.mvp * vec4(positions.xy, 0.0, 1.0);
	outUV = positions.zw;
}

@frag
#include "utils.hglsl"

layout (set = 1, binding = 0) uniform sampler2D samplerDepth;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragColor;

void main() {
	vec2 texCoord = inUV;
	texCoord.y = 1.0 - texCoord.y;
	float r = texture(samplerDepth, texCoord).r;
	r = linearizeDepth(r, ubo.nearAndFar.x, ubo.nearAndFar.y);
	outFragColor = vec4(vec3(r), 1.0);
}