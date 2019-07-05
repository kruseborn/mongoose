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
  float playerSpeed = 100.0f;
  float playerBulletSpeed = 50.0f;
  float playerFireColdDown = 1.0f;
  uint32_t startHealth = 3;
  float alienSpeed = 15.0f;
  float alienBulletSpeed = 10.0f;
  float cellSize = 32.0f;
  uint32_t aliensRows = 5;
  uint32_t aliensCols = 12;
};

struct Device {
  LinearAllocator linearAllocator;
};

extern Device device;

void invadersInit(Invaders *invaders);
void invadersDestroy(Invaders *invaders);
void invadersReset(Invaders *invaders);
void invadersSimulate(Invaders *invaders, const mg::FrameData &frameData, float dt);
void invadersRender(const Invaders &invaders);
void invadersEndFrame();