#include "marching_cube_scene.h"

#include "march_cubes.h"
#include "mg/camera.h"
#include "mg/meshUtils.h"
#include "mg/mgSystem.h"
#include "mg/texts.h"
#include "mg/window.h"
#include "quadtree.h"
#include "rendering/rendering.h"
#include "vulkan/singleRenderpass.h"
#include <glm/gtc/matrix_transform.hpp>

static mg::Camera camera;
static mg::SingleRenderPass singleRenderPass;

static mg::MeshId id;

static const uint32_t N = 3;
static mg::MarchingCubesStorages marchingCubesStorages[N][N];
// clang-format off
static mg::Grid grids[N][N];
// clang-format on

static void resizeCallback() {
  mg::resizeSingleRenderPass(&singleRenderPass);
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void initScene() {
  mg::initSingleRenderPass(&singleRenderPass);

  float startx = -250.0f;
  float startz = -50.0f;
  float length = 100;
  for (uint32_t z = 0; z < N; z++) {
    for (uint32_t x = 0; x < N; x++) {
      float xx = startx + x * length;
      float zz = startz + z * length;
      grids[z][x] = {.N = 100, .cellSize = 1.0f, .corner = {xx, -5, zz}};
    }
  }
  for (uint32_t z = 0; z < N; z++) {
    for (uint32_t x = 0; x < N; x++) {
      marchingCubesStorages[z][x] = mg::createMarchingCubesStorages(grids[z][x].N);
    }
  }

   camera = mg::create3DCamera(glm::vec3{0.0f, 50, -60.0f}, glm::vec3{0.0f, 10.0f, 0.0f}, glm::vec3{0.0f, 1.0f,
   0.0f});
  //camera = mg::create3DCamera(glm::vec3{0.0f, 0.0f, -15.0f}, glm::vec3{0.0f, 0, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});

  id = mg::dualContouring();
  //mg::simulateQuadtree();


  mg::mgSystem.textureContainer.setupDescriptorSets();
  mg::vkContext.swapChain->resizeCallack = resizeCallback;
}

void destroyScene() {
  mg::waitForDeviceIdle();

  mg::mgSystem.meshContainer.removeMesh(id);
  for (uint32_t z = 0; z < N; z++) {
    for (uint32_t x = 0; x < N; x++) {
      mg::destroyCreateMarchingCubesStorages(marchingCubesStorages[z][x]);
    }
  }
  destroySingleRenderPass(&singleRenderPass);
}

void updateScene(const mg::FrameData &frameData) {
  if (frameData.keys.r) {
    mg::mgSystem.pipelineContainer.resetPipelineContainer();
  }
  if (frameData.mouse.right)
    mg::handleTools(frameData, &camera);
  if (frameData.keys.w) {
    camera.startPosition.z += 5.0f;
    camera.startAim.z += 5.0f;
  }
  if (frameData.keys.s) {
    camera.startPosition.z -= 5.0f;
    camera.startAim.z -= 5.0f;
  }
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

   mg::Sb sbs[N][N];

   for (uint32_t z = 0; z < N; z++) {
    for (uint32_t x = 0; x < N; x++) {
      sbs[z][x] = mg::simulate(marchingCubesStorages[z][x], grids[z][x]);
    }
  }


  mg::beginSingleRenderPass(singleRenderPass);
  {
    mg::RenderContext renderContext = {};
    renderContext.renderPass = singleRenderPass.vkRenderPass;

    renderContext.projection = glm::perspective(
        glm::radians(camera.fov), mg::vkContext.screen.width / float(mg::vkContext.screen.height), 0.1f, 1000.f);
    renderContext.view = glm::lookAt(camera.position, camera.aim, camera.up);

    renderContext.renderPass = singleRenderPass.vkRenderPass;

    // for (uint32_t z = 0; z < N; z++) {
    //  for (uint32_t x = 0; x < N; x++) {
    //    mg::renderMC(renderContext, sbs[z][x], marchingCubesStorages[z][x], grids[z][x]);
    //  }
    //}

    mg::renderMeshWithNormals(renderContext, id, glm::identity<glm::mat4>(), glm::vec4{1, 0, 0, 1});
   // mg::renderQuadTree(renderContext);

    // mg::renderText(renderContext, texts);

     mg::mgSystem.imguiOverlay.draw(renderContext, frameData, mg::renderGUI, nullptr);
  }
  mg::endSingleRenderPass();

  mg::endRendering();
  mg::waitForDeviceIdle();
}
