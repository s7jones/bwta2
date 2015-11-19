#pragma once

#ifdef DEBUG_DRAW
#include "Painter.h"
#endif

#include "MapData.h"
#include "RegionImpl.h"
#include "functions.h"
#include "BWTA_Result.h"
#pragma once

namespace BWTA
{
	void	computeDistanceTransform();

	void	distanceTransform();
	int		getMaxTransformDistance(int x, int y);
	void	maxDistanceOfRegion();
}