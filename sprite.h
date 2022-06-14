#pragma once

namespace Tmpl8
{

class Sprite
{
public:
	Sprite( const char* fileName );
	Sprite( const char* fileName, int2 topLeft, int2 bottomRight, int size, int frames );
	Sprite( const char* fileName, int frames );
	void ScaleAlpha( uint scale );
	uint* pixels;
	int frameCount, frameSize;
};

class SpriteInstance
{
public:
	SpriteInstance() = default;
	SpriteInstance( Sprite* s ) : sprite( s ) {}
	void Draw( Surface* target, float2 pos, int frame );
	void DrawAdditive( Surface* target, float2 pos, int frame );
	void Remove();
	Sprite* sprite = 0;
	uint* backup = 0;
	int2 lastPos;
	Surface* lastTarget = 0;
};

} // namespace Tmpl8