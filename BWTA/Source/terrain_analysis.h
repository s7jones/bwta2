#pragma once

#ifdef DEBUG_DRAW
#include "Painter.h"
#endif

#include "functions.h"
#include "BWTA.h"
#include "Globals.h"
#include "RegionImpl.h"
#include "ChokepointImpl.h"
#include "BaseLocationImpl.h"
#include "BaseLocationGenerator.h"
#include "BWTA_Result.h"
#include "MapData.h"
#include "Heap.h"
#include "PolygonGenerator.h"
#include "RegionGenerator.h"
#include "GraphColoring.h"
#include "ClosestObjectMap.h"

namespace BWTA
{
  void analyze();
  void analyze_map();
  void load_data(std::string filename);
  void save_data(std::string filename);
  void save_data_xml();
}