#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Rendering/Renderer.h"

#include "FourierSurface.h"

const float FourierSurface::K_COORD_MULT = glm::two_pi<float>() / 100.0f;

namespace
{
	int ReverseBits(int val, int digitCount);
}

FourierSurface::FourierSurface(float gravity)
{
	GenerateWaveData(gravity);
	RegenerateCoordLookup();

	unsigned int gridSize = GetLastGridSize();

	initFreqTex =
		Renderer::CreateTexture2D(MAX_GRID_SIZE, MAX_GRID_SIZE, GL_RGB32F, GL_RGB, GL_FLOAT, freqWaveData.data());
	curFreqTex =
		Renderer::CreateTexture2D(MAX_GRID_SIZE, MAX_GRID_SIZE, GL_RG32F, GL_RG, GL_FLOAT, nullptr);
	coordLookupTex =
		Renderer::CreateTexture2D(MAX_GRID_SIZE / 2, MAX_GRID_SIZE_POWER + 1, GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_INT, coordLookup.data());
	bufferTex1 =
		Renderer::CreateTexture2D(MAX_GRID_SIZE, MAX_GRID_SIZE, GL_RG32F, GL_RG, GL_FLOAT, nullptr);
	bufferTex2 =
		Renderer::CreateTexture2D(MAX_GRID_SIZE, MAX_GRID_SIZE, GL_RG32F, GL_RG, GL_FLOAT, nullptr);
	choppyBufferTex1 =
		Renderer::CreateTexture2D(MAX_GRID_SIZE, MAX_GRID_SIZE, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
	choppyBufferTex2 =
		Renderer::CreateTexture2D(MAX_GRID_SIZE, MAX_GRID_SIZE, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
	slopeBufferTex1 =
		Renderer::CreateTexture2D(MAX_GRID_SIZE, MAX_GRID_SIZE, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
	slopeBufferTex2 =
		Renderer::CreateTexture2D(MAX_GRID_SIZE, MAX_GRID_SIZE, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
	// these two need to be regenerated each time using the correct size
	displacementTex =
		Renderer::CreateTexture2D(gridSize, gridSize, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR, GL_REPEAT);
	normalTex =
		Renderer::CreateTexture2D(gridSize, gridSize, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR, GL_REPEAT);
}

void FourierSurface::RegenerateWaveData(float gravity)
{
	if (prevGridSizePower != gridSizePower)
	{
		prevGridSizePower = gridSizePower;

		unsigned int gridSize = GetNextGridSize();
		RegenerateCoordLookup();
		Renderer::SubTexture2DData(coordLookupTex, 0, 0, MAX_GRID_SIZE / 2, MAX_GRID_SIZE_POWER + 1,
								   GL_RG_INTEGER, GL_UNSIGNED_INT, coordLookup.data());
		displacementTex =
			Renderer::CreateTexture2D(gridSize, gridSize, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR, GL_REPEAT);
		normalTex =
			Renderer::CreateTexture2D(gridSize, gridSize, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr, GL_LINEAR, GL_REPEAT);
	}

	GenerateWaveData(gravity);
	Renderer::SubTexture2DData(initFreqTex, 0, 0, MAX_GRID_SIZE, MAX_GRID_SIZE, GL_RGB, GL_FLOAT, freqWaveData.data());
}

void FourierSurface::GenerateWaveData(float gravity)
{
	unsigned int gridSize = GetNextGridSize();
	int halfGridSize = gridSize / 2;

	float windAngleRad = glm::radians(windAngle);
	glm::vec2 windDir{ cos(windAngleRad), sin(windAngleRad) };
	float L = windSpeed * windSpeed / gravity;

	for (int i = 0; i < gridSize; i++)
	{
		float kx = (i - halfGridSize) * K_COORD_MULT;
		for (int j = 0; j < gridSize; j++)
		{
			float ky = (j - halfGridSize) * K_COORD_MULT;

			int index = 3 * (i + MAX_GRID_SIZE * j);
			freqWaveData[index + 0] = freqWaveData[index + 1] = freqWaveData[index + 2] = 0.0f;

			//glm::vec2 kVec{ (float)i / fourierGridSize - 0.5f, (float)j / fourierGridSize - 0.5f };
			glm::vec2 kVec{ kx, ky };
			float k = glm::length(kVec);
			if (k > 1e-8)
			{
				glm::vec2 kNorm = kVec / k;
				if (glm::dot(kNorm, windDir) < 0) continue;
				float kSq = k * k;
				float ampExp = frequencyAmplitude * exp(-1.0f / (kSq * L * L)) * exp(-kSq * smallWaveSize * smallWaveSize);
				float sqrtPhOver2 = sqrt(ampExp) * glm::dot(kNorm, windDir) / (2 * kSq);
				freqWaveData[index + 0] = phillipsParamDist(randomEngine) * sqrtPhOver2;
				freqWaveData[index + 1] = phillipsParamDist(randomEngine) * sqrtPhOver2;
				freqWaveData[index + 2] = sqrt(gravity * k);
			}
		}
	}
}

void FourierSurface::RegenerateCoordLookup()
{
	unsigned int gridSize = GetNextGridSize(), halfGridSize = gridSize / 2;

	// level 0 - bit reversal of index
	for (int i = 0; i < halfGridSize; i++)
	{
		int index = 2 * i;
		coordLookup[index + 0] = ReverseBits(i, gridSizePower);
		coordLookup[index + 1] = ReverseBits(i + halfGridSize, gridSizePower);
	}
	// remaining levels
	int N = gridSize, level = gridSizePower;
	while (N > 1)
	{
		int k = 0, i = 0;
		while (k < gridSize)
		{
			for (int j = 0; j < N / 2; i++, j++, k++)
			{
				int index = 2 * (i + (MAX_GRID_SIZE / 2) * level);
				coordLookup[index + 0] = k;
				coordLookup[index + 1] = k + N / 2;
			}
			k += N / 2;
		}
		N /= 2;
		level--;
	}
}

void FourierSurface::PrepareRender(float simTime, bool useDisplacement)
{
	unsigned int gridSize = GetLastGridSize();

	Renderer::UseShader(ShaderMode::ComputeFreqWave);
	Renderer::SetTexture2D(GL_TEXTURE0, "freqWaveTex", initFreqTex);
	Renderer::SetImage(0, "curFreqWaveTex", curFreqTex, GL_WRITE_ONLY, GL_RG32F);
	Renderer::SetUint("fourierGridSize", gridSize);
	Renderer::SetFloat("t", simTime);

	int workGroupCount = gridSize / COMPUTE_WORK_GROUP_SIZE;
	glDispatchCompute(workGroupCount, workGroupCount, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	Renderer::UseShader(ShaderMode::ComputeIFFTX);
	Renderer::SetTexture2D(GL_TEXTURE0, "coordLookupTex", coordLookupTex);
	Renderer::SetUint("fourierGridSize", gridSize);

	bool readFromFirst = true;
	GLuint readTex, writeTex, readChoppyTex, writeChoppyTex, readSlopeTex, writeSlopeTex;
	for (int level = 0, N = 1; N <= gridSize; level++, N *= 2)
	{
		if (readFromFirst)
		{
			readTex = level == 0 ? curFreqTex : bufferTex1;
			writeTex = bufferTex2;
			readChoppyTex = choppyBufferTex1;
			writeChoppyTex = choppyBufferTex2;
			readSlopeTex = slopeBufferTex1;
			writeSlopeTex = slopeBufferTex2;
		}
		else
		{
			readTex = bufferTex2;
			writeTex = bufferTex1;
			readChoppyTex = choppyBufferTex2;
			writeChoppyTex = choppyBufferTex1;
			readSlopeTex = slopeBufferTex2;
			writeSlopeTex = slopeBufferTex1;
		}
		readFromFirst = !readFromFirst;
		Renderer::SetImage(0, "readTex", readTex, GL_READ_ONLY, GL_RG32F);
		Renderer::SetImage(1, "writeTex", writeTex, GL_WRITE_ONLY, GL_RG32F);
		Renderer::SetImage(2, "readChoppyTex", readChoppyTex, GL_READ_ONLY, GL_RGBA32F);
		Renderer::SetImage(3, "writeChoppyTex", writeChoppyTex, GL_WRITE_ONLY, GL_RGBA32F);
		Renderer::SetImage(4, "readSlopeTex", readSlopeTex, GL_READ_ONLY, GL_RGBA32F);
		Renderer::SetImage(5, "writeSlopeTex", writeSlopeTex, GL_WRITE_ONLY, GL_RGBA32F);

		Renderer::SetUint("N", N);
		Renderer::SetUint("level", level);
		glDispatchCompute(workGroupCount / 2, workGroupCount, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	Renderer::UseShader(ShaderMode::ComputeIFFTY);
	Renderer::SetTexture2D(GL_TEXTURE0, "coordLookupTex", coordLookupTex);
	Renderer::SetUint("fourierGridSize", gridSize);
	for (int level = 0, N = 1; N <= gridSize; level++, N *= 2)
	{
		if (N == gridSize)
		{
			Renderer::UseShader(ShaderMode::ComputeIFFTYLastPass);
			Renderer::SetTexture2D(GL_TEXTURE0, "coordLookupTex", coordLookupTex);
		}
		if (readFromFirst)
		{
			readTex = bufferTex1;
			writeTex = bufferTex2;
			readChoppyTex = choppyBufferTex1;
			writeChoppyTex = choppyBufferTex2;
			readSlopeTex = slopeBufferTex1;
			writeSlopeTex = slopeBufferTex2;
		}
		else
		{
			readTex = bufferTex2;
			writeTex = bufferTex1;
			readChoppyTex = choppyBufferTex2;
			writeChoppyTex = choppyBufferTex1;
			readSlopeTex = slopeBufferTex2;
			writeSlopeTex = slopeBufferTex1;
		}
		readFromFirst = !readFromFirst;
		Renderer::SetImage(0, "readTex", readTex, GL_READ_ONLY, GL_RG32F);
		Renderer::SetImage(1, "writeTex", writeTex, GL_WRITE_ONLY, GL_RG32F);
		Renderer::SetImage(2, "readChoppyTex", readChoppyTex, GL_READ_ONLY, GL_RGBA32F);
		Renderer::SetImage(3, "writeChoppyTex", writeChoppyTex, GL_WRITE_ONLY, GL_RGBA32F);
		Renderer::SetImage(4, "readSlopeTex", readSlopeTex, GL_READ_ONLY, GL_RGBA32F);
		Renderer::SetImage(5, "writeSlopeTex", writeSlopeTex, GL_WRITE_ONLY, GL_RGBA32F);

		Renderer::SetUint("N", N);
		Renderer::SetUint("level", level);
		glDispatchCompute(workGroupCount, workGroupCount / 2, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	if (useSobelNormals)
		Renderer::UseShader(ShaderMode::ComputeNormalSobel);
	else
	{
		Renderer::UseShader(ShaderMode::ComputeNormalFourier);
		Renderer::SetImage(1, "slopeTex", readFromFirst ? slopeBufferTex1 : slopeBufferTex2, GL_READ_ONLY, GL_RGBA32F);
		Renderer::SetInt("slopeTex", 1);
	}
	Renderer::SetImage(0, "heightTex", readFromFirst ? bufferTex1 : bufferTex2, GL_READ_ONLY, GL_RG32F);
	Renderer::SetImage(2, "choppyTex", readFromFirst ? choppyBufferTex1 : choppyBufferTex2, GL_READ_ONLY, GL_RGBA32F);
	Renderer::SetImage(3, "displacementTex", displacementTex, GL_WRITE_ONLY, GL_RGBA32F);
	Renderer::SetImage(4, "normalTex", normalTex, GL_WRITE_ONLY, GL_RGBA32F);
	Renderer::SetUint("fourierGridSize", gridSize);
	glDispatchCompute(workGroupCount, workGroupCount, 1);

	Renderer::UseShader(useDisplacement ? ShaderMode::SurfaceDisplacement : ShaderMode::SurfaceHeight);
	Renderer::SetTexture2D(GL_TEXTURE0, "displacementTex", displacementTex);
	Renderer::SetTexture2D(GL_TEXTURE1, "normalTex", normalTex);
}

namespace
{
	int ReverseBits(int val, int digitCount)
	{
		int res = 0;
		while (digitCount-- > 0)
		{
			res = (res << 1) | (val & 1);
			val >>= 1;
		}
		return res;
	}
}