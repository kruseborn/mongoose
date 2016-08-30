#pragma once
#include "glm\glm.hpp"

void renderFinalDeferred(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix);
void renderMRT(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix);
void renderSSAO(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix);
void renderBlurSSAO(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix);