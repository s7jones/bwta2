#ifndef _STDAFX_H_
#define _STDAFX_H_

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define NOMINMAX

// standard includes
#include <vector>
#include <map>
#include <list>
#include <set>
#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <sstream>

// BWAPI
#include <BWAPI.h>

// BOOST
#include <boost/filesystem.hpp>

// CGAL
#include <CGAL/MP_Float.h>
#include <CGAL/Quotient.h>
#include <CGAL/basic.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Filtered_kernel.h>
#include <CGAL/Lazy_exact_nt.h>
#include <CGAL/Segment_Delaunay_graph_traits_2.h>
#include <CGAL/Segment_Delaunay_graph_2.h>
#include <CGAL/Line_2.h>
#include <CGAL/Direction_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Point_set_2.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Arr_simple_point_location.h>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Gmpq.h>

#define PI 3.1415926

// Internal utilities
#include "Timer.h"

// Typedefs
using TileID = uint16_t;

#endif