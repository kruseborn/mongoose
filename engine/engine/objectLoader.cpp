#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc

#include "tinyobjloader\tiny_obj_loader.h"
#include <iostream>
#include "mesh.h"
#include "meshes.h"
#include "materials.h"
#include "vulkanContext.h"
#include "glm\glm.hpp"
#include "material.h"
#include "pipelines.h"

std::vector<tinyobj::shape_t> shapes;
std::vector<tinyobj::material_t> tinyMaterials;
void loadObjfile(VulkanContext &context, Meshes &meshes, Materials &materials, Pipelines &pipelines, char* filename, const char* basepath, unsigned int flags) {
	std::string err;
	bool ret = tinyobj::LoadObj(shapes, tinyMaterials, err, filename, basepath, 1);

	if (!err.empty()) { // `err` may contain warning message.
		std::cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	std::cout << "# of shapes    : " << shapes.size() << std::endl;
	std::cout << "# of tinyMaterials : " << tinyMaterials.size() << std::endl;

	for (uint32_t i = 0; i < tinyMaterials.size(); i++) {
		auto tinyMaterial = tinyMaterials[i];
		MaterialProperties properties;
		properties.ambient = glm::vec4(tinyMaterial.ambient[0], tinyMaterial.ambient[1], tinyMaterial.ambient[2], 1.0f);
		properties.diffuse = glm::vec4(tinyMaterial.diffuse[0], tinyMaterial.diffuse[1], tinyMaterial.diffuse[2], 1.0f);
		properties.specular = glm::vec4(tinyMaterial.specular[0], tinyMaterial.specular[1], tinyMaterial.specular[2], 1.0f);
		properties.opacity = tinyMaterial.dissolve;
		Material material(context, tinyMaterial.name, properties, pipelines.get("solid"), 0);
		materials.insert(i, std::move(material));
	}
	for (uint32_t i = 0; i < shapes.size(); i++) {
		auto data = shapes[i].mesh.positions.data();
		uint32_t vertexSize = (uint32_t)shapes[i].mesh.positions.size() * sizeof(shapes[i].mesh.positions[0]);
		auto indexData = shapes[i].mesh.indices.data();
		uint32_t indexSize = (uint32_t)shapes[i].mesh.indices.size() * sizeof(shapes[i].mesh.indices[0]);

		auto material = materials.get(i);

		meshes.insert({context, data, vertexSize, indexData, indexSize, uint32_t(shapes[i].mesh.indices.size()), material });
		/*
		printf("shape[%ld].name = %s\n", i, shapes[i].name.c_str());
		printf("Size of shape[%ld].indices: %ld\n", i, shapes[i].mesh.indices.size());
		printf("Size of shape[%ld].material_ids: %ld\n", i, shapes[i].mesh.material_ids.size());
		*/
		assert((shapes[i].mesh.indices.size() % 3) == 0);
		printf("\n");
	}
}