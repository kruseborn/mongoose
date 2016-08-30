#pragma once
#include "vulkan\vulkan.h"
struct MeshManager;
struct Pipeline;

void renderFps();
void renderTextureBox(float x, float y, float w, float h, VkDescriptorSet texture, Pipeline *pipeline);
void renderMeshes(MeshManager &meshManager);


//text rendering
enum TextAlign { alignLeft, alignCenter, alignRight };
void renderText(char *text, float x, float y, TextAlign align);


