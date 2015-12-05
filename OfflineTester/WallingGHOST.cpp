#include "WallingGHOST.h"

// BWTA::BaseLocation* homeBase;
// BWTA::Region* homeRegion;

void wallingGHOST(BWTA::Chokepoint* chokepointToWall)
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
	fileTxt << "Center: " << center.x << "," << center.y << std::endl;
	
	int radius = BWTA::getRadius(chokepointToWall);
	BWTA::RectangleArray<int> chokeGrid = BWTA::getChokeGrid(center, radius);
	BWAPI::TilePosition side1(chokepointToWall->getSides().first);
	BWAPI::TilePosition side2(chokepointToWall->getSides().second);
	int s1x = radius + side1.x - center.x;
	int s1y = radius + side1.y - center.y;
	int s2x = radius + side2.x - center.x;
	int s2y = radius + side2.y - center.y;
	fileTxt << "Side 1: " << side1.x << "," << side1.y << " - " << s1x << "," << s1y << std::endl;
	fileTxt << "Side 2: " << side2.x << "," << side2.y << " - " << s2x << "," << s2y << std::endl;
	
	BWTA::generateWallStartingPoints(chokeGrid, s1x, s1y, s2x, s2y, &fileTxt);
	chokeGrid.saveToFile(fileTxt);
	fileTxt.close();
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
		//TODO we don't check offset out of range!!
		for (unsigned int y = 0; y < gridSize; ++y) {
			unsigned int ay = center.y - offset + y;
			for (unsigned int x = 0; x < gridSize; ++x) {
				unsigned int ax = center.x - offset + x;
				if (ax >= 0 && ay >= 0 &&
					ax < MapData::buildability.getWidth() &&
					ay < MapData::buildability.getHeight()) {
					grid[x][y] = MapData::walkability[(center.x - offset + x)*BUILD_TO_WALK_TILE]
						[(center.y - offset + y)*BUILD_TO_WALK_TILE]
					+
						MapData::buildability[center.x - offset + x]
						[center.y - offset + y];
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
	int* findExtremes(BWTA::RectangleArray<int> chokeGrid, int s1x, int s1y, int s2x, int s2y)
	{
		std::cout << "Sides: " << s1x << "," << s1y << " - " << s2x << "," << s2y << std::endl;
		int centerx = (s1x + s2x) / 2;
		int centery = (s1y + s2y) / 2;
		int dx = abs(s2x - s1x);
		int dy = abs(s2y - s1y);
		int *edges = new int[4];
		edges[0] = -1;
		edges[1] = -1;
		edges[2] = -1;
		edges[3] = -1;

		// Do Bresenham towards each side until a non buildable is found:
		extendLineUntilBuildableTile(chokeGrid, centerx, centery, s1x, s1y, dx, dy, edges[0], edges[1]);
		extendLineUntilBuildableTile(chokeGrid, centerx, centery, s2x, s2y, dx, dy, edges[2], edges[3]);

		return edges;
	}


	/*
	Generate the starting and end points of a wall for those chokepoints that have the middle tile as "buildable"
	(i.e. those that are not on a ramp)
	*/
	void generateWallStartingPointsNoRamp(BWTA::RectangleArray<int> chokeGrid, int s1x, int s1y, int s2x, int s2y, std::ofstream *out)
	{
		std::cout << "generateWallStartingPointsRamp" << std::endl;
		int *edges = findExtremes(chokeGrid, s1x, s1y, s2x, s2y);
		*out << "wall from " << edges[0] << "," << edges[1] << " to " << edges[2] << "," << edges[3] << std::endl;
		delete[]edges;
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

	void rotateAroundPoint(int pointX, int pointY, int pivotX, int pivotY, int& resX, int &resY)
	{
		resX = -pointY + pivotX + pivotY;
		resY = pointX - pivotX + pivotY;
	}

	void findFarthestBorderPositions(const RectangleArray<int>& chokeGrid, int centerX, int centerY, int edgeX, int edgeY, std::ofstream& out)
	{
		const int w = chokeGrid.getWidth();
		const int h = chokeGrid.getHeight();

		RectangleArray<int> fillCenter = floodFill(chokeGrid, centerX, centerY);
		RectangleArray<int> fillOutside = floodFill(chokeGrid, edgeX, edgeY);
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
			out << "wall from " << border[best1].first << "," << border[best1].second << " to " <<
				border[best2].first << "," << border[best2].second << std::endl;
		}
	}

	/*
	Generate the starting and end points of a wall for those chokepoints that have the middle tile as "walkable"
	(i.e. those that are on a ramp)
	*/
	void generateWallStartingPointsRamp(RectangleArray<int> chokeGrid, int s1x, int s1y, int s2x, int s2y, std::ofstream *out)
	{
		std::cout << "generateWallStartingPointsRamp" << std::endl;
		int centerx = (s1x + s2x) / 2;
		int centery = (s1y + s2y) / 2;
		int r1x, r1y, r2x, r2y;
		rotateAroundPoint(s1x, s1y, centerx, centery, r1x, r1y);
		rotateAroundPoint(s2x, s2y, centerx, centery, r2x, r2y);

		// draw a line perpendicular to the chokepoint sides, until we get out of the flood filled area in both directions:
		int *edges = findExtremes(chokeGrid, r1x, r1y, r2x, r2y);
		std::cout << "extremes of ramp: " << edges[0] << "," << edges[1] << " and " << edges[2] << "," << edges[3] << std::endl;

		// TODO in fact we only need one wall (the closest to the base?)
		if (edges[0] != -1 && edges[1] != -1) {
			findFarthestBorderPositions(chokeGrid, centerx, centery, edges[0], edges[1], *out);
		}
		if (edges[2] != -1 && edges[3] != -1) {
			findFarthestBorderPositions(chokeGrid, centerx, centery, edges[2], edges[3], *out);
		}

		delete[]edges;
	}



	void generateWallStartingPoints(RectangleArray<int> chokeGrid, int s1x, int s1y, int s2x, int s2y, std::ofstream *out)
	{
		int centerx = (s1x + s2x) / 2;
		int centery = (s1y + s2y) / 2;
		int centerType = chokeGrid[centerx][centery];

		if (centerType == 2) generateWallStartingPointsNoRamp(chokeGrid, s1x, s1y, s2x, s2y, out);
		if (centerType == 1) generateWallStartingPointsRamp(chokeGrid, s1x, s1y, s2x, s2y, out);
	}
}