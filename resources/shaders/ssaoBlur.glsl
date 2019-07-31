#version 450

#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 0) uniform Ubo  {
	vec2 size;
} ubo;

@vert
layout (location = 0) out vec2 outUV;

void main() {
    float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    float y = -1.0 + float((gl_VertexIndex & 2) << 1);
    outUV.x = (x+1.0)*0.5;
    outUV.y = (y+1.0)*0.5;
    gl_Position = vec4(x, y, 0, 1);
}

@frag
#include "utils.hglsl"

layout(set = 1, binding = 0) uniform sampler samplers[2];
layout(set = 1, binding = 1) uniform texture2D textures[128];

layout(push_constant) uniform TextureIndices {
	int ssaoIndex;
}pc;
layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragcolor;

void main()  {
    vec2 textCoord = inUV;
    textCoord.y = 1.0 - textCoord.y;
  	vec2 texelSize = 1.0 / vec2(textureSize(sampler2D(textures[pc.ssaoIndex], samplers[linearBorder]), 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(sampler2D(textures[pc.ssaoIndex], samplers[linearBorder]), textCoord + offset).r;
        }
    }
	outFragcolor =  vec4(result / (4.0 * 4.0));
}