#pragma once;
#include "ConnectedComponent.h"
#include "functions.h"
#include <BWTA/Polygon.h>
namespace BWTA
{
  void find_connected_components(const RectangleArray<bool> &simplified_map
                                ,RectangleArray<ConnectedComponent*> &get_component
                                ,std::list<ConnectedComponent> &components);
  void extract_polygons(const RectangleArray<bool> &walkability
                       ,const std::list<ConnectedComponent> &components
                       ,std::vector<Polygon> &polygons);

  void flood_fill_with_component(const RectangleArray<bool> &read_map
                                ,const PointD start
                                ,ConnectedComponent* component
                                ,int fill_type
                                ,RectangleArray<ConnectedComponent*> &write_map);
}