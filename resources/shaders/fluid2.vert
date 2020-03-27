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


    out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
  float x = -1.0 + float((gl_VertexIndex & 1) << 2);
  float y = -1.0 + float((gl_VertexIndex & 2) << 1);
  gl_Position = vec4(x, y, 0, 1);
}

