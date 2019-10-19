#include "invaders_scene.h"
#include "hashmap.h"
#include "invaders_helper.h"
#include "mg/mgSystem.h"
#include "mg/window.h"
#include "player.h"
#include "rendering/rendering.h"
#include "transforms.h"
#include "vulkan/singleRenderpass.h"
#include "mg/texts.h"
#include <cstdio>

// global
Device device = {};
static const Settings settings = {};
static mg::SingleRenderPass singleRenderPass;

static void resizeCallback() {
  mg::resizeSingleRenderPass(&singleRenderPass);
  mg::mgSystem.textureContainer.setupDescriptorSets();
}

void invadersInit(Invaders *invaders) {
  mg::initSingleRenderPass(&singleRenderPass);

  assert(invaders);
  device.linearAllocator.init(1024 * 1024);
  invadersReset(invaders);

  mg::mgSystem.textureContainer.setupDescriptorSets();
  mg::vkContext.swapChain->resizeCallack = resizeCallback;
}

void invadersDestroy(Invaders *invaders) {
  assert(invaders);
  *invaders = {};
  device.linearAllocator.destroy();
  mg::waitForDeviceIdle();
  destroySingleRenderPass(&singleRenderPass);
}

void invadersEndFrame() { device.linearAllocator.clear(); }
void invadersReset(Invaders *invaders) { invadersReset(invaders, settings); }

void invadersSimulate(Invaders *invaders, const mg::FrameData &frameData, float dt) {
  assert(invaders);

  if (frameData.keys.r) {
    mg::mgSystem.pipelineContainer.resetPipelineContainer();
  }

  Hashmap alienHashmap = {};
  Hashmap alienBulletsHashmap = {};
  alienHashmap.init(&device.linearAllocator, 512);
  alienBulletsHashmap.init(&device.linearAllocator, 512);

  auto &player = invaders->player;
  auto &aliens = invaders->aliens;
  auto &playerBullets = invaders->playerBullets;
  auto &alienBullets = invaders->alienBullets;

  // transform
  transformPlayer(&player, mg::vkContext.screen.width, settings.playerSize, frameData, dt);
  const auto minMaxAliens = transformAliens(settings, &aliens, dt);
  transformPositions(playerBullets.x, playerBullets.y, playerBullets.nrBullets, playerBullets.direction, playerBullets.speed, dt);
  transformPositions(alienBullets.x, alienBullets.y, alienBullets.nrBullets, alienBullets.direction, alienBullets.speed, dt);

  // fire bullets
  if (shouldFirePlayerBullet(&player, frameData, dt)) {
    assert(playerBullets.nrBullets < MAX_BULLETS);
    playerBullets.x[playerBullets.nrBullets] = player.position.x;
    playerBullets.y[playerBullets.nrBullets++] = player.position.y;
  }
  fireAlienBullets(settings, aliens, &alienBullets);

  // add entities to grid
  addTransformToGrid(aliens.x, aliens.y, aliens.nrAliens, &alienHashmap, settings.cellSize);
  addTransformToGrid(alienBullets.x, alienBullets.y, alienBullets.nrBullets, &alienBulletsHashmap, settings.cellSize);

  // handle collisions
  playerBulletsOnAlienCollision(alienHashmap, settings, &aliens, &playerBullets, settings.cellSize);
  alienBulletsOnPlayerCollision(alienBulletsHashmap, settings, &player, &alienBullets, settings.cellSize);
  alienOnPlayerCollision(alienHashmap, settings, &aliens, &player, settings.cellSize);

  // handle borders
  removeBulletsOutsideBorders(&playerBullets);
  removeBulletsOutsideBorders(&alienBullets);

  const bool isAliensOutSideBorder = minMaxAliens.ymax > (mg::vkContext.screen.height - settings.alienSize);
  player.health *= !isAliensOutSideBorder;
}

void invadersRender(const Invaders &invaders, const mg::FrameData &frameData) {
  mg::beginRendering();
  mg::setFullscreenViewport();
  mg::beginSingleRenderPass(singleRenderPass);

  mg::RenderContext renderContext = {};
  renderContext.renderPass = singleRenderPass.vkRenderPass;

  const auto &player = invaders.player;
  const auto &aliens = invaders.aliens;
  const auto &playerBullets = invaders.playerBullets;
  const auto &alienBullets = invaders.alienBullets;

  drawSprites(renderContext, aliens.x, aliens.y, aliens.colors, settings.alienSize, aliens.nrAliens);
  drawSprites(renderContext, &player.position.x, &player.position.y, {45 / 255.0f, 0 / 255.0f, 38 / 255.0f, 1}, settings.playerSize, 1);
  drawSprites(renderContext, playerBullets.x, playerBullets.y, {45 / 255.0f, 41 / 255.0f, 0 / 255.0f, 1}, settings.playerBulletSize, playerBullets.nrBullets);
  drawSprites(renderContext, alienBullets.x, alienBullets.y, {233 / 255.0f, 75 / 255.0f, 60 / 255.0f, 1}, settings.alienBulletSize, alienBullets.nrBullets);

  char textBuffer[40];
  constexpr auto pointsPerAlien = 10;
  const auto killScore = (MAX_ALIENS - aliens.nrAliens) * pointsPerAlien;
  snprintf(textBuffer, sizeof(textBuffer), "Health: %d score: %d", player.health, killScore);

  mg::Texts texts = {};
  mg::Text text = {textBuffer};
  mg::pushText(&texts, text);

  mg::validateTexts(texts);
  mg::renderText(renderContext, texts);

  mg::endSingleRenderPass();
  mg::endRendering();
}