#include "functions.h"

#include "Heap.h"
#include <sys/stat.h>
#include "MapData.h"
#include <BWTA/Polygon.h>

namespace BWTA
{

  int str2int(std::string str) {
    std::stringstream t(str);
    int x;
    t>>x;
    return x;
  }

  double distance_to_border(Polygon& polygon,int width, int height)
  {
	  double distance = std::min(width / 2, height / 2);
    for(size_t i=0;i<polygon.size();i++)
    {
      if (polygon[i].x<distance)
        distance=polygon[i].x;
      if (polygon[i].y<distance)
        distance=polygon[i].y;
      if (width-polygon[i].x<distance)
        distance=width-polygon[i].x;
      if (height-polygon[i].y<distance)
        distance=height-polygon[i].y;
    }
    if (distance<0) distance=0;
    return distance;
  }

  int get_set(std::vector<int> &a,int i)
  {
    if (i==a[i]) return i;
    a[i]=get_set(a,a[i]);
    return a[i];
  }

  void calculateWalkDistances(const RectangleArray<bool> &read_map,
	  const BWAPI::WalkPosition &start,
	  int max_distance,
	  RectangleArray<int> &distance_map)
  {
	  Heap< BWAPI::WalkPosition, int > heap(true);
	  for (unsigned int x = 0; x < distance_map.getWidth(); x++) {
		  for (unsigned int y = 0; y<distance_map.getHeight(); y++) {
			  distance_map[x][y] = -1;
		  }
	  }
	  heap.push(std::make_pair(start, 0));
	  int sx = (int)start.x;
	  int sy = (int)start.y;
	  distance_map[sx][sy] = 0;
	  while (!heap.empty()) {
		  BWAPI::WalkPosition pos = heap.top().first;
		  int distance = heap.top().second;
		  heap.pop();
		  int x = (int)pos.x;
		  int y = (int)pos.y;
		  if (distance>max_distance && max_distance>0) break;
		  int min_x = std::max(x - 1, 0);
		  int max_x = std::min(x + 1, (int)read_map.getWidth() - 1);
		  int min_y = std::max(y - 1, 0);
		  int max_y = std::min(y + 1, (int)read_map.getHeight() - 1);
		  for (int ix = min_x; ix <= max_x; ix++) {
			  for (int iy = min_y; iy <= max_y; iy++) {
				  int f = std::abs(ix - x) * 10 + std::abs(iy - y) * 10;
				  if (f > 10) { f = 14; }
				  int v = distance + f;
				  if (distance_map[ix][iy] > v) {
					  heap.push(std::make_pair(BWAPI::WalkPosition(x, y), v));
					  distance_map[ix][iy] = v;
				  } else {
					  if (distance_map[ix][iy] == -1 && read_map[ix][iy] == true) {
						  distance_map[ix][iy] = v;
						  heap.push(std::make_pair(BWAPI::WalkPosition(ix, iy), v));
					  }
				  }
			  }
		  }
	  }
  }

  void calculateTileDistances(const RectangleArray<bool>& walkable,
	  const BWAPI::TilePosition &start,
	  const double& maxDistance,
	  RectangleArray<double>& distanceMap)
  {
	  Heap<BWAPI::TilePosition, double> heap(true);
	  distanceMap.setTo(-1);
	  heap.push(std::make_pair(start, 0));
	  distanceMap[start.x][start.y] = 0;
	  while (!heap.empty()) {
		  BWAPI::TilePosition pos = heap.top().first;
		  double distance = heap.top().second;
		  heap.pop();
		  int x = pos.x;
		  int y = pos.y;
		  if (distance > maxDistance && maxDistance > 0) break;
		  int min_x = std::max(x - 1, 0);
		  int max_x = std::min(x + 1, (int)walkable.getWidth() - 1);
		  int min_y = std::max(y - 1, 0);
		  int max_y = std::min(y + 1, (int)walkable.getHeight() - 1);
		  for (int ix = min_x; ix <= max_x; ix++) {
			  for (int iy = min_y; iy <= max_y; iy++) {
				  double f = std::abs(ix - x) + std::abs(iy - y);
				  if (f > 1) f = 1.4;
				  double v = distance + f;
				  if (distanceMap[ix][iy] > v) {
					  heap.push(std::make_pair(BWAPI::TilePosition(x, y), v));
					  distanceMap[ix][iy] = v;
				  } else {
					  if (distanceMap[ix][iy] == -1 && walkable[ix][iy]) {
						  distanceMap[ix][iy] = v;
						  heap.push(std::make_pair(BWAPI::TilePosition(ix, iy), v));
					  }
				  }
			  }
		  }
	  }
  }

  void calculate_walk_distances_area(const BWAPI::Position &start
                                    ,int width
                                    ,int height
                                    ,int max_distance
                                    ,RectangleArray<int> &distance_map)
  {
    Heap< BWAPI::Position , int > heap(true);
    for(unsigned int x=0;x<distance_map.getWidth();x++) {
      for(unsigned int y=0;y<distance_map.getHeight();y++) {
        distance_map[x][y]=-1;
      }
    }
    int sx=(int)start.x;
    int sy=(int)start.y;
    for(int x=sx;x<sx+width;x++) {
      for(int y=sy;y<sy+height;y++) {
        heap.push(std::make_pair(BWAPI::Position(x,y),0));
        distance_map[x][y]=0;
      }
    }
    while (!heap.empty()) {
      BWAPI::Position pos=heap.top().first;
      int distance=heap.top().second;
      heap.pop();
      int x=(int)pos.x;
      int y=(int)pos.y;
      if (distance>max_distance && max_distance>0) break;
	  int min_x = std::max(x - 1, 0);
	  int max_x = std::min(x + 1, MapData::mapWidthWalkRes - 1);
	  int min_y = std::max(y - 1, 0);
	  int max_y = std::min(y + 1, MapData::mapHeightWalkRes - 1);
      for(int ix=min_x;ix<=max_x;ix++) {
        for(int iy=min_y;iy<=max_y;iy++) {
			int f = std::abs(ix - x) * 10 + std::abs(iy - y) * 10;
          if (f>10) {f=14;}
          int v=distance+f;
          if (distance_map[ix][iy]>v) {
            heap.push(std::make_pair(BWAPI::Position(x, y), v));
            distance_map[ix][iy]=v;
          } else {
            if (distance_map[ix][iy]==-1 && MapData::rawWalkability[ix][iy]==true) {
              distance_map[ix][iy]=v;
              heap.push(std::make_pair(BWAPI::Position(ix,iy),v));
            }
          }
        }
      }
    }
  }

  bool fileExists(std::string filename)
  {
    struct stat stFileInfo;
    return stat(filename.c_str(),&stFileInfo) == 0;
  }

  int fileVersion(std::string filename)
  {
      std::ifstream file_in;
      file_in.open(filename.c_str());
      int version;
      file_in >> version;
      file_in.close();
      return version;
  } 
}