#include "DistanceTransform.h"

using namespace std;
using namespace BWAPI;
namespace BWTA
{
	void distanceTransform()
	{
		int mapW = MapData::mapWidth * 4;
		int mapH = MapData::mapHeight * 4;

		bool finish = false;
		int maxDistance = 0;
		while (!finish) {
			finish = true;

			for(int x=0; x < mapW; ++x) {
				for(int y=0; y < mapH; ++y) {
					if (MapData::distanceTransform[x][y] == -1) {
						if		(x>0	  && y>0	  && MapData::distanceTransform[x-1][y-1] == maxDistance) MapData::distanceTransform[x][y] = maxDistance+1;
						else if (			 y>0	  && MapData::distanceTransform[x][y-1] == maxDistance) MapData::distanceTransform[x][y] = maxDistance+1;
						else if (x+1<mapW && y>0	  && MapData::distanceTransform[x+1][y-1] == maxDistance) MapData::distanceTransform[x][y] = maxDistance+1;
						else if (x>0				  && MapData::distanceTransform[x-1][y] == maxDistance) MapData::distanceTransform[x][y] = maxDistance+1;
						else if (x>0				  && MapData::distanceTransform[x+1][y] == maxDistance) MapData::distanceTransform[x][y] = maxDistance+1;
						else if (x>0	  && y+1<mapH && MapData::distanceTransform[x-1][y+1] == maxDistance) MapData::distanceTransform[x][y] = maxDistance+1;
						else if (			 y+1<mapH && MapData::distanceTransform[x][y+1] == maxDistance) MapData::distanceTransform[x][y] = maxDistance+1;
						else if (x+1<mapW && y+1<mapH && MapData::distanceTransform[x+1][y+1] == maxDistance) MapData::distanceTransform[x][y] = maxDistance+1;
					}

					if (MapData::distanceTransform[x][y] == -1) finish = false;
				}
			}

			maxDistance++;
		}
		MapData::maxDistanceTransform = maxDistance;
	}

	int getMaxTransformDistance(int x, int y)
	{
		int maxDistance = 0;
		maxDistance = max(maxDistance, MapData::distanceTransform[x*4][y*4]);
		maxDistance = max(maxDistance, MapData::distanceTransform[(x*4)+1][y*4]);
		maxDistance = max(maxDistance, MapData::distanceTransform[(x*4)+2][y*4]);
		maxDistance = max(maxDistance, MapData::distanceTransform[(x*4)+3][y*4]);

		maxDistance = max(maxDistance, MapData::distanceTransform[x*4][(y*4)+1]);
		maxDistance = max(maxDistance, MapData::distanceTransform[(x*4)+1][(y*4)+1]);
		maxDistance = max(maxDistance, MapData::distanceTransform[(x*4)+2][(y*4)+1]);
		maxDistance = max(maxDistance, MapData::distanceTransform[(x*4)+3][(y*4)+1]);

		maxDistance = max(maxDistance, MapData::distanceTransform[x*4][(y*4)+2]);
		maxDistance = max(maxDistance, MapData::distanceTransform[(x*4)+1][(y*4)+2]);
		maxDistance = max(maxDistance, MapData::distanceTransform[(x*4)+2][(y*4)+2]);
		maxDistance = max(maxDistance, MapData::distanceTransform[(x*4)+3][(y*4)+2]);

		maxDistance = max(maxDistance, MapData::distanceTransform[x*4][(y*4)+3]);
		maxDistance = max(maxDistance, MapData::distanceTransform[(x*4)+1][(y*4)+3]);
		maxDistance = max(maxDistance, MapData::distanceTransform[(x*4)+2][(y*4)+3]);
		maxDistance = max(maxDistance, MapData::distanceTransform[(x*4)+3][(y*4)+3]);
		return maxDistance;
	}

	// get the maximum distance of each region
	// warning: MapData::distanceTransform is in walkable tiles (8x8)
	//			regionMap is in TILE_SIZE (32x32)
	void maxDistanceOfRegion()
	{
		int maxDistance;
		for(int x=0; x < MapData::mapWidth; ++x) {
			for(int y=0; y < MapData::mapHeight; ++y) {
				Region* region = getRegion(x, y);
				if (region != NULL) {
					maxDistance = getMaxTransformDistance(x, y);
					((BWTA::RegionImpl*)(region))->_maxDistance = max(((BWTA::RegionImpl*)(region))->_maxDistance, maxDistance);
				}
			}
		}
	}

	void computeDistanceTransform()
	{
		// compute distance transform map
		distanceTransform();

		// calculate maximum distance of each region
		maxDistanceOfRegion();

#ifdef DEBUG_DRAW
		Painter painter;
		painter.drawHeatMap(MapData::distanceTransform, (float)MapData::maxDistanceTransform);
		painter.drawMapBorder();
		painter.drawPolygons(BWTA_Result::unwalkablePolygons);
		painter.render("DT");
#endif
	}
}