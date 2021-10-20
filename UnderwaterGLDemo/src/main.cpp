#include <iostream>
#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//#define cimg_use_tiff
//#include <CImg/CImg.h>

#include "Rendering/DynamicPointMesh.h"
#include "Rendering/Model.h"
#include "Rendering/Plane.h"
#include "Rendering/Scene.h"
#include "Rendering/Shader.h"
#include "Rendering/Renderer.h"

#include "Water/FourierSurface.h"
#include "Water/GerstnerSurface.h"

const unsigned int WINDOW_WIDTH = 1600;
const unsigned int WINDOW_HEIGHT = 900;
const unsigned int CHUNK_VERTEX_COUNT = 64;
const unsigned int MIN_GRID_COUNT = 10;
const unsigned int MAX_GRID_COUNT = 2000;
const float MIN_SURFACE_SIZE = 1.0f;
const float MAX_SURFACE_SIZE = 100.0f;
const float SURFACE_MOVE_BUFFER = 10.0f;
const float SCENE_HEIGHT = 30.0f;
const float CAM_MOVE_SPEED = 10.0f;
const float CAM_ROTATE_SPEED = 0.1f;
const unsigned int COMPUTE_CHUNK = 32;

const float GRAVITY = 9.8f;

float lastX = WINDOW_WIDTH / 2, lastY = WINDOW_HEIGHT / 2;

void ProcessKeyboard(GLFWwindow* window, float dt);
void ProcessMouse(GLFWwindow* window, double posX, double posY);

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
	Scene sceneCornellOriginal{ "assets/scenes/cornell/CornellBox-Sphere.obj" };

	// water things
	float waterColor[]{ 0.2f, 0.3f, 0.3f };
	glm::vec3 defWaterColor{ waterColor[0], waterColor[1], waterColor[2] };
	Material waterMat{ defWaterColor, glm::vec3{0.5f}, glm::vec3{0.5f}, 10.0f };
	int gridVertexCount = 512; // TODO: power
	int patchCountLevel = 1, patchCount = 2 * patchCountLevel - 1;
	float surfaceSize = 20.0f;
	Plane waterPlane = MakeXZPlane(waterMat, gridVertexCount, CHUNK_VERTEX_COUNT);
	waterPlane.SetScale(surfaceSize);

	GLuint surfaceBoundingBoxesTex =
		Renderer::CreateTexture2D(511, 2, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr); // TODO: replace 511 with max water size or sth

	GerstnerSurface gerstnerSurface{ GRAVITY };
	FourierSurface fourierSurface{ GRAVITY };
	BaseSurface* currentSurface;

	const int DEBUG_PHOTON_SIZE_1 = 10, DEBUG_PHOTON_SIZE_2 = 10;
	DynamicPointMesh DEBUG_DPM{ DEBUG_PHOTON_SIZE_1 * DEBUG_PHOTON_SIZE_2 * 1024, 5.0f, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f} };

	float timeMult = 1.0f;
	float lastTime = glfwGetTime(), simTime = 0;
	bool useDisplacement = false;
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
			currentSurface = &fourierSurface;

			if (ImGui::Button(fourierSurface.useSobelNormals ? "IFFT normals" : "Sobel normals"))
			{
				fourierSurface.useSobelNormals = !fourierSurface.useSobelNormals;
			}
			std::string fourierGridSizeString = std::to_string(fourierSurface.GetNextGridSize());
			ImGui::SliderInt("Fourier grid size", &fourierSurface.gridSizePower,
							 fourierSurface.MIN_GRID_SIZE_POWER, fourierSurface.MAX_GRID_SIZE_POWER,
							 fourierGridSizeString.c_str(), ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput);

			ImGui::SliderFloat("Frequency amplitude", &fourierSurface.frequencyAmplitude,
							   100.0f, 1000000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Wind speed", &fourierSurface.windSpeed,
							   0.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Wind angle", &fourierSurface.windAngle,
							   0.0f, 360.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Min wave length", &fourierSurface.smallWaveSize,
							   0.0f, 50.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::Button("Regenerate waves"))
			{
				fourierSurface.RegenerateWaveData(GRAVITY);
			}
		}
		else
		{
			// GERSTNER WAVES
			currentSurface = &gerstnerSurface;

			ImGui::SliderInt("Texture resolution",
							 &gerstnerSurface.textureResolution, gerstnerSurface.MIN_TEXTURE_RESOLUTION, gerstnerSurface.MAX_TEXTURE_RESOLUTION,
							 "%d", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderInt("Wave count", &gerstnerSurface.waveCount,
							 1, gerstnerSurface.MAX_WAVE_COUNT, "%d", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Min angle", &gerstnerSurface.minAngle,
							   0.0f, 360.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Max angle", &gerstnerSurface.maxAngle,
							   0.0f, 360.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Min amplitude", &gerstnerSurface.minAmplitude,
							   0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Max amplitude", &gerstnerSurface.maxAmplitude,
							   0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Min k", &gerstnerSurface.minK,
							   0.001f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp); // TODO: wavelength or wind speed instead of k
			ImGui::SliderFloat("Max k", &gerstnerSurface.maxK,
							   0.001f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Depth", &gerstnerSurface.depth,
							   0.01f, gerstnerSurface.DEPTH_INFINITE,
							   gerstnerSurface.depth == gerstnerSurface.DEPTH_INFINITE ? "Infinite" : "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Surface tension scale", &gerstnerSurface.surfaceTension,
							   gerstnerSurface.SURFACE_TENSION_NONE, 1.0f,
							   gerstnerSurface.surfaceTension == gerstnerSurface.SURFACE_TENSION_NONE ? "None" : "%.3f", ImGuiSliderFlags_AlwaysClamp);

			if (ImGui::Button("Regenerate waves"))
			{
				gerstnerSurface.RegenerateWaveData(GRAVITY);
			}
		}

		currentSurface->PrepareRender(simTime, useDisplacement);

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
		//unsigned int curDisplacementTex = useFourierWaves ? fourierDisplacementTex : gerstnerDisplacementTex;
		//unsigned int curNormalTex		= useFourierWaves ? fourierNormalTex : gerstnerNormalTex;

		//Renderer::UseShader(ShaderMode::ComputeSurfaceBoundingBoxes);
		//waterPlane.BindVertexSSBO(0);
		//waterPlane.BindChunkInfoSSBO(1);
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, curDisplacementTex);
		//Renderer::SetInt("displacementTex", 0);
		//
		//glDispatchCompute((CHUNK_VERTEX_COUNT * CHUNK_VERTEX_COUNT) / 1024, 1, 1); // TODO: calculate based on surface chunk count
		//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		//Renderer::UseShader(ShaderMode::ComputePhotonMappingCastRays);
		//sceneCornellOriginal.EnableSceneModelMatrix();
		//sceneCornellOriginal.BindSSBOs(0, 1, 2);

		//waterPlane.EnableModelMatrix("surfaceM");
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, curDisplacementTex);
		//Renderer::SetInt("surfaceDisplacementTex", 0);
		//glActiveTexture(GL_TEXTURE1);
		//glBindTexture(GL_TEXTURE_2D, curNormalTex);
		//Renderer::SetInt("surfaceNormalTex", 1);
		//Renderer::SetInt("surfacePatchCount", patchCount);
		//waterPlane.BindSSBOs(3, 4, 5);

		//DEBUG_DPM.BindSSBO(6);
		//glDispatchCompute(DEBUG_PHOTON_SIZE_1, DEBUG_PHOTON_SIZE_2, 1);
		//glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

		//Renderer::UseShader(ShaderMode::Point);
		//DEBUG_DPM.Render();
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
