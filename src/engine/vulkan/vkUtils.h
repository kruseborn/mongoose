#pragma once

#include <cassert>
#include <cstdio>
#include <vector>

#include "vkContext.h"
#include "mg/mgAssert.h"
#include "mg/logger.h"

namespace mg {

int32_t findMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties &memoryProperties, uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties, VkMemoryPropertyFlags preferredProperties = 0);

void setViewPort(float x, float y, float width, float height, float minDepth, float maxDepth);
void setFullscreenViewport();

void beginRendering();
void endRendering();
void acquireNextSwapChainImage();
void waitForDeviceIdle();

inline char* errorString(VkResult errorCode) {
	switch (errorCode) {
#define STR(r) case VK_ ##r: return (char*)#r
		STR(NOT_READY);
		STR(TIMEOUT);
		STR(EVENT_SET);
		STR(EVENT_RESET);
		STR(INCOMPLETE);
		STR(ERROR_OUT_OF_HOST_MEMORY);
		STR(ERROR_OUT_OF_DEVICE_MEMORY);
		STR(ERROR_INITIALIZATION_FAILED);
		STR(ERROR_DEVICE_LOST);
		STR(ERROR_MEMORY_MAP_FAILED);
		STR(ERROR_LAYER_NOT_PRESENT);
		STR(ERROR_EXTENSION_NOT_PRESENT);
		STR(ERROR_FEATURE_NOT_PRESENT);
		STR(ERROR_INCOMPATIBLE_DRIVER);
		STR(ERROR_TOO_MANY_OBJECTS);
		STR(ERROR_FORMAT_NOT_SUPPORTED);
		STR(ERROR_SURFACE_LOST_KHR);
		STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		STR(SUBOPTIMAL_KHR);
		STR(ERROR_OUT_OF_DATE_KHR);
		STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		STR(ERROR_VALIDATION_FAILED_EXT);
		STR(ERROR_INVALID_SHADER_NV);
#undef STR
	default:
		return (char*)"UNKNOWN_ERROR";
	}
}
inline VkResult checkResult(VkResult result) {
	if (result != VK_SUCCESS) {
		char st[60];
		snprintf(st, sizeof(st), "Fatal : VkResult returned %s", errorString(result));
		LOG(st);
		mgAssert(result == VK_SUCCESS);
	}
	return result;
}

struct SubAllocationGui {
  bool free;
  uint32_t offset, size;
};

struct GuiAllocation {
  std::string type;
  uint32_t totalSize;
  uint32_t memoryTypeIndex, heapIndex;
  std::vector<SubAllocationGui> elements;
  uint32_t totalNrOfAllocation;
  uint32_t allocationNotFreed;
  bool showFullSize;
  bool largeStagingAllocation;
  bool largeDeviceAllocation;
};

} //namespace mg