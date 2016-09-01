#include "RegionGenerator.h"

#include "Painter.h"
#include <boost/geometry/index/rtree.hpp>

namespace BWTA
{
	static const std::size_t VISITED_COLOR = 1;
	static const int SKIP_NEAR_BORDER = 3;

	void addVerticalBorder(std::vector<VoronoiSegment>& segments, std::vector<BoostSegmentI>& rtreeSegments, size_t& idPoint, 
		const std::set<int>& border, int x, int maxY)
	{
// 		for (const auto& val : border) {
// 			LOG(val);
// 		}

		auto it = border.begin();
		if (*it == 0) ++it;
		else {
			VoronoiPoint point1(x, 0); 
			VoronoiPoint point2(x, maxY);
			if (it != border.end()) {
				point2.y(*it); ++it;
			}
// 			LOG("(" << point1.x() << "," << point1.y() << ") - (" << point2.x() << "," << point2.y() << ")");
			segments.push_back(VoronoiSegment(point1, point2));
			rtreeSegments.push_back(std::make_pair(BoostSegment(BoostPoint(point1.x(), point1.y()), BoostPoint(point2.x(), point2.y())), idPoint++));

		}
		for (it; it != border.end();) {
			if (*it == maxY) break;
			VoronoiPoint point1(x, *it); ++it;
			VoronoiPoint point2(x, maxY);
			if (it != border.end()) {
				point2.y(*it); ++it;
			}
// 			LOG("(" << point1.x() << "," << point1.y() << ") - (" << point2.x() << "," << point2.y() << ")");
			segments.push_back(VoronoiSegment(point1, point2));
			rtreeSegments.push_back(std::make_pair(BoostSegment(BoostPoint(point1.x(), point1.y()), BoostPoint(point2.x(), point2.y())), idPoint++));
		}
	}

	void addHorizontalBorder(std::vector<VoronoiSegment>& segments, std::vector<BoostSegmentI>& rtreeSegments, size_t& idPoint, 
		const std::set<int>& border, int y, int maxX)
	{
// 		for (const auto& val : border) {
// 			LOG(val);
// 		}

		auto it = border.begin();
		if (*it == 0) ++it;
		else {
			VoronoiPoint point1(0, y);
			VoronoiPoint point2(maxX, y);
			if (it != border.end()) {
				point2.x(*it); ++it;
			}
// 			LOG("(" << point1.x() << "," << point1.y() << ") - (" << point2.x() << "," << point2.y() << ")");
			segments.push_back(VoronoiSegment(point1, point2));
			rtreeSegments.push_back(std::make_pair(BoostSegment(BoostPoint(point1.x(), point1.y()), BoostPoint(point2.x(), point2.y())), idPoint++));
		}
		for (it; it != border.end();) {
			if (*it == maxX) break;
			VoronoiPoint point1(*it, y); ++it;
			VoronoiPoint point2(maxX, y);
			if (it != border.end()) {
				point2.x(*it); ++it;
			}
// 			LOG("(" << point1.x() << "," << point1.y() << ") - (" << point2.x() << "," << point2.y() << ")");
			segments.push_back(VoronoiSegment(point1, point2));
			rtreeSegments.push_back(std::make_pair(BoostSegment(BoostPoint(point1.x(), point1.y()), BoostPoint(point2.x(), point2.y())), idPoint++));
		}
	}


	void generateVoronoid(const std::vector<Polygon>& polygons, const RectangleArray<int>& labelMap, 
		RegionGraph& graph, bgi::rtree<BoostSegmentI, bgi::quadratic<16> >& rtree)
	{
		std::vector<VoronoiSegment> segments;
		std::vector<BoostSegmentI> rtreeSegments;
		size_t idPoint = 0;

		// containers for border points (we need to fill the borders of the map with segments)
		std::set<int> rightBorder, leftBorder, topBorder; // bottomBorder not needed since it's always unwalkable
		int maxX = MapData::walkability.getWidth() - 1;
		int maxY = MapData::walkability.getHeight() - 1;

		// Add the line segments of each polygon to VoronoiSegment list and points to R-tree points list
// 		auto polygon = polygons.at(1);
		for (const auto& polygon : polygons) {
			// Add the vertices of the polygon
			size_t lastPoint = polygon.size() - 1;
			for (size_t i = 0; i < lastPoint; i++) {
				// save border points
				if (polygon[i].x == 0) leftBorder.insert(polygon[i].y);
				else if (polygon[i].x == maxX) rightBorder.insert(polygon[i].y);
				if (polygon[i].y == 0) topBorder.insert(polygon[i].x);

				int j = i + 1; // Notice that polygons are closed, i.e. the last vertex is equal to the first

// 				LOG("Segment (" << polygon[j].x << "," << polygon[j].y << ") to (" << polygon[i].x << "," << polygon[i].y << ")");
				segments.push_back(VoronoiSegment(
					VoronoiPoint(polygon[j].x, polygon[j].y),
					VoronoiPoint(polygon[i].x, polygon[i].y)));

				rtreeSegments.push_back(std::make_pair(BoostSegment(BoostPoint(polygon[j].x, polygon[j].y), BoostPoint(polygon[i].x, polygon[i].y)), idPoint++));
			}
			// TODO Add the vertices of each hole in the polygon
// 			for (const auto& hole : polygon.holes) {
// 				for (size_t i = 0; i < hole.size(); i++) {
// 					int j = (i + 1) % hole.size();
// 					VoronoiPoint a(hole[i].x, hole[i].y);
// 					VoronoiPoint b(hole[j].x, hole[j].y);
// 					segments.push_back(VoronoiSegment(b, a));
// 				}
// 			}
		}

		// add remain border segments
		LOG(" - Generating borders");
		addVerticalBorder(segments, rtreeSegments, idPoint, leftBorder, 0, maxY);
		addVerticalBorder(segments, rtreeSegments, idPoint, rightBorder, maxX, maxY);
		addHorizontalBorder(segments, rtreeSegments, idPoint, topBorder, 0, maxX);

		BoostVoronoi voronoi;
		boost::polygon::construct_voronoi(segments.begin(), segments.end(), &voronoi);
		LOG(" - Voronoi constructed");

		const int maxXborder = maxX - SKIP_NEAR_BORDER;
		const int maxYborder = maxY - SKIP_NEAR_BORDER;

		// traverse the Voronoi diagram and generate graph nodes and edges
		for (const auto& edge : voronoi.edges()) {
			if (!edge.is_primary() || !edge.is_finite() || edge.color() == VISITED_COLOR) continue;

			// mark half-edge twin as visited
			edge.twin()->color(VISITED_COLOR);

			const BoostVoronoi::vertex_type* v0 = edge.vertex0();
			const BoostVoronoi::vertex_type* v1 = edge.vertex1();
			BWAPI::WalkPosition p0((int)v0->x(), (int)v0->y());
			BWAPI::WalkPosition p1((int)v1->x(), (int)v1->y());

			// skip edge if ...
			// ... is outside map or near border
// 			if (p0.x < SKIP_NEAR_BORDER || p0.x > maxXborder || p0.y < SKIP_NEAR_BORDER || p0.y >maxYborder
// 				|| p1.x < SKIP_NEAR_BORDER || p1.x >maxXborder || p1.y < SKIP_NEAR_BORDER || p1.y >maxYborder) continue;

			// ... any of its endpoints is inside an obstacle
			if (labelMap[p0.x][p0.y] > 0 || labelMap[p1.x][p1.y] > 0) continue;

			nodeID v0ID = graph.addNode(v0, p0);
			nodeID v1ID = graph.addNode(v1, p1);
			graph.addEdge(v0ID, v1ID);
		}

		// create an R-tree to query nearest points/obstacle
		bgi::rtree<BoostSegmentI, bgi::quadratic<16> > rtreeConstruct(rtreeSegments);
		rtree = rtreeConstruct;
		graph.minDistToObstacle.resize(graph.nodes.size());
		for (size_t i = 0; i < graph.nodes.size(); ++i) {
			std::vector<BoostSegmentI> returnedValues;
			BoostPoint pt(graph.nodes[i].x, graph.nodes[i].y);
			rtree.query(bgi::nearest(pt, 1), std::back_inserter(returnedValues));
			// TODO can be speed up using comaparable_distance
			graph.minDistToObstacle[i] = bg::distance(returnedValues.front().first, pt);
		}
	}

	nodeID RegionGraph::addNode(const BoostVoronoi::vertex_type* vertex, const BWAPI::WalkPosition& pos)
	{
		nodeID vID;
		// add new point if not present in the graph
		auto vIt = voronoiVertexToNode.find(vertex);
		if (vIt == voronoiVertexToNode.end()) {
			voronoiVertexToNode[vertex] = vID = nodes.size();
			nodes.push_back(pos);
			nodeType.push_back(NONE);
		} else {
			vID = vIt->second;
		}

		return vID;
	}

	nodeID RegionGraph::addNode(const BWAPI::WalkPosition& pos, const double& minDist)
	{
		nodeID vID;
		// add new point if not present in the graph
		auto it = std::find(nodes.begin(), nodes.end(), pos);
		if (it == nodes.end()) {
			vID = nodes.size();
			nodes.push_back(pos);
			nodeType.push_back(NONE);
			minDistToObstacle.push_back(minDist);
		} else {
			vID = it - nodes.begin();
		}

		return vID;
	}

	void RegionGraph::addEdge(const nodeID& v0, const nodeID& v1)
	{
		// extend adjacency list if needed
		size_t maxId = std::max(v0, v1);
		if (adjacencyList.size() < maxId + 1) adjacencyList.resize(maxId + 1);

		adjacencyList[v0].insert(v1);
		adjacencyList[v1].insert(v0);
	}

	void pruneGraph(RegionGraph& graph)
	{
		// get the list of all leafs (nodes with only one element in the adjacent list)
		std::queue<nodeID> nodeToPrune;
		for (size_t id = 0; id < graph.adjacencyList.size(); ++id) {
			if (graph.adjacencyList.at(id).size() == 1) {
				nodeToPrune.push(id);
			}
		}

		// using leafs as starting points, prune the RegionGraph
		while (!nodeToPrune.empty()) {
			// pop first element
			nodeID v0 = nodeToPrune.front();
			nodeToPrune.pop();

			if (graph.adjacencyList.at(v0).empty()) continue; // isolated point

			nodeID v1 = *graph.adjacencyList.at(v0).begin();
			// remove node if it's too close to an obstacle, or parent is farther to an obstacle
			if (graph.minDistToObstacle.at(v0) < 6.0 
				|| graph.minDistToObstacle.at(v0) - 0.9 <= graph.minDistToObstacle.at(v1)) 
			{
				graph.adjacencyList.at(v0).clear();
				graph.adjacencyList.at(v1).erase(v0);
				if (graph.adjacencyList.at(v1).size() == 1) {
					nodeToPrune.push(v1);
				}
			}
		}
	}

	void markRegionNodes(RegionGraph& graph, const std::vector<Polygon>& polygons)
	{
		const double MIN_RADII_DIFF = 4.5;
		struct parentNode_t {
			nodeID id;
			bool isMaximal;
			parentNode_t() : id(0), isMaximal(false) {}
		};
		// container to mark visited nodes, and parent list
		std::vector<bool> visited;
		visited.resize(graph.nodes.size());
		std::vector<parentNode_t> parentNodes;
		parentNodes.resize(graph.nodes.size());

		std::stack<nodeID> nodeToVisit;
		parentNode_t parentNode;
		// find a leaf node to start
		for (size_t id = 0; id < graph.adjacencyList.size(); ++id) {
			if (graph.adjacencyList.at(id).size() == 1) {
				// mark node as Region
				visited.at(id) = true;
				graph.regionNodes.insert(id);
				graph.nodeType.at(id) = RegionGraph::REGION;
				parentNode.id = id; parentNode.isMaximal = true;
				parentNodes.at(id) = parentNode;
// 				LOG("REGION " << graph.nodes.at(id) << " radius " << graph.minDistToObstacle.at(id));
				// add children to explore
				for (const auto& v1 : graph.adjacencyList.at(id)) {
					nodeToVisit.push(v1);
					visited.at(v1) = true;
					parentNodes.at(v1) = parentNode;
				}
				break;
			}
		}

#ifdef DEBUG_DRAW
		Painter painter;
#endif
		int nodesDetected = 0;
		int oldNodesDetected = 0;

		while (!nodeToVisit.empty()) {
			// pop first element
			nodeID v0 = nodeToVisit.top();
			nodeToVisit.pop();
			// get parent
			parentNode = parentNodes.at(v0);

			if (graph.adjacencyList.at(v0).size() != 2) {
// 				LOG("REGION !=2 " << graph.nodes.at(v0) << " radius: " << graph.minDistToObstacle.at(v0) << " parent: " << graph.minDistToObstacle.at(parentNode.id));
				// if parent chokepoint too close, delete parent
				if (graph.nodeType.at(parentNode.id) == RegionGraph::CHOKEPOINT
					&& graph.nodes.at(v0).getApproxDistance(graph.nodes.at(parentNode.id)) < 7) {
					graph.chokeNodes.erase(parentNode.id);
					graph.nodeType.at(parentNode.id) = RegionGraph::NONE;
				}
				nodesDetected++;
				graph.regionNodes.insert(v0);
				graph.nodeType.at(v0) = RegionGraph::REGION;
				parentNode.id = v0; parentNode.isMaximal = true;
			} else {
				// look if the node is a local minimal (chokepoint node)
				bool localMinimal = true;
				for (const auto& v1 : graph.adjacencyList.at(v0)) {
					if (graph.minDistToObstacle.at(v0) > graph.minDistToObstacle.at(v1)) {
						localMinimal = false;
						break;
					}
				}
				if (localMinimal) {
					if (!parentNode.isMaximal) {
						// we have two consecutive minimals
						// if parent is CHOKE
// 						if (graph.nodeType.at(parentNode.id) == RegionGraph::CHOKEPOINT) {
// 							// if the new chokepoint is smaller, keep the new as CHOKEPOINT
// 							if (graph.minDistToObstacle.at(v0) < graph.minDistToObstacle.at(parentNode.id)) {
// 								graph.chokeNodes.erase(parentNode.id);
// 								graph.nodeType.at(parentNode.id) = RegionGraph::NONE;
// 
// 								nodesDetected++;
// 								graph.chokeNodes.insert(v0);
// 								graph.nodeType.at(v0) = RegionGraph::CHOKEPOINT;
// 								parentNode.id = v0; parentNode.isMaximal = false;
// 							} else { // otherwise, parent becomes GATEA and this GATEB
// 								graph.chokeNodes.erase(parentNode.id);
// 								graph.gateNodesA.insert(parentNode.id);
// 								graph.nodeType.at(parentNode.id) = RegionGraph::CHOKEGATEA;
// 
// 								nodesDetected++;
// 								graph.gateNodesB.insert(v0);
// 								graph.nodeType.at(v0) = RegionGraph::CHOKEGATEB;
// 								parentNode.id = v0; parentNode.isMaximal = false;
// 							}
// 						}
// 
// 						// else if parent is GATEB, remove parent and keep this as GATEB
// 						else if (graph.nodeType.at(parentNode.id) == RegionGraph::CHOKEGATEB) {
// 							// TODO if between GATEA and GATEB there is too much difference ...
// 							// ... we have a cone, keep only the smallest
// 							graph.gateNodesB.erase(parentNode.id);
// 							graph.nodeType.at(v0) = RegionGraph::NONE;
// 
// 							nodesDetected++;
// 							graph.gateNodesB.insert(v0);
// 							graph.nodeType.at(v0) = RegionGraph::CHOKEGATEB;
// 							parentNode.id = v0; parentNode.isMaximal = false;
// 						}

						// keep the min
						if (graph.minDistToObstacle.at(v0) < graph.minDistToObstacle.at(parentNode.id)) {
// 							LOG("CHOKE (better consecutive) " << graph.nodes.at(v0) << " radius: " << graph.minDistToObstacle.at(v0) << " parent: " << graph.minDistToObstacle.at(parentNode.id));
							graph.chokeNodes.erase(parentNode.id);
							graph.nodeType.at(parentNode.id) = RegionGraph::NONE;
								
							nodesDetected++;
							graph.chokeNodes.insert(v0);
							graph.nodeType.at(v0) = RegionGraph::CHOKEPOINT;
							parentNode.id = v0; parentNode.isMaximal = false;
						}
					} else { // parent is maximal
						if (std::abs(graph.minDistToObstacle.at(v0) - graph.minDistToObstacle.at(parentNode.id)) > MIN_RADII_DIFF
							&& graph.nodes.at(v0).getApproxDistance(graph.nodes.at(parentNode.id)) > 10) {
// 							LOG("CHOKEPOINT " << graph.nodes.at(v0) << " radius: " << graph.minDistToObstacle.at(v0) << " parent: " << graph.minDistToObstacle.at(parentNode.id)
// 								<< " diff: " << std::abs(graph.minDistToObstacle.at(v0) - graph.minDistToObstacle.at(parentNode.id)));
							nodesDetected++;
							graph.chokeNodes.insert(v0);
							graph.nodeType.at(v0) = RegionGraph::CHOKEPOINT;
							parentNode.id = v0; parentNode.isMaximal = false;
						} else {
							// the radius difference is too small, we mark the node as a "choke-gate"
// 							LOG("CHOKEGATE " << graph.nodes.at(v0) << " radius: " << graph.minDistToObstacle.at(v0) << " parent: " << graph.minDistToObstacle.at(parentNode.id)
// 								<< " diff: " << std::abs(graph.minDistToObstacle.at(v0) - graph.minDistToObstacle.at(parentNode.id)));
// 							nodesDetected++;
// 							graph.gateNodes.insert(v0);
// 							graph.nodeType.at(v0) = RegionGraph::CHOKEGATE;
// 							parentNode.id = v0; parentNode.isMaximal = false;
						}
					}
				} else {
					// look if the node is a local maximal (region node)
					bool localMaximal = true;
					for (const auto& v1 : graph.adjacencyList.at(v0)) {
						if (graph.minDistToObstacle.at(v0) < graph.minDistToObstacle.at(v1)) {
							localMaximal = false;
							break;
						}
					}
					if (localMaximal) {
						if (parentNode.isMaximal) {
							// we have two consecutive maximals, keep the max
							if (graph.minDistToObstacle.at(v0) > graph.minDistToObstacle.at(parentNode.id)) {
// 								LOG("REGION (better consecutive) " << graph.nodes.at(v0) << " radius: " << graph.minDistToObstacle.at(v0) << " parent: " << graph.minDistToObstacle.at(parentNode.id));
								// only delete parent if size == 2
								if (graph.adjacencyList.at(parentNode.id).size() == 2) {
									graph.regionNodes.erase(parentNode.id);
									graph.nodeType.at(parentNode.id) = RegionGraph::NONE;
								}
								nodesDetected++;
								graph.regionNodes.insert(v0);
								graph.nodeType.at(v0) = RegionGraph::REGION;
								parentNode.id = v0; parentNode.isMaximal = true;
							}
						} else { // parent is minimal
							if (std::abs(graph.minDistToObstacle.at(v0) - graph.minDistToObstacle.at(parentNode.id)) > MIN_RADII_DIFF
								&& graph.nodes.at(v0).getApproxDistance(graph.nodes.at(parentNode.id)) > 10) {
// 								LOG("REGION " << graph.nodes.at(v0) << " radius: " << graph.minDistToObstacle.at(v0) << " parent: " << graph.minDistToObstacle.at(parentNode.id)
// 									<< " diff: " << std::abs(graph.minDistToObstacle.at(v0) - graph.minDistToObstacle.at(parentNode.id)));
								nodesDetected++;
								graph.regionNodes.insert(v0);
								graph.nodeType.at(v0) = RegionGraph::REGION;
								parentNode.id = v0; parentNode.isMaximal = true;
							} else {
								// the radius difference is too small, if parent is a "choke-gate" we add it
// 								if (graph.nodeType.at(parentNode.id) == RegionGraph::CHOKEGATEA) {
// 									LOG("REGION (parent chokegateA) " << graph.nodes.at(v0) << " radius: " << graph.minDistToObstacle.at(v0) << " parent: " << graph.minDistToObstacle.at(parentNode.id));
// 									nodesDetected++;
// 									graph.regionNodes.insert(v0);
// 									graph.nodeType.at(v0) = RegionGraph::REGION;
// 									parentNode.id = v0; parentNode.isMaximal = true;
// 								}
							}
						}
					}
				}
			}

			// keep exploring unvisited neighbors
			for (const auto& v1 : graph.adjacencyList.at(v0)) {
				if (!visited.at(v1)) {
					nodeToVisit.push(v1);
					visited.at(v1) = true;
					parentNodes.at(v1) = parentNode;
				}
			}

// #ifdef DEBUG_DRAW
// 			// iterative drawing of nodes detected
// 			if (oldNodesDetected != nodesDetected) {
// 				painter.drawPolygons(polygons);
// 				painter.drawGraph(graph);
// 				painter.drawNodes(graph, graph.regionNodes, Qt::blue);
// 				painter.drawNodes(graph, graph.chokeNodes, Qt::red);
// 				painter.drawNodes(graph, graph.gateNodesA, Qt::green);
// 				painter.drawNodes(graph, graph.gateNodesB, Qt::darkGreen);
// 				painter.render();
// 				oldNodesDetected = nodesDetected;
// 			}
// #endif
		}
	}

	std::string getString(std::set<nodeID> nodeSet)
	{
		std::stringstream stream;
		for (auto& id : nodeSet) {
			stream << id << ",";
		}
		return stream.str();
	}

	void simplifyRegionGraph(const RegionGraph& graph, RegionGraph& graphSimplified)
	{

#ifdef DEBUG_DRAW
		Painter painter;
#endif
		// containers to mark visited nodes, and parent list
		std::vector<bool> visited;
		visited.resize(graph.nodes.size());
		std::vector<nodeID> parentID;
		parentID.resize(graph.nodes.size());

		std::stack<nodeID> nodeToVisit;
		// start with one region node
		nodeID id = *graph.regionNodes.begin();
		visited.at(id) = true;
		nodeID newId = graphSimplified.addNode(graph.nodes.at(id), graph.minDistToObstacle.at(id));
		graphSimplified.regionNodes.insert(newId);
		graphSimplified.nodeType.at(newId) = RegionGraph::REGION;

		// add children to explore
		for (const auto& v1 : graph.adjacencyList.at(id)) {
			nodeToVisit.emplace(v1);
			visited.at(v1) = true;
			parentID.at(v1) = newId;
		}

		while (!nodeToVisit.empty()) {
			// pop first element
			nodeID nodeId = nodeToVisit.top();
			nodeToVisit.pop();
			nodeID parentId = parentID.at(nodeId);

			if (graph.nodeType.at(nodeId) == RegionGraph::CHOKEPOINT) {
				nodeID newId = graphSimplified.addNode(graph.nodes.at(nodeId), graph.minDistToObstacle.at(nodeId));
				if (newId != parentId) { // to avoid self inclusions
					graphSimplified.chokeNodes.insert(newId);
					graphSimplified.nodeType.at(newId) = RegionGraph::CHOKEPOINT;
					graphSimplified.addEdge(newId, parentId);
					parentId = newId;
				}
			} else if (graph.nodeType.at(nodeId) == RegionGraph::REGION) {
				nodeID newId = graphSimplified.addNode(graph.nodes.at(nodeId), graph.minDistToObstacle.at(nodeId));
				if (newId != parentId) { // to avoid self inclusions
					graphSimplified.regionNodes.insert(newId);
					graphSimplified.nodeType.at(newId) = RegionGraph::REGION;
					graphSimplified.addEdge(newId, parentId);
					parentId = newId;
				}
			}

// 			std::stringstream toPrint;
// 			for (const auto& v1 : graph.adjacencyList.at(nodeId)) toPrint << v1 << ",";
// 			LOG("OUT " << nodeId << " CHILDREN: " << toPrint.str());

			// keep exploring unvisited neighbors
			for (const auto& v1 : graph.adjacencyList.at(nodeId)) {
				if (!visited.at(v1) ){
					nodeToVisit.emplace(v1);
					parentID.at(v1) = parentId;
					visited.at(v1) = true;
// 					LOG("IN " << v1);
				} else if (parentID.at(v1) != parentID.at(nodeId)) {
					// if to paths with different parents meet together, add the edge between the parents
					nodeID n1 = parentID.at(v1);
					if (graph.nodeType.at(v1) != RegionGraph::NONE) { // get their own ID
						n1 = graphSimplified.addNode(graph.nodes.at(v1), graph.minDistToObstacle.at(v1));
					}
					nodeID n2 = parentID.at(nodeId);
					if (graph.nodeType.at(nodeId) != RegionGraph::NONE) { // get their own ID
						n2 = graphSimplified.addNode(graph.nodes.at(nodeId), graph.minDistToObstacle.at(nodeId));
					}
					if (n1 != n2) {
						graphSimplified.addEdge(n1, n2);
					}
				}
			}
		}

#ifdef DEBUG_DRAW
		painter.drawGraph(graphSimplified);
		painter.drawNodes(graphSimplified, graphSimplified.regionNodes, Qt::blue);
		painter.drawNodes(graphSimplified, graphSimplified.chokeNodes, Qt::red);
		painter.render("5-NodesSimplified");
#endif

		// Second loop to merge connected regions
		std::set<nodeID> mergeRegions(graphSimplified.regionNodes);

		while (!mergeRegions.empty()) {
// 			LOG("Graph size: " << mergeRegions.size());
			// pop first element
			nodeID v0 = *mergeRegions.begin();
			mergeRegions.erase(mergeRegions.begin());

			for (auto& it = graphSimplified.adjacencyList.at(v0).begin(); it != graphSimplified.adjacencyList.at(v0).end();) {
				nodeID v1 = *it;
				if (v0 == v1) { // This is a self-loop, remove it
					it = graphSimplified.adjacencyList.at(v0).erase(it);
					continue;
				}
				if (graphSimplified.nodeType.at(v0) == RegionGraph::REGION
					&& graphSimplified.nodeType.at(v1) == RegionGraph::REGION) 
				{
// 					LOG(" - Set " << v0 << ":" << getString(graphSimplified.adjacencyList.at(v0)));
// 					LOG(" - Set " << v1 << ":" << getString(graphSimplified.adjacencyList.at(v1)));
					if (graphSimplified.minDistToObstacle.at(v0) > graphSimplified.minDistToObstacle.at(v1)) {
						// keep v0
						graphSimplified.adjacencyList.at(v0).insert(graphSimplified.adjacencyList.at(v1).begin(), 
							graphSimplified.adjacencyList.at(v1).end());
						// for each adjacent, remove v1 add v0 (except v0)
						for (const auto& v2 : graphSimplified.adjacencyList.at(v1)) {
							graphSimplified.adjacencyList.at(v2).erase(v1);
							if (v2 == v0) graphSimplified.adjacencyList.at(v0).erase(v0);
							else graphSimplified.adjacencyList.at(v2).insert(v0);
						}
						graphSimplified.adjacencyList.at(v1).clear();
// 						LOG(" - Set " << v0 << " merged:" << getString(graphSimplified.adjacencyList.at(v0)));
						graphSimplified.regionNodes.erase(v1);
// 						LOG(v1 << " erased, " << v0 << " keeped");
						// restart iterator
						it = graphSimplified.adjacencyList.at(v0).begin();
// #ifdef DEBUG_DRAW
// 						painter.drawGraph(graphSimplified);
// 						painter.drawNodes(graphSimplified, graphSimplified.regionNodes, Qt::blue);
// 						painter.drawNodes(graphSimplified, graphSimplified.chokeNodes, Qt::red);
// 						painter.render();
// #endif
					} else {
						// keep v1
						graphSimplified.adjacencyList.at(v1).insert(graphSimplified.adjacencyList.at(v0).begin(),
							graphSimplified.adjacencyList.at(v0).end());
						// for each adjacent, remove v0 add v1 (except v1)
						for (const auto& v2 : graphSimplified.adjacencyList.at(v0)) {
							graphSimplified.adjacencyList.at(v2).erase(v0);
							if (v2 == v1) graphSimplified.adjacencyList.at(v1).erase(v1);
							else graphSimplified.adjacencyList.at(v2).insert(v1);
						}
						graphSimplified.adjacencyList.at(v0).clear();
// 						LOG(" - Set " << v1 << " merged:" << getString(graphSimplified.adjacencyList.at(v1)));
						graphSimplified.regionNodes.erase(v0);
// 						LOG(v0 << " erased, " << v1 << " keeped");
						// v1 should be re-analyzed
						mergeRegions.insert(v1);
// #ifdef DEBUG_DRAW
// 						painter.drawGraph(graphSimplified);
// 						painter.drawNodes(graphSimplified, graphSimplified.regionNodes, Qt::blue);
// 						painter.drawNodes(graphSimplified, graphSimplified.chokeNodes, Qt::red);
// 						painter.render();
// #endif
						break;
					}
				} else {
					++it;
				}
			}
		}
	}

	BWAPI::WalkPosition getProjectedPoint(BoostPoint p, BoostSegment s)
	{
		double dx = s.second.x() - s.first.x();
		double dy = s.second.y() - s.first.y();
		if (dx != 0 || dy != 0) {
			double t = ((p.x() - s.first.x()) * dx + (p.y() - s.first.y()) * dy) / (dx * dx + dy * dy);
			if (t > 1) {
				return BWAPI::WalkPosition((int)s.second.x(), (int)s.second.y());
			} else if (t > 0) {
				return BWAPI::WalkPosition(int(s.first.x() + dx * t), int(s.first.y() + dy * t));
			}
		}
		return BWAPI::WalkPosition((int)s.first.x(), (int)s.first.y());
	}

	inline BoostPoint getMidpoint(BoostPoint a, BoostPoint b)
	{
		boost::geometry::subtract_point(a, b);
		boost::geometry::divide_value(a, 2);
		boost::geometry::add_point(a, b);
		return a;
	}

	void getChokepointSides(const std::vector<Polygon>& polygons, const RegionGraph& graph, const bgi::rtree<BoostSegmentI, bgi::quadratic<16> >& rtree, std::map<nodeID, chokeSides_t>& chokepointSides)
	{
// #ifdef DEBUG_DRAW
// 		Painter painter;
// #endif

		for (const auto& id : graph.chokeNodes) {
			BoostPoint pt(graph.nodes[id].x, graph.nodes[id].y);
			std::vector<BoostSegment> nearestSegments;
			BoostSegment s1, s2;

			// get nearest value
			auto it = rtree.qbegin(bgi::nearest(pt, 100));
			s1 = (*it).first;
			nearestSegments.push_back((*it).first);
			++it;

			// iterate over nearest Values
			for (it; it != rtree.qend(); ++it) {

// #ifdef DEBUG_DRAW
// 				// iterative drawing of nodes detected
// 				painter.drawPolygons(polygons);
// 				// explored segments
// 				for (const auto& seg : nearestSegments) {
// 					painter.drawLine(seg, Qt::blue);
// 				}
// 				painter.drawLine(s1, Qt::green); // first segment
// 				painter.drawLine((*it).first, Qt::red); // current segment
// 				painter.render();
// #endif

				// if distance to all previous nearest segments is different than 0, we have a second segment candidate
				bool found = true;
				for (const auto& seg : nearestSegments) {
					double dist = boost::geometry::comparable_distance(seg, (*it).first);
					if (dist == 0) {
						found = false;
						break;
					}
				}
				if (found) {
					// get midpoint of the segment
// 					BoostPoint midpoint = getMidpoint(s1.first, (*it).first.first);
// 					LOG("Midpoint of (" << s1.first.x() << "," << s1.first.y() << ") and (" << (*it).first.first.x() << "," << (*it).first.first.y() << ") is (" << midpoint.x() << "," << midpoint.y() << ")");
// 					double distToSide = boost::geometry::comparable_distance(s1, midpoint);
// 					double distToMidpoint = boost::geometry::comparable_distance(pt, midpoint);
// 					LOG("DistChokeToMidpoint: " << distToMidpoint << " ditMidPointToSide: " << distToSide);
// 					if (distToMidpoint < distToSide) {
						s2 = (*it).first;
						break;
// 					}
				}
				nearestSegments.push_back((*it).first);
			}

			BWAPI::WalkPosition side1 = getProjectedPoint(pt, s1);
			BWAPI::WalkPosition side2 = getProjectedPoint(pt, s2);
			chokepointSides.emplace(id, chokeSides_t(side1, side2));
		}
	}

	void createRegionsFromGraph(const std::vector<BoostPolygon>& polygons, const RectangleArray<int>& labelMap,
		const RegionGraph& graph, const std::map<nodeID, chokeSides_t>& chokepointSides, 
		std::set<Region*>& regions, std::set<Chokepoint*>& chokepoints,
		std::vector<BoostPolygon>& polReg)
	{
		// create regions polygons
		// *********************************************

		// first we start with the "negative" geometry of polygon obstacles
		// Set vertices of the map
		BoostPoint topLeft(0, 0);
		BoostPoint bottomLeft(0, labelMap.getHeight() - 1);
		BoostPoint bottomRight(labelMap.getWidth() - 1, labelMap.getHeight() - 1);
		BoostPoint topRight(labelMap.getWidth() - 1, 0);
		std::vector<BoostPoint> points = { topLeft, topRight, bottomRight, bottomLeft, topLeft };
		BoostPolygon mapBorder;
		boost::geometry::assign_points(mapBorder, points);
		boost::geometry::correct(mapBorder);

		// subtract obstacles
		typedef boost::geometry::model::multi_polygon<BoostPolygon> BoostMultiPoly;
		BoostMultiPoly output, obstacles;
		obstacles.reserve(polygons.size());
		std::copy(polygons.begin(), polygons.end(), back_inserter(obstacles));
		boost::geometry::difference(mapBorder, obstacles, output);

		// divide regions
		typedef boost::geometry::model::linestring<BoostPoint> BoostLine;
		typedef boost::geometry::model::multi_linestring<BoostLine> BoostMultiLine;
		BoostMultiLine chokeLines;
		for (const auto& choke : chokepointSides) {
			BoostPoint a(choke.second.side1.x, choke.second.side1.y);
			BoostPoint b(choke.second.side2.x, choke.second.side2.y);
			std::vector<BoostPoint> line = { a, b };
			BoostLine chokeLine;
			boost::geometry::assign_points(chokeLine, line);
			chokeLines.push_back(chokeLine);
		}

		// Declare strategies
		const double buffer_distance = 0.5;
		boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(buffer_distance);
		boost::geometry::strategy::buffer::join_miter join_strategy;
		boost::geometry::strategy::buffer::end_round end_strategy;
		boost::geometry::strategy::buffer::point_square circle_strategy;
		boost::geometry::strategy::buffer::side_straight side_strategy;
		// Create the buffer of a linestring
		BoostMultiPoly cutPolygons;
		boost::geometry::buffer(chokeLines, cutPolygons,
			distance_strategy, side_strategy,
			join_strategy, end_strategy, circle_strategy);

		BoostMultiPoly regionsPoly;
		boost::geometry::difference(output, cutPolygons, regionsPoly);
		
// 		LOG("Output size: " << regionsPoly.size());
		polReg.reserve(regionsPoly.size());
		std::copy(regionsPoly.begin(), regionsPoly.end(), back_inserter(polReg));

		// Create regions from graph nodes
		std::map<nodeID, Region*> node2region;
		for (const auto& regionNodeId : graph.regionNodes) {
			Polygon poly;
			// find polygon
			BoostPoint regionPoint(graph.nodes[regionNodeId].x, graph.nodes[regionNodeId].y);
			for (const auto& regionPol : regionsPoly) {
				if (boost::geometry::within(regionPoint, regionPol)) {
					// Translate BoostPolygon to BWTA polygon (Polygon) (and WalkPosition to Position)
					for (const auto& polyPoint : regionPol.outer()) {
						poly.emplace_back((int)polyPoint.x() * 8, (int)polyPoint.y() * 8);
					}
					// Create the BWTA region
					Region* newRegion = new RegionImpl(poly);
					regions.insert(newRegion);
					node2region.emplace(regionNodeId, newRegion);
					break;
				}
			}
		}

		//LOG(" - Finding chokepoints and linking them to regions.");
		std::map<nodeID, Chokepoint*> node2chokepoint;
		for (const auto& chokeNodeId : graph.chokeNodes) {
			std::set<nodeID>::iterator i = graph.adjacencyList[chokeNodeId].begin();
			Region* r1 = node2region[*i];
			i++;
			Region* r2 = node2region[*i];
			BWAPI::Position side1(chokepointSides.at(chokeNodeId).side1);
			BWAPI::Position side2(chokepointSides.at(chokeNodeId).side2);
			Chokepoint* newChokepoint = new ChokepointImpl(std::make_pair(r1, r2), std::make_pair(side1, side2));
			chokepoints.insert(newChokepoint);
			node2chokepoint.insert(std::make_pair(chokeNodeId, newChokepoint));
		}
		//LOG(" - Linking regions to chokepoints.");
		for (const auto& regionNodeId : graph.regionNodes) {
			Region* region = node2region[regionNodeId];
			std::set<Chokepoint*> chokepoints;
			for (const auto& chokeNodeId : graph.adjacencyList[regionNodeId]) {
				chokepoints.insert(node2chokepoint[chokeNodeId]);
			}
			((RegionImpl*)region)->_chokepoints = chokepoints;
		}

	}
}