#pragma once
#include <BWAPI.h>
#include <BWTA.h>
namespace BWTA
{
  namespace MapData
  {
    extern std::set<BWAPI::Unit*> minerals;
    extern std::set<BWAPI::Unit*> rawMinerals;
    extern std::set<BWAPI::Unit*> geysers;
    extern RectangleArray<bool> walkability;
    extern RectangleArray<bool> rawWalkability;
    extern RectangleArray<bool> lowResWalkability;
    extern RectangleArray<bool> buildability;
    extern RectangleArray<BWAPI::TilePosition> tileParent;
    extern std::set<BWAPI::TilePosition> startLocations;
    extern std::string hash;
    extern int mapWidth;
    extern int mapHeight;
  }
}