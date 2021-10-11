#pragma once

#include "Material.h"
#include "Mesh.h"

template <typename VertexType>
class Model
{
protected:
	Material material;
	Mesh<VertexType> mesh;

public:
	Model(Material mat, std::vector<VertexType>& vert, std::vector<unsigned int>& ind);
	void Render(bool ignoreModelMatrix = false, const char* modelMatrixName = "M");
	void RenderInstanced(int instanceCount, bool ignoreModelMatrix = false, const char* modelMatrixName = "M");
	void BindSSBOs(int bindingVertex, int bindingIndex);
	void EnableModelMatrix(const char* modelMatrixName = "M");

	void SetColor(float newCol[3]);
	void SetColor(glm::vec3& newCol);
	void SetColor(float r, float g, float b);

	void SetPosition(float newPos[3]);
	void SetPosition(glm::vec3& newPos);
	void SetPosition(float x, float y, float z);

	void SetScale(float newScale);

	unsigned int GetVertexCount();
};


template<typename VertexType>
inline Model<VertexType>::Model(Material mat, std::vector<VertexType>& vert, std::vector<unsigned int>& ind)
	: material(mat), mesh(vert, ind)
{}

template<typename VertexType>
inline void Model<VertexType>::Render(bool ignoreModelMatrix, const char* modelMatrixName)
{
	material.Set();
	mesh.Render(ignoreModelMatrix, modelMatrixName);
}

template<typename VertexType>
inline void Model<VertexType>::RenderInstanced(int instanceCount, bool ignoreModelMatrix, const char* modelMatrixName)
{
	material.Set();
	mesh.RenderInstanced(instanceCount, ignoreModelMatrix, modelMatrixName);
}

template<typename VertexType>
inline void Model<VertexType>::BindSSBOs(int bindingVertex, int bindingIndex)
{
	mesh.BindSSBOs(bindingVertex, bindingIndex);
}

template<typename VertexType>
inline void Model<VertexType>::EnableModelMatrix(const char* modelMatrixName)
{
	mesh.EnableModelMatrix(modelMatrixName);
}

template<typename VertexType>
inline void Model<VertexType>::SetColor(float newCol[3])
{
	material.ambientColor = glm::vec3(newCol[0], newCol[1], newCol[2]);
}
template<typename VertexType>
inline void Model<VertexType>::SetColor(glm::vec3& newCol)
{
	material.ambientColor = newCol;
}
template<typename VertexType>
inline void Model<VertexType>::SetColor(float r, float g, float b)
{
	material.ambientColor = glm::vec3(r, g, b);
}

template<typename VertexType>
inline void Model<VertexType>::SetPosition(float newPos[3])
{
	mesh.SetPosition(glm::vec3(newPos[0], newPos[1], newPos[2]));
}
template<typename VertexType>
inline void Model<VertexType>::SetPosition(glm::vec3& newPos)
{
	mesh.SetPosition(newPos);
}
template<typename VertexType>
inline void Model<VertexType>::SetPosition(float x, float y, float z)
{
	mesh.SetPosition(glm::vec3(x, y, z));
}

template<typename VertexType>
inline void Model<VertexType>::SetScale(float newScale)
{
	mesh.SetScale(newScale);
}

template<typename VertexType>
inline unsigned int Model<VertexType>::GetVertexCount()
{
	return mesh.GetVertexCount();
}