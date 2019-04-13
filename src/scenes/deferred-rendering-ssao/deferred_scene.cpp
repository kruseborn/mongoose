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

static mg::Camera camera;
static DeferredRenderPass deferredRenderPass;
static mg::ObjMeshes objMeshes;
#include <iostream>
using namespace std;
void initScene() {

  camera = mg::create3DCamera(glm::vec3(0.5, 200, 470), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

  objMeshes = mg::loadMeshesFromFile(mg::getDataPath() + "rungholt_obj/", "rungholt.obj", true);
  //objMeshes = mg::loadMeshesFromBinary("", "rungholt.obj");
  initDeferredRenderPass(&deferredRenderPass);
}

void destroyScene() {
  mg::waitForDeviceIdle();
  mg::removeTexture("noise");
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

void renderScene(const mg::FrameData &frameData) {
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
    renderSSAO(renderContext);

    vkCmdNextSubpass(mg::vkContext.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    renderContext.subpass = 2;
    renderBlurSSAO(renderContext);

    vkCmdNextSubpass(mg::vkContext.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    renderContext.subpass = 3;
    renderFinalDeferred(renderContext);

    mg::renderBoxWithTexture(renderContext, {-0.98f + 0.32f, -0.9f, 0.3f, 0.3f}, "albedo");
    mg::renderBoxWithTexture(renderContext, {-0.98f + 0.32f * 2.0f, -0.9f, 0.3f, 0.3f}, "ssaoblur");
    mg::renderBoxWithTexture(renderContext, {-0.98f + 0.32f * 3.0f, -0.9f, 0.3f, 0.3f}, "normal");
    mg::renderBoxWithDepthTexture(renderContext, {-0.98f + 0.32f * 4.0f, -0.9f, 0.3f, 0.3f}, "depth");

    mg::validateTexts(texts);
    mg::renderText(renderContext, texts);

    mg::mgSystem.imguiOverlay.draw(renderContext, frameData);
  }
  endDeferredRenderPass();
  mg::endRendering();
}
