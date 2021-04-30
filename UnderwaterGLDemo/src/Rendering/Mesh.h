#pragma once

#include <vector>
#include <glad/glad.h>

class Mesh // TODO: template
{
private:
	GLuint vbo, vao;
	std::vector<float> vertices;

public:
	Mesh();
	void Render();
};