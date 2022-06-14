#include "precomp.h"

// Tank constructor
Tank::Tank( Sprite* s, int2 p, int2 t, int f, int a )
{
	// set position and destination
	pos = make_float2( p );
	target = make_float2( t );
	// create sprite instance based on existing sprite
	sprite = SpriteInstance( s );
	// set intial orientation / sprite frame; 0: north; 64: east; 128: south; 192: east
	frame = f;
	// assign tank to the specified army
	army = a;
	// create the static array of directions if it doesn't exist yet
	if (directions == 0)
	{
		directions = new float2[256];
		for (int i = 0; i < 256; i++) directions[i] = make_float2( sinf( i * PI / 128 ), -cosf( i * PI / 128 ) );
	}
	// set direction based on specified orientation
	dir = directions[frame];
}

// Tank::Tick : tank behaviour
bool Tank::Tick()
{
	// handle incoming bullets
	if (hitByBullet)
	{
		MyApp::actorPool.push_back( new ParticleExplosion( this ) );
		return false;
	}
	// fire bullet if cooled down and enemy is in range
	if (coolDown > 200 && MyApp::coolDown > 4)
	{
		// query a grid to rapidly obtain a list of nearby tanks
		ActorList& nearby = MyApp::grid.FindNearbyTanks( pos + dir * 200 );
		for (int i = 0; i < nearby.count; i++) if (nearby.tank[i]->army != this->army)
		{
			float2 toActor = normalize( nearby.tank[i]->pos - this->pos );
			if (dot( toActor, dir ) > 0.8f /* within view cone*/)
			{
				// create a bullet and add it to the actor list
				Bullet* newBullet = new Bullet( make_int2( pos + 20 * dir ), frame, army );
				MyApp::actorPool.push_back( newBullet );
				// reset cooldown timer so we don't do rapid fire
				coolDown = 0;
				MyApp::coolDown = 0;
				break;
			}
		}
	}
	coolDown++;
	// accumulate forces for steering left or right
	// 1. target attracts
	float2 toTarget = normalize( target - pos );
	float2 toRight = make_float2( -dir.y, dir.x );
	float steer = 2 * dot( toRight, toTarget );
	// 2. mountains repel
	float2 probePos = pos + 8 * dir;
	for (int s = (int)MyApp::peaks.size(), i = 0; i < s; i++)
	{
		float peakMag = MyApp::peaks[i].z / 2;
		float2 toPeak = make_float2( MyApp::peaks[i].x, MyApp::peaks[i].y ) - probePos;
		float sqrDist = dot( toPeak, toPeak );
		if (sqrDist < sqrf( peakMag ))
			toPeak = normalize( toPeak ),
			steer -= dot( toRight, toPeak ) * peakMag / sqrtf( sqrDist );
	}
	// 3. evade other tanks
	ActorList& nearby = MyApp::grid.FindNearbyTanks( this );
	for (int i = 0; i < nearby.count; i++)
	{
		Tank* tank = nearby.tank[i];
		float2 toActor = tank->pos - this->pos;
		float sqrDist = dot( toActor, toActor );
		if (sqrDist < 400 && dot( toActor, dir ) > 0.35f)
		{
			steer -= (400 - sqrDist) * 0.02f * dot( toActor, toRight ) > 0 ? 1 : -1;
			break;
		}
	}
	// adjust heading and move
	float speed = 1.0f;
	if (steer < -0.2f) frame = (frame + 255 /* i.e. -1 */) & 255, dir = directions[frame], speed = 0.35f;
	else if (steer > 0.2f) frame = (frame + 1) & 255, dir = directions[frame], speed = 0.35f;
	else {
		// draw tank tracks, only when not turning
		float2 perp( -dir.y, dir.x );
		float2 trackPos1 = pos - 9 * dir + 4.5f * perp;
		float2 trackPos2 = pos - 9 * dir - 5.5f * perp;
		MyApp::map.bitmap->BlendBilerp( trackPos1.x, trackPos1.y, 0, 12 );
		MyApp::map.bitmap->BlendBilerp( trackPos2.x, trackPos2.y, 0, 12 );
	}
	pos += dir * speed * 0.5f;
	// tanks never die
	return true;
}

// Bullet constructor
Bullet::Bullet( int2 p, int f, int a )
{
	// set position and direction
	pos = make_float2( p );
	dir = directions[f];
	frame = f;
	frameCounter = 0; // for keeping track of bullet lifetime
	army = a;
	if (flash == 0)
	{
		// load static sprite data if not done yet
		flash = new Sprite( "assets/flash.png" );
		bullet = new Sprite( "assets/bullet.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 32, 256 );
	}
	// create sprite instances based on the static sprite data
	sprite = SpriteInstance( bullet );
	flashSprite = SpriteInstance( flash );
}

// Bullet 'undraw': erase previously rendered pixels
void Bullet::Remove()
{
	// first frame uses the 'flash' sprite; subsequent frames the bullet sprite
	if (frameCounter == 1)
		flashSprite.Remove();
	else
		sprite.Remove();
}

// Bullet behaviour
bool Bullet::Tick()
{
	// update bullet position
	pos += dir * 8;
	// destroy bullet if it travelled too long
	frameCounter++;
	if (frameCounter == 110)
	{
		MyApp::actorPool.push_back( new SpriteExplosion( this ) );
		return false;
	}
	// destroy bullet if it leaves the map
	if (pos.x < 0 || pos.y < 0 || pos.x > MyApp::map.width || pos.y > MyApp::map.height) return false;
	// check if the bullet hit a tank
	ActorList& tanks = MyApp::grid.FindNearbyTanks( pos );
	for (int s = (int)tanks.count, i = 0; i < s; i++)
	{
		Tank* tank = tanks.tank[i]; // a tank, thankfully
		if (tank->army == this->army) continue; // no friendly fire. Disable for madness.
		float dist = length( this->pos - tank->pos );
		if (dist < 10)
		{
			tank->hitByBullet = true; // tank will need to draw it's own conclusion
			return false; // bees die from stinging. Disable for rail gun.
		}
	}
	// stayin' alive
	return true;
}

// Bullet Draw
void Bullet::Draw()
{
	// first frame uses the 'flash' sprite; subsequent frames the bullet sprite
	if (frameCounter == 1 || frameCounter == 159)
		flashSprite.Draw( Map::bitmap, pos, 0 );
	else
		sprite.Draw( Map::bitmap, pos, frame );
}

// ParticleExplosion constructor
ParticleExplosion::ParticleExplosion( Tank* tank )
{
	// read the pixels from the sprite of the specified tank
	Sprite* sprite = tank->sprite.sprite;
	uint size = sprite->frameSize;
	uint stride = sprite->frameSize * sprite->frameCount;
	uint* src = sprite->pixels + tank->frame * size;
	for (uint y = 0; y < size; y++) for (uint x = 0; x < size; x++)
	{
		uint pixel = src[x + y * stride];
		uint alpha = pixel >> 24;
		if (alpha > 64) for (int i = 0; i < 2; i++) // twice for a denser cloud
		{
			color.push_back( pixel & 0xffffff );
			float fx = tank->pos.x - size * 0.5f + x;
			float fy = tank->pos.y - size * 0.5f + y;
			pos.push_back( make_float2( fx, fy ) );
			dir.push_back( make_float2( 0, 0 ) );
		}
	}
}

// ParticleExplosion Draw
void ParticleExplosion::Draw()
{
	// create space to backup the map pixels we are about to modify
	if (!backup) backup = new uint[(int)pos.size() * 4];
	// draw the particles, with bilinear interpolation for smooth movement
	for (int s = (int)pos.size(), i = 0; i < s; i++)
	{
		int2 intPos = make_int2( pos[i] );
		// backup 2x2 pixels
		backup[i * 4 + 0] = MyApp::map.bitmap->Read( intPos.x, intPos.y );
		backup[i * 4 + 1] = MyApp::map.bitmap->Read( intPos.x + 1, intPos.y );
		backup[i * 4 + 2] = MyApp::map.bitmap->Read( intPos.x, intPos.y + 1 );
		backup[i * 4 + 3] = MyApp::map.bitmap->Read( intPos.x + 1, intPos.y + 1 );
		// draw one particle with bilerp, affecting four pixels
		MyApp::map.bitmap->BlendBilerp( pos[i].x, pos[i].y, color[i], fade );
	}
}

// ParticleExplosion behaviour
bool ParticleExplosion::Tick()
{
	for (int s = (int)pos.size(), i = 0; i < s; i++)
	{
		// move by adding particle speed stored in dir
		pos[i] += dir[i];
		// adjust dir randomly
		dir[i] -= make_float2( RandomFloat() * 0.05f + 0.02f, RandomFloat() * 0.02f - 0.01f );
	}
	// fadeout and kill actor when completely faded out
	if (fade-- == 0) return false; else return true;
}

// ParticleExplosion 'undraw'
void ParticleExplosion::Remove()
{
	// restore map pixels that we changed, in reverse order (this is important)
	for (int s = (int)pos.size(), i = s - 1; i >= 0; i--)
	{
		int2 intPos = make_int2( pos[i] );
		MyApp::map.bitmap->Plot( intPos.x, intPos.y, backup[i * 4 + 0] );
		MyApp::map.bitmap->Plot( intPos.x + 1, intPos.y, backup[i * 4 + 1] );
		MyApp::map.bitmap->Plot( intPos.x, intPos.y + 1, backup[i * 4 + 2] );
		MyApp::map.bitmap->Plot( intPos.x + 1, intPos.y + 1, backup[i * 4 + 3] );
	}
}

// SpriteExplosion constructor
SpriteExplosion::SpriteExplosion( Bullet* bullet )
{
	// load the static sprite data if it doesn't exist yet
	if (!anim) anim = new Sprite( "assets/explosion1.png", 16 );
	// set member variables
	sprite = SpriteInstance( anim );
	pos = bullet->pos;
	frame = 0;
}

// Fast dust code by George Psomathianos

// Particle constructor
Particle::Particle( Sprite* s[4], float2 p[4], uint c[4], uint d[4] )
{
	pos4[0] = _mm_set_ps( p[0].x, p[1].x, p[2].x, p[3].x );
	pos4[1] = _mm_set_ps( p[0].y, p[1].y, p[2].y, p[3].y );

	dir4[0] = _mm_set_ps( -1 - RandomFloat() * 4, -1 - RandomFloat() * 4, -1 - RandomFloat() * 4, -1 - RandomFloat() * 4 );
	dir4[1] = _mm_setzero_ps();
	//dir = make_float2( -1 - RandomFloat() * 4, 0 );
	color4 = _mm_setr_epi32( c[0], c[1], c[2], c[3] );
	frameChange4 = _mm_setr_epi32( d[0], d[1], d[2], d[3] );
	sprite[0] = SpriteInstance( s[0] );
	sprite[1] = SpriteInstance( s[1] );
	sprite[2] = SpriteInstance( s[2] );
	sprite[3] = SpriteInstance( s[3] );

}

const __m128 Particle::c0_95 = _mm_set1_ps( 0.95f );
const __m128 Particle::c0_025 = _mm_set1_ps( 0.025f );
const __m128 Particle::c0_05 = _mm_set1_ps( 0.05f );
const __m128i Particle::c255 = _mm_set1_epi32( 255 );
const __m128i Particle::c256 = _mm_set1_epi32( 256 );
const __m128 Particle::zero = _mm_setzero_ps();
const __m128 Particle::one = _mm_set1_ps( 1.0f );
// Particle behaviour
void Particle::Tick()
{
	pos4[0] = _mm_add_ps( pos4[0], dir4[0] );
	pos4[1] = _mm_add_ps( pos4[1], dir4[1] );
	dir4[1] = _mm_mul_ps( dir4[1], c0_95 );
	__m128 mask = _mm_cmplt_ps( pos4[0], zero );
	__m128 temp = _mm_and_ps( _mm_set1_ps( (float)MyApp::map.bitmap->width - 1 ), *(__m128*) & mask );
	pos4[0] = _mm_blendv_ps( pos4[0], temp, mask );
	temp = _mm_and_ps(
		(_mm_set_ps(
			(float)(RandomUInt() % MyApp::map.bitmap->height),
			(float)(RandomUInt() % MyApp::map.bitmap->height),
			(float)(RandomUInt() % MyApp::map.bitmap->height),
			(float)(RandomUInt() % MyApp::map.bitmap->height) )), *(__m128*) & mask );
	pos4[1] = _mm_blendv_ps( pos4[1], temp, mask );
	temp = _mm_and_ps( _mm_set_ps( -1 - RandomFloat() * 2.0, -1 - RandomFloat() * 2.0, -1 - RandomFloat() * 2.0, -1 - RandomFloat() * 2.0 ),
		*(__m128*) & mask );
	dir4[0] = _mm_blendv_ps( dir4[0], temp, mask );
	dir4[1] = _mm_blendv_ps( dir4[1], zero, mask );
	__m128 c0_02 = _mm_set1_ps( 0.02f );
	for (int s = (int)MyApp::peaks.size(), i = 0; i < s; i++)
	{
		__m128 peak4[3];
		peak4[0] = _mm_set1_ps( MyApp::peaks[i].x );
		peak4[1] = _mm_set1_ps( MyApp::peaks[i].y );
		peak4[2] = _mm_set1_ps( MyApp::peaks[i].z );
		__m128 toPeak4[2];
		toPeak4[0] = _mm_sub_ps( peak4[0], pos4[0] );
		toPeak4[1] = _mm_sub_ps( peak4[1], pos4[1] );
		__m128 dotToPeak4[3];
		dotToPeak4[0] = _mm_mul_ps( peak4[0], peak4[0] );
		dotToPeak4[1] = _mm_mul_ps( peak4[1], peak4[1] );
		__m128 dotAdded = _mm_add_ps( dotToPeak4[0], dotToPeak4[1] );
		__m128 sqrdot = _mm_sqrt_ps( dotAdded );
		__m128 g4 = _mm_div_ps( _mm_mul_ps( peak4[2], c0_02 ), sqrdot );
		dir4[1] = _mm_sub_ps( dir4[1], _mm_mul_ps( g4, _mm_div_ps( toPeak4[1], sqrdot ) ) );
	}
	__m128 random4 = _mm_set_ps( RandomFloat(), RandomFloat(), RandomFloat(), RandomFloat() );
	dir4[1] = _mm_add_ps( dir4[1],
		_mm_sub_ps( _mm_mul_ps( random4, c0_05 ), c0_025 ) );
	frame4 = _mm_and_si128( _mm_add_epi32( _mm_add_epi32( frame4, frameChange4 ), c256 ), c255 );
}