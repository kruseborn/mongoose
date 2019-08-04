#include "ray_scene.h"
#include "mg/camera.h"
#include "mg/mgAssert.h"
#include "mg/mgSystem.h"
#include "mg/textureContainer.h"
#include "mg/tools.h"
#include "mg/window.h"
#include "ray_rendering.h"
#include "ray_renderpass.h"
#include "ray_utils.h"
#include "rendering/rendering.h"
#include "vulkan/vkContext.h"
#include <glm/gtc/matrix_transform.hpp>

static mg::Camera camera;
static RayInfo rayinfo;
static mg::SingleRenderPass singleRenderPass;

void initScene() {
  camera = mg::create3DCamera(glm::vec3{0,0, -2.5f}, glm::vec3{0,0,0},
                              glm::vec3{0, 1, 0});
  createScene(&rayinfo);
  mg::initSingleRenderPass(&singleRenderPass);
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void destroyScene() {
  mg::waitForDeviceIdle();
  destroySingleRenderPass(&singleRenderPass);
}

void updateScene(const mg::FrameData &frameData) {
  if (frameData.resize) {
    //    resizerayRenderPass(&rayRenderPass);
  }
  if (frameData.keys.r) {
    mg::mgSystem.pipelineContainer.resetPipelineContainer();
  }
  if (frameData.mouse.left) {
    mg::handleTools(frameData, &camera);
  }
  mg::setCameraTransformation(&camera);
}

void renderScene(const mg::FrameData &frameData) {
  mg::beginRendering();
  mg::RenderContext renderContext = {};
  renderContext.projection = glm::perspective(
      glm::radians(camera.fov), mg::vkContext.screen.width / float(mg::vkContext.screen.height), 0.1f, 1000.f);
  renderContext.view = glm::lookAt(camera.position, camera.aim, camera.up);

  traceTriangle(renderContext, rayinfo);

  mg::beginSingleRenderPass(singleRenderPass);
  renderContext.renderPass = singleRenderPass.vkRenderPass;

  drawImageStorage(renderContext, rayinfo);

  mg::endSingleRenderPass();


  mg::endRendering();
}