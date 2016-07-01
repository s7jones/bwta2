#pragma once

#include <BWTA/Polygon.h>

#include "MapData.h"

namespace BWTA
{
	typedef size_t nodeID;

	class RegionGraph
	{
	private:
		// mapping between Voronoi vertices and graph nodes
		std::map<const BoostVoronoi::vertex_type*, nodeID> voronoiVertexToNode;

	public:
		enum NodeType {NONE, REGION, CHOKEPOINT};

		// TODO, create struct
		std::vector<BWAPI::WalkPosition> nodes;
		std::vector<std::set<nodeID>> adjacencyList;
		std::vector<double> minDistToObstacle;
		std::vector<NodeType> nodeType;

		std::set<nodeID> regionNodes;
		std::set<nodeID> chokeNodes;

		nodeID addNode(const BoostVoronoi::vertex_type* vertex, const BWAPI::WalkPosition& pos);
		nodeID addNode(const BWAPI::WalkPosition& pos, const double& minDist);
		void addEdge(const nodeID& v0, const nodeID& v1);
	};


	void generateVoronoid(const std::vector<Polygon>& polygons, const RectangleArray<int>& labelMap, 
		RegionGraph& graph, bgi::rtree<BoostPointI, bgi::quadratic<16> >& rtree);
	void pruneGraph(RegionGraph& graph);
	void markRegionNodes(RegionGraph& graph);
	void simplifyRegionGraph(const RegionGraph& graph, RegionGraph& graphSimplified);
}