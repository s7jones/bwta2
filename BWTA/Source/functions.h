#pragma once

#include <BWTA/BaseLocation.h>
#include <BWTA/Polygon.h>

#define BWTA_FILE_VERSION 6
namespace BWTA
{

  bool createDir(std::string& path);
  void loadMapFromBWAPI();
  void loadMap();
  int str2int(std::string str);

  void calculateWalkDistances(const RectangleArray<bool> &read_map,
	  const BWAPI::WalkPosition &start,
	  int max_distance,
	  RectangleArray<int> &distance_map);

  void calculateTileDistances(const RectangleArray<bool>& walkable,
	  const BWAPI::TilePosition &start,
	  const double& maxDistance,
	  RectangleArray<double>& distanceMap);

  void calculate_walk_distances_area(const BWAPI::Position &start
                                    ,int width
                                    ,int height
                                    ,int max_distance
                                    ,RectangleArray<int> &distance_map);

  int get_set(std::vector<int> &a,int i);

  bool fileExists(std::string filename);
  int fileVersion(std::string filename);

  double AstarSearchDistance(BWAPI::TilePosition start, BWAPI::TilePosition end);
  std::pair<BWAPI::TilePosition,double> AstarSearchDistance(BWAPI::TilePosition start, std::set<BWAPI::TilePosition>& end);
  std::map<BWAPI::TilePosition,double> AstarSearchDistanceAll(BWAPI::TilePosition start, std::set<BWAPI::TilePosition>& end);
  std::vector<BWAPI::TilePosition> AstarSearchPath(BWAPI::TilePosition start, BWAPI::TilePosition end);
  std::vector<BWAPI::TilePosition> AstarSearchPath(BWAPI::TilePosition start, std::set<BWAPI::TilePosition> end);
}