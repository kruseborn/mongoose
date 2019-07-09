#pragma once
#include <cinttypes>
#include <glm/glm.hpp>

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

void drawSprites(const mg::RenderContext &renderContext, const float *xPositions, const float *yPositions,
                 const glm::vec4 *colors, float size, uint32_t count);

void drawSprites(const mg::RenderContext &renderContext, const float *xPositions, const float *yPositions, const glm::vec4 &color,
                 float size, uint32_t count);

void invadersReset(Invaders *invaders, const Settings &settings);
MinMax transformAliens(const Settings &settings, Aliens *aliens, float dt);
void fireAlienBullets(const Settings &settings, const Aliens &aliens, Bullets *bullets);
void playerBulletsOnAlienCollision(const Hashmap &hashmap, const Settings &settings, Aliens *aliens, Bullets *bullets,
                                   float cellSize);
void alienBulletsOnPlayerCollision(const Hashmap &hashmap, const Settings &settings, Player *player, Bullets *bullets,
                                   float cellSize);
void removeBulletsOutsideBorders(Bullets *bullets);
void alienOnPlayerCollision(const Hashmap &hashmap, const Settings &settings, Aliens *aliens, Player *player, float cellSize);