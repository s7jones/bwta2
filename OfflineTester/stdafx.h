#pragma message("Compiling precompiled headers (you should see this only once)")

#include <boost/filesystem.hpp>

// Typedefs
typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

typedef signed   __int8  s8;
typedef signed   __int16 s16;
typedef signed   __int32 s32;
typedef signed   __int64 s64;
typedef u16 TileID;

#pragma warning(disable: 4244) // known precision conversion warning with algorithms::buffer http://lists.boost.org/boost-users/2015/05/84281.php
#include <boost/geometry.hpp>
#pragma warning(default: 4244) 
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

// geometry typedefs
typedef boost::geometry::model::d2::point_xy<double> BoostPoint;
typedef boost::geometry::model::segment<BoostPoint> BoostSegment;
using BoostSegmentI = std::pair < BoostSegment, std::size_t > ;
typedef boost::geometry::model::polygon<BoostPoint> BoostPolygon;