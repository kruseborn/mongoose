#include "invaders_scene.h"
#include "hashmap.h"
#include "invaders_helper.h"
#include "mg/mgSystem.h"
#include "mg/window.h"
#include "player.h"
#include "rendering/rendering.h"
#include "transforms.h"
#include "vulkan/singleRenderpass.h"
#include <cstdio>

// global
Device device = {};
static const Settings settings = {};
static mg::SingleRenderPass singleRenderPass;

void invadersInit(Invaders *invaders) {
  mg::initSingleRenderPass(&singleRenderPass);

  assert(invaders);
  device.linearAllocator.init(1024 * 1024);
  invadersReset(invaders);
}

void invadersDestroy(Invaders *invaders) {
  assert(invaders);
  *invaders = {};
  device.linearAllocator.destroy();
  destroySingleRenderPass(&singleRenderPass);
}

void invadersEndFrame() { device.linearAllocator.clear(); }
void invadersReset(Invaders *invaders) { invadersReset(invaders, settings); }

void invadersSimulate(Invaders *invaders, const mg::FrameData &frameData, float dt) {
  assert(invaders);

  if (frameData.resize) {
    mg::resizeSingleRenderPass(&singleRenderPass);
  }
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
  transformPlayer(&player, mg::getScreenWidth(), settings.playerSize, frameData, dt);
  const auto minMaxAliens = transformAliens(&aliens, dt);
  transformPositions(playerBullets.x, playerBullets.y, playerBullets.nrBullets, playerBullets.direction, playerBullets.speed, dt);
  transformPositions(alienBullets.x, alienBullets.y, alienBullets.nrBullets, alienBullets.direction, alienBullets.speed, dt);

  // fire bullets
  if (shouldFirePlayerBullet(&player, frameData, dt)) {
    assert(playerBullets.nrBullets < MAX_BULLETS);
    playerBullets.x[playerBullets.nrBullets] = player.position.x;
    playerBullets.y[playerBullets.nrBullets++] = player.position.y;
  }
  fireAlienBullets(aliens, &alienBullets);

  // add entities to grid
  addTransformToGrid(aliens.x, aliens.y, aliens.nrAliens, &alienHashmap, settings.cellSize);
  addTransformToGrid(alienBullets.x, alienBullets.y, alienBullets.nrBullets, &alienBulletsHashmap, settings.cellSize);

  // handle collisions
  playerBulletsOnAlienCollision(alienHashmap, &aliens, &playerBullets, settings.cellSize);
  alienBulletsOnPlayerCollision(alienBulletsHashmap, &player, &alienBullets, settings.cellSize);
  alienOnPlayerCollision(alienHashmap, &aliens, &player, settings.cellSize);

  // handle borders
  removeBulletsOutsideBorders(&playerBullets);
  removeBulletsOutsideBorders(&alienBullets);

  const bool isAliensOutSideBorder = minMaxAliens.ymax > (mg::getScreenHeight() - settings.alienSize);
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

  drawSprites(renderContext, aliens.x, aliens.y, settings.alienSize, aliens.nrAliens);
  drawSprites(renderContext, &player.position.x, &player.position.y, settings.playerSize, 1);
  drawSprites(renderContext, playerBullets.x, playerBullets.y, settings.playerSize, playerBullets.nrBullets);
  drawSprites(renderContext, alienBullets.x, alienBullets.y, settings.playerSize, alienBullets.nrBullets);

  char textBuffer[40];
  constexpr auto pointsPerAlien = 10;
  const auto killScore = (MAX_ALIENS - aliens.nrAliens) * pointsPerAlien;
  snprintf(textBuffer, sizeof(textBuffer), "Health: %d score: %d\n", player.health, killScore);

  mg::endSingleRenderPass();
  mg::endRendering();
  // engine->drawText(textBuffer, 0, 0);
}