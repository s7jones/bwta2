#include "Node.h"
#include "Graph.h"
namespace BWTA
  {
  Node::Node(Point point, double radius, bool is_region):
  point(point),radius(radius),is_region(is_region)
  {
  }
  Node::Node(Point point, double radius, bool is_region,Arrangement_2::Vertex_handle handle):
  point(point),radius(radius),is_region(is_region),handle(handle)
  {
  }
  bool Node::operator<(const Node& other) const
  {
    return ptcmp()(point,other.point);
  }
  PolygonD Node::get_polygon() const
  {
	//log("calling get_polygon from a node at position ("<<point.x()<<","<<point.x()<<")");
    Arrangement_2::Face_const_handle f;
    Arrangement_2::Halfedge_const_handle h;
    Arrangement_2::Vertex_const_handle v;
    PolygonD poly;
    if (CGAL::assign(f,spl.locate(Point_2(point.x(),point.y()))))
    {
      Arrangement_2::Ccb_halfedge_const_circulator c=f->outer_ccb();
      Arrangement_2::Ccb_halfedge_const_circulator start=c;
      do
      {
        poly.push_back(PointD(cast_to_double(c->source()->point().x()),cast_to_double(c->source()->point().y())));
        c++;
      } while (c!=start);
    }
    else if (CGAL::assign(h,spl.locate(Point_2(point.x(),point.y()))))
    {
      log("error: expected face, not halfedge!");
    }
    else if (CGAL::assign(v,spl.locate(Point_2(point.x(),point.y()))))
    {
      log("error: expected face, not vertex!");
    }
    else
    {
      log("error: expected face, didn't find anything!");
    }
    return poly;
  }
  Node* Node::a_neighbor() const
  {
    assert(this->neighbors.size()>0);
    std::set<Node*>::const_iterator i=this->neighbors.begin();
    return *i;
  }
  Node* Node::other_neighbor(Node* n) const
  {
    assert(this->neighbors.size()>1);
    assert(this->neighbors.find(n)!=this->neighbors.end());
    std::set<Node*>::const_iterator i=this->neighbors.begin();
    if (*i==n)
      i++;
    return *i;
  }

}