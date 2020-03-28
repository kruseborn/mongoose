#include "fluid_scene.h"

#include "mg/camera.h"
#include "mg/meshUtils.h"
#include "mg/mgSystem.h"
#include "mg/texts.h"
#include "mg/window.h"
#include "navier_stoke.h"
#include "rendering/rendering.h"
#include "vulkan/singleRenderpass.h"
#include <glm/gtc/matrix_transform.hpp>

static mg::Camera camera;
static mg::SingleRenderPass singleRenderPass;
static mg::MeshId meshId;
static mg::Storages storages;
static uint32_t N = 1024;

static void resizeCallback() {
  mg::resizeSingleRenderPass(&singleRenderPass);
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void initScene() {
  mg::initSingleRenderPass(&singleRenderPass);

  camera = mg::create3DCamera(glm::vec3{0.0f, 0.0f, -5.0f}, glm::vec3{0.0f, 0.0f, 0.0f},
                              glm::vec3{0.0f, 1.0f, 0.0f});

  storages = mg::createStorages(N);
  mg::mgSystem.textureContainer.setupDescriptorSets();
  mg::vkContext.swapChain->resizeCallack = resizeCallback;
}

void destroyScene() {
  mg::waitForDeviceIdle();
  mg::destroyStorages(&storages);
  destroySingleRenderPass(&singleRenderPass);
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

  mg::Text text1 = {fps};
  mg::pushText(&texts, text1);


  mg::beginRendering();
  mg::setFullscreenViewport();

  mg::simulateNavierStoke(storages, frameData, N);

  mg::beginSingleRenderPass(singleRenderPass);
  {
    mg::RenderContext renderContext = {};
    renderContext.renderPass = singleRenderPass.vkRenderPass;

    mg::renderNavierStoke(renderContext, storages);
    mg::validateTexts(texts);
    mg::renderText(renderContext, texts);
  }
  mg::endSingleRenderPass();

  mg::endRendering();
}