#version 450

#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

struct Light {
	vec4 position;
	vec4 color;
};
const int lightCount = 32;

layout (set = 0, binding = 0) uniform Ubo  {
	mat4 projection;
	mat4 view;
	Light lights[lightCount];
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
	int normalIndex;
	int diffuseIndex;
  int ssaoBluredIndex;
  int worldViewPostionIndex;
}pc;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragcolor;

vec3 lin2srgb(in vec3 color) { return sqrt(color); }
vec3 srgb2lin(in vec3 color) { return color * color; }
float diffuse(in vec3 N, in vec3 L) { return max(0.0, dot(N, L)); }

float specularBlinnPhong(in vec3 N, in vec3 H, float specularPower) {
  float normalization = (8.0 + specularPower) / 8.0;
  return pow(max(0.0, dot(N, H)), specularPower) * normalization;
}

void main()  {
	vec2 textCoord = inUV;
	textCoord.y = 1.0 - textCoord.y;

	vec3 N = normalize(sphericalToCartesian(texture(sampler2D(textures[pc.normalIndex], samplers[linearBorder]), textCoord).xy));
	vec4 diffuse_material = texture(sampler2D(textures[pc.diffuseIndex], samplers[linearBorder]), textCoord);
	float ssao = texture(sampler2D(textures[pc.ssaoBluredIndex], samplers[linearBorder]), textCoord).r;	
	vec3 V = texture(sampler2D(textures[pc.worldViewPostionIndex], samplers[linearBorder]), textCoord).xyz;

	vec3 outputColor = vec3(0.0);
	for(int i = 0; i < lightCount; i++) {
		Light light = ubo.lights[i];
		const float ambient = 0.0;
		vec3 lightPos = (ubo.view * light.position).xyz;
		vec3 lightDir = lightPos - V;

		float atten = light.color.w / (pow(length(lightDir), 2.0) + 1.0);

		vec3 L = normalize(lightDir);
		vec3 H = normalize(V + L);

		float specularPower = 30.0;
		vec3 specularColor = vec3(0.04);
		vec3 diffuseColor = srgb2lin(diffuse_material.xyz);
		diffuseColor *= (1.0 - specularColor);

		vec3 color = diffuseColor * diffuse(N, L);
		color += specularColor * specularBlinnPhong(N, H, specularPower);

		outputColor += lin2srgb(color) * atten * light.color.xyz;
	}
	outFragcolor = vec4(outputColor * ssao, 1.0);
}