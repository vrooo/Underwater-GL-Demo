#pragma once

#include <vector>

#include "BaseSurface.h"

class GerstnerSurface : public BaseSurface
{
public:
	static const int MAX_WAVE_COUNT = 100;
	static const int MIN_TEXTURE_RESOLUTION = 32;
	static const int MAX_TEXTURE_RESOLUTION = 2048;

	static const float DEPTH_INFINITE;
	static const float SURFACE_TENSION_NONE;

private:
	static const int COMPUTE_WORK_GROUP_SIZE = 32;

	int prevWaveCount = 20;
	int prevTextureResolution = 512;

	std::uniform_real_distribution<float> phaseShiftDist{ glm::radians(0.0f), glm::radians(360.0f) };

	GLuint waveTex;
	std::vector<float> waveData = std::vector<float>(MAX_WAVE_COUNT * 2 * 4);
	void GenerateWaveData(float gravity);

public:
	int waveCount = 20;
	int textureResolution = 512;
	float minAngle = 110.0f, maxAngle = 120.0f;
	float minAmplitude = 0.001f, maxAmplitude = 0.004f;
	float minK = 1.0f, maxK = 30.0f;
	float depth = 10.0f;
	float surfaceTension = 0.0f;

	GerstnerSurface(float gravity);
	void RegenerateWaveData(float gravity);

	void PrepareRender(float simTime, bool useDisplacement) override;
};