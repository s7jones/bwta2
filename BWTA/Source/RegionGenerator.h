#pragma once

#include <BWTA/Polygon.h>

#include "MapData.h"

namespace BWTA
{
	void generateVoronoid(const std::vector<Polygon>& polygons, BoostVoronoi& voronoi);
}