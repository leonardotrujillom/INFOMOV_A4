#include "precomp.h"
#include <intrin.h>

// Optimized flag code by Erik Welling.

VerletFlag::VerletFlag( int2 location, Surface* pattern )
{
	width = pattern->width;
	height = pattern->height;
	polePos = make_float2( location );
	pos = new float2[width * height];
	prevPos = new float2[width * height];
	color = new uint[width * height];
	backup = new uint[width * height * 4];
	memcpy( color, pattern->pixels, width * height * 4 );
	for (int x = 0; x < width; x++) for (int y = 0; y < height; y++)
		pos[x + y * width] = make_float2( location.x - x * 1.2f, y * 1.2f + location.y );
	memcpy( prevPos, pos, width * height * 8 );
}

void VerletFlag::Draw()
{
	for (int x = 0; x < width; x++) {
		int index = x;
		for (int y = 0; y < height; y++)
		{
			float2 p = pos[index];
			int2 intPos = make_int2( p );
			backup[index * 4 + 0] = MyApp::map.bitmap->Read( intPos.x, intPos.y );
			backup[index * 4 + 1] = MyApp::map.bitmap->Read( intPos.x + 1, intPos.y );
			backup[index * 4 + 2] = MyApp::map.bitmap->Read( intPos.x, intPos.y + 1 );
			backup[index * 4 + 3] = MyApp::map.bitmap->Read( intPos.x + 1, intPos.y + 1 );
			hasBackup = true;
			MyApp::map.bitmap->PlotBilerp( p.x, p.y, color[index] );
			index += width;
		}
	}
}

float fastInvSqrt( float number ) {
	long i;
	float x2, y;
	y = number;
	i = *(long*)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float*)&i;
	return y;
}

float2 FastNormalize( float2 input ) {
	return input * fastInvSqrt( dot( input, input ) );
}

bool VerletFlag::Tick()
{
	uint64_t start = __rdtsc();

	float windForce = 0.1f + 0.05f * RandomFloat();
	float2 wind = windForce * FastNormalize( make_float2( -1.0f, (RandomFloat() * 0.5f) - 0.25f ) );

	// move vertices
	int c = width * height;
	for (int index = 0; index < c; index++) {
		float2 delta = pos[index] - prevPos[index];
		prevPos[index] = pos[index];
		pos[index] += delta;
	}

	// apply forces
	for (int index = 0; index < c; index++) {
		pos[index] += wind;
		if ((RandomUInt() & 31) == 31)
		{
			// small chance of a random nudge to add a bit of noise to the animation
			float2 nudge = make_float2( RandomFloat() - 0.5f, RandomFloat() - 0.5f );
			pos[index] += nudge;
		}
	}

	// constraints: limit distance
	for (int i = 0; i < 25; i++)
	{
		float squaredDelta = 0;

		for (int x = 1; x < width; x++) {
			int index = x;
			for (int y = 0; y < height; y++)
			{
				float2 right = pos[index - 1] - pos[index];

				float sqrL = sqrLength( right );
				if (sqrL > 1.3225f)
				{
					squaredDelta += sqrL - 1.3225f;

					float invL = fastInvSqrt( sqrL );

					float2 halfExcess = (right - (right * invL) * 1.15f) * 0.5f;
					pos[index] += halfExcess;
					pos[index - 1] -= halfExcess;
				}

				index += width;
			}
		}
		for (int y = 0; y < height; y++) pos[y * width] = polePos + make_float2( 0.0f, y * 1.2f );

		if (squaredDelta < c * 1.11f)
		{
			break;
		}
	}
	// all done
	return true; // flags don't die
}

void VerletFlag::Remove()
{
	if (hasBackup) for (int x = width - 1; x >= 0; x--) for (int y = height - 1; y >= 0; y--)
	{
		int index = x + y * width;
		int2 intPos = make_int2( pos[index] );
		MyApp::map.bitmap->Plot( intPos.x, intPos.y, backup[index * 4 + 0] );
		MyApp::map.bitmap->Plot( intPos.x + 1, intPos.y, backup[index * 4 + 1] );
		MyApp::map.bitmap->Plot( intPos.x, intPos.y + 1, backup[index * 4 + 2] );
		MyApp::map.bitmap->Plot( intPos.x + 1, intPos.y + 1, backup[index * 4 + 3] );
	}
}