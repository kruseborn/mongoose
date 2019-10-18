#pragma once

struct SDL_Window;

namespace mg {

bool createVulkanContext(SDL_Window *window);

void createCommandBuffers();
void createCommandBufferFences();
void createCommandBufferSemaphores();

void createCommandBufferSemaphores();
void destroyVulkanWindow();
void destoyInstance();

void resizeWindow();
} // namespace mg