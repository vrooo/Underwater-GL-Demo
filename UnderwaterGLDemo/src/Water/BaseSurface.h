#pragma once

#include <random>

#include <glad/glad.h>

class BaseSurface
{
protected:
	std::mt19937 randomEngine;

	GLuint normalTex = 0, displacementTex = 0;

	BaseSurface();
public:
	void SetNormalTexture(GLenum textureUnit, const char* name);
	void SetDisplacementTexture(GLenum textureUnit, const char* name);

	virtual void PrepareRender(float simTime, bool useDisplacement) = 0;
};