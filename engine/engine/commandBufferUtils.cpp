#include "vkUtils.h"
#include "vulkanContext.h"

//namespace vkUtils {
void beginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags) {
	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.flags = usageFlags;

	checkResult(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));
}

void endCommandBuffer(VkCommandBuffer commandBuffer) {
	checkResult(vkEndCommandBuffer(commandBuffer));
}

VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level) {
	VkCommandBuffer commandBuffer;

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = vkContext.commandPool;
	commandBufferAllocateInfo.level = level;
	commandBufferAllocateInfo.commandBufferCount = 1;

	checkResult(vkAllocateCommandBuffers(vkContext.device, &commandBufferAllocateInfo, &commandBuffer));
	return commandBuffer;
}
void submitCommandBufferToQueue(VkCommandBuffer commandBuffer) {
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	checkResult(vkQueueSubmit(vkContext.queue, 1, &submitInfo, VK_NULL_HANDLE));
}
void waitForQueueToBeIdle() {
	checkResult(vkQueueWaitIdle(vkContext.queue));
}
void freeCommandBuffer(VkCommandBuffer commandBuffer) {
	vkFreeCommandBuffers(vkContext.device, vkContext.commandPool, 1, &commandBuffer);
}
//}