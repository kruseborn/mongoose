#include "boids_scene.h"

#include "boids.h"
#include "boids_simd.h"
#include "mg/camera.h"
#include "mg/meshUtils.h"
#include "mg/mgSystem.h"
#include "mg/texts.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include "vulkan/singleRenderpass.h"
#include <glm/gtc/matrix_transform.hpp>

struct World {
  mg::Camera camera;
  mg::SingleRenderPass singleRenderPass;
  mg::MeshId meshId;
  bds::Boids boids;
  bds_simd::Boids boids_simd;
};

static World world{};
static BoidsTime boidsTime = {};

static void resizeCallback() {
  mg::resizeSingleRenderPass(&world.singleRenderPass);
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void initScene() {
  mg::initSingleRenderPass(&world.singleRenderPass);

  world.camera = mg::create3DCamera(glm::vec3{0.0f, 0.0f, -260.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});
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

  world.boids_simd = bds_simd::create();
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

  boidsTime.count++;
  if (frameData.dt != 0.f)
  bds_simd::update(world.boids_simd, frameData, &boidsTime);
    //bds::update(world.boids, frameData, &boidsTime);
  if (boidsTime.count > 1000) {
    boidsTime.count = 0;
    boidsTime.textPos = boidsTime.updatePositionTime / 1000.0f;
    boidsTime.textApply = boidsTime.applyBehaviourTime / 1000.0f;
    boidsTime.textMoveInside = boidsTime.moveInside / 1000.0f;

    boidsTime.updatePositionTime = 0;
    boidsTime.applyBehaviourTime = 0;
    boidsTime.moveInside = 0;
  }
}

void renderScene(const mg::FrameData &frameData) {
  mg::Texts texts = {};
  char fps[50];
  char pos[50];
  char beh[50];
  char mov[50];

  snprintf(fps, sizeof(fps), "Fps: %u", uint32_t(frameData.fps));
  snprintf(pos, sizeof(pos), "Update Position: %.3f us", boidsTime.textPos);
  snprintf(beh, sizeof(beh), "Apply Behaviour: %.3f us", boidsTime.textApply);
  snprintf(mov, sizeof(mov), "Move Inside: %.3f us", boidsTime.textMoveInside);

  mg::Text text1 = {fps};
  mg::Text text2 = {pos};
  mg::Text text3 = {beh};
  mg::Text text4 = {mov};

  mg::pushText(&texts, text1);
  mg::pushText(&texts, text2);
  mg::pushText(&texts, text3);
  mg::pushText(&texts, text4);

  mg::beginRendering();
  mg::setFullscreenViewport();

  mg::beginSingleRenderPass(world.singleRenderPass);
  {
    mg::RenderContext renderContext = {};
    renderContext.renderPass = world.singleRenderPass.vkRenderPass;
    renderContext.projection = glm::perspective(glm::radians(world.camera.fov),
                                                mg::vkContext.screen.width / float(mg::vkContext.screen.height), 0.1f, 1000.f);
    renderContext.view = glm::lookAt(world.camera.position, world.camera.aim, world.camera.up);
    renderContext.renderPass = world.singleRenderPass.vkRenderPass;

    //bds::render(world.boids, frameData, renderContext, world.meshId);
    bds_simd::render(world.boids_simd, frameData, renderContext, world.meshId);
    mg::validateTexts(texts);
    mg::renderText(renderContext, texts);
  }
  mg::endSingleRenderPass();
  mg::endRendering();
}