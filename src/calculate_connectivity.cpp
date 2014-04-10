#include "functions.h"
#include <BWTA.h>
#include "BWTA_Result.h"
#include "BaseLocationImpl.h"
#include "RegionImpl.h"
#include "ChokepointImpl.h"
#include "ConnectedComponent.h"
#include "MapData.h"
#include "Heap.h"
#include <BWTA/RectangleArray.h>
namespace BWTA
{
  void calculate_connectivity()
  {
    std::map<BWTA::RegionImpl*,BWTA::RegionImpl*> regionGroup;
    for(std::set<BWTA::Region*>::iterator r=BWTA::BWTA_Result::regions.begin();r!=BWTA::BWTA_Result::regions.end();r++)
    {
      BWTA::RegionImpl* region1=(BWTA::RegionImpl*)(*r);
      for(std::set<BWTA::Chokepoint*>::const_iterator c=(*r)->getChokepoints().begin();c!=(*r)->getChokepoints().end();c++)
      {
        BWTA::ChokepointImpl* cp=(BWTA::ChokepointImpl*)(*c);
        BWTA::RegionImpl* region2=(BWTA::RegionImpl*)(cp->_regions.first);
        if (region1==region2)
          region2=(BWTA::RegionImpl*)(cp->_regions.second);
        regionGroup[get_set2(regionGroup,region2)]=get_set2(regionGroup,region1);
      }
    }
    for(std::set<BWTA::Region*>::iterator r=BWTA::BWTA_Result::regions.begin();r!=BWTA::BWTA_Result::regions.end();r++)
    {
      BWTA::RegionImpl* region1=(BWTA::RegionImpl*)(*r);
      region1->reachableRegions.insert(region1);
      for(std::set<BWTA::Region*>::iterator r2=BWTA::BWTA_Result::regions.begin();r2!=BWTA::BWTA_Result::regions.end();r2++)
      {
        BWTA::RegionImpl* region2=(BWTA::RegionImpl*)(*r2);
        if (region1==region2)
          continue;
        if (get_set2(regionGroup,region1)==get_set2(regionGroup,region2))
        {
          region1->reachableRegions.insert(region2);
          region2->reachableRegions.insert(region1);
        }
      }
    }
    for(int x=0;x<(int)BWTA::BWTA_Result::getRegion.getWidth();x++)
    {
      for(int y=0;y<(int)BWTA::BWTA_Result::getRegion.getHeight();y++)
      {
        BWTA::Region* closestRegion = NULL;
        bool inRegion = false;
        BWTA::Polygon* closestUnwalkablePolygon = NULL;
        bool inPolygon = false;
        double minPolygonDist = -1;
        BWAPI::Position point=BWAPI::Position(x*32+16,y*32+16);
        BWTA::BWTA_Result::getChokepoint[x][y]=NULL;
        BWTA::BWTA_Result::getBaseLocation[x][y]=NULL;
        for(std::set<BWTA::Region*>::iterator r=BWTA::BWTA_Result::regions.begin();r!=BWTA::BWTA_Result::regions.end();r++)
        {
          if ((*r)->getPolygon().isInside(point))
          {
            closestRegion=*r;
            inRegion=true;
            break;
          }
        }
        for(std::set<BWTA::Polygon*>::iterator p=BWTA::BWTA_Result::unwalkablePolygons.begin();p!=BWTA::BWTA_Result::unwalkablePolygons.end();p++)
        {
          
          if ((*p)->isInside(point))
          {
            closestUnwalkablePolygon=*p;
            inPolygon=true;
            minPolygonDist=0;
            break;
          }
          else
          {
            double dist=(*p)->getNearestPoint(point).getDistance(point);
            if (minPolygonDist == -1 || dist<minPolygonDist)
            {
              minPolygonDist=dist;
              closestUnwalkablePolygon=*p;
            }
          }
        }
        if (inRegion)
          BWTA::BWTA_Result::getRegion[x][y]=closestRegion;
        BWTA::BWTA_Result::getUnwalkablePolygon[x][y]=closestUnwalkablePolygon;
      }
    }
    RectangleArray<double> minDistanceMap(MapData::mapWidth*4,MapData::mapHeight*4);
    minDistanceMap.setTo(-1);
    RectangleArray<double> distanceMap;
    for (std::set<BaseLocation*>::iterator i=BWTA::BWTA_Result::baselocations.begin();i!=BWTA::BWTA_Result::baselocations.end();i++)
    {
      BWTA::getGroundWalkDistanceMap((*i)->getTilePosition().x()*4+8,(*i)->getTilePosition().y()*4+6,distanceMap);
      for(int x=0;x<MapData::mapWidth*4;x++)
      {
        for(int y=0;y<MapData::mapHeight*4;y++)
        {
          if (distanceMap[x][y]==-1) continue;
          if (minDistanceMap[x][y]==-1 || distanceMap[x][y]<minDistanceMap[x][y])
          {
            minDistanceMap[x][y]=distanceMap[x][y];
            BWTA::BWTA_Result::getBaseLocationW[x][y]=*i;
          }
        }
      }
    }
    for(int x=0;x<MapData::mapWidth;x++)
    {
      for(int y=0;y<MapData::mapHeight;y++)
      {
        Heap<BaseLocation*,int> h;
        for(int xi=0;xi<4;xi++)
        {
          for(int yi=0;yi<4;yi++)
          {
            BaseLocation* bl = BWTA::BWTA_Result::getBaseLocationW[x*4+xi][y*4+yi];
            if (bl==NULL) continue;
            if (h.contains(bl))
            {
              int n=h.get(bl)+1;
              h.set(bl,n);
            }
            else
              h.push(std::make_pair(bl,1));
          }
        }
        if (h.empty()==false)
          BWTA::BWTA_Result::getBaseLocation[x][y]=h.top().first;
      }
    }
    minDistanceMap.setTo(-1);
    for (std::set<Chokepoint*>::iterator i=BWTA::BWTA_Result::chokepoints.begin();i!=BWTA::BWTA_Result::chokepoints.end();i++)
    {
      BWTA::getGroundWalkDistanceMap((*i)->getCenter().x()/8,(*i)->getCenter().y()/8,distanceMap);
      for(int x=0;x<MapData::mapWidth*4;x++)
      {
        for(int y=0;y<MapData::mapHeight*4;y++)
        {
          if (distanceMap[x][y]==-1) continue;
          if (minDistanceMap[x][y]==-1 || distanceMap[x][y]<minDistanceMap[x][y])
          {
            minDistanceMap[x][y]=distanceMap[x][y];
            BWTA::BWTA_Result::getChokepointW[x][y]=*i;
          }
        }
      }
    }
    for(int x=0;x<MapData::mapWidth;x++)
    {
      for(int y=0;y<MapData::mapHeight;y++)
      {
        Heap<Chokepoint*,int> h;
        for(int xi=0;xi<4;xi++)
        {
          for(int yi=0;yi<4;yi++)
          {
            Chokepoint* cp = BWTA::BWTA_Result::getChokepointW[x*4+xi][y*4+yi];
            if (cp==NULL) continue;
            if (h.contains(cp))
            {
              int n=h.get(cp)+1;
              h.set(cp,n);
            }
            else
              h.push(std::make_pair(cp,1));
          }
        }
        if (h.empty()==false)
          BWTA::BWTA_Result::getChokepoint[x][y]=h.top().first;
      }
    }
  }
}