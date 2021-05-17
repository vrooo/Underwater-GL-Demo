#include <iostream>
#include <random>
#include <vector>

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
const float SCENE_HEIGHT = 20.0f;
const float CAM_MOVE_SPEED = 10.0f;
const float CAM_ROTATE_SPEED = 0.1f;

const int WATER_GRID = 256;

float lastX = WINDOW_WIDTH / 2, lastY = WINDOW_HEIGHT / 2;

void ProcessKeyboard(GLFWwindow* window, float dt);
void ProcessMouse(GLFWwindow* window, double posX, double posY);

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

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "ERROR: Failed to initialize GLAD\n";
		return -1;
	}

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	Mesh plane = Mesh::MakeXZPlane(SCENE_SIZE, SCENE_SIZE, WATER_GRID, WATER_GRID);

	try
	{
		Renderer::Init(WINDOW_WIDTH, WINDOW_HEIGHT, glm::vec3{ SCENE_SIZE / 2, SCENE_HEIGHT, SCENE_SIZE / 2 });
	}
	catch(std::exception const& e)
	{
		std::cout << "ERROR: " << e.what() << "\n";
		return -1;
	}

	glfwSetCursorPosCallback(window, ProcessMouse);

	std::cout << "Initialization complete\n";

	// TODO: wave generation somewhere else
	std::random_device randomDevice{};
	std::mt19937 engine{ randomDevice() };
	std::uniform_real_distribution<float> waveDist{ -10.0f, 10.0f };
	std::normal_distribution<float> amplDist{ 0.002f, 0.0005f };

	int waveCount = 20;
	float gravity = 9.8f;
	std::vector<glm::vec2> waves{};
	std::vector<float> ampls{};
	std::vector<float> omegas{};
	for (int i = 0; i < waveCount; i++)
	{
		glm::vec2 newWave{ waveDist(engine), waveDist(engine) };
		waves.push_back(newWave);
		ampls.push_back(amplDist(engine));
		omegas.push_back(sqrt(gravity * newWave.length()));
	}

	float waveData[WATER_GRID][WATER_GRID] = { 0 };

	unsigned int waveTex;
	glGenTextures(1, &waveTex);
	glBindTexture(GL_TEXTURE_2D, waveTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, WATER_GRID, WATER_GRID, 0, GL_RED, GL_FLOAT, waveData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	float sceneStep = SCENE_SIZE / (WATER_GRID - 1);
	float lastT = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		float t = glfwGetTime();
		ProcessKeyboard(window, t - lastT);
		lastT = t;

		for (int i = 0; i < WATER_GRID; i++)
		{
			for (int j = 0; j < WATER_GRID; j++)
			{
				float x0 = -SCENE_SIZE + i * sceneStep, z0 = -SCENE_SIZE + j * sceneStep;
				waveData[i][j] = 0;
				for (int k = 0; k < waveCount; k++)
				{
					waveData[i][j] += ampls[k] * cos(waves[k].x * x0 + waves[k].y * z0 - omegas[k] * t);
				}
			}
		}
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WATER_GRID, WATER_GRID, GL_RED, GL_FLOAT, waveData);

		glClearColor(0.9f, 0.8f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		Renderer::UseShader(ShaderMode::Surface);
		// TODO
		//glBindTexture(GL_TEXTURE_2D, waveTex);
		plane.Render();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

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