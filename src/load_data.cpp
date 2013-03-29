#include "functions.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <list>
#include <BWAPI.h>
#include "BWTA_Result.h"
#include "BaseLocationImpl.h"
#include "ChokepointImpl.h"
#include "RegionImpl.h"
#include "find_base_locations.h"
#include "MapData.h"
#include "terrain_analysis.h"
//#include <tinyxml.h>
using namespace std;
using namespace BWAPI;
namespace BWTA
{
  bool load_map()
  {
    int b_width=MapData::mapWidth;
    int b_height=MapData::mapHeight;
    int width=MapData::mapWidth*4;
    int height=MapData::mapHeight*4;
    MapData::buildability.resize(b_width,b_height);
    MapData::lowResWalkability.resize(b_width,b_height);
    MapData::walkability.resize(width,height);
    MapData::rawWalkability.resize(width,height);

    //copy buildability data into buildability array
    for(int x=0;x<b_width;x++)
    {
      for(int y=0;y<b_height;y++)
      {
        MapData::buildability[x][y]=BWAPI::Broodwar->isBuildable(x,y);
        MapData::lowResWalkability[x][y]=true;
      }
    }
    //copy and simplify walkability data as it is copies into walkability array
    for(int x=0;x<width;x++)
    {
      for(int y=0;y<height;y++)
      {
        MapData::rawWalkability[x][y]=BWAPI::Broodwar->isWalkable(x,y);
        MapData::walkability[x][y]=true;
      }
    }
    for(int x=0;x<width;x++)
    {
      for(int y=0;y<height;y++)
      {
        for(int x2=max(x-1,0);x2<=min(width-1,x+1);x2++)
        {
          for(int y2=max(y-1,0);y2<=min(height-1,y+1);y2++)
          {
            MapData::walkability[x2][y2]&=MapData::rawWalkability[x][y];
          }
        }
        MapData::lowResWalkability[x/4][y/4]&=MapData::rawWalkability[x][y];
      }
    }
    BWTA_Result::getRegion.resize(b_width,b_height);
    BWTA_Result::getChokepoint.resize(b_width,b_height);
    BWTA_Result::getBaseLocation.resize(b_width,b_height);
    BWTA_Result::getChokepointW.resize(b_width*4,b_height*4);
    BWTA_Result::getBaseLocationW.resize(b_width*4,b_height*4);
    BWTA_Result::getUnwalkablePolygon.resize(b_width,b_height);
    BWTA_Result::getRegion.setTo(NULL);
    BWTA_Result::getChokepoint.setTo(NULL);
    BWTA_Result::getBaseLocation.setTo(NULL);
    BWTA_Result::getChokepointW.setTo(NULL);
    BWTA_Result::getBaseLocationW.setTo(NULL);
    BWTA_Result::getUnwalkablePolygon.setTo(NULL);
    return true;
  }

  bool load_resources()
  {
    MapData::rawMinerals=BWAPI::Broodwar->getStaticMinerals();
    //filter out all mineral patches under 200
    for(std::set<BWAPI::Unit*>::iterator m=MapData::rawMinerals.begin();m!=MapData::rawMinerals.end();m++)
    {
      if ((*m)->getInitialResources()>200)
      {
        MapData::minerals.insert(*m);
      }
    }
    MapData::geysers=BWAPI::Broodwar->getStaticGeysers();
    return true;
  }

  void load_data(std::string filename)
  {
    int version;
    int unwalkablePolygon_amount;
    int baselocation_amount;
    int chokepoint_amount;
    int region_amount;
    int map_width;
    int map_height;
    std::vector<Polygon*> unwalkablePolygons;
    std::vector<BaseLocation*> baselocations;
    std::vector<Chokepoint*> chokepoints;
    std::vector<Region*> regions;
    std::ifstream file_in;
    file_in.open(filename.c_str());
    file_in >> version;
    if (version!=BWTA_FILE_VERSION)
    {
      file_in.close();
      return;
    }
    file_in >> unwalkablePolygon_amount;
    file_in >> baselocation_amount;
    file_in >> chokepoint_amount;
    file_in >> region_amount;
    file_in >> map_width;
    file_in >> map_height;
    for(int i=0;i<unwalkablePolygon_amount;i++)
    {
      Polygon* p=new Polygon();
      unwalkablePolygons.push_back(p);
      BWTA_Result::unwalkablePolygons.insert(p);
    }
    for(int i=0;i<baselocation_amount;i++)
    {
      BaseLocation* b=new BaseLocationImpl();
      baselocations.push_back(b);
      BWTA_Result::baselocations.insert(b);
    }
    for(int i=0;i<chokepoint_amount;i++)
    {
      Chokepoint* c=new ChokepointImpl();
      chokepoints.push_back(c);
      BWTA_Result::chokepoints.insert(c);
    }
    for(int i=0;i<region_amount;i++)
    {
      Region* r=new RegionImpl();
      regions.push_back(r);
      BWTA_Result::regions.insert(r);
    }
    for(int i=0;i<unwalkablePolygon_amount;i++)
    {
      int id, polygon_size;
      file_in >> id >> polygon_size;
      for(int j=0;j<polygon_size;j++)
      {
        int x,y;
        file_in >> x >> y;
        unwalkablePolygons[i]->push_back(BWAPI::Position(x,y));
      }
      int hole_count;
      file_in >> hole_count;
      for(int j=0;j<hole_count;j++)
      {
        file_in >> polygon_size;
        Polygon h;
        for(int k=0;k<polygon_size;k++)
        {
          int x,y;
          file_in >> x >> y;
          h.push_back(BWAPI::Position(x,y));
        }
        unwalkablePolygons[i]->holes.push_back(h);
      }
    }
    for(int i=0;i<baselocation_amount;i++)
    {
      int id,px,py,tpx,tpy;
      file_in >> id >> px >> py >> tpx >> tpy;
      ((BaseLocationImpl*)baselocations[i])->position=BWAPI::Position(px,py);
      ((BaseLocationImpl*)baselocations[i])->tilePosition=BWAPI::TilePosition(tpx,tpy);
      int rid;
      file_in >> rid;
      ((BaseLocationImpl*)baselocations[i])->region=regions[rid];
      for(int j=0;j<baselocation_amount;j++)
      {
        double g_dist, a_dist;
        file_in >> g_dist >> a_dist;
        ((BaseLocationImpl*)baselocations[i])->ground_distances[baselocations[j]]=g_dist;
        ((BaseLocationImpl*)baselocations[i])->air_distances[baselocations[j]]=a_dist;
      }
      file_in >> ((BaseLocationImpl*)baselocations[i])->island;
      file_in >> ((BaseLocationImpl*)baselocations[i])->start;
      if (((BaseLocationImpl*)baselocations[i])->start)
        BWTA::BWTA_Result::startlocations.insert(baselocations[i]);
    }
    for(int i=0;i<chokepoint_amount;i++)
    {
      int id,rid1,rid2,s1x,s1y,s2x,s2y,cx,cy;
      double width;
      file_in >> id >> rid1 >> rid2 >> s1x >> s1y >> s2x >> s2y >> cx >> cy >> width;
      ((ChokepointImpl*)chokepoints[i])->_regions.first=regions[rid1];
      ((ChokepointImpl*)chokepoints[i])->_regions.second=regions[rid2];
      ((ChokepointImpl*)chokepoints[i])->_sides.first=BWAPI::Position(s1x,s1y);
      ((ChokepointImpl*)chokepoints[i])->_sides.second=BWAPI::Position(s2x,s2y);
      ((ChokepointImpl*)chokepoints[i])->_center=BWAPI::Position(cx,cy);
      ((ChokepointImpl*)chokepoints[i])->_width=width;
    }
    for(int i=0;i<region_amount;i++)
    {
      int id, polygon_size;
      file_in >> id >> polygon_size;
      for(int j=0;j<polygon_size;j++)
      {
        int x,y;
        file_in >> x >> y;
        ((RegionImpl*)regions[i])->_polygon.push_back(BWAPI::Position(x,y));
      }
      int cx,cy,chokepoints_size;
      file_in >> cx >> cy >> chokepoints_size;
      ((RegionImpl*)regions[i])->_center=BWAPI::Position(cx,cy);
      for(int j=0;j<chokepoints_size;j++)
      {
        int cid;
        file_in >> cid;
        ((RegionImpl*)regions[i])->_chokepoints.insert(chokepoints[cid]);
      }
      int baselocations_size;
      file_in >> baselocations_size;
      for(int j=0;j<baselocations_size;j++)
      {
        int bid;
        file_in >> bid;
        ((RegionImpl*)regions[i])->baseLocations.insert(baselocations[bid]);
      }
      for(int j=0;j<region_amount;j++)
      {
        int connected=0;
        file_in >> connected;
        if (connected==1)
          ((RegionImpl*)regions[i])->reachableRegions.insert(regions[j]);
      }
    }
    BWTA_Result::getRegion.resize(map_width,map_height);
    BWTA_Result::getChokepoint.resize(map_width,map_height);
    BWTA_Result::getBaseLocation.resize(map_width,map_height);
    BWTA_Result::getUnwalkablePolygon.resize(map_width,map_height);
    for(int x=0;x<map_width;x++)
    {
      for(int y=0;y<map_height;y++)
      {
        int rid;
        file_in >> rid;
        if (rid==-1)
          BWTA_Result::getRegion[x][y]=NULL;
        else
          BWTA_Result::getRegion[x][y]=regions[rid];
      }
    }
    for(int x=0;x<map_width;x++)
    {
      for(int y=0;y<map_height;y++)
      {
        int cid;
        file_in >> cid;
        if (cid==-1)
          BWTA_Result::getChokepoint[x][y]=NULL;
        else
          BWTA_Result::getChokepoint[x][y]=chokepoints[cid];
      }
    }
    for(int x=0;x<map_width;x++)
    {
      for(int y=0;y<map_height;y++)
      {
        int bid;
        file_in >> bid;
        if (bid==-1)
          BWTA_Result::getBaseLocation[x][y]=NULL;
        else
          BWTA_Result::getBaseLocation[x][y]=baselocations[bid];
      }
    }
    for(int x=0;x<map_width*4;x++)
    {
      for(int y=0;y<map_height*4;y++)
      {
        int cid;
        file_in >> cid;
        if (cid==-1)
          BWTA_Result::getChokepointW[x][y]=NULL;
        else
          BWTA_Result::getChokepointW[x][y]=chokepoints[cid];
      }
    }
    for(int x=0;x<map_width*4;x++)
    {
      for(int y=0;y<map_height*4;y++)
      {
        int bid;
        file_in >> bid;
        if (bid==-1)
          BWTA_Result::getBaseLocationW[x][y]=NULL;
        else
          BWTA_Result::getBaseLocationW[x][y]=baselocations[bid];
      }
    }
    for(int x=0;x<map_width;x++)
    {
      for(int y=0;y<map_height;y++)
      {
        int pid;
        file_in >> pid;
        if (pid==-1)
          BWTA_Result::getUnwalkablePolygon[x][y]=NULL;
        else
          BWTA_Result::getUnwalkablePolygon[x][y]=unwalkablePolygons[pid];
      }
    }
    file_in.close();
    attach_resources_to_base_locations(BWTA_Result::baselocations);
  }

  void save_data(std::string filename)
  {
    //save_data_xml();
    std::map<Polygon*,int> pid;
    std::map<BaseLocation*,int> bid;
    std::map<Chokepoint*,int> cid;
    std::map<Region*,int> rid;
    int p_id=0;
    int b_id=0;
    int c_id=0;
    int r_id=0;
    for(std::set<Polygon*>::const_iterator p=BWTA_Result::unwalkablePolygons.begin();p!=BWTA_Result::unwalkablePolygons.end();p++)
    {
      pid[*p]=p_id++;
    }
    for(std::set<BaseLocation*>::const_iterator b=BWTA_Result::baselocations.begin();b!=BWTA_Result::baselocations.end();b++)
    {
      bid[*b]=b_id++;
    }
    for(std::set<Chokepoint*>::const_iterator c=BWTA_Result::chokepoints.begin();c!=BWTA_Result::chokepoints.end();c++)
    {
      cid[*c]=c_id++;
    }
    for(std::set<Region*>::const_iterator r=BWTA_Result::regions.begin();r!=BWTA_Result::regions.end();r++)
    {
      rid[*r]=r_id++;
    }
    std::ofstream file_out;
    file_out.open(filename.c_str());
    int file_version=BWTA_FILE_VERSION;
    file_out << file_version << "\n";
    file_out << BWTA_Result::unwalkablePolygons.size() << "\n";
    file_out << BWTA_Result::baselocations.size() << "\n";
    file_out << BWTA_Result::chokepoints.size() << "\n";
    file_out << BWTA_Result::regions.size() << "\n";
    file_out << BWTA_Result::getRegion.getWidth() << "\n";
    file_out << BWTA_Result::getRegion.getHeight() << "\n";
    for(std::set<Polygon*>::const_iterator p=BWTA_Result::unwalkablePolygons.begin();p!=BWTA_Result::unwalkablePolygons.end();p++)
    {
      file_out << pid[*p] << "\n";
      file_out << (*p)->size() << "\n";
      for(unsigned int i=0;i<(*p)->size();i++)
      {
        file_out << (**p)[i].x() << "\n";
        file_out << (**p)[i].y() << "\n";
      }
      file_out << (*p)->getHoles().size() << "\n";
      for(std::vector<Polygon>::const_iterator h=(*p)->getHoles().begin();h!=(*p)->getHoles().end();h++)
      {
        file_out << (*h).size() << "\n";
        for(unsigned int i=0;i<(*h).size();i++)
        {
          file_out << (*h)[i].x() << "\n";
          file_out << (*h)[i].y() << "\n";
        }
      }
    }
    for(std::set<BaseLocation*>::const_iterator b=BWTA_Result::baselocations.begin();b!=BWTA_Result::baselocations.end();b++)
    {
      file_out << bid[*b] << "\n";
      file_out << (*b)->getPosition().x() << "\n";
      file_out << (*b)->getPosition().y() << "\n";
      file_out << (*b)->getTilePosition().x() << "\n";
      file_out << (*b)->getTilePosition().y() << "\n";
      file_out << rid[(*b)->getRegion()] << "\n";
      for(std::set<BaseLocation*>::const_iterator b2=BWTA_Result::baselocations.begin();b2!=BWTA_Result::baselocations.end();b2++)
      {
        file_out << (*b)->getGroundDistance(*b2) << "\n";
        file_out << (*b)->getAirDistance(*b2) << "\n";
      }
      file_out << (*b)->isIsland() << "\n";
      file_out << (*b)->isStartLocation() << "\n";
    }
    for(std::set<Chokepoint*>::const_iterator c=BWTA_Result::chokepoints.begin();c!=BWTA_Result::chokepoints.end();c++)
    {
      file_out << cid[*c] << "\n";
      file_out << rid[(*c)->getRegions().first] << "\n";
      file_out << rid[(*c)->getRegions().second] << "\n";
      file_out << (*c)->getSides().first.x() << "\n";
      file_out << (*c)->getSides().first.y() << "\n";
      file_out << (*c)->getSides().second.x() << "\n";
      file_out << (*c)->getSides().second.y() << "\n";
      file_out << (*c)->getCenter().x() << "\n";
      file_out << (*c)->getCenter().y() << "\n";
      file_out << (*c)->getWidth() << "\n";
    }
    for(std::set<Region*>::const_iterator r=BWTA_Result::regions.begin();r!=BWTA_Result::regions.end();r++)
    {
      file_out << rid[*r] << "\n";
      Polygon poly=(*r)->getPolygon();
      file_out << poly.size() << "\n";
      for(unsigned int i=0;i<poly.size();i++)
      {
        file_out << poly[i].x() << "\n";
        file_out << poly[i].y() << "\n";
      }
      file_out << (*r)->getCenter().x() << "\n";
      file_out << (*r)->getCenter().y() << "\n";
      file_out << (*r)->getChokepoints().size() << "\n";
      for(std::set<Chokepoint*>::const_iterator c=(*r)->getChokepoints().begin();c!=(*r)->getChokepoints().end();c++)
      {
        file_out << cid[*c] << "\n";
      }
      file_out << (*r)->getBaseLocations().size() << "\n";
      for(std::set<BaseLocation*>::const_iterator b=(*r)->getBaseLocations().begin();b!=(*r)->getBaseLocations().end();b++)
      {
        file_out << bid[*b] << "\n";
      }
      for(std::set<Region*>::const_iterator r2=BWTA_Result::regions.begin();r2!=BWTA_Result::regions.end();r2++)
      {
        int connected=0;
        if ((*r)->isReachable(*r2))
          connected=1;
        file_out << connected << "\n";
      }
    }
    for(int x=0;x<(int)BWTA_Result::getRegion.getWidth();x++)
    {
      for(int y=0;y<(int)BWTA_Result::getRegion.getHeight();y++)
      {
        if (BWTA_Result::getRegion[x][y]==NULL)
          file_out << "-1\n";
        else
          file_out << rid[BWTA_Result::getRegion[x][y]] << "\n";
      }
    }
    for(int x=0;x<(int)BWTA_Result::getChokepoint.getWidth();x++)
    {
      for(int y=0;y<(int)BWTA_Result::getChokepoint.getHeight();y++)
      {
        if (BWTA_Result::getChokepoint[x][y]==NULL)
          file_out << "-1\n";
        else
          file_out << cid[BWTA_Result::getChokepoint[x][y]] << "\n";
      }
    }
    for(int x=0;x<(int)BWTA_Result::getBaseLocation.getWidth();x++)
    {
      for(int y=0;y<(int)BWTA_Result::getBaseLocation.getHeight();y++)
      {
        if (BWTA_Result::getBaseLocation[x][y]==NULL)
          file_out << "-1\n";
        else
          file_out << bid[BWTA_Result::getBaseLocation[x][y]] << "\n";
      }
    }
    for(int x=0;x<(int)BWTA_Result::getChokepointW.getWidth();x++)
    {
      for(int y=0;y<(int)BWTA_Result::getChokepointW.getHeight();y++)
      {
        if (BWTA_Result::getChokepointW[x][y]==NULL)
          file_out << "-1\n";
        else
          file_out << cid[BWTA_Result::getChokepointW[x][y]] << "\n";
      }
    }
    for(int x=0;x<(int)BWTA_Result::getBaseLocationW.getWidth();x++)
    {
      for(int y=0;y<(int)BWTA_Result::getBaseLocationW.getHeight();y++)
      {
        if (BWTA_Result::getBaseLocationW[x][y]==NULL)
          file_out << "-1\n";
        else
          file_out << bid[BWTA_Result::getBaseLocationW[x][y]] << "\n";
      }
    }
    for(int x=0;x<(int)BWTA_Result::getRegion.getWidth();x++)
    {
      for(int y=0;y<(int)BWTA_Result::getRegion.getHeight();y++)
      {
        if (BWTA_Result::getUnwalkablePolygon[x][y]==NULL)
          file_out << "-1\n";
        else
          file_out << pid[BWTA_Result::getUnwalkablePolygon[x][y]] << "\n";
      }
    }
    file_out.close();
  }
  /*void save_data_xml()
  {
    std::map<Polygon*,int> pid;
    std::map<BaseLocation*,int> bid;
    std::map<Chokepoint*,int> cid;
    std::map<Region*,int> rid;
    int p_id=0;
    int b_id=0;
    int c_id=0;
    int r_id=0;
    for(std::set<Polygon*>::const_iterator p=BWTA_Result::unwalkablePolygons.begin();p!=BWTA_Result::unwalkablePolygons.end();p++)
    {
      pid[*p]=p_id++;
    }
    for(std::set<BaseLocation*>::const_iterator b=BWTA_Result::baselocations.begin();b!=BWTA_Result::baselocations.end();b++)
    {
      bid[*b]=b_id++;
    }
    for(std::set<Chokepoint*>::const_iterator c=BWTA_Result::chokepoints.begin();c!=BWTA_Result::chokepoints.end();c++)
    {
      cid[*c]=c_id++;
    }
    for(std::set<Region*>::const_iterator r=BWTA_Result::regions.begin();r!=BWTA_Result::regions.end();r++)
    {
      rid[*r]=r_id++;
    }
    TiXmlDocument doc;
    TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
    TiXmlElement * element = new TiXmlElement( "BWTA" );
    TiXmlElement * unwalkable_polygons = new TiXmlElement( "unwalkable_polygons" );


    for(std::set<Polygon*>::const_iterator p=BWTA_Result::unwalkablePolygons.begin();p!=BWTA_Result::unwalkablePolygons.end();p++)
    {
      TiXmlElement * polygon = new TiXmlElement( "unwalkable_polygon" );
      polygon->SetAttribute("id",pid[*p]);
      TiXmlElement * vertex_list = new TiXmlElement( "vertex_list" );
      for(unsigned int i=0;i<(*p)->size();i++)
      {
        TiXmlElement * vertex = new TiXmlElement( "vertex" );
        vertex->SetAttribute("x",(**p)[i].x()*8);
        vertex->SetAttribute("y",(**p)[i].y()*8);
        vertex_list->LinkEndChild(vertex);
      }
      polygon->LinkEndChild(vertex_list);
      TiXmlElement * holes = new TiXmlElement( "holes" );
      for(std::vector<Polygon>::const_iterator h=(*p)->getHoles().begin();h!=(*p)->getHoles().end();h++)
      {
        TiXmlElement * vertex_list = new TiXmlElement( "vertex_list" );
        for(unsigned int i=0;i<(*h).size();i++)
        {
          TiXmlElement * vertex = new TiXmlElement( "vertex" );
          vertex->SetAttribute("x",(*h)[i].x()*8);
          vertex->SetAttribute("y",(*h)[i].y()*8);
          vertex_list->LinkEndChild(vertex);
        }
        holes->LinkEndChild(vertex_list);
      }
      polygon->LinkEndChild(holes);
      unwalkable_polygons->LinkEndChild(polygon);
    }
    element->LinkEndChild(unwalkable_polygons);

    TiXmlElement * regions = new TiXmlElement( "regions" );
    for(std::set<Region*>::const_iterator r=BWTA_Result::regions.begin();r!=BWTA_Result::regions.end();r++)
    {
      TiXmlElement * region = new TiXmlElement( "region" );
      region->SetAttribute("id",rid[*r]);
      TiXmlElement * center = new TiXmlElement( "center" );
      center->SetAttribute("x", (*r)->getCenter().x());
      center->SetAttribute("y", (*r)->getCenter().y());
      region->LinkEndChild(center);
      Polygon poly=(*r)->getPolygon();
      TiXmlElement * vertex_list = new TiXmlElement( "vertex_list" );
      for(unsigned int i=0;i<poly.size();i++)
      {
        TiXmlElement * vertex = new TiXmlElement( "vertex" );
        vertex->SetAttribute("x",poly[i].x());
        vertex->SetAttribute("y",poly[i].y());
        vertex_list->LinkEndChild(vertex);
      }
      region->LinkEndChild(vertex_list);
      TiXmlElement * chokepoints = new TiXmlElement( "chokepoints" );
      for(std::set<Chokepoint*>::const_iterator c=(*r)->getChokepoints().begin();c!=(*r)->getChokepoints().end();c++)
      {
        TiXmlElement * chokepoint = new TiXmlElement( "chokepoint" );
        chokepoint->SetAttribute("id",cid[*c]);
        chokepoints->LinkEndChild(chokepoint);
      }
      region->LinkEndChild(chokepoints);
      TiXmlElement * base_locations = new TiXmlElement( "base_locations" );
      for(std::set<BaseLocation*>::const_iterator b=(*r)->getBaseLocations().begin();b!=(*r)->getBaseLocations().end();b++)
      {
        TiXmlElement * base_location = new TiXmlElement( "base_location" );
        base_location->SetAttribute("id",bid[*b]);
        base_locations->LinkEndChild(base_location);
      }
      region->LinkEndChild(base_locations);
      regions->LinkEndChild(region);
    }
    element->LinkEndChild(regions);

    TiXmlElement * chokepoints = new TiXmlElement( "chokepoints" );
    for(std::set<Chokepoint*>::const_iterator c=BWTA_Result::chokepoints.begin();c!=BWTA_Result::chokepoints.end();c++)
    {
      TiXmlElement * chokepoint = new TiXmlElement( "chokepoint" );
      chokepoint->SetAttribute("id",cid[*c]);
      chokepoint->SetAttribute("width",(int)(*c)->getWidth());
      TiXmlElement * center = new TiXmlElement( "center" );
      center->SetAttribute("x",(*c)->getCenter().x());
      center->SetAttribute("y",(*c)->getCenter().y());
      chokepoint->LinkEndChild(center);
      TiXmlElement * regions = new TiXmlElement( "regions" );
      regions->SetAttribute("a",rid[(*c)->getRegions().first]);
      regions->SetAttribute("b",rid[(*c)->getRegions().second]);
      chokepoint->LinkEndChild(regions);
      TiXmlElement * side_a = new TiXmlElement( "side_a" );
      side_a->SetAttribute("x",(*c)->getSides().first.x());
      side_a->SetAttribute("y",(*c)->getSides().first.y());
      chokepoint->LinkEndChild(side_a);
      TiXmlElement * side_b = new TiXmlElement( "side_b" );
      side_b->SetAttribute("x",(*c)->getSides().second.x());
      side_b->SetAttribute("y",(*c)->getSides().second.y());
      chokepoint->LinkEndChild(side_b);
      chokepoints->LinkEndChild(chokepoint);
    }
    element->LinkEndChild(chokepoints);

    TiXmlElement * base_locations = new TiXmlElement( "base_locations" );
    for(std::set<BaseLocation*>::const_iterator b=BWTA_Result::baselocations.begin();b!=BWTA_Result::baselocations.end();b++)
    {
      TiXmlElement * base_location = new TiXmlElement( "base_location" );
      base_location->SetAttribute("id",bid[*b]);
      base_location->SetAttribute("region",rid[(*b)->getRegion()]);
      base_location->SetAttribute("is_island",(*b)->isIsland());
      base_location->SetAttribute("is_start_location",(*b)->isStartLocation());
      TiXmlElement * position = new TiXmlElement( "position" );
      position->SetAttribute("x",(*b)->getPosition().x());
      position->SetAttribute("y",(*b)->getPosition().y());
      base_location->LinkEndChild(position);
      TiXmlElement * tile_position = new TiXmlElement( "tile_position" );
      tile_position->SetAttribute("x",(*b)->getTilePosition().x());
      tile_position->SetAttribute("y",(*b)->getTilePosition().y());
      base_location->LinkEndChild(tile_position);
      base_locations->LinkEndChild(base_location);
    }
    element->LinkEndChild(base_locations);

    doc.LinkEndChild( decl );
    doc.LinkEndChild( element );
    string filename = string("bwapi-data/BWTA/")+Broodwar->mapFileName()+".xml";
    doc.SaveFile(filename.c_str());


    {
      ofstream myfile;
      std::string filename("bwapi-data/BWTA/");
      filename+=Broodwar->mapFileName();
      filename+="-raw.xml";
      myfile.open (filename.c_str());
      myfile << "<?xml version=\"1.0\" ?>\n";
      myfile << "<map width=\"" << Broodwar->mapWidth() << "\" height=\"" << Broodwar->mapHeight() << "\" name=\"" << Broodwar->mapName() << "\" hash=\"" << Broodwar->mapHash() << "\">\n";
      myfile << "  <start_locations>\n";
      for each(TilePosition tp in Broodwar->getStartLocations())
      {
        int tx=tp.x();
        int ty=tp.y();
        myfile << "    <start_location x=\"" << tx << "\" y=\"" << ty << "\">\n";
      }
      myfile << "  </start_locations>\n";
      myfile << "  <minerals>\n";
      for each(Unit* mineral in Broodwar->getMinerals())
      {
        int tx=mineral->getTilePosition().x();
        int ty=mineral->getTilePosition().y();
        myfile << "    <mineral x=\"" << tx << "\" y=\"" << ty << "\" resources=\"" << mineral->getResources() << "\">\n";
      }
      myfile << "  </minerals>\n";
      myfile << "  <geysers>\n";
      for each(Unit* geyser in Broodwar->getGeysers())
      {
        int tx=geyser->getTilePosition().x();
        int ty=geyser->getTilePosition().y();
        myfile << "    <geyser x=\"" << tx << "\" y=\"" << ty << "\" resources=\"" << geyser->getResources() << "\">\n";
      }
      myfile << "  </geysers>\n";
      myfile << "  <buildability>\n";
      for(int y=0;y<Broodwar->mapHeight();y++)
      {
        std::string line;
        for(int x=0;x<Broodwar->mapWidth();x++)
        {
          if (Broodwar->isBuildable(x,y))
            line+=" ";
          else
            line+="X";
        }
        myfile << "    <row data=\"" << line << "\" />\n";
      }
      myfile << "  </buildability>\n";
      myfile << "  <walkability>\n";
      for(int y=0;y<Broodwar->mapHeight()*4;y++)
      {
        std::string line;
        for(int x=0;x<Broodwar->mapWidth()*4;x++)
        {
          if (Broodwar->isWalkable(x,y))
            line+=" ";
          else
            line+="X";
        }
        myfile << "    <row data=\"" << line << "\" />\n";
      }
      myfile << "  </walkability>\n";
      myfile.close();
    }
  }*/
}