#include "Color.h"
#include "VertexData.h"
#include "functions.h"
#include "Graph.h"
#include "Node.h"
#include <BWAPI.h>
#include "BWTA.h"
#include "Globals.h"
#include "RegionImpl.h"
#include "ChokepointImpl.h"
#include "BaseLocationImpl.h"
#include "find_base_locations.h"
#include "extract_polygons.h"
#include "BWTA_Result.h"
#include "MapData.h"
#include "Heap.h"
#include "terrain_analysis.h"
#ifdef DEBUG_DRAW
  #include <QtGui>
  #include <CGAL/Qt/GraphicsViewNavigation.h>
  #include <QLineF>
  #include <QRectF>
#endif

using namespace std;
namespace BWTA
{
  #ifdef DEBUG_DRAW
    int render(int step);
    void draw_border();
    void draw_arrangement(Arrangement_2* arr_ptr);
    void draw_polygons(vector<Polygon>* polygons_ptr);
  #endif
  void simplify_voronoi_diagram(Arrangement_2* arr_ptr, std::map<Point, double, ptcmp>* distance);
  void identify_region_nodes(Arrangement_2* arr_ptr,Graph* g_ptr);
  void identify_chokepoint_nodes(Graph* g_ptr, std::map<Point, double, ptcmp>* distance, std::map<Point, std::set< Point >, ptcmp >* nearest);
  double calculate_merge_value(Node* c);
  void merge_adjacent_regions(Graph* g_ptr);
  void remove_voronoi_diagram_from_arrangement(Arrangement_2* arr_ptr);
  void wall_off_chokepoints(Graph* g_ptr,Arrangement_2* arr_ptr);

  class My_observer : public CGAL::Arr_observer<Arrangement_2>
  {
    public:
    My_observer (Arrangement_2& arr) :
      CGAL::Arr_observer<Arrangement_2> (arr)
    {}

    virtual void after_split_edge ( Halfedge_handle e1, Halfedge_handle e2)
    {
      if (e1->data()==BLACK || e1->data()==BLUE || e1->data()==RED)
      {
        e1->twin()->set_data(e1->data());
        e2->set_data(e1->data());
        e2->twin()->set_data(e1->data());
      }
      else if (e2->data()==BLACK || e2->data()==BLUE || e2->data()==RED)
      {
        e2->twin()->set_data(e2->data());
        e1->set_data(e2->data());
        e1->twin()->set_data(e2->data());
      }
      else if (e1->twin()->data()==BLACK || e1->twin()->data()==BLUE || e1->twin()->data()==RED)
      {
        e1->set_data(e1->twin()->data());
        e2->set_data(e1->twin()->data());
        e2->twin()->set_data(e1->twin()->data());
      }
      else if (e2->twin()->data()==BLACK || e2->twin()->data()==BLUE || e2->twin()->data()==RED)
      {
        e2->set_data(e2->twin()->data());
        e1->set_data(e2->twin()->data());
        e1->twin()->set_data(e2->twin()->data());
      }
    }
  };

  #ifdef DEBUG_DRAW
    QGraphicsScene* scene_ptr;
    QApplication* app_ptr;
  #endif
  void readMap()
  {
    MapData::mapWidth=BWAPI::Broodwar->mapWidth();
    MapData::mapHeight=BWAPI::Broodwar->mapHeight();
    load_map();
    load_resources();
    MapData::hash=BWAPI::Broodwar->mapHash();
    MapData::startLocations=BWAPI::Broodwar->getStartLocations();
  }
  void analyze()
  {
	std::ofstream logFile( "bwapi-data/logs/BWTA.log");
	logFile << "Map name: " << BWAPI::Broodwar->mapName() << std::endl;
    std::string filename = "bwapi-data/BWTA/" + BWAPI::Broodwar->mapHash() + ".bwta";
    if (fileExists(filename) && fileVersion(filename)==BWTA_FILE_VERSION)
    {
      log("Recognized map, loading map data...");
      load_data(filename);
      log("Loaded map data.");
    }
    else
    {
      log("Analyzing new map...");
      time_t t1=clock();
      analyze_map();
      time_t t2=clock();
      double seconds = (t2-t1)*1.0/CLOCKS_PER_SEC;
      log("running time: " << seconds);
      log("Analyzed map.");
      save_data(filename);
      log("Saved map data.");
    }
  }

  void analyze_map()
  {
    #ifdef DEBUG_DRAW
      int argc=0;
      char* argv="";
      QApplication app(argc,&argv);
      app_ptr=&app;
      QGraphicsScene scene;
      scene_ptr=&scene;
    #endif

    // Delete and clear any old regions, chokepoints, and unwalkable polygons
    for(std::set<BWTA::Region*>::iterator i=BWTA_Result::regions.begin();i!=BWTA_Result::regions.end();i++)
      delete *i;
    BWTA_Result::regions.clear();
    for(std::set<BWTA::Chokepoint*>::iterator i=BWTA_Result::chokepoints.begin();i!=BWTA_Result::chokepoints.end();i++)
      delete *i;
    BWTA_Result::chokepoints.clear();
    for(std::set<BWTA::Polygon*>::iterator i=BWTA_Result::unwalkablePolygons.begin();i!=BWTA_Result::unwalkablePolygons.end();i++)
      delete *i;
    BWTA_Result::unwalkablePolygons.clear();



    std::vector< std::vector< BWAPI::Unit* > > clusters;

    // Give find_mineral_clusters the walkability data, minerals, and geysers, so it can compute the resource cluters
    find_mineral_clusters(MapData::walkability,MapData::minerals,MapData::geysers,clusters);
    log("Found " << clusters.size() << " mineral clusters.");

    // For each build tile, base_build_map[x][y] is true if we can build a base
    // (resource depot) at that position (top left corner being on the given tile)
    RectangleArray<bool> base_build_map;

    // Give calculate_base_build_map the buildability data and clusters so it can compute the base_build_map
    calculate_base_build_map(MapData::buildability,clusters,base_build_map);
    log("Calculated base build map.");
    RectangleArray<ConnectedComponent*> get_component;
    std::list<ConnectedComponent> components;

    // Give find_connected_components the walkability data so it can compute the list of connected components,
    // and determine which component each tile belongs to
    find_connected_components(MapData::walkability,get_component,components);
    log("Calculated connected components.");

    // Give calculate_base_locations the walkability data, base_build_map, and clusters so it can compute the base locations
    calculate_base_locations(MapData::walkability,base_build_map,clusters,BWTA_Result::baselocations);

    log("Calculated base locations.");
    vector<Polygon> polygons;
    // Give extract_polygons the walkability data and connected components so it can compute the polygonal obstacles
    extract_polygons(MapData::walkability,components,polygons);
    log("Extracted polygons.");

    // Discard polygons that are too small
    for(unsigned int p=0;p<polygons.size();)
    {
      if (abs(polygons[p].getArea())<=256 && distance_to_border(polygons[p],MapData::walkability.getWidth(),MapData::walkability.getHeight())>1)
      {
        polygons.erase(polygons.begin()+p);
      }
      else
      {
        p++;
      }
    }

    // Save the remaining polygons in BWTA_Result::unwalkablePolygons
    for(size_t i=0;i<polygons.size();i++)
    {
      BWTA_Result::unwalkablePolygons.insert(new Polygon(polygons[i]));
    }

    #ifdef DEBUG_DRAW
      log("Drawing results of step 1");
      draw_border();
      draw_polygons(&polygons);
      render(1);
    #endif

    // All line segments we create we will store in the sites vector and also insert into the 2d segmented delaunay graph object sdg

    // Create the sites vector and 2d segmented delaunary graph
    vector<SDGS2> sites;
    SDG2 sdg;

    // Add line segments of the 4 edges of the map to the sites vector
    sites.push_back(SDGS2::construct_site_2(PointD(0,0),PointD(0,MapData::walkability.getHeight()-1)));
    sites.push_back(SDGS2::construct_site_2(PointD(0,MapData::walkability.getHeight()-1),PointD(MapData::walkability.getWidth()-1,MapData::walkability.getHeight()-1)));
    sites.push_back(SDGS2::construct_site_2(PointD(MapData::walkability.getWidth()-1,MapData::walkability.getHeight()-1),PointD(MapData::walkability.getWidth()-1,0)));
    sites.push_back(SDGS2::construct_site_2(PointD(MapData::walkability.getWidth()-1,0),PointD(0,0)));

    // Add these same line segments to the sdg
    for(unsigned int i=0;i<sites.size();i++)
    {
      sdg.insert(sites[i]);
    }
    // Add the line segments of each polygon to sites and sdg
    for(unsigned int p=0;p<polygons.size();p++)
    {
      SDG2::Vertex_handle h;
      // Add the edges of the border of polygons[p] to sites and sdg
      for(size_t i=0;i<polygons[p].size();i++)
      {
        int j=(i+1)%polygons[p].size();
        PointD a(polygons[p][i].x(),polygons[p][i].y());
        PointD b(polygons[p][j].x(),polygons[p][j].y());
        sites.push_back(SDGS2::construct_site_2(b,a));
        if (i==0)
        {
          // This is the first vertex of this polygon, so don't specify a vertex handle
          h=sdg.insert(sites[sites.size()-1]);
        }
        else
        {
          // This vertex is probably close to the previous vertex of this polygon so give the insert
          // function the handle of the previous vertex as a hint for where to insert this vertex
          h=sdg.insert(sites[sites.size()-1],h);
        }
      }
      // Add the edges of each hole in polygons[p] to sites and sdg
      for(std::vector<Polygon>::iterator hole=polygons[p].holes.begin();hole!=polygons[p].holes.end();hole++)
      {
        // Add the edges of this hole to sites and sdg
        for(size_t i=0;i<hole->size();i++)
        {
          int j=(i+1)%hole->size();
          PointD a((*hole)[i].x(),(*hole)[i].y());
          PointD b((*hole)[j].x(),(*hole)[j].y());
          sites.push_back(SDGS2::construct_site_2(b,a));
          h=sdg.insert(sites[sites.size()-1],h);
        }
      }
    }
    log("Created voronoi diagram.");
    // The sites vector and sdg object not contain all of the edges of each polygon as well as
    // the four edges that form the border of the map
    // Check to see if the 2d segmented delaunay graph is still valid
    assert( sdg.is_valid(true, 1) );
    cout << endl << endl;
    log("Verified voronoi diagram.");

    vector< Segment > voronoi_diagram_edges;
    std::map<Point, std::set< Point >, ptcmp > nearest;
    std::map<Point, double, ptcmp> distance;
    get_voronoi_edges(sdg,voronoi_diagram_edges,nearest,distance,polygons);
    log("Got voronoi edges.");
    Arrangement_2 arr;
    My_observer obs(arr);
    Graph g(&arr);

    // Insert all line segments from polygons into arrangement
    for(unsigned int i=0;i<sites.size();i++)
    {
      NumberType x0(sites[i].segment().vertex(0).x());
      NumberType y0(sites[i].segment().vertex(0).y());
      NumberType x1(sites[i].segment().vertex(1).x());
      NumberType y1(sites[i].segment().vertex(1).y());
      if (x0!=x1 || y0!=y1)
      {
        if (is_real(x0)!=0 && is_real(y0)!=0
         && is_real(x1)!=0 && is_real(y1)!=0)
        {
          CGAL::insert(arr,Segment_2(Point_2(sites[i].segment().vertex(0).x(),sites[i].segment().vertex(0).y()),Point_2(sites[i].segment().vertex(1).x(),sites[i].segment().vertex(1).y())));
        }
      }
    }
    log("Inserted polygons into arrangement.");
    //color all initial segments and vertices from the polygons BLACK
    for (Arrangement_2::Edge_iterator eit = arr.edges_begin(); eit != arr.edges_end(); ++eit)
    {
      eit->data()=BLACK;
    }
    for (Arrangement_2::Vertex_iterator vit = arr.vertices_begin(); vit != arr.vertices_end(); ++vit)
    {
      vit->data().c=BLACK;
    }

    // Insert into arrangement all segments from voronoi diagram which are not bounded by any polygon
    for(unsigned int i=0;i<voronoi_diagram_edges.size();i++)
    {
      NumberType x0(voronoi_diagram_edges[i].vertex(0).x());
      NumberType y0(voronoi_diagram_edges[i].vertex(0).y());
      NumberType x1(voronoi_diagram_edges[i].vertex(1).x());
      NumberType y1(voronoi_diagram_edges[i].vertex(1).y());
      if (x0!=x1 || y0!=y1)
      {
        if (is_real(x0)!=0 && is_real(y0)!=0
         && is_real(x1)!=0 && is_real(y1)!=0)
        {
          bool add=true;
          for(unsigned int p=0;p<polygons.size();p++)
          {
            if (polygons[p].isInside(BWAPI::Position(int(cast_to_double(voronoi_diagram_edges[i].vertex(0).x())),int(cast_to_double(voronoi_diagram_edges[i].vertex(0).y())))))
            {
              // First end point of this line segment is inside polygons[p], so don't add it to the arrangement
              add=false;
              break;
            }
            if (polygons[p].isInside(BWAPI::Position(int(cast_to_double(voronoi_diagram_edges[i].vertex(1).x())),int(cast_to_double(voronoi_diagram_edges[i].vertex(1).y())))))
            {
              // Second end point of this line segment is inside polygons[p], so don't add it to the arrangement
              add=false;
              break;
            }
          }
          if (add)
          {
            CGAL::insert(arr,Segment_2(Point_2(x0,y0),Point_2(x1,y1)));
          }
        }
      }
    }
    log("Added voronoi edges.");
    // Color all the new edges BlUE
    for (Arrangement_2::Edge_iterator eit = arr.edges_begin(); eit != arr.edges_end(); ++eit)
    {
      if (eit->data()!=BLACK)
      {
        eit->data()=BLUE;
        eit->twin()->data()=BLUE;
      }
    }
    // Color all the new verties BlUE
    for (Arrangement_2::Vertex_iterator vit = arr.vertices_begin(); vit != arr.vertices_end(); ++vit)
    {
      if (vit->data().c!=BLACK)
      {
        vit->data().c=BLUE;
      }
    }

    #ifdef DEBUG_DRAW
      log("Drawing results of step 2");
      draw_polygons(&polygons);
      draw_arrangement(&arr);
      render(2);
    #endif

    simplify_voronoi_diagram(&arr,&distance);

    #ifdef DEBUG_DRAW
      log("Drawing results of step 3");
      draw_polygons(&polygons);
      draw_arrangement(&arr);
      render(3);
    #endif

    identify_region_nodes(&arr,&g);

    #ifdef DEBUG_DRAW
      log("Drawing results of step 4");
      draw_polygons(&polygons);
      draw_arrangement(&arr);
      for(std::set<Node*>::iterator r=g.regions_begin();r!=g.regions_end();r++)
      {
        double x0=cast_to_double((*r)->point.x());
        double y0=cast_to_double((*r)->point.y());
        #ifdef DRAW_COLOR
          scene.addEllipse(QRectF(x0-6,y0-6,12,12),QPen(QColor(0,0,255)),QBrush(QColor(0,0,255)));
        #else
          scene.addEllipse(QRectF(x0-6,y0-6,12,12),QPen(QColor(0,0,0)),QBrush(QColor(0,0,0)));
        #endif
      }
      render(4);
    #endif

    identify_chokepoint_nodes(&g,&distance,&nearest);

    #ifdef DEBUG_DRAW
      log("Drawing results of step 5");
      draw_polygons(&polygons);
      draw_arrangement(&arr);
      for(std::set<Node*>::iterator r=g.regions_begin();r!=g.regions_end();r++)
      {
        double x0=cast_to_double((*r)->point.x());
        double y0=cast_to_double((*r)->point.y());
        #ifdef DRAW_COLOR
          scene.addEllipse(QRectF(x0-6,y0-6,12,12),QPen(QColor(0,0,255)),QBrush(QColor(0,0,255)));
        #else
          scene.addEllipse(QRectF(x0-6,y0-6,12,12),QPen(QColor(0,0,0)),QBrush(QColor(0,0,0)));
        #endif
      }
      for(std::set<Node*>::iterator c=g.chokepoints_begin();c!=g.chokepoints_end();c++)
      {
        double x0=cast_to_double((*c)->point.x());
        double y0=cast_to_double((*c)->point.y());
        #ifdef DRAW_COLOR
          scene.addEllipse(QRectF(x0-6,y0-6,12,12),QPen(QColor(255,0,0)),QBrush(QColor(255,0,0)));
        #else
          QVector<QPointF> qp;
          qp.push_back(QPointF(x0,y0-6));
          qp.push_back(QPointF(x0-7,y0+6));
          qp.push_back(QPointF(x0+7,y0+6));
          scene_ptr->addPolygon(QPolygonF(qp),QPen(QColor(0,0,0)),QBrush(QColor(0,0,0)));  
        #endif
      }
      render(5);
    #endif

    merge_adjacent_regions(&g);

    log("Merged regions.");
    remove_voronoi_diagram_from_arrangement(&arr);
    log("Removed voronoi edges.");

    #ifdef DEBUG_DRAW
      log("Drawing results of step 6");
      draw_polygons(&polygons);
      draw_arrangement(&arr);
      for(std::set<Node*>::iterator r=g.regions_begin();r!=g.regions_end();r++)
      {
        double x0=cast_to_double((*r)->point.x());
        double y0=cast_to_double((*r)->point.y());  
        for(std::set<Node*>::iterator n=(*r)->neighbors.begin();n!=(*r)->neighbors.end();n++)
        {
          double x1=cast_to_double((*n)->point.x());
          double y1=cast_to_double((*n)->point.y());
          QPen qp(QColor(0,0,0));
          qp.setWidth(2);
          scene.addLine(QLineF(x0,y0,x1,y1),qp);
        }
        #ifdef DRAW_COLOR
          scene.addEllipse(QRectF(x0-6,y0-6,12,12),QPen(QColor(0,0,255)),QBrush(QColor(0,0,255)));
        #else
          scene.addEllipse(QRectF(x0-6,y0-6,12,12),QPen(QColor(0,0,0)),QBrush(QColor(0,0,0)));
        #endif
        //scene.addEllipse(QRectF(x0-(*r)->radius,y0-(*r)->radius,2*(*r)->radius,2*(*r)->radius),QPen(QColor(0,0,255)));
      }
      for(std::set<Node*>::iterator c=g.chokepoints_begin();c!=g.chokepoints_end();c++)
      {
        double x0=cast_to_double((*c)->point.x());
        double y0=cast_to_double((*c)->point.y());
        //scene.addEllipse(QRectF(x0-3,y0-3,6,6),QPen(QColor(255,0,0)),QBrush(QColor(255,0,0)));
//        scene.addRect(QRectF(x0-5,y0-5,10,10),QPen(QColor(0,0,0)),QBrush(QColor(0,0,0)));
        #ifdef DRAW_COLOR
          scene.addEllipse(QRectF(x0-6,y0-6,12,12),QPen(QColor(255,0,0)),QBrush(QColor(255,0,0)));
        #else
          QVector<QPointF> qp;
          qp.push_back(QPointF(x0,y0-6));
          qp.push_back(QPointF(x0-7,y0+6));
          qp.push_back(QPointF(x0+7,y0+6));
          scene_ptr->addPolygon(QPolygonF(qp),QPen(QColor(0,0,0)),QBrush(QColor(0,0,0)));  
        #endif
        if (calculate_merge_value(*c)>0)
          scene.addEllipse(QRectF(x0-(*c)->radius,y0-(*c)->radius,2*(*c)->radius,2*(*c)->radius));
      }
      render(6);
    #endif

    wall_off_chokepoints(&g,&arr);

    #ifdef DEBUG_DRAW
      log("Drawing results of step 7");
      draw_polygons(&polygons);
      draw_arrangement(&arr);
      for(std::set<Node*>::iterator r=g.regions_begin();r!=g.regions_end();r++)
      {
        double x0=cast_to_double((*r)->point.x());
        double y0=cast_to_double((*r)->point.y());  
        for(std::set<Node*>::iterator n=(*r)->neighbors.begin();n!=(*r)->neighbors.end();n++)
        {
          double x1=cast_to_double((*n)->point.x());
          double y1=cast_to_double((*n)->point.y());
          QPen qp(QColor(0,0,0));
          qp.setWidth(2);
          scene.addLine(QLineF(x0,y0,x1,y1),qp);
        }
        #ifdef DRAW_COLOR
          scene.addEllipse(QRectF(x0-6,y0-6,12,12),QPen(QColor(0,0,255)),QBrush(QColor(0,0,255)));
        #else
          scene.addEllipse(QRectF(x0-6,y0-6,12,12),QPen(QColor(0,0,0)),QBrush(QColor(0,0,0)));
        #endif
      }
      for(std::set<Node*>::iterator c=g.chokepoints_begin();c!=g.chokepoints_end();c++)
      {
        double x0=cast_to_double((*c)->point.x());
        double y0=cast_to_double((*c)->point.y());
//        scene.addEllipse(QRectF(x0-3,y0-3,6,6),QPen(QColor(255,0,0)),QBrush(QColor(255,0,0)));
//        scene.addRect(QRectF(x0-5,y0-5,10,10),QPen(QColor(0,0,0)),QBrush(QColor(0,0,0)));
        #ifdef DRAW_COLOR
          scene.addEllipse(QRectF(x0-6,y0-6,12,12),QPen(QColor(255,0,0)),QBrush(QColor(255,0,0)));
        #else
          QVector<QPointF> qp;
          qp.push_back(QPointF(x0,y0-6));
          qp.push_back(QPointF(x0-7,y0+6));
          qp.push_back(QPointF(x0+7,y0+6));
          scene_ptr->addPolygon(QPolygonF(qp),QPen(QColor(0,0,0)),QBrush(QColor(0,0,0)));  
        #endif

      }
      render(7);
    #endif
    log("Finding regions.");
    BWTA_Result::chokepoints.clear();
    std::map<Node*,Region*> node2region;
    for(std::set<Node*>::iterator r=g.regions_begin();r!=g.regions_end();r++)
    {
      Polygon poly;
      PolygonD pd=(*r)->get_polygon();
      for(int i=0;i<pd.size();i++)
      {
        poly.push_back(BWAPI::Position((int)(pd[i].x()*8),(int)(pd[i].y()*8)));
      }
      Region* new_region= new RegionImpl(poly);
      BWTA_Result::regions.insert(new_region);
      node2region.insert(std::make_pair(*r,new_region));
    }

    #ifdef DEBUG_DRAW
      log("Drawing results of step 8");
      draw_polygons(&polygons);
      #ifdef DRAW_COLOR
      
        for(std::set<Node*>::iterator r=g.regions_begin();r!=g.regions_end();r++)
        {
          (*r)->hue=rand()*1.0/RAND_MAX;
        }
        for(int l=0;l<6;l++)
        {
          for(std::set<Node*>::iterator r=g.regions_begin();r!=g.regions_end();r++)
          {
            for(std::set<Node*>::iterator n=(*r)->neighbors.begin();n!=(*r)->neighbors.end();n++)
            {
              Node* r2=(*n)->other_neighbor(*r);
              double d=r2->hue-(*r)->hue;
              if (d>0.5) d=d-1.0;
              if (d<-0.5) d=d+1.0;
              double s=d-0.5;
              if (d<0) s+=1.0;
              s*=0.05;
              (*r)->hue+=s;
              r2->hue-=s;
              while ((*r)->hue<0) (*r)->hue+=1.0;
              while ((*r)->hue>=1.0) (*r)->hue-=1.0;
              while (r2->hue<0) r2->hue+=1.0;
              while (r2->hue>=1.0) r2->hue-=1.0;
            }
          }
        }
        for(std::set<Node*>::iterator r=g.regions_begin();r!=g.regions_end();r++)
        {
          double x0=cast_to_double((*r)->point.x());
          double y0=cast_to_double((*r)->point.y());
          PolygonD boundary=(*r)->get_polygon();
          QVector<QPointF> qp;
          for(int i=0;i<boundary.size();i++)
          {
            qp.push_back(QPointF(boundary.vertex(i).x(),boundary.vertex(i).y()));
          }
          scene.addPolygon(QPolygonF(qp),QPen(QColor(0,0,0)),QBrush(hsl2rgb((*r)->hue,1.0,0.75)));    
  //        scene.addPolygon(QPolygonF(qp),QPen(QColor(0,0,0)),QBrush(QColor(255,255,255)));    
        }
      
      #endif
      draw_arrangement(&arr);
      render(8);
    #endif

    log("Finding chokepoints and linking them to regions.");
    std::map<Node*,Chokepoint*> node2chokepoint;
    for(std::set<Node*>::iterator c=g.chokepoints_begin();c!=g.chokepoints_end();c++)
    {
      std::set<Node*>::iterator i=(*c)->neighbors.begin();
      Region* r1=node2region[*i];
      i++;
      Region* r2=node2region[*i];
      BWAPI::Position side1((int)(cast_to_double((*c)->side1.x())*8),(int)(cast_to_double((*c)->side1.y())*8));
      BWAPI::Position side2((int)(cast_to_double((*c)->side2.x())*8),(int)(cast_to_double((*c)->side2.y())*8));
      Chokepoint* new_chokepoint= new ChokepointImpl(std::make_pair(r1,r2),std::make_pair(side1,side2));
      BWTA_Result::chokepoints.insert(new_chokepoint);
      node2chokepoint.insert(std::make_pair(*c,new_chokepoint));
    }
    log("Linking regions to chokepoints.");
    for(std::set<Node*>::iterator r=g.regions_begin();r!=g.regions_end();r++)
    {
      Region* region=node2region[*r];
      std::set<Chokepoint*> chokepoints;
      for(std::set<Node*>::iterator i=(*r)->neighbors.begin();i!=(*r)->neighbors.end();i++)
      {
        chokepoints.insert(node2chokepoint[*i]);
      }
      ((RegionImpl*)region)->_chokepoints=chokepoints;
    }
    calculate_connectivity();
    calculate_base_location_properties(get_component,components,BWTA_Result::baselocations);
    log("Calculated base location properties.");
    
	// This looks like a file output to plot the distance transform of the map using Mathematica
    /*#ifdef DEBUG_DRAW
      writeFile("bwapi-data/logs/heightMap.txt","h={");
      string comma1="";
      string comma2="";
      for(int x=0;x<BWAPI::Broodwar->mapWidth();x++)
      {
        writeFile("bwapi-data/logs/heightMap.txt","%s{",comma1.c_str());
        comma1=",";
        comma2="";
        for(int y=0;y<BWAPI::Broodwar->mapHeight();y++)
        {
          BWAPI::Position p(x*32+16,y*32+16);
          double dist=p.getDistance(BWTA::getNearestUnwalkablePosition(p));
          if (BWTA::getRegion(x,y)==NULL)
            dist=0;
          writeFile("bwapi-data/logs/heightMap.txt","%s%d",comma2.c_str(),(int)dist);
          comma2=",";
        }
        writeFile("bwapi-data/logs/heightMap.txt","}");
      }
      writeFile("bwapi-data/logs/heightMap.txt","}\n");
      writeFile("bwapi-data/logs/heightMap.txt","p1=ListPlot3D[h, Mesh -> None, BoxRatios -> {100, 100, 10}]\n");
      writeFile("bwapi-data/logs/heightMap.txt","c={\n");
      comma1="";
      for(std::set<Chokepoint*>::const_iterator i=BWTA::getChokepoints().begin();i!=getChokepoints().end();i++)
      {
        BWAPI::Position p((*i)->getCenter().x(),(*i)->getCenter().y());
        double dist=p.getDistance(BWTA::getNearestUnwalkablePosition(p));
        writeFile("bwapi-data/logs/heightMap.txt","%s{%d,%d,%d}\n",comma1.c_str(),(*i)->getCenter().y()/32,(*i)->getCenter().x()/32,(int)dist);
        comma1=",";
      }
      writeFile("bwapi-data/logs/heightMap.txt","}\n");
      writeFile("bwapi-data/logs/heightMap.txt","p2=ListPointPlot3D[c,PlotStyle -> PointSize[Large]]\n");
      writeFile("bwapi-data/logs/heightMap.txt","Show[p1,p2]\n");
    #endif*/
    log("Created result sets.");
  }
  #ifdef DEBUG_DRAW
    int render(int step)
    {
      if (step==1 || step==8 || true)
      {
        QImage* image = new QImage(BWAPI::Broodwar->mapWidth()*8,BWAPI::Broodwar->mapHeight()*8, QImage::Format::Format_ARGB32_Premultiplied);
        QPainter* p = new QPainter(image);
        p->setRenderHint(QPainter::Antialiasing);
        scene_ptr->render(p);
        p->end();
        // Save it..
        std::string filename("bwapi-data/BWTA/");
        filename+=BWAPI::Broodwar->mapFileName();
        if (step==1)
          filename+="-1.png";
        else if (step==2)
          filename+="-2.png";
        else if (step==3)
          filename+="-3.png";
        else if (step==4)
          filename+="-4.png";
        else if (step==5)
          filename+="-5.png";
        else if (step==6)
          filename+="-6.png";
        else if (step==7)
          filename+="-7.png";
        else if (step==8)
          filename+="-8.png";
        image->save(filename.c_str(), "PNG");
      }

      // show a GUI with the image
      /*QGraphicsView* view = new QGraphicsView(scene_ptr);
      CGAL::Qt::GraphicsViewNavigation navigation;
      view->installEventFilter(&navigation);
      view->viewport()->installEventFilter(&navigation);
      view->setRenderHint(QPainter::Antialiasing);
      view->show();
      app_ptr->exec();*/
      
      QList<QGraphicsItem *> list = scene_ptr->items();
      QList<QGraphicsItem *>::Iterator it = list.begin();
      for ( ; it != list.end(); ++it )
      {
        if ( *it )
        {
          scene_ptr->removeItem(*it);
          delete *it;
        }
      }
      return 0;
    }
    void draw_arrangement(Arrangement_2* arr_ptr)
    {
      for (Arrangement_2::Edge_iterator eit = arr_ptr->edges_begin(); eit != arr_ptr->edges_end(); ++eit)
      {
        double x0=cast_to_double(eit->curve().source().x());
        double y0=cast_to_double(eit->curve().source().y());
        double x1=cast_to_double(eit->curve().target().x());
        double y1=cast_to_double(eit->curve().target().y());
        QColor color(0,0,0);
        if (eit->data()==BLUE)
        {
          color=QColor(0,0,255);
        }
        else if (eit->data()==BLACK)
        {
          color=QColor(0,0,0);
        }
        else if (eit->data()==RED)
        {
          color=QColor(255,0,0);
        }
        else
        {
          color=QColor(0,180,0);
        }
        #ifndef DRAW_COLOR
          //all black
          color=QColor(0,0,0);
        #endif
        QPen qp(color);
        qp.setWidth(2);
        scene_ptr->addLine(QLineF(x0,y0,x1,y1),qp);
      }
    }
    void draw_border()
    {
      QPen qp(QColor(0,0,0));
      qp.setWidth(2);
      scene_ptr->addLine(QLineF(0,0,0,MapData::walkability.getHeight()-1),qp);
      scene_ptr->addLine(QLineF(0,MapData::walkability.getHeight()-1,MapData::walkability.getWidth()-1,MapData::walkability.getHeight()-1),qp);
      scene_ptr->addLine(QLineF(MapData::walkability.getWidth()-1,MapData::walkability.getHeight()-1,MapData::walkability.getWidth()-1,0),qp);
      scene_ptr->addLine(QLineF(MapData::walkability.getWidth()-1,0,0,0),qp);
    }
    void draw_polygon(Polygon& p, QColor qc)
    {
      QVector<QPointF> qp;
      for(int i=0;i<(int)p.size();i++)
      {
        int j=(i+1)%p.size();
        //scene_ptr->addLine(QLineF(p[i].x(),p[i].y(),p[j].x(),p[j].y()),qc);
        qp.push_back(QPointF(p[i].x(),p[i].y()));
      }
      scene_ptr->addPolygon(QPolygonF(qp),QPen(QColor(0,0,0)),QBrush(qc));  
    }
    void draw_polygons(vector<Polygon>* polygons_ptr)
    {
      for(int i=0;i<(int)polygons_ptr->size();i++)
      {
        Polygon boundary=(*polygons_ptr)[i];
        draw_polygon(boundary,QColor(180,180,180));
        for(vector<Polygon>::iterator h=boundary.holes.begin();h!=boundary.holes.end();h++)
        {
          draw_polygon(*h,QColor(255,100,255));
        }
      }
    }
  #endif
  void remove_voronoi_diagram_from_arrangement(Arrangement_2* arr_ptr)
  {
    bool redo=true;
    while (redo)
    {
      redo=false;
      for (Arrangement_2::Edge_iterator eit = arr_ptr->edges_begin(); eit != arr_ptr->edges_end();++eit)
      {
        if (eit->data()==BLUE)
        {
          arr_ptr->remove_edge(eit);
          redo=true;
          break;
        }
      }
      if (!redo)
      {
        for (Arrangement_2::Vertex_iterator vit = arr_ptr->vertices_begin(); vit != arr_ptr->vertices_end();++vit)
        {
          if (vit->data().c==BLUE && vit->degree()==0)
          {
            arr_ptr->remove_isolated_vertex(vit);
            redo=true;
            break;
          }
        }
      }
    }
  }
  void simplify_voronoi_diagram(Arrangement_2* arr_ptr, std::map<Point, double, ptcmp>* distance)
  {
    bool redo=true;
    while (redo)
    {
      redo=false;
      for (Arrangement_2::Edge_iterator eit = arr_ptr->edges_begin(); eit != arr_ptr->edges_end();++eit)
      {
        if (eit->data()==BLUE)
        {
          if (eit->source()->data().c==BLACK || eit->target()->data().c==BLACK ||
              eit->source()->data().c==NONE  || eit->target()->data().c==NONE)
          {
            arr_ptr->remove_edge(eit,false,true);
            redo=true;
            break;
          }
        }
      }
    }
    for (Arrangement_2::Vertex_iterator vit = arr_ptr->vertices_begin(); vit != arr_ptr->vertices_end(); ++vit)
    {
      if (vit->data().c==BLUE)
      {
        Point p(vit->point().x(),vit->point().y());
        double dist_node=distance->find(p)->second;
        vit->data().radius=dist_node;
      }
    }
    redo=true;
    std::set<Arrangement_2::Vertex_handle,vhradiuscmp> vertices;
    while (redo)
    {
      redo=false;
      for (Arrangement_2::Vertex_iterator vit = arr_ptr->vertices_begin(); vit != arr_ptr->vertices_end(); ++vit)
      {
        vertices.insert(vit);
      }
      for(std::set<Arrangement_2::Vertex_handle,vhradiuscmp>::iterator vh=vertices.begin();vh!=vertices.end();vh++)
      {
        if ((*vh)->data().c==BLUE)
        {
          double dist_node=(*vh)->data().radius;
          if ((*vh)->degree()==0)
          {
            if (dist_node<10)
            {
              arr_ptr->remove_isolated_vertex(*vh);
            }
          }
          else if ((*vh)->degree()==1)
          {
            Arrangement_2::Halfedge_handle h((*vh)->incident_halfedges());
            double dist_parent;
            if (h->source()==*vh)
            {
              dist_parent=h->target()->data().radius;
            }
            else
            {
              dist_parent=h->source()->data().radius;
            }
            if (dist_node<=dist_parent || dist_node<10)
            {
              arr_ptr->remove_edge(h,false,true);
              redo=true;
              break;
            }
          }
        }
      }
      vertices.clear();
    }
  }

  void identify_region_nodes(Arrangement_2* arr_ptr,Graph* g_ptr)
  {
    log("Identifying Region Nodes");
    for (Arrangement_2::Vertex_iterator vit = arr_ptr->vertices_begin(); vit != arr_ptr->vertices_end(); ++vit)
    {
      if (vit->data().c==BLUE)
      {
        if (vit->degree()!=2)
        {
          g_ptr->add_region(Point(vit->point().x(),vit->point().y()),vit->data().radius,vit);
          vit->data().is_region=true;
        }
        else
        {
          Arrangement_2::Vertex_handle start_node=vit;
          double start_radius=vit->data().radius;
          if (start_radius>4*4)
          {
            bool region=true;

            Arrangement_2::Halfedge_around_vertex_circulator e=vit->incident_halfedges();
            Arrangement_2::Vertex_handle node=vit;
            for(int i=0;i<2;i++)
            {
              double distance_travelled=0;
              Arrangement_2::Halfedge_around_vertex_circulator mye=e;
              while(distance_travelled<start_radius && region)
              {
                if (mye->source()==node)
                  node=mye->target();
                else
                  node=mye->source();
                distance_travelled+=get_distance(mye->target()->point(),mye->source()->point());
                if (node->data().radius>=start_radius)
                  region=false;
                Arrangement_2::Halfedge_around_vertex_circulator newe=node->incident_halfedges();
                if (Arrangement_2::Halfedge_handle(newe)==Arrangement_2::Halfedge_handle(mye->twin()))
                  newe++;
                mye=newe;
              }
              if (!region) break;
              e++;
            }
            if (region)
            {
              g_ptr->add_region(Point(vit->point().x(),vit->point().y()),vit->data().radius,vit);
              vit->data().is_region=true;
            }
          }
        }
      }
    }
  }

  void identify_chokepoint_nodes(Graph* g_ptr, std::map<Point, double, ptcmp>* distance, std::map<Point, std::set< Point >, ptcmp >* nearest)
  {
    log("Identifying Chokepoint Nodes");
    std::set<Node*> chokepoints_to_merge;
    for(std::set<Node*>::iterator r=g_ptr->regions_begin();r!=g_ptr->regions_end();r++)
    {
      log("On next region");
      if ((*r)->handle->is_isolated()) continue;
      bool first_outer=true;
      for(Arrangement_2::Halfedge_around_vertex_circulator e=(*r)->handle->incident_halfedges();
        first_outer || e!=(*r)->handle->incident_halfedges();e++)
      {
        log("On next chokepoint of current region");
        first_outer=false;
        Arrangement_2::Vertex_handle node=(*r)->handle;
        Arrangement_2::Vertex_handle chokepoint_node=(*r)->handle;
        bool first=true;
        double min_radius;
        Arrangement_2::Halfedge_around_vertex_circulator mye=e;
        Point pt;
        Point min_pt;
        double distance_before=0;
        double distance_after=0;
        double distance_travelled=0;

        int count=0;
        while(first || node->data().is_region==false)
        {
          count++;
          if (count>1000)
          {
            log("Error: Could not find other end of chokepoint path.");
            break;
          }
          log("On next vertex of current chokepoint path");
          pt=Point(node->point().x(),node->point().y());
          double dist=node->data().radius;
          if (first || dist<min_radius || (dist==min_radius && pt<min_pt))
          {
            first=false;
            min_radius=dist;
            min_pt=pt;
            chokepoint_node=node;
            distance_after=0;
          }

          if (mye->source()==node)
            node=mye->target();
          else
            node=mye->source();
          distance_travelled+=get_distance(mye->target()->point(),mye->source()->point());
          distance_after+=get_distance(mye->target()->point(),mye->source()->point());
          Arrangement_2::Halfedge_around_vertex_circulator newe=node->incident_halfedges();
          if (Arrangement_2::Halfedge_handle(newe)==Arrangement_2::Halfedge_handle(mye->twin()))
            newe++;
          mye=newe;
        }
        if (count>1000) continue;
        distance_before=distance_travelled-distance_after;
        bool is_last=false;
        pt=Point(node->point().x(),node->point().y());
        double dist=node->data().radius;
        if (first || dist<min_radius || (dist==min_radius && pt<min_pt))
        {
          first=false;
          min_radius=dist;
          chokepoint_node=node;
          is_last=true;
        }
        Node* other_region=g_ptr->get_region(Point(node->point().x(),node->point().y()));
        Node* new_chokepoint = NULL;
        if (*r!=other_region)
          new_chokepoint =g_ptr->add_chokepoint(Point(chokepoint_node->point().x(),chokepoint_node->point().y()),chokepoint_node->data().radius,chokepoint_node,*r,other_region);
        else
          continue;
        bool adding_this=true;
        if (is_last)
        {
          chokepoints_to_merge.insert(new_chokepoint);
          adding_this=false;
        }
        if (chokepoint_node!=(*r)->handle && adding_this)
        {
          // assign side1 and side2
          NumberType x0=chokepoint_node->point().x();
          NumberType y0=chokepoint_node->point().y();
          std::set<Point> npoints=(*nearest)[Point(x0,y0)];
          if (npoints.size()==2)
          {
            std::set<Point>::iterator p=npoints.begin();
            new_chokepoint->side1=*p;
            p++;
            new_chokepoint->side2=*p;
          }
          else
          {
            double min_pos_angle=PI;
            Point min_pos_point=*npoints.begin();
            double max_neg_angle=-PI;
            Point max_neg_point=*npoints.begin();
            double max_dist=-1;
            for(std::set<Point>::iterator p=npoints.begin();p!=npoints.end();p++)
              for(std::set<Point>::iterator p2=npoints.begin();p2!=npoints.end();p2++)
              {
                double dist=sqrt(to_double((p->x()-p2->x())*(p->x()-p2->x())+(p->y()-p2->y())*(p->y()-p2->y())));
                if (dist>max_dist)
                {
                  max_dist=dist;
                  min_pos_point=*p;
                  max_neg_point=*p2;
                }
              }
            new_chokepoint->side1=min_pos_point;
            new_chokepoint->side2=max_neg_point;
          }
        }
      }
    }
    log("Merging regions");
    for(std::set<Node*>::iterator i=chokepoints_to_merge.begin();i!=chokepoints_to_merge.end();i++)
    {
      g_ptr->merge_chokepoint(*i);
    }
    log("Done Identifying Chokepoint Nodes");
  }
  double calculate_merge_value(Node* c)
  {
    Node* smaller=c->a_neighbor();
    Node* larger=c->other_neighbor(smaller);
    if (larger->radius<smaller->radius)
    {
      Node* temp=larger;
      larger=smaller;
      smaller=temp;
    }
    double lt=0.85;
    double st=0.9;
    double diff=max(c->radius-larger->radius*lt,c->radius-smaller->radius*st);
    for(Node* region=smaller;;region=larger)
    {
      if (region->neighbors.size()==2)
      {
        Node* otherc=region->other_neighbor(c);
        double d1x=to_double(c->point.x()-region->point.x());
        double d1y=to_double(c->point.y()-region->point.y());
        double d1=sqrt(d1x*d1x+d1y*d1y);
        d1x/=d1;
        d1y/=d1;
        double d2x=to_double(otherc->point.x()-region->point.x());
        double d2y=to_double(otherc->point.y()-region->point.y());
        double d2=sqrt(d2x*d2x+d2y*d2y);
        d2x/=d2;
        d2y/=d2;
        double dx=0.5*(d1x-d2x);
        double dy=0.5*(d1y-d2y);
        double dist=sqrt(dx*dx+dy*dy);
        if (c->radius > otherc->radius || (c->radius == otherc->radius && c->point > otherc->point))
        {
          double value=c->radius*dist-region->radius*0.7;
          diff=max(diff,value);
        }
      }
      if (region==larger)
        break;
    }
    return diff;
  }
  void merge_adjacent_regions(Graph* g_ptr)
  {
    std::set<Node*> chokepoints_to_merge;
    bool finished;
    finished=false;
    while (!finished)
    {
      finished=true;
      Node* chokepoint_to_merge=NULL;
      Heap<Node*, double> h(false);
      for(std::set<Node*>::iterator c=g_ptr->chokepoints_begin();c!=g_ptr->chokepoints_end();c++)
      {
        double cost=(*c)->radius;
        if (calculate_merge_value(*c)>0)
          h.set(*c,cost);
      }
      finished = (h.size() == 0);
      while (h.size()>0)
      {
        while (h.size()>0 && !g_ptr->has_chokepoint(h.top().first))
          h.pop();
        if (h.size()==0)
          break;
        chokepoint_to_merge=h.top().first;
        h.pop();
        Node* smaller=chokepoint_to_merge->a_neighbor();
        Node* larger=chokepoint_to_merge->other_neighbor(smaller);
        if (larger->radius<smaller->radius)
        {
          Node* temp=larger;
          larger=smaller;
          smaller=temp;
        }
        double smaller_radius=smaller->radius;
        smaller->radius=larger->radius;
        std::set<Node*> affectedChokepoints;
        for(std::set<Node*>::iterator c=smaller->neighbors.begin();c!=smaller->neighbors.end();c++)
        {
          affectedChokepoints.insert(*c);
          Node* otherr=(*c)->other_neighbor(smaller);
          for(std::set<Node*>::iterator c2=otherr->neighbors.begin();c2!=otherr->neighbors.end();c2++)
          {
            affectedChokepoints.insert(*c2);
          }
        }
        for(std::set<Node*>::iterator c=larger->neighbors.begin();c!=larger->neighbors.end();c++)
        {
          affectedChokepoints.insert(*c);
          Node* otherr=(*c)->other_neighbor(larger);
          for(std::set<Node*>::iterator c2=otherr->neighbors.begin();c2!=otherr->neighbors.end();c2++)
          {
            affectedChokepoints.insert(*c2);
          }
        }
        for(std::set<Node*>::iterator c=affectedChokepoints.begin();c!=affectedChokepoints.end();c++)
        {
          double cost=(*c)->radius;
          if (calculate_merge_value(*c)>0)
            h.set(*c,cost);
          else
          {
            if (h.contains(*c))
              h.erase(*c);
          }
        }
        smaller->radius=smaller_radius;
        g_ptr->merge_chokepoint(chokepoint_to_merge);
      }
    }
    bool redo=true;
    while(redo)
    {
      redo=false;
      for(std::set<Node*>::iterator r=g_ptr->regions_begin();r!=g_ptr->regions_end();r++)
      {
        if ((*r)->radius<8)
        {
          g_ptr->remove_region(*r);
          redo=true;
          break;
        }
      }
    }
  }

  void wall_off_chokepoints(Graph* g_ptr,Arrangement_2* arr_ptr)
  {
    std::list< Segment_2 > new_segments;
    for(std::set<Node*>::iterator c=g_ptr->chokepoints_begin();c!=g_ptr->chokepoints_end();c++)
    {
      Node* chokepoint_node=*c;
      NumberType x0=chokepoint_node->point.x();
      NumberType y0=chokepoint_node->point.y();
      CGAL::insert_point(*arr_ptr,Point_2(x0,y0));
      NumberType x1=chokepoint_node->side1.x();
      NumberType y1=chokepoint_node->side1.y();
      NumberType x2=chokepoint_node->side2.x();
      NumberType y2=chokepoint_node->side2.y();

      double length=sqrt(to_double((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0)));
      x1+=(x1-x0)/length;
      y1+=(y1-y0)/length;
      new_segments.push_back(Segment_2(Point_2(x0,y0),Point_2(x1,y1)));

      length=sqrt(to_double((x2-x0)*(x2-x0)+(y2-y0)*(y2-y0)));
      x2+=(x2-x0)/length;
      y2+=(y2-y0)/length;
      new_segments.push_back(Segment_2(Point_2(x0,y0),Point_2(x2,y2)));
    }
    for (Arrangement_2::Vertex_iterator vit = arr_ptr->vertices_begin(); vit != arr_ptr->vertices_end(); ++vit)
    {
      if (vit->data().c!=BLACK)
      {
        vit->data().c=RED;
      }
    }
    for(std::list<Segment_2>::iterator s=new_segments.begin();s!=new_segments.end();s++)
    {
      CGAL::insert(*arr_ptr,*s);
    }
    for (Arrangement_2::Edge_iterator eit = arr_ptr->edges_begin(); eit != arr_ptr->edges_end(); ++eit)
    {
      if ((eit->data()!=BLACK && eit->data()!=BLUE) || eit->source()->data().c==RED || eit->target()->data().c==RED)
      {
        eit->data()=RED;
        eit->twin()->data()=RED;
      }
    }
    bool redo=true;
    while (redo)
    {
      redo=false;
      for (Arrangement_2::Edge_iterator eit = arr_ptr->edges_begin(); eit != arr_ptr->edges_end();++eit)
      {
        if (eit->data()==RED)
        {
          if (eit->source()->degree()==1 || eit->target()->degree()==1 || (eit->source()->data().c!=RED && eit->target()->data().c!=RED))
          {
            arr_ptr->remove_edge(eit);
            redo=true;
            break;
          }
        }
      }
    }
  }
}
