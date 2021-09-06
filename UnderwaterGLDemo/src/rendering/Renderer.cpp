#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <fstream>

Shader* Renderer::current = nullptr;
std::vector<Shader> Renderer::shaders{};

glm::vec3 Renderer::sceneBoundary{};

glm::mat4 Renderer::P{ 0.1f };
glm::vec3 Renderer::cameraPos{ 0.0f, 10.0f, 0.0f };
glm::vec3 Renderer::cameraUp{ 0.0f, 1.0f, 0.0f };
glm::vec3 Renderer::cameraForward{ 0.0f, 0.0f, -1.0f };
float Renderer::cameraPitch = 0.0f;
float Renderer::cameraYaw = 0.0f;

#define GL_SHADER_INCLUDE_ARB 0x8DAE
typedef void (*NamedStringARBPtr)(GLenum, GLint, const char*, GLint, const char*);
NamedStringARBPtr glNamedStringARB;
//typedef void (*CompileShaderIncludeARBPtr)(GLuint, GLsizei, const char* const*, const int*);
//CompileShaderIncludeARBPtr glCompileShaderIncludeARB;

const float FOV = glm::half_pi<float>();
const float Z_NEAR = 0.1f, Z_FAR = 100.0f;

void Renderer::Init(float width, float height, glm::vec3 boundary)
{
	//glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	sceneBoundary = boundary;
	P = glm::perspectiveFov(FOV, width, height, Z_NEAR, Z_FAR);

	glNamedStringARB = reinterpret_cast<NamedStringARBPtr>(glfwGetProcAddress("glNamedStringARB"));
	AddShaderIncludeDir("assets/shaders/include");

	shaders.push_back(Shader::CreateShaderVF("assets/shaders/pass.vert", "assets/shaders/pass.frag"));			// ShaderMode::PassThrough
	shaders.push_back(Shader::CreateShaderVF("assets/shaders/mvp.vert", "assets/shaders/pass.frag"));			// ShaderMode::Basic
	shaders.push_back(Shader::CreateShaderVF("assets/shaders/surface.vert", "assets/shaders/surface.frag"));	// ShaderMode::SurfaceGerstner
	shaders.push_back(Shader::CreateShaderVF("assets/shaders/surfHeight.vert", "assets/shaders/surface.frag"));	// ShaderMode::SurfaceHeightMap
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/currentFreqWave.comp"));						// ShaderMode::ComputeFreqWave
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/ifft_x.comp"));								// ShaderMode::ComputeIFFTX
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/ifft_y.comp"));								// ShaderMode::ComputeIFFTY
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/ifft_y_last.comp"));							// ShaderMode::ComputeIFFTYLastPass
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/normal.comp"));								// ShaderMode::ComputeNormal
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/normalSobel.comp"));							// ShaderMode::ComputeNormalSobel
	UseShader(ShaderMode::PassThrough);
}

void Renderer::UseShader(ShaderMode mode)
{
	current = &shaders[static_cast<int>(mode)];
	current->Use();
	SetMat4("P", P);
	glm::mat4 V = glm::lookAt(cameraPos, cameraPos + cameraForward, cameraUp); // TODO: cache
	glm::mat4 invV = glm::inverse(V);
	SetMat4("V", V);
	SetMat4("invV", invV);
}

void Renderer::TranslateCamera(float forward, float right, float up)
{
	cameraPos += forward * cameraForward + right * glm::normalize(glm::cross(cameraForward, cameraUp));
	cameraPos.y += up;
	cameraPos = glm::clamp(cameraPos, -sceneBoundary, sceneBoundary);
}

void Renderer::RotateCamera(float pitch, float yaw)
{
	cameraPitch = glm::clamp(cameraPitch + pitch, -89.0f, 89.0f);
	cameraYaw = glm::mod(cameraYaw + yaw, 360.0f);
	float yawRad = glm::radians(cameraYaw);
	float pitchRad = glm::radians(cameraPitch);

	cameraForward.x = cos(yawRad) * cos(pitchRad);
	cameraForward.y = sin(pitchRad);
	cameraForward.z = sin(yawRad) * cos(pitchRad);
	cameraForward = glm::normalize(cameraForward);
}

void Renderer::SetInt(const char* name, int value)
{
	current->SetInt(name, value);
}

void Renderer::SetUint(const char* name, unsigned int value)
{
	current->SetUint(name, value);
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

unsigned int Renderer::CreateTexture2D(GLsizei width, GLsizei height, GLint internalFormat, GLenum format, GLenum type, const void* pixels)
{
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	return texture;
}

void Renderer::AddShaderIncludeDir(const char* dir)
{
	for (const auto& entry : std::filesystem::directory_iterator(dir))
	{
		auto path = entry.path();
		auto filename = path.filename().string();

		auto virtualPath = std::string("/") + filename;

		std::ifstream fileStream;
		fileStream.exceptions(std::ifstream::badbit | std::ifstream::failbit);
		fileStream.open(path.string().c_str());
		std::ostringstream stringStream;
		stringStream << fileStream.rdbuf();
		fileStream.close();
		std::string file = stringStream.str();

		glNamedStringARB(GL_SHADER_INCLUDE_ARB, virtualPath.length(), virtualPath.c_str(), file.length(), file.c_str());
	}
}