#include <filesystem>
#include <fstream>
#include <map>

#include "Scene.h"

Scene::Scene(std::vector<std::unique_ptr<Model<PositionNormalTexVertex>>> models)
	: models(std::move(models))
{
	for (int i = 0; i < models.size(); i++)
	{

	}

	glGenBuffers(1, &ssboVertices);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVertices);
	//glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(shader_data), &shader_data, GL_DYNAMIC_COPY);
}

void Scene::SetPosition(float newPos[3])
{
	for (int i = 0; i < models.size(); i++)
		models[i]->SetPosition(newPos);
}

void Scene::SetScale(float newScale)
{
	for (int i = 0; i < models.size(); i++)
		models[i]->SetScale(newScale);
}

void Scene::Render()
{
	for (int i = 0; i < models.size(); i++)
		models[i]->Render();
}


Scene CreateSceneFromObj(const char* objPath)
{
	std::filesystem::path fsPath{ objPath };
	fsPath.replace_extension(".mtl");
	auto mtlPath = fsPath.c_str();

	std::map<std::string, Material> materialMap;
	std::vector<std::unique_ptr<Model<PositionNormalTexVertex>>> models;

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
	int curIndex = 0, readIndex = 0;

	auto createModelIfExists = [&]()
	{
		if (!curIndices.empty())
		{
			std::vector<PositionNormalTexVertex> curVertices;
			for (int i = 0; i < curPositions.size(); i++)
			{
				curVertices.push_back({ curPositions[i], curNormals[i], curTexCoords[i] });
			}
			auto curModel = std::make_unique<Model<PositionNormalTexVertex>>(curMat, curVertices, curIndices);
			models.push_back(std::move(curModel));

			curPositions.clear();
			curNormals.clear();
			curTexCoords.clear();
			curIndices.clear();
			curIndex = 0;
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
				curPositions.push_back(positions[readIndex - 1]);
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
				curIndices.push_back(curIndex++);
			}
		}
	}
	createModelIfExists();
	objFile.close();
	return { std::move(models) };
}