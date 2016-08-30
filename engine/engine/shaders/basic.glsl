#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform UBO {
	mat4 mvp;
} ubo;

@vert

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;

layout (location = 0) out vec4 out_texcoord;
layout (location = 1) out vec4 out_color;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = ubo.mvp * vec4(in_position, 1.0f);
	out_texcoord = vec4(in_texcoord.xy, 0.0f, 0.0f);
	out_color = vec4(in_normal, 1.0);
}

@frag
layout (location = 0) in vec4 in_texcoord;
layout (location = 1) in vec4 in_color;

layout (location = 0) out vec4 out_frag_color;

void main() {
	out_frag_color = in_color;// + push_constants.color;
}