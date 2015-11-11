#pragma once
#include "functions.h"
namespace BWTA
  {
  class Node;
  class Graph
  {
    public:
    Graph(Arrangement_2* arrangement);
    Node* add_region(Point p, double radius);
    Node* add_region(Point p, double radius,Arrangement_2::Vertex_handle handle);
    Node* add_chokepoint(Point p, double radius,Arrangement_2::Vertex_handle handle,Node* region1, Node* region2);
    Node* get_region(Point p);
    Node* get_chokepoint(Point p);
    void remove_region(Node* region);
    void remove_chokepoint(Node* ckpt);
    void merge_chokepoint(Node* ckpt);
    bool has_region(Node* region);
    bool has_chokepoint(Node* ckpt);

    std::set<Node*>::iterator regions_begin();
    std::set<Node*>::iterator regions_end();
    std::set<Node*>::iterator chokepoints_begin();
    std::set<Node*>::iterator chokepoints_end();
    private:
    void connect_nodes(Node* a,Node* b);
    void disconnect_nodes(Node* a,Node* b);
    std::set<Node*> regions;
    std::set<Node*> chokepoints;
    std::map<Point, Node*, ptcmp > m_get_region;
    std::map<Point, Node*, ptcmp > m_get_chokepoint;
  };
  extern Arrangement_2* arrangement;
  extern CGAL::Arr_simple_point_location<Arrangement_2> spl;
}