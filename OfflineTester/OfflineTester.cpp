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
	BWTA::BaseLocation* homeBase = *BWTA::getStartLocations().begin();
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


	wallingGHOST(smallestChokepoint); // Test GHOST walling
// 	wallingASP(smallestChokepoint, homeBase); // Test Certicky's walling


	BWTA::cleanMemory();
	return 0;
}
