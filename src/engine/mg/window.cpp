#include "window.h"

#include "mg/logger.h"
#include "mg/mgAssert.h"
#include "mg/mgSystem.h"
#include "mg/mgUtils.h"
#include "vulkan/vkContext.h"
#include "vulkan/vkWindow.h"
#include <cassert>
#include <cstdio>
#include <SDL.h>
#include <cstdlib>

namespace mg {

static glm::vec2 prevXY = {};
static SDL_Window *window;
//static GLFWwindow *window = nullptr;
//static void glfwErrorCallback(int error, const char *description) { printf("Error: %s\n", description); }

glm::vec2 cursorPosition(uint32_t width, uint32_t height) {
  int32_t xpos, ypos;
  SDL_GetMouseState(&xpos, &ypos);
  return {xpos / float(width), 1.0f - ypos / float(height)};
}

static uint64_t getCurrentTimeUs() {
  auto t0 = std::chrono::high_resolution_clock::now();
  auto nanosec = t0.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::microseconds>(nanosec).count();
}

void initWindow(uint32_t width, uint32_t height) {
  if(SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Couldn't initialize SDL: %s",
            SDL_GetError()
        );
    exit(1);
  }

  window = SDL_CreateWindow("Live long and prosper", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
  mgAssert(window && "Could not create SDL window");

  int32_t w, h;
  SDL_GetWindowSize(window, &w, &h);
  LOG("initializing Vulkan");
  initVulkan(window);
  prevXY = cursorPosition(width, height);

  mg::createMgSystem(&mgSystem);
}

void destroyWindow() {
  mg::destroyMgSystem(&mgSystem);
  mg::destroyVulkan();
  SDL_DestroyWindow(window);
  SDL_Quit();
}

bool startFrame() { 
  SDL_Event event;
  SDL_PollEvent(&event);
  return event.type != SDL_QUIT;
}

float getTime() { return float(SDL_GetTicks()); }
void endFrame() { }

float getScreenWidth() { return float(vkContext.screen.width); }
float getScreenHeight() { return float(vkContext.screen.height); };

FrameData getFrameData() {
  FrameData frameData = {};
  static auto startTime = getCurrentTimeUs();
  static auto currentTime = getCurrentTimeUs();
  static auto prevTime = getCurrentTimeUs();
  static uint64_t frames = 0;
  frames++;

  currentTime = getCurrentTimeUs();
  const auto newDt = (currentTime - prevTime) / 100000.0f;
  frameData.dt = frameData.dt * 0.99f + 0.01f * newDt;
  prevTime = getCurrentTimeUs();
  const auto newFps = frames / ((currentTime - startTime) / 1000000.0);
  if (frameData.fps == 0)
    frameData.fps = uint32_t(newFps);
  frameData.fps = uint32_t(frameData.fps * 0.1 + 0.9 * newFps);

  int32_t width, height;
  SDL_GetWindowSize(window, &width, &height);
  frameData.width = width;
  frameData.height = height;
  if (frameData.width != vkContext.screen.width || frameData.height != vkContext.screen.height) {
    frameData.resize = true;
    vkContext.screen.width = frameData.width;
    vkContext.screen.height = frameData.height;
    resizeWindow();
  }

  frameData.mouse.prevXY = prevXY;
  frameData.mouse.xy = cursorPosition(frameData.width, frameData.height);

  prevXY = frameData.mouse.xy;

  SDL_PumpEvents();
  frameData.mouse.left = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT);
  frameData.mouse.middle = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_MIDDLE);
  frameData.mouse.right = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT);

  const Uint8 *state = SDL_GetKeyboardState(NULL);

  frameData.tool = mg::Tool::ROTATE;
  if (state[SDL_SCANCODE_LCTRL]) {
    frameData.tool = mg::Tool::ZOOM;
  } else if(state[SDL_SCANCODE_LSHIFT]) {
    frameData.tool = mg::Tool::PAN;
  }
  frameData.keys.r = state[SDL_SCANCODE_R];
  frameData.keys.n = state[SDL_SCANCODE_N];
  frameData.keys.m = state[SDL_SCANCODE_M];

  frameData.keys.left = state[SDL_SCANCODE_LEFT];
  frameData.keys.right = state[SDL_SCANCODE_RIGHT];
  frameData.keys.space = state[SDL_SCANCODE_SPACE];

  return frameData;
}

} // namespace mg
