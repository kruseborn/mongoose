#include "volume_scene.h"
#include "mg/camera.h"
#include "mg/mgAssert.h"
#include "mg/mgSystem.h"
#include "mg/tools.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include "volume_rendering.h"
#include "volume_renderpass.h"
#include "volume_utils.h"
#include "vulkan/vkContext.h"
#include <glm/gtc/matrix_transform.hpp>
#include "mg/textureContainer.h"

static mg::Camera camera;
static VolumeRenderPass volumeRenderPass;
static VolumeInfo volumeInfo;

void initScene() {
  camera = mg::create3DCamera(glm::vec3{277 / 2.0f, 277 / 2.0f, -400.0f}, glm::vec3{277 / 2.0f, 277 / 2.0f, 0.0f},
                              glm::vec3{0, 1, 0});

  volumeInfo = parseDatFile();
  initVolumeRenderPass(&volumeRenderPass);
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void destroyScene() { 
  mg::waitForDeviceIdle();
  destroyVolumeRenderPass(&volumeRenderPass); 
}

void updateScene(const mg::FrameData &frameData) {
  if (frameData.resize) {
    resizeVolumeRenderPass(&volumeRenderPass);
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
  renderContext.renderPass = volumeRenderPass.vkRenderPass;
  renderContext.projection =
      glm::perspective(glm::radians(camera.fov), mg::vkContext.screen.width / float(mg::vkContext.screen.height), 0.1f, 10000.f);
  renderContext.view = glm::lookAt(camera.position, camera.aim, camera.up);

  beginVolumeRenderPass(volumeRenderPass);
  { 
    renderContext.subpass = 0;
    drawFrontAndBack(renderContext, volumeInfo); 
    
    vkCmdNextSubpass(mg::vkContext.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    renderContext.subpass = 1;
    drawVolume(renderContext, camera, volumeInfo, frameData, volumeRenderPass);

    vkCmdNextSubpass(mg::vkContext.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    renderContext.subpass = 2;
    drawDenoise(renderContext, volumeRenderPass);
  }
  endVolumeRenderPass();

  mg::endRendering();
}