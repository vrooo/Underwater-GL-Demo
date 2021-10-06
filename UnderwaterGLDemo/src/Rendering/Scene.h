#pragma once

#include <vector>

#include "Model.h"

class Scene
{
private:
	GLuint ssboVertices, ssboIndices, ssboData;
public:
	std::vector<std::unique_ptr<Model<PositionNormalTexVertex>>> models;

	Scene(std::vector<std::unique_ptr<Model<PositionNormalTexVertex>>> models);

	void SetPosition(float newPos[3]);
	void SetScale(float newScale);
	void Render();
};

Scene CreateSceneFromObj(const char* objPath);