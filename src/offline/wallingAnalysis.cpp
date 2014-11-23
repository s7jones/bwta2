#include <stack>

#include "../MapData.h"


/*
	This function does the following:
	- finds the center point of s1x,s1y and s2x,s2y
	- then it computes a line starting at the center point, and that crosses both points
	- it extends this line until the tile type is different from that at the center
	- it returns the two extremes
	- if previous == true, it returns the edges right before changing tile type
	- if previous == false, it returns the edges right after changing tile type
*/
int *findExtremes(BWTA::RectangleArray<int> chokeGrid, int s1x, int s1y, int s2x, int s2y, bool previous) 
{
	int centerx = (s1x + s2x)/2;
	int centery = (s1y + s2y)/2;
	int dx = abs(s2x - s1x);
	int dy = abs(s2y - s1y);
	int sx = 0;
	int sy = 0;
	int *edges = new int[4];
	int type = chokeGrid[centerx][centery];
	edges[0] = -1;
	edges[1] = -1;
	edges[2] = -1;
	edges[3] = -1;

	// Do Bresenham towards each side until a non buildable is found:
	{
		int x = centerx;
		int y = centery;
		int oldx = centerx;
		int oldy = centery;
		if (centerx < s1x) sx = 1;
			  		  else sx = -1;
		if (centery < s1y) sy = 1;
			  		  else sy = -1;
		int err = dx - dy;
//		std::cout << "line " << centerx << "," << centery << " to " << s1x << "," << s1y << std::endl;
		do {
//			std::cout << x << "," << y << std::endl;
			// check for a wall
			if (chokeGrid[x][y]!=type) {
				// extreme 1 found:
				if (previous) {
					edges[0] = oldx;
					edges[1] = oldy;
				} else {
					edges[0] = x;
					edges[1] = y;
				}
				break;
			}
			oldx = x;
			oldy = y;
//			if (x==s1x && y==s1y) break;
			int e2 = 2*err;
			if (e2 > -dy) {
				err -= dy;
				x+=sx;
			}
			if (e2<dx) {
				err += dx;
				y+=sy;
			}
			if (x<0 || x>=(int)chokeGrid.getWidth()) break;
			if (y<0 || y>=(int)chokeGrid.getHeight()) break;
		} while(true);
	}
	{
		int x = centerx;
		int y = centery;
		int oldx = centerx;
		int oldy = centery;
		if (centerx < s2x) sx = 1;
			  		  else sx = -1;
		if (centery < s2y) sy = 1;
			  		  else sy = -1;
		int err = dx - dy;
		do {
			// check for a wall
			if (chokeGrid[x][y]!=type) {
				// extreme 2 found:
				if (previous) {
					edges[2] = oldx;
					edges[3] = oldy;
				} else {
					edges[2] = x;
					edges[3] = y;
				}
				break;
			}
			oldx = x;
			oldy = y;
//			if (x==s2x && y==s2y) break;
			int e2 = 2*err;
			if (e2 > -dy) {
				err -= dy;
				x+=sx;
			}
			if (e2<dx) {
				err += dx;
				y+=sy;
			}
			if (x<0 || x>=(int)chokeGrid.getWidth()) break;
			if (y<0 || y>=(int)chokeGrid.getHeight()) break;
		} while(true);
	}
	return edges;
}



/*
	Generate the starting and end points of a wall for those chokepoints that have the middle tile as "buildable"
	(i.e. those that are not on a ramp)
*/
void generateWallStartingPointsNoRamp(BWTA::RectangleArray<int> chokeGrid, int s1x, int s1y, int s2x, int s2y, std::ofstream *out) 
{
	int *edges = findExtremes(chokeGrid,s1x,s1y,s2x,s2y,true);
	*out << "wall from " << edges[0] << "," << edges[1] << " to " << edges[2] << "," << edges[3] << std::endl;
	delete []edges;
}



BWTA::RectangleArray<int> floodFill(BWTA::RectangleArray<int> chokeGrid, int x, int y) {
	int w = chokeGrid.getWidth();
	int h = chokeGrid.getHeight();
	BWTA::RectangleArray<int> result;
	result.resize(w,h);
	for(int i = 0;i<h;i++) 
		for(int j = 0;j<w;j++) result[j][i] = 0;
	std::stack<std::pair<int,int>> stack;
	int type = chokeGrid[x][y];
	int offx[4] = {-1, 0,1,0};
	int offy[4] = { 0,-1,0,1};

	stack.push(std::pair<int,int>(x,y));
	while(stack.size()!=0) {
		std::pair<int,int> current = stack.top();
		stack.pop();
		if (chokeGrid[current.first][current.second]==type) {
			result[current.first][current.second] = 1;
			for(int i = 0;i<4;i++) {
				std::pair<int,int> next(current.first+offx[i],current.second+offy[i]);
				if (next.first>=0 && next.first<w &&
					next.second>=0 && next.second<h &&
					result[next.first][next.second]==0) {
					stack.push(next);
				}
			}
		}
	}
	return result;
}



/*
	Generate the starting and end points of a wall for those chokepoints that have the middle tile as "walkable"
	(i.e. those that are on a ramp)
*/
void generateWallStartingPointsRamp(BWTA::RectangleArray<int> chokeGrid, int s1x, int s1y, int s2x, int s2y, std::ofstream *out)
{
	int w = chokeGrid.getWidth();
	int h = chokeGrid.getHeight();
	int centerx = (s1x + s2x)/2;
	int centery = (s1y + s2y)/2;
	int dx = abs(s2x - s1x);
	int dy = abs(s2y - s1y);
	int rotated_dx = -dy;
	int rotated_dy = dx;

	// draw a line perpendicular to the chokepoint sides, until we get out of the floodfilled area in both directions:
	int *edges = findExtremes(chokeGrid,centerx-rotated_dx, centery-rotated_dy,
										centerx+rotated_dx, centery+rotated_dy,false);
//	*out << "rotated edges are " << edges[0] << "," << edges[1] << " to " << edges[2] << "," << edges[3] << std::endl;

	if (edges[0]!=-1 && edges[1]!=-1) {
		// First direction:
//		(*out) << "first direction" << std::endl;
		BWTA::RectangleArray<int> fillCenter = floodFill(chokeGrid,centerx,centery);
		BWTA::RectangleArray<int> fillOutside = floodFill(chokeGrid,edges[0],edges[1]);
		int offx[4] = {-1, 0,1,0};
		int offy[4] = { 0,-1,0,1};
		std::vector<std::pair<int,int>> border;

		// find the list of points in "fillOutside" that touch a point in "fillCenter":
		double borderCenterX = 0;
		double borderCenterY = 0;
		for(int i = 0;i<h;i++) {
			for(int j = 0;j<w;j++) {
				if (fillOutside[j][i]==1) {
					for(int k = 0;k<4;k++) {
						int x = j+offx[k];
						int y = i+offy[k];
						if (x>=0 && x<w && y>=0 && y<h && fillCenter[x][y]) {
							// border point:
							border.push_back(std::pair<int,int>(j,i));
							borderCenterX+=j;
							borderCenterY+=i;
//							(*out) << j << "," << i << " : " << chokeGrid[j][i] << std::endl;
							break;
						}
					}
				}
			}
		}
		borderCenterX/=border.size();
		borderCenterY/=border.size();

		// find the two extremes of that list (right now we find the two points furthest form the center,
		// which might not work for all situations, but it's a simple solution):
		int best1 = -1;
		double bestd = 0;
		for(unsigned int i = 0;i<border.size();i++) {
			std::pair<int,int> p = border[i];
			double d = sqrt((borderCenterX-p.first)*(borderCenterX-p.first)+(borderCenterY-p.second)*(borderCenterY-p.second));
			if (best1==-1 || d>bestd) {
				best1 = i;
				bestd = d;
			}
		}
		if (best1!=-1) {
			int best2 = -1;
			bestd = 0;
			for(unsigned int i = 0;i<border.size();i++) {
				std::pair<int,int> p = border[i];
				double d = sqrt(double((border[best1].first-p.first)*(border[best1].first-p.first)+(border[best1].second-p.second)*(border[best1].second-p.second)));
				if (best2==-1 || d>bestd) {
					best2 = i;
					bestd = d;
				}
			}
			(*out) << "wall from " << border[best1].first << "," << border[best1].second << " to " <<
									  border[best2].first << "," << border[best2].second << std::endl;
		}
	}

	if (edges[2]!=-1 && edges[3]!=-1) {
		// First direction:
//		(*out) << "second direction" << std::endl;
		BWTA::RectangleArray<int> fillCenter = floodFill(chokeGrid,centerx,centery);
		BWTA::RectangleArray<int> fillOutside = floodFill(chokeGrid,edges[2],edges[3]);
		int offx[4] = {-1, 0,1,0};
		int offy[4] = { 0,-1,0,1};
		std::vector<std::pair<int,int>> border;

		// find the list of points in "fillOutside" that touch a point in "fillCenter":
		double borderCenterX = 0;
		double borderCenterY = 0;
		for(int i = 0;i<h;i++) {
			for(int j = 0;j<w;j++) {
				if (fillOutside[j][i]==1) {
					for(int k = 0;k<4;k++) {
						int x = j+offx[k];
						int y = i+offy[k];
						if (x>=0 && x<w && y>=0 && y<h && fillCenter[x][y]) {
							// border point:
							border.push_back(std::pair<int,int>(j,i));
							borderCenterX+=j;
							borderCenterY+=i;
//							(*out) << j << "," << i << " : " << chokeGrid[j][i] << std::endl;
							break;
						}
					}
				}
			}
		}
		borderCenterX/=border.size();
		borderCenterY/=border.size();

		// find the two extremes of that list (right now we find the two points furthest form the center,
		// which might not work for all situations, but it's a simple solution):
		int best1 = -1;
		double bestd = 0;
		for(unsigned int i = 0;i<border.size();i++) {
			std::pair<int,int> p = border[i];
			double d = sqrt((borderCenterX-p.first)*(borderCenterX-p.first)+(borderCenterY-p.second)*(borderCenterY-p.second));
			if (best1==-1 || d>bestd) {
				best1 = i;
				bestd = d;
			}
		}
		if (best1!=-1) {
			int best2 = -1;
			bestd = 0;
			for(unsigned int i = 0;i<border.size();i++) {
				std::pair<int,int> p = border[i];
				double d = sqrt(double((border[best1].first-p.first)*(border[best1].first-p.first)+(border[best1].second-p.second)*(border[best1].second-p.second)));
				if (best2==-1 || d>bestd) {
					best2 = i;
					bestd = d;
				}
			}
			(*out) << "wall from " << border[best1].first << "," << border[best1].second << " to " <<
									  border[best2].first << "," << border[best2].second << std::endl;
		}
	}

	delete []edges;
}



void generateWallStartingPoints(BWTA::RectangleArray<int> chokeGrid, int s1x, int s1y, int s2x, int s2y, std::ofstream *out) 
{
	int centerx = (s1x + s2x)/2;
	int centery = (s1y + s2y)/2;
	int centerType = chokeGrid[centerx][centery];

	if (centerType==2) generateWallStartingPointsNoRamp(chokeGrid, s1x, s1y, s2x, s2y, out);
	if (centerType==1) generateWallStartingPointsRamp(chokeGrid, s1x, s1y, s2x, s2y, out);
}

