#include "../Rendering/Renderer.h"

#include "BaseSurface.h"

BaseSurface::BaseSurface()
{
	std::random_device randomDevice{};
	randomEngine = std::mt19937{ randomDevice() };
}

void BaseSurface::SetNormalTexture(GLenum textureUnit, const char* name)
{
	Renderer::SetTexture2D(textureUnit, name, normalTex);
}

void BaseSurface::SetDisplacementTexture(GLenum textureUnit, const char* name)
{
	Renderer::SetTexture2D(textureUnit, name, displacementTex);
}
