#include "Mesh.h"

Mesh::Mesh() : vertices {-0.5f, -0.5f, 0.0f,
						  0.5f, -0.5f, 0.0f,
						  0.0f,  0.5f, 0.0f}
{
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);
}

void Mesh::Render()
{
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}