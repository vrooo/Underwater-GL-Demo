#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Mesh.h"
#include "Vertices.h"

class Plane : public Mesh<PositionTexCoordVertex>
{
public:
	Plane(std::vector<PositionTexCoordVertex>& vert, std::vector<unsigned int>& ind, glm::vec3& col);
	//void Render();

	void Recreate(float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ);
	static Plane MakeXZPlane(float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ, glm::vec3& col);
};