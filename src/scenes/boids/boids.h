#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <span>

void init(float w, float h, float d);

std::span<glm::vec3> simulate(float w, float h, float d, float dt);