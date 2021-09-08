#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

struct PositionTexCoordVertex
{
	glm::vec3 position;
	glm::vec2 texCoord;
	static void SetVertexAttributes()
	{
		auto vertexTypeSize = sizeof(PositionTexCoordVertex);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
	}
};