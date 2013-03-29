#include "functions.h"
#include <BWAPI.h>
#include <BWTA.h>
#include "BWTA_Result.h"
#include "BaseLocationImpl.h"
#include "RegionImpl.h"
#include "ConnectedComponent.h"
#include "MapData.h"
namespace BWTA
{
  // calculate_base_build_map
  // This clusters the minerals using a simple algorithm where every pair of resources within
  // a minimum distance get put into the same cluster. It also discards clusters that are too small
  // Inputs: simplified_map - the walkability data to build tile resolution
  //         minerals - the set of minerals to cluster
  //         geysers - the set of geysers to cluster
  // Output: resource_clusters - a vector of resource clusters. A resource cluster is a vector of units

  void find_mineral_clusters(const RectangleArray<bool> &simplified_map
                            ,const std::set< BWAPI::Unit* > &minerals
                            ,const std::set< BWAPI::Unit* > &geysers
                            ,std::vector< std::vector< BWAPI::Unit* > > &resource_clusters)
  {
    //minerals less than this distance will be grouped into the same cluster
    int mineral_cluster_distance=6*4;

    //a cluster with less than 4 mineral patches will be considered useless
    int mineral_group_discard_size=3;


    RectangleArray<int> distance_map;
    std::vector<BWAPI::Unit*> resources;
    //put all minerals and geysers into the resources vector
    for(std::set<BWAPI::Unit*>::const_iterator i=minerals.begin();i!=minerals.end();i++)
    {
      resources.push_back(*i);
    }
    for(std::set<BWAPI::Unit*>::const_iterator i=geysers.begin();i!=geysers.end();i++)
    {
      resources.push_back(*i);
    }
    
    distance_map.resize(simplified_map.getWidth(),simplified_map.getHeight());

    //we will now group the minerals into clusters based on distance
    //we begin by putting each mineral in its own group
    std::vector<int> resource_set;
    for(unsigned int i=0;i<resources.size();i++) {
      resource_set.push_back(i);
    }
    //we will now join sets of minerals that are close together
    for(unsigned int index=0;index<resources.size();index++) {
      BWAPI::Position pos=BWAPI::Position(resources[index]->getInitialTilePosition().x()*4+4,resources[index]->getInitialTilePosition().y()*4+2);
      if (resources[index]->getInitialType()==BWAPI::UnitTypes::Resource_Vespene_Geyser) {
        pos=BWAPI::Position(resources[index]->getInitialTilePosition().x()*4+8,resources[index]->getInitialTilePosition().y()*4+4);
      }
      //this will flood the nearby tiles with their distances to the current mineral
      calculate_walk_distances(simplified_map,pos,(mineral_cluster_distance+4*4)*10,distance_map);
      //lets look at some other minerals and see if they are close enough to
      //merge with this one
      for(unsigned int index2=index+1;index2<resources.size();index2++) {
        BWAPI::Position pos2=BWAPI::Position(resources[index2]->getInitialTilePosition().x()*4+4,resources[index2]->getInitialTilePosition().y()*4+2);
        if (resources[index2]->getInitialType()==BWAPI::UnitTypes::Resource_Vespene_Geyser) {
          pos2=BWAPI::Position(resources[index2]->getInitialTilePosition().x()*4+8,resources[index2]->getInitialTilePosition().y()*4+4);
        }
        //merge only if less than this distance
        int x2=(int)pos2.x();
        int y2=(int)pos2.y();
        int dist=distance_map[x2][y2];
        int augmented_cluster_distance=mineral_cluster_distance;

        if (resources[index]->getInitialType()==BWAPI::UnitTypes::Resource_Vespene_Geyser || resources[index2]->getInitialType()==BWAPI::UnitTypes::Resource_Vespene_Geyser) {
          //vespene geysers are often farther away from minerals than minerals 
          //are from each other. So we add some extra distance for vespene geyers
          augmented_cluster_distance+=3*4;
        }
        if (dist>=0 && dist<augmented_cluster_distance*10) {
          resource_set[get_set(resource_set,index2)]=get_set(resource_set,index);
        }
      }
    }

    //we will now map the cluster IDs to indices
    //first we will find the unique clusters
    std::map<int,int> unique_clusters;
    for(unsigned int index=0;index<resources.size();index++) {
      int set_index=get_set(resource_set,index);
      unique_clusters.insert(std::make_pair(set_index,0));
    }
    //now we will set their index values
    int index=0;
    for(std::map<int,int>::iterator iter=unique_clusters.begin();
      iter!=unique_clusters.end();iter++) {
      (*iter).second=index;
      index++;
    }
    //here we make the resource clusters vector of vectors large enough
    for(unsigned int i=0;i<unique_clusters.size();i++) {
      std::vector<BWAPI::Unit*> b;
      resource_clusters.push_back(b);
    }
    //fill the resource clusters array
    for(unsigned int index=0;index<resources.size();index++) {
      int set_index=(*unique_clusters.find(get_set(resource_set,index))).second;
      resource_clusters[set_index].push_back(resources[index]);
    }
    //remove resource clusters that are too small
    for(unsigned int index=0;index<resource_clusters.size();) {
      if ((int)resource_clusters[index].size()<=mineral_group_discard_size) {
        resource_clusters.erase(resource_clusters.begin()+index);
      } else {
        index++;
      }
    }
  }

  // calculate_base_build_map
  // Inputs: build_map - the buildability data for each tile.
  //         resource_clusters - the clusters of resources that concern us
  // Output: base_build_map - the buildability data for each tile for a resource depot,
  //                          taking into account the units given in resource_clusters

  void calculate_base_build_map(const RectangleArray<bool> &build_map
                               ,const std::vector< std::vector< BWAPI::Unit* > > &resource_clusters
                               ,RectangleArray<bool> &base_build_map)
  {
    // Base_build_map[x][y] is true if build_map[ix][iy] is true for all x<=ix<x+4 and all y<=yi<+3
    // 4 and 3 are the tile width and height of a command center/nexus/hatchery
    base_build_map.resize(build_map.getWidth(),build_map.getHeight());
    for(int x=0;x<(int)build_map.getWidth();x++) {
      for(int y=0;y<(int)build_map.getHeight();y++) {
        int max_x=min(x+4,(int)build_map.getWidth());
        int max_y=min(y+3,(int)build_map.getHeight());
        base_build_map[x][y]=true;
        for(int ix=x;ix<max_x;ix++) {
          for(int iy=y;iy<max_y;iy++) {
            // Set base_build_map[x][y] to false if build_map[ix][iy] is false
            base_build_map[x][y] &= build_map[ix][iy];
          }
        }

        // If this tile is too close to the bottom or right of the map, set it to false
        if (x+4>(int)build_map.getWidth() || y+3>(int)build_map.getHeight()) {
          base_build_map[x][y]=false;
        }
      }
    }
    // Set build tiles too close to resources in any cluster to false in base_build_map
    for(int i=0;i<(int)resource_clusters.size();i++) {
      // Set build tiles too close to resources in resource_clusters[i] to false in base_build_map
      for(int j=0;j<(int)resource_clusters[i].size();j++) {
        // Here we set build tiles too close to the unit resource_clusters[i][j] to false in base_build_map
        // Get the tile position of this unit
        int x=(int)resource_clusters[i][j]->getInitialTilePosition().x();
        int y=(int)resource_clusters[i][j]->getInitialTilePosition().y();

        // Minerals and geysers affect tiles differently
        if (resource_clusters[i][j]->getInitialType()==BWAPI::UnitTypes::Resource_Mineral_Field) {
          int min_x=max(x-6,0);
          int max_x=min(x+4,(int)build_map.getWidth()-1);
          int min_y=max(y-5,0);
          int max_y=min(y+3,(int)build_map.getHeight()-1);
          for(int ix=min_x;ix<=max_x;ix++) {
            for(int iy=min_y;iy<=max_y;iy++) {
              base_build_map[ix][iy]=false;
            }
          }
        } else {
          int min_x=max(x-6,0);
          int max_x=min(x+6,build_map.getWidth()-1);
          int min_y=max(y-5,0);
          int max_y=min(y+4,build_map.getHeight()-1);
          for(int ix=min_x;ix<=max_x;ix++) {
            for(int iy=min_y;iy<=max_y;iy++) {
              base_build_map[ix][iy]=false;
            }
          }
        }
      }
    }
  }


  // calculate_base_build_map
  // Inputs: simplified_map - the walkability data to build tile resolution
  //         base_build_map - a tile is true in this array if we can build a base on it
  //                             - computed in calculate_base_build_map
  //         resource_clusters - the resource clusters
  //                             - computed from find_mineral_clusters
  // Output: base_locations - the set of base locations for this map

  void calculate_base_locations(const RectangleArray<bool> &simplified_map
                               ,const RectangleArray<bool> &base_build_map
                               ,const std::vector< std::vector< BWAPI::Unit* > > &resource_clusters
                               ,std::set< BWTA::BaseLocation* > &base_locations)
  {
    //Now that we have the map of the locations where we can place a command center
    //And have the clusters of minerals, we will try to find a base location for each cluster
    int max_influence_distance=12;
    RectangleArray<double> tile_scores;
    RectangleArray<int> distance_map;
    tile_scores.resize(base_build_map.getWidth(),base_build_map.getHeight());
    distance_map.resize(simplified_map.getWidth(),simplified_map.getHeight());

    for(unsigned int i=0;i<resource_clusters.size();i++) {
      for(int x=0;x<(int)tile_scores.getWidth();x++) {
        for(int y=0;y<(int)tile_scores.getHeight();y++) {
          tile_scores[x][y]=0;
        }
      }
      for(unsigned int j=0;j<resource_clusters[i].size();j++) {
        BWAPI::Position pos(resource_clusters[i][j]->getInitialTilePosition().x()*4+4,resource_clusters[i][j]->getInitialTilePosition().y()*4+2);
        if (resource_clusters[i][j]->getInitialType()==BWAPI::UnitTypes::Resource_Vespene_Geyser) {
          pos=BWAPI::Position(resource_clusters[i][j]->getInitialTilePosition().x()*4+8,resource_clusters[i][j]->getInitialTilePosition().y()*4+4);
        }
        //this will flood the nearby tiles with their distances to the current mineral
        calculate_walk_distances(simplified_map,pos,max_influence_distance*4*10,distance_map);
        int x=(int)pos.x()/4;
        int y=(int)pos.y()/4;
        int min_x=max(x-max_influence_distance,0);
        int max_x=min(x+max_influence_distance,base_build_map.getWidth()-1);
        int min_y=max(y-max_influence_distance,0);
        int max_y=min(y+max_influence_distance,base_build_map.getHeight()-1);
        for(int ix=min_x;ix<=max_x;ix++) {
          for(int iy=min_y;iy<=max_y;iy++) {
            if (base_build_map[ix][iy]) {
              double distance=100000;
              for(int tx=ix*4;tx<(ix+4)*4;tx++) {
                for(int ty=iy*4;ty<(iy+3)*4;ty++) {
                  if (distance_map[tx][ty]>=0) {
                    distance=min(distance,(double)distance_map[tx][ty]);
                  }
                }
              }
              if (distance>=0) {
                double score=20.0*4*10.0-distance;
                tile_scores[ix][iy]+=score;
              }
            }
          }
        }
      }
      BWAPI::TilePosition maximum(-1,-1);
      double max_score=0;
      for(int x=0;x<(int)tile_scores.getWidth();x++) {
        for(int y=0;y<(int)tile_scores.getHeight();y++) {
          if (tile_scores[x][y]>max_score) {
            max_score=tile_scores[x][y];
            maximum=BWAPI::TilePosition(x,y);
          }
        }
      }
      if (max_score>0) {
        base_locations.insert(new BWTA::BaseLocationImpl(maximum));
      }
    }
  }

  void attach_resources_to_base_locations(std::set< BWTA::BaseLocation* > &base_locations)
  {
    //attach resources to base locations based on proximity (walk distance)
    RectangleArray<int> distance_map(MapData::mapWidth*4,MapData::mapHeight*4);
    for(std::set<BWTA::BaseLocation*>::iterator i=base_locations.begin();i!=base_locations.end();i++) {
      BWAPI::Position p((*i)->getTilePosition().x()*4,(*i)->getTilePosition().y()*4);
      calculate_walk_distances_area(p,16,12,10*4*10,distance_map);
      BWTA::BaseLocationImpl* ii=(BWTA::BaseLocationImpl*)(*i);
      for(std::set<BWAPI::Unit*>::const_iterator j=MapData::geysers.begin();j!=MapData::geysers.end();j++) {
        int x=(int)(*j)->getInitialTilePosition().x()*4+8;
        int y=(int)(*j)->getInitialTilePosition().y()*4+4;
        if (distance_map[x][y]>=0 && distance_map[x][y]<=4*10*10) {
          ii->geysers.insert(*j);
        }
      }
      for(std::set<BWAPI::Unit*>::const_iterator j=MapData::rawMinerals.begin();j!=MapData::rawMinerals.end();j++) {
        int x=(int)(*j)->getInitialTilePosition().x()*4+4;
        int y=(int)(*j)->getInitialTilePosition().y()*4+2;
        if (distance_map[x][y]>=0 && distance_map[x][y]<=4*10*10) {
          ii->staticMinerals.insert(*j);
        }
      }
    }
  }
  void calculate_base_location_properties(const RectangleArray<ConnectedComponent*> &get_component
                                         ,const std::list<ConnectedComponent> &components
                                         ,std::set< BWTA::BaseLocation* > &base_locations)
  {
    attach_resources_to_base_locations(base_locations);
    RectangleArray<double> distance_map;
    for(std::set<BWTA::BaseLocation*>::iterator i=base_locations.begin();i!=base_locations.end();i++) {
      BWAPI::Position p((*i)->getTilePosition().x()*32+64,(*i)->getTilePosition().y()*32+48);
      BWAPI::TilePosition tp((*i)->getTilePosition().x()+1,(*i)->getTilePosition().y()+1);
      BWTA::getGroundDistanceMap(tp,distance_map);
      BWTA::BaseLocationImpl* ii=(BWTA::BaseLocationImpl*)(*i);
      //assume the base location is an island unless we can walk from this base location to another base location
      ii->island = true;
      for(std::set<BWTA::BaseLocation*>::iterator j=base_locations.begin();j!=base_locations.end();j++) {
        if (*j==*i)
        {
          ii->ground_distances[*j]=0;
          ii->air_distances[*j]=0;
        }
        else
        {
          BWAPI::Position p2((*j)->getTilePosition().x()*32+64,(*j)->getTilePosition().y()*32+48);
          BWAPI::TilePosition tp2((*j)->getTilePosition().x()+1,(*j)->getTilePosition().y()+1);
          if (BWTA::isConnected(tp,tp2)) {
            ii->island=false;
          }
          ii->ground_distances[*j]=distance_map[tp2.x()][tp2.y()];
          ii->air_distances[*j]=p.getDistance(p2);
        }
      }
      ii->start = false;
      for(std::set<BWAPI::TilePosition>::iterator j=MapData::startLocations.begin();j!=MapData::startLocations.end();j++)
      {
        BWAPI::Position pos((*j).x()*32+64,(*j).y()*32+48);
        double distance=pos.getDistance((*i)->getPosition());
        if (distance<32*10)
          ii->start = true;
      }
      if (ii->start)
        BWTA::BWTA_Result::startlocations.insert(*i);

      //find what region this base location is in and tell that region about the base location
      for(std::set<BWTA::Region*>::iterator r=BWTA::BWTA_Result::regions.begin();r!=BWTA::BWTA_Result::regions.end();r++)
      {
        if ((*r)->getPolygon().isInside(ii->getPosition()))
        {
          ii->region=*r;
          ((BWTA::RegionImpl*)(*r))->baseLocations.insert(*i);
          break;
        }
      }

    }
  }
}