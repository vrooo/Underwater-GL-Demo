const float two_pi = 6.28318531;

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

uniform usampler2D coordLookupTex;
layout (rg32f) uniform readonly image2D readTex;
layout (rg32f) uniform writeonly image2D writeTex;
layout (rgba32f) uniform readonly image2D readSlopeTex;
layout (rgba32f) uniform writeonly image2D writeSlopeTex;
uniform uint fourierGridSize;
uniform uint level;
uniform uint N;

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

vec4 pass(vec2 pixel1, vec2 pixel2, int index)
{
	uint m = index % (N / 2);
	vec2 p = pixel1, q = twiddle(pixel2, m);
	return vec4(p + q, p - q);
}