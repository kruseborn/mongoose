#include "octree_scene.h"

#include "empty_render_pass.h"
#include "mg/camera.h"
#include "mg/meshLoader.h"
#include "mg/meshUtils.h"
#include "mg/mgSystem.h"
#include "mg/texts.h"
#include "mg/window.h"
#include "render_sdf.h"
#include "rendering/rendering.h"
#include "sdf.h"
#include "voxelizer.h"
#include "vulkan/singleRenderpass.h"
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>

static mg::Camera camera;
static mg::SingleRenderPass singleRenderPass;
static mg::EmptyRenderPass emptyRenderPass;
static mg::MeshId meshId;
static mg::MeshId sphereId;

static void resizeCallback() {
  mg::resizeSingleRenderPass(&singleRenderPass);
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

static mg::TextureId uploadVolumeToGpu(const float *data, const glm::uvec3 &size) {
  mg::CreateTextureInfo createTextureInfo = {};
  createTextureInfo.id = "sphere";
  createTextureInfo.type = mg::TEXTURE_TYPE::TEXTURE_3D;
  createTextureInfo.size = {size.x, size.y, size.z};
  createTextureInfo.data = (void *)data;
  createTextureInfo.sizeInBytes = size.x*size.y*size.z * sizeof(float);
  createTextureInfo.format = VK_FORMAT_R32_SFLOAT;

  return mg::mgSystem.textureContainer.createTexture(createTextureInfo);
}

void initScene() {
  mg::initSingleRenderPass(&singleRenderPass);
  mg::initEmptyRenderPass(&emptyRenderPass, mg::vkContext.screen.width);

  camera = mg::create3DCamera(glm::vec3{0.0f, 0.0f, -20.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});

  const auto cube = mg::createVolumeCube({-1.0f, -1.0f, -1.0f}, {2, 2, 2});

  mg::CreateMeshInfo createMeshInfo = {.id = "box",
                                       .vertices = (uint8_t *)cube.vertices,
                                       .indices = (uint8_t *)cube.indices,
                                       .verticesSizeInBytes = mg::sizeofArrayInBytes(cube.vertices),
                                       .indicesSizeInBytes = mg::sizeofArrayInBytes(cube.indices),
                                       .nrOfIndices = mg::countof(cube.indices)};

  meshId = mg::mgSystem.meshContainer.createMesh(createMeshInfo);
  auto sphere = mg::loadObjFromFile(mg::getDataPath() + "sphere/sphere.obj");
  sphereId = sphere.meshes.front().id;

  //auto buffer = mg::readBinaryFromDisc(mg::getDataPath() + "out.bin");

  auto sdf = objToSdf(mg::getDataPath() + "sphereTriangles.obj", 0.1f, 2);
  uploadVolumeToGpu((float *)(sdf.grid.data()), glm::uvec3{sdf.x, sdf.y, sdf.z});

  mg::mgSystem.textureContainer.setupDescriptorSets();
  mg::vkContext.swapChain->resizeCallack = resizeCallback;
}

void destroyScene() {
  mg::waitForDeviceIdle();
  destroySingleRenderPass(&singleRenderPass);
  mg::destroyEmptyRenderPass(&emptyRenderPass);
}

void updateScene(const mg::FrameData &frameData) {
  if (frameData.keys.r) {
    mg::mgSystem.pipelineContainer.resetPipelineContainer();
  }
  if (frameData.mouse.left)
    mg::handleTools(frameData, &camera);
  mg::setCameraTransformation(&camera);
}

void renderScene(const mg::FrameData &frameData) {
  mg::Texts texts = {};
  char fps[50];
  snprintf(fps, sizeof(fps), "Fps: %u", uint32_t(frameData.fps));
  mg::pushText(&texts, {fps});

  mg::RenderContext renderContext1 = {.renderPass = emptyRenderPass.vkRenderPass};
  // auto cubes = mg::createOctreeFromMesh(renderContext1, emptyRenderPass, sphereId);
  // mg::buildOctree(cubes, {0, 0, 0}, 8);

  mg::beginRendering();
  mg::setFullscreenViewport();

  mg::beginSingleRenderPass(singleRenderPass);
  {
    mg::RenderContext renderContext = {};
    renderContext.renderPass = singleRenderPass.vkRenderPass;

    renderContext.projection = glm::perspective(
        glm::radians(camera.fov), mg::vkContext.screen.width / float(mg::vkContext.screen.height), 0.1f, 1000.f);
    renderContext.view = glm::lookAt(camera.position, camera.aim, camera.up);

    renderContext.renderPass = singleRenderPass.vkRenderPass;

    static float rotAngle = 0;
    rotAngle += frameData.dt * 20.0f;
    glm::mat4 rotation = glm::rotate(glm::mat4(1), rotAngle, {1, 1, 0});
    // renderMesh(renderContext, meshId, rotation, {1, 0, 0, 1});

    // mg::renderCubes(renderContext, cubes, sphereId);
    mg::renderSdf(renderContext, {}, frameData);

    mg::renderText(renderContext, texts);
  }
  mg::endSingleRenderPass();

  mg::endRendering();
}