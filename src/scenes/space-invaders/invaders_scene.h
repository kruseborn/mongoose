#pragma once

#include "allocator.h"
#include "types.h"

namespace mg {
  struct FrameData;
}

struct Invaders {
  Bullets playerBullets;
  Bullets alienBullets;
  Aliens aliens;
  Player player;
};

struct Settings {
  float playerSize = 20.0f;
  float alienSize = 20.0f;
  float alienBulletSize = 10.0f;
  float playerBulletSize = 10.0f;
  float playerSpeed = 1000.0f;
  float playerBulletSpeed = 500.0f;
  float playerFireColdDown = 0.05f;
  uint32_t startHealth = 3;
  float alienSpeed = 150.0f;
  float alienBulletSpeed = 100.0f;
  float cellSize = 8.0f;
  uint32_t aliensRows = ALIEN_ROWS;
  uint32_t aliensCols = ALIEN_COLS;
};

struct Device {
  LinearAllocator linearAllocator;
};

extern Device device;

void invadersInit(Invaders *invaders);
void invadersDestroy(Invaders *invaders);
void invadersReset(Invaders *invaders);
void invadersSimulate(Invaders *invaders, const mg::FrameData &frameData, float dt);
void invadersRender(const Invaders &invaders, const mg::FrameData &frameData);
void invadersEndFrame();