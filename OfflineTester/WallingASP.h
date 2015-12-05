#pragma once

#include "BWTA.h"
#include "..\BWTA\Source\MapData.h"

extern std::vector<BWAPI::TilePosition> buildTiles;
extern std::vector<BWAPI::TilePosition> outsideTiles;
extern std::vector<BWAPI::TilePosition> walkableTiles;
extern std::vector<BWAPI::TilePosition> supplyTiles;
extern std::vector<BWAPI::TilePosition> barracksTiles;
extern bool optimizeGap;

void wallingASP(BWTA::Chokepoint* chokepointToWall, BWTA::BaseLocation* homeBase);

void analyzeChoke(BWTA::Chokepoint* choke, BWTA::BaseLocation* homeBase);
bool canBuildHere(BWAPI::TilePosition position, BWAPI::UnitType type); // simplier version of BWAPI::canBuildHere()
void initClingoProgramSource(BWTA::BaseLocation* homeBase);
BWAPI::TilePosition findClosestTile(const std::vector<BWAPI::TilePosition>& tiles, BWAPI::TilePosition targetTile);
BWAPI::TilePosition findFarthestTile(const std::vector<BWAPI::TilePosition>& tiles, BWAPI::TilePosition targetTile);