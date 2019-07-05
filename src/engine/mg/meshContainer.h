  #pragma once
#include "vulkan/deviceAllocator.h"
#include "mg/mgUtils.h"
#include "vulkan/vkContext.h"
#include <string>
#include <unordered_map>

namespace mg {

struct MeshId {
  uint32_t generation;
  uint32_t index;
};

struct Mesh {
  VkBuffer buffer;
  VkDeviceSize indicesOffset;
  uint32_t indexCount;
};

struct MeshData {
  Mesh mesh;
  mg::DeviceHeapAllocation heapAllocation;
};

struct CreateMeshInfo {
  std::string id;
  unsigned char *vertices, *indices;
  uint32_t verticesSizeInBytes, indicesSizeInBytes;
  uint32_t nrOfIndices;
};

class MeshContainer : mg::nonCopyable {
public:
  void createMeshContainer() {}
  MeshId createMesh(const CreateMeshInfo &createMeshInfo);
  Mesh getMesh(MeshId meshId) const;
  void removeMesh(MeshId meshId);

  void destroyMeshContainer();
  ~MeshContainer();

private:
  std::vector<MeshData> _idToMesh;
  std::vector<uint32_t> _freeIndices;
  std::vector<uint32_t> _generations;
};

} // namespace mg
