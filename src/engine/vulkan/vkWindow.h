#pragma once

struct GLFWwindow;

namespace mg {

bool createVulkanContext(GLFWwindow *window);

void createCommandBuffers();
void createCommandBufferFences();
void createCommandBufferSemaphores();

void createCommandBufferSemaphores();
void destroyVulkanWindow();
void destoyInstance();

} // namespace mg