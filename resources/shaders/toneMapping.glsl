#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform UBO {
	vec4 color;
} ubo;

@vert

out gl_PerVertex {
	vec4 gl_Position;
};
layout (location = 0) out vec2 outUV;

void main() {
    float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    float y = -1.0 + float((gl_VertexIndex & 2) << 1);
    outUV.x = (x+1.0)*0.5;
    outUV.y = (y+1.0)*0.5;

    gl_Position = vec4(x, y, 0, 1);
}

@frag
layout (set = 1, binding = 0) uniform sampler2D imageSampler;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragColor;

void main() {
    vec2 textCoord = inUV;
    textCoord.y = 1.0-textCoord.y;
	vec4 textureColor = texture(imageSampler, textCoord);

    const float gamma = 2.2;
    vec3 hdrColor = textureColor.rgb;
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
    mapped = pow(mapped, vec3(1.0 / gamma));
  
    outFragColor = vec4(mapped, 1.0);
}
