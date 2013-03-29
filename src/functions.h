#pragma once

#define DEBUG_DRAW 1
#define DRAW_COLOR 1

#include <boost/format.hpp>
#include <CGAL/MP_Float.h>
#include <CGAL/Quotient.h>
#include <vector>
#include <map>
#include <list>

#include <CGAL/basic.h>
// standard includes
#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <sstream>
// define the kernel

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
#include "VertexData.h"
#include <BWTA/RectangleArray.h>
#include <BWAPI.h>
#include <BWTA/BaseLocation.h>
#include <BWTA/Polygon.h>
#ifdef DEBUG_DRAW
  #include <QtGui>
  #include <CGAL/Qt/GraphicsViewNavigation.h>
  #include <QLineF>
  #include <QRectF>
#endif
#define BWTA_FILE_VERSION 6
namespace BWTA
{
  #define PI 3.1415926
  #define USE_EXACT
  typedef CGAL::Lazy_exact_nt<CGAL::Gmpq > NumberType;
  #ifdef USE_EXACT
  #else
  //  typedef double NumberType;
  #endif
  typedef CGAL::Simple_cartesian< NumberType >    CK;
  typedef CGAL::Filtered_kernel<CK>         Kernel;

  typedef CGAL::Simple_cartesian< double >    CKD;
  typedef CGAL::Filtered_kernel<CKD>         KernelD;
  // typedefs for the traits and the algorithm

  typedef CGAL::Segment_Delaunay_graph_traits_2<Kernel> Gt;
  typedef CGAL::Segment_Delaunay_graph_traits_2<KernelD> GtD;

#if 0
  typedef CGAL::Segment_Delaunay_graph_2<GtD> SDG2;
#else
  // This makes some protected members of the base class public because get_voronoi_edges requires these.
  class SDG2: public CGAL::Segment_Delaunay_graph_2<GtD>
  {
    private:
      typedef CGAL::Segment_Delaunay_graph_2<GtD> Base;

    public:
      typedef Base::Construct_sdg_bisector_segment_2 Construct_sdg_bisector_segment_2;

      inline Construct_sdg_bisector_segment_2
      construct_sdg_bisector_segment_2_object() const {
        return Base::construct_sdg_bisector_segment_2_object();
      }
  };
#endif

  typedef CGAL::Segment_Delaunay_graph_site_2< CKD > SDGS2;
  typedef CGAL::Point_2<CK> Point;
  typedef CGAL::Polygon_2<CK> PolygonCK;
  typedef CGAL::Line_2<CK> Line;
  typedef CGAL::Segment_2<CK> Segment;
  typedef CGAL::Circle_2<CK> Circle;
  typedef CGAL::Direction_2<CK> Direction;
  typedef CGAL::Point_set_2<Kernel> Point_set_2;
  typedef CGAL::Point_set_2<Kernel>::Vertex_handle  Vertex_handle;

  typedef CGAL::Point_2<CKD> PointD;
  typedef CGAL::Polygon_2<CKD> PolygonD;
  typedef CGAL::Line_2<CKD> LineD;
  typedef CGAL::Ray_2<CKD> RayD;
  typedef CGAL::Segment_2<CKD> SegmentD;
  typedef CGAL::Circle_2<CKD> CircleD;
  typedef CGAL::Direction_2<CKD> DirectionD;

  typedef CGAL::Arr_segment_traits_2<Kernel>                      Traits_2;
  typedef Traits_2::Point_2                                       Point_2;
  typedef Traits_2::X_monotone_curve_2                            Segment_2;
  typedef CGAL::Arr_extended_dcel<Traits_2,VertexData, Color, int>      Dcel;
  typedef CGAL::Arrangement_2<Traits_2, Dcel>                     Arrangement_2;
  struct ptcmp
  {
    bool operator()(const Point &a, const Point &b) const
    {
      return a.x()<b.x() || (a.x() == b.x() && a.y()<b.y());
    }
  };

  struct vhcmp
  {
    bool operator()(const Arrangement_2::Vertex_handle &a, const Arrangement_2::Vertex_handle &b) const
    {
      return a->point().x()<b->point().x() || (a->point().x() == b->point().x() && a->point().y()<b->point().y());
    }
  };

  struct vhradiuscmp
  {
    bool operator()(const Arrangement_2::Vertex_handle &a, const Arrangement_2::Vertex_handle &b) const
    {
      return a->data().radius<b->data().radius || (a->data().radius==b->data().radius && (a->point()<b->point()));
    }
  };

  void get_voronoi_edges(SDG2 &sdg, std::vector<Segment> &voronoi_diagram_edges, std::map<Point, std::set< Point >, ptcmp> &nearest, std::map<Point,double, ptcmp> &distance, const std::vector<Polygon> &polygons);

  double get_distance(Point a, Point b);
  double get_distance(PointD a, PointD b);
  double cast_to_double( double q);
  double cast_to_double( CGAL::MP_Float q);
  double cast_to_double( CGAL::Quotient<CGAL::MP_Float> q);
  double cast_to_double( CGAL::Gmpq q);
  double cast_to_double( CGAL::Lazy_exact_nt<CGAL::Gmpq > q);
  bool is_real( double q);
  bool is_real( CGAL::MP_Float q);
  bool is_real( CGAL::Quotient<CGAL::MP_Float> q);
  bool is_real( CGAL::Gmpq q);
  bool is_real( CGAL::Lazy_exact_nt<CGAL::Gmpq > q);
  bool load_map();
  bool load_resources();
  int str2int(std::string str);
  std::string int2str(int number);
  int max(int a, int b);
  int min(int a, int b);
  void log(const char* text, ...);
  void writeFile(const char* filename, const char* text, ...);

  template< class T>
  double get_distance(CGAL::Point_2<T> a, CGAL::Point_2<T> b)
  {
    return sqrt(to_double((a.x()-b.x())*(a.x()-b.x())+(a.y()-b.y())*(a.y()-b.y())));
  }
  double distance_to_border(Polygon& polygon,int width, int height);

  void calculate_walk_distances(const RectangleArray<bool> &read_map
                               ,const BWAPI::Position &start
                               ,int max_distance
                               ,RectangleArray<int> &distance_map);

  void calculate_walk_distances_area(const BWAPI::Position &start
                                    ,int width
                                    ,int height
                                    ,int max_distance
                                    ,RectangleArray<int> &distance_map);

  int get_set(std::vector<int> &a,int i);
  template <class _Tp1>
  _Tp1 get_set2(std::map<_Tp1,_Tp1> &a,_Tp1 i)
  {
    if (a.find(i)==a.end()) a[i]=i;
    if (i==a[i]) return i;
    a[i]=get_set2(a,a[i]);
    return a[i];
  }
  void calculate_connectivity();

  float max(float a, float b);
  float min(float a, float b);

  double max(double a, double b);
  double min(double a, double b);
  bool fileExists(std::string filename);
  int fileVersion(std::string filename);

  double AstarSearchDistance(BWAPI::TilePosition start, BWAPI::TilePosition end);
  std::pair<BWAPI::TilePosition,double> AstarSearchDistance(BWAPI::TilePosition start, std::set<BWAPI::TilePosition>& end);
  std::map<BWAPI::TilePosition,double> AstarSearchDistanceAll(BWAPI::TilePosition start, std::set<BWAPI::TilePosition>& end);
  std::vector<BWAPI::TilePosition> AstarSearchPath(BWAPI::TilePosition start, BWAPI::TilePosition end);
  std::vector<BWAPI::TilePosition> AstarSearchPath(BWAPI::TilePosition start, std::set<BWAPI::TilePosition> end);
  
#ifdef DEBUG_DRAW
  QColor hsl2rgb(double h, double sl, double l);
#endif
}