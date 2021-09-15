#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

struct PositionTexVertex
{
	glm::vec3 position;
	glm::vec2 texCoord;
	static void SetVertexAttributes()
	{
		auto vertexTypeSize = sizeof(PositionTexVertex);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
	}
};

struct PositionNormalVertex
{
	glm::vec3 position;
	glm::vec3 normal;
	static void SetVertexAttributes()
	{
		auto vertexTypeSize = sizeof(PositionNormalVertex);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
	}
};

struct PositionNormalTexVertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	static void SetVertexAttributes()
	{
		auto vertexTypeSize = sizeof(PositionNormalTexVertex);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(3 * sizeof(float)));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
	}
};