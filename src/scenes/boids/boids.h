#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <span>

void init(float w, float h);

std::span<glm::vec2> simulate(float w, float h, float dt);