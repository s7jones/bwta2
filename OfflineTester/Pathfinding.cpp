#include "Pathfinding.h"

void compareDistance(BWAPI::TilePosition pos1, BWAPI::TilePosition pos2)
{
	double dist = BWTA::getGroundDistance(pos1, pos2);
	int dist2 = BWTA::getGroundDistance2(pos1, pos2);
	std::cout << "Distance between " << pos1 << " and " << pos2 << std::endl;
	std::cout << " Distance: " << dist << std::endl;
	std::cout << " Distance2: " << dist2 << std::endl;
}