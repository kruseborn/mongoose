#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform UBO {
	mat4 projection;
	mat4 modelview;
	vec2 screendim;
} ubo;

@vert

layout (location = 0) in vec4 in_position;
layout (location = 1) in vec4 in_velocity;

layout (location = 0) out float outGradient;

out gl_PerVertex {
	vec4 gl_Position;
	float gl_PointSize;
};


void main ()  {
	vec4 viewspacePosition = ubo.modelview * vec4(in_position.xyz, 1.0);
	gl_Position = ubo.projection * viewspacePosition;
	gl_PointSize = 50* pow(in_position.w, 1) / length(viewspacePosition);
}

@frag
layout (location = 0) in float inGradient;
layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform sampler2D particleTexture;

void main () {
	vec4 particleColor = texture(particleTexture, gl_PointCoord);
	particleColor.xyz *= vec3(0.0,0.6,0.9);
	outFragColor = particleColor;
}
