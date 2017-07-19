#include "vulkanContext.h"
#include "vkUtils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <algorithm>
#include "framebufferAttachment.h"
#include "textures.h"
#include "deferredRenderPass.h"
#include <cstring>

#define NUM_COMMAND_BUFFERS 2
#define MAX_SWAP_CHAIN_IMAGES 8

#define ENABLE_DEBUGGING false

static uint32_t numOfSwapChainImages;
static char* DEBUG_LAYER = "VK_LAYER_LUNARG_standard_validation";


namespace {
	VkPhysicalDevice physicalDevice;
	VkExtent2D swapChainExtent;
	VkFormat swapChainFormat;
	VkSwapchainKHR oldSwapChain;
	VkSwapchainKHR swapChain;

	VkImage swapChainImages[MAX_SWAP_CHAIN_IMAGES];
	VkImageView swapChainImageViews[MAX_SWAP_CHAIN_IMAGES];
	VkFramebuffer swapChainFramebuffers[MAX_SWAP_CHAIN_IMAGES];

	VkSemaphore imageAquiredSemaphore[MAX_SWAP_CHAIN_IMAGES];
	uint32_t currentSwapchainBuffer;

	uint32_t currentCommandBuffer = 0;
	VkCommandBuffer	 commandBuffers[NUM_COMMAND_BUFFERS];
	VkFence commandBufferFences[NUM_COMMAND_BUFFERS];
	bool commandBufferSubmitted[NUM_COMMAND_BUFFERS];

	struct {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} depthStencil;
	VkFormat depthFormat;
	GLFWwindow* window;

	VkInstance instance;
	VkSurfaceKHR windowSurface;
	VkDebugReportCallbackEXT callback;

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	VkFramebuffer deferredFrameBuffers[MAX_SWAP_CHAIN_IMAGES];


}

void createCommandBuffers() {
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	for (int i = 0; i < NUM_COMMAND_BUFFERS; ++i) {
		commandBuffers[i] = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		checkResult(vkCreateFence(vkContext.device, &fenceCreateInfo, NULL, &commandBufferFences[i]));
	}
}

void preRendering() {
	swapDynamicBuffers();

	if (commandBufferSubmitted[currentCommandBuffer]) {
		checkResult(vkWaitForFences(vkContext.device, 1, &commandBufferFences[currentCommandBuffer], VK_TRUE, UINT64_MAX));
	}
	checkResult(vkResetFences(vkContext.device, 1, &commandBufferFences[currentCommandBuffer]));

	beginCommandBuffer(commandBuffers[currentCommandBuffer]);
	vkContext.commandBuffer = commandBuffers[currentCommandBuffer];

	checkResult(vkAcquireNextImageKHR(vkContext.device, swapChain, UINT64_MAX, imageAquiredSemaphore[currentCommandBuffer], VK_NULL_HANDLE, &currentSwapchainBuffer));
}
void beginRendering() {
	VkRect2D renderArea;
	renderArea.offset.x = 0;
	renderArea.offset.y = 0;
	renderArea.extent.width = vkContext.width;
	renderArea.extent.height = vkContext.height;

	VkClearValue clearValues[2];
	clearValues[0] = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderArea = renderArea;
	renderPassBeginInfo.renderPass = vkContext.renderPass;
	renderPassBeginInfo.framebuffer = swapChainFramebuffers[currentCommandBuffer];
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	vkCmdSetScissor(vkContext.commandBuffer, 0, 1, &renderArea);

	VkViewport viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = float(vkContext.width);
	viewport.height = float(vkContext.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(vkContext.commandBuffer, 0, 1, &viewport);

	vkCmdBeginRenderPass(vkContext.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}
void beginDeferredRendering() {
	VkClearValue clearValues[RENDERPASS::SIZE];
	clearValues[NORMAL].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[ALBEDO].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[DEPTHOUT].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[DEPTH].depthStencil = { 1.0f, 0 };
	clearValues[SWAPCHAIN].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[SSAO].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[SSAOBLUR].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };


	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = vkContext.deferredRenderPass;
	renderPassBeginInfo.framebuffer = deferredFrameBuffers[currentCommandBuffer];
	renderPassBeginInfo.renderArea.extent.width = vkContext.width;
	renderPassBeginInfo.renderArea.extent.height = vkContext.height;
	renderPassBeginInfo.clearValueCount = RENDERPASS::SIZE;
	renderPassBeginInfo.pClearValues = clearValues;

	VkViewport viewport = {};
	viewport.height = (float)vkContext.height;
	viewport.width = (float)vkContext.width;
	viewport.minDepth = (float) 0.0f;
	viewport.maxDepth = (float) 1.0f;
	vkCmdSetViewport(vkContext.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.extent.width = vkContext.width;
	scissor.extent.height = vkContext.height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(vkContext.commandBuffer, 0, 1, &scissor);

	vkCmdBeginRenderPass(vkContext.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void postRendering() {
	checkResult(vkEndCommandBuffer(vkContext.commandBuffer));

	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &commandBuffers[currentCommandBuffer];
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &imageAquiredSemaphore[currentCommandBuffer];
	submit_info.pWaitDstStageMask = &waitDstStageMask;

	checkResult(vkQueueSubmit(vkContext.queue, 1, &submit_info, commandBufferFences[currentCommandBuffer]));

	commandBufferSubmitted[currentCommandBuffer] = true;
	currentCommandBuffer = (currentCommandBuffer + 1) % NUM_COMMAND_BUFFERS;

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &currentSwapchainBuffer;

	checkResult(vkQueuePresentKHR(vkContext.queue, &presentInfo));

}
void endRendering() {
	vkCmdEndRenderPass(vkContext.commandBuffer);
}
void endDefferedRendering() {
	vkCmdEndRenderPass(vkContext.commandBuffer);
}

static VkBool32 debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData) {
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		printf("ERROR: [ %s ] Code %d : %s\n", pLayerPrefix, msgCode, pMsg);
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		printf("WARNING: [ %s ] Code %d : %s\n", pLayerPrefix, msgCode, pMsg);
	}
	return VK_FALSE;
}

static void createDebugCallback() {
	if (ENABLE_DEBUGGING) {
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugCallback;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

		PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		checkResult(CreateDebugReportCallback(instance, &createInfo, nullptr, &callback));
	}
}

void createSemaphores() {

}
static void createInstance() {
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "VulkanClear";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "ClearScreenEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 17);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// Get instance extensions required by GLFW to draw to window
	unsigned int glfwExtensionCount;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions;
	for (size_t i = 0; i < glfwExtensionCount; i++) {
		extensions.push_back(glfwExtensions[i]);
	}
	if (ENABLE_DEBUGGING) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	// Check for extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	assert(extensionCount > 0);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

	printf("supported extensions : ");
	for (const auto& extension : availableExtensions) {
		printf("%s, ", extension.extensionName);
	}
	printf("\n");


	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (ENABLE_DEBUGGING) {
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = &DEBUG_LAYER;
	}

	// Initialize Vulkan instance
	checkResult(vkCreateInstance(&createInfo, nullptr, &instance));
}
static void onWindowResized(GLFWwindow* window, int width, int height) {
}
static void initWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(vkContext.width, vkContext.height, "Live long and prosper", nullptr, nullptr);
	glfwSetWindowSizeCallback(window, onWindowResized);
}
static void createWindowSurface() {
	checkResult(glfwCreateWindowSurface(instance, window, nullptr, &windowSurface));
}

static void findPhysicalDevice() {
	// Try to find 1 Vulkan supported device
	// Note: perhaps refactor to loop through devices and find first one that supports all required features and extensions
	uint32_t deviceCount = 0;
	if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0) {
		printf("failed to get number of physical devices\n");
		exit(1);
	}

	deviceCount = 1;
	VkResult res = vkEnumeratePhysicalDevices(instance, &deviceCount, &physicalDevice);
	if (res != VK_SUCCESS && res != VK_INCOMPLETE) {
		printf("enumerating physical devices failed!\n");
		exit(1);
	}
	if (deviceCount == 0) {
		printf("no physical devices that support vulkan!\n");
		exit(1);
	}
	// Check device features
	// Note: will apiVersion >= appInfo.apiVersion? Probably yes, but spec is unclear.
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(physicalDevice, &vkContext.deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	switch (vkContext.deviceProperties.vendorID)
	{
	case 0x8086:
		printf("Vendor: Intel\n");
		break;
	case 0x10DE:
		printf("Vendor: NVIDIA\n");
		break;
	case 0x1002:
		printf("Vendor: AMD\n");
		break;
	default:
		printf("Vendor: Unknown (0x%x)\n", vkContext.deviceProperties.vendorID);
	}

	printf("Device: %s\n", vkContext.deviceProperties.deviceName);

	uint32_t supportedVersion[] = {
		VK_VERSION_MAJOR(vkContext.deviceProperties.apiVersion),
		VK_VERSION_MINOR(vkContext.deviceProperties.apiVersion),
		VK_VERSION_PATCH(vkContext.deviceProperties.apiVersion)
	};
	printf("physical device supports version %i.%i.%i\n", supportedVersion[0], supportedVersion[1], supportedVersion[2]);
}
static void checkSwapChainSupport() {
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
	assert(extensionCount > 0);

	std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, deviceExtensions.data());

	for (const auto& extension : deviceExtensions) {
		if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
			return;
		}
	}

	printf("physical device doesn't support swap chains\n");
	exit(1);
}

static void createLogicalDevice() {
	// Greate one graphics queue and optionally a separate presentation queue
	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfo[2] = {};

	queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo[0].queueFamilyIndex = vkContext.graphicsQueueFamilyIndex;
	queueCreateInfo[0].queueCount = 1;
	queueCreateInfo[0].pQueuePriorities = &queuePriority;

	queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo[0].queueFamilyIndex = vkContext.presentQueueFamilyIndex;
	queueCreateInfo[0].queueCount = 1;
	queueCreateInfo[0].pQueuePriorities = &queuePriority;

	// Create logical device from physical device
	// Note: there are separate instance and device extensions!
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;

	assert(vkContext.graphicsQueueFamilyIndex == vkContext.presentQueueFamilyIndex);
	deviceCreateInfo.queueCreateInfoCount = 1;

	// Necessary for shader (for some reason)
	VkPhysicalDeviceFeatures enabledFeatures = {};
	enabledFeatures.shaderClipDistance = VK_TRUE;
	enabledFeatures.shaderCullDistance = VK_TRUE;

	const char* deviceExtensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = &deviceExtensions;
	deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

	if (ENABLE_DEBUGGING) {
		deviceCreateInfo.enabledLayerCount = 1;
		deviceCreateInfo.ppEnabledLayerNames = &DEBUG_LAYER;
	}

	checkResult(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &vkContext.device));

	// Get graphics and presentation queues (which may be the same)
	vkGetDeviceQueue(vkContext.device, vkContext.graphicsQueueFamilyIndex, 0, &vkContext.queue);
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &vkContext.deviceMemoryProperties);
}


static VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	// We can either choose any format
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
	}

	// Or go with the standard format - if available
	for (const auto& availableSurfaceFormat : availableFormats) {
		if (availableSurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM) {
			return availableSurfaceFormat;
		}
	}

	// Or fall back to the first available one
	return availableFormats[0];
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) {
	if (surfaceCapabilities.currentExtent.width == -1) {
		VkExtent2D swapChainExtent = {};

		swapChainExtent.width = std::min(std::max(vkContext.width, surfaceCapabilities.minImageExtent.width), surfaceCapabilities.maxImageExtent.width);
		swapChainExtent.height = std::min(std::max(vkContext.height, surfaceCapabilities.minImageExtent.height), surfaceCapabilities.maxImageExtent.height);

		return swapChainExtent;
	}
	else {
		return surfaceCapabilities.currentExtent;
	}
}

static VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> presentModes) {
	for (const auto& presentMode : presentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		}
	}
	// If mailbox is unavailable, fall back to FIFO (guaranteed to be available)
	return VK_PRESENT_MODE_FIFO_KHR;
}


static void findQueueFamilies() {
	// Check queue families
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	assert(queueFamilyCount != 0);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	bool foundGraphicsQueueFamily = false;
	bool foundPresentQueueFamily = false;

	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &presentSupport);

		if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			vkContext.graphicsQueueFamilyIndex = i;
			foundGraphicsQueueFamily = true;

			if (presentSupport) {
				vkContext.presentQueueFamilyIndex = i;
				foundPresentQueueFamily = true;
				break;
			}
		}

		if (!foundPresentQueueFamily && presentSupport) {
			vkContext.presentQueueFamilyIndex = i;
			foundPresentQueueFamily = true;
		}
	}
	assert(foundGraphicsQueueFamily && foundPresentQueueFamily);
}


static void createSwapChain() {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	checkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &surfaceCapabilities));
	if (surfaceCapabilities.currentExtent.width != vkContext.width || surfaceCapabilities.currentExtent.height != vkContext.height) {
		printf("Surface doesn't match video width or height");
		exit(1);
	}

	uint32_t formatCount;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0) {
		printf("failed to get number of supported surface formats\n");
		exit(1);
	}

	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	checkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, surfaceFormats.data()));

	// Find supported present modes
	uint32_t presentModeCount;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0) {
		printf("failed to get number of supported presentation modes\n");
		exit(1);
	}

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	checkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, presentModes.data()));

	// Determine number of images for swap chain
	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount) {
		imageCount = surfaceCapabilities.maxImageCount;
	}

	// Select a surface format
	VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(surfaceFormats);
	// Select swap chain size
	swapChainExtent = chooseSwapExtent(surfaceCapabilities);

	// Determine transformation to use (preferring no transform)
	VkSurfaceTransformFlagBitsKHR surfaceTransform;
	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else {
		surfaceTransform = surfaceCapabilities.currentTransform;
	}
	// Choose presentation mode (preferring MAILBOX ~= triple buffering)
	VkPresentModeKHR presentMode = choosePresentMode(presentModes);

	// Finally, create the swap chain
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = windowSurface;
	createInfo.minImageCount = 2;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapChainExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.preTransform = surfaceTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapChain;

	checkResult(vkCreateSwapchainKHR(vkContext.device, &createInfo, nullptr, &swapChain));

	if (oldSwapChain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(vkContext.device, oldSwapChain, nullptr);
	}
	oldSwapChain = swapChain;

	swapChainFormat = surfaceFormat.format;

	// Store the images used by the swap chain
	// Note: these are the images that swap chain image indices refer to
	// Note: actual number of images may differ from requested number, since it's a lower bound
	checkResult(vkGetSwapchainImagesKHR(vkContext.device, swapChain, &numOfSwapChainImages, nullptr));
	assert(numOfSwapChainImages > 0 && numOfSwapChainImages < MAX_SWAP_CHAIN_IMAGES);
	checkResult(vkGetSwapchainImagesKHR(vkContext.device, swapChain, &numOfSwapChainImages, swapChainImages));
}

static void createRenderPass() {
	VkAttachmentDescription attachments[2] = {};
	//color attachment
	attachments[0].format = swapChainFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//depth attachment
	attachments[1].format = depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPassDescription = {};
	subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPassDescription.colorAttachmentCount = 1;
	subPassDescription.pColorAttachments = &colorAttachmentReference;
	subPassDescription.pDepthStencilAttachment = &depthReference;

	// Create the render pass
	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 2;
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subPassDescription;

	checkResult(vkCreateRenderPass(vkContext.device, &createInfo, nullptr, &vkContext.renderPass));
}

VkFormat getSupportedDepthFormat() {
	VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	// Since all depth formats may be optional, we need to find a suitable depth format to use
	// Start with the highest precision packed format
	VkFormat depthFormats[5] = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM
	};
	for (auto& format : depthFormats) {
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
		// Format must support depth stencil attachment for optimal tiling
		if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			depthFormat = format;
			return depthFormat;
		}
	}
	assert(false && "could not find any depth format that was supported");
	return depthFormat;
}

static void setupDepthStencil() {
	depthFormat = getSupportedDepthFormat();
	VkImageCreateInfo imageCreateInfo = createImageCreateInfo();

	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.extent = { vkContext.width, vkContext.height, 1 };
	imageCreateInfo.format = depthFormat;

	depthStencil.image = createImage(imageCreateInfo);

	allocateDeviceMemoryForImage(depthStencil.image, depthStencil.mem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkImageViewCreateInfo depthStencilView = createImageViewCreateInfo();
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.format = depthFormat;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.image = depthStencil.image;

	depthStencil.view = createImageView(depthStencilView);
}

static void createFramebuffers() {
	VkImageView attachments[MAX_SWAP_CHAIN_IMAGES] = {};
	attachments[1] = depthStencil.view;

	for (size_t i = 0; i < numOfSwapChainImages; i++) {
		VkImageViewCreateInfo viewCreateInfo = createImageViewCreateInfo();
		viewCreateInfo.image = swapChainImages[i];
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = swapChainFormat;
		viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		swapChainImageViews[i] = createImageView(viewCreateInfo);
		attachments[0] = swapChainImageViews[i];

		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = vkContext.renderPass;
		createInfo.attachmentCount = numOfSwapChainImages;
		createInfo.pAttachments = attachments;
		createInfo.width = swapChainExtent.width;
		createInfo.height = swapChainExtent.height;
		createInfo.layers = 1;

		checkResult(vkCreateFramebuffer(vkContext.device, &createInfo, nullptr, &swapChainFramebuffers[i]));

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		checkResult(vkCreateSemaphore(vkContext.device, &semaphoreCreateInfo, nullptr, &imageAquiredSemaphore[i]));

	}
}

static void destroyWindow() {
	vkDestroyImageView(vkContext.device, depthStencil.view, nullptr);
	vkDestroyImage(vkContext.device, depthStencil.image, nullptr);
	vkFreeMemory(vkContext.device, depthStencil.mem, nullptr);

	for (size_t i = 0; i < numOfSwapChainImages; i++) {
		vkDestroyFramebuffer(vkContext.device, swapChainFramebuffers[i], nullptr);
		vkDestroyImageView(vkContext.device, swapChainImageViews[i], nullptr);
		vkDestroySemaphore(vkContext.device, imageAquiredSemaphore[i], nullptr);
	}
	vkDestroySwapchainKHR(vkContext.device, swapChain, nullptr);


	vkDestroySurfaceKHR(instance, windowSurface, nullptr);

	if (ENABLE_DEBUGGING) {
		PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		DestroyDebugReportCallback(instance, callback, nullptr);
	}
	vkDestroyInstance(instance, nullptr);
}

int windowShouldClose() {
	return glfwWindowShouldClose(window);
}
void windowPollEvents() {
	glfwPollEvents();
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {

}

void createVulkanContext() {
	initWindow();
	createInstance();
	createDebugCallback();
	createWindowSurface();
	findPhysicalDevice();
	checkSwapChainSupport();
	findQueueFamilies();
	createLogicalDevice();
	createSwapChain();
	setupDepthStencil();

	//forward rendering pass
	createRenderPass();
	createFramebuffers();

	glfwSetCursorPosCallback(window, cursor_position_callback);
}

void destroyVkWindow() {
	for (uint32_t i = 0; i < numOfSwapChainImages; i++) {
		//vkDestroyImage(vkContext.device, swapChainImages[i], nullptr);
		vkDestroyImageView(vkContext.device, swapChainImageViews[i], nullptr);
		vkDestroyFramebuffer(vkContext.device, swapChainFramebuffers[i], nullptr);

		vkDestroyFramebuffer(vkContext.device, deferredFrameBuffers[i], nullptr);
		vkDestroySemaphore(vkContext.device, imageAquiredSemaphore[i], nullptr);
		vkDestroyFence(vkContext.device, commandBufferFences[i], nullptr);
	}
	vkDestroyImage(vkContext.device, depthStencil.image, nullptr);
	vkDestroyImageView(vkContext.device, depthStencil.view, nullptr);
	vkFreeMemory(vkContext.device, depthStencil.mem, nullptr);

	//vkDestroySwapchainKHR(vkContext.device, oldSwapChain, nullptr);
	vkDestroySwapchainKHR(vkContext.device, swapChain, nullptr);


	glfwDestroyWindow(window);
}
void initDeferredRendering() {
	createDeferredRenderPass(swapChainFormat, depthFormat);
	createDeferredFrameBuffer(depthStencil.view, swapChainImageViews, numOfSwapChainImages, deferredFrameBuffers);
}
void getMousePosition(float *xpos, float *ypos) {
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	*xpos = float(x);
	*ypos = float(y);
	
}
bool isMouseButtonDown() {
	return glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != 0;
}
bool isKeyDown(char c) {
	switch (c) {
	case 'w':
		return glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
		break;
	case 's':
		return glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
		break;
	case 'a':
		return glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
		break;
	case 'd':
		return glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
		break;
	}
	assert(false && "key is not supported");
	return false;
}
