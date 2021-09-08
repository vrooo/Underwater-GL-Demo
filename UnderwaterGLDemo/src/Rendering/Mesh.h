#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Vertices.h"
#include "Renderer.h"

template <typename VertexType>
class Mesh // TODO: Mesh<T> as base class
{
private:
	GLuint vbo, ebo, vao;
protected:
	std::vector<VertexType> vertices;
	std::vector<unsigned int> indices;

	glm::vec3 position;
	glm::vec3 rotation;
	float scale;
	glm::vec3 color;

	void UpdateBuffers();
public:
	Mesh(std::vector<VertexType>& vert, std::vector<unsigned int>& ind, glm::vec3& col);
	void Render();
	void SetColor(glm::vec3& newCol);
	void SetColor(float r, float g, float b);
	void SetColor(float newCol[3]);
	void SetScale(float newScale);
};

template<typename VertexType>
inline Mesh<VertexType>::Mesh(std::vector<VertexType>& vert, std::vector<unsigned int>& ind, glm::vec3& col) :
	vertices{ vert },
	indices{ ind },
	color{ col },
	position{}, rotation{}, scale{ 1.0f }
{
	auto vertexTypeSize = sizeof(VertexType);

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()* vertexTypeSize, &vertices[0], GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_DYNAMIC_DRAW);

	VertexType::SetVertexAttributes();
}

template<typename VertexType>
inline void Mesh<VertexType>::UpdateBuffers()
{
	auto vertexTypeSize = sizeof(VertexType);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * vertexTypeSize, &vertices[0], GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_DYNAMIC_DRAW);
}

template<typename VertexType>
inline void Mesh<VertexType>::Render()
{
	// TODO: actual model matrix and color
	glm::mat4 M{ 1.0f };
	M = glm::scale(M, glm::vec3{ scale });

	Renderer::SetMat4("M", M);
	Renderer::SetVec3("color", color);

	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

template<typename VertexType>
inline void Mesh<VertexType>::SetColor(glm::vec3& newCol)
{
	color = newCol;
}

template<typename VertexType>
inline void Mesh<VertexType>::SetColor(float r, float g, float b)
{
	color = glm::vec3(r, g, b);
}

template<typename VertexType>
inline void Mesh<VertexType>::SetColor(float newCol[3])
{
	color = glm::vec3(newCol[0], newCol[1], newCol[2]);
}

template<typename VertexType>
inline void Mesh<VertexType>::SetScale(float newScale)
{
	scale = newScale;
}
