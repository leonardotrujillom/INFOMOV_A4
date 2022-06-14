#pragma once

namespace Tmpl8
{

#define GRIDSIZE		64
#define CELLCAPACITY	256

struct ActorList 
{ 
	Tank* tank[CELLCAPACITY]; 
	int count = 0; 
};

class Grid
{
public:
	Grid() = default;
	void Clear();
	void Populate( const vector<Actor*>& actors );
	ActorList& FindNearbyTanks( Tank* aTank, float radius = 30 );
	ActorList& FindNearbyTanks( float2 position, float radius = 30, Tank* tank = 0 );
	ActorList cell[GRIDSIZE * GRIDSIZE];
	ActorList answer; // we'll use this to return a list of nearby actors
};

} // namespace Tmpl8