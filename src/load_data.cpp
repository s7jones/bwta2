#include "functions.h"
#include "BWTA_Result.h"
#include "BaseLocationImpl.h"
#include "ChokepointImpl.h"
#include "RegionImpl.h"
#include "find_base_locations.h"
#include "MapData.h"
#include "terrain_analysis.h"

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
    MapData::distanceTransform.resize(width, height);

    // copy buildability data into buildability array
    for(int x=0;x<b_width;x++) {
      for(int y=0;y<b_height;y++) {
        MapData::buildability[x][y]=BWAPI::Broodwar->isBuildable(x,y);
        MapData::lowResWalkability[x][y]=true;
      }
    }
    // copy and simplify walkability data as it is copies into walkability array
	  // init distance transform map
    for(int x=0;x<width;x++) {
      for(int y=0;y<height;y++) {
        MapData::rawWalkability[x][y]=BWAPI::Broodwar->isWalkable(x,y);
        MapData::walkability[x][y]=true;
		
		    if (BWAPI::Broodwar->isWalkable(x, y)) {
			    if (x==0 || x==width-1 || y==0 || y==height-1){
				MapData::distanceTransform[x][y] = 1;
			} else {
				MapData::distanceTransform[x][y] = -1;
			}
		} else {
			MapData::distanceTransform[x][y] = 0;
		}
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

	//Block static neutral units
	int x1,y1,x2,y2;
	BWAPI::UnitType unitType;
	std::set<BWAPI::Unit*>::iterator unit;
	std::set<BWAPI::Unit*> neutralUnits = BWAPI::Broodwar->getStaticNeutralUnits();
	for(unit = neutralUnits.begin(); unit != neutralUnits.end(); ++unit) {
		// check if it is a resource container
		unitType = (*unit)->getType();
		if (unitType == BWAPI::UnitTypes::Resource_Vespene_Geyser || unitType.isMineralField()) continue;
		// get build area
		x1 = (*unit)->getTilePosition().x()*4;
		y1 = (*unit)->getTilePosition().y()*4;
		x2 = x1 + unitType.tileWidth()*4;
		y2 = y1 + unitType.tileHeight()*4;
		// sanitize
		if (x1 < 0) x1 = 0;
		if (y1 < 0) y1 = 0;
		if (x2 >= width) x2 = width-1;
		if (y2 >= height) y2 = height-1;
		// map area
		for (int x = x1; x <= x2; x++) {
			for (int y = y1; y <= y2; y++) {
				for(int x3=max(x-1,0);x3<=min(width-1,x+1);x3++) {
					for(int y3=max(y-1,0);y3<=min(height-1,y+1);y3++) {
						MapData::walkability[x3][y3] = false;
					}
				}
				MapData::distanceTransform[x][y] = 0;
				MapData::lowResWalkability[x/4][y/4] = false;
			}
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
}