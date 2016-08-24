#pragma once

#include <BWTA/Polygon.h>

#include "MapData.h"
#include "RegionImpl.h"
#include "ChokepointImpl.h"

namespace BWTA
{
	typedef size_t nodeID;

	struct chokeSides_t {
		BWAPI::WalkPosition side1;
		BWAPI::WalkPosition side2;
		chokeSides_t(BWAPI::WalkPosition s1, BWAPI::WalkPosition s2) : side1(s1), side2(s2) {};
	};

	class RegionGraph
	{
	private:
		// mapping between Voronoi vertices and graph nodes
		std::map<const BoostVoronoi::vertex_type*, nodeID> voronoiVertexToNode;

	public:
		enum NodeType { NONE, REGION, CHOKEPOINT, CHOKEGATEA, CHOKEGATEB };

		// TODO, create struct
		std::vector<BWAPI::WalkPosition> nodes;
		std::vector<std::set<nodeID>> adjacencyList;
		std::vector<double> minDistToObstacle;
		std::vector<NodeType> nodeType;

		std::set<nodeID> regionNodes;
		std::set<nodeID> chokeNodes;
		std::set<nodeID> gateNodesA;
		std::set<nodeID> gateNodesB;

		nodeID addNode(const BoostVoronoi::vertex_type* vertex, const BWAPI::WalkPosition& pos);
		nodeID addNode(const BWAPI::WalkPosition& pos, const double& minDist);
		void addEdge(const nodeID& v0, const nodeID& v1);
	};


	void generateVoronoid(const std::vector<Polygon>& polygons, const RectangleArray<int>& labelMap, 
		RegionGraph& graph, bgi::rtree<BoostSegmentI, bgi::quadratic<16> >& rtree);
	void pruneGraph(RegionGraph& graph);
	void markRegionNodes(RegionGraph& graph, const std::vector<Polygon>& polygons);
	void simplifyRegionGraph(const RegionGraph& graph, RegionGraph& graphSimplified);
	void getChokepointSides(const RegionGraph& graph, const bgi::rtree<BoostSegmentI, bgi::quadratic<16> >& rtree, std::map<nodeID, chokeSides_t>& chokepointSides);
	void createRegionsFromGraph(const std::vector<BoostPolygon>& polygons, const RectangleArray<int>& labelMap,
		const RegionGraph& graph, const std::map<nodeID, chokeSides_t>& chokepointSides,
		std::set<Region*>& regions, std::set<Chokepoint*>& chokepoints,
		std::vector<BoostPolygon>& polReg);
}