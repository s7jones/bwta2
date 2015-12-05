#include <BWTA.h>
#include <BWTA/RectangleArray.h>

#include "BWTA_Result.h"
#include "BaseLocationImpl.h"
#include "RegionImpl.h"
#include "ChokepointImpl.h"
#include "ConnectedComponent.h"
#include "MapData.h"
#include "Heap.h"

namespace BWTA
{
	template <class _Tp1>
	_Tp1 get_set2(std::map<_Tp1, _Tp1> &a, _Tp1 i)
	{
		if (a.find(i) == a.end()) a[i] = i;
		if (i == a[i]) return i;
		a[i] = get_set2(a, a[i]);
		return a[i];
	}

	void calculate_connectivity()
	{
		// compute reachable region for each region
		std::map<RegionImpl*, RegionImpl*> regionGroup;
		for (auto regionInterface : BWTA_Result::regions) {
			RegionImpl* region1 = (RegionImpl*)regionInterface;
			for (auto chokepointInterface : regionInterface->getChokepoints()) {
				ChokepointImpl* chokepoint = (ChokepointImpl*)chokepointInterface;
				RegionImpl* region2 = (RegionImpl*)(chokepoint->_regions.first);
				if (region1 == region2) {
					region2 = (RegionImpl*)(chokepoint->_regions.second);
				}
				regionGroup[get_set2(regionGroup, region2)] = get_set2(regionGroup, region1);
			}
		}
		
		for (auto regionInterface : BWTA_Result::regions) {
			RegionImpl* region1 = (RegionImpl*)regionInterface;
			region1->reachableRegions.insert(region1);
			for (auto regionInterface2 : BWTA_Result::regions) {
				RegionImpl* region2 = (RegionImpl*)regionInterface2;
				if (region1 == region2) continue;
				if (get_set2(regionGroup, region1) == get_set2(regionGroup, region2)) {
					region1->reachableRegions.insert(region2);
					region2->reachableRegions.insert(region1);
				}
			}
		}

		// compute closestRegion and closestUnwalkablePolygon maps
		for (int x = 0; x < (int)BWTA_Result::getRegion.getWidth(); x++) {
			for (int y = 0; y < (int)BWTA_Result::getRegion.getHeight(); y++) {
				Region* closestRegion = NULL;
				bool inRegion = false;
				Polygon* closestUnwalkablePolygon = NULL;
				bool inPolygon = false;
				double minPolygonDist = -1;
				BWAPI::Position tilePoint = BWAPI::Position(x * TILE_SIZE + 16, y * TILE_SIZE + 16);
				
				for (auto region : BWTA_Result::regions) {
					if (region->getPolygon().isInside(tilePoint)) {
						closestRegion = region;
						inRegion = true;
						break;
					}
				}

				for (auto unwalkablePolygon : BWTA_Result::unwalkablePolygons) {
					if (unwalkablePolygon->isInside(tilePoint)) {
						closestUnwalkablePolygon = unwalkablePolygon;
						inPolygon = true;
						minPolygonDist = 0;
						break;
					} else {
						double dist = unwalkablePolygon->getNearestPoint(tilePoint).getDistance(tilePoint);
						if (minPolygonDist == -1 || dist < minPolygonDist) {
							minPolygonDist = dist;
							closestUnwalkablePolygon = unwalkablePolygon;
						}
					}
				}
				if (inRegion) BWTA_Result::getRegion[x][y] = closestRegion;
				BWTA_Result::getUnwalkablePolygon[x][y] = closestUnwalkablePolygon;
			}
		}

#ifdef OFFLINE
// 		BWTA_Result::getRegion.saveToFile("logs/getRegion.txt");
// 		BWTA_Result::getUnwalkablePolygon.saveToFile("logs/getUnwalkablePolygon.txt");
#endif

		// compute closest base location map
		// TODO SLOW
		RectangleArray<double> minDistanceMap(MapData::mapWidth * 4, MapData::mapHeight * 4);
		minDistanceMap.setTo(-1);
		RectangleArray<double> distanceMap;
		for (auto baseLocation : BWTA_Result::baselocations) {
			getGroundWalkDistanceMap(baseLocation->getTilePosition().x * 4 + 8, 
				baseLocation->getTilePosition().y * 4 + 6, distanceMap);
			for (int x = 0; x < MapData::mapWidth * 4; x++) {
				for (int y = 0; y < MapData::mapHeight * 4; y++) {
					if (distanceMap[x][y] == -1) continue;
					if (minDistanceMap[x][y] == -1 || distanceMap[x][y] < minDistanceMap[x][y]) {
						minDistanceMap[x][y] = distanceMap[x][y];
						BWTA_Result::getBaseLocationW[x][y] = baseLocation;
					}
				}
			}
		}

#ifdef OFFLINE
// 		minDistanceMap.saveToFile("logs/minDistanceMap.txt");
// 		BWTA_Result::getBaseLocationW.saveToFile("logs/getBaseLocationW.txt");
#endif

		for (int x = 0; x < MapData::mapWidth; x++) {
			for (int y = 0; y < MapData::mapHeight; y++) {
				Heap<BaseLocation*, int> h;
				for (int xi = 0; xi < 4; xi++) {
					for (int yi = 0; yi < 4; yi++) {
						BaseLocation* bl = BWTA_Result::getBaseLocationW[x * 4 + xi][y * 4 + yi];
						if (bl == NULL) continue;
						if (h.contains(bl)) {
							int n = h.get(bl) + 1;
							h.set(bl, n);
						} else {
							h.push(std::make_pair(bl, 1));
						}
					}
				}
				if (h.empty() == false) {
					BWTA_Result::getBaseLocation[x][y] = h.top().first;
				}
			}
		}

#ifdef OFFLINE
// 		BWTA_Result::getBaseLocation.saveToFile("logs/getBaseLocation.txt");
#endif

		// compute closest chokepoint map
		// TODO SLOW
		minDistanceMap.setTo(-1);
		for (auto chokepoint : BWTA_Result::chokepoints) {
			getGroundWalkDistanceMap(chokepoint->getCenter().x / 8, chokepoint->getCenter().y / 8, distanceMap);
			for (int x = 0; x < MapData::mapWidth * 4; x++) {
				for (int y = 0; y < MapData::mapHeight * 4; y++) {
					if (distanceMap[x][y] == -1) continue;
					if (minDistanceMap[x][y] == -1 || distanceMap[x][y] < minDistanceMap[x][y]) {
						minDistanceMap[x][y] = distanceMap[x][y];
						BWTA_Result::getChokepointW[x][y] = chokepoint;
					}
				}
			}
		}

#ifdef OFFLINE
// 		BWTA_Result::getChokepointW.saveToFile("logs/getChokepointW.txt");
#endif

		for (int x = 0; x < MapData::mapWidth; x++) {
			for (int y = 0; y < MapData::mapHeight; y++) {
				Heap<Chokepoint*, int> h;
				for (int xi = 0; xi < 4; xi++) {
					for (int yi = 0; yi < 4; yi++) {
						Chokepoint* cp = BWTA_Result::getChokepointW[x * 4 + xi][y * 4 + yi];
						if (cp == NULL) continue;
						if (h.contains(cp)) {
							int n = h.get(cp) + 1;
							h.set(cp, n);
						} else {
							h.push(std::make_pair(cp, 1));
						}
					}
				}
				if (h.empty() == false) {
					BWTA_Result::getChokepoint[x][y] = h.top().first;
				}
			}
		}

#ifdef OFFLINE
// 		BWTA_Result::getChokepoint.saveToFile("logs/getChokepoint.txt");
#endif
	}
}