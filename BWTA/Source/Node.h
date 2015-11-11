#pragma once
#include "functions.h"
namespace BWTA
{
  class Node
  {
  public:
    Node(Point point, double radius, bool is_region);
    Node(Point point, double radius, bool is_region,Arrangement_2::Vertex_handle handle);
    bool operator<(const Node& other) const;
    PolygonD get_polygon() const;
    Node* a_neighbor() const;
    Node* other_neighbor(Node* n) const;
    Point point;
    double radius;
    std::set<Node*> neighbors;
    Point side1;
    Point side2;
    bool is_region;
    Arrangement_2::Vertex_handle handle;
    double hue;
  };
}