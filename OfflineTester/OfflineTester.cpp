#include "..\OfflineExtractor\MapFileParser.h"
#include "WallingGHOST.h"
#include "WallingASP.h"

#include <tlhelp32.h>

int main(int argc, char * argv[])
{
	// Off-line map file parsing
	bool ok = BWTA::parseMapFile("maps\\(3)Aztec.scx");
	if (!ok) return 1;
	// Normal procedure to analyze map
	BWTA::analyze();


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

	std::cout << "walling chokepoint " << smallestChokepoint->getCenter() << " and pref region " << homeRegion->getCenter() << std::endl;
	wallingGHOST(smallestChokepoint, homeRegion); // Test GHOST walling
// 	wallingASP(smallestChokepoint, homeBase); // Test Certicky's walling


	BWTA::cleanMemory();
	return 0;
}
