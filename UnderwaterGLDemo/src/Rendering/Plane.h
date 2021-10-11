#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Model.h"
#include "Vertices.h"

class Plane : public Model<PositionTexSurfaceVertex>
{
public:
	Plane(Material mat, std::vector<PositionTexSurfaceVertex>& vert, std::vector<unsigned int>& ind);
	void Recreate(float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ);
};

Plane MakeXZPlane(Material mat, float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ);