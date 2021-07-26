#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader
{
public:
	unsigned int ID;

	static Shader CreateShaderVF(const char* vertPath, const char* fragPath);
	static Shader CreateShaderVGF(const char* vertPath, const char* geomPath, const char* fragPath);
	static Shader CreateShaderCompute(const char* compPath);

	void Use();
	void SetInt(const char* name, int value);
	void SetFloat(const char* name, float value);
	void SetVec3(const char* name, float x, float y, float z);
	void SetVec3(const char* name, glm::vec3& vec);
	void SetVec4(const char* name, float x, float y, float z, float w);
	void SetVec4(const char* name, glm::vec4& vec);
	void SetMat4(const char* name, glm::mat4& mat);

private:
	Shader(int id);
};