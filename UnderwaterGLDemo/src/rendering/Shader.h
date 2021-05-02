#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader
{
public:
	unsigned int ID;

	Shader(const char* vertPath, const char* fragPath); // catch std::exception const&
	void Use();
	void SetInt(const char* name, int value);
	void SetFloat(const char* name, float value);
	void SetVec3(const char* name, float x, float y, float z);
	void SetVec3(const char* name, glm::vec3& vec);
	void SetVec4(const char* name, float x, float y, float z, float w);
	void SetVec4(const char* name, glm::vec4& vec);
	void SetMat4(const char* name, glm::mat4& mat);
private:
	int CompileShader(const char* path, GLenum shaderType, int* success, char* infoLog);
};