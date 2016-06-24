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

	template<typename T>
	struct objectDistance_t {
		int x;
		int y;
		T* objectRef;
		int distance;

		objectDistance_t(int xTmp, int yTmp, T* ref, int dis = 0)
			: x(xTmp), y(yTmp), objectRef(ref), distance(dis) {};
	};

	using baseDistance_t = objectDistance_t < BaseLocation > ;
	using chokeDistance_t = objectDistance_t < Chokepoint > ;

	template<typename T>
	inline void addToExplore(const int& x, const int& y, const objectDistance_t<T>& current, std::queue<objectDistance_t<T>>& toExplore)
	{
		// if change from walkable to not walkable, increase distance cost
		if (MapData::walkability[current.x][current.y] && !MapData::walkability[x][y]) {
			toExplore.emplace(x, y, current.objectRef, current.distance + 100000);
		} else {
			toExplore.emplace(x, y, current.objectRef, current.distance);
		}
	}

	template<typename T>
	void computeClosestObjectMap(std::queue<objectDistance_t<T>> seedPositions, RectangleArray<T*>& closestObjectMap) {
		RectangleArray<double> minDistanceMap(MapData::mapWidthWalkRes, MapData::mapHeightWalkRes);
		minDistanceMap.setTo(-1);
		std::queue<objectDistance_t<T>> postionsToExplore(seedPositions);
		while (!postionsToExplore.empty()) {
			// pop first element
			objectDistance_t<T> check = postionsToExplore.front();
			postionsToExplore.pop();

			if (minDistanceMap[check.x][check.y] == -1 || minDistanceMap[check.x][check.y] > check.distance) {
				closestObjectMap[check.x][check.y] = check.objectRef;
				minDistanceMap[check.x][check.y] = check.distance;
				// look if the 8-connectivity are valid positions
				check.distance += 8; // straight move cost
				int left = check.x - 1;
				if (left >= 0) addToExplore(left, check.y, check, postionsToExplore);
				int right = check.x + 1;
				if (right < MapData::mapWidthWalkRes) addToExplore(right, check.y, check, postionsToExplore);
				int up = check.y - 1;
				if (up >= 0) addToExplore(check.x, up, check, postionsToExplore);
				int down = check.y + 1;
				if (down < MapData::mapHeightWalkRes) addToExplore(check.x, down, check, postionsToExplore);
				check.distance += 3; // increment on diagonal move cost
				if (left >= 0 && up >= 0) addToExplore(left, up, check, postionsToExplore);
				if (left >= 0 && down < MapData::mapHeightWalkRes) addToExplore(left, down, check, postionsToExplore);
				if (right < MapData::mapWidthWalkRes && up >= 0) addToExplore(right, up, check, postionsToExplore);
				if (right < MapData::mapWidthWalkRes && down < MapData::mapHeightWalkRes) {
					addToExplore(right, down, check, postionsToExplore);
				}
			}
		}
	}

	template<typename T>
	void walkResMapToTileResMap(const RectangleArray<T*>& walkResMap, RectangleArray<T*>& tileResMap)
	{
		for (int x = 0; x < MapData::mapWidthTileRes; ++x) {
			for (int y = 0; y < MapData::mapHeightTileRes; ++y) {
				Heap<T*, int> h;
				for (int xi = 0; xi < 4; ++xi) {
					for (int yi = 0; yi < 4; ++yi) {
						T* bl = walkResMap[x * 4 + xi][y * 4 + yi];
						if (bl == NULL) continue;
						if (h.contains(bl)) {
							int n = h.get(bl) + 1;
							h.set(bl, n);
						} else {
							h.push(std::make_pair(bl, 1));
						}
					}
				}
				if (!h.empty()) {
					tileResMap[x][y] = h.top().first;
				}
			}
		}
	}

	void calculate_connectivity()
	{
		Timer timer;
		timer.start();

		// compute reachable region for each region
		// ===========================================================================
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

		LOG(" - Reachable regions computed in " << timer.stopAndGetTime() << " seconds");
		timer.start();

		// compute region and closestUnwalkablePolygon maps
		// ===========================================================================
		// TODO this can be optimized if we know a point inside each polygon (right now getCenter can be outside polygons)
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

		LOG(" - RegionMap and Closest UnwalkablePolygonMap computed in " << timer.stopAndGetTime() << " seconds");
		timer.start();
		
		// compute closest BaseLocation map
		// ===========================================================================

		std::queue<baseDistance_t> seedPositions;
		for (const auto& baseLocation : BWTA_Result::baselocations) {
			seedPositions.emplace(baseLocation->getTilePosition().x * 4 + 8,
				baseLocation->getTilePosition().y * 4 + 6, baseLocation);
		}
		computeClosestObjectMap(seedPositions, BWTA_Result::getBaseLocationW);
		walkResMapToTileResMap(BWTA_Result::getBaseLocationW, BWTA_Result::getBaseLocation);

		LOG(" - Closest BaseLocation Map computed in " << timer.stopAndGetTime() << " seconds");
		timer.start();

		// compute closest Chokepoint map
		// ===========================================================================

		std::queue<chokeDistance_t> seedPositions2;
		for (const auto& chokepoint : BWTA_Result::chokepoints) {
			seedPositions2.emplace(chokepoint->getCenter().x / 8, chokepoint->getCenter().y / 8, chokepoint);
		}
		computeClosestObjectMap(seedPositions2, BWTA_Result::getChokepointW);
		walkResMapToTileResMap(BWTA_Result::getChokepointW, BWTA_Result::getChokepoint);

		LOG(" - Closest Chokepoint Map computed in " << timer.stopAndGetTime() << " seconds");
	}
}