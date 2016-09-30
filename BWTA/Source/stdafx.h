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

#define PI 3.1415926

// Internal utilities
#include "Timer.h"
#include <BWTA/RectangleArray.h>

// Typedefs
using TileID = uint16_t;

// Temporal boost stuff
#pragma warning(disable: 4244) // known precision conversion warning with algorithms::buffer http://lists.boost.org/boost-users/2015/05/84281.php
#include <boost/geometry.hpp>
#pragma warning(default: 4244) 
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
// #include <boost/assign/std/vector.hpp>

#include <boost/polygon/voronoi.hpp>
#include <boost/polygon/polygon.hpp>

// geometry typedefs
typedef boost::geometry::model::d2::point_xy<double> BoostPoint;
typedef boost::geometry::model::segment<BoostPoint> BoostSegment;
using BoostSegmentI = std::pair < BoostSegment, std::size_t >;
typedef boost::geometry::model::polygon<BoostPoint> BoostPolygon;
typedef std::vector<BoostPoint> Contour;
// Voronoi typedefs
typedef boost::polygon::voronoi_diagram<double> BoostVoronoi;
typedef boost::int32_t int32;
typedef boost::polygon::point_data<int32> VoronoiPoint;
typedef boost::polygon::segment_data<int32> VoronoiSegment;

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

#ifdef OFFLINE
#define BWTA_PATH "logs/"
const std::string LOG_FILE_PATH = "logs/BWTA.log";
#define LOG(message) { std::cout << message << std::endl; }
#else
#define BWTA_PATH "bwapi-data/BWTA2/"
const std::string LOG_FILE_PATH = "bwapi-data/logs/BWTA.log";
#define LOG(message) { \
	  std::ofstream logFile(LOG_FILE_PATH , std::ios_base::out | std::ios_base::app ); \
	  logFile << message << std::endl; }
#endif

#endif