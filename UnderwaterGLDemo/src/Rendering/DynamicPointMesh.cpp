#include "DynamicPointMesh.h"

#include "Renderer.h"

DynamicPointMesh::DynamicPointMesh(unsigned int pointCount, float pointSize, glm::vec4 color)
	: pointCount(pointCount), pointSize(pointSize), color(color)
{
	auto vertexTypeSize = sizeof(glm::vec4);

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	std::vector<glm::vec4> defaultVertices{ pointCount, glm::vec4{0.0f, 0.0f, 0.0f, 1.0f} };
	glBufferData(GL_ARRAY_BUFFER, pointCount * vertexTypeSize, &defaultVertices[0], GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)0);
	glEnableVertexAttribArray(0);
}

void DynamicPointMesh::Render(bool showOnTop)
{
	if (showOnTop)
		glDisable(GL_DEPTH_TEST);
	glPointSize(pointSize);
	Renderer::SetVec4("color", color);

	glBindVertexArray(vao);
	glDrawArrays(GL_POINTS, 0, pointCount);

	if (showOnTop)
		glEnable(GL_DEPTH_TEST);
	glPointSize(1.0f);
}

void DynamicPointMesh::BindSSBO(int bindingVertex)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingVertex, vbo);
}