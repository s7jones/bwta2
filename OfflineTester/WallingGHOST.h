#pragma once

#include <stack>

#include <BWTA.h>
#include "..\BWTA\Source\MapData.h"

namespace BWTA
{
	int getRadius(Chokepoint* chokepoint);
	RectangleArray<int> getChokeGrid(BWAPI::TilePosition center, int offset);
	void generateWallStartingPoints(RectangleArray<int> chokeGrid, int s1x, int s1y, int s2x, int s2y, std::ofstream *out);
}

void wallingGHOST(BWTA::Chokepoint* chokepointToWall);