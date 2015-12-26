#pragma once

#include <stack>

#include <BWTA.h>
#include "..\BWTA\Source\MapData.h"

struct ExtremePoints {
	BWAPI::TilePosition side1;
	BWAPI::TilePosition side2;
	ExtremePoints() :side1(BWAPI::TilePositions::None), side2(BWAPI::TilePositions::None){};
	ExtremePoints(BWAPI::TilePosition s1, BWAPI::TilePosition s2) :side1(s1), side2(s2){};
	ExtremePoints(int s1x, int s1y, int s2x, int s2y) :side1(BWAPI::TilePosition(s1x, s1y)), side2(BWAPI::TilePosition(s2x, s2y)){};
};

namespace BWTA
{
	int getRadius(Chokepoint* chokepoint);
	RectangleArray<int> getChokeGrid(BWAPI::TilePosition center, int offset);
	ExtremePoints getWallExtremePoints(RectangleArray<int> chokeGrid, BWAPI::TilePosition s1, BWAPI::TilePosition s2, BWTA::Region* prefRegion);
}

double wallingGHOST(BWTA::Chokepoint* chokepointToWall, BWTA::Region* prefRegion);