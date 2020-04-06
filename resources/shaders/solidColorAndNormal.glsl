#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform Ubo {
	mat4 mvp;
	vec4 color;
} ubo;

@vert

layout (location = 0) in vec4 in_position;
layout (location = 1) in vec4 in_normal;
layout (location = 0) out vec4 out_normal;
layout (location = 1) out vec4 out_position;


out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec4 position = in_position;
	out_position = in_position;
	out_normal = vec4(in_normal.xyz, 0.0);
	gl_Position = ubo.mvp * position;
}

@frag
layout (location = 0) in vec3 in_normal;
layout (location = 1) in vec4 in_position;
layout (location = 0) out vec4 out_frag_color;

void main() {
	vec3 mat = vec3(1,1,1);

	vec3 grass = vec3(0,0.12,0.0);
	vec3 water = vec3(0,0,0.18);
	vec3 dirt = vec3(0.10, 0.05, 0.0);
	vec3 snow = vec3(0.20);
	if(in_position.y > 10.5)
		mat = mix(dirt, snow, clamp(in_position.y - 10.5, 0, 1));
	else if(in_position.y > 5.0)
		mat = mix(grass, dirt, clamp(in_position.y - 5.0, 0, 1));
	else
		mat = mix(water, grass, clamp(in_position.y - 3.5, 0, 1));

	vec3 nor = in_normal.xyz;
	vec3 ldir = normalize(vec3(100, 100, -30) - in_position.xyz);
	float diff = clamp(dot(ldir, in_normal.xyz), 0, 1);
	vec3 lColor = vec3(1, 0.9, 0.05);
	vec3 ambient = vec3(0.9, 0.9, 0.3);

	vec3 sun_dir = normalize(vec3(0.8, 0.4, 0.2));
	float sun_dif = clamp(dot(nor, sun_dir), 0, 1);
	float sky_dif = clamp(0.5 + 0.5 *dot(nor, vec3(0,1,0)), 0, 1);
	float bou_dif = clamp(0.5 + 0.5 *dot(nor, vec3(0,-1,0)), 0, 1);

	vec3 col = mat*vec3(4.0, 2.0, 1.0) * sun_dif;
	col += mat*vec3(0.5, 0.8, 0.9) * sky_dif;
	col += mat*vec3(0.5, 0.3, 0.0) * bou_dif;

	col = pow(col, vec3(0.4545));
	out_frag_color = vec4(col, 1.0);
}