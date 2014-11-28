#include "MapData.h"

namespace BWTA
{
	namespace MapData
	{
		RectangleArray<bool> walkability;
		RectangleArray<bool> rawWalkability;
		RectangleArray<bool> lowResWalkability;
		RectangleArray<bool> buildability;
		RectangleArray<int> distanceTransform;
		BWAPI::TilePosition::set startLocations;
		std::string hash;
		std::string mapFileName;
		int mapWidth;
		int mapHeight;
		int maxDistanceTransform;
		// data for HPA*
		ChokepointGraph chokeNodes;
		
		// offline map data
		RectangleArray<bool> isWalkable;
		TileID   *TileArray = NULL;
		TileType *TileSet   = NULL;
		MiniTileMaps_type *MiniTileFlags = NULL;
		std::vector<UnitTypePosition> staticNeutralBuildings;
		std::vector<UnitTypeWalkPosition> resourcesWalkPositions;
	}
}