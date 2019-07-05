#include "invaders_helper.h"
#include "collisions.h"
#include "hashmap.h"
#include "invaders_scene.h"
#include "transforms.h"
#include "types.h"
#include "invaders_utils.h"
#include <cstdlib>

void drawSprites(const float *xPositions, const float *yPositions, uint32_t size, const mg::FrameData &frameData) {
  assert(xPositions);
  assert(yPositions);
  for (uint32_t i = 0; i < size; i++);
//    engine->drawSprite(sprite, int32_t(xPositions[i]), int32_t(yPositions[i]));
}

void drawSprites(const float *xPositions, const float *yPositions, const mg::FrameData &frameData) {
  assert(xPositions);
  assert(yPositions);
  //assert(engine);
  //assert(sprites);
  //for (uint32_t i = 0; i < size; i++)
  //  engine->drawSprite(sprites[i], int32_t(xPositions[i]), int32_t(yPositions[i]));
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
  alienBullets.direction = {0, 1};
  //constexpr Engine::Sprite alienSprites[] = {Engine::Sprite::Enemy1, Engine::Sprite::Enemy2};
  for (uint32_t y = 0; y < settings.aliensRows; y++) {
    for (uint32_t x = 0; x < settings.aliensCols; x++) {
    //  aliens.sprites[aliens.nrAliens] = alienSprites[x & 1];
     // aliens.x[aliens.nrAliens] = float(Engine::SpriteSize * x);
     // aliens.y[aliens.nrAliens++] = float(y * Engine::SpriteSize);
    }
  }
  // player
  player.health = settings.startHealth;
  player.weaponColdDown = settings.playerFireColdDown;
  player.speed = settings.playerSpeed;
  //player.position.x = float(Engine::CanvasWidth >> 1);
  //player.position.y = Engine::CanvasHeight - Engine::SpriteSize;
  playerBullets.speed = settings.playerBulletSpeed;
  playerBullets.direction = {0, -1};
}

MinMax transformAliens(Aliens *aliens, float dt) {
  assert(aliens);
  const auto dir = vec2{float(AlienXDirs[aliens->dirIndex]), float(AlienYDirs[aliens->dirIndex])};
  const auto minMax = transformPositionsCalculateLimits(aliens->x, aliens->y, aliens->nrAliens, dir, aliens->speed, dt);
  Limits limits = {};
  // limits.left = Engine::SpriteSize * 0.1f;
  // limits.right = Engine::CanvasWidth - Engine::SpriteSize;
  // limits.down = Engine::SpriteSize * 0.8f;
  // changeAlienDirectionIfNeeded(aliens, minMax, limits, dt);
  return minMax;
}

void fireAlienBullets(const Aliens &aliens, Bullets *bullets) {
  assert(bullets);
  const auto alienIndex = rand() % (aliens.nrAliens * 100);
  if (alienIndex >= aliens.nrAliens)
    return;
  assert(bullets->nrBullets < MAX_BULLETS);
  bullets->x[bullets->nrBullets] = aliens.x[alienIndex];
  //bullets->y[bullets->nrBullets++] = aliens.y[alienIndex] + float(Engine::SpriteSize >> 1);
}

void playerBulletsOnAlienCollision(const Hashmap &hashmap, Aliens *aliens, Bullets *bullets, float cellSize) {
  assert(aliens);
  assert(bullets);

  // constexpr uint32_t alienRadius = Engine::SpriteSize >> 1;
  // constexpr uint32_t playerBulletRadius = Engine::SpriteSize >> 5;

  CheckCollisionInfo checkCollisionInfo = {};
  checkCollisionInfo.gridObjects.x = aliens->x;
  checkCollisionInfo.gridObjects.y = aliens->y;
  checkCollisionInfo.gridObjects.size = aliens->nrAliens;
  //checkCollisionInfo.gridObjects.radius = alienRadius;

  checkCollisionInfo.collisionObjects.x = bullets->x;
  checkCollisionInfo.collisionObjects.y = bullets->y;
  checkCollisionInfo.collisionObjects.size = bullets->nrBullets;
  //checkCollisionInfo.collisionObjects.radius = playerBulletRadius;
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
    //aliens->sprites[ids.gridObjects[i]] = aliens->sprites[aliens->nrAliens];
  }
}

void alienBulletsOnPlayerCollision(const Hashmap &hashmap, Player *player, Bullets *bullets, float cellSize) {
  assert(player);
  assert(bullets);

  // constexpr uint32_t playerRadius = Engine::SpriteSize >> 1;
  // constexpr uint32_t alienBulletRadius = Engine::SpriteSize >> 4;

  CheckCollisionInfo checkCollisionInfo = {};
  checkCollisionInfo.gridObjects.x = bullets->x;
  checkCollisionInfo.gridObjects.y = bullets->y;
  checkCollisionInfo.gridObjects.size = bullets->nrBullets;
  //checkCollisionInfo.gridObjects.radius = alienBulletRadius;

  checkCollisionInfo.collisionObjects.x = &player->position.x;
  checkCollisionInfo.collisionObjects.y = &player->position.y;
  checkCollisionInfo.collisionObjects.size = 1;
  //checkCollisionInfo.collisionObjects.radius = playerRadius;
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

void alienOnPlayerCollision(const Hashmap &hashmap, Aliens *aliens, Player *player, float cellSize) {
  assert(aliens);
  assert(player);

  //constexpr uint32_t radius = Engine::SpriteSize >> 1;

  CheckCollisionInfo checkCollisionInfo = {};
  checkCollisionInfo.gridObjects.x = aliens->x;
  checkCollisionInfo.gridObjects.y = aliens->y;
  checkCollisionInfo.gridObjects.size = aliens->nrAliens;
  //checkCollisionInfo.gridObjects.radius = radius;

  checkCollisionInfo.collisionObjects.x = &player->position.x;
  checkCollisionInfo.collisionObjects.y = &player->position.y;
  checkCollisionInfo.collisionObjects.size = 1;
  //checkCollisionInfo.collisionObjects.radius = radius;
  checkCollisionInfo.cellSize = cellSize;

  CollisionIds ids = {};
  ids.collisionObjects.init(&device.linearAllocator, 0, SIMD_ALIGNMENT);
  ids.gridObjects.init(&device.linearAllocator, 0, SIMD_ALIGNMENT);
  getCollisionIds(hashmap, checkCollisionInfo, &ids);

  for (uint32_t i = 0; i < ids.gridObjects.size(); i++) {
    aliens->x[ids.gridObjects[i]] = aliens->x[--aliens->nrAliens];
    aliens->y[ids.gridObjects[i]] = aliens->y[aliens->nrAliens];
    //aliens->sprites[ids.gridObjects[i]] = aliens->sprites[aliens->nrAliens];
  }
  player->health -= ids.gridObjects.size();
}

void removeBulletsOutsideBorders(Bullets *bullets) {
  assert(bullets);
  for (int32_t i = int32_t(bullets->nrBullets) - 1; i >= 0; i--) {
    //if (!inRange(bullets->y[i], 0.0f, float(Engine::CanvasHeight))) {
     // bullets->x[i] = bullets->x[--bullets->nrBullets];
     // bullets->y[i] = bullets->y[bullets->nrBullets];
    //}
  }
}