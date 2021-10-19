#pragma once

#include <random>

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
	static const int COMPUTE_CHUNK = 32;

	int prevWaveCount = 20;
	int prevTextureResolution = 512;

	std::random_device randomDevice{};
	std::mt19937 engine{ randomDevice() };

	GLuint waveTex;
	float waveData[MAX_WAVE_COUNT * 2][4] = { 0 };
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
	void PrepareRender(float simTime, bool useDisplacement);
	void SetWaveTexture(const char* name, GLenum textureUnit);
};