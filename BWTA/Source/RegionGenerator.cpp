#include "RegionGenerator.h"

namespace BWTA
{
	static const std::size_t EXTERNAL_COLOR = 1;

	void colorExterior(const BoostVoronoi::edge_type* edge) {
		if (edge->color() == EXTERNAL_COLOR) {
			return;
		}
		edge->color(EXTERNAL_COLOR);
		edge->twin()->color(EXTERNAL_COLOR);
		const BoostVoronoi::vertex_type* v = edge->vertex1();
		if (v == NULL || !edge->is_primary()) {
			return;
		}
		v->color(EXTERNAL_COLOR);
		const BoostVoronoi::edge_type* e = v->incident_edge();
		do {
			colorExterior(e);
			e = e->rot_next();
		} while (e != v->incident_edge());
	}

	void colorConnected(BoostVoronoi& vd) {
		const BoostVoronoi::vertex_type& vertex = vd.vertices().at(0);
// 		for (const auto& vertex : vd.vertices()) {
		const BoostVoronoi::edge_type* edge = vertex.incident_edge();
		do {
			if (edge->is_primary()) {
				edge->color(EXTERNAL_COLOR);
				edge->twin()->color(EXTERNAL_COLOR);
			}
			edge = edge->rot_next();
		} while (edge != vertex.incident_edge());
// 		}

	}

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
// 		for (const auto& val : border) {
// 			LOG(val);
// 		}

		auto it = border.begin();
		if (*it == 0) ++it;
		for (it; it != border.end();) {
			if (*it == maxX) break;
			VoronoiPoint point1(*it, y); ++it;
			VoronoiPoint point2(*it, y); ++it;
			segments.push_back(VoronoiSegment(point1, point2));
// 			LOG("(" << point1.x() << "," << point1.y() << ") - (" << point2.x() << "," << point2.y() << ")");
		}
	}


	void generateVoronoid(const std::vector<Polygon>& polygons, BoostVoronoi& voronoi)
	{
		std::vector<VoronoiSegment> segments;

		// containers for border points (we need to fill the borders of the map with segments)
		std::set<int> rightBorder, leftBorder, topBorder; // bottomBorder not needed since it's always unwalkable
		int maxX = MapData::walkability.getWidth() - 1;
		int maxY = MapData::walkability.getHeight() - 1;

		// Add the line segments of each polygon
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

// 				if (polygon[i].x < 4) {
// 					LOG("(" << polygon[i].x << "," << polygon[i].y << ") - (" << polygon[j].x << "," << polygon[j].y << ")");
// 				}
			}
			// Add the vertices of each hole in the polygon
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
		
		boost::polygon::construct_voronoi(segments.begin(), segments.end(), &voronoi);

		// Color exterior edges.
// 		for (auto it = voronoi.edges().begin(); it != voronoi.edges().end(); ++it) {
// 			if (!it->is_finite()) {
// 				colorExterior(&(*it));
// 			}
// 		}

		colorConnected(voronoi);
	}
}