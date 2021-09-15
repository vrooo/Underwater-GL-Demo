#include "Plane.h"

#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>

Plane::Plane(Material mat, std::vector<PositionTexVertex>& vert, std::vector<unsigned int>& ind) :
	Model(mat, vert, ind) { }

void Plane::Recreate(float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ)
{
	// TODO: dry the code
	unsigned int divX = vertX - 1, divZ = vertZ - 1;
	std::vector<PositionTexVertex> vert{};
	std::vector<unsigned int> ind{};

	float stepX = sizeX / divX, stepZ = sizeZ / divZ;
	float startX = -sizeX / 2.0f, startZ = -sizeZ / 2.0f;
	float y = 0.0f;

	unsigned int i = 0;
	for (unsigned int ix = 0; ix < divX + 1; ix++)
	{
		for (unsigned int iz = 0; iz < divZ + 1; iz++)
		{
			vert.push_back(PositionTexVertex{ glm::vec3{ startX + ix * stepX, y, startZ + iz * stepZ },
											  glm::vec2{ (float)ix / divX, (float)iz / divZ } });
			if (ix < divX && iz < divZ)
			{
				unsigned int j = i + divZ + 2;

				ind.push_back(i);
				ind.push_back(i + 1);
				ind.push_back(j);

				ind.push_back(i);
				ind.push_back(j);
				ind.push_back(j - 1);
			}
			i++;
		}
	}

	mesh.ReplaceData(vert, ind);
}

Plane MakeXZPlane(Material mat, float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ)
{
	unsigned int divX = vertX - 1, divZ = vertZ - 1;
	std::vector<PositionTexVertex> vert{};
	std::vector<unsigned int> ind{};

	float stepX = sizeX / divX, stepZ = sizeZ / divZ;
	float startX = -sizeX / 2.0f, startZ = -sizeZ / 2.0f;
	float y = 0.0f;

	unsigned int i = 0;
	for (unsigned int ix = 0; ix < divX + 1; ix++)
	{
		for (unsigned int iz = 0; iz < divZ + 1; iz++)
		{
			vert.push_back(PositionTexVertex{ glm::vec3{ startX + ix * stepX, y, startZ + iz * stepZ },
											  glm::vec2{ (float)ix / divX, (float)iz / divZ } });
			if (ix < divX && iz < divZ)
			{
				unsigned int j = i + divZ + 2;

				ind.push_back(i);
				ind.push_back(i + 1);
				ind.push_back(j);

				ind.push_back(i);
				ind.push_back(j);
				ind.push_back(j - 1);
			}
			i++;
		}
	}

	return Plane{ mat, vert, ind };
}