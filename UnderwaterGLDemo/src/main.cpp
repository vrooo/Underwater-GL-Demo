#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Rendering/Mesh.h"
#include "Rendering/Shader.h"

const int WINDOW_WIDTH = 1600;
const int WINDOW_HEIGHT = 900;

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

	Mesh mesh{};

	// TODO: move shader creation to renderer
#pragma region Shader creation
	Shader passShader{"assets/shaders/pass.vert", "assets/shaders/pass.frag"};
#pragma endregion

	std::cout << "Initialization completed\n";

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.3f, 0.4f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		passShader.Use();
		mesh.Render();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}