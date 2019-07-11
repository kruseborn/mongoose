#include "nbody_scene.h"
#include "mg/camera.h"
#include "mg/mgAssert.h"
#include "mg/mgSystem.h"
#include "mg/tools.h"
#include "mg/window.h"
#include "nbody_rendering.h"
#include "nbody_utils.h"
#include "rendering/rendering.h"
#include "vulkan/singleRenderpass.h"
#include "vulkan/vkContext.h"
#include "vulkan/vkUtils.h"
#include <lodepng.h>

static mg::Camera camera;
static mg::SingleRenderPass singleRenderPass;

static ComputeData computeData = {};

void initScene() {
  mg::initSingleRenderPass(&singleRenderPass);
  camera = mg::create3DCamera(glm::vec3{0.0f, 0.0f, 0.5f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});

  const int WIDTH = 3200;  // Size of rendered mandelbrot set.
  const int HEIGHT = 2400; // Size of renderered mandelbrot set.
  computeData.storageId = mg::mgSystem.storageContainer.createStorage(sizeof(glm::vec4) * computeData.width * computeData.height);
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



void renderScene(const mg::FrameData &frameData) {
  mg::beginRendering();
  mg::setFullscreenViewport();

  computeMandelbrot(computeData);

  mg::beginSingleRenderPass(singleRenderPass);
  mg::RenderContext renderContext = {};
  renderContext.renderPass = singleRenderPass.vkRenderPass;

  mg::endSingleRenderPass();

  mg::endRendering();
}