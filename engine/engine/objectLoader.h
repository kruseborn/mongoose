#pragma once

struct Meshes;
struct Materials;
struct VulkanContext;
struct Pipelines;
void loadObjfile(VulkanContext &context, Meshes &meshes, Materials &materials, Pipelines &pipelines, char* filename, const char* basepath = nullptr, unsigned int flags = 1);