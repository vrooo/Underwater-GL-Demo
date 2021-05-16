#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

struct PositionTexCoordVertex
{
	glm::vec3 position;
	glm::ivec2 texCoord;
};