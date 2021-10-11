#include <filesystem>
#include <fstream>
#include <map>

#include "Scene.h"

Scene::Scene(const char* objPath)
	: position{}, rotation{}, scale{ 1.0f }
{
	std::filesystem::path fsPath{ objPath };
	fsPath.replace_extension(".mtl");
	auto mtlPath = fsPath.c_str();

	std::map<std::string, Material> materialMap;

	std::ifstream mtlFile;
	mtlFile.exceptions(std::ifstream::badbit);
	mtlFile.open(mtlPath, std::ifstream::in);

	std::string line, command, curMatName;
	Material curMat;
	while (std::getline(mtlFile, line))
	{
		std::stringstream lineStream{ line };
		lineStream >> command;

		if (command == "newmtl")
		{
			if (!curMatName.empty())
			{
				materialMap.emplace(curMatName, curMat);
			}
			curMat = Material{};
			lineStream >> curMatName;
		}
		else if (command == "Ka")
		{
			lineStream >> curMat.ambientColor.x >> curMat.ambientColor.y >> curMat.ambientColor.z;
		}
		else if (command == "Kd")
		{
			lineStream >> curMat.diffuseColor.x >> curMat.diffuseColor.y >> curMat.diffuseColor.z;
		}
		else if (command == "Ks")
		{
			lineStream >> curMat.specularColor.x >> curMat.specularColor.y >> curMat.specularColor.z;
		}
		else if (command == "Ns")
		{
			lineStream >> curMat.specularHighlight;
		}
	}
	if (!curMatName.empty())
	{
		materialMap.emplace(curMatName, curMat);
	}
	mtlFile.close();

	std::ifstream objFile;
	objFile.exceptions(std::ifstream::badbit);
	objFile.open(objPath, std::ifstream::in);

	curMat = Material{};
	std::vector<glm::vec2> texCoords, curTexCoords;
	std::vector<glm::vec3> positions, normals, curPositions, curNormals;
	std::vector<unsigned int> curIndices;
	float minX = FLT_MAX, maxX = -FLT_MAX;
	float minY = FLT_MAX, maxY = -FLT_MAX;
	float minZ = FLT_MAX, maxZ = -FLT_MAX;
	int curIndex = 0, readIndex = 0;

	std::vector<PositionNormalTexVertex> allVertices;
	std::vector<unsigned int> allIndices;
	std::vector<ModelInfo> allModelInfo;
	unsigned int allIndicesCurShift = 0, curModelIndexOffset = 0;

	auto createModelIfExists = [&]()
	{
		if (!curIndices.empty())
		{
			std::vector<PositionNormalTexVertex> curVertices;
			for (int i = 0; i < curPositions.size(); i++)
			{
				PositionNormalTexVertex newVertex{ curPositions[i], curNormals[i], curTexCoords[i] };
				curVertices.push_back(newVertex);
				allVertices.push_back(newVertex);
			}
			auto curModel = std::make_unique<Model<PositionNormalTexVertex>>(curMat, curVertices, curIndices);
			models.push_back(std::move(curModel));
			unsigned int curIndicesSize = (unsigned int)curIndices.size();
			allModelInfo.push_back({ curModelIndexOffset, curIndicesSize, minX, maxX, minY, maxY, minZ, maxZ });

			curModelIndexOffset += curIndicesSize;
			allIndicesCurShift += curVertices.size();
			curPositions.clear();
			curNormals.clear();
			curTexCoords.clear();
			curIndices.clear();
			curIndex = 0;
			minX = minY = minZ = FLT_MAX;
			maxX = maxY = maxZ = -FLT_MAX;
		}
	};

	glm::vec2 curVec2{};
	glm::vec3 curVec3{};
	while (std::getline(objFile, line))
	{
		std::stringstream lineStream{ line };
		lineStream >> command;

		if (command == "v")
		{
			lineStream >> curVec3.x >> curVec3.y >> curVec3.z;
			positions.push_back(curVec3);
		}
		else if (command == "vn")
		{
			lineStream >> curVec3.x >> curVec3.y >> curVec3.z;
			normals.push_back(curVec3);
		}
		else if (command == "vt")
		{
			lineStream >> curVec2.x >> curVec2.y;
			texCoords.push_back(curVec2);
		}
		else if (command == "usemtl")
		{
			createModelIfExists();
			lineStream >> curMatName;
			curMat = materialMap.at(curMatName);
		}
		else if (command == "f")
		{
			for (int i = 0; i < 3; i++)
			{
				lineStream >> readIndex;
				glm::vec3 pos = positions[readIndex - 1];
				curPositions.push_back(pos);

				minX = std::min(minX, pos.x);
				minY = std::min(minY, pos.y);
				minZ = std::min(minZ, pos.z);

				maxX = std::max(maxX, pos.x);
				maxY = std::max(maxY, pos.y);
				maxZ = std::max(maxZ, pos.z);

				lineStream.ignore(1);
				if (lineStream.peek() != '/')
				{
					lineStream >> readIndex;
					curTexCoords.push_back(texCoords[readIndex - 1]);
				}
				else
				{
					curTexCoords.push_back(glm::vec2{ 0 });
				}
				lineStream.ignore(1);
				lineStream >> readIndex;
				curNormals.push_back(normals[readIndex - 1]);

				unsigned int newIndex = curIndex++;
				curIndices.push_back(newIndex);
				allIndices.push_back(newIndex + allIndicesCurShift);
			}
		}
	}
	createModelIfExists();
	objFile.close();

	glGenBuffers(1, &ssboVertices);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVertices);
	glBufferData(GL_SHADER_STORAGE_BUFFER, allVertices.size() * sizeof(PositionNormalTexVertex), &allVertices[0], GL_DYNAMIC_COPY);

	glGenBuffers(1, &ssboIndices);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboIndices);
	glBufferData(GL_SHADER_STORAGE_BUFFER, allIndices.size() * sizeof(unsigned int), &allIndices[0], GL_DYNAMIC_COPY);

	glGenBuffers(1, &ssboModelInfo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboModelInfo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, allModelInfo.size() * sizeof(ModelInfo), &allModelInfo[0], GL_DYNAMIC_COPY);
}

void Scene::SetPosition(float newPos[3])
{
	position = glm::vec3(newPos[0], newPos[1], newPos[2]);
}

void Scene::SetScale(float newScale)
{
	scale = newScale;
}

void Scene::Render()
{
	EnableSceneModelMatrix();
	for (int i = 0; i < models.size(); i++)
		models[i]->Render(true);
}

void Scene::EnableSceneModelMatrix()
{
	// TODO: rotation
	glm::mat4 M{ 1.0f };
	M = glm::translate(M, position);
	M = glm::scale(M, glm::vec3{ scale });

	Renderer::SetMat4("M", M);
}

void Scene::BindSSBOs(int bindingVertex, int bindingIndex, int bindingModelInfo)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingVertex, ssboVertices);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, ssboIndices);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingModelInfo, ssboModelInfo);
}