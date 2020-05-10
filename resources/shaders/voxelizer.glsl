#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform Ubo {
  float resolution;
} ubo;

@vert

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec3 position = in_position + in_normal*0.000000000001 + vec3(in_tex, 0.0)*0.000000000001;
	gl_Position = vec4(position, 1.0);
}

@geom
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in gl_PerVertex
{
  vec4 gl_Position;
} gl_in[];

layout (location = 0) out vec3 gVoxelPos;

vec2 project(in vec3 v, in int axis) { return axis == 0 ? v.yz : (axis == 1 ? v.xz : v.xy); }

void main() {
	vec3 axis_weight = abs(cross(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz));
	int axis = 2;
  if(axis_weight.x >= axis_weight.y && axis_weight.x >= axis_weight.z) axis = 0;
  else if(axis_weight.y >= axis_weight.z && axis_weight.y >= axis_weight.x) axis = 1;

  vec3 pos0 = gl_in[0].gl_Position.xyz;
  vec3 pos1 = gl_in[1].gl_Position.xyz;
  vec3 pos2 = gl_in[2].gl_Position.xyz;

  gVoxelPos = (pos0 + 1.0) * 0.5 * ubo.resolution;
	gl_Position = vec4(project(pos0, axis), 1.0, 1.0);
	EmitVertex();

  gVoxelPos = (pos1 + 1.0) * 0.5 * ubo.resolution;
	gl_Position = vec4(project(pos1, axis), 1.0, 1.0);
	EmitVertex();

  gVoxelPos = (pos2 + 1.0) * 0.5 * ubo.resolution;
	gl_Position = vec4(project(pos2, axis), 1.0, 1.0);
	EmitVertex();

  EndPrimitive();
}

@frag
layout (set = 0, binding = 1) writeonly buffer Voxels {
  uvec4 count;
  uvec4 values[10000];
} voxels;

layout (location = 0) in vec3 gVoxelPos;

void main() {
	uint cur = atomicAdd(voxels.count.x, 1);
  uvec3 uvoxel_pos = clamp(uvec3(gVoxelPos), uvec3(0u), uvec3(ubo.resolution - 1u));
  voxels.values[cur] = uvec4(uvoxel_pos, cur);
}