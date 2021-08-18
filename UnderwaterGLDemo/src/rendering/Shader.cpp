#include "Shader.h"
#include <exception>
#include <fstream>
#include <sstream>
#include <string>

#include <glm/gtc/type_ptr.hpp>

const int INFO_LOG_SIZE = 512;

// helper functions - declarations
int AttachShader(int id, const char* path, GLenum shaderType, int& success, char* infoLog);
void LinkProgram(int id, int& success, char* infoLog);

// class methods
Shader Shader::CreateShaderVF(const char* vertPath, const char* fragPath)
{
	int success;
	char infoLog[INFO_LOG_SIZE];
	int id = glCreateProgram();

	int vertShader = AttachShader(id, vertPath, GL_VERTEX_SHADER, success, infoLog);
	int fragShader = AttachShader(id, fragPath, GL_FRAGMENT_SHADER, success, infoLog);

	LinkProgram(id, success, infoLog);

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return Shader{ id };
}

Shader Shader::CreateShaderVGF(const char* vertPath, const char* geomPath, const char* fragPath)
{
	int success;
	char infoLog[INFO_LOG_SIZE];
	int id = glCreateProgram();

	int vertShader = AttachShader(id, vertPath, GL_VERTEX_SHADER, success, infoLog);
	int geomShader = AttachShader(id, geomPath, GL_GEOMETRY_SHADER, success, infoLog);
	int fragShader = AttachShader(id, fragPath, GL_FRAGMENT_SHADER, success, infoLog);

	LinkProgram(id, success, infoLog);

	glDeleteShader(vertShader);
	glDeleteShader(geomShader);
	glDeleteShader(fragShader);

	return Shader{ id };
}

Shader Shader::CreateShaderCompute(const char* compPath)
{
	int success;
	char infoLog[INFO_LOG_SIZE];
	int id = glCreateProgram();

	int compShader = AttachShader(id, compPath, GL_COMPUTE_SHADER, success, infoLog);

	LinkProgram(id, success, infoLog);

	glDeleteShader(compShader);

	return Shader{ id };
}

Shader::Shader(int id) : ID(id) {}

void Shader::Use()
{
	glUseProgram(ID);
}

void Shader::SetInt(const char* name, int value)
{
	glUniform1i(glGetUniformLocation(ID, name), value);
}

void Shader::SetUint(const char* name, unsigned int value)
{
	glUniform1ui(glGetUniformLocation(ID, name), value);
}

void Shader::SetFloat(const char* name, float value)
{
	glUniform1f(glGetUniformLocation(ID, name), value);
}

void Shader::SetVec3(const char* name, float x, float y, float z)
{
	glUniform3f(glGetUniformLocation(ID, name), x, y, z);
}

void Shader::SetVec3(const char* name, glm::vec3& vec)
{
	glUniform3fv(glGetUniformLocation(ID, name), 1, glm::value_ptr(vec));
}

void Shader::SetVec4(const char* name, float x, float y, float z, float w)
{
	glUniform4f(glGetUniformLocation(ID, name), x, y, z, w);
}

void Shader::SetVec4(const char* name, glm::vec4& vec)
{
	glUniform4fv(glGetUniformLocation(ID, name), 1, glm::value_ptr(vec));
}

void Shader::SetMat4(const char* name, glm::mat4& mat)
{
	glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, glm::value_ptr(mat));
}

// helper functions - definitions
int AttachShader(int id, const char* path, GLenum shaderType, int& success, char* infoLog)
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
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, INFO_LOG_SIZE, nullptr, infoLog);
		throw std::runtime_error(infoLog);
	}

	glAttachShader(id, shader);
	return shader;
}

void LinkProgram(int id, int& success, char* infoLog)
{
	glLinkProgram(id);
	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(id, INFO_LOG_SIZE, nullptr, infoLog);
		throw std::runtime_error(infoLog);
	}
}
