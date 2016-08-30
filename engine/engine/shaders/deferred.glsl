#version 450

#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

struct Light {
	vec4 position;
	vec3 color;
	float radius;
};
#define lightCount 32

layout (set = 0, binding = 0) uniform UBO  {
	mat4 projection;
	mat4 view;
	mat4 model;
	mat4 mvp;
	Light lights[32];
} ubo;

@vert
layout (location = 0) in vec4 position;
layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outPosition;

void main() {
	outUV = position.zw;
	gl_Position = ubo.mvp * vec4(vec3(position.xy, 0.0), 1.0);
	gl_Position.y  = -gl_Position.y;
	outPosition = inverse(ubo.projection) * vec4(vec3(position.xy, 1.0), 1.0);
	outPosition.xyz /= outPosition.w;
}

@frag
#include "utils.glsl"
layout (set = 1, binding = 0) uniform sampler2D samplerNormal;
layout (set = 2, binding = 0) uniform sampler2D samplerAlbedo;
layout (set = 3, binding = 0) uniform sampler2D samplerSSAOBlured;
layout (set = 4, binding = 0) uniform sampler2D depthTexture;


layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inPosition;
layout (location = 0) out vec4 outFragcolor;

void main()  {
	vec2 textCoord = inUV;
	textCoord.y = 1.0 - textCoord.y;
	vec3 normal = sphericalToCartesian(texture(samplerNormal, textCoord).xy);
	vec4 albedo = texture(samplerAlbedo, textCoord);
	float ssao = texture(samplerSSAOBlured, textCoord).r;	

	vec3 viewRay = normalize(inPosition.xyz);
	float viewDepth = texture(depthTexture, textCoord).x;
	vec3 fragPos = viewRay * viewDepth;

	#define ambient 0.0
	
	// Ambient part
	vec3 fragcolor = albedo.rgb * ambient;
	for(int i = 0; i < lightCount; ++i) {
		// Vector to light
		vec3 L = (ubo.view * vec4(ubo.lights[i].position.xyz, 1.0)).xyz - fragPos;
		// Distance from light to fragment position
		float dist = length(L);
		L = normalize(L);
		float atten = ubo.lights[i].radius / (pow(dist, 2.0) + 1.0);

		vec3 N = normalize(normal);

		vec3 R = reflect(-L, N);
		float NdotR = max(0.0, dot(N, R));
		float NdotL = max(0.0, dot(N, L));
		vec3 diff = ubo.lights[i].color.xyz * albedo.rgb * NdotL * atten;

		fragcolor += diff;
	}    	
	outFragcolor = vec4(fragcolor*ssao, 1.0);
}