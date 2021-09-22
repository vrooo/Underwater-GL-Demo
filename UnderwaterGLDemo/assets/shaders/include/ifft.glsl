const float two_pi = 6.28318531f;
const float k_coord_mult = two_pi / 100.0f;

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

uniform usampler2D coordLookupTex;
layout (rg32f) uniform readonly image2D readTex;
layout (rg32f) uniform writeonly image2D writeTex;
layout (rgba32f) uniform readonly image2D readChoppyTex;
layout (rgba32f) uniform writeonly image2D writeChoppyTex;
layout (rgba32f) uniform readonly image2D readSlopeTex;
layout (rgba32f) uniform writeonly image2D writeSlopeTex;
uniform uint fourierGridSize;
uniform uint level;
uniform uint N;

vec2 getK(ivec2 pixelCoord)
{
	//return pixelCoord / float(fourierGridSize) - 0.5f;
	return (pixelCoord - int(fourierGridSize) / 2) * k_coord_mult;
}

vec2 conjAndScale(vec2 v)
{
	return vec2(v.x, -v.y) / N;
}

vec2 twiddle(vec2 q, uint m)
{
	float arg = -(two_pi * m) / N;
	float sinarg = sin(arg), cosarg = cos(arg);
	return vec2(q.x * cosarg + q.y * sinarg, q.y * cosarg - q.x * sinarg);
}

vec4 combinePixels(vec2 pixel1, vec2 pixel2, int index)
{
	uint m = index % (N / 2);
	vec2 p = pixel1, q = twiddle(pixel2, m);
	return vec4(p + q, p - q);
}

void doFirstPass(ivec2 pixelCoord1, ivec2 pixelCoord2, ivec2 writeCoord1, ivec2 writeCoord2)
{
	vec2 pixel1 = imageLoad(readTex, pixelCoord1).rg;
	vec2 pixel2 = imageLoad(readTex, pixelCoord2).rg;

	vec2 k1 = getK(pixelCoord1), k2 = getK(pixelCoord2);
	vec2 k1norm = normalize(k1), k2norm = normalize(k2);
	if (isnan(k1norm.x)) k1norm = vec2(0.0f);
	if (isnan(k2norm.x)) k2norm = vec2(0.0f);

	// write from pixelCoordK to writeCoordK (conjugated)
	imageStore(writeTex, writeCoord1, vec4(pixel1.r, -pixel1.g, 0.0f, 1.0f));
	imageStore(writeTex, writeCoord2, vec4(pixel2.r, -pixel2.g, 0.0f, 1.0f));
	imageStore(writeChoppyTex, writeCoord1, vec4(k1norm.x * pixel1.g, k1norm.x * pixel1.r, k1norm.y * pixel1.g, k1norm.y * pixel1.r));
	imageStore(writeChoppyTex, writeCoord2, vec4(k2norm.x * pixel2.g, k2norm.x * pixel2.r, k2norm.y * pixel2.g, k2norm.y * pixel2.r));
	imageStore(writeSlopeTex, writeCoord1, vec4(-k1.x * pixel1.g, -k1.x * pixel1.r, -k1.y * pixel1.g, -k1.y * pixel1.r));
	imageStore(writeSlopeTex, writeCoord2, vec4(-k2.x * pixel2.g, -k2.x * pixel2.r, -k2.y * pixel2.g, -k2.y * pixel2.r));
}

void doPass(ivec2 pixelCoord1, ivec2 pixelCoord2, int index)
{
	vec2 pixel1 = imageLoad(readTex, pixelCoord1).rg;
	vec2 pixel2 = imageLoad(readTex, pixelCoord2).rg;
	vec4 pixelChoppy1 = imageLoad(readChoppyTex, pixelCoord1);
	vec4 pixelChoppy2 = imageLoad(readChoppyTex, pixelCoord2);
	vec4 pixelSlope1 = imageLoad(readSlopeTex, pixelCoord1);
	vec4 pixelSlope2 = imageLoad(readSlopeTex, pixelCoord2);

	vec4 res = combinePixels(pixel1, pixel2, index);
	vec2 res1 = res.xy, res2 = res.zw;

	vec4 resChoppyX = combinePixels(pixelChoppy1.xy, pixelChoppy2.xy, index);
	vec2 resChoppyX1 = resChoppyX.xy, resChoppyX2 = resChoppyX.zw;
	vec4 resChoppyY = combinePixels(pixelChoppy1.zw, pixelChoppy2.zw, index);
	vec2 resChoppyY1 = resChoppyY.xy, resChoppyY2 = resChoppyY.zw;

	vec4 resSlopeX = combinePixels(pixelSlope1.xy, pixelSlope2.xy, index);
	vec2 resSlopeX1 = resSlopeX.xy, resSlopeX2 = resSlopeX.zw;
	vec4 resSlopeY = combinePixels(pixelSlope1.zw, pixelSlope2.zw, index);
	vec2 resSlopeY1 = resSlopeY.xy, resSlopeY2 = resSlopeY.zw;
		
	if (N == fourierGridSize)
	{
		// conjugate and scale
		res1 = conjAndScale(res1);
		res2 = conjAndScale(res2);
		resChoppyX1 = conjAndScale(resChoppyX1);
		resChoppyX2 = conjAndScale(resChoppyX2);
		resChoppyY1 = conjAndScale(resChoppyY1);
		resChoppyY2 = conjAndScale(resChoppyY2);
		resSlopeX1 = conjAndScale(resSlopeX1);
		resSlopeX2 = conjAndScale(resSlopeX2);
		resSlopeY1 = conjAndScale(resSlopeY1);
		resSlopeY2 = conjAndScale(resSlopeY2);
	}
	imageStore(writeTex, pixelCoord1, vec4(res1, 0.0f, 1.0f));
	imageStore(writeTex, pixelCoord2, vec4(res2, 0.0f, 1.0f));
	imageStore(writeChoppyTex, pixelCoord1, vec4(resChoppyX1, resChoppyY1));
	imageStore(writeChoppyTex, pixelCoord2, vec4(resChoppyX2, resChoppyY2));
	imageStore(writeSlopeTex, pixelCoord1, vec4(resSlopeX1, resSlopeY1));
	imageStore(writeSlopeTex, pixelCoord2, vec4(resSlopeX2, resSlopeY2));
}

void doLastPass(ivec2 pixelCoord1, ivec2 pixelCoord2, int index)
{
	vec2 pixel1 = imageLoad(readTex, pixelCoord1).rg;
	vec2 pixel2 = imageLoad(readTex, pixelCoord2).rg;
	vec4 pixelChoppy1 = imageLoad(readChoppyTex, pixelCoord1);
	vec4 pixelChoppy2 = imageLoad(readChoppyTex, pixelCoord2);
	vec4 pixelSlope1 = imageLoad(readSlopeTex, pixelCoord1);
	vec4 pixelSlope2 = imageLoad(readSlopeTex, pixelCoord2);

	vec4 res = combinePixels(pixel1, pixel2, index);
	vec2 res1 = res.xy, res2 = res.zw;
	
	vec4 resChoppyX = combinePixels(pixelChoppy1.xy, pixelChoppy2.xy, index);
	vec2 resChoppyX1 = resChoppyX.xy, resChoppyX2 = resChoppyX.zw;
	vec4 resChoppyY = combinePixels(pixelChoppy1.zw, pixelChoppy2.zw, index);
	vec2 resChoppyY1 = resChoppyY.xy, resChoppyY2 = resChoppyY.zw;

	vec4 resSlopeX = combinePixels(pixelSlope1.xy, pixelSlope2.xy, index);
	vec2 resSlopeX1 = resSlopeX.xy, resSlopeX2 = resSlopeX.zw;
	vec4 resSlopeY = combinePixels(pixelSlope1.zw, pixelSlope2.zw, index);
	vec2 resSlopeY1 = resSlopeY.xy, resSlopeY2 = resSlopeY.zw;
		
	res1 = conjAndScale(res1);
	res2 = conjAndScale(res2);
	resChoppyX1 = conjAndScale(resChoppyX1);
	resChoppyX2 = conjAndScale(resChoppyX2);
	resChoppyY1 = conjAndScale(resChoppyY1);
	resChoppyY2 = conjAndScale(resChoppyY2);
	resSlopeX1 = conjAndScale(resSlopeX1);
	resSlopeX2 = conjAndScale(resSlopeX2);
	resSlopeY1 = conjAndScale(resSlopeY1);
	resSlopeY2 = conjAndScale(resSlopeY2);

	imageStore(writeTex, pixelCoord1, vec4(length(res1), 0.0f, 0.0f, 1.0f));
	imageStore(writeTex, pixelCoord2, vec4(length(res2), 0.0f, 0.0f, 1.0f));
	imageStore(writeChoppyTex, pixelCoord1, vec4(length(resChoppyX1), length(resChoppyY1), 0.0f, 1.0f));
	imageStore(writeChoppyTex, pixelCoord2, vec4(length(resChoppyX2), length(resChoppyY2), 0.0f, 1.0f));
	imageStore(writeSlopeTex, pixelCoord1, vec4(length(resSlopeX1), length(resSlopeY1), 0.0f, 1.0f));
	imageStore(writeSlopeTex, pixelCoord2, vec4(length(resSlopeX2), length(resSlopeY2), 0.0f, 1.0f));
}