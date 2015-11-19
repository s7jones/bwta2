#include "Painter.h"

namespace BWTA {
	Painter::Painter() :
		renderCounter(1)
	{
		// PNG
		image = new QImage(MapData::mapWidth * 4, MapData::mapHeight * 4, QImage::Format_ARGB32_Premultiplied);
		painter = new QPainter(image);
		painter->setRenderHint(QPainter::Antialiasing);

		// SVG
// 		std::string filename(BWTA_PATH);
// 		filename += MapData::mapFileName + "-" + std::to_string(renderCounter) + ".svg";
// 		renderCounter++;
// 		svg = new QSvgGenerator();
// 		svg->setFileName(filename.c_str());
// 		svg->setSize(QSize(MapData::mapWidth * 4, MapData::mapHeight * 4));
// 		svg->setViewBox(QRect(0, 0, MapData::mapWidth * 4, MapData::mapHeight * 4));
// 		painter = new QPainter();
// 		painter->begin(svg);
	}

	Painter::~Painter()
	{
		delete painter;
		delete image;
// 		delete svg;
	}

	void Painter::render(const std::string& label)
	{
		// save PNG
		std::string filename(BWTA_PATH);
		if (label.empty()) {
			filename += MapData::mapFileName + "-" + std::to_string(renderCounter) + ".png";
			renderCounter++;
		} else {
			filename += MapData::mapFileName + "-" + label + ".png";
		}

		image->save(filename.c_str(), "PNG");
		image->fill(Qt::white); // set background to white again

		// save SVG
// 		std::string filename(BWTA_PATH);
// 		filename += MapData::mapFileName + "-" + std::to_string(renderCounter) + ".svg";
// 		renderCounter++;
// 
// 		painter->end();
// 		delete painter;
// 		delete svg;
// 		svg = new QSvgGenerator();
// 		svg->setFileName(filename.c_str());
// 		svg->setSize(QSize(MapData::mapWidth * 4, MapData::mapHeight * 4));
// 		svg->setViewBox(QRect(0, 0, MapData::mapWidth * 4, MapData::mapHeight * 4));
// 		painter = new QPainter();
// 		painter->begin(svg);
	}

	void Painter::drawMapBorder() {
		QPen qp(QColor(0, 0, 0));
		qp.setWidth(2);
		painter->setPen(qp);
		painter->drawLine(0, 0, 0, MapData::walkability.getHeight() - 1);
		painter->drawLine(0, MapData::walkability.getHeight() - 1, MapData::walkability.getWidth() - 1, MapData::walkability.getHeight() - 1);
		painter->drawLine(MapData::walkability.getWidth() - 1, MapData::walkability.getHeight() - 1, MapData::walkability.getWidth() - 1, 0);
		painter->drawLine(MapData::walkability.getWidth() - 1, 0, 0, 0);
	}

	void Painter::drawArrangement(Arrangement_2* arrangement) {
		for (Arrangement_2::Edge_iterator eit = arrangement->edges_begin(); eit != arrangement->edges_end(); ++eit)
		{
			double x0 = cast_to_double(eit->curve().source().x());
			double y0 = cast_to_double(eit->curve().source().y());
			double x1 = cast_to_double(eit->curve().target().x());
			double y1 = cast_to_double(eit->curve().target().y());
			QColor color(0, 0, 0);
			if (eit->data() == BLUE) {
				color = QColor(0, 0, 255);
			} else if (eit->data() == BLACK)  {
				color = QColor(0, 0, 0);
			} else if (eit->data() == RED) {
				color = QColor(255, 0, 0);
			} else {
				color = QColor(0, 180, 0);
			}
			QPen qp(color);
			qp.setWidth(2);
			painter->setPen(qp);
			painter->drawLine(x0, y0, x1, y1);
		}
	}

	void Painter::drawPolygon(Polygon& polygon, QColor color) {
		QVector<QPointF> qp;
		for (int i = 0; i < (int)polygon.size(); i++) {
			int j = (i + 1) % polygon.size();
			qp.push_back(QPointF(polygon[i].x, polygon[i].y));
		}
		painter->setPen(QPen(Qt::black));
		painter->setBrush(QBrush(color));
		painter->drawPolygon(QPolygonF(qp));
	}

	void Painter::drawPolygon(PolygonD& polygon, QColor color) {
		QVector<QPointF> qp;
		for (unsigned int i = 0; i < polygon.size(); i++) {
			qp.push_back(QPointF(cast_to_double(polygon.vertex(i).x()), cast_to_double(polygon.vertex(i).y())));
		}
		painter->setPen(QPen(Qt::black));
		painter->setBrush(QBrush(color));
		painter->drawPolygon(QPolygonF(qp));
	}

	void Painter::drawPolygons(std::vector<Polygon>& polygons) {
		for (auto& polygon : polygons) {
			drawPolygon(polygon, QColor(180, 180, 180));
			for (auto& hole : polygon.holes) {
				drawPolygon(hole, QColor(255, 100, 255));
			}
		}
	}

	void Painter::drawPolygons(std::set<Polygon*>& polygons) {
		for (auto& polygon : polygons) {
			drawPolygon(*polygon, QColor(180, 180, 180));
			for (auto& hole : polygon->holes) {
				drawPolygon(hole, QColor(255, 100, 255));
			}
		}
	}

	void Painter::drawNodes(std::set<Node*> nodes, QColor color) {
		painter->setPen(QPen(color));
		painter->setBrush(QBrush(color));
		for (const auto& node : nodes) {
			double x0 = cast_to_double(node->point.x());
			double y0 = cast_to_double(node->point.y());
			painter->drawEllipse(x0 - 6, y0 - 6, 12, 12);
		}
	}

	void Painter::drawNodesAndConnectToNeighbors(std::set<Node*> nodes, QColor nodeColor) {
		QPen penLine(Qt::black);
		penLine.setWidth(2);
		for (const auto& node : nodes) {
			double x0 = cast_to_double(node->point.x());
			double y0 = cast_to_double(node->point.y());
			for (const auto& node2 : node->neighbors) {
				double x1 = cast_to_double(node2->point.x());
				double y1 = cast_to_double(node2->point.y());
				painter->setPen(penLine);
				painter->drawLine(x0, y0, x1, y1);
			}
			painter->setPen(QPen(nodeColor));
			painter->setBrush(QBrush(nodeColor));
			painter->drawEllipse(x0 - 6, y0 - 6, 12, 12);
		}
	}

	void Painter::drawFourColorMap(std::set<Node*> regions) {
		// Assign a HUE to each region
		double d, s;
		for (auto& region : regions) {
			region->hue = rand()*1.0 / RAND_MAX;
		}
		for (int l = 0; l<6; l++) {
			for (auto& region : regions) {
				for (auto& neighbor : region->neighbors) {
					Node* region2 = neighbor->other_neighbor(region);
					d = region2->hue - region->hue;
					if (d > 0.5) d = d - 1.0;
					if (d < -0.5) d = d + 1.0;
					s = d - 0.5;
					if (d < 0) s += 1.0;
					s *= 0.05;
					region->hue += s;
					region2->hue -= s;
					while (region->hue < 0) region->hue += 1.0;
					while (region->hue >= 1.0) region->hue -= 1.0;
					while (region2->hue < 0) region2->hue += 1.0;
					while (region2->hue >= 1.0) region2->hue -= 1.0;
				}
			}
		}
		// Draw regions
		for (auto& region : regions) {
			PolygonD polygon = region->get_polygon();
			drawPolygon(region->get_polygon(), hsl2rgb(region->hue, 1.0, 0.75));
		}
	}


	QColor Painter::hsl2rgb(double h, double sl, double l)
	{
		double v;
		double r, g, b;
		r = l;   // default to gray
		g = l;
		b = l;
		v = (l <= 0.5) ? (l * (1.0 + sl)) : (l + sl - l * sl);
		if (v > 0)
		{
			double m;
			double sv;
			int sextant;
			double fract, vsf, mid1, mid2;
			m = l + l - v;
			sv = (v - m) / v;
			h *= 6.0;
			sextant = (int)h;
			fract = h - sextant;
			vsf = v * sv * fract;
			mid1 = m + vsf;
			mid2 = v - vsf;
			switch (sextant)
			{
			case 0:
				r = v;
				g = mid1;
				b = m;
				break;
			case 1:
				r = mid2;
				g = v;
				b = m;
				break;
			case 2:
				r = m;
				g = v;
				b = mid1;
				break;
			case 3:
				r = m;
				g = mid2;
				b = v;
				break;
			case 4:
				r = mid1;
				g = m;
				b = v;
				break;
			case 5:
				r = v;
				g = m;
				b = mid2;
				break;
			}
		}
		return QColor(r*255.0, g*255.0, b*255.0);
	}

	void Painter::getHeatMapColor(float value, float &red, float &green, float &blue)
	{
		const int NUM_COLORS = 3;
		static float color[NUM_COLORS][3] = { { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 } };
		// a static array of 3 colors:  (red, green, blue,   green)

		int idx1;        // |-- our desired color will be between these two indexes in "color"
		int idx2;        // |
		float fractBetween = 0;  // fraction between "idx1" and "idx2" where our value is

		if (value <= 0)      { idx1 = idx2 = 0; }    // accounts for an input <=0
		else if (value >= 1)  { idx1 = idx2 = NUM_COLORS - 1; }    // accounts for an input >=0
		else
		{
			value = value * (NUM_COLORS - 1);        // will multiply value by 3
			idx1 = (int)floor(value);                  // our desired color will be after this index
			idx2 = idx1 + 1;                        // ... and before this index (inclusive)
			fractBetween = value - float(idx1);    // distance between the two indexes (0-1)
		}

		red = (color[idx2][0] - color[idx1][0])*fractBetween + color[idx1][0];
		green = (color[idx2][1] - color[idx1][1])*fractBetween + color[idx1][1];
		blue = (color[idx2][2] - color[idx1][2])*fractBetween + color[idx1][2];
	}

	void Painter::drawHeatMap(RectangleArray<int> map, float maxValue)
	{
		float red, green, blue;
		QColor heatColor;
		for (unsigned int x = 0; x < map.getWidth(); ++x) {
			for (unsigned int y = 0; y < map.getHeight(); ++y) {
				float normalized = (float)map[x][y] / maxValue;
				getHeatMapColor(normalized, red, green, blue);
				heatColor = QColor((int)red, (int)green, (int)blue);
				painter->setPen(QPen(heatColor));
				painter->setBrush(QBrush(heatColor));
				painter->drawEllipse(x, y, 1, 1);
			}
		}
	}
}