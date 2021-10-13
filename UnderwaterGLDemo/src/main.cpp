#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define cimg_use_tiff
#include <CImg/CImg.h>

#include "Rendering/DynamicPointMesh.h"
#include "Rendering/Model.h"
#include "Rendering/Plane.h"
#include "Rendering/Scene.h"
#include "Rendering/Shader.h"
#include "Rendering/Renderer.h"

#define SAVE_FOURIER_DEBUG false

const unsigned int WINDOW_WIDTH = 1600;
const unsigned int WINDOW_HEIGHT = 900;
const float MIN_SURFACE_SIZE = 1.0f;
const float MAX_SURFACE_SIZE = 100.0f;
const float SURFACE_MOVE_BUFFER = 10.0f;
const float SCENE_HEIGHT = 30.0f;
const float CAM_MOVE_SPEED = 10.0f;
const float CAM_ROTATE_SPEED = 0.1f;

const unsigned int CHUNK_VERTEX_COUNT = 64;
const unsigned int MAX_WAVE_COUNT = 100;
const unsigned int MIN_GRID_COUNT = 10;
const unsigned int MAX_GRID_COUNT = 2000;
const unsigned int MIN_FOURIER_POWER = 6;
const unsigned int MAX_FOURIER_POWER = 11;
const unsigned int MAX_FOURIER_GRID_SIZE = 1 << MAX_FOURIER_POWER;
const unsigned int COMPUTE_CHUNK = 32;

const float DEPTH_INF = 100.0f;
const float TENSION_NONE = 0.0f;

const float GRAVITY = 9.8f;

float lastX = WINDOW_WIDTH / 2, lastY = WINDOW_HEIGHT / 2;
float gerstnerWaveData[MAX_WAVE_COUNT * 2][4];
float freqWaveData[MAX_FOURIER_GRID_SIZE * MAX_FOURIER_GRID_SIZE][3];
float debugFreqWaveData[MAX_FOURIER_GRID_SIZE * MAX_FOURIER_GRID_SIZE * 3];
unsigned int fourierCoordLookup[(MAX_FOURIER_GRID_SIZE / 2) * (MAX_FOURIER_POWER + 1)][2];

void ProcessKeyboard(GLFWwindow* window, float dt);
void ProcessMouse(GLFWwindow* window, double posX, double posY);
void GenerateGerstnerWaves(int waveCount, float minAngle, float maxAngle, float minAmp, float maxAmp, float minK, float maxK,
				   float d, float l, float waveData[MAX_WAVE_COUNT * 2][4]);
void GenerateFourierWaves(float amplitude, float windSpeed, float windAngle, float smallWaveLen, unsigned int fourierGridSize, float patchSize);
void GenerateFourierCoordLookup(unsigned int fourierGridSize, int fourierPower);
int ReverseBits(int val, int digitCount);
void Debug_WriteImage(unsigned int fourierGridSize);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Underwater Demo", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "ERROR: Failed to create window\n";
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "ERROR: Failed to initialize GLAD\n";
		return -1;
	}

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	try
	{
		Renderer::Init(WINDOW_WIDTH, WINDOW_HEIGHT, glm::vec3{ MAX_SURFACE_SIZE / 2 + SURFACE_MOVE_BUFFER, SCENE_HEIGHT, MAX_SURFACE_SIZE / 2 + SURFACE_MOVE_BUFFER });
	}
	catch(std::exception const& e)
	{
		std::cout << "ERROR: " << e.what() << "\n";
		return -1;
	}

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");

	glfwSetCursorPosCallback(window, ProcessMouse);

	std::cout << "Initialization complete\n";

	// scenes
	float scenePosition[]{ 0.0f, 0.0f, 0.0f };
	float sceneSize = 1.0f;
	Scene sceneCornellOriginal{ "assets/scenes/cornell/CornellBox-Original.obj" };

	// water things
	float waterColor[]{ 0.2f, 0.3f, 0.3f };
	glm::vec3 defWaterColor{ waterColor[0], waterColor[1], waterColor[2] };
	Material waterMat{ defWaterColor, glm::vec3{0.5f}, glm::vec3{0.5f}, 10.0f };
	int gridVertexCount = 512; // TODO: power
	int patchCountLevel = 1, patchCount = 2 * patchCountLevel - 1;
	float surfaceSize = 20.0f;
	Plane waterPlane = MakeXZPlane(waterMat, gridVertexCount, CHUNK_VERTEX_COUNT);
	waterPlane.SetScale(surfaceSize);

	unsigned int surfaceBoundingBoxesTex =
		Renderer::CreateTexture2D(511, 2, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr); // TODO: replace 511 with max water size or sth

	int waveCount = 20, newWaveCount = 20;
	int gerstnerTexResolution = 512, newGerstnerTexResolution = 512; // TODO: power
	float minAngle = 110.0f, maxAngle = 120.0f;
	float minAmp = 0.001f, maxAmp = 0.004f;
	float minK = 1.0f, maxK = 30.0f; // TODO: maybe wavelength instead?
	float d = 10.0f, l = 0.0f;
	GenerateGerstnerWaves(waveCount, minAngle, maxAngle, minAmp, maxAmp, minK, maxK, d, l, gerstnerWaveData);

	unsigned int gerstnerWaveTex =
		Renderer::CreateTexture2D(MAX_WAVE_COUNT, 2, GL_RGBA32F, GL_RGBA, GL_FLOAT, gerstnerWaveData);
	// these two need to be regenerated each time using the correct size
	unsigned int gerstnerDisplacementTex =
		Renderer::CreateTexture2D(gerstnerTexResolution, gerstnerTexResolution, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR);
	unsigned int gerstnerNormalTex =
		Renderer::CreateTexture2D(gerstnerTexResolution, gerstnerTexResolution, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR);

	float fourierAmp = 500.0f;
	float fourierWindSpeed = 100.0f, fourierWindAngle = 135.0f;
	float fourierSmallWave = 0.01f;
	int fourierPower = 9;
	unsigned int fourierGridSize = 1 << fourierPower;
	GenerateFourierWaves(fourierAmp, fourierWindSpeed, fourierWindAngle, fourierSmallWave, fourierGridSize, surfaceSize);
	if (SAVE_FOURIER_DEBUG)
	{
		Debug_WriteImage(fourierGridSize);
	}
	GenerateFourierCoordLookup(fourierGridSize, fourierPower);

	unsigned int fourierInitFreqTex =
		Renderer::CreateTexture2D(MAX_FOURIER_GRID_SIZE, MAX_FOURIER_GRID_SIZE, GL_RGB32F, GL_RGB, GL_FLOAT, freqWaveData);
	unsigned int fourierCurFreqTex =
		Renderer::CreateTexture2D(MAX_FOURIER_GRID_SIZE, MAX_FOURIER_GRID_SIZE, GL_RG32F, GL_RG, GL_FLOAT, nullptr);
	unsigned int fourierCoordLookupTex =
		Renderer::CreateTexture2D(MAX_FOURIER_GRID_SIZE / 2, MAX_FOURIER_POWER + 1, GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_INT, fourierCoordLookup);
	unsigned int fourierBufferTex1 =
		Renderer::CreateTexture2D(MAX_FOURIER_GRID_SIZE, MAX_FOURIER_GRID_SIZE, GL_RG32F, GL_RG, GL_FLOAT, nullptr);
	unsigned int fourierBufferTex2 =
		Renderer::CreateTexture2D(MAX_FOURIER_GRID_SIZE, MAX_FOURIER_GRID_SIZE, GL_RG32F, GL_RG, GL_FLOAT, nullptr);
	unsigned int fourierChoppyBufferTex1 =
		Renderer::CreateTexture2D(MAX_FOURIER_GRID_SIZE, MAX_FOURIER_GRID_SIZE, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
	unsigned int fourierChoppyBufferTex2 =
		Renderer::CreateTexture2D(MAX_FOURIER_GRID_SIZE, MAX_FOURIER_GRID_SIZE, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
	unsigned int fourierSlopeBufferTex1 =
		Renderer::CreateTexture2D(MAX_FOURIER_GRID_SIZE, MAX_FOURIER_GRID_SIZE, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
	unsigned int fourierSlopeBufferTex2 =
		Renderer::CreateTexture2D(MAX_FOURIER_GRID_SIZE, MAX_FOURIER_GRID_SIZE, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
	// these two need to be regenerated each time using the correct size
	unsigned int fourierDisplacementTex =
		Renderer::CreateTexture2D(fourierGridSize, fourierGridSize, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR, GL_REPEAT);
	unsigned int fourierNormalTex =
		Renderer::CreateTexture2D(fourierGridSize, fourierGridSize, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR, GL_REPEAT);

	const int DEBUG_PHOTON_SIZE_1 = 10, DEBUG_PHOTON_SIZE_2 = 10;
	DynamicPointMesh DEBUG_DPM{ DEBUG_PHOTON_SIZE_1 * DEBUG_PHOTON_SIZE_2 * 1024, 5.0f, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f} };

	float timeMult = 1.0f;
	float lastTime = glfwGetTime(), simTime = 0;
	bool useDisplacement = false;
	bool gerstnerTexResolutionChanged = false;
	bool useFourierWaves = false, useFourierSobelNormals = true, fourierGridSizeChanged = false;
	while (!glfwWindowShouldClose(window))
	{
		float t = glfwGetTime(), diffT = t - lastTime;
		ProcessKeyboard(window, diffT);
		lastTime = t;

		glClearColor(0.9f, 0.8f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Menu");
		ImGui::Text("%.1f FPS, %.3f ms per frame", io.Framerate, 1000.0f / io.Framerate);
		ImGui::SliderFloat("Time multiplier", &timeMult, 0.01f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		simTime += timeMult * diffT;

		if (ImGui::SliderFloat("Surface size", &surfaceSize, MIN_SURFACE_SIZE, MAX_SURFACE_SIZE, "%.3f", ImGuiSliderFlags_AlwaysClamp))
		{
			waterPlane.SetScale(surfaceSize);
		}
		std::string patchCountString = std::to_string(patchCount);
		if (ImGui::SliderInt("Patch count", &patchCountLevel, 1, 5, patchCountString.c_str(), ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput))
		{
			patchCount = 2 * patchCountLevel - 1;
		}
		ImGui::SliderInt("Grid count", &gridVertexCount, MIN_GRID_COUNT, MAX_GRID_COUNT, "%d", ImGuiSliderFlags_AlwaysClamp);
		if (ImGui::Button("Regenerate surface"))
		{
			waterPlane.Recreate(gridVertexCount, CHUNK_VERTEX_COUNT);
		}
		if (ImGui::ColorEdit3("Surface color", waterColor))
		{
			waterPlane.SetColor(waterColor);
		}
		ImGui::Checkbox("Displacement", &useDisplacement);

		if (ImGui::Button(useFourierWaves ? "Gerstner waves" : "Fourier waves"))
		{
			useFourierWaves = !useFourierWaves;
		}

		if (useFourierWaves)
		{
			// FOURIER WAVES
			Renderer::UseShader(ShaderMode::ComputeFreqWave);

			if (ImGui::Button(useFourierSobelNormals ? "IFFT normals" : "Sobel normals"))
			{
				useFourierSobelNormals = !useFourierSobelNormals;
			}
			std::string fourierGridSizeString = std::to_string(1 << fourierPower);
			if (ImGui::SliderInt("Fourier grid size", &fourierPower, MIN_FOURIER_POWER, MAX_FOURIER_POWER, fourierGridSizeString.c_str(), ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput))
			{
				fourierGridSizeChanged = true;
			}

			ImGui::SliderFloat("Frequency amplitude", &fourierAmp, 100.0f, 1000000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Wind speed", &fourierWindSpeed, 0.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Wind angle", &fourierWindAngle, 0.0f, 360.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Min wave length", &fourierSmallWave, 0.0f, 50.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::Button("Regenerate waves"))
			{
				if (fourierGridSizeChanged)
				{
					fourierGridSizeChanged = false;
					fourierGridSize = 1 << fourierPower;

					GenerateFourierCoordLookup(fourierGridSize, fourierPower);
					glBindTexture(GL_TEXTURE_2D, fourierCoordLookupTex);
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MAX_FOURIER_GRID_SIZE / 2, MAX_FOURIER_POWER + 1, GL_RG_INTEGER, GL_UNSIGNED_INT, fourierCoordLookup);

					fourierDisplacementTex = 
						Renderer::CreateTexture2D(fourierGridSize, fourierGridSize, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR, GL_REPEAT);
					fourierNormalTex = 
						Renderer::CreateTexture2D(fourierGridSize, fourierGridSize, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR, GL_REPEAT);

				}
				GenerateFourierWaves(fourierAmp, fourierWindSpeed, fourierWindAngle, fourierSmallWave, fourierGridSize, surfaceSize);
				glBindTexture(GL_TEXTURE_2D, fourierInitFreqTex);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MAX_FOURIER_GRID_SIZE, MAX_FOURIER_GRID_SIZE, GL_RGB, GL_FLOAT, freqWaveData);
			}

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fourierInitFreqTex);
			Renderer::SetInt("freqWaveTex", 0);
			glBindImageTexture(0, fourierCurFreqTex, 0, true, 0, GL_WRITE_ONLY, GL_RG32F);
			Renderer::SetInt("curFreqWaveTex", 0);
			Renderer::SetUint("fourierGridSize", fourierGridSize);
			Renderer::SetFloat("t", simTime);

			int fourierGroupSize = fourierGridSize / COMPUTE_CHUNK;
			glDispatchCompute(fourierGroupSize, fourierGroupSize, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			if (SAVE_FOURIER_DEBUG)
			{
				glBindTexture(GL_TEXTURE_2D, fourierCurFreqTex);
				glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, freqWaveData);
				Debug_WriteImage(fourierGridSize);
			}

			Renderer::UseShader(ShaderMode::ComputeIFFTX);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fourierCoordLookupTex);
			Renderer::SetInt("coordLookupTex", 0);
			Renderer::SetUint("fourierGridSize", fourierGridSize);
			bool readFromFirst = true;
			for (int level = 0, N = 1; N <= fourierGridSize; level++, N *= 2)
			{
				if (readFromFirst)
				{
					if (level == 0)
					{
						glBindImageTexture(0, fourierCurFreqTex, 0, true, 0, GL_READ_ONLY, GL_RG32F);
					}
					else
					{
						glBindImageTexture(0, fourierBufferTex1, 0, true, 0, GL_READ_ONLY, GL_RG32F);
						glBindImageTexture(2, fourierChoppyBufferTex1, 0, true, 0, GL_READ_ONLY, GL_RGBA32F);
						glBindImageTexture(4, fourierSlopeBufferTex1, 0, true, 0, GL_READ_ONLY, GL_RGBA32F);
					}
					glBindImageTexture(1, fourierBufferTex2, 0, true, 0, GL_WRITE_ONLY, GL_RG32F);
					glBindImageTexture(3, fourierChoppyBufferTex2, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
					glBindImageTexture(5, fourierSlopeBufferTex2, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
				}
				else
				{
					glBindImageTexture(0, fourierBufferTex2, 0, true, 0, GL_READ_ONLY, GL_RG32F);
					glBindImageTexture(1, fourierBufferTex1, 0, true, 0, GL_WRITE_ONLY, GL_RG32F);
					glBindImageTexture(2, fourierChoppyBufferTex2, 0, true, 0, GL_READ_ONLY, GL_RGBA32F);
					glBindImageTexture(3, fourierChoppyBufferTex1, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
					glBindImageTexture(4, fourierSlopeBufferTex2, 0, true, 0, GL_READ_ONLY, GL_RGBA32F);
					glBindImageTexture(5, fourierSlopeBufferTex1, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
				}
				readFromFirst = !readFromFirst;
				Renderer::SetInt("readTex", 0);
				Renderer::SetInt("writeTex", 1);
				Renderer::SetInt("readChoppyTex", 2);
				Renderer::SetInt("writeChoppyTex", 3);
				Renderer::SetInt("readSlopeTex", 4);
				Renderer::SetInt("writeSlopeTex", 5);
				Renderer::SetUint("N", N);
				Renderer::SetUint("level", level);
				glDispatchCompute(fourierGroupSize / 2, fourierGroupSize, 1);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
			}

			Renderer::UseShader(ShaderMode::ComputeIFFTY);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fourierCoordLookupTex);
			Renderer::SetInt("coordLookupTex", 0);
			Renderer::SetUint("fourierGridSize", fourierGridSize);
			for (int level = 0, N = 1; N <= fourierGridSize; level++, N *= 2)
			{
				if (N == fourierGridSize)
				{
					Renderer::UseShader(ShaderMode::ComputeIFFTYLastPass);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, fourierCoordLookupTex);
					Renderer::SetInt("coordLookupTex", 0);
				}
				if (readFromFirst)
				{
					glBindImageTexture(0, fourierBufferTex1, 0, true, 0, GL_READ_ONLY, GL_RG32F);
					glBindImageTexture(1, fourierBufferTex2, 0, true, 0, GL_WRITE_ONLY, GL_RG32F);
					glBindImageTexture(2, fourierChoppyBufferTex1, 0, true, 0, GL_READ_ONLY, GL_RGBA32F);
					glBindImageTexture(3, fourierChoppyBufferTex2, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
					glBindImageTexture(4, fourierSlopeBufferTex1, 0, true, 0, GL_READ_ONLY, GL_RGBA32F);
					glBindImageTexture(5, fourierSlopeBufferTex2, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
				}
				else
				{
					glBindImageTexture(0, fourierBufferTex2, 0, true, 0, GL_READ_ONLY, GL_RG32F);
					glBindImageTexture(1, fourierBufferTex1, 0, true, 0, GL_WRITE_ONLY, GL_RG32F);
					glBindImageTexture(2, fourierChoppyBufferTex2, 0, true, 0, GL_READ_ONLY, GL_RGBA32F);
					glBindImageTexture(3, fourierChoppyBufferTex1, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
					glBindImageTexture(4, fourierSlopeBufferTex2, 0, true, 0, GL_READ_ONLY, GL_RGBA32F);
					glBindImageTexture(5, fourierSlopeBufferTex1, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
				}
				readFromFirst = !readFromFirst;
				Renderer::SetInt("readTex", 0);
				Renderer::SetInt("writeTex", 1);
				Renderer::SetInt("readChoppyTex", 2);
				Renderer::SetInt("writeChoppyTex", 3);
				Renderer::SetInt("readSlopeTex", 4);
				Renderer::SetInt("writeSlopeTex", 5);
				Renderer::SetUint("N", N);
				Renderer::SetUint("level", level);
				glDispatchCompute(fourierGroupSize, fourierGroupSize / 2, 1);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
			}

			if (useFourierSobelNormals)
				Renderer::UseShader(ShaderMode::ComputeNormalSobel);
			else
			{
				Renderer::UseShader(ShaderMode::ComputeNormalFourier);
				glBindImageTexture(1, readFromFirst ? fourierSlopeBufferTex1 : fourierSlopeBufferTex2, 0, true, 0, GL_READ_ONLY, GL_RGBA32F);
				Renderer::SetInt("slopeTex", 1);
			}
			glBindImageTexture(0, readFromFirst ? fourierBufferTex1 : fourierBufferTex2, 0, true, 0, GL_READ_ONLY, GL_RG32F);
			glBindImageTexture(2, readFromFirst ? fourierChoppyBufferTex1 : fourierChoppyBufferTex2, 0, true, 0, GL_READ_ONLY, GL_RGBA32F);
			glBindImageTexture(3, fourierDisplacementTex, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
			glBindImageTexture(4, fourierNormalTex, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
			Renderer::SetInt("heightTex", 0);
			Renderer::SetInt("choppyTex", 2);
			Renderer::SetInt("displacementTex", 3);
			Renderer::SetInt("normalTex", 4);
			Renderer::SetUint("fourierGridSize", fourierGridSize);
			glDispatchCompute(fourierGroupSize, fourierGroupSize, 1);

			Renderer::UseShader(useDisplacement ? ShaderMode::SurfaceDisplacement : ShaderMode::SurfaceHeight);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fourierDisplacementTex);
			Renderer::SetInt("displacementTex", 0);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, fourierNormalTex);
			Renderer::SetInt("normalTex", 1);
		}
		else
		{
			// GERSTNER WAVES
			Renderer::UseShader(ShaderMode::ComputeGerstner);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gerstnerWaveTex);

			if (ImGui::SliderInt("Texture resolution", &newGerstnerTexResolution, MIN_GRID_COUNT, MAX_GRID_COUNT, "%d", ImGuiSliderFlags_AlwaysClamp))
			{
				gerstnerTexResolutionChanged = true;
			}
			ImGui::SliderInt("Wave count", &newWaveCount, 1, MAX_WAVE_COUNT, "%d", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Min angle", &minAngle, 0.0f, 360.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Max angle", &maxAngle, 0.0f, 360.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Min amplitude", &minAmp, 0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Max amplitude", &maxAmp, 0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Min k", &minK, 0.001f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp); // TODO: wavelength or wind speed instead of k
			ImGui::SliderFloat("Max k", &maxK, 0.001f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Depth", &d, 0.01f, DEPTH_INF, d == DEPTH_INF ? "Infinite" : "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Surface tension scale", &l, TENSION_NONE, 1.0f, l == TENSION_NONE ? "None" : "%.3f", ImGuiSliderFlags_AlwaysClamp);

			if (ImGui::Button("Regenerate waves"))
			{
				waveCount = newWaveCount;
				GenerateGerstnerWaves(waveCount, minAngle, maxAngle, minAmp, maxAmp, minK, maxK, d, l, gerstnerWaveData);
				//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, waveCount, 2, GL_RGBA, GL_FLOAT, gerstnerWaveData);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MAX_WAVE_COUNT, 2, GL_RGBA, GL_FLOAT, gerstnerWaveData); // TODO: why can't I sub only waveCount columns?

				if (gerstnerTexResolutionChanged)
				{
					gerstnerTexResolutionChanged = false;
					gerstnerTexResolution = newGerstnerTexResolution;
					gerstnerDisplacementTex =
						Renderer::CreateTexture2D(gerstnerTexResolution, gerstnerTexResolution, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR);
					gerstnerNormalTex =
						Renderer::CreateTexture2D(gerstnerTexResolution, gerstnerTexResolution, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR);
				}
			}

			glBindImageTexture(0, gerstnerWaveTex, 0, true, 0, GL_READ_ONLY, GL_RGBA32F);
			Renderer::SetInt("waveTex", 0);
			glBindImageTexture(1, gerstnerDisplacementTex, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
			Renderer::SetInt("displacementTex", 1);
			glBindImageTexture(2, gerstnerNormalTex, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
			Renderer::SetInt("normalTex", 2);

			Renderer::SetInt("texResolution", gerstnerTexResolution);
			Renderer::SetInt("waveCount", waveCount);
			Renderer::SetFloat("t", simTime);

			int gerstnerGroupSize = ceil((float)gerstnerTexResolution / COMPUTE_CHUNK);
			glDispatchCompute(gerstnerGroupSize, gerstnerGroupSize, 1);

			Renderer::UseShader(useDisplacement ? ShaderMode::SurfaceDisplacement : ShaderMode::SurfaceHeight);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gerstnerDisplacementTex);
			Renderer::SetInt("displacementTex", 0);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gerstnerNormalTex);
			Renderer::SetInt("normalTex", 1);
		}
		Renderer::SetInt("patchCount", patchCount);
		waterPlane.RenderInstanced(patchCount * patchCount);

		if (ImGui::SliderFloat("Scene size", &sceneSize, 0.1f, 20.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
		{
			sceneCornellOriginal.SetScale(sceneSize);
		}
		if (ImGui::SliderFloat3("Scene position", scenePosition, -20.0f, 20.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
		{
			sceneCornellOriginal.SetPosition(scenePosition);
		}

		Renderer::UseShader(ShaderMode::Phong);
		sceneCornellOriginal.Render();

		// TODO: test
		unsigned int curDisplacementTex = useFourierWaves ? fourierDisplacementTex : gerstnerDisplacementTex;
		unsigned int curNormalTex		= useFourierWaves ? fourierNormalTex : gerstnerNormalTex;

		Renderer::UseShader(ShaderMode::ComputeSurfaceBoundingBoxes);
		waterPlane.BindVertexSSBO(0);
		waterPlane.BindChunkInfoSSBO(1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, curDisplacementTex);
		Renderer::SetInt("displacementTex", 0);
		
		glDispatchCompute((CHUNK_VERTEX_COUNT * CHUNK_VERTEX_COUNT) / 1024, 1, 1); // TODO: calculate based on surface chunk count
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		Renderer::UseShader(ShaderMode::ComputePhotonMappingCastRays);
		sceneCornellOriginal.EnableSceneModelMatrix();
		sceneCornellOriginal.BindSSBOs(0, 1, 2);

		waterPlane.EnableModelMatrix("surfaceM");
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, curDisplacementTex);
		Renderer::SetInt("surfaceDisplacementTex", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, curNormalTex);
		Renderer::SetInt("surfaceNormalTex", 1);
		Renderer::SetInt("surfacePatchCount", patchCount);
		waterPlane.BindSSBOs(3, 4, 5);

		DEBUG_DPM.BindSSBO(6);
		glDispatchCompute(DEBUG_PHOTON_SIZE_1, DEBUG_PHOTON_SIZE_2, 1);
		glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

		Renderer::UseShader(ShaderMode::Point);
		DEBUG_DPM.Render();
		// TODO: end test

		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
}

void ProcessKeyboard(GLFWwindow* window, float dt)
{
	float forward = 0.0f, right = 0.0f, up = 0.0f;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		forward += CAM_MOVE_SPEED * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		forward -= CAM_MOVE_SPEED * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		right += CAM_MOVE_SPEED * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		right -= CAM_MOVE_SPEED * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		up += CAM_MOVE_SPEED * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		up -= CAM_MOVE_SPEED * dt;
	}
	Renderer::TranslateCamera(forward, right, up);
}

void ProcessMouse(GLFWwindow* window, double posX, double posY)
{
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT))
	{
		Renderer::RotateCamera(CAM_ROTATE_SPEED * (lastY - posY), CAM_ROTATE_SPEED * (lastX - posX));
	}
	lastX = posX;
	lastY = posY;
}

void GenerateGerstnerWaves(int waveCount, float minAngle, float maxAngle, float minAmp, float maxAmp, float minK, float maxK,
				   float d, float l, float waveData[MAX_WAVE_COUNT * 2][4])
{
	// TODO: wave generation somewhere else
	static std::random_device randomDevice{};
	static std::mt19937 engine{ randomDevice() };

	if (minAngle > maxAngle) maxAngle += 360.0f;
	if (minAmp > maxAmp) std::swap(minAmp, maxAmp);
	if (minK > maxK) std::swap(minK, maxK);

	std::uniform_real_distribution<float> angleDist{ glm::radians(minAngle), glm::radians(maxAngle) };
	std::uniform_real_distribution<float> phaseShiftDist{ glm::radians(0.0f), glm::radians(360.0f) };
	std::uniform_real_distribution<float> ampDist{ minAmp, maxAmp };
	std::uniform_real_distribution<float> kDist{ minK, maxK };

	float baseOmega = glm::two_pi<float>();

	for (int i = 0; i < waveCount; i++)
	{
		float angle = angleDist(engine);
		float k = kDist(engine);
		waveData[i][0] = cos(angle);
		waveData[i][1] = sin(angle);
		waveData[i][2] = k;
		waveData[i][3] = ampDist(engine);
		//waveData[MAX_WAVE_COUNT + i][0] = sqrt(GRAVITY * k * tanh(k * d) * (1 + k * k * l * l));
		waveData[MAX_WAVE_COUNT + i][1] = phaseShiftDist(engine);

		float omegaSq = GRAVITY * k;
		if (d < DEPTH_INF)
		{
			omegaSq *= tanh(k * d);
		}
		if (l > TENSION_NONE)
		{
			omegaSq *= 1 + k * k * l * l;
		}
		waveData[MAX_WAVE_COUNT + i][0] = (int)(sqrt(omegaSq) / baseOmega) * baseOmega;
	}
}

void GenerateFourierWaves(float amplitude, float windSpeed, float windAngle, float smallWaveLen, unsigned int fourierGridSize, float patchSize)
{
	static std::random_device randomDevice{};
	static std::mt19937 engine{ randomDevice() };
	static std::normal_distribution<float> phillipsParamDist{ 0.0f, 1.0f };
	static float coordMult = glm::two_pi<float>() / 100.0f;

	int halfGridSize = fourierGridSize / 2;

	float windAngleRad = glm::radians(windAngle);
	glm::vec2 windDir{ cos(windAngleRad), sin(windAngleRad) };
	float L = windSpeed * windSpeed / GRAVITY;

	for (int i = 0; i < fourierGridSize; i++)
	{
		float kx = (i - halfGridSize) * coordMult;
		for (int j = 0; j < fourierGridSize; j++)
		{
			float ky = (j - halfGridSize) * coordMult;

			int index = i + MAX_FOURIER_GRID_SIZE * j;
			freqWaveData[index][0] = freqWaveData[index][1] = freqWaveData[index][2] = 0.0f;

			//glm::vec2 kVec{ (float)i / fourierGridSize - 0.5f, (float)j / fourierGridSize - 0.5f };
			glm::vec2 kVec{ kx, ky };
			float k = glm::length(kVec);
			if (k > 1e-8)
			{
				glm::vec2 kNorm = kVec / k;
				if (glm::dot(kNorm, windDir) < 0) continue;
				float kSq = k * k;
				float ampExp = amplitude * exp(-1.0f / (kSq * L * L)) * exp(-kSq * smallWaveLen * smallWaveLen);
				float sqrtPhOver2 = sqrt(ampExp) * glm::dot(kNorm, windDir) / (2 * kSq);
				freqWaveData[index][0] = phillipsParamDist(engine) * sqrtPhOver2;
				freqWaveData[index][1] = phillipsParamDist(engine) * sqrtPhOver2;
				freqWaveData[index][2] = sqrt(GRAVITY * k);
			}
		}
	}
}

void GenerateFourierCoordLookup(unsigned int fourierGridSize, int fourierPower)
{
	int halfGridSize = fourierGridSize / 2;

	// level 0 - bit reversal of index
	for (int i = 0; i < halfGridSize; i++)
	{
		// index = i + FOURIER_GRID_SIZE_HALF * 0
		fourierCoordLookup[i][0] = ReverseBits(i, fourierPower);
		fourierCoordLookup[i][1] = ReverseBits(i + halfGridSize, fourierPower);
	}
	// remaining levels
	int N = fourierGridSize, level = fourierPower;
	while (N > 1)
	{
		int k = 0, i = 0;
		while (k < fourierGridSize)
		{
			for (int j = 0; j < N / 2; i++, j++, k++)
			{
				int index = i + (MAX_FOURIER_GRID_SIZE / 2) * level;
				fourierCoordLookup[index][0] = k;
				fourierCoordLookup[index][1] = k + N / 2;
			}
			k += N / 2;
		}
		N /= 2;
		level--;
	}
}

int ReverseBits(int val, int digitCount)
{
	int res = 0;
	while (digitCount-- > 0)
	{
		res = (res << 1) | (val & 1);
		val >>= 1;
	}
	return res;
}

void Debug_WriteImage(unsigned int fourierGridSize)
{
	static const unsigned int COUNTER_LENGTH = 6;
	static int counter = 0;

	if (counter >= 1e6)
	{
		return;
	}

	for (int i = 0; i < fourierGridSize; i++)
	{
		for (int j = 0; j < fourierGridSize; j++)
		{
			for (int k = 0; k < 3; k++)
			{
				debugFreqWaveData[i + fourierGridSize * (j + fourierGridSize * k)] = k == 2 ? 0 : 128 + freqWaveData[i + fourierGridSize * j][k];
			}
		}
	}

	cimg_library::CImg<float> image{ debugFreqWaveData, fourierGridSize, fourierGridSize, 1, 3 };
	std::string counterStr = std::to_string(counter);
	std::string zeroStr = std::string( COUNTER_LENGTH - counterStr.length(), '0' );
	std::string name = std::string{ "../../debug/img/test_" }.append(zeroStr).append(counterStr).append(".tiff");
	image.save_tiff(name.c_str());

	counter++;
}