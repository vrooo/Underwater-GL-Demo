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

float lastX = WINDOW_WIDTH / 2, lastY = WINDOW_HEIGHT / 2;

void ProcessKeyboard(GLFWwindow* window, float dt);
void ProcessMouse(GLFWwindow* window, double posX, double posY);
void GenerateWaves(int waveCount, float waveData[MAX_WAVE_COUNT][4]);

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
	int gridVertexCount = 100;
	Plane waterPlane = Plane::MakeXZPlane(SCENE_SIZE, SCENE_SIZE, gridVertexCount, gridVertexCount, defWaterColor);

	int waveCount = 20, newWaveCount = 20;
	float waveData[MAX_WAVE_COUNT][4];
	GenerateWaves(waveCount, waveData);

	unsigned int waveTex;
	glGenTextures(1, &waveTex);
	glBindTexture(GL_TEXTURE_1D, waveTex);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, MAX_WAVE_COUNT, 0, GL_RGBA, GL_FLOAT, waveData);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	float lastT = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		float t = glfwGetTime();
		ProcessKeyboard(window, t - lastT);
		lastT = t;

		glClearColor(0.9f, 0.8f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		Renderer::UseShader(ShaderMode::Surface);
		// TODO
		//glBindTexture(GL_TEXTURE_1D, waveTex);
		Renderer::SetFloat("waveCount", waveCount);
		Renderer::SetFloat("t", t);
		waterPlane.Render();

		ImGui::Begin("Menu");
		ImGui::Text("%.1f FPS, %.3f ms per frame", io.Framerate, 1000.0f / io.Framerate);

		ImGui::SliderInt("Wave count", &newWaveCount, 1, MAX_WAVE_COUNT);
		if (ImGui::Button("Regenerate waves"))
		{
			waveCount = newWaveCount;
			GenerateWaves(waveCount, waveData);
			glTexSubImage1D(GL_TEXTURE_1D, 0, 0, waveCount, GL_RGBA, GL_FLOAT, waveData);
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

void GenerateWaves(int waveCount, float waveData[MAX_WAVE_COUNT][4])
{
	// TODO: wave generation somewhere else
	static std::random_device randomDevice{};
	static std::mt19937 engine{ randomDevice() };
	static std::uniform_real_distribution<float> waveDist{ -10.0f, 10.0f };
	static std::normal_distribution<float> amplDist{ 0.01f, 0.005f };

	// TODO: phase shift?
	float gravity = 9.8f;
	for (int i = 0; i < waveCount; i++)
	{
		glm::vec2 newWave{ waveDist(engine), waveDist(engine) };
		waveData[i][0] = newWave.x;
		waveData[i][1] = newWave.y;
		waveData[i][2] = amplDist(engine);
		waveData[i][3] = sqrt(gravity * newWave.length());
	}
}