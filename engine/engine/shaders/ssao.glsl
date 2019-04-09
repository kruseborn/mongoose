#version 450

#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

#define kernelSize 16


layout (set = 0, binding = 0) uniform UBO  {
	mat4 projection;
	mat4 mvp;
	vec4 kernel[kernelSize];
	vec2 noiseScale;
} ubo;

@vert
layout (location = 0) in vec4 position;
layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outPosition;

void main() {
	outUV = position.zw;
	gl_Position = ubo.mvp * vec4(vec3(position.xy, 0.0), 1.0);
	gl_Position.y = -gl_Position.y;

	outPosition = inverse(ubo.projection) * vec4(vec3(position.xy, 1.0), 1.0);
	outPosition.xyz /= outPosition.w;
}

@frag
#include "utils.glsl"

layout (set = 1, binding = 0) uniform sampler2D samplerNormal;
layout (set = 2, binding = 0) uniform sampler2D samplerNoise;
layout (set = 3, binding = 0) uniform sampler2D samplerDepth;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inPosition;

layout (location = 0) out vec4 outFragcolor;

#define radius 1

void main()  {
	vec2 textCoord = inUV;
	textCoord.y = 1.0 - textCoord.y;
	vec3 normal = sphericalToCartesian(texture(samplerNormal, textCoord).xy);

	vec3 viewRay = normalize(inPosition.xyz);
	float viewDepth = texture(samplerDepth, textCoord).x;
	vec3 fragPos = viewRay * viewDepth;

	vec2 noiseTexCoords = vec2(textureSize(samplerDepth, 0)) / vec2(textureSize(samplerNoise, 0));

	vec3 randomVec = texture(samplerNoise, textCoord * noiseTexCoords).xyz;

	float fragDepth = length(fragPos);
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);  

	float occlusion = 0.0;
	for (int i = 0; i < kernelSize; i++) {
		//Get sample position
		vec3 samplePosition = TBN * ubo.kernel[i].xyz;
		samplePosition = fragPos + samplePosition * radius;

		vec4 offset = vec4(samplePosition, 1.0);
		offset = ubo.projection * offset; // from view to clip-space
		offset.xyz /= offset.w; // perspective divide
		offset.xy = offset.xy * 0.5 + 0.5; // transform to range 0.0 - 1.0
		offset.y = 1.0-offset.y;

		float sampleDepth = texture(samplerDepth, offset.xy).x;

		float samplerpositionDepth = length(samplePosition);
		//Range check and accumulate

		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragDepth - sampleDepth));
		occlusion += (sampleDepth >= (samplerpositionDepth-0.2) ? 0.0 : 1.0) * rangeCheck; 
	}

	occlusion = 1.0 - (occlusion / float(kernelSize));
	outFragcolor = vec4(vec3(occlusion), 1.0);
}