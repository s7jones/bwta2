#pragma once

#include <BWTA.h>

namespace BWTA
{
	namespace BWTA_Result
	{
		// List of objects extracted
		// TODO instead of set should be vector?
		extern std::set<Region*> regions;
		extern std::set<Chokepoint*> chokepoints;
		extern std::set<BaseLocation*> baselocations;
		extern std::set<BaseLocation*> startlocations;
		extern std::set<Polygon*> unwalkablePolygons;

		// Distance Map to closest elements (by defaults in Tile resolution, W = Walk resolution)
		extern RectangleArray<Region*> getRegion;
		extern RectangleArray<Polygon*> getUnwalkablePolygon;
		extern RectangleArray<Chokepoint*> getChokepointW;
		extern RectangleArray<Chokepoint*> getChokepoint;
		extern RectangleArray<BaseLocation*> getBaseLocationW;
		extern RectangleArray<BaseLocation*> getBaseLocation;
	};
}