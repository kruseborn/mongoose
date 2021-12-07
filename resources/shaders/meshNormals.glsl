#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform Ubo {
	mat4 mvp;
	vec4 color;
} ubo;

@vert

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 0) out vec4 out_normal;


out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	out_normal = vec4(in_normal.xyz, 0.0);
	gl_Position = ubo.mvp * vec4(in_position,1);
}

@frag
layout (location = 0) in vec3 in_normal;
layout (location = 0) out vec4 out_frag_color;

void main() {
	out_frag_color = ubo.color * 0.00001 + vec4(in_normal, 1.0);
}