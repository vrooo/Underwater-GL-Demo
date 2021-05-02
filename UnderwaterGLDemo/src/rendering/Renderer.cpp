#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>

Shader* Renderer::current = nullptr;
std::vector<Shader> Renderer::shaders{};

glm::mat4 Renderer::P{ 0.1f };
glm::vec3 Renderer::cameraPos{ 0.0f, 2.0f, -5.0f };
glm::vec3 Renderer::cameraUp{ 0.0f, 1.0f, 0.0f };
glm::vec3 Renderer::cameraForward{ 0.0f, 0.0f, 1.0f };
float Renderer::cameraPitch = 0.0f;
float Renderer::cameraYaw = 0.0f;

const float FOV = glm::half_pi<float>();
const float Z_NEAR = 0.1f, Z_FAR = 100.0f;

void Renderer::Init(float width, float height)
{
	P = glm::perspectiveFov(FOV, width, height, Z_NEAR, Z_FAR);

	shaders.push_back(Shader{ "assets/shaders/pass.vert", "assets/shaders/pass.frag" });	// ShaderMode::PassThrough
	shaders.push_back(Shader{ "assets/shaders/mvp.vert", "assets/shaders/pass.frag" });		// ShaderMode::Basic
	UseShader(ShaderMode::PassThrough);
}

void Renderer::UseShader(ShaderMode mode)
{
	current = &shaders[static_cast<int>(mode)];
	current->Use();
	SetMat4("P", P);
	glm::mat4 V = glm::lookAt(cameraPos, cameraPos + cameraForward, cameraUp); // TODO: cache
	SetMat4("V", V);
}

void Renderer::TranslateCamera(float forward, float right, float up)
{
	cameraPos += forward * cameraForward + right * glm::normalize(glm::cross(cameraForward, cameraUp));
	cameraPos.y += up;
}

void Renderer::RotateCamera(float pitch, float yaw)
{
	cameraPitch = glm::clamp(cameraPitch + pitch, -89.0f, 89.0f);
	cameraYaw += yaw;
	cameraForward.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
	cameraForward.y = sin(glm::radians(cameraPitch));
	cameraForward.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
	cameraForward = glm::normalize(cameraForward);
}

void Renderer::SetInt(const char* name, int value)
{
	current->SetInt(name, value);
}

void Renderer::SetFloat(const char* name, float value)
{
	current->SetFloat(name, value);
}

void Renderer::SetVec3(const char* name, float x, float y, float z)
{
	current->SetVec3(name, x, y, z);
}

void Renderer::SetVec3(const char* name, glm::vec3& vec)
{
	current->SetVec3(name, vec);
}

void Renderer::SetVec4(const char* name, float x, float y, float z, float w)
{
	current->SetVec4(name, x, y, z, w);
}

void Renderer::SetVec4(const char* name, glm::vec4& vec)
{
	current->SetVec4(name, vec);
}

void Renderer::SetMat4(const char* name, glm::mat4& mat)
{
	current->SetMat4(name, mat);
}