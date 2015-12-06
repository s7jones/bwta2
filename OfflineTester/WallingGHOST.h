#pragma once

#include <stack>

#include <BWTA.h>
#include "..\BWTA\Source\MapData.h"

namespace BWTA
{
	int getRadius(Chokepoint* chokepoint);
	RectangleArray<int> getChokeGrid(BWAPI::TilePosition center, int offset);
	void generateWallStartingPoints(RectangleArray<int> chokeGrid, BWAPI::TilePosition s1, BWAPI::TilePosition s2, std::ofstream& out);
}

void wallingGHOST(BWTA::Chokepoint* chokepointToWall);