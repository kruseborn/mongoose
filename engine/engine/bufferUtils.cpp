#include "vkUtils.h"
#include "vulkanContext.h"
#include <cstring>

//namespace vkUtils {
void createBuffer(VkBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage) {
	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.size = size;
	vertexBufferInfo.usage = usage;

	checkResult(vkCreateBuffer(vkContext.device, &vertexBufferInfo, nullptr, &buffer));
}
void allocateDeviceMemoryFromBuffer(VkBuffer &buffer, VkDeviceMemory &memory, VkMemoryPropertyFlags type) {
	VkMemoryRequirements memReqs;
	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	vkGetBufferMemoryRequirements(vkContext.device, buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, type);
	checkResult(vkAllocateMemory(vkContext.device, &memAlloc, nullptr, &memory));
	checkResult(vkBindBufferMemory(vkContext.device, buffer, memory, 0));
}
void copyDataToDeviceMemory(VkDeviceMemory &memory, void *data, VkDeviceSize size) {
	void *pData;
	checkResult(vkMapMemory(vkContext.device, memory, 0, size, 0, &pData));
	memcpy(pData, data, size);
	vkUnmapMemory(vkContext.device, memory);
}
void destoyBuffer(VkBuffer buffer) {
	vkDestroyBuffer(vkContext.device, buffer, nullptr);
}
void destroyDeviceMemory(VkDeviceMemory memory) {
	vkFreeMemory(vkContext.device, memory, nullptr);
}

void createDeviceVisibleBuffer(void *data, const uint32_t size, VkBuffer &buffer, VkDeviceMemory &deviceMemory, VkBufferUsageFlags usage) {
	struct StagingBuffer {
		VkDeviceMemory memory;
		VkBuffer buffer;
	} stagingBuffer;

	createBuffer(stagingBuffer.buffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	allocateDeviceMemoryFromBuffer(stagingBuffer.buffer, stagingBuffer.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	copyDataToDeviceMemory(stagingBuffer.memory, data, size);

	createBuffer(buffer, size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	allocateDeviceMemoryFromBuffer(buffer, deviceMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Now copy data from host visible buffer to gpu only buffer
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkCommandBuffer copyCommandBuffer = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	beginCommandBuffer(copyCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(copyCommandBuffer, stagingBuffer.buffer, buffer, 1, &copyRegion);

	vkEndCommandBuffer(copyCommandBuffer);

	submitCommandBufferToQueue(copyCommandBuffer);
	waitForQueueToBeIdle();
	freeCommandBuffer(copyCommandBuffer);

	vkDestroyBuffer(vkContext.device, stagingBuffer.buffer, nullptr);
	vkFreeMemory(vkContext.device, stagingBuffer.memory, nullptr);
}
void createHostVisibleBuffer(void *data, const uint32_t size, VkBuffer buffer, VkDeviceMemory deviceMemory, VkBufferUsageFlags usage) {
	createBuffer(buffer, size, usage);
	allocateDeviceMemoryFromBuffer(buffer, deviceMemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	copyDataToDeviceMemory(deviceMemory, data, size);
}