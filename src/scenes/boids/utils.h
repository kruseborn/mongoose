#pragma once

struct BoidsTime {
  uint64_t count = 0;
  uint32_t updatePositionTime = 0;
  uint32_t applyBehaviourTime = 0;
  uint32_t moveInside = 0;

  float textPos = 0;
  float textApply = 0;
  float textMoveInside = 0;
};