#include <iostream>
#include <random>
#include <vector>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Rendering/Mesh.h"
#include "Rendering/Shader.h"
#include "Rendering/Renderer.h"

const int WINDOW_WIDTH = 1600;
const int WINDOW_HEIGHT = 900;
const float SCENE_SIZE = 50.0f;
const float SCENE_HEIGHT = 30.0f;
const float CAM_MOVE_SPEED = 10.0f;
const float CAM_ROTATE_SPEED = 0.1f;

const int MAX_WAVE_COUNT = 100;
const int MIN_GRID_SIZE = 10;
const int MAX_GRID_SIZE = 2000;

const float DEPTH_INF = 100.0f;
const float TENSION_NONE = 0.0f;

float lastX = WINDOW_WIDTH / 2, lastY = WINDOW_HEIGHT / 2;

void ProcessKeyboard(GLFWwindow* window, float dt);
void ProcessMouse(GLFWwindow* window, double posX, double posY);
void GenerateWaves(int waveCount, float minAngle, float maxAngle, float minAmp, float maxAmp, float minK, float maxK,
				   float d, float l, float waveData[MAX_WAVE_COUNT * 2][4]);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
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
	ImGui_ImplOpenGL3_Init("#version 460"); // TODO: this version?

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
	float waveData[MAX_WAVE_COUNT * 2][4] = { 0 };
	GenerateWaves(waveCount, minAngle, maxAngle, minAmp, maxAmp, minK, maxK, d, l, waveData);

	unsigned int waveTex;
	glGenTextures(1, &waveTex);
	glBindTexture(GL_TEXTURE_2D, waveTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, MAX_WAVE_COUNT, 2, 0, GL_RGBA, GL_FLOAT, waveData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	float multTime = 1.0f;
	float lastTime = glfwGetTime(), simTime = 0;
	while (!glfwWindowShouldClose(window))
	{
		float t = glfwGetTime(), diffT = t - lastTime;
		ProcessKeyboard(window, diffT);
		lastTime = t;

		glClearColor(0.9f, 0.8f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		Renderer::UseShader(ShaderMode::Surface);
		// TODO
		//glBindTexture(GL_TEXTURE_2D, waveTex);

		ImGui::Begin("Menu");
		ImGui::Text("%.1f FPS, %.3f ms per frame", io.Framerate, 1000.0f / io.Framerate);
		ImGui::SliderFloat("Time multiplier", &multTime, 0.01f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

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
			GenerateWaves(waveCount, minAngle, maxAngle, minAmp, maxAmp, minK, maxK, d, l, waveData);
			//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, waveCount, 2, GL_RGBA, GL_FLOAT, waveData);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MAX_WAVE_COUNT, 2, GL_RGBA, GL_FLOAT, waveData); // TODO: why can't I sub only waveCount columns?
		}
		ImGui::SliderInt("Grid size", &gridVertexCount, MIN_GRID_SIZE, MAX_GRID_SIZE);
		if (ImGui::Button("Regenerate grid"))
		{
			waterPlane.Recreate(SCENE_SIZE, SCENE_SIZE, gridVertexCount, gridVertexCount);
		}
		if (ImGui::ColorEdit3("Surface color", waterColor))
		{
			waterPlane.SetColor(waterColor);
		}
		ImGui::End();

		Renderer::SetFloat("waveCount", waveCount);
		simTime += multTime * diffT;
		Renderer::SetFloat("t", simTime);
		waterPlane.Render();

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

void GenerateWaves(int waveCount, float minAngle, float maxAngle, float minAmp, float maxAmp, float minK, float maxK,
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

	float gravity = 9.8f;
	for (int i = 0; i < waveCount; i++)
	{
		float angle = angleDist(engine);
		float k = kDist(engine);
		waveData[i][0] = cos(angle);
		waveData[i][1] = sin(angle);
		waveData[i][2] = k;
		waveData[i][3] = ampDist(engine);
		//waveData[MAX_WAVE_COUNT + i][0] = sqrt(gravity * k * tanh(k * d) * (1 + k * k * l * l));
		waveData[MAX_WAVE_COUNT + i][1] = phaseShiftDist(engine);

		float omegaSq = gravity * k;
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