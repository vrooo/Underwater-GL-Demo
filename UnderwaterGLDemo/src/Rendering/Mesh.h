#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Vertices.h"
#include "Renderer.h"

template <typename VertexType>
class Mesh
{
private:
	GLuint vbo, ebo, vao;
protected:
	std::vector<VertexType> vertices;
	std::vector<unsigned int> indices;

	glm::vec3 position;
	glm::vec3 rotation;
	float scale;

	GLenum primitiveMode;

	void CreateBuffers();
	void PrepareRender();
public:
	Mesh(std::vector<VertexType>& vert, std::vector<unsigned int>& ind, GLenum primitive = GL_TRIANGLES);
	void ReplaceData(std::vector<VertexType>& vert, std::vector<unsigned int>& ind);
	void Render();
	void RenderInstanced(int instanceCount);
	void BindToSSBO(int binding);
	void EnableModelMatrix();

	void SetScale(float newScale);
	void SetPosition(glm::vec3 newPos);

	unsigned int GetVertexCount();
};

template<typename VertexType>
inline Mesh<VertexType>::Mesh(std::vector<VertexType>& vert, std::vector<unsigned int>& ind, GLenum primitive) :
	vertices{ vert },
	indices{ ind },
	position{}, rotation{}, scale{ 1.0f },
	primitiveMode{ primitive }
{
	auto vertexTypeSize = sizeof(VertexType);

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()* vertexTypeSize, &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_STATIC_DRAW);

	VertexType::SetVertexAttributes();
}

template<typename VertexType>
inline void Mesh<VertexType>::CreateBuffers()
{
	auto vertexTypeSize = sizeof(VertexType);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * vertexTypeSize, &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_STATIC_DRAW);
}

template<typename VertexType>
inline void Mesh<VertexType>::ReplaceData(std::vector<VertexType>& vert, std::vector<unsigned int>& ind)
{
	vertices = vert;
	indices = ind;
	CreateBuffers();
}

template<typename VertexType>
inline void Mesh<VertexType>::PrepareRender()
{
	EnableModelMatrix();
	glBindVertexArray(vao);
}

template<typename VertexType>
inline void Mesh<VertexType>::Render()
{
	PrepareRender();
	glDrawElements(primitiveMode, indices.size(), GL_UNSIGNED_INT, 0);
}
template<typename VertexType>
inline void Mesh<VertexType>::RenderInstanced(int instanceCount)
{
	PrepareRender();
	glDrawElementsInstanced(primitiveMode, indices.size(), GL_UNSIGNED_INT, 0, instanceCount);
}

template<typename VertexType>
inline void Mesh<VertexType>::BindToSSBO(int binding)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, vbo);
}

template<typename VertexType>
inline void Mesh<VertexType>::EnableModelMatrix()
{
	// TODO: rotation
	glm::mat4 M{ 1.0f };
	M = glm::translate(M, position);
	M = glm::scale(M, glm::vec3{ scale });

	Renderer::SetMat4("M", M);
}

template<typename VertexType>
inline void Mesh<VertexType>::SetScale(float newScale)
{
	scale = newScale;
}
template<typename VertexType>
inline void Mesh<VertexType>::SetPosition(glm::vec3 newPos)
{
	position = newPos;
}

template<typename VertexType>
inline unsigned int Mesh<VertexType>::GetVertexCount()
{
	return vertices.size();
}