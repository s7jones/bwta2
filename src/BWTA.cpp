#include <BWTA.h>
#include "BWTA_Result.h"
#include "Heap.h"
#include "MapData.h"
#include "functions.h"

namespace BWTA
{
  const std::set<Region*>& getRegions()
  {
    return BWTA_Result::regions;
  }
  const std::set<Chokepoint*>& getChokepoints()
  {
    return BWTA_Result::chokepoints;
  }
  const std::set<BaseLocation*>& getBaseLocations()
  {
    return BWTA_Result::baselocations;
  }
  const std::set<BaseLocation*>& getStartLocations()
  {
    return BWTA_Result::startlocations;
  }
  const std::set<Polygon*>& getUnwalkablePolygons()
  {
    return BWTA_Result::unwalkablePolygons;
  }
  BWAPI::Position getNearestUnwalkablePosition(BWAPI::Position position)
  {
    Polygon* p = BWTA::getNearestUnwalkablePolygon(position.x()/32,position.y()/32);
    BWAPI::Position nearest = BWAPI::Positions::None;
    if (p == NULL)
    {
      //use an edge of the map if we don't find a polygon
      nearest = BWAPI::Position(0,position.y());
    }
    else
    {
      nearest = p->getNearestPoint(position);
    }
    if (position.x()<position.getDistance(nearest))
      nearest=BWAPI::Position(0,position.y());
    if (position.y()<position.getDistance(nearest))
      nearest=BWAPI::Position(position.x(),0);
    if (BWAPI::Broodwar->mapWidth()*32-position.x()<position.getDistance(nearest))
      nearest=BWAPI::Position(BWAPI::Broodwar->mapWidth()*32,position.y());
    if (BWAPI::Broodwar->mapHeight()*32-position.y()<position.getDistance(nearest))
      nearest=BWAPI::Position(position.x(),BWAPI::Broodwar->mapHeight()*32);
    return nearest;
  }
  BaseLocation* getStartLocation(BWAPI::Player* player)
  {
    if (player==NULL) return NULL;
    return getNearestBaseLocation(player->getStartLocation());
  }
  Region* getRegion(int x, int y)
  {
    return BWTA::BWTA_Result::getRegion.getItemSafe(x,y);
  }
  Region* getRegion(BWAPI::TilePosition tileposition)
  {
    return BWTA::BWTA_Result::getRegion.getItemSafe(tileposition.x(),tileposition.y());
  }
  std::set<Region*> fourRegions;
  Region* getRegion(BWAPI::Position position)
  {
    Region* r00 = BWTA::BWTA_Result::getRegion.getItemSafe((position.x()-16)/32 + 0,(position.y()-16)/32 + 0);
    Region* r01 = BWTA::BWTA_Result::getRegion.getItemSafe((position.x()-16)/32 + 0,(position.y()-16)/32 + 1);
    Region* r10 = BWTA::BWTA_Result::getRegion.getItemSafe((position.x()-16)/32 + 1,(position.y()-16)/32 + 0);
    Region* r11 = BWTA::BWTA_Result::getRegion.getItemSafe((position.x()-16)/32 + 1,(position.y()-16)/32 + 1);
    Region* aNotNullRegion = r00;
    if (aNotNullRegion == NULL)
      aNotNullRegion = r01;
    if (aNotNullRegion == NULL)
      aNotNullRegion = r10;
    if (aNotNullRegion == NULL)
      aNotNullRegion = r11;
    //If any of the regions is not null, aNotNullRegion will not be null.
    //If all regions are null or equal to aNotNullRegion, return the aNotNullRegion
    if ((r00 == aNotNullRegion || r00 == NULL) &&
        (r01 == aNotNullRegion || r01 == NULL) &&
        (r10 == aNotNullRegion || r10 == NULL) &&
        (r11 == aNotNullRegion || r11 == NULL))
      return aNotNullRegion;
    
    //If we get here, we have 2 or more regions that are not null.
    //We need to determine which of these regions we are really inside or closest to.

    fourRegions.clear();
    if (r00!=NULL)
      fourRegions.insert(r00);
    if (r01!=NULL)
      fourRegions.insert(r01);
    if (r10!=NULL)
      fourRegions.insert(r10);
    if (r11!=NULL)
      fourRegions.insert(r11);

    for(std::set<Region*>::iterator i=fourRegions.begin();i!=fourRegions.end();i++)
    {
      if ((*i)->getPolygon().isInside(position))
      {
        return *i;
      }
    }

    //we are not in any of the regions, return the one we are closest to
    double minDist = 10000;
    Region* nearest = aNotNullRegion;
    for(std::set<Region*>::iterator i=fourRegions.begin();i!=fourRegions.end();i++)
    {
      double dist = (*i)->getPolygon().getNearestPoint(position).getDistance(position);
      if (dist<minDist)
      {
        minDist = dist;
        nearest = *i;
      }
    }
    return nearest;
  }
  Chokepoint* getNearestChokepoint(int x, int y)
  {
    return BWTA::BWTA_Result::getChokepoint.getItemSafe(x,y);
  }
  Chokepoint* getNearestChokepoint(BWAPI::TilePosition position)
  {
    return BWTA::BWTA_Result::getChokepoint.getItemSafe(position.x(),position.y());
  }
  Chokepoint* getNearestChokepoint(BWAPI::Position position)
  {
    return BWTA::BWTA_Result::getChokepointW.getItemSafe(position.x()/8,position.y()/8);
  }
  BaseLocation* getNearestBaseLocation(int x, int y)
  {
    return BWTA::BWTA_Result::getBaseLocation.getItemSafe(x,y);
  }
  BaseLocation* getNearestBaseLocation(BWAPI::TilePosition tileposition)
  {
    return BWTA::BWTA_Result::getBaseLocation.getItemSafe(tileposition.x(),tileposition.y());
  }
  BaseLocation* getNearestBaseLocation(BWAPI::Position position)
  {
    return BWTA::BWTA_Result::getBaseLocationW.getItemSafe(position.x()/8,position.y()/8);
  }
  Polygon* getNearestUnwalkablePolygon(int x, int y)
  {
    return BWTA::BWTA_Result::getUnwalkablePolygon.getItemSafe(x,y);
  }
  Polygon* getNearestUnwalkablePolygon(BWAPI::TilePosition tileposition)
  {
    return BWTA::BWTA_Result::getUnwalkablePolygon.getItemSafe(tileposition.x(),tileposition.y());
  }

  bool isConnected(int x1, int y1, int x2, int y2)
  {
    if (getRegion(x1,y1)==NULL) return false;
    if (getRegion(x2,y2)==NULL) return false;
    return getRegion(x1,y1)->isReachable(getRegion(x2,y2));
  }
  bool isConnected(BWAPI::TilePosition a, BWAPI::TilePosition b)
  {
    if (getRegion(a)==NULL) return false;
    if (getRegion(b)==NULL) return false;
    return getRegion(a.x(),a.y())->isReachable(getRegion(b.x(),b.y()));
  }
  std::pair<BWAPI::TilePosition, double> getNearestTilePosition(BWAPI::TilePosition start, const std::set<BWAPI::TilePosition>& targets)
  {
    std::set<BWAPI::TilePosition> valid_targets;
    for(std::set<BWAPI::TilePosition>::const_iterator i=targets.begin();i!=targets.end();i++)
    {
      if (isConnected(start,*i))
        valid_targets.insert(*i);
    }
    if (valid_targets.empty())
      return std::make_pair(BWAPI::TilePositions::None,-1);
    return AstarSearchDistance(start,valid_targets);
  }
  double getGroundDistance(BWAPI::TilePosition start, BWAPI::TilePosition end)
  {
    if (!isConnected(start,end))
      return -1;
    return AstarSearchDistance(start,end);
  }
  std::map<BWAPI::TilePosition, double> getGroundDistances(BWAPI::TilePosition start, const std::set<BWAPI::TilePosition>& targets)
  {
    std::map<BWAPI::TilePosition, double> answer;
    std::set<BWAPI::TilePosition> valid_targets;
    for(std::set<BWAPI::TilePosition>::const_iterator i=targets.begin();i!=targets.end();i++)
    {
      if (isConnected(start,*i))
        valid_targets.insert(*i);
      else
        answer[*i]=-1;
    }
    if (valid_targets.empty())
      return answer;
    return AstarSearchDistanceAll(start,valid_targets);
  }
  void getGroundDistanceMap(BWAPI::TilePosition start, RectangleArray<double>& distanceMap)
  {
    distanceMap.resize(BWAPI::Broodwar->mapWidth(),BWAPI::Broodwar->mapHeight());
    Heap< BWAPI::TilePosition , int > heap(true);
    for(unsigned int x=0;x<distanceMap.getWidth();x++) {
      for(unsigned int y=0;y<distanceMap.getHeight();y++) {
        distanceMap[x][y]=-1;
      }
    }
    heap.push(std::make_pair(start,0));
    int sx=(int)start.x();
    int sy=(int)start.y();
    distanceMap[sx][sy]=0;
    while (!heap.empty()) {
      BWAPI::TilePosition pos=heap.top().first;
      int distance=heap.top().second;
      heap.pop();
      int x=(int)pos.x();
      int y=(int)pos.y();
      int min_x=max(x-1,0);
      int max_x=min(x+1,distanceMap.getWidth()-1);
      int min_y=max(y-1,0);
      int max_y=min(y+1,distanceMap.getHeight()-1);
      for(int ix=min_x;ix<=max_x;ix++) {
        for(int iy=min_y;iy<=max_y;iy++) {
          int f=abs(ix-x)*32+abs(iy-y)*32;
          if (f>32) {f=45;}
          int v=distance+f;
          if (distanceMap[ix][iy]>v) {
            heap.set(BWAPI::TilePosition(x,y),v);
            distanceMap[ix][iy]=v;
          } else {
            if (distanceMap[ix][iy]==-1 && MapData::lowResWalkability[ix][iy]==true) {
              distanceMap[ix][iy]=v;
              heap.push(std::make_pair(BWAPI::TilePosition(ix,iy),v));
            }
          }
        }
      }
    }
  }
  void getGroundWalkDistanceMap(int walkx, int walky, RectangleArray<double>& distanceMap)
  {
    distanceMap.resize(BWAPI::Broodwar->mapWidth()*4,BWAPI::Broodwar->mapHeight()*4);
    Heap< BWAPI::TilePosition , int > heap(true);
    for(unsigned int x=0;x<distanceMap.getWidth();x++)
    {
      for(unsigned int y=0;y<distanceMap.getHeight();y++)
      {
        distanceMap[x][y]=-1;
      }
    }
    BWAPI::TilePosition start(walkx,walky);
    heap.push(std::make_pair(start,0));
    int sx=(int)start.x();
    int sy=(int)start.y();
    distanceMap[sx][sy]=0;
    while (!heap.empty()) {
      BWAPI::TilePosition pos=heap.top().first;
      int distance=heap.top().second;
      heap.pop();
      int x=(int)pos.x();
      int y=(int)pos.y();
      int min_x=max(x-1,0);
      int max_x=min(x+1,distanceMap.getWidth()-1);
      int min_y=max(y-1,0);
      int max_y=min(y+1,distanceMap.getHeight()-1);
      for(int ix=min_x;ix<=max_x;ix++)
      {
        for(int iy=min_y;iy<=max_y;iy++)
        {
          int f=abs(ix-x)*32+abs(iy-y)*32;
          if (f>32) {f=45;}
          if (MapData::walkability[ix][iy]==false)
            f+=100000;
          int v=distance+f;
          if (distanceMap[ix][iy]>v)
          {
            heap.set(BWAPI::TilePosition(x,y),v);
            distanceMap[ix][iy]=v;
          } else {
            if (distanceMap[ix][iy]==-1)
            {
              distanceMap[ix][iy]=v;
              heap.push(std::make_pair(BWAPI::TilePosition(ix,iy),v));
            }
          }
        }
      }
    }
  }
  std::vector<BWAPI::TilePosition> getShortestPath(BWAPI::TilePosition start, BWAPI::TilePosition end)
  {
    std::vector<BWAPI::TilePosition> path;
    if (!isConnected(start,end))
      return path;
    return AstarSearchPath(start,end);
  }
  std::vector<BWAPI::TilePosition> getShortestPath(BWAPI::TilePosition start, const std::set<BWAPI::TilePosition>& targets)
  {
    std::vector<BWAPI::TilePosition> path;
    std::set<BWAPI::TilePosition> valid_targets;
    for(std::set<BWAPI::TilePosition>::const_iterator i=targets.begin();i!=targets.end();i++)
    {
      if (isConnected(start,*i))
        valid_targets.insert(*i);
    }
    if (valid_targets.empty())
      return path;
    return AstarSearchPath(start,valid_targets);
  }

  int getMaxDistanceTransform()
  {
	  return MapData::maxDistanceTransform;
  }

  RectangleArray<int>* getDistanceTransformMap()
  {
	  return &MapData::distanceTransform;
  }
}