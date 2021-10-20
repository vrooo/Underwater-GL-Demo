#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <fstream>

Shader* Renderer::current = nullptr;
std::vector<Shader> Renderer::shaders{};

glm::vec3 Renderer::sceneBoundary{};

glm::mat4 Renderer::P{ 0.1f };
glm::vec3 Renderer::cameraPos{ 0.0f, 5.0f, 15.0f };
glm::vec3 Renderer::cameraUp{ 0.0f, 1.0f, 0.0f };
glm::vec3 Renderer::cameraForward{ 0.0f, 0.0f, -1.0f };
float Renderer::cameraPitch = 0.0f;
float Renderer::cameraYaw = 180.0f;

#define GL_SHADER_INCLUDE_ARB 0x8DAE
typedef void (*NamedStringARBPtr)(GLenum, GLint, const char*, GLint, const char*);
NamedStringARBPtr glNamedStringARB;

const float FOV = glm::half_pi<float>();
const float Z_NEAR = 0.5f, Z_FAR = 200.0f;

void Renderer::Init(float width, float height, glm::vec3 boundary)
{
	//glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	sceneBoundary = boundary;
	P = glm::perspectiveFov(FOV, width, height, Z_NEAR, Z_FAR);

	glNamedStringARB = reinterpret_cast<NamedStringARBPtr>(glfwGetProcAddress("glNamedStringARB"));
	AddShaderIncludeDir("assets/shaders/include");

	shaders.push_back(Shader::CreateShaderVF("assets/shaders/pass.vert", "assets/shaders/pass.frag"));				// ShaderMode::PassThrough
	shaders.push_back(Shader::CreateShaderVF("assets/shaders/point.vert", "assets/shaders/pass.frag"));				// ShaderMode::Point
	shaders.push_back(Shader::CreateShaderVF("assets/shaders/phong.vert", "assets/shaders/phong.frag"));			// ShaderMode::Phong
	shaders.push_back(Shader::CreateShaderVF("assets/shaders/surfaceDisplace.vert", "assets/shaders/phong.frag"));	// ShaderMode::SurfaceDisplacement
	shaders.push_back(Shader::CreateShaderVF("assets/shaders/surfaceHeight.vert", "assets/shaders/phong.frag"));	// ShaderMode::SurfaceHeight
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/currentFreqWave.comp"));							// ShaderMode::ComputeFreqWave
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/ifftX.comp"));									// ShaderMode::ComputeIFFTX
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/ifftY.comp"));									// ShaderMode::ComputeIFFTY
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/ifftYLast.comp"));								// ShaderMode::ComputeIFFTYLastPass
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/normalFourier.comp"));							// ShaderMode::ComputeNormalFourier
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/normalSobel.comp"));								// ShaderMode::ComputeNormalSobel
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/gerstner.comp"));									// ShaderMode::ComputeGerstner
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/surfaceBoundingBoxes.comp"));						// ShaderMode::ComputeSurfaceBoundingBoxes
	shaders.push_back(Shader::CreateShaderCompute("assets/shaders/photonMappingCastRays.comp"));					// ShaderMode::ComputePhotonMappingCastRays
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

	cameraForward.x = sin(yawRad) * cos(pitchRad);
	cameraForward.y = sin(pitchRad);
	cameraForward.z = cos(yawRad) * cos(pitchRad);
	cameraForward = glm::normalize(cameraForward);
}

void Renderer::SetTexture2D(GLenum textureUnit, const char* name, GLuint texture)
{
	glActiveTexture(textureUnit);
	glBindTexture(GL_TEXTURE_2D, texture);
	SetInt(name, textureUnit - GL_TEXTURE0);
}

void Renderer::SetImage(GLuint imageUnit, const char* name, GLuint image, GLenum access, GLenum format)
{
	glBindImageTexture(imageUnit, image, 0, true, 0, access, format);
	Renderer::SetInt(name, imageUnit);
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

GLuint Renderer::CreateTexture2D(GLsizei width, GLsizei height, GLint internalFormat, GLenum format, GLenum type, const void* pixels,
								 GLint filterType, GLint texWrapType)
{
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterType);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterType);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texWrapType);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texWrapType);
	return texture;
}

void Renderer::SubTexture2DData(GLuint texture, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, width, height, format, type, pixels);
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