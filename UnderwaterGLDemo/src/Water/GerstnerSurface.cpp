#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Rendering/Renderer.h"

#include "GerstnerSurface.h"

const float GerstnerSurface::DEPTH_INFINITE = 100.0f;
const float GerstnerSurface::SURFACE_TENSION_NONE = 0.0f;

GerstnerSurface::GerstnerSurface(float gravity)
{
	GenerateWaveData(gravity);
	waveTex =
		Renderer::CreateTexture2D(MAX_WAVE_COUNT, 2, GL_RGBA32F, GL_RGBA, GL_FLOAT, waveData);
	// these two need to be regenerated each time using the correct size
	displacementTex =
		Renderer::CreateTexture2D(textureResolution, textureResolution, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR);
	normalTex =
		Renderer::CreateTexture2D(textureResolution, textureResolution, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR);
}

void GerstnerSurface::RegenerateWaveData(float gravity)
{
	GenerateWaveData(gravity);
	Renderer::BindTexture2D(GL_TEXTURE0, waveTex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MAX_WAVE_COUNT, 2, GL_RGBA, GL_FLOAT, waveData); // TODO: why can't I sub only waveCount columns?

	if (prevTextureResolution != textureResolution)
	{
		displacementTex =
			Renderer::CreateTexture2D(textureResolution, textureResolution, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR);
		normalTex =
			Renderer::CreateTexture2D(textureResolution, textureResolution, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR);
	}
	prevTextureResolution = textureResolution;
	prevWaveCount = waveCount;
}

void GerstnerSurface::PrepareRender(float simTime, bool useDisplacement)
{
	Renderer::UseShader(ShaderMode::ComputeGerstner);

	Renderer::SetImage("waveTex", waveTex, 0, GL_READ_ONLY, GL_RGBA32F);
	Renderer::SetImage("displacementTex", displacementTex, 1, GL_WRITE_ONLY, GL_RGBA32F);
	Renderer::SetImage("normalTex", normalTex, 2, GL_WRITE_ONLY, GL_RGBA32F);

	Renderer::SetInt("texResolution", prevTextureResolution);
	Renderer::SetInt("waveCount", prevWaveCount);
	Renderer::SetFloat("t", simTime);

	int gerstnerGroupSize = prevTextureResolution / COMPUTE_CHUNK;
	glDispatchCompute(gerstnerGroupSize, gerstnerGroupSize, 1);

	Renderer::UseShader(useDisplacement ? ShaderMode::SurfaceDisplacement : ShaderMode::SurfaceHeight);
	Renderer::SetTexture2D("displacementTex", GL_TEXTURE0, displacementTex);
	Renderer::SetTexture2D("normalTex", GL_TEXTURE1, normalTex);
}

void GerstnerSurface::GenerateWaveData(float gravity)
{
	if (minAngle > maxAngle) maxAngle += 360.0f;
	if (minAmplitude > maxAmplitude) std::swap(minAmplitude, maxAmplitude);
	if (minK > maxK) std::swap(minK, maxK);

	std::uniform_real_distribution<float> angleDist{ glm::radians(minAngle), glm::radians(maxAngle) };
	std::uniform_real_distribution<float> phaseShiftDist{ glm::radians(0.0f), glm::radians(360.0f) };
	std::uniform_real_distribution<float> ampDist{ minAmplitude, maxAmplitude };
	std::uniform_real_distribution<float> kDist{ minK, maxK };

	float baseOmega = glm::two_pi<float>();

	for (int i = 0; i < waveCount; i++)
	{
		float angle = angleDist(engine);
		float k = kDist(engine);
		waveData[i][0] = cos(angle);
		waveData[i][1] = sin(angle);
		waveData[i][2] = k;
		waveData[i][3] = ampDist(engine);
		waveData[MAX_WAVE_COUNT + i][1] = phaseShiftDist(engine);

		float omegaSq = gravity * k;
		if (depth < DEPTH_INFINITE)
		{
			omegaSq *= tanh(k * depth);
		}
		if (surfaceTension > SURFACE_TENSION_NONE)
		{
			omegaSq *= 1 + k * k * surfaceTension * surfaceTension;
		}
		waveData[MAX_WAVE_COUNT + i][0] = (int)(sqrt(omegaSq) / baseOmega) * baseOmega;
	}
}

void GerstnerSurface::SetWaveTexture(const char* name, GLenum textureUnit)
{
	Renderer::SetTexture2D(name, textureUnit, waveTex);
}
