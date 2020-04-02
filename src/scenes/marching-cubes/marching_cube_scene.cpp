#include "marching_cube_scene.h"

#include "march_cubes.h"
#include "mg/camera.h"
#include "mg/meshUtils.h"
#include "mg/mgSystem.h"
#include "mg/texts.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include "vulkan/singleRenderpass.h"
#include <glm/gtc/matrix_transform.hpp>

static mg::Camera camera;
static mg::SingleRenderPass singleRenderPass;
static mg::MeshId meshId;

static void resizeCallback() {
  mg::resizeSingleRenderPass(&singleRenderPass);
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void initScene() {
  mg::initSingleRenderPass(&singleRenderPass);

  camera = mg::create3DCamera(glm::vec3{0.0f, 15, -40.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});

  mg::init();

  mg::mgSystem.textureContainer.setupDescriptorSets();
  mg::vkContext.swapChain->resizeCallack = resizeCallback;
}

void destroyScene() {
  mg::waitForDeviceIdle();
  mg::destroy();
  destroySingleRenderPass(&singleRenderPass);
}

void updateScene(const mg::FrameData &frameData) {
  if (frameData.keys.r) {
    mg::mgSystem.pipelineContainer.resetPipelineContainer();
  }
  if (frameData.mouse.right)
    mg::handleTools(frameData, &camera);
  mg::setCameraTransformation(&camera);
}

void renderScene(const mg::FrameData &frameData) {
  mg::Texts texts = {};
  char fps[50];
  snprintf(fps, sizeof(fps), "Fps: %u", uint32_t(frameData.fps));

  mg::Text text1 = {fps};
  mg::pushText(&texts, text1);
  mg::waitForDeviceIdle();


  mg::beginRendering();
  mg::setFullscreenViewport();

  auto sb = mg::simulate();

  mg::beginSingleRenderPass(singleRenderPass);
  {
    mg::RenderContext renderContext = {};
    renderContext.renderPass = singleRenderPass.vkRenderPass;

    renderContext.projection = glm::perspective(
        glm::radians(camera.fov), mg::vkContext.screen.width / float(mg::vkContext.screen.height), 0.1f, 1000.f);
    renderContext.view = glm::lookAt(camera.position, camera.aim, camera.up);

    renderContext.renderPass = singleRenderPass.vkRenderPass;

    static float rotAngle = 0;
    rotAngle += frameData.dt * 20.0f;
    glm::mat4 rotation = glm::rotate(glm::mat4(1), rotAngle, {1, 1, 0});
    mg::renderMC(renderContext, sb);
    //mg::render2(renderContext);
    mg::renderText(renderContext, texts);
    mg::mgSystem.imguiOverlay.draw(renderContext, frameData, mg::renderGUI);

  }
  mg::endSingleRenderPass();

  mg::endRendering();
  mg::waitForDeviceIdle();
  
}