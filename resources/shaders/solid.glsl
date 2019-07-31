#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform Ubo {
	mat4 mvp;
} ubo;

@vert

struct StorageData {
	vec4 color;
	vec4 position;
};

layout(set = 0, binding = 1) readonly buffer Storage {
     StorageData storageData[];
} storage;

layout (location = 0) in vec3 in_position;
layout (location = 0) out vec4 out_color;

out gl_PerVertex {
	vec4 gl_Position;
};


void main() {
	vec3 position = in_position;
	position.xy += storage.storageData[gl_InstanceIndex].position.xy;

	out_color = storage.storageData[gl_InstanceIndex].color;
	gl_Position = ubo.mvp * vec4(position, 1.0);
}

@frag
layout (location = 0) in vec4 in_color;
layout (location = 0) out vec4 out_frag_color;

void main() {
	out_frag_color = in_color;
}
