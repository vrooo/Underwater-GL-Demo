#pragma once

#include <vector>

#include "Model.h"

struct ModelInfo
{
	unsigned int indexOffset, indexCount;
	// TODO: material data
};

class Scene
{
private:
	GLuint ssboVertices, ssboIndices, ssboModelInfo;
public:
	std::vector<std::unique_ptr<Model<PositionNormalTexVertex>>> models;
	glm::vec3 position;
	glm::vec3 rotation;
	float scale;

	Scene(const char* objPath);

	void SetPosition(float newPos[3]);
	void SetScale(float newScale);
	void Render();
	void EnableSceneModelMatrix();
	void BindSSBOs(int bindingVertex, int bindingIndex, int bindingModelInfo);
};