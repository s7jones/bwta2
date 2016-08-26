#include "terrain_analysis.h"

namespace BWTA
{
	bool createDir(std::string& path)
	{
		boost::filesystem::path dir(path);
		if (boost::filesystem::create_directories(dir)) {
			return true;
		}
		return false;
	}

	class My_observer : public CGAL::Arr_observer < Arrangement_2 >
	{
	public:
		My_observer(Arrangement_2& arr)
			: CGAL::Arr_observer<Arrangement_2>(arr)
		{}

		virtual void after_split_edge(Halfedge_handle e1, Halfedge_handle e2)
		{
			if (e1->data() == BLACK || e1->data() == BLUE || e1->data() == RED) {
				e1->twin()->set_data(e1->data());
				e2->set_data(e1->data());
				e2->twin()->set_data(e1->data());
			} else if (e2->data() == BLACK || e2->data() == BLUE || e2->data() == RED) {
				e2->twin()->set_data(e2->data());
				e1->set_data(e2->data());
				e1->twin()->set_data(e2->data());
			} else if (e1->twin()->data() == BLACK || e1->twin()->data() == BLUE || e1->twin()->data() == RED) {
				e1->set_data(e1->twin()->data());
				e2->set_data(e1->twin()->data());
				e2->twin()->set_data(e1->twin()->data());
			} else if (e2->twin()->data() == BLACK || e2->twin()->data() == BLUE || e2->twin()->data() == RED) {
				e2->set_data(e2->twin()->data());
				e1->set_data(e2->twin()->data());
				e1->twin()->set_data(e2->twin()->data());
			}
		}
	};

	void readMap(){} // for backwards interface compatibility

	void analyze()
	{
		cleanMemory();

		Timer timer;

#ifndef OFFLINE
		loadMapFromBWAPI();
#endif

		// compute extra map info
		loadMap();

		// Verify if "bwta2" directory exists, and create it if it doesn't.
		std::string bwtaPath(BWTA_PATH);
		if (!boost::filesystem::exists(bwtaPath)) {
			createDir(bwtaPath);
		}

		std::string filename = bwtaPath + MapData::hash + ".bwta";

		if (fileExists(filename) && fileVersion(filename) == BWTA_FILE_VERSION) {
			LOG("Recognized map, loading map data...");

			timer.start();
			load_data(filename);
			LOG("Loaded map data in " << timer.stopAndGetTime() << " seconds");
		} else {
			LOG("Analyzing new map...");

			timer.start();
			analyze_map();
			LOG("Map analyzed in " << timer.stopAndGetTime() << " seconds");

			save_data(filename);
			LOG("Saved map data.");
		}

#ifndef OFFLINE
		attachResourcePointersToBaseLocations(BWTA_Result::baselocations);
#endif
	}

	void analyze_map()
	{
#ifdef DEBUG_DRAW
		Painter painter;
#endif
		Timer timer;
		timer.start();

		// *************************************************
		// OUR METHOD
		// *************************************************

		std::vector<Polygon> polygons;
		std::vector<BoostPolygon> boostPolygons;
		RectangleArray<int> labelMap(MapData::walkability.getWidth(), MapData::walkability.getHeight());
// 		LOG("Size of labelMap " << MapData::walkability.getWidth() << "x" << MapData::walkability.getHeight());
		labelMap.setTo(0);
		generatePolygons(boostPolygons, labelMap);

		// translate Boost polygons to BWTA polygons
		for (const auto& pol : boostPolygons) {
			Polygon BwtaPolygon;
			for (const auto& pos : pol.outer()) BwtaPolygon.emplace_back((int)pos.x(), (int)pos.y());
			// TODO add holes
			polygons.push_back(BwtaPolygon);
		}

		LOG(" [Detected BOOST polygons in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		painter.drawPolygons(polygons);
		painter.render("1-BoostPolygons");
// 		for (auto tmpPol : polygons) {
// 			painter.drawPolygon(tmpPol, QColor(180, 180, 180));
// 			painter.render();
// 		}
#endif
		timer.start();

		RegionGraph graph;
		bgi::rtree<BoostSegmentI, bgi::quadratic<16> > rtree;
		generateVoronoid(polygons, labelMap, graph, rtree);
		
		LOG(" [Computed BOOST Voronoi in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		painter.drawPolygons(polygons);
		painter.drawGraph(graph);
		painter.render("2-BoostVoronoi");
#endif
		timer.start();

		pruneGraph(graph);

		LOG(" [Pruned Voronoi in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		painter.drawPolygons(polygons);
		painter.drawGraph(graph);
		painter.render("3-VoronoiPruned");
#endif
		timer.start();

		markRegionNodes(graph, polygons);

		LOG(" [Identified region/chokepoints nodes in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		painter.drawPolygons(polygons);
		painter.drawGraph(graph);
		painter.drawNodes(graph, graph.regionNodes, Qt::blue);
		painter.drawNodes(graph, graph.chokeNodes, Qt::red);
		painter.render("5-NodesDetected");
#endif
		timer.start();

		RegionGraph graphSimplified;
		simplifyRegionGraph(graph, graphSimplified);

		LOG(" [Simplified region graph in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		painter.drawPolygons(polygons);
		painter.drawGraph(graphSimplified);
		painter.drawNodes(graphSimplified, graphSimplified.regionNodes, Qt::blue);
		painter.drawNodes(graphSimplified, graphSimplified.chokeNodes, Qt::red);
		painter.render("6-NodesPruned");
#endif
		timer.start();

		std::map<nodeID, chokeSides_t> chokepointSides;
		getChokepointSides(polygons, graphSimplified, rtree, chokepointSides);

		LOG(" [Wall of chokepoints in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		painter.drawPolygons(polygons);
		painter.drawGraph(graphSimplified);
		painter.drawNodes(graphSimplified, graphSimplified.regionNodes, Qt::blue);
		painter.drawLines(chokepointSides, Qt::red);
		painter.render("7-WallOffChokepoints");
#endif
		timer.start();

		std::vector<BoostPolygon> polReg;
		createRegionsFromGraph(boostPolygons, labelMap, graphSimplified, chokepointSides,
			BWTA_Result::regions, BWTA_Result::chokepoints,
			polReg);

		LOG(" [Creating BWTA regions/chokepoints in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		painter.drawPolygons(polReg);
		painter.render("8-Regions");
#endif
		timer.start();


		// *************************************************
		// BWTA ORIGINAL
		// *************************************************
/*

		// Give find_connected_components the walkability data so it can compute the list of connected components,
		// and determine which component each tile belongs to
		RectangleArray<ConnectedComponent*> get_component;
		std::list<ConnectedComponent> components;
		find_connected_components(MapData::walkability, get_component, components);
		LOG(" - Calculated connected components");

		// Give extract_polygons the walkability data and connected components so it can compute the polygonal obstacles
		std::vector<Polygon> polygons;
		extract_polygons(MapData::walkability, components, polygons);
		LOG(" - Extracted " << polygons.size() << " polygons.");

		// Discard polygons that are too small
		int removed = 0;
		for (size_t p = 0; p < polygons.size();) {
		if (std::abs(polygons[p].getArea()) <= 256 &&
		distance_to_border(polygons[p], MapData::walkability.getWidth(), MapData::walkability.getHeight()) > 1) {
		polygons.erase(polygons.begin() + p);
		removed++;
		} else {
		p++;
		}
		}
		LOG(" - Removed " << removed << " small polygons.");

		// Save the remaining polygons in BWTA_Result::unwalkablePolygons
		for (const auto& polygon : polygons) BWTA_Result::unwalkablePolygons.insert(new Polygon(polygon));

		LOG(" [Detected polygons in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		painter.drawPolygons(polygons);
		painter.render();
#endif
		timer.start();


		// All line segments we create we will store in the segments vector and also insert into the 2d segmented Delaunay graph object sdg
		// Create the segments vector and 2d segmented Delaunay graph
		std::vector<SDG2::Site_2> segments;
		SDG2 sdg;

		// Set vertices of the map
		Point topLeft(0, 0);
		Point bottomLeft(0, MapData::walkability.getHeight() - 1);
		Point bottomRight(MapData::walkability.getWidth() - 1, MapData::walkability.getHeight() - 1);
		Point topRight(MapData::walkability.getWidth() - 1, 0);

		// Add line segments of the 4 edges of the map
		segments.push_back(SDG2::Site_2::construct_site_2(topLeft, bottomLeft));
// 		segments.push_back(SDG2::Site_2::construct_site_2(bottomLeft, bottomRight)); // this should be unnecessary since bottom tiles are always unwalkable
		segments.push_back(SDG2::Site_2::construct_site_2(bottomRight, topRight));
		segments.push_back(SDG2::Site_2::construct_site_2(topRight, topLeft));

		// Add the line segments of each polygon
		for (const auto& polygon : polygons) {
			// Add the vertices of the polygon
			for (size_t i = 0; i < polygon.size(); i++) {
				int j = (i + 1) % polygon.size();
				Point a(polygon[i].x, polygon[i].y);
				Point b(polygon[j].x, polygon[j].y);
				segments.push_back(SDG2::Site_2::construct_site_2(b, a));
			}
			// Add the vertices of each hole in the polygon
			for (const auto& hole : polygon.holes) {
				for (size_t i = 0; i < hole.size(); i++) {
					int j = (i + 1) % hole.size();
					Point a(hole[i].x, hole[i].y);
					Point b(hole[j].x, hole[j].y);
					segments.push_back(SDG2::Site_2::construct_site_2(b, a));
				}
			}
		}

		// Add all line segments to the sdg
		sdg.insert_segments(segments.begin(), segments.end());

		// Check to see if the 2d segment Delaunay graph is valid
		assert(sdg.is_valid(true, 1));
		LOG(" - Created 2D Segment Delaunay Graph.");

		std::vector< Segment > voronoi_diagram_edges;
		std::map<Point, std::set< Point >, ptcmp > nearest;
		std::map<Point, double, ptcmp> distance;
		get_voronoi_edges(sdg, voronoi_diagram_edges, nearest, distance, polygons);
		LOG(" - Got voronoi edges.");

		Arrangement_2 arr;
		My_observer obs(arr);
		Graph g(&arr);

		// Insert all line segments from polygons into arrangement
		for (const auto& segment : segments) {
			NumberType x0(segment.segment().vertex(0).x());
			NumberType y0(segment.segment().vertex(0).y());
			NumberType x1(segment.segment().vertex(1).x());
			NumberType y1(segment.segment().vertex(1).y());
			if (x0 != x1 || y0 != y1) {
				if (is_real(x0) != 0 && is_real(y0) != 0 && is_real(x1) != 0 && is_real(y1) != 0) {
					CGAL::insert(arr, Segment_2(Point_2(x0, y0), Point_2(x1, y1)));
				}
			}
		}
		LOG(" - Inserted polygons into arrangement.");

		//color all initial segments and vertices from the polygons BLACK
		for (auto eit = arr.edges_begin(); eit != arr.edges_end(); ++eit) eit->data() = BLACK;
		for (auto vit = arr.vertices_begin(); vit != arr.vertices_end(); ++vit) vit->data().c = BLACK;

		// Insert into arrangement all segments from Voronoi diagram which are not bounded by any polygon
		for (unsigned int i = 0; i < voronoi_diagram_edges.size(); i++) {
			NumberType x0(voronoi_diagram_edges[i].vertex(0).x());
			NumberType y0(voronoi_diagram_edges[i].vertex(0).y());
			NumberType x1(voronoi_diagram_edges[i].vertex(1).x());
			NumberType y1(voronoi_diagram_edges[i].vertex(1).y());

			if (x0 != x1 || y0 != y1) {
				if (is_real(x0) != 0 && is_real(y0) != 0
					&& is_real(x1) != 0 && is_real(y1) != 0)
				{
					bool add = true;
					for (unsigned int p = 0; p < polygons.size(); p++) {
						if (polygons[p].isInside(BWAPI::Position(int(cast_to_double(voronoi_diagram_edges[i].vertex(0).x())), int(cast_to_double(voronoi_diagram_edges[i].vertex(0).y())))))
						{
							// First end point of this line segment is inside polygons[p], so don't add it to the arrangement
							add = false;
							break;
						}
						if (polygons[p].isInside(BWAPI::Position(int(cast_to_double(voronoi_diagram_edges[i].vertex(1).x())), int(cast_to_double(voronoi_diagram_edges[i].vertex(1).y())))))
						{
							// Second end point of this line segment is inside polygons[p], so don't add it to the arrangement
							add = false;
							break;
						}
					}
					if (add) {
						CGAL::insert(arr, Segment_2(Point_2(x0, y0), Point_2(x1, y1)));
					}
				}
			}
		}
		LOG(" - Added Voronoi edges.");
		// Color all the new edges BLUE
		for (Arrangement_2::Edge_iterator eit = arr.edges_begin(); eit != arr.edges_end(); ++eit) {
			if (eit->data() != BLACK) {
				eit->data() = BLUE;
				eit->twin()->data() = BLUE;
			}
		}
		// Color all the new vertices BLUE
		for (Arrangement_2::Vertex_iterator vit = arr.vertices_begin(); vit != arr.vertices_end(); ++vit) {
			if (vit->data().c != BLACK) {
				vit->data().c = BLUE;
			}
		}

		LOG(" [Computed Voronoi diagram in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		LOG(" - Drawing Voronoi");
		painter.drawPolygons(polygons);
		painter.drawArrangement(&arr);
		painter.render("2-Voronoi");
#endif
		timer.start();

		simplify_voronoi_diagram(&arr, &distance);

		LOG(" [Pruned Voronoi diagram in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		LOG(" - Drawing Voronoi Pruned");
		painter.drawPolygons(polygons);
		painter.drawArrangement(&arr);
		painter.render("3-VoronoiPruned");
#endif
		timer.start();

		identify_region_nodes(&arr, &g);

		LOG(" [Identified region nodes in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		LOG(" - Drawing region nodes");
		painter.drawPolygons(polygons);
		painter.drawArrangement(&arr);
		painter.drawNodes(g.getRegions(), Qt::blue);
		painter.render("4-RegionNodes");
#endif
		timer.start();

		identify_chokepoint_nodes(&g, &distance, &nearest);

		LOG(" [Identified choke points nodes in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		LOG(" - Drawing chokepoints nodes");
		painter.drawPolygons(polygons);
		painter.drawArrangement(&arr);
		painter.drawNodes(g.getRegions(), Qt::blue);
		painter.drawNodes(g.getChokepoints(), Qt::red);
		painter.render("5-ChokepointsNodes");
#endif
		timer.start();

		merge_adjacent_regions(&g);
		LOG(" - Merged regions.");

		remove_voronoi_diagram_from_arrangement(&arr);
		LOG(" - Removed Voronoi edges.");

		LOG(" [Merged adjacent regions in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		LOG(" - Drawing results of step 6");
		painter.drawPolygons(polygons);
		painter.drawArrangement(&arr);
		painter.drawNodesAndConnectToNeighbors(g.getRegions(), Qt::blue);
		painter.drawNodes(g.getChokepoints(), Qt::red);
		painter.render("6-NodesPruned");
#endif
		timer.start();

		wall_off_chokepoints(&g, &arr);

		LOG(" [Wall of chokepoints in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		LOG(" - Drawing results of step 7");
		painter.drawPolygons(polygons);
		painter.drawArrangement(&arr);
		painter.drawNodesAndConnectToNeighbors(g.getRegions(), Qt::blue);
		painter.drawNodes(g.getChokepoints(), Qt::red);
		painter.render("7-WallOffChokepoints");
#endif
		timer.start();

		BWTA_Result::chokepoints.clear();
		std::map<Node*, Region*> node2region;
		std::vector<Polygon> polygonsNotSimple;
		for (std::set<Node*>::iterator r = g.regions_begin(); r != g.regions_end(); r++) {
			Polygon poly;
			PolygonD pd = (*r)->get_polygon();
			// Translate CGAL polygon (PolygonD) to BWTA polygon (Polygon)
			for (size_t i = 0; i < pd.size(); i++) {
				poly.push_back(BWAPI::Position((int)CGAL::to_double(pd[i].x() * 8), (int)CGAL::to_double(pd[i].y() * 8)));
			}
			// Check if the polygons are still simple
			if (!pd.is_simple()) polygonsNotSimple.push_back(poly);
			// Create the BWTA region
			Region* new_region = new RegionImpl(poly);
			BWTA_Result::regions.insert(new_region);
			node2region.insert(std::make_pair(*r, new_region));
		}

		if (!polygonsNotSimple.empty()) {
			LOG("  ERROR!!! Found " << polygonsNotSimple.size() << " polygons not simple");
#ifdef DEBUG_DRAW
			painter.drawPolygons(polygonsNotSimple);
			painter.render("Error-PolygonsNotSimple");
#endif
		}

		// Computed the polygon from the face of a 2D arrangement
		LOG(" [Creating BWTA regions in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		LOG("Drawing results of step 8");
		painter.drawPolygons(polygons);
		painter.drawFourColorMap(g.getRegions());
		painter.drawArrangement(&arr);
		painter.render("8-Regions");
#endif
		timer.start();

// 		exit(-1);

		LOG(" - Finding chokepoints and linking them to regions.");
		std::map<Node*, Chokepoint*> node2chokepoint;
		for (std::set<Node*>::iterator c = g.chokepoints_begin(); c != g.chokepoints_end(); c++) {
			std::set<Node*>::iterator i = (*c)->neighbors.begin();
			Region* r1 = node2region[*i];
			i++;
			Region* r2 = node2region[*i];
			BWAPI::Position side1((int)(cast_to_double((*c)->side1.x()) * 8), (int)(cast_to_double((*c)->side1.y()) * 8));
			BWAPI::Position side2((int)(cast_to_double((*c)->side2.x()) * 8), (int)(cast_to_double((*c)->side2.y()) * 8));
			Chokepoint* new_chokepoint = new ChokepointImpl(std::make_pair(r1, r2), std::make_pair(side1, side2));
			BWTA_Result::chokepoints.insert(new_chokepoint);
			node2chokepoint.insert(std::make_pair(*c, new_chokepoint));
		}
		LOG(" - Linking regions to chokepoints.");
		for (std::set<Node*>::iterator r = g.regions_begin(); r != g.regions_end(); r++) {
			Region* region = node2region[*r];
			std::set<Chokepoint*> chokepoints;
			for (std::set<Node*>::iterator i = (*r)->neighbors.begin(); i != (*r)->neighbors.end(); i++)
			{
				chokepoints.insert(node2chokepoint[*i]);
			}
			((RegionImpl*)region)->_chokepoints = chokepoints;
		}

		LOG(" [Linked choke points with regions in " << timer.stopAndGetTime() << " seconds]");
		timer.start();

		// END BWTA ORIGINAL
		// *************************************************
*/

		exit(-1);

		detectBaseLocations(BWTA_Result::baselocations);
// 		for (auto i : BWTA_Result::baselocations) {
// 			log("BaseLocation at Position " << i->getPosition() << " Tile " << i->getTilePosition());
// 		}

		LOG(" [Calculated base locations in " << timer.stopAndGetTime() << " seconds]");
		timer.start();

		calculate_connectivity();

		LOG(" [Calculated connectivity in " << timer.stopAndGetTime() << " seconds]");
		timer.start();

		calculateBaseLocationProperties();
// 		log("Debug BaseLocationProperties");
// 		for (const auto& base : BWTA_Result::baselocations) {
// 			BaseLocationImpl* baseI = (BaseLocationImpl*)base;
// 			log("Base Position" << baseI->getTilePosition() << ",isIsland:" << baseI->isIsland()
// 				<< ",isStartLocation:" << baseI->isStartLocation()
// 				<< ",regionCenter" << baseI->getRegion()->getCenter());
// 		}

		LOG(" [Calculated base location properties in " << timer.stopAndGetTime() << " seconds]");
#ifdef DEBUG_DRAW
		painter.drawClosestBaseLocationMap(BWTA_Result::getBaseLocationW, BWTA_Result::baselocations);
		painter.render("ClosestBaseLocationMap");
		painter.drawClosestChokepointMap(BWTA_Result::getChokepointW, BWTA_Result::chokepoints);
		painter.render("ClosestChokepointMap");
#endif

	}

	void remove_voronoi_diagram_from_arrangement(Arrangement_2* arr_ptr)
	{
		bool redo = true;
		while (redo) {
			redo = false;
			for (Arrangement_2::Edge_iterator eit = arr_ptr->edges_begin(); eit != arr_ptr->edges_end(); ++eit) {
				if (eit->data() == BLUE) {
					arr_ptr->remove_edge(eit);
					redo = true;
					break;
				}
			}
			if (!redo) {
				for (Arrangement_2::Vertex_iterator vit = arr_ptr->vertices_begin(); vit != arr_ptr->vertices_end(); ++vit) {
					if (vit->data().c == BLUE && vit->degree() == 0) {
						arr_ptr->remove_isolated_vertex(vit);
						redo = true;
						break;
					}
				}
			}
		}
	}

	void simplify_voronoi_diagram(Arrangement_2* arr_ptr, std::map<Point, double, ptcmp>* distance)
	{
		bool redo = true;
		while (redo) {
			redo = false;
			for (Arrangement_2::Edge_iterator eit = arr_ptr->edges_begin(); eit != arr_ptr->edges_end(); ++eit) {
				if (eit->data() == BLUE) {
					if (eit->source()->data().c == BLACK || eit->target()->data().c == BLACK ||
						eit->source()->data().c == NONE || eit->target()->data().c == NONE)
					{
						arr_ptr->remove_edge(eit, false, true);
						redo = true;
						break;
					}
				}
			}
		}

		for (Arrangement_2::Vertex_iterator vit = arr_ptr->vertices_begin(); vit != arr_ptr->vertices_end(); ++vit) {
			if (vit->data().c == BLUE) {
				Point p(vit->point().x(), vit->point().y());
				double dist_node = distance->find(p)->second;
				vit->data().radius = dist_node;
			}
		}

		redo = true;
		std::set<Arrangement_2::Vertex_handle, vhradiuscmp> vertices;
		while (redo) {
			redo = false;
			for (Arrangement_2::Vertex_iterator vit = arr_ptr->vertices_begin(); vit != arr_ptr->vertices_end(); ++vit)
			{
				vertices.insert(vit);
			}
			for (std::set<Arrangement_2::Vertex_handle, vhradiuscmp>::iterator vh = vertices.begin(); vh != vertices.end(); vh++)
			{
				if ((*vh)->data().c == BLUE) {

// 					Point v0((*vh)->point());
// 					if (v0.x() <= 0 || v0.x() >= MapData::mapWidthWalkRes - 1
// 						|| v0.y() <= 0 || v0.y() >= MapData::mapHeightWalkRes - 1)
// 					{
// 						if ((*vh)->degree() == 0) {
// 							arr_ptr->remove_isolated_vertex(*vh);
// 						} else {
// 							Arrangement_2::Halfedge_handle h((*vh)->incident_halfedges());
// 							arr_ptr->remove_edge(h, false, true);
// 							redo = true;
// 							break;
// 						}
// 					}


					double dist_node = (*vh)->data().radius;
					if ((*vh)->degree() == 0) {
						if (dist_node < 10) {
							arr_ptr->remove_isolated_vertex(*vh);
						}
					} else if ((*vh)->degree() == 1) {
						Arrangement_2::Halfedge_handle h((*vh)->incident_halfedges());
						double dist_parent;
						if (h->source() == *vh) {
							dist_parent = h->target()->data().radius;
						} else {
							dist_parent = h->source()->data().radius;
						}
						if (dist_node <= dist_parent || dist_node < 10) {
							arr_ptr->remove_edge(h, false, true);
							redo = true;
							break;
						}
					}
				}
			}
			vertices.clear();
		}
	}

	void identify_region_nodes(Arrangement_2* arr_ptr, Graph* g_ptr)
	{
		//log("Identifying Region Nodes");
		for (Arrangement_2::Vertex_iterator vit = arr_ptr->vertices_begin(); vit != arr_ptr->vertices_end(); ++vit)
		{
			if (vit->data().c == BLUE) {
				if (vit->degree() != 2) {
					g_ptr->add_region(Point(vit->point().x(), vit->point().y()), vit->data().radius, vit);
					vit->data().is_region = true;
				} else {
					Arrangement_2::Vertex_handle start_node = vit;
					double start_radius = vit->data().radius;
					if (start_radius > 4 * 4) {
						bool region = true;

						Arrangement_2::Halfedge_around_vertex_circulator e = vit->incident_halfedges();
						Arrangement_2::Vertex_handle node = vit;
						for (int i = 0; i < 2; i++) {
							double distance_travelled = 0;
							Arrangement_2::Halfedge_around_vertex_circulator mye = e;
							while (distance_travelled < start_radius && region) {
								if (mye->source() == node) {
									node = mye->target();
								} else {
									node = mye->source();
								}
								distance_travelled += get_distance(mye->target()->point(), mye->source()->point());
								if (node->data().radius >= start_radius) {
									region = false;
								}
								Arrangement_2::Halfedge_around_vertex_circulator newe = node->incident_halfedges();
								if (Arrangement_2::Halfedge_handle(newe) == Arrangement_2::Halfedge_handle(mye->twin())) {
									newe++;
								}
								mye = newe;
							}
							if (!region) break;
							e++;
						}
						if (region) {
							g_ptr->add_region(Point(vit->point().x(), vit->point().y()), vit->data().radius, vit);
							vit->data().is_region = true;
						}
					}
				}
			}
		}
	}

	void identify_chokepoint_nodes(Graph* g_ptr, std::map<Point, double, ptcmp>* distance, std::map<Point, std::set< Point >, ptcmp >* nearest)
	{
		//log("Identifying Chokepoint Nodes");
		std::set<Node*> chokepoints_to_merge;
		for (std::set<Node*>::iterator r = g_ptr->regions_begin(); r != g_ptr->regions_end(); r++) {
			//log("On next region");
			if ((*r)->handle->is_isolated()) continue;
			bool first_outer = true;
			for (Arrangement_2::Halfedge_around_vertex_circulator e = (*r)->handle->incident_halfedges();
				first_outer || e != (*r)->handle->incident_halfedges(); e++)
			{
				//log("On next chokepoint of current region");
				first_outer = false;
				Arrangement_2::Vertex_handle node = (*r)->handle;
				Arrangement_2::Vertex_handle chokepoint_node = (*r)->handle;
				bool first = true;
				double min_radius;
				Arrangement_2::Halfedge_around_vertex_circulator mye = e;
				Point pt;
				Point min_pt;
				double distance_before = 0;
				double distance_after = 0;
				double distance_travelled = 0;

				int count = 0;
				while (first || node->data().is_region == false) {
					count++;
					if (count > 1000) {
						LOG("Error: Could not find other end of chokepoint path.");
						break;
					}
					//log("On next vertex of current chokepoint path");
					pt = Point(node->point().x(), node->point().y());
					double dist = node->data().radius;
					if (first || dist < min_radius || (dist == min_radius && pt < min_pt)) {
						first = false;
						min_radius = dist;
						min_pt = pt;
						chokepoint_node = node;
						distance_after = 0;
					}

					if (mye->source() == node) {
						node = mye->target();
					} else {
						node = mye->source();
					}
					distance_travelled += get_distance(mye->target()->point(), mye->source()->point());
					distance_after += get_distance(mye->target()->point(), mye->source()->point());
					Arrangement_2::Halfedge_around_vertex_circulator newe = node->incident_halfedges();
					if (Arrangement_2::Halfedge_handle(newe) == Arrangement_2::Halfedge_handle(mye->twin())) {
						newe++;
					}
					mye = newe;
				}
				if (count > 1000) continue;
				distance_before = distance_travelled - distance_after;
				bool is_last = false;
				pt = Point(node->point().x(), node->point().y());
				double dist = node->data().radius;
				if (first || dist < min_radius || (dist == min_radius && pt < min_pt)) {
					first = false;
					min_radius = dist;
					chokepoint_node = node;
					is_last = true;
				}
				Node* other_region = g_ptr->get_region(Point(node->point().x(), node->point().y()));
				Node* new_chokepoint = NULL;
				if (*r != other_region) {
					new_chokepoint = g_ptr->add_chokepoint(Point(chokepoint_node->point().x(), chokepoint_node->point().y()), chokepoint_node->data().radius, chokepoint_node, *r, other_region);
				} else {
					continue;
				}
				bool adding_this = true;
				if (is_last) {
					chokepoints_to_merge.insert(new_chokepoint);
					adding_this = false;
				}
				if (chokepoint_node != (*r)->handle && adding_this) {
					// assign side1 and side2
					NumberType x0 = chokepoint_node->point().x();
					NumberType y0 = chokepoint_node->point().y();
					std::set<Point> npoints = (*nearest)[Point(x0, y0)];
					if (npoints.size() == 2) {
						std::set<Point>::iterator p = npoints.begin();
						new_chokepoint->side1 = *p;
						p++;
						new_chokepoint->side2 = *p;
					} else {
						double min_pos_angle = PI;
						Point min_pos_point = *npoints.begin();
						double max_neg_angle = -PI;
						Point max_neg_point = *npoints.begin();
						double max_dist = -1;
						for (std::set<Point>::iterator p = npoints.begin(); p != npoints.end(); p++)
							for (std::set<Point>::iterator p2 = npoints.begin(); p2 != npoints.end(); p2++)
							{
							double dist = sqrt(to_double((p->x() - p2->x())*(p->x() - p2->x()) + (p->y() - p2->y())*(p->y() - p2->y())));
							if (dist > max_dist)
							{
								max_dist = dist;
								min_pos_point = *p;
								max_neg_point = *p2;
							}
							}
						new_chokepoint->side1 = min_pos_point;
						new_chokepoint->side2 = max_neg_point;
					}
				}
			}
		}
		//log("Merging regions");
		for (std::set<Node*>::iterator i = chokepoints_to_merge.begin(); i != chokepoints_to_merge.end(); i++)
		{
			g_ptr->merge_chokepoint(*i);
		}
		//log("Done Identifying Chokepoint Nodes");
	}

	double calculate_merge_value(Node* c)
	{
		Node* smaller = c->a_neighbor();
		Node* larger = c->other_neighbor(smaller);
		if (larger->radius < smaller->radius)
		{
			Node* temp = larger;
			larger = smaller;
			smaller = temp;
		}
		double lt = 0.85;
		double st = 0.9;
		double diff = std::max(c->radius - larger->radius*lt, c->radius - smaller->radius*st);
		for (Node* region = smaller;; region = larger)
		{
			if (region->neighbors.size() == 2)
			{
				Node* otherc = region->other_neighbor(c);
				double d1x = to_double(c->point.x() - region->point.x());
				double d1y = to_double(c->point.y() - region->point.y());
				double d1 = sqrt(d1x*d1x + d1y*d1y);
				d1x /= d1;
				d1y /= d1;
				double d2x = to_double(otherc->point.x() - region->point.x());
				double d2y = to_double(otherc->point.y() - region->point.y());
				double d2 = sqrt(d2x*d2x + d2y*d2y);
				d2x /= d2;
				d2y /= d2;
				double dx = 0.5*(d1x - d2x);
				double dy = 0.5*(d1y - d2y);
				double dist = sqrt(dx*dx + dy*dy);
				if (c->radius > otherc->radius || (c->radius == otherc->radius && c->point > otherc->point))
				{
					double value = c->radius*dist - region->radius*0.7;
					diff = std::max(diff, value);
				}
			}
			if (region == larger)
				break;
		}
		return diff;
	}

	void merge_adjacent_regions(Graph* g_ptr)
	{
		std::set<Node*> chokepoints_to_merge;
		bool finished;
		finished = false;
		while (!finished)
		{
			finished = true;
			Node* chokepoint_to_merge = NULL;
			Heap<Node*, double> h(false);
			for (std::set<Node*>::iterator c = g_ptr->chokepoints_begin(); c != g_ptr->chokepoints_end(); c++) {
				BWTA::Node* node = *c;
				double cost = node->radius;
				if (calculate_merge_value(node) > 0)  h.set(node, cost);
			}
			finished = (h.size() == 0);
			while (h.size() > 0)
			{
				while (h.size() > 0 && !g_ptr->has_chokepoint(h.top().first))
					h.pop();
				if (h.size() == 0)
					break;
				chokepoint_to_merge = h.top().first;
				h.pop();
				Node* smaller = chokepoint_to_merge->a_neighbor();
				Node* larger = chokepoint_to_merge->other_neighbor(smaller);
				if (larger->radius < smaller->radius)
				{
					Node* temp = larger;
					larger = smaller;
					smaller = temp;
				}
				double smaller_radius = smaller->radius;
				smaller->radius = larger->radius;
				std::set<Node*> affectedChokepoints;
				for (std::set<Node*>::iterator c = smaller->neighbors.begin(); c != smaller->neighbors.end(); c++)
				{
					affectedChokepoints.insert(*c);
					Node* otherr = (*c)->other_neighbor(smaller);
					for (std::set<Node*>::iterator c2 = otherr->neighbors.begin(); c2 != otherr->neighbors.end(); c2++)
					{
						affectedChokepoints.insert(*c2);
					}
				}
				for (std::set<Node*>::iterator c = larger->neighbors.begin(); c != larger->neighbors.end(); c++)
				{
					affectedChokepoints.insert(*c);
					Node* otherr = (*c)->other_neighbor(larger);
					for (std::set<Node*>::iterator c2 = otherr->neighbors.begin(); c2 != otherr->neighbors.end(); c2++)
					{
						affectedChokepoints.insert(*c2);
					}
				}
				for (std::set<Node*>::iterator c = affectedChokepoints.begin(); c != affectedChokepoints.end(); c++) {
					BWTA::Node* node = *c;
					double cost = node->radius;
					if (calculate_merge_value(node) > 0)  h.set(node, cost);
					else if (h.contains(node)) h.erase(node);
				}
				smaller->radius = smaller_radius;
				g_ptr->merge_chokepoint(chokepoint_to_merge);
			}
		}
		bool redo = true;
		while (redo)
		{
			redo = false;
			for (std::set<Node*>::iterator r = g_ptr->regions_begin(); r != g_ptr->regions_end(); r++)
			{
				if ((*r)->radius < 8)
				{
					g_ptr->remove_region(*r);
					redo = true;
					break;
				}
			}
		}
	}

	void wall_off_chokepoints(Graph* g_ptr, Arrangement_2* arr_ptr)
	{
		std::list< Segment_2 > new_segments;
		for (std::set<Node*>::iterator c = g_ptr->chokepoints_begin(); c != g_ptr->chokepoints_end(); c++)
		{
			Node* chokepoint_node = *c;
			NumberType x0 = chokepoint_node->point.x();
			NumberType y0 = chokepoint_node->point.y();
			CGAL::insert_point(*arr_ptr, Point_2(x0, y0));
			NumberType x1 = chokepoint_node->side1.x();
			NumberType y1 = chokepoint_node->side1.y();
			NumberType x2 = chokepoint_node->side2.x();
			NumberType y2 = chokepoint_node->side2.y();

			double length = sqrt(to_double((x1 - x0)*(x1 - x0) + (y1 - y0)*(y1 - y0)));
			x1 += (x1 - x0) / length;
			y1 += (y1 - y0) / length;
			new_segments.push_back(Segment_2(Point_2(x0, y0), Point_2(x1, y1)));

			length = sqrt(to_double((x2 - x0)*(x2 - x0) + (y2 - y0)*(y2 - y0)));
			x2 += (x2 - x0) / length;
			y2 += (y2 - y0) / length;
			new_segments.push_back(Segment_2(Point_2(x0, y0), Point_2(x2, y2)));
		}
		for (Arrangement_2::Vertex_iterator vit = arr_ptr->vertices_begin(); vit != arr_ptr->vertices_end(); ++vit)
		{
			if (vit->data().c != BLACK)
			{
				vit->data().c = RED;
			}
		}
		for (std::list<Segment_2>::iterator s = new_segments.begin(); s != new_segments.end(); s++)
		{
			CGAL::insert(*arr_ptr, *s);
		}
		for (Arrangement_2::Edge_iterator eit = arr_ptr->edges_begin(); eit != arr_ptr->edges_end(); ++eit)
		{
			if ((eit->data() != BLACK && eit->data() != BLUE) || eit->source()->data().c == RED || eit->target()->data().c == RED)
			{
				eit->data() = RED;
				eit->twin()->data() = RED;
			}
		}
		bool redo = true;
		while (redo)
		{
			redo = false;
			for (Arrangement_2::Edge_iterator eit = arr_ptr->edges_begin(); eit != arr_ptr->edges_end(); ++eit)
			{
				if (eit->data() == RED)
				{
					if (eit->source()->degree() == 1 || eit->target()->degree() == 1 || (eit->source()->data().c != RED && eit->target()->data().c != RED))
					{
						arr_ptr->remove_edge(eit);
						redo = true;
						break;
					}
				}
			}
		}
	}
}