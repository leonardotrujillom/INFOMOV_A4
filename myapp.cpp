#include "precomp.h"

TheApp* CreateApp() { return new MyApp(); }

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void MyApp::Init()
{
	// load tank sprites
	tank1 = new Sprite( "assets/tanks.png", make_int2( 128, 100 ), make_int2( 310, 360 ), 36, 256 );
	tank2 = new Sprite( "assets/tanks.png", make_int2( 327, 99 ), make_int2( 515, 349 ), 36, 256 );
	// load bush sprite for dust streams
	bush[0] = new Sprite( "assets/bush1.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 10, 256 );
	bush[1] = new Sprite( "assets/bush2.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 14, 256 );
	bush[2] = new Sprite( "assets/bush3.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 20, 256 );
	bush[0]->ScaleAlpha( 96 );
	bush[1]->ScaleAlpha( 64 );
	bush[2]->ScaleAlpha( 128 );
	// pointer
	pointer = new SpriteInstance( new Sprite( "assets/pointer.png" ) );
	// create armies
	for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) // main groups
	{
		Actor* army1Tank = new Tank( tank1, make_int2( 520 + x * 32, 2420 - y * 32 ), make_int2( 5000, -500 ), 0, 0 );
		Actor* army2Tank = new Tank( tank2, make_int2( 3300 - x * 32, y * 32 + 700 ), make_int2( -1000, 4000 ), 10, 1 );
		actorPool.push_back( army1Tank );
		actorPool.push_back( army2Tank );
	}
	for (int y = 0; y < 12; y++) for (int x = 0; x < 12; x++) // backup
	{
		Actor* army1Tank = new Tank( tank1, make_int2( 40 + x * 32, 2620 - y * 32 ), make_int2( 5000, -500 ), 0, 0 );
		Actor* army2Tank = new Tank( tank2, make_int2( 3900 - x * 32, y * 32 + 300 ), make_int2( -1000, 4000 ), 10, 1 );
		actorPool.push_back( army1Tank );
		actorPool.push_back( army2Tank );
	}
	for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) // small forward groups
	{
		Actor* army1Tank = new Tank( tank1, make_int2( 1440 + x * 32, 2220 - y * 32 ), make_int2( 3500, -500 ), 0, 0 );
		Actor* army2Tank = new Tank( tank2, make_int2( 2400 - x * 32, y * 32 + 900 ), make_int2( 1300, 4000 ), 128, 1 );
		actorPool.push_back( army1Tank );
		actorPool.push_back( army2Tank );
	}
	// load mountain peaks
	Surface mountains( "assets/peaks.png" );
	for (int y = 0; y < mountains.height; y++) for (int x = 0; x < mountains.width; x++)
	{
		uint p = mountains.pixels[x + y * mountains.width];
		if ((p & 0xffff) == 0) peaks.push_back( make_float3( make_int3( x * 8, y * 8, (p >> 16) & 255 ) ) );
	}
	// add sandstorm
	for (int i = 0; i < 7500; i += 4)
	{
		int x[4] = { RandomUInt() % map.bitmap->width, RandomUInt() % map.bitmap->width, RandomUInt() % map.bitmap->width, RandomUInt() % map.bitmap->width };
		int y[4] = { RandomUInt() % map.bitmap->height, RandomUInt() % map.bitmap->height, RandomUInt() % map.bitmap->height, RandomUInt() % map.bitmap->height };
		uint d[4] = { (RandomUInt() & 15) - 8, (RandomUInt() & 15) - 8, (RandomUInt() & 15) - 8, (RandomUInt() & 15) - 8 };
		Sprite* s[4] = { bush[(4 * i) % 3], bush[(4 * i + 1) % 3], bush[(4 * i + 2) % 3], bush[(4 * i + 3) % 3] };
		float2 p[4] = { make_float2( x[0], y[0] ), make_float2( x[1], y[1] ), make_float2( x[2], y[2] ), make_float2( x[3], y[3] ) };
		uint c[4] = { map.bitmap->pixels[x[0] + y[0] * map.bitmap->width], map.bitmap->pixels[x[1] + y[1] * map.bitmap->width], map.bitmap->pixels[x[2] + y[2] * map.bitmap->width], map.bitmap->pixels[x[3] + y[3] * map.bitmap->width] };
		sand.push_back( new Particle( s, p, c, d ) );
	}
	// place flags
	Surface* flagPattern = new Surface( "assets/flag.png" );
	VerletFlag* flag1 = new VerletFlag( make_int2( 3000, 848 ), flagPattern );
	actorPool.push_back( flag1 );
	VerletFlag* flag2 = new VerletFlag( make_int2( 1076, 1870 ), flagPattern );
	actorPool.push_back( flag2 );
	// initialize map view
	map.UpdateView( screen, zoom );
}

// -----------------------------------------------------------
// Advanced zooming
// -----------------------------------------------------------
void MyApp::MouseWheel( float y )
{
	// fetch current pointer location
	int2 pointerPos = map.ScreenToMap( mousePos );
	// adjust zoom
	zoom -= 10 * y; 
	if (zoom < 20) zoom = 20; 
	if (zoom > 100) zoom = 100;
	// adjust focus so that pointer remains stationary, if possible
	map.UpdateView( screen, zoom );
	int2 newPointerPos = map.ScreenToMap( mousePos );
	map.SetFocus( map.GetFocus() + (pointerPos - newPointerPos) );
	map.UpdateView( screen, zoom );
}

// -----------------------------------------------------------
// Process mouse input
// -----------------------------------------------------------
void MyApp::HandleInput()
{
	// anything that happens only once at application start goes here
	static bool wasDown = false, dragging = false;
	if (mouseDown && !wasDown) dragging = true, dragStart = mousePos, focusStart = map.GetFocus();
	if (!mouseDown) dragging = false;
	wasDown = mouseDown;
	if (dragging)
	{
		int2 delta = dragStart - mousePos;
		delta.x = (int)((delta.x * zoom) / 32);
		delta.y = (int)((delta.y * zoom) / 32);
		map.SetFocus( focusStart + delta );
		map.UpdateView( screen, zoom );
	}
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void MyApp::Tick( float deltaTime )
{
	Timer t;
	// draw the map
	map.Draw( screen );
	// rebuild actor grid
	grid.Clear();
	grid.Populate( actorPool );
	// update and render actors
	pointer->Remove();
	for (int s = (int)sand.size(), i = s - 1; i >= 0; i--) sand[i]->Remove();
	for (int s = (int)actorPool.size(), i = s - 1; i >= 0; i--) actorPool[i]->Remove();
	for (int s = (int)sand.size(), i = 0; i < s; i++) sand[i]->Tick();
	for (int i = 0; i < (int)actorPool.size(); i++) if (!actorPool[i]->Tick())
	{
		// actor got deleted, replace by last in list
		Actor* lastActor = actorPool.back();
		Actor* toDelete = actorPool[i];
		actorPool.pop_back();
		if (lastActor != toDelete) actorPool[i] = lastActor;
		delete toDelete;
		i--;
	}
	coolDown++;
	for (int s = (int)actorPool.size(), i = 0; i < s; i++) actorPool[i]->Draw();
	for (int s = (int)sand.size(), i = 0; i < s; i++) sand[i]->Draw();
	int2 cursorPos = map.ScreenToMap( mousePos );
	pointer->Draw( map.bitmap, make_float2( cursorPos ), 0 );
	// handle mouse
	HandleInput();
	// report frame time
	static float frameTimeAvg = 10.0f; // estimate
	frameTimeAvg = 0.95f * frameTimeAvg + 0.05f * t.elapsed() * 1000;
	printf( "frame time: %5.2fms\n", frameTimeAvg );
}