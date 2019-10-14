#include "ray_scene.h"
#include "mg/camera.h"
#include "mg/mgAssert.h"
#include "mg/mgSystem.h"
#include "mg/texts.h"
#include "mg/textureContainer.h"
#include "mg/tools.h"
#include "mg/window.h"
#include "ray_rendering.h"
#include "ray_utils.h"
#include "rendering/rendering.h"
#include "sobol.h"
#include "vulkan/vkContext.h"
#include <glm/gtc/matrix_transform.hpp>
#include <random>

static mg::Camera camera = {};
static RayInfo rayinfo = {};
static mg::SingleRenderPass singleRenderPass = {};
static World world = {};

static std::default_random_engine generator;

float rng() { return std::generate_canonical<float, std::numeric_limits<double>::digits>(generator); }

glm::vec4 toVec4(const glm::vec3 &v, float s) { return glm::vec4{v.x, v.y, v.z, s}; }

// http://gruenschloss.org/
static void generateSobol() {
  constexpr uint32_t size = 2048 * 4;
  float sequence[size];
  for (uint32_t i = 0; i < size; i += 4) {
    int index = i + 10;
    sequence[i] = sobol::sample(index, 2);
    sequence[i + 1] = sobol::sample(index, 3);
    sequence[i + 2] = sobol::sample(index, 5);
    sequence[i + 3] = sobol::sample(index, 7);
  }
  mg::CreateTextureInfo texturInfo = {.id = "sobol",
                                      .type = mg::TEXTURE_TYPE::TEXTURE_2D,
                                      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                      .size = {2048, 1, 1},
                                      .sizeInBytes = sizeof(sequence),
                                      .data = sequence};

  rayinfo.sobolId = mg::mgSystem.textureContainer.createTexture(texturInfo);
}

void initScene() {
  generateSobol();
  world.blueNoise = mg::uploadPngImage("HDR_RGBA_0.png");

  generator.seed(2);

  addSphere({.world = &world,
             .position = {0, -1000.0f, 0, 1000.0f},
             .albedo = {0.5f, 0.5f, 0.5f, 1},
             .material = World::LAMBERTH});

  for (int32_t a = -11; a < 11; a++) {
    for (int32_t b = -11; b < 11; b++) {
      float chooseMat = rng();
      glm::vec3 center = {a + 0.9 * rng(), 0.2, (b + 0.9 * rng()) * -1.0f};

      if (glm::distance(center, {4, 0.2, 0}) > 0.9f) {
        if (chooseMat < 0.8) { // diffuse
          addSphere({.world = &world,
                     .position = toVec4(center, 0.2f),
                     .albedo = {rng() * rng(), rng() * rng(), rng() * rng(), 1},
                     .material = World::LAMBERTH});
        } else if (chooseMat < 0.95f) { // metal
          addSphere({.world = &world,
                     .position = toVec4(center, 0.2f),
                     .albedo = {0.5f * (1 + rng()), 0.5f * (1 + rng()), 0.5f * (1 + rng()), 0.5f * rng()},
                     .material = World::METAL});
        } else { // glass
          addSphere({.world = &world,
                     .position = toVec4(center, 0.2f),
                     .albedo = {1, 1, 1, 1.5f},
                     .material = World::DIELECTRIC});
        }
      }
    }
  }
  addSphere({.world = &world, .position = {0, 1, 0, 1}, .albedo = {0.8, 0.8, 0.8, 1.5}, .material = World::DIELECTRIC});
  addSphere({.world = &world, .position = {-4, 1, 0, 1}, .albedo = {0.4, 0.2, 0.1, 1}, .material = World::LAMBERTH});
  addSphere({.world = &world, .position = {4, 1, 0, 1}, .albedo = {0.7, 0.6, 0.5, 0.0}, .material = World::METAL});

  camera = mg::create3DCamera(glm::vec3{12, 4, -4}, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});
  createRayInfo(world, &rayinfo);
  mg::initSingleRenderPass(&singleRenderPass);
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void destroyScene() {
  destroyRayInfo(&rayinfo);
  mg::waitForDeviceIdle();
  destroySingleRenderPass(&singleRenderPass);
}

void updateScene(const mg::FrameData &frameData) {
  if (frameData.resize) {
    rayinfo.resetAccumulationImage = true;
    mg::resizeSingleRenderPass(&singleRenderPass);
    resetSizeStorageImages(&rayinfo);
  }
  if (frameData.keys.r) {
    rayinfo.resetAccumulationImage = true;
    mg::mgSystem.pipelineContainer.resetPipelineContainer();
  }
  if (frameData.mouse.left) {
    rayinfo.resetAccumulationImage = true;
    mg::handleTools(frameData, &camera);
  }
  mg::setCameraTransformation(&camera);
}

void renderScene(const mg::FrameData &frameData) {
  mg::Texts texts = {};
  char fps[50];
  snprintf(fps, sizeof(fps), "Fps: %u", uint32_t(frameData.fps));
  mg::Text text = {fps};

  mg::pushText(&texts, text);

  mg::beginRendering();
  mg::RenderContext renderContext = {};
  renderContext.projection = glm::perspective(
      glm::radians(camera.fov), mg::vkContext.screen.width / float(mg::vkContext.screen.height), 0.1f, 1000.f);
  renderContext.view = glm::lookAt(camera.position, camera.aim, camera.up);

  traceTriangle(world, camera, renderContext, rayinfo);

  mg::beginSingleRenderPass(singleRenderPass);
  renderContext.renderPass = singleRenderPass.vkRenderPass;

  drawImageStorage(renderContext, rayinfo);

  mg::validateTexts(texts);
  mg::renderText(renderContext, texts);

  mg::endSingleRenderPass();

  mg::endRendering();
  rayinfo.resetAccumulationImage = false;
}