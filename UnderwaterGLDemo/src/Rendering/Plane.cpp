#include "Plane.h"

#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>

void CalculateXZPlane(unsigned int vertexCount, unsigned int chunkCount, float size,
					  std::vector<PositionTexSurfaceVertex>& vert, std::vector<unsigned int>& ind,
					  std::vector<ChunkInfo>& chunkInfo);

Plane::Plane(Material mat, std::vector<PositionTexSurfaceVertex>& vert, std::vector<unsigned int>& ind,
			 std::vector<ChunkInfo>& chunkInfo) :
	Model(mat, vert, ind)
{
	glGenBuffers(1, &ssboChunkInfo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboChunkInfo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, chunkInfo.size() * sizeof(ChunkInfo), &chunkInfo[0], GL_DYNAMIC_COPY);
}

void Plane::Recreate(unsigned int vertexCount, unsigned int chunkCount, float size)
{
	std::vector<PositionTexSurfaceVertex> vert{};
	std::vector<unsigned int> ind{};
	std::vector<ChunkInfo> chunkInfo{};
	CalculateXZPlane(vertexCount, chunkCount, size, vert, ind, chunkInfo);

	mesh.ReplaceData(vert, ind);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboChunkInfo);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, chunkInfo.size() * sizeof(ChunkInfo), &chunkInfo[0]);
}

Plane MakeXZPlane(Material mat, unsigned int vertexCount, unsigned int chunkCount, float size)
{
	std::vector<PositionTexSurfaceVertex> vert{};
	std::vector<unsigned int> ind{};
	std::vector<ChunkInfo> chunkInfo{};
	CalculateXZPlane(vertexCount, chunkCount, size, vert, ind, chunkInfo);

	return Plane{ mat, vert, ind, chunkInfo };
}

void Plane::BindSSBOs(int bindingVertex, int bindingIndex, int bindingModelInfo)
{
	BindSSBOs(bindingVertex, bindingIndex);
	BindChunkInfoSSBO(bindingModelInfo);
}

void Plane::BindChunkInfoSSBO(int bindingModelInfo)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingModelInfo, ssboChunkInfo);
}

void CalculateXZPlane(unsigned int vertexCount, unsigned int chunkCount, float size,
					  std::vector<PositionTexSurfaceVertex>& vert, std::vector<unsigned int>& ind,
					  std::vector<ChunkInfo>& chunkInfo)
{
	unsigned int divs = vertexCount - 1;
	unsigned int verticesPerChunk = vertexCount / chunkCount;
	unsigned int totalVerticesPerChunk = verticesPerChunk * verticesPerChunk;
	float step = size / divs;
	float start = -size / 2.0f;

	auto getIndex = [chunkCount, verticesPerChunk](unsigned int cx, unsigned int cz, unsigned int iix, unsigned int iiz)
	{
		if (iix >= verticesPerChunk)
		{
			iix -= verticesPerChunk;
			cx++;
		}
		if (iiz >= verticesPerChunk)
		{
			iiz -= verticesPerChunk;
			cz++;
		}
		return iiz + (iix + (cz + cx * chunkCount) * verticesPerChunk) * verticesPerChunk;
	};

	unsigned int i = 0;
	unsigned int indexOffset = 0;
	for (unsigned int cx = 0; cx < chunkCount; cx++)
	{
		for (unsigned int cz = 0; cz < chunkCount; cz++)
		{
			for (unsigned int iix = 0; iix < verticesPerChunk; iix++)
			{
				unsigned int ix = cx * verticesPerChunk + iix;
				for (unsigned int iiz = 0; iiz < verticesPerChunk; iiz++)
				{
					unsigned int iz = cz * verticesPerChunk + iiz;
					vert.push_back(PositionTexSurfaceVertex{ glm::vec2{ start + ix * step, start + iz * step },
															 glm::vec2{ (float)ix / divs, (float)iz / divs } });
					if (ix < divs && iz < divs)
					{
						unsigned int j = getIndex(cx, cz, iix + 1, iiz + 1);

						ind.push_back(i);
						ind.push_back(j);
						ind.push_back(getIndex(cx, cz, iix + 1, iiz));

						ind.push_back(i);
						ind.push_back(getIndex(cx, cz, iix, iiz + 1));
						ind.push_back(j);
					}
					i++;
				}
			}
			unsigned int indexSize = (unsigned int)ind.size();
			// TODO: sometimes we might miss a hit because bounding boxes won't include heights at borders of chunks (cx + 1, cz) and (cx, cz + 1)
			chunkInfo.push_back({(unsigned int)vert.size() - totalVerticesPerChunk, totalVerticesPerChunk,
								indexOffset, indexSize - indexOffset,
								start + cx * verticesPerChunk * step, start + ((cx + 1) * verticesPerChunk - (cx == chunkCount - 1 ? 1 : 0)) * step,
								0.0f, 0.0f,
								start + cz * verticesPerChunk * step, start + ((cz + 1) * verticesPerChunk - (cz == chunkCount - 1 ? 1 : 0)) * step });
			indexOffset = indexSize;
		}
	}
}