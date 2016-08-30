#version 450

#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 0) uniform UBO  {
	mat4 mvp;
	vec2 size;
} ubo;

@vert
layout (location = 0) in vec4 position;
layout (location = 0) out vec2 outUV;

void main() {
	outUV = position.zw;
	gl_Position = ubo.mvp * vec4(vec3(position.xy, 0.0), 1.0);
    gl_Position.y = -gl_Position.y;
}

@frag
layout (set = 1, binding = 0) uniform sampler2D samplerSSAO;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragcolor;

void main()  {
    vec2 textCoord = inUV;
    textCoord.y = 1.0 - textCoord.y;
  	vec2 texelSize = 1.0 / vec2(textureSize(samplerSSAO, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(samplerSSAO, textCoord + offset).r;
        }
    }
	outFragcolor =  vec4(result / (4.0 * 4.0));
}