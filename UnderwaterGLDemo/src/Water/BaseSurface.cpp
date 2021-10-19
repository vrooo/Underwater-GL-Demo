#include "../Rendering/Renderer.h"

#include "BaseSurface.h"

void BaseSurface::SetNormalTexture(const char* name, GLenum textureUnit)
{
	Renderer::SetTexture2D(name, textureUnit, normalTex);
}

void BaseSurface::SetDisplacementTexture(const char* name, GLenum textureUnit)
{
	Renderer::SetTexture2D(name, textureUnit, displacementTex);
}
