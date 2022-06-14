#include "precomp.h"

void Grid::Clear()
{
	for( int i = 0; i < GRIDSIZE * GRIDSIZE; i++ ) cell[i].count = 0;
}

void Grid::Populate( const vector<Actor*>& actors )
{
	int2 mapSize = MyApp::map.MapSize();
	float2 posScale = GRIDSIZE * make_float2( 1.0f / mapSize.x, 1.0f / mapSize.y );
	for( int s = (int)actors.size(), i = 0; i < s; i++ ) if (actors[i]->GetType() == Actor::TANK)
	{
		// calculate actor position in grid space
		int2 gridPos = make_int2( posScale * actors[i]->pos );
		// add actor to cell
		if (gridPos.x < 0 || gridPos.y < 0 || gridPos.x >= GRIDSIZE || gridPos.y >= GRIDSIZE) continue;
		ActorList& c = cell[gridPos.x + gridPos.y * GRIDSIZE];
		c.tank[c.count++ & (CELLCAPACITY - 1) /* better than overflow */] = (Tank*)actors[i];
	}
}

ActorList& Grid::FindNearbyTanks( Tank* tank, float radius )
{
	return FindNearbyTanks( tank->pos, radius, tank );
}

ActorList& Grid::FindNearbyTanks( float2 position, float radius, Tank* tank )
{
	int2 mapSize = MyApp::map.MapSize();
	float2 posScale = GRIDSIZE * make_float2( 1.0f / mapSize.x, 1.0f / mapSize.y );
	int2 gridPos = make_int2( posScale * position );
	int2 topLeft( max( 0, gridPos.x - 1 ), max( 0, gridPos.y - 1 ) );
	int2 bottomRight( min( GRIDSIZE - 1, gridPos.x + 1 ), min( GRIDSIZE - 1, gridPos.y + 1 ) );
	answer.count = 0;
	for (int x = topLeft.x; x <= bottomRight.x; x++)
	{
		for (int y = topLeft.y; y <= bottomRight.y; y++)
		{
			for (int i = 0; i < cell[x + y * GRIDSIZE].count; i++)
			{
				Tank* other = cell[x + y * GRIDSIZE].tank[i];
				if (other == tank) continue;
				float sqrDist = sqrLength( other->pos - position );
				if (sqrDist > radius * radius) continue;
				answer.tank[answer.count++ & (CELLCAPACITY - 1)] = other;
			}
		}
	}
	return answer;
}