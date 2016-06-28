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
		std::vector<BWAPI::WalkPosition> nodes;
		std::vector<std::set<nodeID>> adjacencyList;
		std::vector<double> minDistToObstacle;
		std::vector<nodeID> regionNodes;
		std::vector<nodeID> chokeNodes;

		nodeID addNode(const BoostVoronoi::vertex_type* vertex, const BWAPI::WalkPosition& pos);
		void addEdge(const nodeID& v0, const nodeID& v1);
	};


	void generateVoronoid(const std::vector<Polygon>& polygons, const RectangleArray<int>& labelMap, RegionGraph& graph);
	void pruneGraph(RegionGraph& graph);
	void markRegionNodes(RegionGraph& graph);
}