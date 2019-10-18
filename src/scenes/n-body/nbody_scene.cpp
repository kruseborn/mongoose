#include "nbody_scene.h"
#include "mg/camera.h"
#include "mg/mgAssert.h"
#include "mg/mgSystem.h"
#include "mg/texts.h"
#include "mg/tools.h"
#include "mg/window.h"
#include "nbody_rendering.h"
#include "nbody_renderpass.h"
#include "nbody_utils.h"
#include "rendering/rendering.h"
#include "vulkan/vkContext.h"
#include "vulkan/vkUtils.h"
#include <lodepng.h>

static mg::Camera camera;
static NBodyRenderPass nbodyRenderPass = {};

static ComputeData computeData = {};

void initScene() {
  initNBodyRenderPass(&nbodyRenderPass);

  computeData.particleId = mg::uploadPngImage("particle2.png");
  camera = mg::create3DCamera(glm::vec3{0.0f, 0.0f, -5.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});

  initParticles(&computeData);
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void destroyScene() {
  mg::mgSystem.storageContainer.removeStorage(computeData.storageId);
  mg::waitForDeviceIdle();
  destroyNBodyRenderPass(&nbodyRenderPass);
}

void updateScene(const mg::FrameData &frameData) {
  if (frameData.resize) {
    resizeNBodyRenderPass(&nbodyRenderPass);
  }
  if (frameData.keys.r) {
    mg::mgSystem.pipelineContainer.resetPipelineContainer();
  }
  if (frameData.mouse.left)
    mg::handleTools(frameData, &camera);
  mg::setCameraTransformation(&camera);
}

bool renderScene(const mg::FrameData &frameData) {
  mg::Texts texts = {};
  char fps[50];
  snprintf(fps, sizeof(fps), "Fps: %u", uint32_t(frameData.fps));
  mg::Text text = {fps};

  mg::pushText(&texts, text);
  mg::beginRendering();
  mg::setFullscreenViewport();

  simulatePartices(computeData, frameData);

  beginNBodyRenderPass(nbodyRenderPass);
  mg::RenderContext renderContext = {};
  renderContext.renderPass = nbodyRenderPass.vkRenderPass;
  renderContext.subpass = 0;
  renderParticels(renderContext, computeData, camera);

  vkCmdNextSubpass(mg::vkContext.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
  renderContext.subpass = 1;

  renderToneMapping(renderContext, nbodyRenderPass);

  mg::validateTexts(texts);
  mg::renderText(renderContext, texts);

  endNBodyRenderPass();

  return mg::endRendering();
}