#pragma once

#include <vector>

#include "BaseSurface.h"

class FourierSurface : public BaseSurface
{
public:
	static const unsigned int MIN_GRID_SIZE_POWER = 6;
	static const unsigned int MAX_GRID_SIZE_POWER = 11;
	static const unsigned int MAX_GRID_SIZE = 1 << MAX_GRID_SIZE_POWER;

private:
	static const int COMPUTE_WORK_GROUP_SIZE = 32;
	static const float K_COORD_MULT;

	int prevGridSizePower = 9;

	GLuint initFreqTex, curFreqTex;
	GLuint coordLookupTex;
	GLuint bufferTex1, bufferTex2;
	GLuint choppyBufferTex1, choppyBufferTex2;
	GLuint slopeBufferTex1, slopeBufferTex2;

	std::normal_distribution<float> phillipsParamDist{ 0.0f, 1.0f };

	std::vector<float> freqWaveData = std::vector<float>(MAX_GRID_SIZE * MAX_GRID_SIZE * 3);
	std::vector<unsigned int> coordLookup = std::vector<unsigned int>((MAX_GRID_SIZE / 2)* (MAX_GRID_SIZE_POWER + 1) * 2);

	void GenerateWaveData(float gravity);

public:
	float frequencyAmplitude = 500.0f;
	float windSpeed = 100.0f;
	float windAngle = 135.0f;
	float smallWaveSize = 0.01f;
	int gridSizePower = 9;
	bool useSobelNormals = true;

	FourierSurface(float gravity);
	void RegenerateWaveData(float gravity);
	void RegenerateCoordLookup();

	void PrepareRender(float simTime, bool useDisplacement) override;

	inline unsigned int GetNextGridSize() { return 1 << gridSizePower; }
	inline unsigned int GetPrevGridSize() { return 1 << prevGridSizePower; }
};