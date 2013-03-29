#include "functions.h"
#include <string>
#include <sstream>
#include "Heap.h"
#include <stdarg.h>
#include <sys/stat.h>
#include "MapData.h"
#include <BWTA/Polygon.h>

namespace BWTA
{
  double cast_to_double( double q)
  {
    return q;
  }
  double cast_to_double( CGAL::MP_Float q)
  {
    return CGAL::to_double(q);
  }
  double cast_to_double( CGAL::Quotient<CGAL::MP_Float> q)
  {
    return CGAL::INTERN_MP_FLOAT::to_double(q.numerator())/CGAL::INTERN_MP_FLOAT::to_double(q.denominator());
  }

  double cast_to_double( CGAL::Gmpq q)
  {
    return q.to_double();
  }
  double cast_to_double( CGAL::Lazy_exact_nt<CGAL::Gmpq > q)
  {
    return CGAL::to_double(q.approx());
  }
  bool is_real( double q)
  {
    return q+1>q;
  }
  bool is_real( CGAL::MP_Float q)
  {
    return q+1>q;
  }
  bool is_real( CGAL::Quotient<CGAL::MP_Float> q)
  {
    return q+1>q;
  }
  bool is_real( CGAL::Gmpq q)
  {
    return q+1>q;
  }
  bool is_real( CGAL::Lazy_exact_nt<CGAL::Gmpq > q)
  {
    return q+1>q;
  }
  #ifdef USE_EXACT
  double get_distance(Point a, Point b)
  {
    return sqrt(cast_to_double((a.x()-b.x())*(a.x()-b.x())+(a.y()-b.y())*(a.y()-b.y())));
  }
  #endif
  double get_distance(PointD a, PointD b)
  {
    return sqrt((a.x()-b.x())*(a.x()-b.x())+(a.y()-b.y())*(a.y()-b.y()));
  }
  int str2int(std::string str) {
    std::stringstream t(str);
    int x;
    t>>x;
    return x;
  }

  std::string int2str(int number) {
    std::stringstream t;
    t<<number;
    std::string str=t.str();
    return str;
  }

  int max(int a, int b) {return (a>b) ? a : b;}
  int min(int a, int b) {return (a<b) ? a : b;}



  double distance_to_border(Polygon& polygon,int width, int height)
  {
    double distance=min(width/2,height/2);
    for(size_t i=0;i<polygon.size();i++)
    {
      if (polygon[i].x()<distance)
        distance=polygon[i].x();
      if (polygon[i].y()<distance)
        distance=polygon[i].y();
      if (width-polygon[i].x()<distance)
        distance=width-polygon[i].x();
      if (height-polygon[i].y()<distance)
        distance=height-polygon[i].y();
    }
    if (distance<0) distance=0;
    return distance;
  }

  char buffer[1024];
  void log(const char* text, ...)
  {
    FILE * pFile;
    pFile = fopen ("bwapi-data/logs/BWTA.log","a");
    
    va_list ap;
    va_start(ap, text);
    vsprintf(buffer,text,ap );
    va_end(ap);
    if (pFile!=NULL)
    {
      fputs (buffer,pFile);
      fputs ("\n",pFile);
      fclose (pFile);
    }
  }
  void writeFile(const char* filename, const char* text, ...)
  {
    FILE * pFile;
    pFile = fopen (filename,"a");
    
    va_list ap;
    va_start(ap, text);
    vsprintf(buffer,text,ap );
    va_end(ap);
    if (pFile!=NULL)
    {
      fputs (buffer,pFile);
      fclose (pFile);
    }
  }


  int get_set(std::vector<int> &a,int i)
  {
    if (i==a[i]) return i;
    a[i]=get_set(a,a[i]);
    return a[i];
  }

  void calculate_walk_distances(const RectangleArray<bool> &read_map
                               ,const BWAPI::Position &start
                               ,int max_distance
                               ,RectangleArray<int> &distance_map)
  {
    Heap< BWAPI::Position , int > heap(true);
    for(unsigned int x=0;x<distance_map.getWidth();x++) {
      for(unsigned int y=0;y<distance_map.getHeight();y++) {
        distance_map[x][y]=-1;
      }
    }
    heap.push(std::make_pair(start,0));
    int sx=(int)start.x();
    int sy=(int)start.y();
    distance_map[sx][sy]=0;
    while (!heap.empty()) {
      BWAPI::Position pos=heap.top().first;
      int distance=heap.top().second;
      heap.pop();
      int x=(int)pos.x();
      int y=(int)pos.y();
      if (distance>max_distance && max_distance>0) break;
      int min_x=max(x-1,0);
      int max_x=min(x+1,read_map.getWidth()-1);
      int min_y=max(y-1,0);
      int max_y=min(y+1,read_map.getHeight()-1);
      for(int ix=min_x;ix<=max_x;ix++) {
        for(int iy=min_y;iy<=max_y;iy++) {
          int f=abs(ix-x)*10+abs(iy-y)*10;
          if (f>10) {f=14;}
          int v=distance+f;
          if (distance_map[ix][iy]>v) {
            heap.set(BWAPI::Position(x,y),v);
            distance_map[ix][iy]=v;
          } else {
            if (distance_map[ix][iy]==-1 && read_map[ix][iy]==true) {
              distance_map[ix][iy]=v;
              heap.push(std::make_pair(BWAPI::Position(ix,iy),v));
            }
          }
        }
      }
    }
  }

  void calculate_walk_distances_area(const BWAPI::Position &start
                                    ,int width
                                    ,int height
                                    ,int max_distance
                                    ,RectangleArray<int> &distance_map)
  {
    Heap< BWAPI::Position , int > heap(true);
    for(unsigned int x=0;x<distance_map.getWidth();x++) {
      for(unsigned int y=0;y<distance_map.getHeight();y++) {
        distance_map[x][y]=-1;
      }
    }
    int sx=(int)start.x();
    int sy=(int)start.y();
    for(int x=sx;x<sx+width;x++) {
      for(int y=sy;y<sy+height;y++) {
        heap.push(std::make_pair(BWAPI::Position(x,y),0));
        distance_map[x][y]=0;
      }
    }
    while (!heap.empty()) {
      BWAPI::Position pos=heap.top().first;
      int distance=heap.top().second;
      heap.pop();
      int x=(int)pos.x();
      int y=(int)pos.y();
      if (distance>max_distance && max_distance>0) break;
      int min_x=max(x-1,0);
      int max_x=min(x+1,BWAPI::Broodwar->mapWidth()*4-1);
      int min_y=max(y-1,0);
      int max_y=min(y+1,BWAPI::Broodwar->mapHeight()*4-1);
      for(int ix=min_x;ix<=max_x;ix++) {
        for(int iy=min_y;iy<=max_y;iy++) {
          int f=abs(ix-x)*10+abs(iy-y)*10;
          if (f>10) {f=14;}
          int v=distance+f;
          if (distance_map[ix][iy]>v) {
            heap.set(BWAPI::Position(x,y),v);
            distance_map[ix][iy]=v;
          } else {
            if (distance_map[ix][iy]==-1 && MapData::rawWalkability[ix][iy]==true) {
              distance_map[ix][iy]=v;
              heap.push(std::make_pair(BWAPI::Position(ix,iy),v));
            }
          }
        }
      }
    }
  }

  float max(float a, float b) {return (a>b) ? a : b;}
  float min(float a, float b) {return (a<b) ? a : b;}

  double max(double a, double b) {return (a>b) ? a : b;}
  double min(double a, double b) {return (a<b) ? a : b;}


  bool fileExists(std::string filename)
  {
    struct stat stFileInfo;
    return stat(filename.c_str(),&stFileInfo) == 0;
  }

  int fileVersion(std::string filename)
  {
      std::ifstream file_in;
      file_in.open(filename.c_str());
      int version;
      file_in >> version;
      file_in.close();
      return version;
  }


#ifdef DEBUG_DRAW
  QColor hsl2rgb(double h, double sl, double l)
  {
    double v;
    double r,g,b;
    r = l;   // default to gray
    g = l;
    b = l;
    v = (l <= 0.5) ? (l * (1.0 + sl)) : (l + sl - l * sl);
    if (v > 0)
    {
      double m;
      double sv;
      int sextant;
      double fract, vsf, mid1, mid2;
      m = l + l - v;
      sv = (v - m ) / v;
      h *= 6.0;
      sextant = (int)h;
      fract = h - sextant;
      vsf = v * sv * fract;
      mid1 = m + vsf;
      mid2 = v - vsf;
      switch (sextant)
      {
        case 0:
          r = v;
          g = mid1;
          b = m;
        break;
        case 1:
          r = mid2;
          g = v;
          b = m;
        break;
        case 2:
          r = m;
          g = v;
          b = mid1;
        break;
        case 3:
          r = m;
          g = mid2;
          b = v;
        break;
        case 4:
          r = mid1;
          g = m;
          b = v;
        break;
        case 5:
          r = v;
          g = m;
          b = mid2;
        break;
      }
    }
    return QColor(r*255.0,g*255.0,b*255.0);
  }
#endif

 
}