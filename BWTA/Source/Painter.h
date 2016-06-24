#pragma once

#ifdef DEBUG_DRAW
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtSvg/QSvgGenerator>

#include "MapData.h"
#include "functions.h"
#include "Node.h"

namespace BWTA {
	class Painter {
	public:
		Painter();
		~Painter();
		void render(const std::string& label = std::string());
		void drawMapBorder();
		void drawArrangement(Arrangement_2* arrangement);
		void drawPolygon(Polygon& polygon, QColor color);
		void drawPolygon(PolygonD& polygon, QColor color);
		void drawPolygons(std::vector<Polygon>& polygons);
		void drawPolygons(std::set<Polygon*>& polygons);
		void drawPolygons(const std::vector<BoostPolygon>& polygons);
		void drawNodes(std::set<Node*> nodes, QColor color);
		void drawNodesAndConnectToNeighbors(std::set<Node*> nodes, QColor nodeColor);
		void drawFourColorMap(std::set<Node*> regions);
		void drawHeatMap(RectangleArray<int> map, float maxValue);
		void drawClosestBaseLocationMap(RectangleArray<BaseLocation*> map, std::set<BaseLocation*> baseLocations);
		void drawClosestChokepointMap(RectangleArray<Chokepoint*> map, std::set<Chokepoint*> chokepoints);
		void drawEdges(std::vector<boost::polygon::voronoi_edge<double>> edges);

	private:
		QPainter* painter;
		QImage* image;
		QSvgGenerator* svg;
		int renderCounter;

		QColor hsl2rgb(double h, double sl, double l);
		void getHeatMapColor(float value, float &red, float &green, float &blue);
	};
}
#endif