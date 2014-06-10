#ifndef _STDAFX_H_
#define _STDAFX_H_

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#pragma warning(disable:4503)   // Disable warning http://msdn.microsoft.com/en-us/library/074af4b6%28VS.80%29.aspx
//#define BOOST_ALL_DYN_LINK

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

// Typedefs
typedef unsigned __int8  u8 ;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

typedef signed   __int8  s8 ;
typedef signed   __int16 s16;
typedef signed   __int32 s32;
typedef signed   __int64 s64;
typedef u16 TileID;

#endif