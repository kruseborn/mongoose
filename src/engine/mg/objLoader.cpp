#include "meshLoader.h"
#include "mg/mgSystem.h"
#include <glm/glm.hpp>
#include <unordered_set>
#include <vector>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include <stb_image.h>
#include <tiny_obj_loader.h>

namespace mg {

static std::vector<unsigned char> resizeRGBToRGBA(unsigned char *rgb, uint32_t size) {
  std::vector<unsigned char> rgba;
  rgba.resize(size * 4);
  for (uint32_t i = 0; i < size; i++) {
    rgba[i * 4 + 0] = rgb[i * 3 + 0];
    rgba[i * 4 + 1] = rgb[i * 3 + 0];
    rgba[i * 4 + 2] = rgb[i * 3 + 0];
    rgba[i * 4 + 3] = 1.0f;
  }
  return rgba;
}

static std::string getBaseDir(const std::string &filepath) {
  if (filepath.find_last_of("/\\") != std::string::npos)
    return filepath.substr(0, filepath.find_last_of("/\\"));
  assert(false);
  return "";
}

static std::string getName(const std::string &filepath) {
  if (filepath.find_last_of("/\\") != std::string::npos)
    return filepath.substr(filepath.find_last_of("/\\") + 1, std::string::npos);
  assert(false);
  return "";
}

static bool FileExists(const std::string &abs_filename) {
  bool ret;
  FILE *fp = fopen(abs_filename.c_str(), "rb");
  if (fp) {
    ret = true;
    fclose(fp);
  } else {
    ret = false;
  }

  return ret;
}

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
  float v10[3];
  v10[0] = v1[0] - v0[0];
  v10[1] = v1[1] - v0[1];
  v10[2] = v1[2] - v0[2];

  float v20[3];
  v20[0] = v2[0] - v0[0];
  v20[1] = v2[1] - v0[1];
  v20[2] = v2[2] - v0[2];

  N[0] = v20[1] * v10[2] - v20[2] * v10[1];
  N[1] = v20[2] * v10[0] - v20[0] * v10[2];
  N[2] = v20[0] * v10[1] - v20[1] * v10[0];

  float len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
  if (len2 > 0.0f) {
    float len = sqrtf(len2);

    N[0] /= len;
    N[1] /= len;
    N[2] /= len;
  }
}

static void normalizeVector(glm::vec3 &v) {
  float len2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
  if (len2 > 0.0f) {
    float len = sqrtf(len2);

    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
  }
}

static bool hasSmoothingGroup(const tinyobj::shape_t &shape) {
  for (uint32_t i = 0; i < shape.mesh.smoothing_group_ids.size(); i++) {
    if (shape.mesh.smoothing_group_ids[i] > 0) {
      return true;
    }
  }
  return false;
}

static void computeSmoothingNormals(const tinyobj::attrib_t &attrib, const tinyobj::shape_t &shape,
                                    std::map<int, glm::vec3> &smoothVertexNormals) {
  smoothVertexNormals.clear();
  std::map<int, glm::vec3>::iterator iter;

  for (uint32_t f = 0; f < shape.mesh.indices.size() / 3; f++) {
    // Get the three indexes of the face (all faces are triangular)
    tinyobj::index_t idx0 = shape.mesh.indices[3 * f + 0];
    tinyobj::index_t idx1 = shape.mesh.indices[3 * f + 1];
    tinyobj::index_t idx2 = shape.mesh.indices[3 * f + 2];

    // Get the three vertex indexes and coordinates
    int vi[3];     // indexes
    float v[3][3]; // coordinates

    for (int k = 0; k < 3; k++) {
      vi[0] = idx0.vertex_index;
      vi[1] = idx1.vertex_index;
      vi[2] = idx2.vertex_index;
      assert(vi[0] >= 0);
      assert(vi[1] >= 0);
      assert(vi[2] >= 0);

      v[0][k] = attrib.vertices[3 * vi[0] + k];
      v[1][k] = attrib.vertices[3 * vi[1] + k];
      v[2][k] = attrib.vertices[3 * vi[2] + k];
    }

    // Compute the normal of the face
    float normal[3];
    CalcNormal(normal, v[0], v[1], v[2]);

    // Add the normal to the three vertexes
    for (uint32_t i = 0; i < 3; ++i) {
      iter = smoothVertexNormals.find(vi[i]);
      if (iter != smoothVertexNormals.end()) {
        // add
        iter->second[0] += normal[0];
        iter->second[1] += normal[1];
        iter->second[2] += normal[2];
      } else {
        smoothVertexNormals[vi[i]][0] = normal[0];
        smoothVertexNormals[vi[i]][1] = normal[1];
        smoothVertexNormals[vi[i]][2] = normal[2];
      }
    }

  } // f

  // Normalize the normals, that is, make them unit vectors
  for (iter = smoothVertexNormals.begin(); iter != smoothVertexNormals.end(); iter++) {
    normalizeVector(iter->second);
  }
}

static ObjMeshes readObjFromBinary(std::ifstream &offsets, const std::string &name) {
  ObjMeshes objMeshes = {};
  const auto binary = mg::readBinaryFromDisc(name + ".bin");
  const auto materials = mg::readBinaryFromDisc(name + ".mat");
  const auto mesh = mg::readBinaryFromDisc(name + ".mesh");

  objMeshes.materials.resize(materials.size() / sizeof(ObjMaterial));
  std::memcpy(objMeshes.materials.data(), materials.data(), mg::sizeofContainerInBytes(materials));

  objMeshes.meshes.resize(mesh.size() / sizeof(ObjMesh));
  std::memcpy(objMeshes.meshes.data(), mesh.data(), mg::sizeofContainerInBytes(mesh));

  uint32_t currentOffset = 0;
  for (uint32_t i = 0; i < objMeshes.meshes.size(); i++) {
    uint32_t vertexsize;
    char delimiter;
    offsets >> vertexsize >> delimiter;
    mgAssert((currentOffset + vertexsize) <= binary.size());
    mg::CreateMeshInfo createMeshInfo = {};
    createMeshInfo.id = "mesh" + std::to_string(i);
    createMeshInfo.vertices = (unsigned char *)&binary[currentOffset];
    createMeshInfo.verticesSizeInBytes = vertexsize;
    createMeshInfo.nrOfIndices = vertexsize / (sizeof(float)* (3+3+2));

    currentOffset += vertexsize;

    const auto meshId = mg::mgSystem.meshContainer.createMesh(createMeshInfo);
    objMeshes.meshes[i].id = meshId;
  }
  return objMeshes;
}

struct Outputs {
  std::ofstream binary, sizes, materials, mesh;
};

ObjMeshes loadObjFromFile(const std::string &filename) {
  ObjMeshes tinyObjMeshes = {};
  Outputs outputs = {};
  const auto name = getName(filename);
  std::ifstream ifText(name + ".txt");
  if (ifText.good()) {
    return readObjFromBinary(ifText, name);
  } else {
    outputs.binary.open(name + ".bin", std::fstream::binary);
    outputs.sizes.open(name + ".txt");
    outputs.materials.open(name + ".mat", std::fstream::binary);
    outputs.mesh.open(name + ".mesh", std::fstream::binary);
  }

  std::vector<tinyobj::material_t> materials;
  std::unordered_set<std::string> loadedTextures;

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;

  std::string base_dir = getBaseDir(filename);
  if (base_dir.empty()) {
    base_dir = ".";
  }
#ifdef _WIN32
  base_dir += "\\";
#else
  base_dir += "/";
#endif

  const auto start = mg::timer::now();
  std::string warn;
  std::string err;
  bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), base_dir.c_str());
  if (!warn.empty()) {
    printf("WARN: %s\n", warn.c_str());
    mgAssert(false);
  }
  if (!err.empty()) {
    printf("ERR: %s\n", err.c_str());
    mgAssert(false);
  }

  if (!ret) {
    printf("Failed to load %s\n", filename.c_str());
    mgAssert(false);
  }

  const auto end = mg::timer::now();
  printf("Parsing time: %ul [ms]\n", mg::timer::durationInMs(start, end));

  printf("# of vertices  = %d\n", (int32_t)(attrib.vertices.size()) / 3);
  printf("# of normals   = %d\n", (int32_t)(attrib.normals.size()) / 3);
  printf("# of texcoords = %d\n", (int32_t)(attrib.texcoords.size()) / 2);
  printf("# of materials = %d\n", (int32_t)materials.size());
  printf("# of shapes    = %d\n", (int32_t)shapes.size());

  // Append `default` material
  materials.push_back(tinyobj::material_t());

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%d].diffuse_texname = %s\n", int(i), materials[i].diffuse_texname.c_str());
  }

  // Load diffuse textures
  {
    for (size_t m = 0; m < materials.size(); m++) {
      tinyobj::material_t *mp = &materials[m];

      if (mp->diffuse_texname.length() > 0) {
        // Only load the texture if it is not already loaded
        if (loadedTextures.find(mp->diffuse_texname) == std::end(loadedTextures)) {
          uint32_t texture_id;
          int w, h;
          int comp;

          std::string textureFilename = mp->diffuse_texname;
          if (!FileExists(textureFilename)) {
            // Append base dir.
            textureFilename = base_dir + mp->diffuse_texname;
            if (!FileExists(textureFilename)) {
              printf("Unable to find file: %s\n", mp->diffuse_texname.c_str());
              mgAssert(false);
            }
          }

          unsigned char *image = stbi_load(textureFilename.c_str(), &w, &h, &comp, STBI_default);
          if (!image) {
            printf("Unable to load texture: %s\n", textureFilename.c_str());
            mgAssert(false);
          }
          printf("Loaded texture: %s, w = %d, h = %d, comp = %d\n", textureFilename.c_str(), w, h, comp);

          mg::CreateTextureInfo createTextureInfo = {};
          createTextureInfo.id = mp->diffuse_texname;
          createTextureInfo.textureSamplers = {mg::TEXTURE_SAMPLER::LINEAR_CLAMP_TO_BORDER};
          createTextureInfo.type = mg::TEXTURE_TYPE::TEXTURE_2D;
          createTextureInfo.size = {uint32_t(w), uint32_t(h), 1};
          createTextureInfo.format = VK_FORMAT_R8G8B8A8_UINT;

          if (comp == 3) {
            const auto rgba = resizeRGBToRGBA(image, w * h);
            createTextureInfo.data = (void *)rgba.data();
            mg::mgSystem.textureContainer.createTexture(createTextureInfo);
          } else if (comp == 4) {
            createTextureInfo.data = image;
            mg::mgSystem.textureContainer.createTexture(createTextureInfo);
          } else
            mgAssert(false);
          loadedTextures.insert(mp->diffuse_texname);
          stbi_image_free(image);
        }
      }
    }
  }

  {
    for (uint32_t s = 0; s < uint32_t(shapes.size()); s++) {
      ObjMesh o = {};
      std::vector<float> buffer; // pos(3float), normal(3float)

      // Check for smoothing group and compute smoothing normals
      std::map<int, glm::vec3> smoothVertexNormals;
      if (hasSmoothingGroup(shapes[s]) > 0) {
        printf("Compute smoothingNormal for shape [%d]", s);
        computeSmoothingNormals(attrib, shapes[s], smoothVertexNormals);
      }

      for (size_t f = 0; f < shapes[s].mesh.indices.size() / 3; f++) {
        tinyobj::index_t idx0 = shapes[s].mesh.indices[3 * f + 0];
        tinyobj::index_t idx1 = shapes[s].mesh.indices[3 * f + 1];
        tinyobj::index_t idx2 = shapes[s].mesh.indices[3 * f + 2];

        int current_material_id = shapes[s].mesh.material_ids[f];

        if ((current_material_id < 0) || (current_material_id >= static_cast<int>(materials.size()))) {
          // Invaid material ID. Use default material.
          current_material_id = materials.size() - 1; // Default material is added to the last item in `materials`.
        }
        float diffuse[3];
        for (size_t i = 0; i < 3; i++) {
          diffuse[i] = materials[current_material_id].diffuse[i];
        }
        float tc[3][2];
        if (attrib.texcoords.size() > 0) {
          if ((idx0.texcoord_index < 0) || (idx1.texcoord_index < 0) || (idx2.texcoord_index < 0)) {
            // face does not contain valid uv index.
            tc[0][0] = 0.0f;
            tc[0][1] = 0.0f;
            tc[1][0] = 0.0f;
            tc[1][1] = 0.0f;
            tc[2][0] = 0.0f;
            tc[2][1] = 0.0f;
          } else {
            assert(attrib.texcoords.size() > size_t(2 * idx0.texcoord_index + 1));
            assert(attrib.texcoords.size() > size_t(2 * idx1.texcoord_index + 1));
            assert(attrib.texcoords.size() > size_t(2 * idx2.texcoord_index + 1));

            // Flip Y coord.
            tc[0][0] = attrib.texcoords[2 * idx0.texcoord_index];
            tc[0][1] = 1.0f - attrib.texcoords[2 * idx0.texcoord_index + 1];
            tc[1][0] = attrib.texcoords[2 * idx1.texcoord_index];
            tc[1][1] = 1.0f - attrib.texcoords[2 * idx1.texcoord_index + 1];
            tc[2][0] = attrib.texcoords[2 * idx2.texcoord_index];
            tc[2][1] = 1.0f - attrib.texcoords[2 * idx2.texcoord_index + 1];
          }
        } else {
          tc[0][0] = 0.0f;
          tc[0][1] = 0.0f;
          tc[1][0] = 0.0f;
          tc[1][1] = 0.0f;
          tc[2][0] = 0.0f;
          tc[2][1] = 0.0f;
        }

        float v[3][3];
        for (int k = 0; k < 3; k++) {
          int f0 = idx0.vertex_index;
          int f1 = idx1.vertex_index;
          int f2 = idx2.vertex_index;
          assert(f0 >= 0);
          assert(f1 >= 0);
          assert(f2 >= 0);

          v[0][k] = attrib.vertices[3 * f0 + k];
          v[1][k] = attrib.vertices[3 * f1 + k];
          v[2][k] = attrib.vertices[3 * f2 + k];
        }

        float n[3][3];
        {
          bool invalid_normal_index = false;
          if (attrib.normals.size() > 0) {
            int nf0 = idx0.normal_index;
            int nf1 = idx1.normal_index;
            int nf2 = idx2.normal_index;

            if ((nf0 < 0) || (nf1 < 0) || (nf2 < 0)) {
              // normal index is missing from this face.
              invalid_normal_index = true;
            } else {
              for (int k = 0; k < 3; k++) {
                assert(size_t(3 * nf0 + k) < attrib.normals.size());
                assert(size_t(3 * nf1 + k) < attrib.normals.size());
                assert(size_t(3 * nf2 + k) < attrib.normals.size());
                n[0][k] = attrib.normals[3 * nf0 + k];
                n[1][k] = attrib.normals[3 * nf1 + k];
                n[2][k] = attrib.normals[3 * nf2 + k];
              }
            }
          } else {
            invalid_normal_index = true;
          }

          if (invalid_normal_index && !smoothVertexNormals.empty()) {
            // Use smoothing normals
            int f0 = idx0.vertex_index;
            int f1 = idx1.vertex_index;
            int f2 = idx2.vertex_index;

            if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
              n[0][0] = smoothVertexNormals[f0][0];
              n[0][1] = smoothVertexNormals[f0][1];
              n[0][2] = smoothVertexNormals[f0][2];

              n[1][0] = smoothVertexNormals[f1][0];
              n[1][1] = smoothVertexNormals[f1][1];
              n[1][2] = smoothVertexNormals[f1][2];

              n[2][0] = smoothVertexNormals[f2][0];
              n[2][1] = smoothVertexNormals[f2][1];
              n[2][2] = smoothVertexNormals[f2][2];

              invalid_normal_index = false;
            }
          }

          if (invalid_normal_index) {
            // compute geometric normal
            CalcNormal(n[0], v[0], v[1], v[2]);
            n[1][0] = n[0][0];
            n[1][1] = n[0][1];
            n[1][2] = n[0][2];
            n[2][0] = n[0][0];
            n[2][1] = n[0][1];
            n[2][2] = n[0][2];
          }
        }

        for (int k = 0; k < 3; k++) {
          buffer.push_back(v[k][0]);
          buffer.push_back(v[k][1]);
          buffer.push_back(v[k][2]);
          buffer.push_back(n[k][0]);
          buffer.push_back(n[k][1]);
          buffer.push_back(n[k][2]);

          buffer.push_back(tc[k][0]);
          buffer.push_back(tc[k][1]);
        }
      }

      // OpenGL viewer does not support texturing with per-face material.
      if (shapes[s].mesh.material_ids.size() > 0 && shapes[s].mesh.material_ids.size() > s) {
        o.materialId = shapes[s].mesh.material_ids[0]; // use the material ID
                                                        // of the first face.
      } else {
        o.materialId = materials.size() - 1; // = ID for default material.
      }
      printf("shape[%d] material_id %d\n", int(s), int(o.materialId));

      if (buffer.size() > 0) {
        const auto nrOfIndices = buffer.size() / (3 + 3 + 2); // 3:vtx, 3:normal, 2:texcoord

        mg::CreateMeshInfo createMeshInfo = {};
        createMeshInfo.vertices = (uint8_t *)buffer.data();
        createMeshInfo.verticesSizeInBytes = mg::sizeofContainerInBytes(buffer);
        createMeshInfo.nrOfIndices = nrOfIndices;

        o.id = mg::mgSystem.meshContainer.createMesh(createMeshInfo);
        printf("shape[%d] # of triangles = %d\n", static_cast<int>(s), nrOfIndices);

        outputs.binary.write((char *)buffer.data(), mg::sizeofContainerInBytes(buffer));
        outputs.sizes << mg::sizeofContainerInBytes(buffer) << '/';
      }

      tinyObjMeshes.meshes.push_back(o);
    }
  }

  for (uint32_t i = 0; i < materials.size(); i++) {
    ObjMaterial material = {};
    const auto &m = materials[i];
    material.diffuse = {m.diffuse[0], m.diffuse[1], m.diffuse[2], 1.0f};
    tinyObjMeshes.materials.push_back(material);
  }

  outputs.materials.write((char *)tinyObjMeshes.materials.data(), mg::sizeofContainerInBytes(tinyObjMeshes.materials));
  outputs.mesh.write((char *)tinyObjMeshes.meshes.data(), mg::sizeofContainerInBytes(tinyObjMeshes.meshes));
  
  return tinyObjMeshes;
}

} // namespace mg