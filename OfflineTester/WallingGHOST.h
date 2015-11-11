#pragma once

#include <stack>

#include "BWTA.h"
#include "..\BWTA\Source\MapData.h"

namespace BWTA
{
	RectangleArray<int> getChokeGrid(Chokepoint* chokepoint);
	void generateWallStartingPoints(RectangleArray<int> chokeGrid, int s1x, int s1y, int s2x, int s2y, std::ofstream *out);
}

void wallingGHOST();