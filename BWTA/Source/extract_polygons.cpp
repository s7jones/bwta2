#include "functions.h"
#include "ConnectedComponent.h"
#include "extract_polygons.h"
namespace BWTA
{
  void rotate_cw(int &x,int &y);
  void rotate_ccw(int &x,int &y);
  void simplify(Polygon &polygon, double error_tol);

  bool order_polygons_decreasing_area(const Polygon &a, const Polygon &b)
  {
    return a.getArea()>b.getArea();
  }

  void extract_polygons(const RectangleArray<bool> &walkability
                       ,const std::list<ConnectedComponent> &components
                       ,std::vector<Polygon> &polygons)
  {
    Point pos;
    std::vector<Polygon> walkable_polygons;
    for(std::list<ConnectedComponent>::const_iterator c=components.begin();c!=components.end();c++)
    {
      bool walkable=c->isWalkable();
      int nextx=1;
      int nexty=2;
      int cx=(int)CGAL::to_double(c->top_left().x());
	  int cy = (int)CGAL::to_double(c->top_left().y());
      bool adjcol[3][3];
      Polygon newpoly;
      newpoly.push_back(BWAPI::Position(cx,cy));
      bool first=true;
      while(cx!=newpoly[0].x || cy!=newpoly[0].y || first)
      {
        first=false;
        for(int i=0;i<3;i++) {
          for(int j=0;j<3;j++) adjcol[i][j]=!walkable;
        }
        adjcol[1][1]=walkability[cx][cy];
        if (cx>0 && cy>0)
          adjcol[0][0]=walkability[cx-1][cy-1];
        if (cy>0)
          adjcol[1][0]=walkability[cx][cy-1];
        if (cx<(int)walkability.getWidth()-1 && cy>0)
          adjcol[2][0]=walkability[cx+1][cy-1];
        if (cx>0)
          adjcol[0][1]=walkability[cx-1][cy];
        if (cx<(int)walkability.getWidth()-1)
          adjcol[2][1]=walkability[cx+1][cy];
        if (cx>0 && cy<(int)walkability.getHeight()-1)
          adjcol[0][2]=walkability[cx-1][cy+1];
        if (cy<(int)walkability.getHeight()-1)
          adjcol[1][2]=walkability[cx][cy+1];
        if (cx<(int)walkability.getWidth()-1 && cy<(int)walkability.getHeight()-1)
          adjcol[2][2]=walkability[cx+1][cy+1];
        bool done=false;
        rotate_cw(nextx,nexty);
        rotate_cw(nextx,nexty);
        if (adjcol[nextx][nexty]!=walkable)
        {
          for(int count=0;count<=8 && adjcol[nextx][nexty]!=walkable;count++) {
            rotate_ccw(nextx,nexty);
            if (walkable)
              rotate_ccw(nextx,nexty);
            if (count==8) done=true;
          }
        }
        if (done) break;
        cx=cx+nextx-1;
        cy=cy+nexty-1;
        newpoly.push_back(BWAPI::Position(cx,cy));
      }
      if (newpoly.getArea()>16)
      {
        if (walkable)
          walkable_polygons.push_back(newpoly);
        else
          polygons.push_back(newpoly);
      }
    }
    sort(polygons.begin(),polygons.end(),order_polygons_decreasing_area);
    sort(walkable_polygons.begin(),walkable_polygons.end(),order_polygons_decreasing_area);
    for(size_t i=0;i<walkable_polygons.size();i++)
    {
      if (walkable_polygons[i].getArea()>=256)
      {
        for(size_t j=0;j<polygons.size();j++)
        {
          if (polygons[j].getArea()>walkable_polygons[i].getArea() && polygons[j].isInside(walkable_polygons[i][0]))
          {
            polygons[j].holes.push_back(walkable_polygons[i]);
            break;
          }
        }
      }
    }

	// Simplify polygons
	for (auto& polygon : polygons) {
		simplify(polygon, 1.0);
		anchorToBorder(polygon, walkability.getWidth(), walkability.getHeight());
		for (auto& hole : polygon.holes) {
			simplify(hole, 1.0);
			anchorToBorder(hole, walkability.getWidth(), walkability.getHeight());
		}
	}
  }

  void rotate_cw(int &x,int &y) {
    if (x==0 && y==0) {
      x=1;
      return;
    }
    if (x==0 && y==1) {
      y=0;
      return;
    }
    if (x==0 && y==2) {
      y=1;
      return;
    }
    if (x==1 && y==2) {
      x=0;
      return;
    }
    if (x==2 && y==2) {
      x=1;
      return;
    }
    if (x==2 && y==1) {
      y=2;
      return;
    }
    if (x==2 && y==0) {
      y=1;
      return;
    }
    if (x==1 && y==0) {
      x=2;
      return;
    }
  }

  void rotate_ccw(int &x,int &y) {
    if (x==0 && y==0) {
      y=1;
      return;
    }
    if (x==0 && y==1) {
      y=2;
      return;
    }
    if (x==0 && y==2) {
      x=1;
      return;
    }
    if (x==1 && y==2) {
      x=2;
      return;
    }
    if (x==2 && y==2) {
      y=1;
      return;
    }
    if (x==2 && y==1) {
      y=0;
      return;
    }
    if (x==2 && y==0) {
      x=1;
      return;
    }
    if (x==1 && y==0) {
      x=0;
      return;
    }
  }

  void simplify(Polygon &polygon, double error_tol)
  {
    polygon.push_back(polygon[0]);
    for(size_t i=0;i+1<polygon.size();i++)
    {
      size_t j=i+1;
      int last_good_point=j;
      bool within_tol=true;
      while (within_tol && j<polygon.size())
      {
        j++;
        if (j==polygon.size())
          break;
        for(size_t k=i+1;k<j;k++)
        {
          double dx=polygon[i].x-polygon[j].x;
          double dy=polygon[i].y-polygon[j].y;
          double d=sqrt(dx*dx+dy*dy);
          double nx=dy/d;
          double ny=-dx/d;
		  double distance = std::abs((polygon[k].x - polygon[i].x)*nx + (polygon[k].y - polygon[i].y)*ny);
          if (distance>=error_tol)
          {
            within_tol=false;
            break;
          }
        }
        if (within_tol)
        {
          last_good_point=j;
        }
      }
      if (i+1!=last_good_point)
      {
        polygon.erase(polygon.begin()+i+1,polygon.begin()+last_good_point);
      }
    }
    if (polygon[0]==polygon.back())
    {
      polygon.erase(polygon.begin()+polygon.size()-1);
    }
  }

  // anchor vertices near borders of the map to the border
  // used to fix errors from simplify polygon
  void anchorToBorder(Polygon &polygon, int mapWidth, int mapHeight)
  {
	  const int ANCHOR_MARGIN = 2;
	  for (auto& vertex : polygon) {
		  if (vertex.x <= ANCHOR_MARGIN) vertex.x = 0;
		  if (vertex.y <= ANCHOR_MARGIN) vertex.y = 0;
		  if (vertex.x >= mapWidth - 1 - ANCHOR_MARGIN) vertex.x = mapWidth - 1;
		  if (vertex.y >= mapHeight - 1 - ANCHOR_MARGIN) vertex.y = mapHeight - 1;
	  }
  }

  void find_connected_components(const RectangleArray<bool> &simplified_map
                                ,RectangleArray<ConnectedComponent*> &get_component
                                ,std::list<ConnectedComponent> &components)
  {
    int currentID=1;
    components.clear();
    get_component.resize(simplified_map.getWidth(),simplified_map.getHeight());
	get_component.setTo(NULL);

    for (unsigned int x = 0; x < simplified_map.getWidth(); x++) {
      for (unsigned int y = 0; y < simplified_map.getHeight(); y++) {
        if (get_component[x][y] == NULL) {
          components.push_back(ConnectedComponent(currentID++,simplified_map[x][y]));
          ConnectedComponent *current_component=&(components.back());
          int fill_type=0;
          if (simplified_map[x][y]==false) fill_type=2;
          current_component->set_top_left(Point(x,y));
          flood_fill_with_component(simplified_map,Point(x,y),current_component,fill_type,get_component);
        }
      }
    }
  }

  void flood_fill_with_component(const RectangleArray<bool> &read_map
                                ,const Point start
                                ,ConnectedComponent* component
                                ,int fill_type
                                ,RectangleArray<ConnectedComponent*> &write_map)
  {
    if (component==NULL)
      return;
	std::list<Point> q;
    q.push_back(start);
	bool fill = false;
    while (!q.empty())
    {
	  Point p = q.front();
      if (p.x()<component->top_left().x() || (p.x()==component->top_left().x() && p.y()<component->top_left().y()))
      {
        component->set_top_left(p);
      }
	  int x = (int)CGAL::to_double(p.x());
	  int y = (int)CGAL::to_double(p.y());
      q.pop_front();
      if (write_map[x][y]==NULL)
      {
        write_map[x][y]=component;
		int min_x = std::max(x - 1, 0);
		int max_x = std::min(x + 1, (int)read_map.getWidth() - 1);
		int min_y = std::max(y - 1, 0);
		int max_y = std::min(y + 1, (int)read_map.getHeight() - 1);
        for(int ix=min_x;ix<=max_x;ix++)
        {
          for(int iy=min_y;iy<=max_y;iy++)
          {
			  fill = false;
			  if (fill_type == 0) { //4-directional movement
				  if (x == ix || y == iy) {
					  if (read_map[ix][iy] == component->isWalkable() && write_map[ix][iy] == NULL) fill = true;
				  }
			  } else if (fill_type == 1) { //limited 8-directional movement
				  if (x == ix || y == iy || read_map[x][iy] == component->isWalkable()
					  || read_map[ix][y] == component->isWalkable()) {
					  if (read_map[ix][iy] == component->isWalkable() && write_map[ix][iy] == NULL) fill = true;
				  }
			  } else if (fill_type == 2) { //full 8-directional movement
				  if (read_map[ix][iy] == component->isWalkable() && write_map[ix][iy] == NULL) fill = true;
			  }
			  if (fill) {
				  q.push_back(Point(ix, iy));
			  }
          }
        }
      }
    }
  }
}