#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform Ubo  {
	vec4 temp;
} ubo;

layout(set = 1, binding = 0, rgba8) readonly uniform image2D image;

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
	out_frag_color = imageLoad(image, ivec2(gl_FragCoord.xy));
  //out_frag_color.ra = vec2(1.0);
}
