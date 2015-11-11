#pragma once

#include "BWTA.h"
#include "..\BWTA\Source\MapData.h"

extern std::vector<BWAPI::TilePosition> buildTiles;
extern std::vector<BWAPI::TilePosition> outsideTiles;
extern std::vector<BWAPI::TilePosition> walkableTiles;
extern std::vector<BWAPI::TilePosition> supplyTiles;
extern std::vector<BWAPI::TilePosition> barracksTiles;
extern BWTA::BaseLocation* homeBase;
extern BWTA::Region* homeRegion;
extern bool optimizeGap;

void wallingASP();
void analyzeChoke(BWTA::Chokepoint* choke);
bool canBuildHere(BWAPI::TilePosition position, BWAPI::UnitType type); // simplier version of BWAPI::canBuildHere()
void initClingoProgramSource();
BWAPI::TilePosition findClosestTile(const std::vector<BWAPI::TilePosition>& tiles, BWAPI::TilePosition targetTile);
BWAPI::TilePosition findFarthestTile(const std::vector<BWAPI::TilePosition>& tiles, BWAPI::TilePosition targetTile);