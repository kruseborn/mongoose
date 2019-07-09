#pragma once

#include <cinttypes>

namespace mg {
    struct FrameData;
}

struct Player;

void transformPlayer(Player *player, uint32_t screenWidth, float spriteSize, const mg::FrameData &frameData, float dt);
bool shouldFirePlayerBullet(Player *player, const mg::FrameData &frameData, float dt);
