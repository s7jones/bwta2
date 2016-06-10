#pragma once

#ifdef DEBUG_DRAW
#include "Painter.h"
#endif

#include "Color.h"
#include "VertexData.h"
#include "functions.h"
#include "Graph.h"
#include "Node.h"
#include "BWTA.h"
#include "Globals.h"
#include "RegionImpl.h"
#include "ChokepointImpl.h"
#include "BaseLocationImpl.h"
#include "find_base_locations.h"
#include "extract_polygons.h"
#include "BWTA_Result.h"
#include "MapData.h"
#include "Heap.h"
#include "Timer.h"

namespace BWTA
{
  void analyze();
  void analyze_map();
  void load_data(std::string filename);
  void save_data(std::string filename);
  void save_data_xml();

  void simplify_voronoi_diagram(Arrangement_2* arr_ptr, std::map<Point, double, ptcmp>* distance);
  void identify_region_nodes(Arrangement_2* arr_ptr,Graph* g_ptr);
  void identify_chokepoint_nodes(Graph* g_ptr, std::map<Point, double, ptcmp>* distance, std::map<Point, std::set< Point >, ptcmp >* nearest);
  double calculate_merge_value(Node* c);
  void merge_adjacent_regions(Graph* g_ptr);
  void remove_voronoi_diagram_from_arrangement(Arrangement_2* arr_ptr);
  void wall_off_chokepoints(Graph* g_ptr,Arrangement_2* arr_ptr);
}