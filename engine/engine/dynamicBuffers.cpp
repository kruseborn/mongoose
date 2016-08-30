#include "vulkan\vulkan.h"
#include <cstdio>
#include "vkUtils.h"
#include <cstring>

struct DynamicBuffer {
	VkBuffer			buffer;
	uint32_t			current_offset;
	unsigned char *		data;
};
#define DYNAMIC_VERTEX_BUFFER_SIZE_KB	1024
#define DYNAMIC_UNIFORM_BUFFER_SIZE_KB	128
#define NUM_DYNAMIC_BUFFERS				2
#define MAX_UNIFORM_ALLOC				2048

static VkDeviceMemory	dyn_vertex_buffer_memory;
static VkDeviceMemory	dyn_uniform_buffer_memory;
static DynamicBuffer		dyn_vertex_buffers[NUM_DYNAMIC_BUFFERS];
static DynamicBuffer		dyn_uniform_buffers[NUM_DYNAMIC_BUFFERS];
static int				current_dyn_buffer_index = 0;
static VkDescriptorSet	ubo_descriptor_sets[NUM_DYNAMIC_BUFFERS];


void freeDynamicBuffers() {
	for (uint32_t i = 0; i < NUM_DYNAMIC_BUFFERS; i++) {
		vkDestroyBuffer(vkContext.device, dyn_vertex_buffers[i].buffer, nullptr);
		vkDestroyBuffer(vkContext.device, dyn_uniform_buffers[i].buffer, nullptr);
	}
	vkFreeMemory(vkContext.device, dyn_vertex_buffer_memory, nullptr);
	vkFreeMemory(vkContext.device, dyn_uniform_buffer_memory, nullptr);
}
static void initDynamicVertexBuffers() {
	for (int i = 0; i < NUM_DYNAMIC_BUFFERS; ++i) {
		createBuffer(dyn_vertex_buffers[i].buffer, DYNAMIC_VERTEX_BUFFER_SIZE_KB * 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(vkContext.device, dyn_vertex_buffers[0].buffer, &memory_requirements);

	const VkDeviceSize align_mod = memory_requirements.size % memory_requirements.alignment;
	const VkDeviceSize aligned_size = ((memory_requirements.size % memory_requirements.alignment) == 0)
		? memory_requirements.size
		: (memory_requirements.size + memory_requirements.alignment - align_mod);

	VkMemoryAllocateInfo memory_allocate_info;
	memset(&memory_allocate_info, 0, sizeof(memory_allocate_info));
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = NUM_DYNAMIC_BUFFERS * aligned_size;
	memory_allocate_info.memoryTypeIndex = memoryTypeFromProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	checkResult(vkAllocateMemory(vkContext.device, &memory_allocate_info, NULL, &dyn_vertex_buffer_memory));

	for (int i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
		checkResult(vkBindBufferMemory(vkContext.device, dyn_vertex_buffers[i].buffer, dyn_vertex_buffer_memory, i * aligned_size));

	unsigned char *data;
	checkResult(vkMapMemory(vkContext.device, dyn_vertex_buffer_memory, 0, NUM_DYNAMIC_BUFFERS * aligned_size, 0, (void**)&data));
	for (int i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
		dyn_vertex_buffers[i].data = data + (i * aligned_size);
}


static void initDynamicUniformBuffers() {
	VkBufferCreateInfo buffer_create_info;
	memset(&buffer_create_info, 0, sizeof(buffer_create_info));
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = DYNAMIC_UNIFORM_BUFFER_SIZE_KB * 1024;
	buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	for (int i = 0; i < NUM_DYNAMIC_BUFFERS; ++i) {
		dyn_uniform_buffers[i].current_offset = 0;
		checkResult(vkCreateBuffer(vkContext.device, &buffer_create_info, NULL, &dyn_uniform_buffers[i].buffer));
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(vkContext.device, dyn_uniform_buffers[0].buffer, &memory_requirements);

	const VkDeviceSize align_mod = memory_requirements.size % memory_requirements.alignment;
	const VkDeviceSize aligned_size = ((memory_requirements.size % memory_requirements.alignment) == 0)
		? memory_requirements.size
		: (memory_requirements.size + memory_requirements.alignment - align_mod);

	VkMemoryAllocateInfo memory_allocate_info;
	memset(&memory_allocate_info, 0, sizeof(memory_allocate_info));
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = NUM_DYNAMIC_BUFFERS * aligned_size;
	memory_allocate_info.memoryTypeIndex = memoryTypeFromProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	checkResult(vkAllocateMemory(vkContext.device, &memory_allocate_info, NULL, &dyn_uniform_buffer_memory));

	for (int i = 0; i < NUM_DYNAMIC_BUFFERS; ++i) {
		checkResult(vkBindBufferMemory(vkContext.device, dyn_uniform_buffers[i].buffer, dyn_uniform_buffer_memory, i * aligned_size));
	}

	unsigned char * data;
	checkResult(vkMapMemory(vkContext.device, dyn_uniform_buffer_memory, 0, NUM_DYNAMIC_BUFFERS * aligned_size, 0, (void**)&data));

	for (int i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
		dyn_uniform_buffers[i].data = data + (i * aligned_size);

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
	memset(&descriptor_set_allocate_info, 0, sizeof(descriptor_set_allocate_info));
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool = vkContext.descriptorPool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts = &vkContext.descriptorSetLayout.ubo;

	vkAllocateDescriptorSets(vkContext.device, &descriptor_set_allocate_info, &ubo_descriptor_sets[0]);
	vkAllocateDescriptorSets(vkContext.device, &descriptor_set_allocate_info, &ubo_descriptor_sets[1]);

	VkDescriptorBufferInfo buffer_info;
	memset(&buffer_info, 0, sizeof(buffer_info));
	buffer_info.offset = 0;
	buffer_info.range = MAX_UNIFORM_ALLOC;

	VkWriteDescriptorSet ubo_write;
	memset(&ubo_write, 0, sizeof(ubo_write));
	ubo_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ubo_write.dstBinding = 0;
	ubo_write.dstArrayElement = 0;
	ubo_write.descriptorCount = 1;
	ubo_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	ubo_write.pBufferInfo = &buffer_info;

	for (int i = 0; i < NUM_DYNAMIC_BUFFERS; ++i) {
		buffer_info.buffer = dyn_uniform_buffers[i].buffer;
		ubo_write.dstSet = ubo_descriptor_sets[i];
		vkUpdateDescriptorSets(vkContext.device, 1, &ubo_write, 0, NULL);
	}
}

uint8_t *allocateBuffer(int size, VkBuffer *buffer, VkDeviceSize *buffer_offset) {
	DynamicBuffer *dyn_vb = &dyn_vertex_buffers[current_dyn_buffer_index];
	if ((dyn_vb->current_offset + size) > (DYNAMIC_VERTEX_BUFFER_SIZE_KB * 1024))
		printf("Out of dynamic vertex buffer space, increase DYNAMIC_VERTEX_BUFFER_SIZE_KB");

	*buffer = dyn_vb->buffer;
	*buffer_offset = dyn_vb->current_offset;

	unsigned char *data = dyn_vb->data + dyn_vb->current_offset;
	dyn_vb->current_offset += size;

	return data;
}


uint8_t* allocateUniform(int size, VkBuffer *buffer, uint32_t *buffer_offset, VkDescriptorSet *descriptor_set) {
	if (size > MAX_UNIFORM_ALLOC)
		printf("Increase MAX_UNIFORM_ALLOC");

	const int align_mod = size % 256;
	const int aligned_size = ((size % 256) == 0) ? size : (size + 256 - align_mod);

	DynamicBuffer *dyn_ub = &dyn_uniform_buffers[current_dyn_buffer_index];

	if ((dyn_ub->current_offset + MAX_UNIFORM_ALLOC) > (DYNAMIC_UNIFORM_BUFFER_SIZE_KB * 1024))
		printf("Out of dynamic uniform buffer space, increase DYNAMIC_UNIFORM_BUFFER_SIZE_KB");

	*buffer = dyn_ub->buffer;
	*buffer_offset = dyn_ub->current_offset;

	unsigned char *data = dyn_ub->data + dyn_ub->current_offset;
	dyn_ub->current_offset += aligned_size;

	*descriptor_set = ubo_descriptor_sets[current_dyn_buffer_index];

	return data;
}

void createDynamicBuffers() {
	initDynamicVertexBuffers();
	initDynamicUniformBuffers();
}
void swapDynamicBuffers() {
	current_dyn_buffer_index = (current_dyn_buffer_index + 1) % NUM_DYNAMIC_BUFFERS;
	dyn_vertex_buffers[current_dyn_buffer_index].current_offset = 0;
	dyn_uniform_buffers[current_dyn_buffer_index].current_offset = 0;
}