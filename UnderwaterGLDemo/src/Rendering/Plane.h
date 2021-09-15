#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Model.h"
#include "Vertices.h"

class Plane : public Model<PositionTexVertex>
{
public:
	Plane(Material mat, std::vector<PositionTexVertex>& vert, std::vector<unsigned int>& ind);
	//void Render();

	void Recreate(float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ);
};

Plane MakeXZPlane(Material mat, float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ);