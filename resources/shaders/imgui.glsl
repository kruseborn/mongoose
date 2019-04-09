#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct Data {
    vec4 Color;
    vec2 UV;
};

layout (set = 0, binding = 0) uniform UBO {
    vec2 uScale;
    vec2 uTranslate;
 } ubo;

@vert
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

out gl_PerVertex{
    vec4 gl_Position;
};

layout (location = 0) out Data outData;

void main()
{
    outData.Color = inColor;
    outData.UV = inUV;
    gl_Position = vec4(inPos*ubo.uScale+ubo.uTranslate, 0, 1);
}

@frag
layout(location = 0) out vec4 outColor;
layout (location = 0) in Data inData;

layout(set=1, binding=0) uniform sampler2D sTexture;

void main() {
    outColor = inData.Color * texture(sTexture, inData.UV.st);
}