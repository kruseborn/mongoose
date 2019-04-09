#version 450

#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

const int kernelSize = 64;

layout (set = 0, binding = 0) uniform UBO  {
	mat4 projection;
	vec4 kernel[kernelSize];
	vec2 noiseScale;
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

layout (set = 1, binding = 0) uniform sampler2D samplerNormal;
layout (set = 2, binding = 0) uniform sampler2D samplerNoise;
layout (set = 3, binding = 0) uniform sampler2D samplerWordViewPosition;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;

const float radius = 2.0;
const float bias = 0.05;

void main()  {
	vec2 textCoord = inUV;
	textCoord.y = 1.0 - textCoord.y;

	// get fragPosition from depth value
	vec3 fragPos = texture(samplerWordViewPosition, textCoord).xyz;
	vec3 normal = sphericalToCartesian(texture(samplerNormal, textCoord).xy).xyz;
	vec3 randomVec = texture(samplerNoise, textCoord * ubo.noiseScale).xyz;
	
	// create TBN change-of-basis matrix: from tangent-space to view-space
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);  

	float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i) {
        // get sample position
        vec3 samplePosition = TBN * ubo.kernel[i].xyz;
		samplePosition = fragPos + samplePosition * radius; 
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePosition, 1.0);
        offset = ubo.projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xy = offset.xy * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
		offset.y = 1.0-offset.y;
        const float sampleDepth = texture(samplerWordViewPosition, offset.xy).z; // get depth value of kernel sample
        
        // range check & accumulate
		const float depthDiff = max(abs(fragPos.z - sampleDepth), 0.00001);
        const float rangeCheck = smoothstep(0.0, 1.0, radius / depthDiff);
        occlusion += (sampleDepth <= samplePosition.z + bias ? 1.0 : 0.0) * rangeCheck;           
    }

	occlusion = 1.0 - (occlusion / float(kernelSize));
	outFragcolor = vec4(occlusion, occlusion, occlusion, 1.0);
}