#include <filesystem>
#include <fstream>
#include <map>

#include "Model.h"

std::vector<std::unique_ptr<BaseModel>> CreateSceneFromObj(const char* objPath)
{
	std::filesystem::path fsPath{ objPath };
	fsPath.replace_extension(".mtl");
	auto mtlPath = fsPath.c_str();

	std::map<std::string, Material> materialMap;
	std::vector<std::unique_ptr<BaseModel>> scene;

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
			if (curTexCoords.empty())
			{
				std::vector<PositionNormalVertex> curVertices;
				for (int i = 0; i < curPositions.size(); i++)
				{
					curVertices.push_back({ curPositions[i], curNormals[i] });
				}
				auto curModel = std::make_unique<Model<PositionNormalVertex>>(curMat, curVertices, curIndices);
				scene.push_back(std::move(curModel));
			}
			else
			{
				std::vector<PositionNormalTexVertex> curVertices;
				for (int i = 0; i < curPositions.size(); i++)
				{
					curVertices.push_back({ curPositions[i], curNormals[i], curTexCoords[i] });
				}
				auto curModel = std::make_unique<Model<PositionNormalTexVertex>>(curMat, curVertices, curIndices);
				scene.push_back(std::move(curModel));
			}

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
				lineStream.ignore(1);
				lineStream >> readIndex;
				curNormals.push_back(normals[readIndex - 1]);
				curIndices.push_back(curIndex++);
			}
		}
	}
	createModelIfExists();
	objFile.close();
	return scene;
}