#include "precomp.h"

#define TILE_WIDTH 960 
#define TILE_HEIGHT 540 

// Fast map rendering code by Conor Holden

Map::Map()
{
	// load color map
	bitmap = new Surface( "assets/colours.png" );
	width = bitmap->width;
	height = bitmap->height;
	// load height map
	Surface heightMap( "assets/heightmap.png" );
	elevation = new int[width * height];
	for (int i = 0; i < width * height; i++) elevation[i] = heightMap.pixels[i] & 255;
	// set intial focus to centre of map
	focus = make_int2( width >> 1, height >> 1 );
	// all done; original maps will be deleted when leaving scope.
}

float inv100 = 1.0f / 100.0f;
const float inv_SCRWIDTH = 1.0f / SCRWIDTH;
const float inv_SCRHEIGHT = 1.0f / SCRHEIGHT;
const float aspectRatio = (float)SCRHEIGHT / (float)SCRWIDTH;

Surface last_frame( SCRWIDTH, SCRHEIGHT );

void Map::UpdateView( Surface* target, float scale )
{
	// determine what map square to draw: centered at location 'focus', clamped to the edges of the map
	const int mapSliceSizeX = (int)((width * scale) * inv100);
	const int mapSliceSizeY = (int)((float)mapSliceSizeX * aspectRatio);
	int mapX1 = max( 0, focus.x - (mapSliceSizeX >> 1) );
	int mapY1 = max( 0, focus.y - (mapSliceSizeY >> 1) );
	int mapX2 = mapX1 + mapSliceSizeX;
	int mapY2 = mapY1 + mapSliceSizeY;
	if (mapX2 >= (width - 1))
	{
		int shift = mapX2 - (width - 2);
		mapX1 = max( 0, mapX1 - shift );
		mapX2 -= shift;
	}
	if (mapY2 >= (height - 1))
	{
		int shift = mapY2 - (height - 2);
		mapY1 = max( 0, mapY1 - shift );
		mapY2 -= shift;
	}
	focus.x = (mapX1 + mapX2) >> 1;
	focus.y = (mapY1 + mapY2) >> 1;
	view.x = mapX1, view.y = mapY1;
	view.z = mapX2, view.w = mapY2;
}

void Map::Draw( Surface* target )
{
	int dx = ((view.z - view.x) * 16384) * inv_SCRWIDTH;
	int dy = ((view.w - view.y) * 16384) * inv_SCRHEIGHT;
	// draw pixels

#if 0 // TILES
	int tile_no_x_half = SCRWIDTH / TILE_WIDTH >> 1;
	int tile_no_y_half = SCRHEIGHT / TILE_HEIGHT >> 1;
	int tile_no_half = tile_no_x_half * tile_no_y_half * 2;
//#pragma omp parallel for
	for (int tile = 0; tile < tile_no_half; tile++) {
		int y_index = (tile / tile_no_x_half);
		int y_start = y_index * TILE_HEIGHT;
		int y_end = y_start + TILE_HEIGHT;
		int x_start = ((tile % tile_no_x_half) * 2 + y_index % 2) * TILE_WIDTH;
		int x_end = x_start + TILE_WIDTH;
		int2 map_start = ScreenToMap(int2(x_start, y_start));
		uint x0_fp = ((view.x) << 14);
		uint y0_fp = ((view.y) << 14);
		uint* mapLine_start = bitmap->pixels;
		uint* dst_start = target->pixels + x_start;
		uint* lst_start = last_frame.pixels + x_start;
		for (int y = y_start; y < y_end; y++)
		{
			uint y_fp = y0_fp + y * dy;
			uint* mapLine = mapLine_start + (y_fp >> 14) * width;
			uint* dst = dst_start + y * SCRWIDTH;
			uint* lst = lst_start + y * SCRWIDTH;
			const uint y_frac = y_fp & 16383;
			for (int x = x_start; x < x_end; x++)
			{
				uint x_fp = x0_fp + x * dx;
				const uint mapPos = x_fp >> 14;
				const uint p1 = mapLine[mapPos];
				const uint p2 = mapLine[mapPos + 1]; // mem
				uint combined = p1 + p2; // int
				const uint p3 = mapLine[mapPos + width]; // mem
				combined += p3; //int
				const uint p4 = mapLine[mapPos + width + 1]; // mem
				combined += p4; //int
				if (*lst != combined)
				{
					const uint x_frac = x_fp & 16383; // integer
					*lst = combined; // memory
					const uint w1 = ((16383 - x_frac) * (16383 - y_frac)) >> 20; // integer
					const uint w3 = ((16383 - x_frac) * y_frac) >> 20;
					const uint w2 = (x_frac * (16383 - y_frac)) >> 20;
					const uint w4 = 255 - (w1 + w2 + w3);
					*dst = ScaleColor(p1, w1) + ScaleColor(p2, w2) + ScaleColor(p3, w3) + ScaleColor(p4, w4);
				}
				dst++;
				lst++;
			}
		}
	}
//#pragma omp parallel fors
	for (int tile = 0; tile < tile_no_half; tile++) {
		int y_index = (tile / tile_no_x_half);
		int y_start = y_index * TILE_HEIGHT;
		int y_end = y_start + TILE_HEIGHT;
		int x_start = ((tile % tile_no_x_half) * 2 + (y_index + 1) % 2) * TILE_WIDTH;
		int x_end = x_start + TILE_WIDTH;
		int2 map_start = ScreenToMap(int2(x_start, y_start));
		uint x0_fp = ((view.x) << 14);
		uint y0_fp = ((view.y) << 14);
		uint* mapLine_start = bitmap->pixels;
		uint* dst_start = target->pixels + x_start;
		uint* lst_start = last_frame.pixels + x_start;
		for (int y = y_start; y < y_end; y++)
		{
			uint y_fp = y0_fp + y * dy;
			uint* mapLine = mapLine_start + (y_fp >> 14) * width;
			uint* dst = dst_start + y * SCRWIDTH;
			uint* lst = lst_start + y * SCRWIDTH;
			const uint y_frac = y_fp & 16383;
			for (int x = x_start; x < x_end; x++)
			{
				uint x_fp = x0_fp + x * dx;
				const uint mapPos = x_fp >> 14;
				const uint p1 = mapLine[mapPos];
				const uint p2 = mapLine[mapPos + 1]; // mem
				uint combined = p1 + p2; // int
				const uint p3 = mapLine[mapPos + width]; // mem
				combined += p3; //int
				const uint p4 = mapLine[mapPos + width + 1]; // mem
				combined += p4; //int
				if (*lst != combined)
				{
					const uint x_frac = x_fp & 16383; // integer
					*lst = combined; // memory
					const uint w1 = ((16383 - x_frac) * (16383 - y_frac)) >> 20; // integer
					const uint w3 = ((16383 - x_frac) * y_frac) >> 20;
					const uint w2 = (x_frac * (16383 - y_frac)) >> 20;
					const uint w4 = 255 - (w1 + w2 + w3);
					*dst = ScaleColor(p1, w1) + ScaleColor(p2, w2) + ScaleColor(p3, w3) + ScaleColor(p4, w4);
				}
				dst++;
				lst++;
			}
		}
	}
#else // NO TILES
	uint x0_fp = view.x << 14;
	uint y0_fp = view.y << 14;
	uint* mapLine_start = bitmap->pixels;
	uint* dst_start = target->pixels;
	uint* lst_start = last_frame.pixels;
#if 1 // VECTORIZE
	for (int y = 0; y < SCRHEIGHT; y++)
	{
		uint y_fp = y0_fp + y * dy;
		uint* mapLine = mapLine_start + (y_fp >> 14) * width;
		uint* dst = dst_start + y * SCRWIDTH;
		uint* lst = lst_start + y * SCRWIDTH;
		const uint y_frac = y_fp & 16383;
		uint x_fp = x0_fp;
		for (int x = 0; x < SCRWIDTH; x++, x_fp += dx)
		{
			const uint mapPos = x_fp >> 14;
			const uint p1 = mapLine[mapPos];
			const uint p2 = mapLine[mapPos + 1]; // mem
			uint combined = p1 + p2; // int
			const uint p3 = mapLine[mapPos + width]; // mem
			combined += p3; //int
			const uint p4 = mapLine[mapPos + width + 1]; // mem
			combined += p4; //int
			if (*lst != combined)
			{
				const uint x_frac = x_fp & 16383; // integer
				*lst = combined; // memory
				const uint w1 = ((16383 - x_frac) * (16383 - y_frac)) >> 20; // integer
				const uint w3 = ((16383 - x_frac) * y_frac) >> 20;
				const uint w2 = (x_frac * (16383 - y_frac)) >> 20;
				const uint w4 = 255 - (w1 + w2 + w3);

				__m128i p_4 = _mm_setr_epi32(p1, p2, p3, p4);
				__m128i w_4 = _mm_setr_epi32(w1, w2, w3, w4);
				p_4 = ScaleColor_4(p_4, w_4);

				*dst = _mm_extract_epi32(p_4, 0) + _mm_extract_epi32(p_4, 1) + _mm_extract_epi32(p_4, 2) + _mm_extract_epi32(p_4, 3);

				//*dst = ScaleColor(p1, w1) + ScaleColor(p2, w2) + ScaleColor(p3, w3) + ScaleColor(p4, w4); // 
			}
			dst++;
			lst++;
		}
	}
#else // NOT_VECTORIZE
	for (int y = 0; y < SCRHEIGHT; y++)
	{
		uint y_fp = y0_fp + y * dy;
		uint* mapLine = mapLine_start + (y_fp >> 14) * width;
		uint* dst = dst_start + y * SCRWIDTH;
		uint* lst = lst_start + y * SCRWIDTH;
		const uint y_frac = y_fp & 16383;
		uint x_fp = x0_fp;
		for (int x = 0; x < SCRWIDTH; x++, x_fp += dx)
		{
			const uint mapPos = x_fp >> 14;
			const uint p1 = mapLine[mapPos];
			const uint p2 = mapLine[mapPos + 1]; // mem
			uint combined = p1 + p2; // int
			const uint p3 = mapLine[mapPos + width]; // mem
			combined += p3; //int
			const uint p4 = mapLine[mapPos + width + 1]; // mem
			combined += p4; //int
			if (*lst != combined)
			{
				const uint x_frac = x_fp & 16383; // integer
				*lst = combined; // memory
				const uint w1 = ((16383 - x_frac) * (16383 - y_frac)) >> 20; // integer
				const uint w3 = ((16383 - x_frac) * y_frac) >> 20;
				const uint w2 = (x_frac * (16383 - y_frac)) >> 20;
				const uint w4 = 255 - (w1 + w2 + w3);
				*dst = ScaleColor(p1, w1) + ScaleColor(p2, w2) + ScaleColor(p3, w3) + ScaleColor(p4, w4); // 
			}
			dst++;
			lst++;
		}
	}
#endif // VECTORIZE

#endif // TILES
}

int2 Map::ScreenToMap( int2 pos )
{
	float u = (float)pos.x * inv_SCRWIDTH;
	float v = (float)pos.y * inv_SCRHEIGHT;
	return make_int2( view.x + (int)(u * (view.z - view.x)), view.y + (int)(v * (view.w - view.y)) );
}