#include "functions.h"
#include <BWAPI.h>
#include <BWTA.h>
#include "BWTA_Result.h"
#include "BaseLocationImpl.h"
#include "RegionImpl.h"
#include "ChokepointImpl.h"
#include "ConnectedComponent.h"
#include "MapData.h"
#include "Heap.h"
namespace BWTA
{
  double AstarSearchDistance(BWAPI::TilePosition start, BWAPI::TilePosition end)
  {
    Heap<BWAPI::TilePosition,int> openTiles(true);
    std::map<BWAPI::TilePosition,int> gmap;
    std::set<BWAPI::TilePosition> closedTiles;
    openTiles.push(std::make_pair(start,0));
    gmap[start]=0;
    while(!openTiles.empty())
    {
      BWAPI::TilePosition p=openTiles.top().first;
      if (p==end)
        return gmap[p]*32.0/10.0;
      int fvalue=openTiles.top().second;
      int gvalue=gmap[p];
      openTiles.pop();
      closedTiles.insert(p);
      int minx=max(p.x()-1,0);
      int maxx=min(p.x()+1,BWAPI::Broodwar->mapWidth()-1);
      int miny=max(p.y()-1,0);
      int maxy=min(p.y()+1,BWAPI::Broodwar->mapHeight()-1);
      for(int x=minx;x<=maxx;x++)
        for(int y=miny;y<=maxy;y++)
        {
          if (!MapData::lowResWalkability[x][y]) continue;
          if (p.x() != x && p.y() != y && !MapData::lowResWalkability[p.x()][y] && !MapData::lowResWalkability[x][p.y()]) continue;
          BWAPI::TilePosition t(x,y);
          if (closedTiles.find(t)!=closedTiles.end()) continue;

          int g=gvalue+10; if (x!=p.x() && y!=p.y()) g+=4;
          int dx=abs(x-end.x()); int dy=abs(y-end.y());
          int h=abs(dx-dy)*10+min(dx,dy)*14;
          int f=g+h;
          if (gmap.find(t)==gmap.end() || gmap[t]>g)
          {
            gmap[t]=g;
            openTiles.set(t,f);
          }
        }
    }
    return -1;
  }
  std::pair<BWAPI::TilePosition,double> AstarSearchDistance(BWAPI::TilePosition start, std::set<BWAPI::TilePosition>& end)
  {
    Heap<BWAPI::TilePosition,int> openTiles(true);
    std::map<BWAPI::TilePosition,int> gmap;
    std::set<BWAPI::TilePosition> closedTiles;
    openTiles.push(std::make_pair(start,0));
    gmap[start]=0;
    while(!openTiles.empty())
    {
      BWAPI::TilePosition p=openTiles.top().first;
      if (end.find(p)!=end.end())
        return std::make_pair(p,gmap[p]*32.0/10.0);
      int fvalue=openTiles.top().second;
      int gvalue=gmap[p];
      openTiles.pop();
      closedTiles.insert(p);
      int minx=max(p.x()-1,0);
      int maxx=min(p.x()+1,BWAPI::Broodwar->mapWidth()-1);
      int miny=max(p.y()-1,0);
      int maxy=min(p.y()+1,BWAPI::Broodwar->mapHeight()-1);
      for(int x=minx;x<=maxx;x++)
        for(int y=miny;y<=maxy;y++)
        {
          if (!MapData::lowResWalkability[x][y]) continue;
          if (p.x() != x && p.y() != y && !MapData::lowResWalkability[p.x()][y] && !MapData::lowResWalkability[x][p.y()]) continue;
          BWAPI::TilePosition t(x,y);
          if (closedTiles.find(t)!=closedTiles.end()) continue;

          int g=gvalue+10; if (x!=p.x() && y!=p.y()) g+=4;
          int h=-1;
          for(std::set<BWAPI::TilePosition>::iterator i=end.begin();i!=end.end();i++)
          {
            int dx=abs(x-i->x()); int dy=abs(y-i->y());
            int ch=abs(dx-dy)*10+min(dx,dy)*14;
            if (h==-1 || ch<h)
              h=ch;
          }
          int f=g+h;
          if (gmap.find(t)==gmap.end() || gmap[t]>g)
          {
            gmap[t]=g;
            openTiles.set(t,f);
          }
        }
    }
    return std::make_pair(BWAPI::TilePositions::None,-1);
  }
  std::map<BWAPI::TilePosition,double> AstarSearchDistanceAll(BWAPI::TilePosition start, std::set<BWAPI::TilePosition>& end)
  {
    Heap<BWAPI::TilePosition,int> openTiles(true);
    std::map<BWAPI::TilePosition,int> gmap;
    std::set<BWAPI::TilePosition> closedTiles;
    openTiles.push(std::make_pair(start,0));
    gmap[start]=0;
    std::map<BWAPI::TilePosition,double> result;
    while(!openTiles.empty())
    {
      BWAPI::TilePosition p=openTiles.top().first;
      if (end.find(p)!=end.end())
      {
        result[p]=gmap[p]*32.0/10.0;
        end.erase(p);
        if (end.empty())
          return result;
      }
      int fvalue=openTiles.top().second;
      int gvalue=gmap[p];
      openTiles.pop();
      closedTiles.insert(p);
      int minx=max(p.x()-1,0);
      int maxx=min(p.x()+1,BWAPI::Broodwar->mapWidth()-1);
      int miny=max(p.y()-1,0);
      int maxy=min(p.y()+1,BWAPI::Broodwar->mapHeight()-1);
      for(int x=minx;x<=maxx;x++)
        for(int y=miny;y<=maxy;y++)
        {
          if (!MapData::lowResWalkability[x][y]) continue;
          if (p.x() != x && p.y() != y && !MapData::lowResWalkability[p.x()][y] && !MapData::lowResWalkability[x][p.y()]) continue;
          BWAPI::TilePosition t(x,y);
          if (closedTiles.find(t)!=closedTiles.end()) continue;

          int g=gvalue+10; if (x!=p.x() && y!=p.y()) g+=4;
          int h=-1;
          for(std::set<BWAPI::TilePosition>::iterator i=end.begin();i!=end.end();i++)
          {
            int dx=abs(x-i->x()); int dy=abs(y-i->y());
            int ch=abs(dx-dy)*10+min(dx,dy)*14;
            if (h==-1 || ch<h)
              h=ch;
          }
          int f=g+h;
          if (gmap.find(t)==gmap.end() || gmap[t]>g)
          {
            gmap[t]=g;
            openTiles.set(t,f);
          }
        }
    }
    return result;
  }
  std::vector<BWAPI::TilePosition> AstarSearchPath(BWAPI::TilePosition start, BWAPI::TilePosition end)
  {

    Heap<BWAPI::TilePosition,int> openTiles(true);
    std::map<BWAPI::TilePosition,int> gmap;
    std::map<BWAPI::TilePosition,BWAPI::TilePosition> parent;
    std::set<BWAPI::TilePosition> closedTiles;
    openTiles.push(std::make_pair(start,0));
    gmap[start]=0;
    parent[start]=start;
    while(!openTiles.empty())
    {
      BWAPI::TilePosition p=openTiles.top().first;
      if (p==end)
      {
        std::vector<BWAPI::TilePosition> reverse_path;
        while(p!=parent[p])
        {
          reverse_path.push_back(p);
          p=parent[p];
        }
        reverse_path.push_back(start);
        std::vector<BWAPI::TilePosition> path;
        for(int i=reverse_path.size()-1;i>=0;i--)
          path.push_back(reverse_path[i]);
        return path;
      }
      int fvalue=openTiles.top().second;
      int gvalue=gmap[p];
      openTiles.pop();
      closedTiles.insert(p);
      int minx=max(p.x()-1,0);
      int maxx=min(p.x()+1,BWAPI::Broodwar->mapWidth()-1);
      int miny=max(p.y()-1,0);
      int maxy=min(p.y()+1,BWAPI::Broodwar->mapHeight()-1);
      for(int x=minx;x<=maxx;x++)
        for(int y=miny;y<=maxy;y++)
        {
          if (!MapData::lowResWalkability[x][y]) continue;
          if (p.x() != x && p.y() != y && !MapData::lowResWalkability[p.x()][y] && !MapData::lowResWalkability[x][p.y()]) continue;
          BWAPI::TilePosition t(x,y);
          if (closedTiles.find(t)!=closedTiles.end()) continue;

          int g=gvalue+10;
          if (x!=p.x() && y!=p.y()) g+=4;
          int dx=abs(x-end.x());
          int dy=abs(y-end.y());
          int h=abs(dx-dy)*10+min(dx,dy)*14;
          int f=g+h;
          if (gmap.find(t)==gmap.end() || g<gmap.find(t)->second)
          {
            gmap[t]=g;
            openTiles.set(t,f);
            parent[t]=p;
          }
        }
    }
    std::vector<BWAPI::TilePosition> nopath;
    return nopath;
  }
  std::vector<BWAPI::TilePosition> AstarSearchPath(BWAPI::TilePosition start, std::set<BWAPI::TilePosition> end)
  {
    Heap<BWAPI::TilePosition,int> openTiles(true);
    std::map<BWAPI::TilePosition,int> gmap;
    std::map<BWAPI::TilePosition,BWAPI::TilePosition> parent;
    std::set<BWAPI::TilePosition> closedTiles;
    openTiles.push(std::make_pair(start,0));
    gmap[start]=0;
    parent[start]=start;
    while(!openTiles.empty())
    {
      BWAPI::TilePosition p=openTiles.top().first;
      if (end.find(p)!=end.end())
      {
        std::vector<BWAPI::TilePosition> reverse_path;
        while(p!=parent[p])
        {
          reverse_path.push_back(p);
          p=parent[p];
        }
        reverse_path.push_back(start);
        std::vector<BWAPI::TilePosition> path;
        for(int i=reverse_path.size()-1;i>=0;i--)
          path.push_back(reverse_path[i]);
        return path;
      }
      int fvalue=openTiles.top().second;
      int gvalue=gmap[p];
      openTiles.pop();
      closedTiles.insert(p);
      int minx=max(p.x()-1,0);
      int maxx=min(p.x()+1,BWAPI::Broodwar->mapWidth()-1);
      int miny=max(p.y()-1,0);
      int maxy=min(p.y()+1,BWAPI::Broodwar->mapHeight()-1);
      for(int x=minx;x<=maxx;x++)
        for(int y=miny;y<=maxy;y++)
        {
          if (!MapData::lowResWalkability[x][y]) continue;
          if (p.x() != x && p.y() != y && !MapData::lowResWalkability[p.x()][y] && !MapData::lowResWalkability[x][p.y()]) continue;
          BWAPI::TilePosition t(x,y);
          if (closedTiles.find(t)!=closedTiles.end()) continue;

          int g=gvalue+10; if (x!=p.x() && y!=p.y()) g+=4;
          int h=-1;
          for(std::set<BWAPI::TilePosition>::iterator i=end.begin();i!=end.end();i++)
          {
            int dx=abs(x-i->x()); int dy=abs(y-i->y());
            int ch=abs(dx-dy)*10+min(dx,dy)*14;
            if (h==-1 || ch<h)
              h=ch;
          }
          int f=g+h;
          if (gmap.find(t)==gmap.end() || gmap[t]>g)
          {
            gmap[t]=g;
            openTiles.set(t,f);
            parent[t]=p;
          }
        }
    }
    std::vector<BWAPI::TilePosition> nopath;
    return nopath;
  }
}