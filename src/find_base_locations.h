#include <BWTA/RectangleArray.h>
#include <BWTA.h>

#include "ConnectedComponent.h"
#include "MapData.h"

namespace BWTA
{
  void findMineralClusters(const RectangleArray<bool> &simplified_map,
	  std::vector<UnitTypeWalkPosition> resources,
	  std::vector< std::vector< UnitTypeWalkPosition > > &resourceClusters);

  void calculateBaseBuildMap(const RectangleArray<bool> &build_map, 
	  const std::vector< std::vector< UnitTypeWalkPosition > > &resourceClusters,
	  RectangleArray<bool> &base_build_map);

  void calculateBaseLocations(const RectangleArray<bool> &simplified_map,
	  const RectangleArray<bool> &base_build_map,
	  const std::vector< std::vector< UnitTypeWalkPosition > > &resourceClusters,
	  std::set< BWTA::BaseLocation* > &base_locations);

  void attachResourcePointersToBaseLocations(std::set< BWTA::BaseLocation* > &baseLocations);

  void calculate_base_location_properties(const RectangleArray<ConnectedComponent*> &get_component
                                         ,const std::list<ConnectedComponent> &components
                                         ,std::set< BWTA::BaseLocation* > &base_locations);
}