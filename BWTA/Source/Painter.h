#pragma once

#ifdef DEBUG_DRAW
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QColor>
#include <QtGui/QPen>

#include "MapData.h"
#include "functions.h"
#include "Node.h"

namespace BWTA {
	class Painter {
	public:
		Painter();
		~Painter();
		void render(int step);
		void drawMapBorder();
		void drawArrangement(Arrangement_2* arrangement);
		void drawPolygon(Polygon& polygon, QColor color);
		void drawPolygon(PolygonD& polygon, QColor color);
		void drawPolygons(std::vector<Polygon>& polygons);
		void drawPolygons(std::set<Polygon*>& polygons);
		void drawNodes(std::set<Node*> nodes, QColor color);
		void drawNodesAndConnectToNeighbors(std::set<Node*> nodes, QColor nodeColor);
		void drawFourColorMap(std::set<Node*> regions);
		void drawHeatMap(RectangleArray<int> map, float maxValue);

	private:
		QPainter* painter;
		QImage* image;

		QColor hsl2rgb(double h, double sl, double l);
		void getHeatMapColor(float value, float &red, float &green, float &blue);
	};
}
#endif