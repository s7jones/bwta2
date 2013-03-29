#include "Graph.h"
#include "Node.h"
#include <assert.h>
namespace BWTA
{
  #ifdef DEBUG_GRAPH
  #undef assert
  #undef _wassert
  extern "C" {
  _CRTIMP void __cdecl _wassert(_In_z_ const wchar_t * _Message, _In_z_ const wchar_t *_File, _In_ unsigned _Line);
  }
  #define assert(_Expression) (void)( (!!(_Expression)) || (_wassert(_CRT_WIDE(#_Expression), _CRT_WIDE(__FILE__), __LINE__), 0) )
  #endif
  int indent=0;
  Arrangement_2* arrangement;
  CGAL::Arr_simple_point_location<Arrangement_2> spl;
  Graph::Graph(Arrangement_2* arr)
  {
    arrangement=arr;
    spl.attach(*arr);
  }
  Node* Graph::add_region(Point p, double radius)
  {
    Node* new_region=new Node(p,radius,true);
    assert(this->regions.find(new_region)==this->regions.end());
    this->regions.insert(new_region);
    assert(this->m_get_region.find(p)==this->m_get_region.end());
    m_get_region.insert(std::make_pair(p,new_region));
    return new_region;
  }
  Node* Graph::add_region(Point p, double radius,Arrangement_2::Vertex_handle handle)
  {
    Node* new_region=new Node(p,radius,true, handle);
    assert(this->regions.find(new_region)==this->regions.end());
    this->regions.insert(new_region);
    assert(this->m_get_region.find(p)==this->m_get_region.end());
    m_get_region.insert(std::make_pair(p,new_region));
    return new_region;
  }
  Node* Graph::add_chokepoint(Point p, double radius,Arrangement_2::Vertex_handle handle,Node* region1, Node* region2)
  {
    assert(region1!=region2);
    if (this->m_get_chokepoint.find(p)!=this->m_get_chokepoint.end())
    {
      Node* copy=this->m_get_chokepoint.find(p)->second;
      assert(copy->point==p);
      assert(copy->radius==radius);
      assert(copy->handle==handle);
      assert(copy->neighbors.find(region1)!=copy->neighbors.end());
      assert(copy->neighbors.find(region2)!=copy->neighbors.end());
      assert(copy->neighbors.size()==2);
      assert(copy->is_region==false);
      return copy;
    }
    assert(region1);
    assert(region2);
    Node* new_ckpt=new Node(p,radius,false,handle);
    assert(this->chokepoints.find(new_ckpt)==this->chokepoints.end());
    this->chokepoints.insert(new_ckpt);
    assert(this->m_get_chokepoint.find(p)==this->m_get_chokepoint.end());
    m_get_chokepoint.insert(std::make_pair(p,new_ckpt));
    connect_nodes(new_ckpt,region1);
    connect_nodes(new_ckpt,region2);
    return new_ckpt;
  }
  Node* Graph::get_region(Point p)
  {
    if (this->m_get_region.find(p)!=this->m_get_region.end())
      return this->m_get_region.find(p)->second;
    return NULL;
  }
  Node* Graph::get_chokepoint(Point p)
  {
    if (this->m_get_chokepoint.find(p)!=this->m_get_chokepoint.end())
      return this->m_get_chokepoint.find(p)->second;
    return NULL;
  }
  void Graph::remove_region(Node* region)
  {
    assert(region);
    assert(region->is_region);
    assert(this->regions.find(region)!=this->regions.end());
    for(std::set<Node*>::iterator i=region->neighbors.begin();i!=region->neighbors.end();i++)
    {
      assert((*i)->is_region==false);
      remove_chokepoint(*i);
    }
    this->regions.erase(region);
    this->m_get_region.erase(region->point);
    delete region;
  }
  void Graph::remove_chokepoint(Node* ckpt)
  {
    assert(ckpt);
    assert(ckpt->is_region==false);
    assert(this->chokepoints.find(ckpt)!=this->chokepoints.end());
    assert(ckpt->neighbors.size()==2);
    Node* r1;
    Node* r2;
    std::set<Node*>::iterator i=ckpt->neighbors.begin();
    r1=*i;
    i++;
    r2=*i;
    assert(r1->is_region);
    assert(r2->is_region);
    disconnect_nodes(ckpt,r1);
    disconnect_nodes(ckpt,r2);
    this->m_get_chokepoint.erase(ckpt->point);
    this->chokepoints.erase(ckpt);
    Arrangement_2::Halfedge_around_vertex_circulator ci=ckpt->handle->incident_halfedges();
    Arrangement_2::Halfedge_around_vertex_circulator cstart=ckpt->handle->incident_halfedges();
    std::list<Arrangement_2::Halfedge_handle> the_remove_edges;
    do
    {
      if (ci->data()==RED)
      {
        the_remove_edges.push_back(ci);
      }
      ci++;
    } while (ci!=cstart);
    for(std::list<Arrangement_2::Halfedge_handle>::iterator i=the_remove_edges.begin();i!=the_remove_edges.end();i++)
    {
      arrangement->remove_edge(*i);
    }
    delete ckpt;
  }
  void Graph::merge_chokepoint(Node* ckpt)
  {

    assert(ckpt!=NULL);
    assert(ckpt->is_region==false);
    assert(this->chokepoints.find(ckpt)!=this->chokepoints.end());
    assert(ckpt->neighbors.size()==2);
    Node* r1;
    Node* r2;
    std::set<Node*>::iterator i=ckpt->neighbors.begin();
    r1=*i;
    i++;
    r2=*i;
    assert(r1->is_region);
    assert(r2->is_region);
    disconnect_nodes(ckpt,r1);
    disconnect_nodes(ckpt,r2);

    this->m_get_chokepoint.erase(ckpt->point);
    this->chokepoints.erase(ckpt);
    Arrangement_2::Halfedge_around_vertex_circulator ci=ckpt->handle->incident_halfedges();
    Arrangement_2::Halfedge_around_vertex_circulator cstart=ckpt->handle->incident_halfedges();
    std::list<Arrangement_2::Halfedge_handle> the_remove_edges;
    do
    {
      if (ci->data()==RED)
      {
        the_remove_edges.push_back(ci);
      }
      ci++;
    } while (ci!=cstart);
    for(std::list<Arrangement_2::Halfedge_handle>::iterator i=the_remove_edges.begin();i!=the_remove_edges.end();i++)
    {
      arrangement->remove_edge(*i);
    }
    delete ckpt;

    //the smaller region will now be merged into the larger region
    Node* stay_region=r1;
    Node* rmve_region=r2;
    if (r2->radius>r1->radius)
    {
      stay_region=r2;
      rmve_region=r1;
    }
    assert(stay_region->radius>=rmve_region->radius);
    std::set<Node*> ckpts_to_remove;
    std::set<Node*> neighbors(rmve_region->neighbors);
    for(std::set<Node*>::iterator adj=neighbors.begin();adj!=neighbors.end();adj++)
    {
      if (stay_region->neighbors.find(*adj)!=stay_region->neighbors.end())
      {
        ckpts_to_remove.insert(*adj);
      }
      else
      {
        disconnect_nodes(rmve_region,*adj);
        connect_nodes(stay_region,*adj);
      }
    }
    for(std::set<Node*>::iterator a=ckpts_to_remove.begin();a!=ckpts_to_remove.end();a++)
    {
      remove_chokepoint(*a);
    }
    remove_region(rmve_region);
  }
  bool Graph::has_region(Node* region)
  {
    return this->regions.find(region)!=this->regions.end();
  }
  bool Graph::has_chokepoint(Node* ckpt)
  {
    return this->chokepoints.find(ckpt)!=this->chokepoints.end();
  }
  void Graph::connect_nodes(Node* a,Node* b)
  {
    assert(a);
    assert(b);
    assert(a->neighbors.find(b)==a->neighbors.end());
    assert(b->neighbors.find(a)==b->neighbors.end());
    a->neighbors.insert(b);
    b->neighbors.insert(a);
  }
  void Graph::disconnect_nodes(Node* a,Node* b)
  {
    assert(a);
    assert(b);
    assert(a->neighbors.find(b)!=a->neighbors.end());
    assert(b->neighbors.find(a)!=b->neighbors.end());
    a->neighbors.erase(b);
    b->neighbors.erase(a);
  }


  std::set<Node*>::iterator Graph::regions_begin()
  {
    return this->regions.begin();
  }
  std::set<Node*>::iterator Graph::regions_end()
  {
    return this->regions.end();
  }
  std::set<Node*>::iterator Graph::chokepoints_begin()
  {
    return this->chokepoints.begin();
  }
  std::set<Node*>::iterator Graph::chokepoints_end()
  {
    return this->chokepoints.end();
  }
}