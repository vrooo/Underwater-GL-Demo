#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Vertices.h"

class Mesh // TODO: template
{
private:
	GLuint vbo, ebo, vao;
	std::vector<PositionTexCoordVertex> vertices;
	std::vector<unsigned int> indices;

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

public:
	Mesh(std::vector<PositionTexCoordVertex>& vert, std::vector<unsigned int>& ind);
	void Render();

	static Mesh MakeXZPlane(float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ);
};