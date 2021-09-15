#pragma once

#include <glm/glm.hpp>

struct Material
{
	glm::vec3 ambientColor  = { 1.0f, 0.0f, 1.0f }; // Ka
	glm::vec3 diffuseColor  = { 0.0f, 0.0f, 0.0f };	// Kd
	glm::vec3 specularColor = { 0.0f, 0.0f, 0.0f };	// Ks
	float specularHighlight = 1.0f;					// Ns
	// TODO: texture file, refraction index, transparency

	void Set();
};