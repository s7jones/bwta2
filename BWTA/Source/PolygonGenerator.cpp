#include "PolygonGenerator.h"

#include <boost/geometry/extensions/algorithms/dissolve.hpp>

namespace BWTA
{
	struct holeLabel_t {
		Contour ring;
		int labelID;
		holeLabel_t(Contour _ring, int id) : labelID(id), ring(_ring) {}
	};

	void scanLineFill(Contour contour, const int& labelID, RectangleArray<int>& labelMap, RectangleArray<bool>& nodeMap) {
		// Detect nodes for scan-line fill algorithm (avoiding edges)
		contour.pop_back(); // since first and last point are the same
		BoostPoint left = contour.back();
		BoostPoint pos, right;
		size_t last = contour.size() - 1;
		for (size_t i = 0; i < last; ++i) {
			pos = contour.at(i);
			right = contour.at(i + 1);
			if ((left.y() <= pos.y() && right.y() <= pos.y()) ||
				(left.y() > pos.y() && right.y() > pos.y()))
			{ // we have an edge
// 				labelMap[(int)pos.x()][(int)pos.y()] = 9;
			} else { // we have a node
// 				labelMap[(int)pos.x()][(int)pos.y()] = 8;
				nodeMap[(int)pos.x()][(int)pos.y()] = true;
			}
			left = pos;
		}
		// check last element
		pos = contour.back();
		right = contour.front();
		if ((left.y() <= pos.y() && right.y() <= pos.y()) ||
			(left.y() > pos.y() && right.y() > pos.y()))
		{ // we have an edge
// 			labelMap[(int)pos.x()][(int)pos.y()] = 9;
		} else { // we have a node
// 			labelMap[(int)pos.x()][(int)pos.y()] = 8;
			nodeMap[(int)pos.x()][(int)pos.y()] = true;
		}

		// find bounding box of polygon
		size_t maxX = 0;
		size_t minX = labelMap.getWidth();
		size_t maxY = 0;
		size_t minY = labelMap.getHeight();
		for (const auto& pos : contour) {
			maxX = std::max(maxX, (size_t)pos.x());
			minX = std::min(minX, (size_t)pos.x());
			maxY = std::max(maxY, (size_t)pos.y());
			minY = std::min(minY, (size_t)pos.y());
		}

		// iterate though the bounding box using a scan-line algorithm to fill the polygon
		bool toFill;
		for (size_t posY = minY; posY < maxY; ++posY) {
			toFill = false;
			for (size_t posX = minX; posX < maxX; ++posX) {
				if (toFill) labelMap[posX][posY] = labelID;
				if (nodeMap[posX][posY]) toFill = !toFill;
			}
		}
	}

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
			if (x < 0 || x >= height || y < 0 || y >= width) {
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
	void connectedComponentLabeling(std::vector<Contour>& contours, const RectangleArray<bool>& bitMap, RectangleArray<int>& labelMap)
	{
		int cy, cx, tracingDirection, connectedComponentsCount = 0, labelId = 0;
		int width = bitMap.getWidth();
		int height = bitMap.getHeight();
		std::vector<holeLabel_t> holesToLabel;

		for (cy = 0; cy < width; ++cy) {
			for (cx = 0, labelId = 0; cx < height; ++cx) {
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
						Contour hole = contourTracing(cy, cx - 1, labelId, tracingDirection, labelMap, bitMap);
						BoostPolygon polygon;
						boost::geometry::assign_points(polygon, hole);
						// if polygon isn't too small, add it to the result
						if (boost::geometry::area(polygon) > MIN_ARE_POLYGON) {
							// TODO a polygon can have walkable polygons as "holes", save them
							LOG(" - Found big HOLE");
						} else {
							// "remove" the hole filling it with the polygon label
							holesToLabel.emplace_back(hole, labelId);
						}
					}
					labelId = 0;
				}
			}
		}

		RectangleArray<bool> nodeMap(width, height);
		nodeMap.setTo(false);
		for (auto& holeToLabel : holesToLabel) {
			scanLineFill(holeToLabel.ring, holeToLabel.labelID, labelMap, nodeMap);
		}
	}

	// anchor vertices near borders of the map to the border
	// used to fix errors from simplify polygon
	void anchorToBorder(BoostPolygon& polygon, const int maxX, const int maxY, const int maxMarginX, const int maxMarginY)
	{
		bool modified = false;
		for (auto& vertex : polygon.outer()) {
			if (vertex.x() <= ANCHOR_MARGIN) { modified = true; vertex.x(0); }
			if (vertex.y() <= ANCHOR_MARGIN) { modified = true; vertex.y(0); }
			if (vertex.x() >= maxMarginX) { modified = true; vertex.x(maxX); }
			if (vertex.y() >= maxMarginY) { modified = true; vertex.y(maxY); }
		}
		// after anchoring we simplify again the polygon to remove unnecessary points
		if (modified) {
			BoostPolygon simPolygon;
			boost::geometry::simplify(polygon, simPolygon, 1.0);
			polygon = simPolygon;
		}
		
// 		std::vector<BoostPolygon> output;
// 		boost::geometry::dissolve(polygon, output);
// 		if (output.size() != 1) {
// 			LOG("ERROR: polygon simplification generated " << output.size()  << " polygons");
// 		} else {
// 			boost::geometry::simplify(output.at(0), polygon, 1.0);
// 		}
	}


	void generatePolygons(std::vector<BoostPolygon>& polygons, RectangleArray<int>& labelMap)
	{
		Timer timer;
		timer.start();

		std::vector<Contour> contours;
		connectedComponentLabeling(contours, MapData::walkability, labelMap);
// 		labelMap.saveToFile("logs/labelMap.txt");

		LOG(" - Component-Labeling Map and Contours extracted in " << timer.stopAndGetTime() << " seconds");
		timer.start();

		const int maxX = MapData::walkability.getWidth() - 1;
		const int maxY = MapData::walkability.getHeight() - 1;
		const int maxMarginX = maxX - ANCHOR_MARGIN;
		const int maxMarginY = maxY - ANCHOR_MARGIN;

		RectangleArray<bool> nodeMap(labelMap.getWidth(), labelMap.getHeight());
		nodeMap.setTo(false);

// 		boost::geometry::model::multi_polygon<BoostPolygon> polygons;
// 		const auto& contour = contours.at(1);
		for (const auto& contour : contours) {
			BoostPolygon polygon, simPolygon;
			boost::geometry::assign_points(polygon, contour);
			// if polygon isn't too small, add it to the result
			if (boost::geometry::area(polygon) > MIN_ARE_POLYGON) {
				// Uses Douglas-Peucker algorithm to simplify points in the polygon
				// https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm
				boost::geometry::simplify(polygon, simPolygon, 2.0);
// 				LOG(" - Simplified polygon " << boost::geometry::dsv(simPolygon));

				// If the starting-ending points are co-linear, this is a special case that is not simplified
				// http://boost-geometry.203548.n3.nabble.com/Simplifying-polygons-with-co-linear-points-td3415757.html
				// To avoid problems with borders, if the initial point is in the border, we rotate the points
				// until we find one that it is not in the border (or all points explored)
				// Notice that we may still have co-linear points, but hopefully not in the border.
				const auto& p0 = simPolygon.outer().at(0);
				if (p0.x() <= 0 || p0.x() >= maxX || p0.y() <= 0 || p0.y() >= maxY) {
					// find index of not border point
					size_t index = 0;
					for (size_t i = 1; i < simPolygon.outer().size(); ++i) {
						const auto& p1 = simPolygon.outer().at(i);
						if (p1.x() > 0 && p1.x() < maxX && p1.y() > 0 && p1.y() < maxY) {
							// not border point found
							index = i;
							break;
						}
					}
					if (index != 0) {
						auto& outerRing = simPolygon.outer();
						std::rotate(outerRing.begin(), outerRing.begin() + index, outerRing.end());
						outerRing.push_back(outerRing.at(0));
					}
// 					LOG(" - Rotated polygon " << boost::geometry::dsv(simPolygon));
				}

				anchorToBorder(simPolygon, maxX, maxY, maxMarginX, maxMarginY);
// 				LOG(" -   Anchored polygon " << boost::geometry::dsv(simPolygon));

				if (!boost::geometry::is_simple(simPolygon)) {
					LOG("Error, polygon not simple!!!!!!!!!!!!!!");
				}
				std::string message;
				if (!boost::geometry::is_valid(simPolygon)) {
					LOG("Error, polygon not valid!!!!!!!!!!!!!! => " << message);
				}
				// transform BOOST_Polygon to BWTA_Polygon
				polygons.push_back(simPolygon);
// 				Polygon BwtaPolygon;
// 				for (const auto& pos : simPolygon.outer()) BwtaPolygon.emplace_back((int)pos.x(), (int)pos.y());
// 				polygons.push_back(BwtaPolygon);
			} else {
				// region discarded, relabel
// 				const auto& p0 = polygon.outer().at(0);
// 				int labelID = labelMap[(int)p0.x()][(int)p0.y()];
// 				LOG("Discarded obstacle with label : " << labelID << " and area: " << boost::geometry::area(polygon));
				// TODO, still has some inaccuracies....
				scanLineFill(contour, 0, labelMap, nodeMap);
			}
		}

// 		labelMap.saveToFile("logs/labelMap.txt");

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