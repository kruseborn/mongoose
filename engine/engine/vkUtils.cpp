#include "vkUtils.h"

int memoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask) {
	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
		if ((type_bits & 1) == 1) {
			if ((vkContext.deviceMemoryProperties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
				return i;
		}
		type_bits >>= 1;
	}

	printf("Could not find memory type");
	return 0;
}

uint32_t getMemoryType(uint32_t typeBits, VkFlags properties) {
	for (uint32_t i = 0; i < 32; i++) {
		if ((typeBits & 1) == 1) {
			if ((vkContext.deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;

			}
		}
		typeBits >>= 1;
	}
	assert(false);
	return -1;
}
