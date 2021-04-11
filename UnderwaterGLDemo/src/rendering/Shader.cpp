#include "Shader.h"
#include <exception>
#include <fstream>
#include <sstream>
#include <string>

const int INFO_LOG_SIZE = 512;

Shader::Shader(const char* vertPath, const char* fragPath)
{
	int success;
	char infoLog[INFO_LOG_SIZE];
	int vertShader = CompileShader(vertPath, GL_VERTEX_SHADER, &success, infoLog);
	int fragShader = CompileShader(fragPath, GL_FRAGMENT_SHADER, &success, infoLog);

	ID = glCreateProgram();
	glAttachShader(ID, vertShader);
	glAttachShader(ID, fragShader);
	glLinkProgram(ID);
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(ID, INFO_LOG_SIZE, nullptr, infoLog);
		throw std::runtime_error(infoLog);
	}

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);
}

int Shader::CompileShader(const char* path, GLenum shaderType, int* success, char* infoLog)
{
	std::ifstream fileStream;
	fileStream.exceptions(std::ifstream::badbit | std::ifstream::failbit);
	fileStream.open(path);

	std::ostringstream stringStream;
	stringStream << fileStream.rdbuf();
	fileStream.close();

	auto shaderString = stringStream.str();
	auto shaderCode = shaderString.c_str();

	unsigned int shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderCode, nullptr);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, success);
	if (!success)
	{
		glGetShaderInfoLog(shader, INFO_LOG_SIZE, nullptr, infoLog);
		throw std::runtime_error(infoLog);
	}

	return shader;
}

void Shader::Use()
{
	glUseProgram(ID);
}

void Shader::SetInt(const char* name, int value)
{
	glUniform1i(glGetUniformLocation(ID, name), value);
}

void Shader::SetFloat(const char* name, float value)
{
	glUniform1f(glGetUniformLocation(ID, name), value);
}
