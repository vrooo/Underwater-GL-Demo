#pragma once
#include <glad/glad.h>

class Shader
{
public:
	unsigned int ID;

	Shader(const char* vertPath, const char* fragPath); // catch std::exception const&
	void Use();
	void SetInt(const char* name, int value);
	void SetFloat(const char* name, float value);
private:
	int CompileShader(const char* path, GLenum shaderType, int* success, char* infoLog);
};