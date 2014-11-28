#include <windows.h>

#include "MapData.h"
#include "offline/sha1.h"
#include "offline/MapFileParser.h"
#include "offline/wallingAnalysis.h"


int main (int argc, char * argv[])
{
	// Check the number of parameters
	if (argc < 2) {
		// Tell the user how to run the program
		std::cerr << "Usage: " << argv[0] << " [mapFile]" << std::endl;
		return 1;
	}

	std::cout << "=======================" << std::endl;
	std::cout << "      OFFLINE BWTA     " << std::endl;
	std::cout << "=======================" << std::endl;

	// Save map name
	std::string strName = argv[1];
	unsigned found = strName.find_last_of("/\\");
	BWTA::MapData::mapFileName = strName.substr(found+1);
	std::cout << "START PARSING FILE " << BWTA::MapData::mapFileName << std::endl;

	
	DWORD dataSize = 0;
	unsigned char *CHKdata = BWTA::extractCHKfile(argv[1], &dataSize);
	if (CHKdata==NULL) return 0;

	std::cout << "Successfully extracted the CHK file (size " << dataSize << ")" << std::endl;
	//BWTA::printCHKchunks(CHKdata, dataSize);

	// Calculate hash
	unsigned char hash[20];
	char hexstring[42];
	sha1::calc(CHKdata, dataSize, hash);
	sha1::toHexString(hash, hexstring);
	BWTA::MapData::hash = std::string(hexstring);

	// Load map dimensions
	unsigned int width = 0, height = 0;
	BWTA::getDimensions(CHKdata, dataSize, &width, &height);
	std::cout << "Map is " << width << "x" << height << "\n";
	BWTA::MapData::mapWidth = width;
	BWTA::MapData::mapHeight = height;

	// Load neutral units (minerals, gas, and starting locations)
	BWTA::getUnits(CHKdata, dataSize);

	// Some doodads (buildings) are not walkable
	BWTA::getDoodads(CHKdata, dataSize);

	// Get map tileset ID
	unsigned int tileset = BWTA::getTileset(CHKdata, dataSize);
	if (tileset > 7) {
		std::cout << "Tileset Unknown (" << tileset << ")\n";
		return 0;
	}
	std::cout << "Map's tilset: " << BWTA::tileSetName[tileset] << "\n";
  
	// Load TileSet file (tileSetName[tileset].cv5) into BWTA::MapData::TileSet
	std::string cv5FileName = "tileset/"+BWTA::tileSetName[tileset]+".cv5";
	BWTA::MapData::TileSet = (TileType*)BWTA::getFileBuffer(cv5FileName.c_str());


	// Load MiniTileFlags file (tileSetName[tileset].vf4) into BWTA::MapData::MiniTileFlags
	std::string vf4FileName = "tileset/" + BWTA::tileSetName[tileset] + ".vf4";
	BWTA::MapData::MiniTileFlags = (BWTA::MapData::MiniTileMaps_type*)BWTA::getFileBuffer(vf4FileName.c_str());


	// Load Map Tiles
	DWORD chunkSize = 0;
	BWTA::MapData::TileArray = (TileID*)BWTA::getChunkPointer((unsigned char *)"MTXM", CHKdata, dataSize, &chunkSize);


	// Set walkability
	BWTA::MapData::isWalkable.resize(BWTA::MapData::mapWidth * 4, BWTA::MapData::mapHeight * 4);
	BWTA::setOfflineWalkability(BWTA::MapData::isWalkable);
	// Test walkability data
	BWTA::MapData::isWalkable.saveToFile("logs/walkable.txt");


	// Set buildability
	BWTA::MapData::buildability.resize(BWTA::MapData::mapWidth, BWTA::MapData::mapHeight);
	BWTA::setOfflineBuildability(BWTA::MapData::buildability);
	// Test buildability data
	BWTA::MapData::buildability.saveToFile("logs/buildable.txt");


	delete CHKdata;
	std::cout << "END PARSING FILE" << std::endl;

	// Normal procedure to analyze map
	std::cout << "All info loaded, analyzing map as regular..." << std::endl;
	BWTA::readMap();
	BWTA::analyze();
	std::cout << "DONE" << std::endl;

	// Generate the grids around all chokepoints:
	std::ofstream fileTxt("logs/output.txt"); 
	const std::set<BWTA::Chokepoint*> chokePoints = BWTA::getChokepoints();
	for(std::set<BWTA::Chokepoint*>::const_iterator c = chokePoints.begin(); c!=chokePoints.end();++c) {
		BWTA::Chokepoint *cp = (*c);
		BWTA::RectangleArray<int> chokeGrid = BWTA::getChokeGrid(cp);
		// Print grid
		BWAPI::TilePosition center = BWAPI::TilePosition(cp->getCenter());
		int radius = int(cp->getWidth()/TILE_SIZE);
		BWAPI::TilePosition region1 = BWAPI::TilePosition(cp->getRegions().first->getCenter());
		BWAPI::TilePosition region2 = BWAPI::TilePosition(cp->getRegions().second->getCenter());
		fileTxt << "Chokepoint:\n";
		fileTxt << "Region 1: " << radius+region1.x-center.x << "," << radius+region1.y-center.y << std::endl;
		fileTxt << "Region 2: " << radius+region2.x-center.x << "," << radius+region2.y-center.y << std::endl;
		BWAPI::TilePosition side1 = BWAPI::TilePosition(cp->getSides().first);
		BWAPI::TilePosition side2 = BWAPI::TilePosition(cp->getSides().second);
		int s1x = radius + side1.x - center.x;
		int s1y = radius + side1.y - center.y;
// 		fileTxt << "Side 1: " << s1x << "," << s1y << " is " << chokeGrid[s1x][s1y] << std::endl;
		int s2x = radius+side2.x-center.x;
		int s2y = radius+side2.y-center.y;
// 		fileTxt << "Side 2: " << s2x << "," << s2y << " is " << chokeGrid[s2x][s2y] << std::endl;
		BWTA::generateWallStartingPoints(chokeGrid, s1x, s1y, s2x, s2y, &fileTxt);
		for (unsigned int y = 0; y < chokeGrid.getHeight(); ++y) {
			for (unsigned int x = 0; x < chokeGrid.getWidth(); ++x) {
				fileTxt << chokeGrid[x][y];
			}
			fileTxt << "\n";
		}
	}
	fileTxt.close();

	return 0;
}
