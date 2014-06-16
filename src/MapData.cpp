#include "MapData.h"

namespace BWTA
{
	namespace MapData
	{
		BWAPI::Unitset minerals;
		BWAPI::Unitset rawMinerals;
		BWAPI::Unitset geysers;
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
    // offline map load
    RectangleArray<bool> isWalkable;
    TileID   *TileArray = NULL;
	  TileType *TileSet   = NULL;
    MiniTileMaps_type *MiniTileFlags = NULL;
	}
}