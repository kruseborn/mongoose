#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform Ubo {
	uvec4 screenSize;
} ubo;

layout(set = 1, binding = 0) readonly buffer D {
   float x[];
}d;

@vert

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
    float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    float y = -1.0 + float((gl_VertexIndex & 2) << 1);
    gl_Position = vec4(x, y, 0, 1);
}

@frag
layout (location = 0) out vec4 out_frag_color;

void main() {
	int index = int(gl_FragCoord.y) * int(ubo.screenSize.x + 2) + int(gl_FragCoord.x);
	out_frag_color = vec4(d.x[index], 0, 0, 1);
}
