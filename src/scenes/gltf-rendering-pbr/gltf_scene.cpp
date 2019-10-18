#include "gltf_scene.h"
#include "gltf_rendering.h"
#include "mg/camera.h"
#include "mg/meshLoader.h"
#include "mg/mgAssert.h"
#include "mg/mgSystem.h"
#include "mg/tools.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include "vulkan/vkContext.h"
#include "vulkan/vkUtils.h"
#include <lodepng.h>
#include "vulkan/singleRenderpass.h"
#include <unordered_map>

static mg::Camera camera;
static mg::SingleRenderPass singleRenderPass;
static mg::MeshId meshId;
static std::unordered_map<std::string, mg::TextureId> nameToTextureId;

void initScene() {
  mg::initSingleRenderPass(&singleRenderPass);

  camera = mg::create3DCamera(glm::vec3{0.0f, 0.0f, 0.5f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});
  auto meshes = mg::parseGltf("box", mg::getDataPath() + "/water_bottle_gltf/", "WaterBottle.gltf");

  const auto mesh = meshes.meshes.front();
  mg::CreateMeshInfo createMeshInfo = {};
  createMeshInfo.id = "box";
  createMeshInfo.vertices = (uint8_t *)mesh.vertices.data();
  createMeshInfo.verticesSizeInBytes = mg::sizeofContainerInBytes(mesh.vertices);
  createMeshInfo.nrOfIndices = mesh.count;
  meshId = mg::mgSystem.meshContainer.createMesh(createMeshInfo);

  for (const auto &image : meshes.images) {
    std::vector<unsigned char> imageData;
    uint32_t width, height;
    const auto error = lodepng::decode(imageData, width, height, image.path + image.name);
    mgAssertDesc(error == 0, "decoder error " << error << ": " << lodepng_error_text(error));

    mg::CreateTextureInfo createTextureInfo = {};
    createTextureInfo.data = imageData.data();
    createTextureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createTextureInfo.id = image.name;
    createTextureInfo.size = {width, height, 1};
    createTextureInfo.sizeInBytes = mg::sizeofContainerInBytes(imageData);
    createTextureInfo.type = mg::TEXTURE_TYPE::TEXTURE_2D;
    auto textureId = mg::mgSystem.textureContainer.createTexture(createTextureInfo);
    nameToTextureId.emplace(image.name, textureId);
  }
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void destroyScene() {
  mg::waitForDeviceIdle();
  destroySingleRenderPass(&singleRenderPass);
}

void updateScene(const mg::FrameData &frameData) {
  if (frameData.resize) {
    mg::resizeSingleRenderPass(&singleRenderPass);
  }
  if (frameData.keys.r) {
    mg::mgSystem.pipelineContainer.resetPipelineContainer();
  }
  if (frameData.mouse.left)
    mg::handleTools(frameData, &camera);
  mg::setCameraTransformation(&camera);
}

bool renderScene(const mg::FrameData &frameData) {
  mg::beginRendering();
  mg::setFullscreenViewport();

  mg::beginSingleRenderPass(singleRenderPass);
  mg::RenderContext renderContext = {};
  renderContext.renderPass = singleRenderPass.vkRenderPass;

  drawGltfMesh(renderContext, meshId, camera, nameToTextureId);

  mg::endSingleRenderPass();

  return mg::endRendering();
}