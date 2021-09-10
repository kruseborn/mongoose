#include "boids_scene.h"

#include "boids_rendering.h"
#include "mg/camera.h"
#include "mg/meshUtils.h"
#include "mg/mgSystem.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include "vulkan/singleRenderpass.h"
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>
#include "mg/texts.h"
#include "boids.h"

static mg::Camera camera;
static mg::SingleRenderPass singleRenderPass;
static mg::MeshId cubeId;


static void resizeCallback() {
  mg::resizeSingleRenderPass(&singleRenderPass);
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void initScene() {
  mg::initSingleRenderPass(&singleRenderPass);

  camera = mg::create3DCamera(glm::vec3{0.0f, 0.0f, -3.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});

  const auto cube = mg::createVolumeCube({0, 0, 0}, {10, 10, 10});

  mg::CreateMeshInfo createMeshInfo = {.id = "box",
                                       .vertices = (uint8_t *)cube.vertices,
                                       .indices = (uint8_t *)cube.indices,
                                       .verticesSizeInBytes = mg::sizeofArrayInBytes(cube.vertices),
                                       .indicesSizeInBytes = mg::sizeofArrayInBytes(cube.indices),
                                       .nrOfIndices = mg::countof(cube.indices)};

  cubeId = mg::mgSystem.meshContainer.createMesh(createMeshInfo);

  mg::mgSystem.textureContainer.setupDescriptorSets();
  mg::vkContext.swapChain->resizeCallack = resizeCallback;

}

void destroyScene() {
  mg::waitForDeviceIdle();
  destroySingleRenderPass(&singleRenderPass);
}

void updateScene(const mg::FrameData &frameData) {
  static bool b = false;
  if (!b) {
    init(float(frameData.width), float(frameData.height));
    b = true;
  }

  if (frameData.keys.r) {
    mg::mgSystem.pipelineContainer.resetPipelineContainer();
  }
  if (frameData.mouse.left)
    mg::handleTools(frameData, &camera);
  mg::setCameraTransformation(&camera);
}

void renderScene(const mg::FrameData &frameData) {
  mg::beginRendering();
  mg::setFullscreenViewport();

  mg::beginSingleRenderPass(singleRenderPass);
  {
    mg::RenderContext renderContext = {};
    renderContext.renderPass = singleRenderPass.vkRenderPass;

    renderContext.projection = glm::ortho(0.0f, float(frameData.width), 0.0f, float(frameData.height), -10.0f, 10.0f);
    renderContext.view = glm::lookAt(camera.position, camera.aim, camera.up);

    renderContext.renderPass = singleRenderPass.vkRenderPass;
    std::span<glm::vec2> positions = simulate(float(frameData.width), float(frameData.height), frameData.dt);

    mg::renderCubes(renderContext, frameData, positions, cubeId);

  }
  mg::endSingleRenderPass();

  mg::endRendering();
}