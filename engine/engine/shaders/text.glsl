#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

@vert
layout (location = 0) in vec4 in_position;
layout (location = 0) out vec2 out_uv;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = vec4(in_position.xy, 0.0, 1.0);
	out_uv = in_position.zw;
	gl_Position.y = -gl_Position.y;
}

@frag
layout (set = 1, binding = 0) uniform sampler2D inTexture;

layout (location = 0) in vec2 in_uv;
layout (location = 0) out vec4 out_frag_color;

void main() {
	vec2 texCoord = in_uv;
	texCoord.y = 1.0 - texCoord.y;
	vec3 r = texture(inTexture, texCoord).rgb;
	out_frag_color = vec4(vec3(r), 1.0);
}