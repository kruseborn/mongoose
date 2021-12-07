#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, set = 0, binding = 0) uniform Ubo {	
	mat4 projection;
} ubo;

struct Data {
 vec2 texCoords;
 vec4 textColor;
};

@vert
layout (location = 0) in vec4 inPosition; // vec2 pos, vec2 tex
layout (location = 1) in vec4 inColor;

layout (location = 0) out Data outData;

void main() {
  gl_Position = ubo.projection * vec4(inPosition.xy, 0.0, 1.0);
  gl_Position.y = gl_Position.y;
  outData.texCoords = inPosition.zw;
  outData.textColor = inColor;
}

@frag
#include "utils.hglsl"

layout (location = 0) in Data inData;

layout(set = 1, binding = 0) uniform sampler samplers[2];
layout(set = 1, binding = 1) uniform texture2D textures[128];

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform TextureIndices {
	int textureIndex;
}pc;

void main() {
  float bitmapValue = texture(sampler2D(textures[pc.textureIndex], samplers[linearBorder]), inData.texCoords ).r;
  if(bitmapValue < 0.01)
    discard;
  outFragColor = inData.textColor;
}