#include "precomp.h"

// Fast sprite code by Marc de Fluiter.

// Enable loop unrolling or not
#define UNROLLING

// OPT: Precalculations
const float2 p[4] = { make_float2( -1, -1 ), make_float2( 1, -1 ), make_float2( 1, 1 ), make_float2( -1, 1 ) };
const float TAU = 2 * PI;
const float oneSixth = 1 / 6;

// OPT: Ternary operator instead of minimum/maximum functions
inline int min( int x, int y )
{
	return (x < y) ? x : y;
}
inline int max( int x, int y )
{
	return (x > y) ? x : y;
}

uint ReadBilerp( Surface& bitmap, float u, float v )
{
	// read from a bitmap with bilinear interpolation.
	// warning: not optimized.
	int iu1 = (int)u % bitmap.width, iv1 = (int)v % bitmap.height;
	int iu2 = (iu1 + 1) % bitmap.width, iv2 = (iv1 + 1) % bitmap.height;
	int fracu = (int)((u - floorf( u )) * 16383), fracv = (int)((v - floorf( v )) * 16383);
	// OPT: Precalculations
	int fracu_rev = 16383 - fracu, fracv_rev = 16384 - fracv;
	return ScaleColor( bitmap.pixels[iu1 + iv1 * bitmap.width], (fracu_rev * fracv_rev) >> 20 ) +
		ScaleColor( bitmap.pixels[iu2 + iv1 * bitmap.width], (fracu * fracv_rev) >> 20 ) +
		ScaleColor( bitmap.pixels[iu1 + iv2 * bitmap.width], (fracu_rev * fracv) >> 20 ) +
		ScaleColor( bitmap.pixels[iu2 + iv2 * bitmap.width], (fracu * fracv) >> 20 );
}

Sprite::Sprite( const char* fileName )
{
	// load original bitmap
	Surface original( fileName );
	// copy to internal data
	frameCount = 1;
	frameSize = original.width;
	// OPT: Precalculation
	int frameSizeSquared = frameSize * frameSize;
	pixels = new uint[frameSizeSquared];
	memcpy( pixels, original.pixels, frameSizeSquared * 4 );
	// fix alpha
	for (int i = 0; i < frameSizeSquared; i++)
	{
		pixels[i] &= 0xffffff;
		// OPT: Introduce ternary operator '?'
		pixels[i] = pixels[i] == 0xff00ff ? 0 : pixels[i] | 0xff000000;
	}
}

Sprite::Sprite( const char* fileName, int frames )
{
	// load original bitmap
	Surface original( fileName );
	// copy to internal data
	frameCount = frames;
	frameSize = original.width / frameCount;
	// OPT: Precalculation
	int frameSizeSqrCount = frameSize * frameSize * frameCount;
	pixels = new uint[frameSizeSqrCount];
	memcpy( pixels, original.pixels, frameSizeSqrCount * 4 );
}

Sprite::Sprite( const char* fileName, int2 topLeft, int2 bottomRight, int size, int frames )
{
	// load original bitmap
	Surface original( fileName );
	// update alpha
	const uint pixelCount = original.width * original.height;
	for (uint i = 0; i < pixelCount; i++)
	{
		original.pixels[i] &= 0xffffff;
		// OPT: Introduce ternary operator '?'
		original.pixels[i] = original.pixels[i] == 0xff00ff ? 0 : original.pixels[i] | 0xff000000;
	}
	// blur alpha for better outlines
	uint* tmp = new uint[pixelCount], w = original.width;
	// OPT: Precalculations
	int pixelCountMinusWidth = pixelCount - w, pixelCountSize = pixelCount - (2 * w + 1);
	uint wPlusOne = w + 1;
	for (int j = 0; j < 4; j++)
	{
		for (uint i = wPlusOne; i < pixelCountMinusWidth; i++)
		{
			uint a1 = original.pixels[i + 1] >> 24, a2 = original.pixels[i - 1] >> 24;
			uint a3 = original.pixels[i + w] >> 24, a4 = original.pixels[i - w] >> 24;
			tmp[i] = (original.pixels[i] & 0xffffff) + (original.pixels[i] * 2 + a1 + a2 + a3 + a4) * oneSixth;
		}
		memcpy( original.pixels + wPlusOne, tmp + wPlusOne, pixelCountSize );
	}
	// WAR: Warning to use delete[]
	delete[] tmp;
	// create quad outline tables
	static float* xleft = 0, * xright = 0, * uleft, * vleft, * uright, * vright;
	// OPT: xleft = 0, therefore !xleft = false, so get rid of conditional
	xleft = new float[size], xright = new float[size];
	uleft = new float[size], uright = new float[size];
	vleft = new float[size], vright = new float[size];
	// OPT: Precalculation
	float sizeMinusOne = (float)size - 1;
	for (int i = 0; i < size; i++)
	{
		xleft[i] = sizeMinusOne, xright[i] = 0;
	}
	// produce rotated frames
	// OPT: Precalculation
	int sizeSquaredFrames = size * frames * size;
	pixels = new uint[sizeSquaredFrames];
	memset( pixels, 0, sizeSquaredFrames * 4 );
	float2 uv[4] = {
		make_float2( (float)topLeft.x, (float)topLeft.y ), make_float2( (float)bottomRight.x, (float)topLeft.y ),
		make_float2( (float)bottomRight.x, (float)bottomRight.y ), make_float2( (float)topLeft.x, (float)bottomRight.y )
	};
	for (int miny = size, maxy = 0, frame = 0; frame < frames; frame++)
	{
		// rotate a square
		float2 pos[4];
		// OPT: Precalculated TAU
		float angle = (TAU * frame) / frames;
		for (int j = 0; j < 4; j++)
			// OPT: Bit-shifts
			pos[j].x = (p[j].x * cosf( angle ) + p[j].y * sinf( angle )) * 0.35f * size + (size >> 1),
			pos[j].y = (p[j].x * sinf( angle ) - p[j].y * cosf( angle )) * 0.35f * size + (size >> 1);
		// populate outline tables
		for (int j = 0; j < 4; j++)
		{
			int h, vert0 = j, vert1 = (j + 1) & 3;
			if (pos[vert0].y > pos[vert1].y) h = vert0, vert0 = vert1, vert1 = h;
			const float y0 = pos[vert0].y, y1 = pos[vert1].y, rydiff = 1.0f / (y1 - y0);
			if (y0 == y1) continue;
			const int iy0 = max( 1, (int)y0 + 1 ), iy1 = min( size - 1, (int)y1 );
			float x0 = pos[vert0].x, dx = (pos[vert1].x - x0) * rydiff,
				u0 = uv[vert0].x, du = (uv[vert1].x - u0) * rydiff,
				v0 = uv[vert0].y, dv = (uv[vert1].y - v0) * rydiff;
			const float f = (float)iy0 - y0;
			x0 += dx * f, u0 += du * f, v0 += dv * f;
			for (int y = iy0; y <= iy1; y++, x0 += dx, u0 += du, v0 += dv)
			{
				if (x0 < xleft[y]) xleft[y] = x0, uleft[y] = u0, vleft[y] = v0;
				if (x0 > xright[y]) xright[y] = x0, uright[y] = u0, vright[y] = v0;
			}
			miny = min( miny, iy0 ), maxy = max( maxy, iy1 );
		}
		// fill the rotated quad using the outline tables
		for (int y = miny; y <= maxy; xleft[y] = (float)size - 1, xright[y++] = 0)
		{
			float x0 = xleft[y], x1 = xright[y], rxdiff = 1.0f / (x1 - x0),
				u0 = uleft[y], du = (uright[y] - u0) * rxdiff,
				v0 = vleft[y], dv = (vright[y] - v0) * rxdiff;
			const int ix0 = (int)x0 + 1, ix1 = min( SCRWIDTH - 2, (int)x1 );
			u0 += ((float)ix0 - x0) * du, v0 += ((float)ix0 - x0) * dv;
			uint* dest = pixels + frame * size + y * size * frames;
			for (int x = ix0; x <= ix1; x++, u0 += du, v0 += dv) dest[x] = ReadBilerp( original, u0, v0 );
		}
	}
	frameCount = frames;
	frameSize = size;
}

void Sprite::ScaleAlpha( uint scale )
{
	// OPT: Precalculated value
	int frameSizeSqrCount = frameSize * frameSize * frameCount;
#ifdef UNROLLING
	if ((frameSizeSqrCount & (frameSizeSqrCount - 1)) == 0)
		for (int i = 0; i < frameSizeSqrCount; i += 2)
		{
			int a = ((pixels[i] >> 24) * scale) >> 8;
			pixels[i] = (pixels[i] & 0xffffff) + (a << 24);
			a = ((pixels[i + 1] >> 24) * scale) >> 8;
			pixels[i + 1] = (pixels[i + 1] & 0xffffff) + (a << 24);
		}
	else
	#endif
		for (int i = 0; i < frameSizeSqrCount; i++)
		{
			int a = ((pixels[i] >> 24) * scale) >> 8;
			pixels[i] = (pixels[i] & 0xffffff) + (a << 24);
		}
}

void SpriteInstance::Draw( Surface* target, float2 pos, int frame )
{
	// save the area of target that we are about to overwrite
	// OPT: Precalculation
	int frameSize = sprite->frameSize;
	int frameSizeTimes4 = frameSize * 4;
	// OPT: Ternary operator
	backup = backup ? backup : new uint[sqr( frameSize + 1 )];
	int2 intPos = make_int2( pos );
	// OPT: Bit-shifts
	int x1 = intPos.x - (frameSize >> 1), x2 = x1 + frameSize;
	int y1 = intPos.y - (frameSize >> 1), y2 = y1 + frameSize;
	if (x1 < 0 || y1 < 0 || x2 >= target->width || y2 >= target->height)
	{
		// out of range; skip
		lastTarget = 0;
		return;
	}
	// OPT: Precalculations
	uint* dst_start = target->pixels + x1 + y1 * target->width;
#ifdef UNROLLING
	if ((frameSize & (frameSize - 1)) == 0)
		for (int v = 0; v < frameSize; v += 2)
		{
			uint* new_backup = backup + v * frameSize;
			uint* new_dst_start = dst_start + v * target->width;
			memcpy( new_backup, new_dst_start, frameSizeTimes4 );
			memcpy( new_backup + frameSize, new_dst_start + target->width, frameSizeTimes4 );
		}
	else
	#endif
		for (int v = 0; v < frameSize; v++)
		{
			memcpy( backup + v * frameSize, dst_start + v * target->width, frameSizeTimes4 );
		}
	lastPos = make_int2( x1, y1 );
	lastTarget = target;
	// calculate bilinear weights - these are constant in this case.
	uint frac_x = (int)(255.0f * (pos.x - floorf( pos.x )));
	uint frac_y = (int)(255.0f * (pos.y - floorf( pos.y )));
	// Precalculations
	uint frac_x_inv = (255 - frac_x), frac_y_inv = (255 - frac_y);
	uint w0 = (frac_x * frac_y) >> 8;
	uint w1 = (frac_x_inv * frac_y) >> 8;
	uint w2 = (frac_x * frac_y_inv) >> 8;
	uint w3 = (frac_x_inv * frac_y_inv) >> 8;
	// draw the sprite frame
	uint stride = sprite->frameCount * frameSize;
	// Precalculations
	uint* src_start = sprite->pixels + frame * frameSize;
	int frameSizeMinusOne = frameSize - 1;
#ifdef UNROLLING
	if ((frameSizeMinusOne & (frameSizeMinusOne - 1)) == 0)
		for (int v = 0; v < frameSizeMinusOne; v += 2)
		{
			uint* new_dst_start = dst_start + v * target->width;
			uint* new_src_start = src_start + v * stride;
			uint* dst = new_dst_start;
			uint* src = new_src_start;
			for (int u = 0; u < frameSizeMinusOne; u++, src++, dst++)
			{
				uint pix = ScaleColor( src[0], w0 )
					+ ScaleColor( src[1], w1 )
					+ ScaleColor( src[stride], w2 )
					+ ScaleColor( src[stride + 1], w3 );
				uint alpha = pix >> 24;
				*dst = ScaleColor( pix, alpha ) + ScaleColor( *dst, 255 - alpha );
				u++, src++, dst++;
				pix = ScaleColor( src[0], w0 )
					+ ScaleColor( src[1], w1 )
					+ ScaleColor( src[stride], w2 )
					+ ScaleColor( src[stride + 1], w3 );
				alpha = pix >> 24;
				*dst = ScaleColor( pix, alpha ) + ScaleColor( *dst, 255 - alpha );
			}
			dst = new_dst_start + target->width;
			src = new_src_start + stride;
			for (int u = 0; u < frameSizeMinusOne; u++, src++, dst++)
			{
				uint pix = ScaleColor( src[0], w0 )
					+ ScaleColor( src[1], w1 )
					+ ScaleColor( src[stride], w2 )
					+ ScaleColor( src[stride + 1], w3 );
				uint alpha = pix >> 24;
				*dst = ScaleColor( pix, alpha ) + ScaleColor( *dst, 255 - alpha );
				u++, src++, dst++;
				pix = ScaleColor( src[0], w0 )
					+ ScaleColor( src[1], w1 )
					+ ScaleColor( src[stride], w2 )
					+ ScaleColor( src[stride + 1], w3 );
				alpha = pix >> 24;
				*dst = ScaleColor( pix, alpha ) + ScaleColor( *dst, 255 - alpha );
			}
		}
	else
	#endif
		for (int v = 0; v < frameSizeMinusOne; v++)
		{
			uint* dst = dst_start + v * target->width;
			uint* src = src_start + v * stride;
			for (int u = 0; u < frameSizeMinusOne; u++, src++, dst++)
			{
				uint pix = ScaleColor( src[0], w0 )
					+ ScaleColor( src[1], w1 )
					+ ScaleColor( src[stride], w2 )
					+ ScaleColor( src[stride + 1], w3 );
				uint alpha = pix >> 24;
				*dst = ScaleColor( pix, alpha ) + ScaleColor( *dst, 255 - alpha );
			}
		}
}

void SpriteInstance::DrawAdditive( Surface* target, float2 pos, int frame )
{
	// save the area of target that we are about to overwrite
	// OPT: Precalculations
	int frameSize = sprite->frameSize, frameCount = sprite->frameCount;
	int frameSizeTimesCount = frameSize * frameCount, frameSizeTimes4 = frameSize * 4;
	int frameTimesFrameSize = frame * frameSize;
	// OPT: Ternary operator
	backup = backup ? backup : new uint[frameSize * frameSize];
	int2 intPos = make_int2( pos );
	// OPT: Bit-shifts
	int x1 = intPos.x - (frameSize >> 1), x2 = x1 + frameSize;
	int y1 = intPos.y - (frameSize >> 1), y2 = y1 + frameSize;
	if (x1 < 0 || y1 < 0 || x2 >= target->width || y2 >= target->height)
	{
		// out of range; skip
		lastTarget = 0;
		return;
	}
	// OPT: Precalculate a part of the dst pointer
	uint* dst_start = target->pixels + x1 + y1 * target->width;
#ifdef UNROLLING
	if ((frameSize & (frameSize - 1)) == 0)
		for (int v = 0; v < frameSize; v += 2)
		{
			uint* new_backup = backup + v * frameSize;
			uint* new_dst_start = dst_start + v * target->width;
			memcpy( new_backup, new_dst_start, frameSizeTimes4 );
			memcpy( new_backup + frameSize, new_dst_start + target->width, frameSizeTimes4 );
		}
	else
	#endif
		for (int v = 0; v < frameSize; v++)
		{
			memcpy( backup + v * frameSize, dst_start + v * target->width, frameSizeTimes4 );
		}
	// draw the sprite frame
#ifdef UNROLLING
	if ((frameSize & (frameSize - 1)) == 0)
		for (int v = 0; v < frameSize; v += 2)
		{
			// OPT: Precalculations
			uint* dst_start_new = dst_start + v * target->width;
			int index_start = frameTimesFrameSize + v * frameSizeTimesCount;
			for (int u = 0; u < frameSize; u += 2)
			{
				uint* dst = dst_start + u;
				int new_start = index_start + u;
				uint pix = sprite->pixels[new_start];
				*dst = AddBlend( *dst, pix );
				dst++;
				pix = sprite->pixels[new_start + 1];
				*dst = AddBlend( *dst, pix );
			}
			dst_start_new = dst_start_new + target->width;
			index_start = index_start + frameSizeTimesCount;
			for (int u = 0; u < frameSize; u += 2)
			{
				uint* dst = dst_start + u;
				int new_start = index_start + u;
				uint pix = sprite->pixels[new_start];
				*dst = AddBlend( *dst, pix );
				dst++;
				pix = sprite->pixels[new_start + 1];
				*dst = AddBlend( *dst, pix );
			}
		}
	else
	#endif
		for (int v = 0; v < frameSize; v++)
		{
			// OPT: Precalculations
			uint* dst_start_new = dst_start + v * target->width;
			int index_start = frameTimesFrameSize + v * frameSizeTimesCount;
			for (int u = 0; u < frameSize; u++)
			{
				uint* dst = dst_start + u;
				uint pix = sprite->pixels[index_start + u];
				*dst = AddBlend( *dst, pix );
			}
		}
	// remember where we drew so it can be removed later
	lastPos = make_int2( x1, y1 );
	lastTarget = target;
}

void SpriteInstance::Remove()
{
	// use the stored pixels to restore the rectangle affected by the sprite.
	// note: sprites must be removed in reverse order to guarantee correct removal.
	if (lastTarget)
	{
		// OPT: Precalculations
		int frameSize = sprite->frameSize;
		int frameSizeTimes4 = frameSize * 4;
		uint* dst_start = lastTarget->pixels + lastPos.x + lastPos.y * lastTarget->width;
	#ifdef UNROLLING
		if ((frameSize & (frameSize - 1)) == 0)
			for (int v = 0; v < frameSize; v += 2)
			{
				uint* new_start = dst_start + v * lastTarget->width;
				uint* new_backup = backup + v * frameSize;
				memcpy( new_start, new_backup, frameSizeTimes4 );
				memcpy( new_start + lastTarget->width, new_backup + frameSize, frameSizeTimes4 );
			}
		else
		#endif
			for (int v = 0; v < frameSize; v++)
			{
				memcpy( dst_start + v * lastTarget->width, backup + v * frameSize, frameSizeTimes4 );
			}
	}
}