#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform Ubo {
  uint resolution;
} ubo;

@vert

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = vec4(in_position.xyz, 1.0);
}

@geom
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in gl_PerVertex
{
  vec4 gl_Position;
} gl_in[];

layout (location = 0) out vec3 gVoxelPos;

vec2 Project(in vec3 v, in int axis) { return axis == 0 ? v.yz : (axis == 1 ? v.xz : v.xy); }

void main() {
	//get projection axis
	vec3 axis_weight = abs(cross(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz));
	int axis = 2;
	if(axis_weight.x >= axis_weight.y && axis_weight.x > axis_weight.z) axis = 0;
	else if(axis_weight.y >= axis_weight.z && axis_weight.y > axis_weight.x) axis = 1;

	//project the positions
	vec3 pos0 = gl_in[0].gl_Position.xyz;
	vec3 pos1 = gl_in[1].gl_Position.xyz;
	vec3 pos2 = gl_in[2].gl_Position.xyz;


	gVoxelPos = (pos0 + 1.0f) * 0.5f * ubo.resolution;
	gl_Position = vec4(Project(pos0, axis), 1.0f, 1.0f);
	EmitVertex();
	gVoxelPos = (pos1 + 1.0f) * 0.5f * ubo.resolution;
	gl_Position = vec4(Project(pos1, axis), 1.0f, 1.0f);
	EmitVertex();
	gVoxelPos = (pos2 + 1.0f) * 0.5f * ubo.resolution;
	gl_Position = vec4(Project(pos2, axis), 1.0f, 1.0f);
	EmitVertex();
	EndPrimitive();
}

@frag
layout (set = 0, binding = 1) writeonly buffer Voxels {
  uvec2 values[1000000];
} voxels;

layout (location = 0) in vec3 gVoxelPos;

void main() {
  uvec3 ucolor = uvec3(100,0,0);
	uint cur = atomicAdd(voxels.values[0].x, 1) + 1;
  uvec3 uvoxel_pos = clamp(uvec3(gVoxelPos), uvec3(0u), uvec3(ubo.resolution - 1u));
  voxels.values[cur].x = uvoxel_pos.x | (uvoxel_pos.y << 12u) | ((uvoxel_pos.z & 0xffu) << 24u); //only have the last 8 bits of uvoxel_pos.z
  voxels.values[cur].y = ((uvoxel_pos.z >> 8u) << 28u) | ucolor.x | (ucolor.y << 8u) | (ucolor.z << 16u);
}