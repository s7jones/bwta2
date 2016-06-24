#include "PolygonGenerator.h"

namespace BWTA
{
	// To vectorize obstacles (unwalkable areas) we use the algorithm described in:
	// "A Linear-Time Component-Labeling Algorithm Using Contour Tracing Technique" Chang et al.
	// http://ocrwks11.iis.sinica.edu.tw/~dar/Download/Papers/Component/component_labeling_cviu.pdf
	// This algorithm produce a component-label map and extract the external/internal contours needed to build the polygon

	int8_t searchDirection[8][2] = { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, -1 }, { -1, 0 }, { -1, 1 } };

	void tracer(int& cy, int& cx, int& tracingdirection, RectangleArray<int>& labelMap, const RectangleArray<bool>& bitMap)
	{
		int i, y, x;
		int width = bitMap.getWidth();
		int height = bitMap.getHeight();

		for (i = 0; i < 7; ++i) {
			y = cy + searchDirection[tracingdirection][0];
			x = cx + searchDirection[tracingdirection][1];

			// filter invalid position (out of range)
			if (x < 0 || x >= width || y < 0 || y >= height) {
				tracingdirection = (tracingdirection + 1) % 8;
				continue;
			}
			if (bitMap[y][x]) {
				labelMap[y][x] = -1;
				tracingdirection = (tracingdirection + 1) % 8;
			} else {
				cy = y;
				cx = x;
				break;
			}
		}
	}

	Contour contourTracing(int cy, int cx, const size_t& labelId, int tracingDirection, RectangleArray<int>& labelMap, const RectangleArray<bool>& bitMap)
	{
		bool tracingStopFlag = false, keepSearching = true;
		int fx, fy, sx = cx, sy = cy;
		Contour contourPoints;

		tracer(cy, cx, tracingDirection, labelMap, bitMap);
		contourPoints.emplace_back(cy, cx);

		if (cx != sx || cy != sy) {
			fx = cx;
			fy = cy;

			while (keepSearching) {
				tracingDirection = (tracingDirection + 6) % 8;
				labelMap[cy][cx] = labelId;
				tracer(cy, cx, tracingDirection, labelMap, bitMap);
				contourPoints.emplace_back(cy, cx);

				if (cx == sx && cy == sy) {
					tracingStopFlag = true;
				} else if (tracingStopFlag) {
					if (cx == fx && cy == fy) {
						keepSearching = false;
					} else {
						tracingStopFlag = false;
					}
				}
			}
		}
		return contourPoints;
	}

	// given a bitmap (a walkability map in our context) it returns the external contour of obstacles
	void connectedComponentLabeling(std::vector<Contour>& contours, const RectangleArray<bool>& bitMap)
	{
		int cy, cx, tracingDirection, connectedComponentsCount = 0, labelId = 0;
		int width = bitMap.getWidth();
		int height = bitMap.getHeight();
		RectangleArray<int> labelMap(bitMap.getWidth(), bitMap.getHeight());
		labelMap.setTo(0);

		for (cy = 0; cy < height; ++cy) {
			for (cx = 0, labelId = 0; cx < width; ++cx) {
				if (!bitMap[cy][cx]) {
					if (labelId != 0) { // use pre-pixel label
						labelMap[cy][cx] = labelId;
					} else {
						labelId = labelMap[cy][cx];

						if (labelId == 0) {
							labelId = ++connectedComponentsCount;
							tracingDirection = 0;
							// external contour
							contours.push_back(contourTracing(cy, cx, labelId, tracingDirection, labelMap, bitMap));
							labelMap[cy][cx] = labelId;
						}
					}
				} else if (labelId != 0) { // walkable & pre-pixel has been labeled
					if (labelMap[cy][cx] == 0) {
						tracingDirection = 1;
						// internal contour
						// TODO a polygon can have walkable polygons as "holes", save them
						contourTracing(cy, cx - 1, labelId, tracingDirection, labelMap, bitMap);
					}
					labelId = 0;
				}
			}
		}
// 		labelMap.saveToFile("logs/areaContours.txt");
	}

	// anchor vertices near borders of the map to the border
	// used to fix errors from simplify polygon
	void anchorToBorder(BoostPolygon& polygon, int mapWidth, int mapHeight)
	{
		const int maxX = mapWidth - 1;
		const int maxMarginX = maxX - ANCHOR_MARGIN;
		const int maxY = mapHeight - 1;
		const int maxMarginY = maxY - ANCHOR_MARGIN;
		for (auto& vertex : polygon.outer()) {
			if (vertex.x() <= ANCHOR_MARGIN) vertex.x(0);
			if (vertex.y() <= ANCHOR_MARGIN) vertex.y(0);
			if (vertex.x() >= maxMarginX) vertex.x(maxX);
			if (vertex.y() >= maxMarginY) vertex.y(maxY);
		}
		// after anchoring we simplify again the polygon to remove unnecessary points
		BoostPolygon simPolygon;
		boost::geometry::simplify(polygon, simPolygon, 0.5);
	}


	void generatePolygons(std::vector<Polygon>& polygons)
	{
		Timer timer;
		timer.start();

		std::vector<Contour> contours;
		connectedComponentLabeling(contours, MapData::walkability);

		LOG(" - Component-Labeling Map and Contours extracted in " << timer.stopAndGetTime() << " seconds");
		timer.start();

// 		boost::geometry::model::multi_polygon<BoostPolygon> polygons;
		for (const auto& contour : contours) {
			BoostPolygon polygon, simPolygon;
			boost::geometry::assign_points(polygon, contour);
			// Uses Douglas-Peucker algorithm to simplify points in the polygon
			// https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm
			boost::geometry::simplify(polygon, simPolygon, 1.0);
			anchorToBorder(simPolygon, MapData::walkability.getWidth(), MapData::walkability.getHeight());

			// if polygon isn't too small, add it to the result
			if (boost::geometry::area(simPolygon) > MIN_ARE_POLYGON) {
				// transform BOOST_Polygon to BWTA_Polygon
				Polygon BwtaPolygon;
				for (const auto& pos : simPolygon.outer()) BwtaPolygon.emplace_back((int)pos.x(), (int)pos.y());
				polygons.push_back(BwtaPolygon);
			}
		}

		LOG(" - Vectorized areas in " << timer.stopAndGetTime() << " seconds");
		

		// build tile resolution version
// 		timer.start();
// 		std::vector<Contour> contours2;
// 		connectedComponentLabeling(contours2, MapData::lowResWalkability);
// 
// 		LOG(" - Component-Labeling Map and Contours extracted in " << timer.stopAndGetTime() << " seconds");
// 		timer.start();
// 
// 		boost::geometry::model::multi_polygon<BoostPolygon> polygons2;
// 		for (const auto& contour : contours2) {
// 			BoostPolygon polygon, scalePolygon, simPolygon;
// 			boost::geometry::assign_points(polygon, contour);
// 			boost::geometry::simplify(polygon, simPolygon, 0.5);
// 			boost::geometry::strategy::transform::scale_transformer<double, 2, 2> scale(4.0);
// 			boost::geometry::transform(simPolygon, scalePolygon, scale);
// 			polygons2.push_back(scalePolygon);
// 		}
// 
// 		LOG(" - Vectorized areas in " << timer.stopAndGetTime() << " seconds");

	}
}