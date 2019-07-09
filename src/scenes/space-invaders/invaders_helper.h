#pragma once
#include <cinttypes>

struct Aliens;
struct Hashmap;
struct Bullets;
struct Player;
struct Invaders;
struct Settings;
struct MinMax;

namespace mg {
    struct RenderContext;
}

void drawSprites(const mg::RenderContext &renderContext, const float *xPositions, const float *yPosition, float size, uint32_t count);

void invadersReset(Invaders *invaders, const Settings &settings);
MinMax transformAliens(Aliens *aliens, float dt);
void fireAlienBullets(const Aliens &aliens, Bullets *bullets);
void playerBulletsOnAlienCollision(const Hashmap &hashmap, Aliens *aliens, Bullets *bullets, float cellSize);
void alienBulletsOnPlayerCollision(const Hashmap &hashmap, Player *player, Bullets *bullets, float cellSize);
void removeBulletsOutsideBorders(Bullets *bullets);
void alienOnPlayerCollision(const Hashmap &hashmap, Aliens *aliens, Player *player, float cellSize);