#include "boids_scene.h"

#include "boids.h"
#include "mg/camera.h"
#include "mg/meshUtils.h"
#include "mg/mgSystem.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include "vulkan/singleRenderpass.h"
#include <glm/gtc/matrix_transform.hpp>

struct World {
  mg::Camera camera;
  mg::SingleRenderPass singleRenderPass;
  mg::MeshId meshId;
  bds::Boids boids;
};

static World world{};

static void resizeCallback() {
  mg::resizeSingleRenderPass(&world.singleRenderPass);
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void initScene() {
  mg::initSingleRenderPass(&world.singleRenderPass);

  world.camera = mg::create3DCamera(glm::vec3{0.0f, 0.0f, -260.0f}, glm::vec3{0.0f, 0.0f, 0.0f},
                              glm::vec3{0.0f, 1.0f, 0.0f});

  const auto cube = mg::createVolumeCube({-0.5f, -0.5f, -0.5f}, {1, 1, 1});

  mg::CreateMeshInfo createMeshInfo = {.id = "box",
                                       .vertices = (uint8_t *)cube.vertices,
                                       .indices = (uint8_t *)cube.indices,
                                       .verticesSizeInBytes = mg::sizeofArrayInBytes(cube.vertices),
                                       .indicesSizeInBytes = mg::sizeofArrayInBytes(cube.indices),
                                       .nrOfIndices = mg::countof(cube.indices)};

  world.meshId = mg::mgSystem.meshContainer.createMesh(createMeshInfo);

  mg::mgSystem.textureContainer.setupDescriptorSets();
  mg::vkContext.swapChain->resizeCallack = resizeCallback;

  world.boids = bds::create();
}

void destroyScene() {
  mg::waitForDeviceIdle();
  destroySingleRenderPass(&world.singleRenderPass);
}

void updateScene(const mg::FrameData &frameData) {
  if (frameData.keys.r) {
    mg::mgSystem.pipelineContainer.resetPipelineContainer();
  }
  if (frameData.mouse.left)
    mg::handleTools(frameData, &world.camera);
  mg::setCameraTransformation(&world.camera);

  bds::update(world.boids, frameData);
}

void renderScene(const mg::FrameData &frameData) {
  mg::beginRendering();
  mg::setFullscreenViewport();

  mg::beginSingleRenderPass(world.singleRenderPass);
  {
    mg::RenderContext renderContext = {};
    renderContext.renderPass = world.singleRenderPass.vkRenderPass;

    renderContext.projection = glm::perspective(
        glm::radians(world.camera.fov), mg::vkContext.screen.width / float(mg::vkContext.screen.height),
        0.1f, 1000.f);
    renderContext.view = glm::lookAt(world.camera.position, world.camera.aim, world.camera.up);

    renderContext.renderPass = world.singleRenderPass.vkRenderPass;

    bds::render(world.boids, frameData, renderContext, world.meshId);
  }
  mg::endSingleRenderPass();

  mg::endRendering();
}