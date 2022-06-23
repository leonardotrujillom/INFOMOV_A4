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
		// width = 128, height = 48
#if 1 // VECTORIZE
		__m128 const_132_4 = _mm_set1_ps(1.3225f);
		__m128 const_1_4 = _mm_set1_ps(1.0f);
		__m128 const_115_4 = _mm_set1_ps(1.15f);
		__m128 const_05_4 = _mm_set1_ps(0.5f);
		for (int x = 1; x < width; x++) {
			for (int y = 0; y < height; y += 4)
			{
				int index = x + y * width;
				int index1 = x + (y + 1) * width;
				int index2 = x + (y + 2) * width;
				int index3 = x + (y + 3) * width;

				float2 right = pos[index - 1] - pos[index];
				float2 right1 = pos[index1 - 1] - pos[index1];
				float2 right2 = pos[index2 - 1] - pos[index2];
				float2 right3 = pos[index3 - 1] - pos[index3];

				__m128 rightX_4 = _mm_setr_ps(right.x, right1.x, right2.x, right3.x);
				__m128 rightY_4 = _mm_setr_ps(right.y, right1.y, right2.y, right3.y);

				float sqrL = sqrLength(right);
				__m128 sqrL_4 = _mm_add_ps(_mm_mul_ps(rightX_4, rightX_4), _mm_mul_ps(rightY_4, rightY_4));

				__m128 mask = _mm_cmpgt_ps(sqrL_4, const_132_4);

				__m128 part1 = _mm_invsqrt_ps(sqrL_4);

				__m128 halfExcessX = _mm_mul_ps(_mm_sub_ps(rightX_4, _mm_mul_ps(_mm_mul_ps(rightX_4, part1), const_115_4)), const_05_4);
				__m128 halfExcessY = _mm_mul_ps(_mm_sub_ps(rightY_4, _mm_mul_ps(_mm_mul_ps(rightY_4, part1), const_115_4)), const_05_4);

				float2 halfExcess = float2(_mm_cvtss_f32(halfExcessX), _mm_cvtss_f32(halfExcessY));

				float2 halfExcess1 = float2(_mm_cvtss_f32(_mm_shuffle_ps(halfExcessX, halfExcessX, _MM_SHUFFLE(0,0,0,1))), 
													_mm_cvtss_f32(_mm_shuffle_ps(halfExcessY, halfExcessY, _MM_SHUFFLE(0, 0, 0, 1))));
				float2 halfExcess2 = float2(_mm_cvtss_f32(_mm_shuffle_ps(halfExcessX, halfExcessX, _MM_SHUFFLE(0, 0, 0, 2))),
													_mm_cvtss_f32(_mm_shuffle_ps(halfExcessY, halfExcessY, _MM_SHUFFLE(0, 0, 0, 2))));
				float2 halfExcess3 = float2(_mm_cvtss_f32(_mm_shuffle_ps(halfExcessX, halfExcessX, _MM_SHUFFLE(0, 0, 0, 3))),
													_mm_cvtss_f32(_mm_shuffle_ps(halfExcessY, halfExcessY, _MM_SHUFFLE(0, 0, 0, 3))));

				pos[index] += halfExcess;
				pos[index1] += halfExcess1;
				pos[index2] += halfExcess2;
				pos[index3] += halfExcess3;
				pos[index - 1] -= halfExcess;
				pos[index1 - 1] -= halfExcess1;
				pos[index2 - 1] -= halfExcess2;
				pos[index3 - 1] -= halfExcess3;
			}
		}
		for (int y = 0; y < height; y++) pos[y * width] = polePos + make_float2(0.0f, y * 1.2f);
#else // NO VECTORIZE
		float squaredDelta = 0;
		for (int x = 1; x < width; x++) {
			for (int y = 0; y < height; y++)
			{
				int index = x + y * width;

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
				//index += width;
			}
		}
		for (int y = 0; y < height; y++) pos[y * width] = polePos + make_float2( 0.0f, y * 1.2f );

		if (squaredDelta < c * 1.11f)
		{
			break;
		}
#endif // VECTORIZE
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