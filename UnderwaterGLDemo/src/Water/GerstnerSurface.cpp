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
		Renderer::CreateTexture2D(MAX_WAVE_COUNT, 2, GL_RGBA32F, GL_RGBA, GL_FLOAT, waveData.data());
	// these two need to be regenerated each time using the correct size
	unsigned int textureResolution = GetNextTextureResolution();
	displacementTex =
		Renderer::CreateTexture2D(textureResolution, textureResolution, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR);
	normalTex =
		Renderer::CreateTexture2D(textureResolution, textureResolution, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR);
}

void GerstnerSurface::RegenerateWaveData(float gravity)
{
	GenerateWaveData(gravity);
	Renderer::SubTexture2DData(waveTex, 0, 0, MAX_WAVE_COUNT, 2, GL_RGBA, GL_FLOAT, waveData.data()); // TODO: why can't I sub only waveCount columns?

	if (prevTextureResolutionPower != textureResolutionPower)
	{
		prevTextureResolutionPower = textureResolutionPower;
		unsigned int textureResolution = GetNextTextureResolution();
		displacementTex =
			Renderer::CreateTexture2D(textureResolution, textureResolution, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR);
		normalTex =
			Renderer::CreateTexture2D(textureResolution, textureResolution, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR);
	}
	prevWaveCount = waveCount;
}

void GerstnerSurface::GenerateWaveData(float gravity)
{
	float curMinAngle = minAngle, curMaxAngle = maxAngle;
	float curMinAmplitude = minAmplitude, curMaxAmplitude = maxAmplitude;
	float curMinK = minK, curMaxK = maxK;

	if (curMinAngle > curMaxAngle) curMaxAngle += 360.0f;
	if (curMinAmplitude > curMaxAmplitude) std::swap(curMinAmplitude, curMaxAmplitude);
	if (curMinK > curMaxK) std::swap(curMinK, curMaxK);

	std::uniform_real_distribution<float> angleDist{ glm::radians(curMinAngle), glm::radians(curMaxAngle) };
	std::uniform_real_distribution<float> ampDist{ curMinAmplitude, curMaxAmplitude };
	std::uniform_real_distribution<float> kDist{ curMinK, curMaxK };

	float baseOmega = glm::two_pi<float>();

	for (int i = 0; i < waveCount; i++)
	{
		int index1 = 4 * i;
		int index2 = 4 * (MAX_WAVE_COUNT + i);
		float angle = angleDist(randomEngine);
		float k = kDist(randomEngine);
		waveData[index1 + 0] = cos(angle);
		waveData[index1 + 1] = sin(angle);
		waveData[index1 + 2] = k;
		waveData[index1 + 3] = ampDist(randomEngine);

		float omegaSq = gravity * k;
		if (depth < DEPTH_INFINITE)
		{
			omegaSq *= tanh(k * depth);
		}
		if (surfaceTension > SURFACE_TENSION_NONE)
		{
			omegaSq *= 1 + k * k * surfaceTension * surfaceTension;
		}
		waveData[index2 + 0] = (int)(sqrt(omegaSq) / baseOmega) * baseOmega;

		waveData[index2 + 1] = phaseShiftDist(randomEngine);
	}
}

void GerstnerSurface::PrepareRender(float simTime, bool useDisplacement)
{
	unsigned int prevTextureResolution = GetPrevTextureResolution();

	Renderer::UseShader(ShaderMode::ComputeGerstner);

	Renderer::SetImage(0, "waveTex", waveTex, GL_READ_ONLY, GL_RGBA32F);
	Renderer::SetImage(1, "displacementTex", displacementTex, GL_WRITE_ONLY, GL_RGBA32F);
	Renderer::SetImage(2, "normalTex", normalTex, GL_WRITE_ONLY, GL_RGBA32F);

	Renderer::SetInt("texResolution", prevTextureResolution);
	Renderer::SetInt("waveCount", prevWaveCount);
	Renderer::SetFloat("t", simTime);

	int workGroupCount = prevTextureResolution / COMPUTE_WORK_GROUP_SIZE;
	glDispatchCompute(workGroupCount, workGroupCount, 1);

	Renderer::UseShader(useDisplacement ? ShaderMode::SurfaceDisplacement : ShaderMode::SurfaceHeight);
	Renderer::SetTexture2D(GL_TEXTURE0, "displacementTex", displacementTex);
	Renderer::SetTexture2D(GL_TEXTURE1, "normalTex", normalTex);
}