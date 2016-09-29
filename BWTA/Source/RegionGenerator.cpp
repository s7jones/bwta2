#include "RegionGenerator.h"

#include "Painter.h"
#include <boost/geometry/index/rtree.hpp>
#include "BWTA_Result.h"

namespace BWTA
{
	static const int SKIP_NEAR_BORDER = 3;
	static const double DIFF_COEFICIENT = 0.21; // relative difference to consider a change between region<->chokepoint
	static const int MIN_REG_DIST = 7; // minimum distance between nodes
// 	#define DEBUG_NODE_DETECTION  // uncomment to print node detection process

	bool enoughDifference(const double& A, const double& B)
	{
		double diff = std::abs(A - B);
		double largest = (B > A) ? B : A;

		if (diff > largest * DIFF_COEFICIENT)
			return true;
		return false;
	}

	void addVerticalBorder(std::vector<VoronoiSegment>& segments, std::vector<BoostSegmentI>& rtreeSegments, size_t& idPoint, 
		const std::set<int>& border, int x, int maxY)
	{
// 		for (const auto& val : border) LOG(val);

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
// 		for (const auto& val : border) LOG(val);

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
// 		auto polygon = polygons.at(6);
		size_t i, j;
		for (const auto& polygon : polygons) {
			// Add the vertices of the polygon
			size_t lastPoint = polygon.size() - 1;
			// Notice that polygons are closed, i.e. the last vertex is equal to the first
			for (i = 0, j = 1; i < lastPoint; ++i, ++j) {

				// save border points
				if (polygon[i].x == 0 && polygon[j].x == 0) {
					leftBorder.insert(polygon[i].y);
					leftBorder.insert(polygon[j].y);
				} else if (polygon[i].x == maxX && polygon[j].x == maxX)  {
					rightBorder.insert(polygon[i].y);
					rightBorder.insert(polygon[j].y);
				}
				if (polygon[i].y == 0 && polygon[j].y == 0) {
					topBorder.insert(polygon[i].x);
					topBorder.insert(polygon[j].x);
				}
				
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
// 		LOG(" - Generating LEFT border");
		addVerticalBorder(segments, rtreeSegments, idPoint, leftBorder, 0, maxY);
// 		LOG(" - Generating RIGHT border");
		addVerticalBorder(segments, rtreeSegments, idPoint, rightBorder, maxX, maxY);
// 		LOG(" - Generating TOP border");
		addHorizontalBorder(segments, rtreeSegments, idPoint, topBorder, 0, maxX);

		BoostVoronoi voronoi;
		boost::polygon::construct_voronoi(segments.begin(), segments.end(), &voronoi);
		LOG(" - Voronoi constructed");

		const int maxXborder = maxX - SKIP_NEAR_BORDER;
		const int maxYborder = maxY - SKIP_NEAR_BORDER;

		// traverse the Voronoi diagram and generate graph nodes and edges
		static const std::size_t VISITED_COLOR = 1;
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

	void RegionGraph::markNodeAsRegion(const nodeID& v0)
	{
		regionNodes.insert(v0);
		nodeType.at(v0) = RegionGraph::REGION;
	}
	void RegionGraph::markNodeAsChoke(const nodeID& v0)
	{
		chokeNodes.insert(v0);
		nodeType.at(v0) = RegionGraph::CHOKEPOINT;
	}
	void RegionGraph::unmarkRegionNode(const nodeID& v0)
	{
		regionNodes.erase(v0);
		nodeType.at(v0) = RegionGraph::NONE;
	}
	void RegionGraph::unmarkChokeNode(const nodeID& v0)
	{
		chokeNodes.erase(v0);
		nodeType.at(v0) = RegionGraph::NONE;
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

			if (graph.adjacencyList.at(v0).empty())  continue;

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
				// TODO make 6.0 const
				if (graph.adjacencyList.at(v1).empty() && graph.minDistToObstacle.at(v1) > 6.0) {
// 					LOG("Found isolated point");
					graph.markNodeAsRegion(v1);
				}
			}
		}
	}

	void detectNodes(RegionGraph& graph, const std::vector<Polygon>& polygons)
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
				graph.markNodeAsRegion(id);
				visited.at(id) = true;
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

#if defined(DEBUG_DRAW) && defined(DEBUG_NODE_DETECTION)
		LOG("We are drawing each node detection step, it might take awhile...");
		Painter painter;
		std::string drawDebugMessage;
		int nodesDetected = 0;
		int oldNodesDetected = 0;
#endif

		while (!nodeToVisit.empty()) {
			// pop first element
			nodeID v0 = nodeToVisit.top();
			nodeToVisit.pop();
			// get parent
			parentNode = parentNodes.at(v0);

			bool neutralNode = false;
			if (graph.adjacencyList.at(v0).size() != 2) {
				// if parent chokepoint too close, delete parent
				if (graph.nodeType.at(parentNode.id) == RegionGraph::CHOKEPOINT
					&& graph.nodes.at(v0).getApproxDistance(graph.nodes.at(parentNode.id)) < MIN_REG_DIST) {
					graph.unmarkChokeNode(parentNode.id);
#if defined(DEBUG_DRAW) && defined(DEBUG_NODE_DETECTION)
					drawDebugMessage += "Parent chokepoint deleted, too close\n";
#endif
				}
				graph.markNodeAsRegion(v0);
				parentNode.id = v0; parentNode.isMaximal = true;
#if defined(DEBUG_DRAW) && defined(DEBUG_NODE_DETECTION)
				drawDebugMessage += "Leaf or intersection";
				nodesDetected++;
#endif
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
						// we have two consecutive minimals, keep the min
						if (graph.minDistToObstacle.at(v0) < graph.minDistToObstacle.at(parentNode.id)) {
							graph.unmarkChokeNode(parentNode.id);
							graph.markNodeAsChoke(v0);
							parentNode.id = v0; parentNode.isMaximal = false;
#if defined(DEBUG_DRAW) && defined(DEBUG_NODE_DETECTION)
							drawDebugMessage += "Better consecutive chokepoint";
							nodesDetected++;
#endif
						}
					} else { // parent is maximal
						if (graph.adjacencyList.at(parentNode.id).size() > 2 
							&& graph.nodes.at(v0).getApproxDistance(graph.nodes.at(parentNode.id)) >= MIN_REG_DIST
							) {
							graph.markNodeAsChoke(v0);
							parentNode.id = v0; parentNode.isMaximal = false;
#if defined(DEBUG_DRAW) && defined(DEBUG_NODE_DETECTION)
							drawDebugMessage += "Parent is intersection";
							nodesDetected++;
#endif
						} else if (enoughDifference(graph.minDistToObstacle.at(v0), graph.minDistToObstacle.at(parentNode.id))) {
								graph.markNodeAsChoke(v0);
								parentNode.id = v0; parentNode.isMaximal = false;
#if defined(DEBUG_DRAW) && defined(DEBUG_NODE_DETECTION)
								drawDebugMessage += "Enough difference";
								nodesDetected++;
#endif
						} else {
							// the radius difference is too small
							neutralNode = true;
#if defined(DEBUG_DRAW) && defined(DEBUG_NODE_DETECTION)
							graph.gateNodesA.insert(v0);
// 							graph.nodeType.at(v0) = RegionGraph::CHOKEGATEA;
							double A = graph.minDistToObstacle.at(v0);
							double B = graph.minDistToObstacle.at(parentNode.id);
							double diff = std::abs(A - B);
							double largest = (B > A) ? B : A;
							std::stringstream stream;
							stream << "Chokepoint omitted, not enough difference\n";
							stream << diff << " < " << largest * DIFF_COEFICIENT << " (" << largest << " * "<< DIFF_COEFICIENT <<")";

							drawDebugMessage += stream.str();
							nodesDetected++;
#endif
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
									graph.unmarkRegionNode(parentNode.id);
								}
								graph.markNodeAsRegion(v0);
								parentNode.id = v0; parentNode.isMaximal = true;
#if defined(DEBUG_DRAW) && defined(DEBUG_NODE_DETECTION)
								drawDebugMessage += "Better consecutive region node";
								nodesDetected++;
#endif
							}
						} else { // parent is minimal
							if (enoughDifference(graph.minDistToObstacle.at(v0), graph.minDistToObstacle.at(parentNode.id))) {
								graph.markNodeAsRegion(v0);
								parentNode.id = v0; parentNode.isMaximal = true;
#if defined(DEBUG_DRAW) && defined(DEBUG_NODE_DETECTION)
								drawDebugMessage += "Enough difference";
								nodesDetected++;
#endif
							} else {
								// the radius difference is too small
								neutralNode = true;
#if defined(DEBUG_DRAW) && defined(DEBUG_NODE_DETECTION)
								graph.gateNodesA.insert(v0);
// 								graph.nodeType.at(v0) = RegionGraph::CHOKEGATEA;
								double A = graph.minDistToObstacle.at(v0);
								double B = graph.minDistToObstacle.at(parentNode.id);
								double diff = std::abs(A - B);
								double largest = (B > A) ? B : A;
								std::stringstream stream;
								stream << "Region omitted, not enough difference\n";
								stream << diff << " < " << largest * DIFF_COEFICIENT << " (" << largest << " * "<< DIFF_COEFICIENT <<")";

								drawDebugMessage += stream.str();
								nodesDetected++;
#endif
							}
						}
					} else { // not local maximal o minimal
						neutralNode = true;
					}
				}
			}

			// keep exploring unvisited neighbors
			for (const auto& v1 : graph.adjacencyList.at(v0)) {
				if (!visited.at(v1)) {
					nodeToVisit.push(v1);
					visited.at(v1) = true;
					parentNodes.at(v1) = parentNode;
				} else {
					// we reached a connection between visited paths.
					// we must check if the connected path between choke-region nodes is too close
					nodeID v0Parent, v1Parent;
					// get right parent of v0
					if (graph.nodeType.at(v0) == RegionGraph::REGION || graph.nodeType.at(v0) == RegionGraph::CHOKEPOINT) {
						v0Parent = v0;
					} else v0Parent = parentNodes.at(v0).id;
					// get right parent of v1
					if (graph.nodeType.at(v1) == RegionGraph::REGION || graph.nodeType.at(v1) == RegionGraph::CHOKEPOINT) {
						v1Parent = v1;
					} else v1Parent = parentNodes.at(v1).id;
					nodeID isMaximal0 = false;
					if (graph.nodeType.at(v0Parent) == RegionGraph::REGION) isMaximal0 = true;
					nodeID isMaximal1 = false;
					if (graph.nodeType.at(v1Parent) == RegionGraph::REGION) isMaximal1 = true;

// 					if (!isMaximal0 && isMaximal1) LOG("Choke-region " << v0Parent << "-" << v1Parent);
// 					if (isMaximal0 && !isMaximal1) LOG("Region-choke " << v0Parent << "-" << v1Parent);

					if (isMaximal0 != isMaximal1 && 
						graph.nodes.at(v0Parent).getApproxDistance(graph.nodes.at(v1Parent)) < MIN_REG_DIST) {
						nodeID nodeToDelete = v0Parent;
						if (isMaximal0)  nodeToDelete = v1Parent;
						graph.unmarkChokeNode(nodeToDelete);
#if defined(DEBUG_DRAW) && defined(DEBUG_NODE_DETECTION)
						graph.gateNodesA.insert(nodeToDelete);
						drawDebugMessage += "\nChoke too close to region (path)";
						nodesDetected++;
#endif
					}
				}
			}

#if defined(DEBUG_DRAW) && defined(DEBUG_NODE_DETECTION)
			// iterative drawing of nodes detected
			if (oldNodesDetected != nodesDetected) {
				painter.drawPolygons(polygons);
				painter.drawGraph(graph);
				painter.drawNodes(graph, graph.regionNodes, Qt::blue);
				painter.drawNodes(graph, graph.chokeNodes, Qt::red);
				painter.drawNodes(graph, graph.gateNodesA, Qt::green);
				painter.drawNodes(graph, graph.gateNodesB, Qt::darkGreen);
				if (!drawDebugMessage.empty()) {
					painter.drawText(5, 12, drawDebugMessage);
					drawDebugMessage.clear();
				}
				painter.render();
				oldNodesDetected = nodesDetected;
			}
#endif
		}
	}

	std::string getString(std::set<nodeID> nodeSet)
	{
		std::stringstream stream;
		for (auto& id : nodeSet) stream << id << ",";
		return stream.str();
	}

	void simplifyGraph(const RegionGraph& graph, RegionGraph& graphSimplified)
	{
		// containers to mark visited nodes, and parent list
		std::vector<bool> visited;
		visited.resize(graph.nodes.size());
		std::vector<nodeID> parentID;
		parentID.resize(graph.nodes.size());

		nodeID leafRegionId = 99;
		nodeID newleafRegionId;
		for (const auto& regionId : graph.regionNodes) {
			if (leafRegionId == 99 && graph.adjacencyList.at(regionId).size() == 1) {
				// start with one leaf region node
				leafRegionId = regionId;
				visited.at(regionId) = true;
				newleafRegionId = graphSimplified.addNode(graph.nodes.at(regionId), graph.minDistToObstacle.at(regionId));
				graphSimplified.markNodeAsRegion(newleafRegionId);
			}
			if (graph.adjacencyList.at(regionId).empty()) {
				// add "island" regions nodes
				nodeID newId = graphSimplified.addNode(graph.nodes.at(regionId), graph.minDistToObstacle.at(regionId));
				graphSimplified.markNodeAsRegion(newId);
			}
		}

		// add children to explore
		std::stack<nodeID> nodeToVisit;
		for (const auto& v1 : graph.adjacencyList.at(leafRegionId)) {
			nodeToVisit.emplace(v1);
			visited.at(v1) = true;
			parentID.at(v1) = newleafRegionId;
		}

		while (!nodeToVisit.empty()) {
			// pop first element
			nodeID nodeId = nodeToVisit.top();
			nodeToVisit.pop();
			nodeID parentId = parentID.at(nodeId);

			if (graph.nodeType.at(nodeId) == RegionGraph::CHOKEPOINT) {
				nodeID newId = graphSimplified.addNode(graph.nodes.at(nodeId), graph.minDistToObstacle.at(nodeId));
				if (newId != parentId) { // to avoid self inclusions
					graphSimplified.markNodeAsChoke(newId);
					graphSimplified.addEdge(newId, parentId);
					parentId = newId;
				}
			} else if (graph.nodeType.at(nodeId) == RegionGraph::REGION) {
				nodeID newId = graphSimplified.addNode(graph.nodes.at(nodeId), graph.minDistToObstacle.at(nodeId));
				if (newId != parentId) { // to avoid self inclusions
					graphSimplified.markNodeAsRegion(newId);
					graphSimplified.addEdge(newId, parentId);
					parentId = newId;
				}
			}

// 			std::stringstream toPrint;
// 			for (const auto& v1 : graph.adjacencyList.at(nodeId)) toPrint << v1 << ",";
// 			LOG("OUT " << nodeId << " CHILDREN: " << toPrint.str());

			// keep exploring unvisited neighbors
			for (const auto& v1 : graph.adjacencyList.at(nodeId)) {
				if (!visited.at(v1)){
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
					if (n1 != n2 && graphSimplified.nodeType.at(n1) != RegionGraph::NONE
						&& graphSimplified.nodeType.at(n2) != RegionGraph::NONE) {
						graphSimplified.addEdge(n1, n2);
					}
				}
			}
		}
	}

	void mergeRegionNodes(RegionGraph& graph)
	{
		std::set<nodeID> mergeRegions(graph.regionNodes);

		while (!mergeRegions.empty()) {
			// pop first element
			nodeID parent = *mergeRegions.begin();
			mergeRegions.erase(mergeRegions.begin());

			for (auto& it = graph.adjacencyList.at(parent).begin(); it != graph.adjacencyList.at(parent).end();) {
				nodeID child = *it;
				if (parent == child) { // This is a self-loop, remove it
					it = graph.adjacencyList.at(parent).erase(it);
					continue;
				}
				if (graph.nodeType.at(parent) == RegionGraph::REGION && graph.nodeType.at(child) == RegionGraph::REGION) {
// 					LOG(" - Set " << parent << ":" << getString(graph.adjacencyList.at(parent)));
// 					LOG(" - Set " << child << ":" << getString(graph.adjacencyList.at(child)));
					if (graph.minDistToObstacle.at(parent) > graph.minDistToObstacle.at(child)) {
						// keep parent
						graph.adjacencyList.at(parent).insert(graph.adjacencyList.at(child).begin(),
							graph.adjacencyList.at(child).end());
						// for each adjacent, remove child add parent (except parent)
						for (const auto& v2 : graph.adjacencyList.at(child)) {
							graph.adjacencyList.at(v2).erase(child);
							if (v2 == parent) graph.adjacencyList.at(parent).erase(parent);
							else graph.adjacencyList.at(v2).insert(parent);
						}
						graph.adjacencyList.at(child).clear();
// 						LOG(" - Set " << parent << " merged:" << getString(graph.adjacencyList.at(parent)));
						graph.regionNodes.erase(child);
// 						LOG(child << " erased, " << parent << " keeped");
						// restart iterator
						it = graph.adjacencyList.at(parent).begin();
// #ifdef DEBUG_DRAW
// 						painter.drawGraph(graph);
// 						painter.drawNodes(graph, graph.regionNodes, Qt::blue);
// 						painter.drawNodes(graph, graph.chokeNodes, Qt::red);
// 						painter.render();
// #endif
					} else {
						// keep child
						graph.adjacencyList.at(child).insert(graph.adjacencyList.at(parent).begin(),
							graph.adjacencyList.at(parent).end());
						// for each adjacent, remove parent add child (except child)
						for (const auto& v2 : graph.adjacencyList.at(parent)) {
							graph.adjacencyList.at(v2).erase(parent);
							if (v2 == child) graph.adjacencyList.at(child).erase(child);
							else graph.adjacencyList.at(v2).insert(child);
						}
						graph.adjacencyList.at(parent).clear();
// 						LOG(" - Set " << child << " merged:" << getString(graph.adjacencyList.at(child)));
						graph.regionNodes.erase(parent);
// 						LOG(parent << " erased, " << child << " keeped");
						// child should be re-analyzed
						mergeRegions.insert(child);
// #ifdef DEBUG_DRAW
// 						painter.drawGraph(graph);
// 						painter.drawNodes(graph, graph.regionNodes, Qt::blue);
// 						painter.drawNodes(graph, graph.chokeNodes, Qt::red);
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

	// extend both line's sides from the line's middle
	void extendLine(BoostPoint& a, BoostPoint& b)
	{
		BoostPoint extendCenter = getMidpoint(a, b);
		// translate to extend
		boost::geometry::subtract_point(a, extendCenter);
		boost::geometry::subtract_point(b, extendCenter);
		// extend
		boost::geometry::multiply_value(a, 1.1);
		boost::geometry::multiply_value(b, 1.1);
		// translate back
		boost::geometry::add_point(a, extendCenter);
		boost::geometry::add_point(b, extendCenter);
	}

	void getChokepointSides(const std::vector<Polygon>& polygons, const RegionGraph& graph, const bgi::rtree<BoostSegmentI, bgi::quadratic<16> >& rtree, std::map<nodeID, chokeSides_t>& chokepointSides)
	{
		for (const auto& id : graph.chokeNodes) {
			BoostPoint pt(graph.nodes[id].x, graph.nodes[id].y);
			BWAPI::WalkPosition side1, side2;
			// get nearest value
			auto it = rtree.qbegin(bgi::nearest(pt, 100));
			side1 = getProjectedPoint(pt, (*it).first);
			++it;

			// iterate over nearest Values
			for (it; it != rtree.qend(); ++it) {
				side2 = getProjectedPoint(pt, (*it).first);
				if ((side1.x < pt.x() && side2.x > pt.x()) ||
					(side1.x > pt.x() && side2.x < pt.x()) ||
					(side1.y < pt.y() && side2.y > pt.y()) ||
					(side1.y > pt.y() && side2.y < pt.y())) {
					break;
				}
			}

			chokepointSides.emplace(id, chokeSides_t(side1, side2));
		}
	}

	template <class _Tp1>
	_Tp1 get_set2(std::map<_Tp1, _Tp1> &a, _Tp1 i)
	{
		if (a.find(i) == a.end()) a[i] = i;
		if (i == a[i]) return i;
		a[i] = get_set2(a, a[i]);
		return a[i];
	}

	void createRegionsFromGraph(const std::vector<BoostPolygon>& polygons, const RectangleArray<int>& labelMap,
		const RegionGraph& graph, const std::map<nodeID, chokeSides_t>& chokepointSides, 
		std::vector<Region*>& regions, std::set<Chokepoint*>& chokepoints,
		std::vector<BoostPolygon>& polReg)
	{
		Timer timer;
		timer.start();

		// create regions polygons
		// ===========================================================================

		// Create a new box geometry of the whole map
		BoostPoint topLeft(0, 0);
		BoostPoint bottomLeft(0, labelMap.getHeight() - 1);
		BoostPoint bottomRight(labelMap.getWidth() - 1, labelMap.getHeight() - 1);
		BoostPoint topRight(labelMap.getWidth() - 1, 0);
		std::vector<BoostPoint> points = { topLeft, topRight, bottomRight, bottomLeft, topLeft };
		BoostPolygon mapBorder;
		boost::geometry::assign_points(mapBorder, points);
		boost::geometry::correct(mapBorder);

		// subtract obstacles to get the "negative" geometry of polygon obstacles
		typedef boost::geometry::model::multi_polygon<BoostPolygon> BoostMultiPoly;
		BoostMultiPoly output, obstacles;
		obstacles.reserve(polygons.size());
		std::copy(polygons.begin(), polygons.end(), back_inserter(obstacles));
		boost::geometry::difference(mapBorder, obstacles, output);

		// convert chokepoints to lines (and extend both sides)
		typedef boost::geometry::model::linestring<BoostPoint> BoostLine;
		typedef boost::geometry::model::multi_linestring<BoostLine> BoostMultiLine;
		BoostMultiLine chokeLines;
		for (const auto& choke : chokepointSides) {
			BoostPoint a(choke.second.side1.x, choke.second.side1.y);
			BoostPoint b(choke.second.side2.x, choke.second.side2.y);
			extendLine(a, b);
			std::vector<BoostPoint> line = { a, b };
			BoostLine chokeLine;
			boost::geometry::assign_points(chokeLine, line);
			chokeLines.push_back(chokeLine);
		}

		// expand (buffer) the choke lines to ensure they will "cross" regions
		const double buffer_distance = 0.5;
		boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(buffer_distance);
		boost::geometry::strategy::buffer::join_miter join_strategy;
		boost::geometry::strategy::buffer::end_round end_strategy;
		boost::geometry::strategy::buffer::point_square circle_strategy;
		boost::geometry::strategy::buffer::side_straight side_strategy;
		
		BoostMultiPoly cutPolygons;
		boost::geometry::buffer(chokeLines, cutPolygons,
			distance_strategy, side_strategy,
			join_strategy, end_strategy, circle_strategy);

		// compute the difference between the expanded choke lines (cutPolygons) and the walkable polygon (output)
		// to get the final polygon regions (polReg)
		BoostMultiPoly regionsPoly;
		boost::geometry::difference(output, cutPolygons, regionsPoly);
		
		polReg.reserve(regionsPoly.size());
		std::copy(regionsPoly.begin(), regionsPoly.end(), back_inserter(polReg));

		LOG(" - Polygon region computed in " << timer.stopAndGetTime() << " seconds");
		timer.start();

		// compute label region map
		// ===========================================================================
		RectangleArray<int> regionLabel(MapData::mapWidthWalkRes, MapData::mapHeightWalkRes);
		regionLabel.setTo(0);
		RectangleArray<bool> nodeMap(MapData::mapWidthWalkRes, MapData::mapHeightWalkRes);
		nodeMap.setTo(false);
		int regionLabelId = 1;
		std::map<int, BoostPolygon*> labelToPolygon;

		for (auto& poly : polReg) {
			// TODO we still need to label the choke lines!!!!!!
			scanLineFill(poly.outer(), regionLabelId, regionLabel);
			labelToPolygon[regionLabelId] = &poly;
			regionLabelId++;
		}
// 		regionLabel.saveToFile("logs/regionLabel.txt");

		LOG(" - Label region map computed in " << timer.stopAndGetTime() << " seconds");
		timer.start();

		// Create regions from graph nodes
		// ===========================================================================
		std::map<nodeID, Region*> node2region;
		for (const auto& regionNodeId : graph.regionNodes) {
			// get node regionLabel
			int labelId = regionLabel[graph.nodes[regionNodeId].x][graph.nodes[regionNodeId].y];
			BoostPolygon* regionPol = labelToPolygon[labelId];
			RegionImpl* newRegionImpl = new RegionImpl(*regionPol, 8); // 8 => walk to pixel resolution
			newRegionImpl->_opennessDistance = graph.minDistToObstacle.at(regionNodeId);
			newRegionImpl->_opennessPoint = BWAPI::Position(graph.nodes.at(regionNodeId));
			newRegionImpl->_labelId = labelId;

			Region* newRegion = newRegionImpl;
			regions.push_back(newRegion);
			node2region.emplace(regionNodeId, newRegion);
		}
		// TODO we don't need regionPol function output (just print BWTA::Region)

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

		LOG(" - Created BWTA Regions and Chokepoints in " << timer.stopAndGetTime() << " seconds");
		timer.start();

		// compute reachable region for each region
		// ===========================================================================
		std::map<RegionImpl*, RegionImpl*> regionGroup;
		for (auto regionInterface : BWTA_Result::regions) {
			RegionImpl* region1 = (RegionImpl*)regionInterface;
			for (auto chokepointInterface : regionInterface->getChokepoints()) {
				ChokepointImpl* chokepoint = (ChokepointImpl*)chokepointInterface;
				RegionImpl* region2 = (RegionImpl*)(chokepoint->_regions.first);
				if (region1 == region2) {
					region2 = (RegionImpl*)(chokepoint->_regions.second);
				}
				regionGroup[get_set2(regionGroup, region2)] = get_set2(regionGroup, region1);
			}
		}

		// TODO set reachable ID to speed up queries later
		for (auto regionInterface : BWTA_Result::regions) {
			RegionImpl* region1 = (RegionImpl*)regionInterface;
			region1->reachableRegions.insert(region1);
			for (auto regionInterface2 : BWTA_Result::regions) {
				RegionImpl* region2 = (RegionImpl*)regionInterface2;
				if (region1 == region2) continue;
				if (get_set2(regionGroup, region1) == get_set2(regionGroup, region2)) {
					region1->reachableRegions.insert(region2);
					region2->reachableRegions.insert(region1);
				}
			}
		}

		LOG(" - Reachable regions computed in " << timer.stopAndGetTime() << " seconds");
		timer.start();
	}
}