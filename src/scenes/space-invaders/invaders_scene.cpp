#include "invaders_scene.h"
#include "hashmap.h"
#include "invaders_helper.h"
#include "player.h"
#include "transforms.h"
#include <cstdio>

// global
Device device = {};
static const Settings settings = {};

void invadersInit(Invaders *invaders) {
  assert(invaders);
  device.linearAllocator.init(1024 * 1024);
  invadersReset(invaders);
}

void invadersDestroy(Invaders *invaders) {
  assert(invaders);
  *invaders = {};
  device.linearAllocator.destroy();
}

void invadersEndFrame() { device.linearAllocator.clear(); }
void invadersReset(Invaders *invaders) { invadersReset(invaders, settings); }

void invadersSimulate(Invaders *invaders, const mg::FrameData &frameData, float dt) {
  assert(invaders);
  Hashmap alienHashmap = {};
  Hashmap alienBulletsHashmap = {};
  alienHashmap.init(&device.linearAllocator, 512);
  alienBulletsHashmap.init(&device.linearAllocator, 512);

  auto &player = invaders->player;
  auto &aliens = invaders->aliens;
  auto &playerBullets = invaders->playerBullets;
  auto &alienBullets = invaders->alienBullets;

  // transform
  //transformPlayer(&player, Engine::CanvasWidth, Engine::SpriteSize, playerInput, dt);
  const auto minMaxAliens = transformAliens(&aliens, dt);
  transformPositions(playerBullets.x, playerBullets.y, playerBullets.nrBullets, playerBullets.direction, playerBullets.speed, dt);
  transformPositions(alienBullets.x, alienBullets.y, alienBullets.nrBullets, alienBullets.direction, alienBullets.speed, dt);

  // fire bullets
  // if (shouldFirePlayerBullet(&player, playerInput, dt)) {
  //   assert(playerBullets.nrBullets < MAX_BULLETS);
  //   playerBullets.x[playerBullets.nrBullets] = player.position.x;
  //   playerBullets.y[playerBullets.nrBullets++] = player.position.y;
  // }
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

  // const bool isAliensOutSideBorder = minMaxAliens.ymax > (Engine::CanvasHeight - Engine::SpriteSize);
  // player.health *= !isAliensOutSideBorder;
}

void invadersRender(const Invaders &invaders) {
  //assert(engine);
  const auto &player = invaders.player;
  const auto &aliens = invaders.aliens;
  const auto &playerBullets = invaders.playerBullets;
  const auto &alienBullets = invaders.alienBullets;

  //drawSprites(aliens.x, aliens.y, aliens.sprites, aliens.nrAliens);
  //drawSprites(&player.position.x, &player.position.y, 1, engine, Engine::Sprite::Player);
  //drawSprites(playerBullets.x, playerBullets.y, playerBullets.nrBullets, engine, Engine::Sprite::Rocket);
  //drawSprites(alienBullets.x, alienBullets.y, alienBullets.nrBullets, engine, Engine::Sprite::Bomb);

  char textBuffer[40];
  constexpr auto pointsPerAlien = 10;
  const auto killScore = (MAX_ALIENS - aliens.nrAliens) * pointsPerAlien;
  snprintf(textBuffer, sizeof(textBuffer), "Health: %d score: %d\n", player.health, killScore);
  //engine->drawText(textBuffer, 0, 0);
}