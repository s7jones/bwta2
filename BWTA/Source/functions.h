#pragma once

#include "VertexData.h"
#include <BWTA/RectangleArray.h>
#include <BWTA/BaseLocation.h>
#include <BWTA/Polygon.h>

#define BWTA_FILE_VERSION 6
namespace BWTA
{
  // Choosing the proper kernel is important http://doc.cgal.org/latest/Kernel_23/index.html
  typedef CGAL::Lazy_exact_nt<CGAL::Gmpq >					NumberType;
  typedef CGAL::Exact_predicates_exact_constructions_kernel Kernel;
  typedef CGAL::Segment_Delaunay_graph_traits_2<Kernel>		Gt; 

  typedef CGAL::Segment_Delaunay_graph_2<Gt> SDG2;
  typedef Gt::Segment_2                      Segment_2_Gt;
  typedef CGAL::Parabola_segment_2<Gt>       Parabola_segment_2_Gt;

  typedef CGAL::Point_2<Kernel>      Point;
  typedef CGAL::Polygon_2<Kernel>    PolygonD;
  typedef CGAL::Line_2<Kernel>       Line;
  typedef CGAL::Segment_2<Kernel>    Segment;
  typedef CGAL::Point_set_2<Kernel>  Point_set_2;
  typedef Point_set_2::Vertex_handle Vertex_handle;

  typedef CGAL::Arr_segment_traits_2<Kernel>                       Traits_2;
  typedef Traits_2::Point_2                                        Point_2;
  typedef Traits_2::X_monotone_curve_2                             Segment_2;
  typedef CGAL::Arr_extended_dcel<Traits_2,VertexData, Color, int> Dcel;
  typedef CGAL::Arrangement_2<Traits_2, Dcel>                      Arrangement_2;

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

  bool createDir(std::string& path);
  double get_distance(Point a, Point b);
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
  void loadMapFromBWAPI();
  void loadMap();
  int str2int(std::string str);
  int max(int a, int b);
  int min(int a, int b);
  //void writeFile(const char* filename, const char* text, ...);
#ifdef OFFLINE
  #define BWTA_PATH "logs/"
  const std::string LOG_FILE_PATH = "logs/BWTA.log";
  #define log(message) { std::cout << message << std::endl; }
#else
  #define BWTA_PATH "bwapi-data/BWTA2/"
  const std::string LOG_FILE_PATH = "bwapi-data/logs/BWTA.log";
  #define log(message) { \
	  std::ofstream logFile(LOG_FILE_PATH , std::ios_base::out | std::ios_base::app ); \
	  logFile << message << std::endl; }
#endif

  template< class T>
  double get_distance(CGAL::Point_2<T> a, CGAL::Point_2<T> b)
  {
    return sqrt(to_double((a.x()-b.x())*(a.x()-b.x())+(a.y()-b.y())*(a.y()-b.y())));
  }
  double distance_to_border(Polygon& polygon,int width, int height);

  void calculateWalkDistances(const RectangleArray<bool> &read_map,
	  const BWAPI::WalkPosition &start,
	  int max_distance,
	  RectangleArray<int> &distance_map);

  void calculate_walk_distances_area(const BWAPI::Position &start
                                    ,int width
                                    ,int height
                                    ,int max_distance
                                    ,RectangleArray<int> &distance_map);

  int get_set(std::vector<int> &a,int i);

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
  
// #ifdef DEBUG_DRAW
//   QColor hsl2rgb(double h, double sl, double l);
// #endif
}