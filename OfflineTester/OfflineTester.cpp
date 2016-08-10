#include "..\OfflineExtractor\MapFileParser.h"
#include "WallingGHOST.h"
#include "WallingASP.h"
#include "Pathfinding.h"

#include <tlhelp32.h>
#include <chrono>

void wallingTest();
void patfhindingTest();

int main(int argc, char * argv[])
{
	// Off-line map file parsing
	bool ok = BWTA::parseMapFile("maps\\(3)Aztec.scx");
	if (!ok) return 1;
	// Normal procedure to analyze map
	BWTA::analyze();


	patfhindingTest();
// 	wallingTest(); // Tests with "maps\\(3)Aztec.scx"


	BWTA::cleanMemory();
	return 0;
}

void patfhindingTest() {
	BWTA::buildChokeNodes();

	compareDistance(BWAPI::TilePosition(9, 84), BWAPI::TilePosition(69, 7));
	compareDistance(BWAPI::TilePosition(9, 84), BWAPI::TilePosition(118, 101));
	compareDistance(BWAPI::TilePosition(69, 7), BWAPI::TilePosition(118, 101));
}

void wallingTest()
{
	// get the first startLocation as home
	BWTA::BaseLocation* homeBase = nullptr;
	// get start location with lowest y position
	int minY = std::numeric_limits<int>::max();
	for (const auto& startLocation : BWTA::getStartLocations()) {
		if (startLocation->getRegion()->getCenter().y < minY) {
			minY = startLocation->getRegion()->getCenter().y;
			homeBase = startLocation;
		}
	}
	BWTA::Region* homeRegion = homeBase->getRegion();

	// iterate through all chokepoints and look for the one with the smallest gap (least width)
	double minWidth = std::numeric_limits<double>::max();
	BWTA::Chokepoint* smallestChokepoint = nullptr;
	for (const auto& chokepoint : homeRegion->getChokepoints()) {
		if (chokepoint->getWidth() < minWidth) {
			minWidth = chokepoint->getWidth();
			smallestChokepoint = chokepoint;
		}
	}
	std::cout << "walling chokepoint " << smallestChokepoint->getCenter() << " (width: " << minWidth << ") and pref region " << homeRegion->getCenter() << std::endl;
	// iterate through all chokepoints and look for the one with the smallest gap (least width)
	double maxWidth = std::numeric_limits<double>::min();
	BWTA::Chokepoint* biggestChokepoint = nullptr;
	for (const auto& chokepoint : BWTA::getChokepoints()) {
		if (chokepoint->getWidth() > maxWidth && chokepoint->getWidth() < 300) {
			maxWidth = chokepoint->getWidth();
			biggestChokepoint = chokepoint;
		}
	}
	std::cout << "walling chokepoint " << biggestChokepoint->getCenter() << " (width: " << maxWidth << ") and pref region " << (biggestChokepoint->getRegions().first)->getCenter() << std::endl;

	const int loops = 1;
	int i = 0;
	double totalTime = 0;
	double success = 0;
	bool runGhost = true;
	// 	BWTA::Chokepoint* selectedChokepoint = biggestChokepoint;
	// 	BWTA::Region* selectedRegion = biggestChokepoint->getRegions().first;
	// 	BWAPI::TilePosition selectedPosition = BWAPI::TilePosition(selectedRegion->getCenter());
	BWTA::Chokepoint* selectedChokepoint = smallestChokepoint;
	BWTA::Region* selectedRegion = homeRegion;
	BWAPI::TilePosition selectedPosition = homeBase->getTilePosition();
	double result;

	for (i; i < loops; ++i) {

		auto start = std::chrono::steady_clock::now();

		if (runGhost)
			result = wallingGHOST(selectedChokepoint, selectedRegion); // Test GHOST walling
		else
			wallingASP(selectedChokepoint, selectedRegion, selectedPosition); // Test Certicky's walling

		auto end = std::chrono::steady_clock::now();
		auto diff = end - start;
		totalTime += std::chrono::duration <double, std::milli>(diff).count();
		if (runGhost && result == 1) ++success;
	}
	std::cout << "=================" << std::endl;
	std::cout << "Time: " << totalTime / (double)loops << " ms" << std::endl;
	if (runGhost) std::cout << "Success: " << (success / (double)loops) * 100 << " %" << std::endl;
}
