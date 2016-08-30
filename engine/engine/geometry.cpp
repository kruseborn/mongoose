#pragma
#include "geometry.h"

namespace geometry {
	void fullscreenCube(PositionVertex *vertices, uint32_t *indices) {
		vertices[0].position.x = -1.0f;
		vertices[0].position.y = -1.0f;
		vertices[0].position.z = 0.0f;
		vertices[0].position.w = 0.0f;

		vertices[1].position.x = 1.0f;
		vertices[1].position.y = -1.0f;
		vertices[1].position.z = 1.0f;
		vertices[1].position.w = 0.0f;

		vertices[2].position.x = 1.0f;
		vertices[2].position.y = 1.0f;
		vertices[2].position.z = 1.0f;
		vertices[2].position.w = 1.0f;

		vertices[3].position.x = -1.0f;
		vertices[3].position.y = 1.0f;
		vertices[3].position.z = 0.0f;
		vertices[3].position.w = 1.0f;

		indices[0] = 0; indices[1] = 1; indices[2] = 3;
		indices[3] = 1; indices[4] = 2; indices[5] = 3;

	}
}