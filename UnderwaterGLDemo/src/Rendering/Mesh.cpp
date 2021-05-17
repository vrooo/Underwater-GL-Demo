#include "Mesh.h"

#include "Renderer.h"

Mesh::Mesh(std::vector<PositionTexCoordVertex>& vert, std::vector<unsigned int>& ind) :
	vertices{ vert },
	indices{ ind },
	position{}, rotation{}, scale{1.0f}
{
	auto vertexSize = sizeof(PositionTexCoordVertex);

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * vertexSize, &vertices[0], GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)0);
	glVertexAttribIPointer(1, 2, GL_INT, vertexSize, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

void Mesh::Render()
{
	// TODO: actual model matrix and color
	glm::mat4 M{ 1.0f };
	Renderer::SetMat4("M", M);
	Renderer::SetVec4("color", 0.2f, 0.3f, 0.3f, 1.0f);

	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

Mesh Mesh::MakeXZPlane(float sizeX, float sizeZ, unsigned int vertX, unsigned int vertZ)
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

	return Mesh{ vert, ind };
}