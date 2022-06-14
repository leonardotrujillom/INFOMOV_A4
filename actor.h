#pragma once

namespace Tmpl8
{

class MyApp;

class Actor
{
public:
	enum { TANK = 0, BULLET, FLAG, PARTICLE_EXPLOSION, SPRITE_EXPLOSION };
	Actor() = default;
	virtual void Remove() { sprite.Remove(); }
	virtual bool Tick() = 0;
	virtual uint GetType() = 0;
	virtual void Draw() { sprite.Draw( Map::bitmap, pos, frame ); }
	SpriteInstance sprite;
	float2 pos, dir;
	int frame;
	static inline float2* directions = 0;
};

class Tank : public Actor
{
public:
	Tank( Sprite* s, int2 p, int2 t, int f, int a );
	bool Tick();
	uint GetType() { return Actor::TANK; }
	float2 target;
	int army, coolDown = 0;
	bool hitByBullet = false;
};

class Bullet : public Actor
{
public:
	Bullet( int2 p, int f, int a );
	void Remove();
	bool Tick();
	void Draw();
	uint GetType() { return Actor::BULLET; }
	SpriteInstance flashSprite;
	int frameCounter, army;
	static inline Sprite* flash = 0, * bullet = 0;
};

class ParticleExplosion : public Actor
{
public:
	ParticleExplosion() = default;
	ParticleExplosion( Tank* tank );
	~ParticleExplosion() { delete backup; }
	void Remove();
	bool Tick();
	void Draw();
	uint GetType() { return Actor::PARTICLE_EXPLOSION; }
	vector<float2> pos;
	vector<float2> dir;
	vector<uint> color;
	uint* backup = 0;
	uint fade = 255;
};

class SpriteExplosion : public Actor
{
public:
	SpriteExplosion() = default;
	SpriteExplosion( Bullet* bullet );
	bool Tick() { if (++frame == 16) return false; }
	void Draw() { sprite.DrawAdditive( Map::bitmap, pos, frame - 1 ); }
	uint GetType() { return Actor::SPRITE_EXPLOSION; }
	static inline Sprite* anim = 0;
};

class Particle
{
public:
	Particle() = default;
	Particle( Sprite* s[4], float2 p[4], uint c[4], uint d[4] );
	void Remove() { sprite[0].Remove(); sprite[1].Remove(); sprite[2].Remove(); sprite[3].Remove(); }
	void Tick();
	void Draw() {
		sprite[0].Draw( Map::bitmap, float2( pos[0], pos[4] ), frame[0] );
		sprite[1].Draw( Map::bitmap, float2( pos[1], pos[5] ), frame[1] );
		sprite[2].Draw( Map::bitmap, float2( pos[2], pos[6] ), frame[2] );
		sprite[3].Draw( Map::bitmap, float2( pos[3], pos[7] ), frame[3] );
	}
	//uint backup[4], color = 0, frame, frameChange;
	//bool hasBackup = false;
	SpriteInstance sprite[4];
	//float2 pos;
	union { __m128 pos4[2]; float pos[8]; };
	//float2 dir;
	union { __m128 dir4[2]; float dir[8]; };
	union { __m128i frame4; int frame[4]; };
	__m128i color4, frameChange4;
	__m128i backup4[4];
	__m128i hasBackup4;
	const static __m128 c0_95;
	const static __m128 c0_05;
	const static __m128 c0_025;
	const static __m128i c256;
	const static __m128i c255;
	const static __m128 zero;
	const static __m128 one;
};

} // namespace Tmpl8