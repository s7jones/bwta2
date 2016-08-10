#include "Pathfinding.h"
#include "../BWTA/Source/Timer.h"

void compareDistance(BWAPI::TilePosition pos1, BWAPI::TilePosition pos2)
{
	std::cout << "Distance between " << pos1 << " and " << pos2 << std::endl;

	Timer timer;

	timer.start();
	double dist = BWTA::getGroundDistance(pos1, pos2);
	std::cout << " Distance: " << dist << " computed in " << timer.stopAndGetTime() << " seconds" << std::endl;

	timer.start();
	int dist2 = BWTA::getGroundDistance2(pos1, pos2);
	std::cout << " Distance2: " << dist2 << " computed in " << timer.stopAndGetTime() << " seconds" << std::endl;
}