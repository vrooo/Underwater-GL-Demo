#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Model.h"
#include "Vertices.h"

struct ChunkInfo
{
	unsigned int vertexOffset, vertexCount;
	unsigned int indexOffset, indexCount;
	float minX, maxX;
	float minY, maxY;
	float minZ, maxZ;
};

class Plane : public Model<PositionTexSurfaceVertex>
{
private:
	GLuint ssboChunkInfo;
public:
	Plane(Material mat, std::vector<PositionTexSurfaceVertex>& vert, std::vector<unsigned int>& ind,
		  std::vector<ChunkInfo>& chunkInfo);
	void Recreate(unsigned int vertexCount, unsigned int chunkCount, float size = 1.0f);
	using Model::BindSSBOs;
	void BindSSBOs(int bindingVertex, int bindingIndex, int bindingChunkInfo);
	void BindChunkInfoSSBO(int bindingChunkInfo);
};

Plane MakeXZPlane(Material mat, unsigned int vertexCount, unsigned int chunkCount, float size = 1.0f);