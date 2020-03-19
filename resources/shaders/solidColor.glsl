#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform Ubo {
	vec4 temp;
} ubo;

layout(push_constant) uniform Model {
	mat4 mvp;
	vec4 color;
}pc;

@vert
layout (location = 0) in vec3 in_position;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec3 position = in_position;
	gl_Position = pc.mvp * vec4(position, 1.0);
}

@frag
layout (location = 0) out vec4 out_frag_color;

void main() {
	out_frag_color = pc.color;
}
