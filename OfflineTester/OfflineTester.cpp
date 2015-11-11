#include "..\OfflineExtractor\MapFileParser.h"
#include "WallingGHOST.h"
#include "WallingASP.h"

int main(int argc, char * argv[])
{
	// Off line map file parsing
	bool ok = BWTA::parseMapFile("maps\\(3)Aztec.scx");
	if (!ok) return 1;
	// Normal procedure to analyze map
	BWTA::analyze();


	wallingGHOST(); // Test GHOST walling
// 	wallingASP(); // Test Certicky's walling


	BWTA::cleanMemory();
	return 0;
}
