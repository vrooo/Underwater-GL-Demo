#pragma once

#include <iostream>
#include <vector>

#include <glm/glm.hpp>

#include "Shader.h"

enum class ShaderMode
{
	PassThrough,
	Basic,
	Surface
};

class Renderer
{
public:
	static void Init(float width, float height, glm::vec3 boundary);
	static void UseShader(ShaderMode mode);

	static void SetInt(const char* name, int value);
	static void SetFloat(const char* name, float value);
	static void SetVec3(const char* name, float x, float y, float z);
	static void SetVec3(const char* name, glm::vec3& vec);
	static void SetVec4(const char* name, float x, float y, float z, float w);
	static void SetVec4(const char* name, glm::vec4& vec);
	static void SetMat4(const char* name, glm::mat4& mat);

	static void TranslateCamera(float forward, float right, float up);
	static void RotateCamera(float pitch, float yaw);

private:
	static Shader* current; // TODO: this is ugly
	static std::vector<Shader> shaders;

	static glm::vec3 sceneBoundary;

	static glm::mat4 P;
	static glm::vec3 cameraPos;
	static glm::vec3 cameraUp;
	static glm::vec3 cameraForward;
	static float cameraPitch;
	static float cameraYaw;
};