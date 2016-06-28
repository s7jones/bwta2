#include "RegionGenerator.h"

#include <boost/geometry/index/rtree.hpp>

namespace BWTA
{
	static const std::size_t VISITED_COLOR = 1;

	void addVerticalBorder(std::vector<VoronoiSegment>& segments, const std::set<int>& border, int x, int maxY)
	{
// 		for (const auto& val : border) {
// 			LOG(val);
// 		}

		auto it = border.begin();
		if (*it == 0) ++it;
		for (it; it != border.end();) {
			if (*it == maxY) break;
			VoronoiPoint point1(x, *it); ++it;
			VoronoiPoint point2(x, *it); ++it;
			segments.push_back(VoronoiSegment(point1, point2));
// 			LOG("(" << point1.x() << "," << point1.y() << ") - (" << point2.x() << "," << point2.y() << ")");
		}
	}

	void addHorizontalBorder(std::vector<VoronoiSegment>& segments, const std::set<int>& border, int y, int maxX)
	{
		auto it = border.begin();
		if (*it == 0) ++it;
		for (it; it != border.end();) {
			if (*it == maxX) break;
			VoronoiPoint point1(*it, y); ++it;
			VoronoiPoint point2(*it, y); ++it;
			segments.push_back(VoronoiSegment(point1, point2));
		}
	}


	void generateVoronoid(const std::vector<Polygon>& polygons, const RectangleArray<int>& labelMap, RegionGraph& graph)
	{
		std::vector<VoronoiSegment> segments;
		using BoostPointI = std::pair<BoostPoint, std::size_t>;
		std::vector<BoostPointI> rtreePoints;
		size_t idPoint = 0;

		// containers for border points (we need to fill the borders of the map with segments)
		std::set<int> rightBorder, leftBorder, topBorder; // bottomBorder not needed since it's always unwalkable
		int maxX = MapData::walkability.getWidth() - 1;
		int maxY = MapData::walkability.getHeight() - 1;

		// Add the line segments of each polygon to VoronoiSegment list and points to R-tree points list
		for (const auto& polygon : polygons) {
			// Add the vertices of the polygon
			for (size_t i = 0; i < polygon.size(); i++) {
				// save border points
				if (polygon[i].x == 0) leftBorder.insert(polygon[i].y);
				else if (polygon[i].x == maxX) rightBorder.insert(polygon[i].y);
				if (polygon[i].y == 0) topBorder.insert(polygon[i].x);

				int j = (i + 1) % polygon.size(); // because polygons aren't close

				segments.push_back(VoronoiSegment(
					VoronoiPoint(polygon[j].x, polygon[j].y),
					VoronoiPoint(polygon[i].x, polygon[i].y)));

				rtreePoints.push_back(std::make_pair(BoostPoint(polygon[i].x, polygon[i].y), idPoint++));
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
		LOG(" - Generating border");
		addVerticalBorder(segments, leftBorder, 0, maxY);
		addVerticalBorder(segments, rightBorder, maxX, maxY);
		addHorizontalBorder(segments, topBorder, 0, maxX);
		
		BoostVoronoi voronoi;
		boost::polygon::construct_voronoi(segments.begin(), segments.end(), &voronoi);


		// traverse the Voronoi diagram and generate graph nodes and edges
		for (const auto& edge : voronoi.edges()) {
			if (!edge.is_primary() || !edge.is_finite() || edge.color() == VISITED_COLOR) continue;

			const BoostVoronoi::vertex_type* v0 = edge.vertex0();
			const BoostVoronoi::vertex_type* v1 = edge.vertex1();
			BWAPI::WalkPosition p0((int)v0->x(), (int)v0->y());
			BWAPI::WalkPosition p1((int)v1->x(), (int)v1->y());

			// skip edge if any of its endpoints is inside an obstacle
			if (labelMap[p0.x][p0.y] > 0 || labelMap[p1.x][p1.y] > 0) continue;

			nodeID v0ID = graph.addNode(v0, p0);
			nodeID v1ID = graph.addNode(v1, p1);
			graph.addEdge(v0ID, v1ID);

			// mark half-edge twin as visited
			edge.twin()->color(VISITED_COLOR);
		}

		// create an R-tree to query nearest points/obstacle
		namespace bg = boost::geometry;
		namespace bgi = boost::geometry::index;
		bgi::rtree<BoostPointI, bgi::quadratic<16> > rtree(rtreePoints);
		graph.minDistToObstacle.resize(graph.nodes.size());
		for (size_t i = 0; i < graph.nodes.size(); ++i) {
			std::vector<BoostPointI> returnedValues;
			BoostPoint pt(graph.nodes[i].x, graph.nodes[i].y);
			rtree.query(bgi::nearest(pt, 1), std::back_inserter(returnedValues));
			graph.minDistToObstacle[i] = bg::distance(returnedValues.front().first, pt);
		}
	}

	nodeID RegionGraph::addNode(const BoostVoronoi::vertex_type* vertex, const BWAPI::WalkPosition& pos)
	{
		// add new point if not present in the graph
		auto vIt = voronoiVertexToNode.find(vertex);
		nodeID vID;
		if (vIt == voronoiVertexToNode.end()) {
			voronoiVertexToNode[vertex] = vID = nodes.size();
			nodes.push_back(pos);
		} else {
			vID = vIt->second;
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

			if (graph.adjacencyList.at(v0).empty()) continue; // isolated point???

			nodeID v1 = *graph.adjacencyList.at(v0).begin();
			// remove node if it's too close to an obstacle, or parent is farther to an obstacle
			if (graph.minDistToObstacle.at(v0) < 5.0 
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

	void markRegionNodes(RegionGraph& graph)
	{
		const double MIN_RADII_DIFF = 1.0;
		struct parentNode_t {
			nodeID id;
			bool isMaximal;
			parentNode_t() : id(0), isMaximal(false) {}
// 			parentNode_t(nodeID _id, bool isMax) : id(_id), isMaximal(isMax) {}
		};
		// container to mark visited nodes, and parent list
		std::vector<bool> visited;
		visited.resize(graph.nodes.size());
		std::vector<parentNode_t> parentNodes;
		parentNodes.resize(graph.nodes.size());

		std::stack<nodeID> nodeToVisit;
		parentNode_t parentNode;
		// find a leaf node to start
// 		for (size_t id = 0; id < graph.adjacencyList.size(); ++id) {
// 			if (graph.adjacencyList.at(id).size() == 1) {
		size_t id = 326;
				// mark node as graph
				visited.at(id) = true;
				graph.regionNodes.push_back(id);
				parentNode.id = id; parentNode.isMaximal = true;
				parentNodes.at(id) = parentNode;
				LOG("Found leaf " << id << " at " << graph.nodes.at(id) << " with radii " << graph.minDistToObstacle.at(id));
				// add children to explore
// 				std::stringstream childList;
				for (const auto& v1 : graph.adjacencyList.at(id)) {
					nodeToVisit.push(v1);
					visited.at(v1) = true;
					parentNodes.at(v1) = parentNode;
// 					childList << v1 << ",";
				}
// 				LOG(" - " << id << " Childs: " << childList.str());
// 				break;
// 			}
// 		}

		while (!nodeToVisit.empty()) {
			// pop first element
			nodeID v0 = nodeToVisit.top();
			nodeToVisit.pop();
// 			LOG("Visiting " << v0);


			if (graph.adjacencyList.at(v0).size() != 2) {
				graph.regionNodes.push_back(v0);
				parentNode.id = v0; parentNode.isMaximal = true;
				LOG("Found leaf/connection " << v0 << " at " << graph.nodes.at(v0) << " with radii " << graph.minDistToObstacle.at(v0));
// 				break;
			} else {

				// get parent
				parentNode = parentNodes.at(v0);

				if (parentNode.isMaximal) {
					// look if the node is a local minimal (chokepoint node)
					bool localMinimal = true;
					for (const auto& v1 : graph.adjacencyList.at(v0)) {
						if (graph.minDistToObstacle.at(v0) > graph.minDistToObstacle.at(v1)) {
							localMinimal = false;
							break;
						}
					}
					if (localMinimal) {
						if (std::abs(graph.minDistToObstacle.at(v0) - graph.minDistToObstacle.at(parentNode.id)) > MIN_RADII_DIFF) {
							LOG("Found minimal " << v0 << " at " << graph.nodes.at(v0) << " with radii " << graph.minDistToObstacle.at(v0) << " and parent " << parentNode.id);
// 							LOG(graph.minDistToObstacle.at(v0) << " - " << graph.minDistToObstacle.at(parentNode.id) << " = " << std::abs(graph.minDistToObstacle.at(v0) - graph.minDistToObstacle.at(parentNode.id)));
							graph.chokeNodes.push_back(v0);
							parentNode.id = v0; parentNode.isMaximal = false;
// 							LOG(" - Parent changed to " << v0);
// 							if (!parentNode.isMaximal) LOG("Found two consecutive minimals");
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
						if (std::abs(graph.minDistToObstacle.at(v0) - graph.minDistToObstacle.at(parentNode.id)) > MIN_RADII_DIFF) {
							LOG("Found maximal " << v0 << " at " << graph.nodes.at(v0) << " with radii " << graph.minDistToObstacle.at(v0) << " and parent " << parentNode.id);
// 							LOG(graph.minDistToObstacle.at(v0) << " - " << graph.minDistToObstacle.at(parentNode.id) << " = " << std::abs(graph.minDistToObstacle.at(v0) - graph.minDistToObstacle.at(parentNode.id)));
							graph.regionNodes.push_back(v0);
							parentNode.id = v0; parentNode.isMaximal = true;
							parentNodes.at(v0) = parentNode;
// 							LOG(" - Parent changed to " << v0);
// 							if (parentNode.isMaximal) LOG("Found two consecutive maximals");
						}
					}
				}
			}

			// keep exploring unvisited neighbors
// 			std::stringstream childList;
			for (const auto& v1 : graph.adjacencyList.at(v0)) {
				if (!visited.at(v1)) {
					nodeToVisit.push(v1);
					visited.at(v1) = true;
					parentNodes.at(v1) = parentNode;
// 					childList << v1 << ",";
				} else {
// 					childList << v1 << "*,";
				}
			}
// 			LOG(" - " << v0 << " Childs: " << childList.str());
		}
	}
}