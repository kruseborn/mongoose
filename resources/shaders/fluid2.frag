#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform Ubo { uvec4 screenSize; }
ubo;

struct StorageData {
  float color;
};

layout(set = 0, binding = 1) readonly buffer Storage { StorageData storageData[]; }
storage;


    layout(location = 0) out vec4 out_frag_color;

void main() {
  int index = int(gl_FragCoord.y) * int(ubo.screenSize.x + 2) + int(gl_FragCoord.x);
  out_frag_color = vec4(storage.storageData[index].color, 0, 0, 1);
}
