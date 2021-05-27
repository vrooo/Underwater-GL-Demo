#include "Mesh.h"

#include "Renderer.h"

Plane::Plane(std::vector<PositionTexCoordVertex>& vert, std::vector<unsigned int>& ind, glm::vec3& col) :
	vertices{ vert },
	indices{ ind },
	color{ col },
	position{}, rotation{}, scale{1.0f}
{
	auto vertexTypeSize = sizeof(PositionTexCoordVertex);

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * vertexTypeSize, &vertices[0], GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)0);
	glVertexAttribIPointer(1, 2, GL_INT, vertexTypeSize, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

void Plane::Render()
{
	// TODO: actual model matrix and color
	glm::mat4 M{ 1.0f };
	Renderer::SetMat4("M", M);
	Renderer::SetVec3("color", color);

	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

void Plane::SetColor(glm::vec3& newCol)
{
	color = newCol;
}

void Plane::SetColor(float r, float g, float b)
{
	color = glm::vec3(r, g, b);
}

void Plane::SetColor(float newCol[3])
{
	color = glm::vec3(newCol[0], newCol[1], newCol[2]);
}

void Plane::Recreate(float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ)
{
	// TODO: dry the code
	unsigned int divX = vertX - 1, divZ = vertZ - 1;
	vertices.clear();
	indices.clear();

	float stepX = sizeX / divX, stepZ = sizeZ / divZ;
	float startX = -sizeX / 2.0f, startZ = -sizeZ / 2.0f;
	float y = 0.0f;

	unsigned int i = 0;
	for (unsigned int ix = 0; ix < divX + 1; ix++)
	{
		for (unsigned int iz = 0; iz < divZ + 1; iz++)
		{
			vertices.push_back(PositionTexCoordVertex{ glm::vec3{ startX + ix * stepX, y, startZ + iz * stepZ },
													glm::ivec2{ ix, iz } });
			if (ix < divX && iz < divZ)
			{
				unsigned int j = i + divZ + 2;

				indices.push_back(i);
				indices.push_back(i + 1);
				indices.push_back(j);

				indices.push_back(i);
				indices.push_back(j);
				indices.push_back(j - 1);
			}
			i++;
		}
	}

	auto vertexTypeSize = sizeof(PositionTexCoordVertex);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * vertexTypeSize, &vertices[0], GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_DYNAMIC_DRAW);
}

Plane Plane::MakeXZPlane(float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ, glm::vec3& col)
{
	unsigned int divX = vertX - 1, divZ = vertZ - 1;
	std::vector<PositionTexCoordVertex> vert{};
	std::vector<unsigned int> ind{};

	float stepX = sizeX / divX, stepZ = sizeZ / divZ;
	float startX = -sizeX / 2.0f, startZ = -sizeZ / 2.0f;
	float y = 0.0f;

	unsigned int i = 0;
	for (unsigned int ix = 0; ix < divX + 1; ix++)
	{
		for (unsigned int iz = 0; iz < divZ + 1; iz++)
		{
			vert.push_back(PositionTexCoordVertex{ glm::vec3{ startX + ix * stepX, y, startZ + iz * stepZ },
													glm::ivec2{ ix, iz } });
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

	return Plane{ vert, ind, col };
}