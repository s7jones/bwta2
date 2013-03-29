#include <BWTA/RectangleArray.h>
#include <list>
#include <vector>
#include <set>
#include <BWAPI.h>
#include <BWTA.h>
#include "ConnectedComponent.h"
namespace BWTA
{
  void find_mineral_clusters(const RectangleArray<bool> &simplified_map
                            ,const std::set< BWAPI::Unit* > &minerals
                            ,const std::set< BWAPI::Unit* > &geysers
                            ,std::vector< std::vector< BWAPI::Unit* > > &resource_clusters);

  void calculate_base_build_map(const RectangleArray<bool> &build_map
                               ,const std::vector< std::vector< BWAPI::Unit* > > &resource_clusters
                               ,RectangleArray<bool> &base_build_map);

  void calculate_base_locations(const RectangleArray<bool> &simplified_map
                               ,const RectangleArray<bool> &base_build_map
                               ,const std::vector< std::vector< BWAPI::Unit* > > &resource_clusters
                               ,std::set< BWTA::BaseLocation* > &base_locations);

  void attach_resources_to_base_locations(std::set< BWTA::BaseLocation* > &base_locations);

  void calculate_base_location_properties(const RectangleArray<ConnectedComponent*> &get_component
                                         ,const std::list<ConnectedComponent> &components
                                         ,std::set< BWTA::BaseLocation* > &base_locations);
}