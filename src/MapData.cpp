#include "MapData.h"

namespace BWTA
{
	namespace MapData
	{
		std::set<BWAPI::Unit*> minerals;
		std::set<BWAPI::Unit*> rawMinerals;
		std::set<BWAPI::Unit*> geysers;
		RectangleArray<bool> walkability;
		RectangleArray<bool> rawWalkability;
		RectangleArray<bool> lowResWalkability;
		RectangleArray<bool> buildability;
		RectangleArray<int> distanceTransform;
		std::set<BWAPI::TilePosition> startLocations;
		std::string hash;
		int mapWidth;
		int mapHeight;
		int maxDistanceTransform;
		// data for HPA*
		ChokepointGraph chokeNodes;
	}
}