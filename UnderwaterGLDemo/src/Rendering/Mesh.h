#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Vertices.h"

class Plane // TODO: Mesh<T> as base class
{
private:
	GLuint vbo, ebo, vao;
	std::vector<PositionTexCoordVertex> vertices;
	std::vector<unsigned int> indices;

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	glm::vec3 color;
public:
	Plane(std::vector<PositionTexCoordVertex>& vert, std::vector<unsigned int>& ind, glm::vec3& col);
	void Render();
	void SetColor(glm::vec3& newCol);
	void SetColor(float r, float g, float b);
	void SetColor(float newCol[3]);

	void Recreate(float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ);
	static Plane MakeXZPlane(float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ, glm::vec3& col);
};