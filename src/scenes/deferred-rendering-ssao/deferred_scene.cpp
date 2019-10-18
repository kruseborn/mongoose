#include "deferred_scene.h"

#include "deferred_rendering.h"
#include "deferred_renderpass.h"
#include "mg/camera.h"
#include "mg/meshLoader.h"
#include "mg/mgAssert.h"
#include "mg/mgSystem.h"
#include "mg/texts.h"
#include "mg/tools.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include <glm/gtc/matrix_transform.hpp>
#include <lodepng.h>
#include "deferred_utils.h"

static mg::Camera camera;
static DeferredRenderPass deferredRenderPass;
static Noise noise;
static mg::ObjMeshes objMeshes;

using namespace std;
void initScene() {

  camera = mg::create3DCamera(glm::vec3(0.5, 200, 470), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
  //camera = mg::create3DCamera(glm::vec3(0.5, 1.0, 4), glm::vec3(0, 1.0, 0), glm::vec3(0, 1, 0));
  objMeshes = mg::loadObjFromFile(mg::getDataPath() + "rungholt_obj/rungholt.obj");
  //objMeshes = mg::loadObjFromFile(mg::getDataPath() + "CornellBox_obj/CornellBox-Original.obj");
  initDeferredRenderPass(&deferredRenderPass);
  noise = createNoise();

  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void destroyScene() {
  mg::waitForDeviceIdle();
  mg::removeTexture(noise.noiseTexture);
  destroyDeferredRenderPass(&deferredRenderPass);
}

void updateScene(const mg::FrameData &frameData) {
  if (frameData.resize) {
    resizeDeferredRenderPass(&deferredRenderPass);
  }
  if (frameData.keys.r) {
    mg::mgSystem.pipelineContainer.resetPipelineContainer();
  }
  if (frameData.mouse.xy.x >= 0 && frameData.mouse.xy.x < 1.0f && frameData.mouse.xy.y >= 0 && frameData.mouse.xy.y < 1.0f) {
    if (frameData.mouse.left) {
      mg::handleTools(frameData, &camera);
    }
    mg::setCameraTransformation(&camera);
  }
}

bool renderScene(const mg::FrameData &frameData) {
  mg::Texts texts = {};
  mg::Text text = {"Rungholt"};

  mg::pushText(&texts, text);

  mg::beginRendering();

  mg::RenderContext renderContext = {};
  renderContext.renderPass = deferredRenderPass.vkRenderPass;

  renderContext.projection =
      glm::perspective(glm::radians(camera.fov), mg::vkContext.screen.width / float(mg::vkContext.screen.height), 0.1f, 1000.f);
  renderContext.view = glm::lookAt(camera.position, camera.aim, camera.up);

  beginDeferredRenderPass(deferredRenderPass);
  {
    renderContext.subpass = 0;
    renderMRT(renderContext, objMeshes);

    vkCmdNextSubpass(mg::vkContext.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    renderContext.subpass = 1;
    renderSSAO(renderContext, deferredRenderPass, noise);

    vkCmdNextSubpass(mg::vkContext.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    renderContext.subpass = 2;
    renderBlurSSAO(renderContext, deferredRenderPass);

    vkCmdNextSubpass(mg::vkContext.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    renderContext.subpass = 3;
    renderFinalDeferred(renderContext, deferredRenderPass);

    mg::renderBoxWithTexture(renderContext, {-0.98f + 0.32f, -0.9f, 0.3f, 0.3f}, deferredRenderPass.albedo);
    mg::renderBoxWithTexture(renderContext, {-0.98f + 0.32f * 2.0f, -0.9f, 0.3f, 0.3f}, deferredRenderPass.ssaoBlur);
    mg::renderBoxWithTexture(renderContext, {-0.98f + 0.32f * 3.0f, -0.9f, 0.3f, 0.3f}, deferredRenderPass.normal);
    mg::renderBoxWithDepthTexture(renderContext, {-0.98f + 0.32f * 4.0f, -0.9f, 0.3f, 0.3f}, deferredRenderPass.depth);

    mg::validateTexts(texts);
    mg::renderText(renderContext, texts);

    mg::mgSystem.imguiOverlay.draw(renderContext, frameData);
  }
  endDeferredRenderPass();
  return mg::endRendering();
}
