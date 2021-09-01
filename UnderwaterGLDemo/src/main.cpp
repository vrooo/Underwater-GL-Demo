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

#include "Rendering/Mesh.h"
#include "Rendering/Shader.h"
#include "Rendering/Renderer.h"

#define SAVE_FOURIER_DEBUG false

const unsigned int WINDOW_WIDTH = 1600;
const unsigned int WINDOW_HEIGHT = 900;
const float SCENE_SIZE = 50.0f;
const float SCENE_HEIGHT = 30.0f;
const float CAM_MOVE_SPEED = 10.0f;
const float CAM_ROTATE_SPEED = 0.1f;

const unsigned int MAX_WAVE_COUNT = 100;
const unsigned int MIN_GRID_SIZE = 10;
const unsigned int MAX_GRID_SIZE = 2000;
const unsigned int FOURIER_GRID_SIZE = 512;
const unsigned int FOURIER_GRID_SIZE_HALF = FOURIER_GRID_SIZE / 2;
const unsigned int FOURIER_DIGIT_COUNT = 9; // log2(FOURIER_GRID_SIZE)
const unsigned int FOURIER_COMPUTE_CHUNK = 32;
const unsigned int FOURIER_GROUP_SIZE = FOURIER_GRID_SIZE / FOURIER_COMPUTE_CHUNK;

const float DEPTH_INF = 100.0f;
const float TENSION_NONE = 0.0f;

const float GRAVITY = 9.8f;

float lastX = WINDOW_WIDTH / 2, lastY = WINDOW_HEIGHT / 2;
float gerstnerWaveData[MAX_WAVE_COUNT * 2][4];
float freqWaveData[FOURIER_GRID_SIZE * FOURIER_GRID_SIZE][3];
float debugFreqWaveData[FOURIER_GRID_SIZE * FOURIER_GRID_SIZE * 3];
unsigned int fourierCoordLookup[FOURIER_GRID_SIZE_HALF * (FOURIER_DIGIT_COUNT + 1)][2];

void ProcessKeyboard(GLFWwindow* window, float dt);
void ProcessMouse(GLFWwindow* window, double posX, double posY);
void GenerateGerstnerWaves(int waveCount, float minAngle, float maxAngle, float minAmp, float maxAmp, float minK, float maxK,
				   float d, float l, float waveData[MAX_WAVE_COUNT * 2][4]);
void GenerateFourierWaves(float amplitude, float windSpeed, float windAngle, float smallWaveLen, float freqWaveData[FOURIER_GRID_SIZE * FOURIER_GRID_SIZE][3]);
void GenerateFourierCoordLookup(unsigned int coordLookup[FOURIER_GRID_SIZE_HALF * (FOURIER_DIGIT_COUNT + 1)][2]);
int ReverseBits(int val, int digitCount);
void Debug_WriteImage();

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
		Renderer::Init(WINDOW_WIDTH, WINDOW_HEIGHT, glm::vec3{ SCENE_SIZE / 2, SCENE_HEIGHT, SCENE_SIZE / 2 });
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

	float waterColor[]{ 0.2f, 0.3f, 0.3f };
	glm::vec3 defWaterColor{ waterColor[0], waterColor[1], waterColor[2] };
	int gridVertexCount = 500;
	Plane waterPlane = Plane::MakeXZPlane(SCENE_SIZE, SCENE_SIZE, gridVertexCount, gridVertexCount, defWaterColor);

	int waveCount = 20, newWaveCount = 20;
	float minAngle = 0.0f, maxAngle = 360.0f;
	float minAmp = 0.001f, maxAmp = 0.02f;
	float minK = 1.0f, maxK = 5.0f; // TODO: maybe wavelength instead?
	float d = 10.0f, l = 0.0f;
	GenerateGerstnerWaves(waveCount, minAngle, maxAngle, minAmp, maxAmp, minK, maxK, d, l, gerstnerWaveData);

	unsigned int waveTex =
		Renderer::CreateTexture2D(MAX_WAVE_COUNT, 2, GL_RGBA32F, GL_RGBA, GL_FLOAT, gerstnerWaveData);

	float fourierAmp = 0.1f;
	float fourierWindSpeed = 30.0f, fourierWindAngle = 90.0f;
	float fourierSmallWave = 2.0f;
	GenerateFourierWaves(fourierAmp, fourierWindSpeed, fourierWindAngle, fourierSmallWave, freqWaveData);
	if (SAVE_FOURIER_DEBUG)
	{
		Debug_WriteImage();
	}
	GenerateFourierCoordLookup(fourierCoordLookup);

	unsigned int initFreqWaveTex =
		Renderer::CreateTexture2D(FOURIER_GRID_SIZE, FOURIER_GRID_SIZE, GL_RGB32F, GL_RGB, GL_FLOAT, freqWaveData);
	unsigned int curFreqWaveTex =
		Renderer::CreateTexture2D(FOURIER_GRID_SIZE, FOURIER_GRID_SIZE, GL_RG32F, GL_RG, GL_FLOAT, nullptr);
	unsigned int fourierCoordLookupTex =
		Renderer::CreateTexture2D(FOURIER_GRID_SIZE_HALF, FOURIER_DIGIT_COUNT + 1, GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_INT, fourierCoordLookup);
	unsigned int fourierBufferTex1 =
		Renderer::CreateTexture2D(FOURIER_GRID_SIZE, FOURIER_GRID_SIZE, GL_RG32F, GL_RG, GL_FLOAT, nullptr);
	unsigned int fourierBufferTex2 =
		Renderer::CreateTexture2D(FOURIER_GRID_SIZE, FOURIER_GRID_SIZE, GL_RG32F, GL_RG, GL_FLOAT, nullptr);
	unsigned int fourierNormalHeightTex =
		Renderer::CreateTexture2D(FOURIER_GRID_SIZE, FOURIER_GRID_SIZE, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);


	float timeMult = 1.0f;
	float lastTime = glfwGetTime(), simTime = 0;
	bool useFourierWaves = false;
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

		ImGui::SliderInt("Grid size", &gridVertexCount, MIN_GRID_SIZE, MAX_GRID_SIZE);
		if (ImGui::Button("Regenerate grid"))
		{
			waterPlane.Recreate(SCENE_SIZE, SCENE_SIZE, gridVertexCount, gridVertexCount);
		}
		if (ImGui::ColorEdit3("Surface color", waterColor))
		{
			waterPlane.SetColor(waterColor);
		}

		if (ImGui::Button(useFourierWaves ? "Gerstner waves" : "Fourier waves"))
		{
			useFourierWaves = !useFourierWaves;
		}

		if (useFourierWaves)
		{
			// FOURIER WAVES
			Renderer::UseShader(ShaderMode::ComputeFreqWave);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, initFreqWaveTex);
			Renderer::SetInt("freqWaveTex", 0);

			ImGui::SliderFloat("Amplitude", &fourierAmp, 0.01f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Wind speed", &fourierWindSpeed, 0.0f, 500.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Wind angle", &fourierWindAngle, 0.0f, 360.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Min wave length", &fourierSmallWave, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::Button("Regenerate waves"))
			{
				GenerateFourierWaves(fourierAmp, fourierWindSpeed, fourierWindAngle, fourierSmallWave, freqWaveData);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FOURIER_GRID_SIZE, FOURIER_GRID_SIZE, GL_RGB, GL_FLOAT, freqWaveData);
			}

			glBindImageTexture(0, curFreqWaveTex, 0, true, 0, GL_WRITE_ONLY, GL_RG32F);
			Renderer::SetInt("curFreqWaveTex", 0);
			Renderer::SetFloat("t", simTime);
			Renderer::SetFloat("fourierGridSizeFloat", FOURIER_GRID_SIZE);
			glDispatchCompute(FOURIER_GROUP_SIZE, FOURIER_GROUP_SIZE, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			if (SAVE_FOURIER_DEBUG)
			{
				glBindTexture(GL_TEXTURE_2D, curFreqWaveTex);
				glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, freqWaveData);
				Debug_WriteImage();
			}

			Renderer::UseShader(ShaderMode::ComputeIFFTX);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fourierCoordLookupTex);
			Renderer::SetInt("coordLookupTex", 0);
			Renderer::SetUint("fourierGridSize", FOURIER_GRID_SIZE);
			bool readFromFirst = true;
			for (int level = 0, N = 1; N <= FOURIER_GRID_SIZE; level++, N *= 2)
			{
				if (readFromFirst)
				{
					if (level == 0)
					{
						glBindImageTexture(0, curFreqWaveTex, 0, true, 0, GL_READ_ONLY, GL_RG32F);
					}
					else
					{
						glBindImageTexture(0, fourierBufferTex1, 0, true, 0, GL_READ_ONLY, GL_RG32F);
					}
					glBindImageTexture(1, fourierBufferTex2, 0, true, 0, GL_WRITE_ONLY, GL_RG32F);
				}
				else
				{
					glBindImageTexture(0, fourierBufferTex2, 0, true, 0, GL_READ_ONLY, GL_RG32F);
					glBindImageTexture(1, fourierBufferTex1, 0, true, 0, GL_WRITE_ONLY, GL_RG32F);
				}
				readFromFirst = !readFromFirst;
				Renderer::SetInt("readTex", 0);
				Renderer::SetInt("writeTex", 1);
				Renderer::SetUint("N", N);
				Renderer::SetUint("level", level);
				glDispatchCompute(FOURIER_GROUP_SIZE / 2, FOURIER_GROUP_SIZE, 1);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
			}

			Renderer::UseShader(ShaderMode::ComputeIFFTY);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fourierCoordLookupTex);
			Renderer::SetInt("coordLookupTex", 0);
			Renderer::SetUint("fourierGridSize", FOURIER_GRID_SIZE);
			for (int level = 0, N = 1; N <= FOURIER_GRID_SIZE; level++, N *= 2)
			{
				if (N == FOURIER_GRID_SIZE)
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
				}
				else
				{
					glBindImageTexture(0, fourierBufferTex2, 0, true, 0, GL_READ_ONLY, GL_RG32F);
					glBindImageTexture(1, fourierBufferTex1, 0, true, 0, GL_WRITE_ONLY, GL_RG32F);
				}
				readFromFirst = !readFromFirst;
				Renderer::SetInt("readTex", 0);
				Renderer::SetInt("writeTex", 1);
				Renderer::SetUint("N", N);
				Renderer::SetUint("level", level);
				glDispatchCompute(FOURIER_GROUP_SIZE, FOURIER_GROUP_SIZE / 2, 1);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
			}
			Renderer::UseShader(ShaderMode::ComputeNormal);
			glBindImageTexture(0, readFromFirst ? fourierBufferTex1 : fourierBufferTex2, 0, true, 0, GL_READ_ONLY, GL_RG32F);
			glBindImageTexture(1, fourierNormalHeightTex, 0, true, 0, GL_WRITE_ONLY, GL_RGBA32F);
			Renderer::SetInt("heightTex", 0);
			Renderer::SetInt("normalHeightTex", 1);
			Renderer::SetUint("fourierGridSize", FOURIER_GRID_SIZE);
			glDispatchCompute(FOURIER_GROUP_SIZE, FOURIER_GROUP_SIZE, 1);

			Renderer::UseShader(ShaderMode::SurfaceHeightMap);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fourierNormalHeightTex);
			Renderer::SetInt("normalHeightTex", 0);
		}
		else
		{
			// GERSTNER WAVES
			Renderer::UseShader(ShaderMode::SurfaceGerstner);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, waveTex);
			Renderer::SetInt("waveTex", 0);

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
			}

			Renderer::SetFloat("waveCount", waveCount);
			Renderer::SetFloat("t", simTime);
		}
		waterPlane.Render();

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
		Renderer::RotateCamera(CAM_ROTATE_SPEED * (lastY - posY), CAM_ROTATE_SPEED * (posX - lastX));
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

	if (minAngle > maxAngle) std::swap(minAngle, maxAngle);
	if (minAmp > maxAmp) std::swap(minAmp, maxAmp);
	if (minK > maxK) std::swap(minK, maxK);

	std::uniform_real_distribution<float> angleDist{ glm::radians(minAngle), glm::radians(maxAngle) };
	std::uniform_real_distribution<float> phaseShiftDist{ glm::radians(0.0f), glm::radians(360.0f) };
	std::uniform_real_distribution<float> ampDist{ minAmp, maxAmp };
	std::uniform_real_distribution<float> kDist{ minK, maxK };

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
		waveData[MAX_WAVE_COUNT + i][0] = sqrt(omegaSq);
	}
}

void GenerateFourierWaves(float amplitude, float windSpeed, float windAngle, float smallWaveLen, float freqWaveData[FOURIER_GRID_SIZE * FOURIER_GRID_SIZE][3])
{
	static std::random_device randomDevice{};
	static std::mt19937 engine{ randomDevice() };
	static std::normal_distribution<float> phillipsParamDist{ 0.0f, 1.0f };

	float windAngleRad = glm::radians(windAngle);
	glm::vec2 windDir{ cos(windAngleRad), sin(windAngleRad) };
	float L = windSpeed * windSpeed / GRAVITY;

	for (int i = 0; i < FOURIER_GRID_SIZE; i++)
	{
		for (int j = 0; j < FOURIER_GRID_SIZE; j++)
		{
			int index = i + FOURIER_GRID_SIZE * j;
			if (i == FOURIER_GRID_SIZE_HALF && j == FOURIER_GRID_SIZE_HALF)
			{
				freqWaveData[index][0] = freqWaveData[index][1] = freqWaveData[index][2] = 0.0f;
			}
			else
			{
				glm::vec2 kVec{ (float)i / FOURIER_GRID_SIZE - 0.5f, (float)j / FOURIER_GRID_SIZE - 0.5f }, kNorm = glm::normalize(kVec);
				float kSq = glm::dot(kVec, kVec), k = sqrt(kSq);
				float ampExp = amplitude * exp(-1.0f / (kSq * L * L)) * exp(-kSq * smallWaveLen * smallWaveLen);
				float sqrtPhOver2 = sqrt(ampExp) * glm::dot(kNorm, windDir) / (2 * kSq);
				freqWaveData[index][0] = phillipsParamDist(engine) * sqrtPhOver2;
				freqWaveData[index][1] = phillipsParamDist(engine) * sqrtPhOver2;
				freqWaveData[index][2] = sqrt(GRAVITY * k);
				// TODO: debug stuff
				/*if (kNorm.x < 0 || kNorm.y < 0)
				{
					freqWaveData[index][0] = freqWaveData[index][1] = freqWaveData[index][2] = 0;
				}*/
			}
		}
	}
}

void GenerateFourierCoordLookup(unsigned int coordLookup[FOURIER_GRID_SIZE_HALF * (FOURIER_DIGIT_COUNT + 1)][2])
{
	// level 0 - bit reversal of index
	for (int i = 0; i < FOURIER_GRID_SIZE_HALF; i++)
	{
		// index = i + FOURIER_GRID_SIZE_HALF * 0
		coordLookup[i][0] = ReverseBits(i, FOURIER_DIGIT_COUNT);
		coordLookup[i][1] = ReverseBits(i + FOURIER_GRID_SIZE_HALF, FOURIER_DIGIT_COUNT);
	}
	// remaining levels
	int N = FOURIER_GRID_SIZE, level = FOURIER_DIGIT_COUNT;
	while (N > 1)
	{
		int k = 0, i = 0;
		while (k < FOURIER_GRID_SIZE)
		{
			for (int j = 0; j < N / 2; i++, j++, k++)
			{
				int index = i + FOURIER_GRID_SIZE_HALF * level;
				coordLookup[index][0] = k;
				coordLookup[index][1] = k + N / 2;
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

void Debug_WriteImage()
{
	static const unsigned int COUNTER_LENGTH = 6;
	static int counter = 0;

	if (counter >= 1e6)
	{
		return;
	}

	for (int i = 0; i < FOURIER_GRID_SIZE; i++)
	{
		for (int j = 0; j < FOURIER_GRID_SIZE; j++)
		{
			for (int k = 0; k < 3; k++)
			{
				debugFreqWaveData[i + FOURIER_GRID_SIZE * (j + FOURIER_GRID_SIZE * k)] = k == 2 ? 0 : 128 + freqWaveData[i + FOURIER_GRID_SIZE * j][k];
			}
		}
	}

	cimg_library::CImg<float> image{ debugFreqWaveData, FOURIER_GRID_SIZE, FOURIER_GRID_SIZE, 1, 3 };
	std::string counterStr = std::to_string(counter);
	std::string zeroStr = std::string( COUNTER_LENGTH - counterStr.length(), '0' );
	std::string name = std::string{ "../../debug/img/test_" }.append(zeroStr).append(counterStr).append(".tiff");
	image.save_tiff(name.c_str());

	counter++;
}