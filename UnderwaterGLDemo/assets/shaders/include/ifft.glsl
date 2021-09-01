const float two_pi = 6.28318531;

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

uniform usampler2D coordLookupTex;
layout (rg32f) uniform readonly image2D readTex;
layout (rg32f) uniform writeonly image2D writeTex;
uniform uint fourierGridSize;
uniform uint level;
uniform uint N;

vec2 twiddle(vec2 q, uint k)
{
	float arg = -(two_pi * k) / N;
	float sinarg = sin(arg), cosarg = cos(arg);
	return vec2(q.x * cosarg + q.y * sinarg, q.y * cosarg - q.x * sinarg);
}

vec4 pass(vec2 pixel1, vec2 pixel2, int index)
{
	uint k = index % (N / 2);
	vec2 p = pixel1, q = twiddle(pixel2, k);
	return vec4(p + q, p - q);
}