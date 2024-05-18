#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

class DynamicPointMesh
{
private:
	GLuint vbo, vao;
	unsigned int pointCount;
	float pointSize;
	glm::vec4 color;
public:
	DynamicPointMesh(unsigned int pointCount, float pointSize, glm::vec4 color);
	void Render(bool showOnTop = false);
	void BindVertexSSBO(int bindingVertex);
};