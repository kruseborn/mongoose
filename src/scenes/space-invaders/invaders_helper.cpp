#include "invaders_helper.h"
#include "collisions.h"
#include "hashmap.h"
#include "invaders_scene.h"
#include "invaders_utils.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include "transforms.h"
#include "types.h"
#include <cstdlib>

void drawSprites(const mg::RenderContext &renderContext, const float *xPositions, const float *yPositions,
                 const glm::vec4 *colors, float size, uint32_t count) {
  assert(xPositions);
  assert(yPositions);
  mg::renderSolidBoxes(renderContext, xPositions, yPositions, colors, {size, size}, count, false);
}

void drawSprites(const mg::RenderContext &renderContext, const float *xPositions, const float *yPositions,
                 const glm::vec4 &color, float size, uint32_t count) {
  assert(xPositions);
  assert(yPositions);
  mg::renderSolidBoxes(renderContext, xPositions, yPositions, &color, {size, size}, count, true);
}

void invadersReset(Invaders *invaders, const Settings &settings) {
  assert(invaders);
  assert(settings.aliensRows * settings.aliensCols == MAX_ALIENS);

  *invaders = {};
  auto &player = invaders->player;
  auto &aliens = invaders->aliens;
  auto &playerBullets = invaders->playerBullets;
  auto &alienBullets = invaders->alienBullets;

  // aliens
  aliens.speed = settings.alienSpeed;
  alienBullets.speed = settings.alienBulletSpeed;
  alienBullets.direction = {0, -1};

  glm::vec4 colors[] = {glm::vec4{45 / 255.0f, 41 / 255.0f, 38 / 255.0f, 1}, glm::vec4{233 / 255.0f, 75 / 255.0f, 60 / 255.0f, 1}};

  for (uint32_t y = 0; y < settings.aliensRows; y++) {
    for (uint32_t x = 0; x < settings.aliensCols; x++) {
      aliens.colors[aliens.nrAliens] = colors[x & 1 ^ y & 1];
      aliens.x[aliens.nrAliens] = float(settings.alienSize * x);
      aliens.y[aliens.nrAliens++] = mg::getScreenHeight() - float((y + 1) * settings.alienSize);
    }
  }
  // player
  player.health = settings.startHealth;
  player.weaponColdDown = settings.playerFireColdDown;
  player.speed = settings.playerSpeed;
  player.position.x = float(mg::getScreenWidth() / 2);
  player.position.y = 0;
  playerBullets.speed = settings.playerBulletSpeed;
  playerBullets.direction = {0, 1};
}

MinMax transformAliens(const Settings &settings, Aliens *aliens, float dt) {
  assert(aliens);
  const auto dir = vec2{float(AlienXDirs[aliens->dirIndex]), float(AlienYDirs[aliens->dirIndex])};
  const auto minMax = transformPositionsCalculateLimits(aliens->x, aliens->y, aliens->nrAliens, dir, aliens->speed, dt);
  Limits limits = {};
  limits.left = settings.alienSize * 0.1f;
  limits.right = mg::getScreenWidth() - settings.alienSize;
  limits.down = settings.alienSize * 0.8f;
  changeAlienDirectionIfNeeded(aliens, minMax, limits, dt);
  return minMax;
}

void fireAlienBullets(const Settings &settings, const Aliens &aliens, Bullets *bullets) {
  assert(bullets);
  const auto alienIndex = rand() % (aliens.nrAliens * 50000);
  if (alienIndex >= aliens.nrAliens)
    return;
  assert(bullets->nrBullets < MAX_BULLETS);
  bullets->x[bullets->nrBullets] = aliens.x[alienIndex];
  bullets->y[bullets->nrBullets++] = aliens.y[alienIndex] + settings.alienSize / 2.0f;
}

void playerBulletsOnAlienCollision(const Hashmap &hashmap, const Settings &settings, Aliens *aliens, Bullets *bullets, float cellSize) {
  assert(aliens);
  assert(bullets);

  CheckCollisionInfo checkCollisionInfo = {};
  checkCollisionInfo.gridObjects.x = aliens->x;
  checkCollisionInfo.gridObjects.y = aliens->y;
  checkCollisionInfo.gridObjects.size = aliens->nrAliens;
  checkCollisionInfo.gridObjects.radius = settings.alienSize / 2.0f;

  checkCollisionInfo.collisionObjects.x = bullets->x;
  checkCollisionInfo.collisionObjects.y = bullets->y;
  checkCollisionInfo.collisionObjects.size = bullets->nrBullets;
  checkCollisionInfo.collisionObjects.radius = settings.playerBulletSize / 2.0f;
  checkCollisionInfo.cellSize = cellSize;

  CollisionIds ids = {};
  ids.collisionObjects.init(&device.linearAllocator, 0, SIMD_ALIGNMENT);
  ids.gridObjects.init(&device.linearAllocator, 0, SIMD_ALIGNMENT);
  getCollisionIds(hashmap, checkCollisionInfo, &ids);

  for (uint32_t i = 0; i < ids.collisionObjects.size(); i++) {
    bullets->x[ids.collisionObjects[i]] = bullets->x[--bullets->nrBullets];
    bullets->y[ids.collisionObjects[i]] = bullets->y[bullets->nrBullets];
  }

  for (uint32_t i = 0; i < ids.gridObjects.size(); i++) {
    aliens->x[ids.gridObjects[i]] = aliens->x[--aliens->nrAliens];
    aliens->y[ids.gridObjects[i]] = aliens->y[aliens->nrAliens];
    aliens->colors[ids.gridObjects[i]] = aliens->colors[aliens->nrAliens];
  }
}

void alienBulletsOnPlayerCollision(const Hashmap &hashmap, const Settings &settings, Player *player, Bullets *bullets, float cellSize) {
  assert(player);
  assert(bullets);

  CheckCollisionInfo checkCollisionInfo = {};
  checkCollisionInfo.gridObjects.x = bullets->x;
  checkCollisionInfo.gridObjects.y = bullets->y;
  checkCollisionInfo.gridObjects.size = bullets->nrBullets;
  checkCollisionInfo.gridObjects.radius = settings.alienBulletSize / 2;

  checkCollisionInfo.collisionObjects.x = &player->position.x;
  checkCollisionInfo.collisionObjects.y = &player->position.y;
  checkCollisionInfo.collisionObjects.size = 1;
  checkCollisionInfo.collisionObjects.radius = settings.playerSize / 2.0f;
  checkCollisionInfo.cellSize = cellSize;

  CollisionIds ids = {};
  ids.collisionObjects.init(&device.linearAllocator, 0, SIMD_ALIGNMENT);
  ids.gridObjects.init(&device.linearAllocator, 0, SIMD_ALIGNMENT);
  getCollisionIds(hashmap, checkCollisionInfo, &ids);

  for (uint32_t i = 0; i < ids.gridObjects.size(); i++) {
    bullets->x[ids.gridObjects[i]] = bullets->x[--bullets->nrBullets];
    bullets->y[ids.gridObjects[i]] = bullets->y[bullets->nrBullets];
  }

  if (ids.collisionObjects.size()) {
    player->health--;
  }
}

void alienOnPlayerCollision(const Hashmap &hashmap, const Settings &settings, Aliens *aliens, Player *player, float cellSize) {
  assert(aliens);
  assert(player);

  CheckCollisionInfo checkCollisionInfo = {};
  checkCollisionInfo.gridObjects.x = aliens->x;
  checkCollisionInfo.gridObjects.y = aliens->y;
  checkCollisionInfo.gridObjects.size = aliens->nrAliens;
  checkCollisionInfo.gridObjects.radius = settings.alienSize / 2.0f;
  ;

  checkCollisionInfo.collisionObjects.x = &player->position.x;
  checkCollisionInfo.collisionObjects.y = &player->position.y;
  checkCollisionInfo.collisionObjects.size = 1;
  checkCollisionInfo.collisionObjects.radius = settings.playerSize / 2.0f;
  checkCollisionInfo.cellSize = cellSize;

  CollisionIds ids = {};
  ids.collisionObjects.init(&device.linearAllocator, 0, SIMD_ALIGNMENT);
  ids.gridObjects.init(&device.linearAllocator, 0, SIMD_ALIGNMENT);
  getCollisionIds(hashmap, checkCollisionInfo, &ids);

  for (uint32_t i = 0; i < ids.gridObjects.size(); i++) {
    aliens->x[ids.gridObjects[i]] = aliens->x[--aliens->nrAliens];
    aliens->y[ids.gridObjects[i]] = aliens->y[aliens->nrAliens];
    aliens->colors[ids.gridObjects[i]] = aliens->colors[aliens->nrAliens];
  }
  player->health -= ids.gridObjects.size();
}

void removeBulletsOutsideBorders(Bullets *bullets) {
  assert(bullets);
  for (int32_t i = int32_t(bullets->nrBullets) - 1; i >= 0; i--) {
     if (!inRange(bullets->y[i], 0.0f, mg::getScreenHeight())) {
     bullets->x[i] = bullets->x[--bullets->nrBullets];
     bullets->y[i] = bullets->y[bullets->nrBullets];
    }
  }
}