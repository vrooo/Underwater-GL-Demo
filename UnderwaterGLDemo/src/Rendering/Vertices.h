#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

struct PositionTexSurfaceVertex
{
	glm::vec2 position;
	glm::vec2 texCoord;
	static inline void SetVertexAttributes()
	{
		auto vertexTypeSize = sizeof(PositionTexSurfaceVertex);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
	}
};

struct PositionNormalVertex
{
	glm::vec3 position;
	glm::vec3 normal;
	static inline void SetVertexAttributes()
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
	// weird order to allow use in SSBOs
	glm::vec3 position;
	float texCoordU;
	glm::vec3 normal;
	float texCoordV;
	static inline void SetVertexAttributes()
	{
		auto vertexTypeSize = sizeof(PositionNormalTexVertex);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)0);
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(3 * sizeof(float)));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(4 * sizeof(float)));
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(7 * sizeof(float)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
	}

	inline PositionNormalTexVertex(glm::vec3 position, glm::vec3 normal, glm::vec2 texCoord)
		: position(position), texCoordU(texCoord.x), normal(normal), texCoordV(texCoord.y)
	{}
};