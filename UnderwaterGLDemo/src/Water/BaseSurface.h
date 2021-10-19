#pragma once

#include <glad/glad.h>

class BaseSurface
{
protected:
	GLuint normalTex, displacementTex;
public:
	void SetNormalTexture(const char* name, GLenum textureUnit);
	void SetDisplacementTexture(const char* name, GLenum textureUnit);
};