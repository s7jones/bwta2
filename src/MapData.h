#pragma once

#include <BWTA.h>
#include "offline/TileType.h"

namespace BWTA
{
	typedef std::list<Chokepoint*> ChokePath;
	typedef std::set< std::pair<Chokepoint*, int> > ChokeCost;
	typedef std::map<Chokepoint*, ChokeCost> ChokepointGraph;

	namespace MapData
	{
		extern BWAPI::Unitset minerals;
		extern BWAPI::Unitset rawMinerals;
		extern BWAPI::Unitset geysers;
		extern RectangleArray<bool> walkability;
		extern RectangleArray<bool> rawWalkability;
		extern RectangleArray<bool> lowResWalkability;
		extern RectangleArray<bool> buildability;
		extern RectangleArray<int> distanceTransform;
		extern BWAPI::TilePosition::set startLocations;
		extern std::string hash;
		extern std::string mapFileName;
		extern int mapWidth;
		extern int mapHeight;
		extern int maxDistanceTransform;
		// data for HPA*
		extern ChokepointGraph chokeNodes;
		
		// offline map load
		extern RectangleArray<bool> isWalkable;
		extern TileID   *TileArray;
		extern TileType *TileSet;
		/** Direct mapping of minitile flags array */
		struct MiniTileMaps_type {
			struct MiniTileFlagArray {
				u16 miniTile[16];
			};
			MiniTileFlagArray tile[0x10000];
		};
		extern MiniTileMaps_type *MiniTileFlags;
	}
}


