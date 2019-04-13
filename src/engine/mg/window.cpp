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

void endFrame() { glfwPollEvents(); }

FrameData getFrameData() {
  FrameData frameData = {};
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
  frameData.mouse.xy = cursorPosition(float(width), float(height));

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


  return frameData;
}

} // namespace mg