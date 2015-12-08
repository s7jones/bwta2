#include "WallingGHOST.h"

#include "../../GHOST/include/variables/building.hpp"
#include "../../GHOST/include/domains/wallinDomain.hpp"
#include "../../GHOST/include/constraints/wallinConstraint.hpp"
#include "../../GHOST/include/objectives/wallinObjective.hpp"
#include "../../GHOST/include/misc/wallinTerran.hpp"
#include "../../GHOST/include/solver.hpp"

using namespace ghost;

BWAPI::TilePosition gridOffset;

void wallingGHOST(BWTA::Chokepoint* chokepointToWall, BWTA::Region* prefRegion)
{
	// Generate the grids around ALL chokepoints:
// 	std::ofstream fileTxt("logs/output.txt");
// 	const std::set<BWTA::Chokepoint*> chokePoints = BWTA::getChokepoints();
// 	for (std::set<BWTA::Chokepoint*>::const_iterator c = chokePoints.begin(); c != chokePoints.end(); ++c) {
// 		BWTA::Chokepoint *cp = (*c);
// 		BWTA::RectangleArray<int> chokeGrid = BWTA::getChokeGrid(cp);
// 		// Print grid
// 		BWAPI::TilePosition center = BWAPI::TilePosition(cp->getCenter());
// 		int radius = int(cp->getWidth() / TILE_SIZE);
// 		BWAPI::TilePosition region1 = BWAPI::TilePosition(cp->getRegions().first->getCenter());
// 		BWAPI::TilePosition region2 = BWAPI::TilePosition(cp->getRegions().second->getCenter());
// 		fileTxt << "Chokepoint:\n";
// 		fileTxt << "Region 1: " << radius + region1.x - center.x << "," << radius + region1.y - center.y << std::endl;
// 		fileTxt << "Region 2: " << radius + region2.x - center.x << "," << radius + region2.y - center.y << std::endl;
// 		BWAPI::TilePosition side1 = BWAPI::TilePosition(cp->getSides().first);
// 		BWAPI::TilePosition side2 = BWAPI::TilePosition(cp->getSides().second);
// 		int s1x = radius + side1.x - center.x;
// 		int s1y = radius + side1.y - center.y;
// // 		fileTxt << "Side 1: " << s1x << "," << s1y << " is " << chokeGrid[s1x][s1y] << std::endl;
// 		int s2x = radius + side2.x - center.x;
// 		int s2y = radius + side2.y - center.y;
// // 		fileTxt << "Side 2: " << s2x << "," << s2y << " is " << chokeGrid[s2x][s2y] << std::endl;
// 		BWTA::generateWallStartingPoints(chokeGrid, s1x, s1y, s2x, s2y, &fileTxt);
// 		for (unsigned int y = 0; y < chokeGrid.getHeight(); ++y) {
// 			for (unsigned int x = 0; x < chokeGrid.getWidth(); ++x) {
// 				fileTxt << chokeGrid[x][y];
// 			}
// 			fileTxt << std::endl;
// 		}
// 	}
// 	fileTxt.close();

	std::ofstream fileTxt("logs/output.txt");
	fileTxt << "Chokepoint:" << std::endl;
	BWAPI::TilePosition center(chokepointToWall->getCenter());
	fileTxt << "Center: " << center << std::endl;
	
	int radius = BWTA::getRadius(chokepointToWall);
	BWTA::RectangleArray<int> chokeGrid = BWTA::getChokeGrid(center, radius);
	BWAPI::TilePosition side1(chokepointToWall->getSides().first);
	BWAPI::TilePosition side2(chokepointToWall->getSides().second);
	// transform BwPosition to gridPosition
	gridOffset = BWAPI::TilePosition(center.x - radius, center.y - radius);
	BWAPI::TilePosition gridSide1(side1 - gridOffset);
	BWAPI::TilePosition gridSide2(side2 - gridOffset);
	fileTxt << "Side 1: " << side1 << " - " << gridSide1 << std::endl;
	fileTxt << "Side 2: " << side2 << " - " << gridSide2 << std::endl;
	
	ExtremePoints extremes = BWTA::getWallExtremePoints(chokeGrid, gridSide1, gridSide2, prefRegion);
	fileTxt << "wall from " << extremes.side1 << " to " << extremes.side2 << std::endl;
	chokeGrid.saveToFile(fileTxt);
	fileTxt.close();

	// GHOST solver
	// ========================

	// list of unbuildable tiles
	// now read the map:
	std::vector< std::pair<int, int> > unbuildables;
	const int MAX_X = static_cast<int>(chokeGrid.getWidth());
	const int MAX_Y = static_cast<int>(chokeGrid.getHeight());
	for (int x = 0; x < MAX_X; ++x) {
		for (int y = 0; y < MAX_Y; ++y) {
			if (chokeGrid[x][y] != 2) unbuildables.push_back(std::pair<int, int>(x, y));
		}
	}

	std::vector<Building> vec = makeTerranBuildings();
	WallinDomain domain(MAX_X, MAX_Y, unbuildables, &vec, extremes.side1.x, extremes.side1.y, extremes.side2.x, extremes.side2.y);
	std::vector< shared_ptr<WallinConstraint> > vecConstraints = makeTerranConstraints(&vec, &domain);

	std::cout << "map size: " << MAX_X << "," << MAX_Y << std::endl;
	std::cout << "calling solver..." << std::endl;

	shared_ptr<WallinObjective> objective = make_shared<GapObj>();
	Solver<Building, WallinDomain, WallinConstraint> solver(&vec, &domain, vecConstraints, objective);

	const int timeLimit = 80;
	solver.solve(timeLimit, 160);

	std::cout << domain << std::endl;
}


namespace BWTA
{
	/*
	Given a center and a offset, it returns the buildability grid around the center
	- 0 is wall
	- 1 is walkable (but not buildable)
	- 2 is walkable and buildable
	- 4 out of the map
	*/
	RectangleArray<int> getChokeGrid(BWAPI::TilePosition center, int offset)
	{
		RectangleArray<int> grid;
		unsigned int gridSize = (offset * 2) + 1;
		int BUILD_TO_WALK_TILE = 4;
		grid.resize(gridSize, gridSize);
// 		std::cout << BWTA::MapData::walkability.getWidth() << "," << BWTA::MapData::buildability.getWidth() << std::endl;
		for (unsigned int y = 0; y < gridSize; ++y) {
			unsigned int ay = center.y - offset + y;
			for (unsigned int x = 0; x < gridSize; ++x) {
				unsigned int ax = center.x - offset + x;
				if (ax >= 0 && ay >= 0 &&
					ax < MapData::buildability.getWidth() &&
					ay < MapData::buildability.getHeight()) {
					grid[x][y] = MapData::walkability[ax*BUILD_TO_WALK_TILE][ay*BUILD_TO_WALK_TILE] + MapData::buildability[ax][ay];
				} else {
					grid[x][y] = 4;
				}
			}
		}
		return grid;
	}

	/*
	Given a chokepoint, it returns the radius around it where the wall should be within
	*/
	int getRadius(Chokepoint* chokepoint)
	{
		BWAPI::TilePosition center(chokepoint->getCenter());
		bool isRamp = !MapData::buildability[center.x][center.y];
		// if it's a ramp we need to check the borders TODO!!!
		if (!MapData::buildability[center.x][center.y]) {
			return static_cast<int>(chokepoint->getWidth() / TILE_SIZE) * 3;
		} else { // else, we can just use the width of the chokepoint
			return static_cast<int>(chokepoint->getWidth() / TILE_SIZE);
		}
	}

	/*
	Uses Bresenham from x,y towards dirX,dirY until finds a buildable tile
	*/
	void extendLineUntilBuildableTile(const BWTA::RectangleArray<int>& chokeGrid, 
		int x, int y, int dirX, int dirY, int dx, int dy, int& resx, int& resy)
	{
		const int BUILDABLE_TILE = 2;
		const int MAX_X = static_cast<int>(chokeGrid.getWidth());
		const int MAX_Y = static_cast<int>(chokeGrid.getHeight());

		int slopeX = 0;
		if (x < dirX) slopeX = 1;
		else slopeX = -1;
		
		int slopeY = 0;
		if (y < dirY) slopeY = 1;
		else slopeY = -1;

		int err = dx - dy;
// 		std::cout << "line " << x << "," << y << " to " << dirX << "," << dirY << std::endl;
		
		for(;;) {
// 			std::cout << x << "," << y << std::endl;
			if (chokeGrid[x][y] == BUILDABLE_TILE) {
				resx = x;
				resy = y;
				break;
			}
			int e2 = 2 * err;
			if (e2 > -dy) {
				err -= dy;
				x += slopeX;
			}
			if (e2 < dx) {
				err += dx;
				y += slopeY;
			}
			if (x < 0 || x >= MAX_X) break;
			if (y < 0 || y >= MAX_Y) break;
		}
	}

	/*
	This function does the following:
	- finds the center point of s1x,s1y and s2x,s2y
	- then it computes a line starting at the center point, and that crosses both points
	- it extends this line until the tile type is buildable
	- it returns the two extremes
	Warning: you are on charge to delete the output
	*/
	int* findExtremes(BWTA::RectangleArray<int> chokeGrid, BWAPI::TilePosition s1, BWAPI::TilePosition s2)
	{
// 		std::cout << "Sides: " << s1 << " - " << s2 << std::endl;
		BWAPI::TilePosition center((s1 + s2) / 2);
		int dx = abs(s2.x - s1.x);
		int dy = abs(s2.y - s1.y);
		int *edges = new int[4];
		edges[0] = -1;
		edges[1] = -1;
		edges[2] = -1;
		edges[3] = -1;

		// Do Bresenham towards each side until a non buildable is found:
		extendLineUntilBuildableTile(chokeGrid, center.x, center.y, s1.x, s1.y, dx, dy, edges[0], edges[1]);
		extendLineUntilBuildableTile(chokeGrid, center.x, center.y, s2.x, s2.y, dx, dy, edges[2], edges[3]);

		return edges;
	}

	RectangleArray<int> floodFill(RectangleArray<int> chokeGrid, int x, int y) {
		const int w = chokeGrid.getWidth();
		const int h = chokeGrid.getHeight();
		RectangleArray<int> result;
		result.resize(w, h);
		result.setTo(0);

		std::stack<std::pair<int, int>> stack;
		int type = chokeGrid[x][y];
		int offx[4] = { -1, 0, 1, 0 };
		int offy[4] = { 0, -1, 0, 1 };

		stack.push(std::pair<int, int>(x, y));
		while (!stack.empty()) {
			std::pair<int, int> current = stack.top();
			stack.pop();
			if (chokeGrid[current.first][current.second] == type) {
				result[current.first][current.second] = 1;
				for (int i = 0; i < 4; i++) {
					std::pair<int, int> next(current.first + offx[i], current.second + offy[i]);
					if (next.first >= 0 && next.first < w &&
						next.second >= 0 && next.second < h &&
						result[next.first][next.second] == 0) {
						stack.push(next);
					}
				}
			}
		}
		return result;
	}

	BWAPI::TilePosition rotateAroundPoint(BWAPI::TilePosition point, BWAPI::TilePosition pivot)
	{
		return BWAPI::TilePosition(-point.y + pivot.x + pivot.y, point.x - pivot.x + pivot.y);
	}

	ExtremePoints getFarthestBorderPositions(const RectangleArray<int>& chokeGrid, BWAPI::TilePosition center, BWAPI::TilePosition edge)
	{
		ExtremePoints extremes;
		const int w = chokeGrid.getWidth();
		const int h = chokeGrid.getHeight();

		RectangleArray<int> fillCenter = floodFill(chokeGrid, center.x, center.y);
		RectangleArray<int> fillOutside = floodFill(chokeGrid, edge.x, edge.y);
		int offx[4] = { -1, 0, 1, 0 };
		int offy[4] = { 0, -1, 0, 1 };
		std::vector<std::pair<int, int>> border;

		// find the list of points in "fillOutside" that touch a point in "fillCenter":
		double borderCenterX = 0;
		double borderCenterY = 0;
		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				if (fillOutside[j][i] == 1) {
					for (int k = 0; k < 4; k++) {
						int x = j + offx[k];
						int y = i + offy[k];
						if (x >= 0 && x < w && y >= 0 && y < h && fillCenter[x][y]) {
							// border point:
							border.push_back(std::pair<int, int>(j, i));
							borderCenterX += j;
							borderCenterY += i;
							break;
						}
					}
				}
			}
		}
		borderCenterX /= border.size();
		borderCenterY /= border.size();

		// find the two extremes of that list (right now we find the two points furthest form the center,
		// which might not work for all situations, but it's a simple solution):
		int best1 = -1;
		double bestd = 0;
		for (unsigned int i = 0; i<border.size(); ++i) {
			std::pair<int, int> p = border[i];
			double d = sqrt((borderCenterX - p.first)*(borderCenterX - p.first) + (borderCenterY - p.second)*(borderCenterY - p.second));
			if (best1 == -1 || d>bestd) {
				best1 = i;
				bestd = d;
			}
		}
		if (best1 != -1) {
			int best2 = -1;
			bestd = 0;
			for (unsigned int i = 0; i<border.size(); ++i) {
				std::pair<int, int> p = border[i];
				double d = sqrt(double((border[best1].first - p.first)*(border[best1].first - p.first) + (border[best1].second - p.second)*(border[best1].second - p.second)));
				if (best2 == -1 || d>bestd) {
					best2 = i;
					bestd = d;
				}
			}
			extremes.side1 = BWAPI::TilePosition(border[best1].first, border[best1].second);
			extremes.side2 = BWAPI::TilePosition(border[best2].first, border[best2].second);
		}
		return extremes;
	}

	/*
	Generate the starting and end points of a wall for those chokepoints that have the middle tile as "walkable"
	(i.e. those that are on a ramp)
	*/
	ExtremePoints getWallExtremePointsRamp(RectangleArray<int> chokeGrid, BWAPI::TilePosition s1, BWAPI::TilePosition s2, BWTA::Region* prefRegion)
	{
		ExtremePoints extremes;
		BWAPI::TilePosition center((s1 + s2) / 2);
		BWAPI::TilePosition r1 = rotateAroundPoint(s1, center);
		BWAPI::TilePosition r2 = rotateAroundPoint(s2, center);

		// draw a line perpendicular to the chokepoint sides, until we get out of the flood filled area in both directions:
		int *edges = findExtremes(chokeGrid, r1, r2);
		ExtremePoints rampExtremes(edges[0], edges[1], edges[2], edges[3]);
		delete[]edges;
// 		std::cout << "extremes of ramp: " << rampExtremes.side1 << " and " << rampExtremes.side2 << std::endl;

		// We only need one wall (the one in prefRegion)
		if (rampExtremes.side1.isValid() && BWTA::getRegion(rampExtremes.side1 + gridOffset) == prefRegion) {
			extremes = getFarthestBorderPositions(chokeGrid, center, rampExtremes.side1);
		} else if (rampExtremes.side2.isValid()) {
			extremes = getFarthestBorderPositions(chokeGrid, center, rampExtremes.side2);
		}

		return extremes;
	}



	ExtremePoints getWallExtremePoints(RectangleArray<int> chokeGrid, BWAPI::TilePosition s1, BWAPI::TilePosition s2, BWTA::Region* prefRegion)
	{
		ExtremePoints extremes;
		BWAPI::TilePosition center((s1 + s2) / 2);
		int centerType = chokeGrid[center.x][center.y];

		if (centerType == 2) { // No ramp
			int *edges = findExtremes(chokeGrid, s1, s2);
			extremes = ExtremePoints(edges[0], edges[1], edges[2], edges[3]);
			delete[]edges;
		}
		if (centerType == 1) { // Ramp
			extremes = getWallExtremePointsRamp(chokeGrid, s1, s2, prefRegion);
		}

		return extremes;
	}
}