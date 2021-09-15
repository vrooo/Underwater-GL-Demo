#include "Material.h"
#include "Renderer.h"

void Material::Set()
{
	Renderer::SetVec3("ambientColor", ambientColor);
	Renderer::SetVec3("diffuseColor", diffuseColor);
	Renderer::SetVec3("specularColor", specularColor);
	Renderer::SetFloat("specularHighlight", specularHighlight);
}