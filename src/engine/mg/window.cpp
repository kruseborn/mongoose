#include "window.h"

#include "mg/logger.h"
#include "mg/mgAssert.h"
#include "mg/mgSystem.h"
#include "mg/mgUtils.h"
#include "vulkan/vkContext.h"
#include "vulkan/vkWindow.h"
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>

namespace mg {

static glm::vec2 prevXY = {};
static GLFWwindow *window = nullptr;
static void glfwErrorCallback(int error, const char *description) { printf("Error: %s\n", description); }

glm::vec2 cursorPosition(uint32_t width, uint32_t height) {
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  return {xpos / float(width), 1.0f - ypos / float(height)};
}

static uint64_t getCurrentTimeUs() {
  auto t0 = std::chrono::high_resolution_clock::now();
  auto nanosec = t0.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::microseconds>(nanosec).count();
}

void initWindow(uint32_t width, uint32_t height) {
  mgAssertDesc(glfwInit(), "could not init glfw");
  glfwSetErrorCallback(glfwErrorCallback);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  mgAssertDesc(glfwVulkanSupported(), "gltw vulkan not supported");
  window = glfwCreateWindow(width, height, "Live long and prosper", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    mgAssertDesc(false, "glfwCreateWindow return null");
  }
  int32_t w, h;
  glfwGetWindowSize(window, &w, &h);
  LOG("initializing Vulkan");
  initVulkan(window);
  prevXY = cursorPosition(width, height);

  mg::createMgSystem(&mgSystem);
}

void destroyWindow() { 
  mg::destroyMgSystem(&mgSystem); 
  mg::destroyVulkan();
}

bool startFrame() { return !glfwWindowShouldClose(window); }
float getTime()  { return float(glfwGetTime()); }
void endFrame() { glfwPollEvents(); }

float getScreenWidth() { return float(vkContext.screen.width); }
float getScreenHeight(){ return float(vkContext.screen.height); };

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
  glfwGetWindowSize(window, &width, &height);
  
  frameData.width = width;
  frameData.height = height;
  if (width != vkContext.screen.width || height != vkContext.screen.height) {
    frameData.resize = true;

    vkContext.screen.width = width;
    vkContext.screen.height = height;
    resizeWindow();
  }

  frameData.mouse.prevXY = prevXY;
  frameData.mouse.xy = cursorPosition(width, height);

  prevXY = frameData.mouse.xy;

  frameData.mouse.left = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  frameData.mouse.middle = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
  frameData.mouse.right = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

  frameData.tool = mg::Tool::ROTATE;
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
    frameData.tool = mg::Tool::ZOOM;
  } else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    frameData.tool = mg::Tool::PAN;
  }
  frameData.keys.r = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
  frameData.keys.n = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
  frameData.keys.m = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;

  frameData.keys.left = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
  frameData.keys.right = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
  frameData.keys.space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;


  return frameData;
}

} // namespace mg
