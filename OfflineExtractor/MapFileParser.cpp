#include "MapFileParser.h"

#include <direct.h>
#include <bitset>
#include <StormLib.h>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include "../BWTA/Source/TileType.h"
#include "sha1.h"
#include "TileSet.h"
#include "MiniTileFlags.h"

namespace BWTA
{
	const bool VERBOSE = false;

	void printError(const char* archive, const char* message, const char* file, DWORD errorMessageID) {
		char cCurrentPath[FILENAME_MAX];
		_getcwd(cCurrentPath, sizeof(cCurrentPath));

		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr);

		std::string errorMessage(messageBuffer, size);
		LocalFree(messageBuffer); // free the buffer
		std::cout << archive << ": Error: " << message << " \"" << cCurrentPath << "\\" << file << "\": " << errorMessage << "\n";
	}

	/*
	Looks for a file with the .chk extension inside of a .scx map, decompresses it, and returns:
	- a pointer to the data in the .chk file
	- the size of the .chk file (in 'dataSize')
	*/
	unsigned char* extractCHKfile(const char* archive, DWORD* dataSize)
	{
		// Open MPQ
		HANDLE hMpq;
		if (!SFileOpenArchive(archive, 0, SFILE_OPEN_FROM_MPQ, &hMpq)) {
			printError(archive, "Cannot open archive", archive, GetLastError());
			return nullptr;
		}

		// Open .CHK file. It always has the same name
		HANDLE hFile;
		if (!SFileOpenFileEx(hMpq, "staredit\\scenario.chk", 0, &hFile)) {
			printError(archive, "Cannot extract CHK file", archive, GetLastError());
			return nullptr;
		}
		DWORD dwSize = SFileGetFileSize(hFile, nullptr);
//		std::cout << "CHK file found, size: " << dwSize << "\n";

		// Read CHK
		unsigned char* CHKdata = new unsigned char[dwSize];
		DWORD dwBytes = 0;
		SFileReadFile(hFile, CHKdata, dwSize, &dwBytes, nullptr);
		*dataSize = dwSize;
//		std::cout << "Read " << dwBytes << " of " << dwSize << " bytes\n";

		// Closing handles
		if (hFile != INVALID_HANDLE_VALUE) SFileFindClose(hFile);
		if (hMpq != INVALID_HANDLE_VALUE) SFileCloseArchive(hMpq);

		return CHKdata;
	}


	/*
	Decodes a 4 byte integer from a string of characters
	*/
	unsigned long decode4ByteUnsigned(unsigned char* ptr, DWORD& offset) {
		unsigned char* ptrOffset = ptr + offset;
		offset += 4;
		unsigned long ul = 0;
		ul += static_cast<unsigned long>(ptrOffset[0]) << 0;
		ul += static_cast<unsigned long>(ptrOffset[1]) << 8;
		ul += static_cast<unsigned long>(ptrOffset[2]) << 16;
		ul += static_cast<unsigned long>(ptrOffset[3]) << 24;
		return ul;
	}


	/*
	Decodes a 2 byte integer from a string of characters
	*/
	unsigned int decode2ByteUnsigned(unsigned char* ptr, DWORD& offset) {
		unsigned char* ptrOffset = ptr + offset;
		offset += 2;
		unsigned int ui = 0;
		ui += static_cast<unsigned long>(ptrOffset[0]) << 0;
		ui += static_cast<unsigned long>(ptrOffset[1]) << 8;
		return ui;
	}



	/*
	Goes over the CHK data printing the different chunk types and their lengths
	(this function is for debugging only)
	*/
	void printCHKchunks(unsigned char *CHKdata, DWORD size) {
		DWORD position = 0;

		while (position<size) {
			char chunkName[5];
			chunkName[0] = CHKdata[position++];
			chunkName[1] = CHKdata[position++];
			chunkName[2] = CHKdata[position++];
			chunkName[3] = CHKdata[position++];
			chunkName[4] = 0;
			DWORD chunkLength = decode4ByteUnsigned(CHKdata, position);

			std::cout << "Chunk '" << chunkName << "', size: " << chunkLength << '\n';
			position += chunkLength;
		}
	}


	/*
	Finds a given chunk inside CHK data and returns a pointer to it, and its length (in 'desiredChunkLength')
	*/
	unsigned char* getChunkPointer(const char* desiredChunk, unsigned char* CHKdata, DWORD size, DWORD* desiredChunkLength) {
		DWORD position = 0;

		while (position < size) {
			char chunkName[5];
			chunkName[0] = CHKdata[position++];
			chunkName[1] = CHKdata[position++];
			chunkName[2] = CHKdata[position++];
			chunkName[3] = CHKdata[position++];
			chunkName[4] = 0;
			DWORD chunkLength = decode4ByteUnsigned(CHKdata, position);

			if (strcmp(desiredChunk, reinterpret_cast<char*>(chunkName)) == 0) {
				*desiredChunkLength = chunkLength;
				return CHKdata + position;
			}
			position += chunkLength;
		}
		return nullptr;
	}

	struct Section
	{
		uint32_t name;
		uint32_t size;
	};

	unsigned char* newMTXM(unsigned char* CHKdata, DWORD size, uint32_t width, uint32_t height, DWORD& chunkSize) {
		DWORD position = 0;
		uint32_t expectedSize = width * height * 2;
		unsigned char* chunkData = nullptr;

		while (position < size) {
			Section* section = reinterpret_cast<Section*>(CHKdata+position);
			position += sizeof(Section);

			// copy the name to end it with 0
			char chunkName[5] = {0};
			memcpy(chunkName,&section[0].name,4);
//			std::cout << "Section: " << chunkName << " size: " << section[0].size << '\n';

			if (strcmp("MTXM", chunkName) == 0) {
				chunkSize = section[0].size;
				std::cout << "Found MTXM with size " << chunkSize << " (" << expectedSize << " expected)";
				if (chunkSize == expectedSize) {
					chunkData = new unsigned char[chunkSize];
					memcpy(chunkData, CHKdata+position, chunkSize);
				} else if (chunkData && chunkSize < expectedSize) {
					memcpy(chunkData, CHKdata+position, chunkSize); // overwrite from the begining
					std::cout << " overwrite to previous one from the begining";
				} else {
					std::cout << " omitted";
				}
				std::cout << '\n';
			}

			position += section[0].size;
		}
		return chunkData;
	}


	/*
	This function gets the dimensions of a StarCraft map from the CHKdata, and returns them in 'width', and 'height'
	*/
	void getDimensions(unsigned char* CHKdata, DWORD size, unsigned int& width, unsigned int& height) {
		DWORD chunkSize = 0;
		unsigned char* DIMdata = getChunkPointer("DIM ", CHKdata, size, &chunkSize);

		if (DIMdata != nullptr) {
			DWORD offset = 0;
			width = decode2ByteUnsigned(DIMdata, offset);
			height = decode2ByteUnsigned(DIMdata, offset);
		}
	}


	/*
	This function will decode the unit information from a CHK map, and return a list of all the units with their
	types, players and coordinates.
	*/
	void getUnits(unsigned char* CHKdata, DWORD size) {
		DWORD chunkSize = 0;
		unsigned char* UNITdata = getChunkPointer("UNIT", CHKdata, size, &chunkSize);

		if (UNITdata != nullptr) {
			int bytesPerUnit = 36;
			int neutralPlayer = 16;

			int nUnits = chunkSize / bytesPerUnit;
			std::cout << "UNIT chunk successfully found, with information about " << nUnits << " units\n";
			for (int i = 0; i<nUnits; i++) {
				DWORD position = i*bytesPerUnit;
				// decoding variables
				unsigned long unitClass = decode4ByteUnsigned(UNITdata, position);
				unsigned int x = decode2ByteUnsigned(UNITdata, position);
				unsigned int y = decode2ByteUnsigned(UNITdata, position);
				unsigned int ID = decode2ByteUnsigned(UNITdata, position);
				position += 2;	// skip relation to another building
				position += 2;	// skip special property flags
				unsigned int mapEditorFlags = decode2ByteUnsigned(UNITdata, position);
				int playerIsValid = mapEditorFlags & 0x0001;	// If this is 0, it is a neutral unit/critter/start location/etc.
				int player = playerIsValid == 1 ? UNITdata[position] : neutralPlayer; position += 1;
				position += 1; // hit points
				position += 1; // shield points
				position += 1; // energy points
				unsigned int resourceAmount = decode4ByteUnsigned(UNITdata, position);

				
				BWAPI::UnitType unitType(ID);
//				BWAPI::Position unitPosition(x, y); // x/y coordinates are the center of the sprite of the unit in pixels
//				BWAPI::WalkPosition unitWalkPosition(unitPosition);
				BWAPI::TilePosition unitTilePosition((x - unitType.dimensionLeft()) / TILE_SIZE, (y - unitType.dimensionUp()) / TILE_SIZE);
				if (VERBOSE) {
 					std::cout << "Unit " << unitType << " at " << x << "," << y << " player " << player << "\n";
				}

				if (unitType.isMineralField() || unitType == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
					MapData::resources.emplace_back(unitType, unitTilePosition, resourceAmount);
				} else if (unitType == BWAPI::UnitTypes::Special_Start_Location) {
					MapData::startLocations.push_back(unitTilePosition);
				} else {
					if (VERBOSE) std::cout << "Ignored unit: " << unitType << " at " << x << "," << y << std::endl;
				}
			}

		}
	}

	/*
	This function decode the doodads information from a CHK map, 
	and store the neutral buildings positions in MapData::staticNeutralBuildings
	*/
	void getDoodads(unsigned char* CHKdata, DWORD size) {
		DWORD chunkSize = 0;
		unsigned char* UNITdata = getChunkPointer("THG2", CHKdata, size, &chunkSize);

		if (UNITdata != nullptr) {
			const int bytesPerUnit = 10;
			int nDoodads = chunkSize / bytesPerUnit;
			std::cout << "THG2 chunk successfully found, with information about " << nDoodads << " doodads\n";

			for (int i = 0; i < nDoodads; ++i) {
				DWORD position = i*bytesPerUnit;
				unsigned long unitNumber = decode2ByteUnsigned(UNITdata, position);
				unsigned int x = decode2ByteUnsigned(UNITdata, position);
				unsigned int y = decode2ByteUnsigned(UNITdata, position);
				int player = UNITdata[position]; position += 1;
				position += 2; // unused
				unsigned int flags = decode2ByteUnsigned(UNITdata, position);
				bool isSprite = flags >> 4 & 1;

				if (!isSprite) {  // if it is a sprite, the terrain tile info should take care of walkability
					BWAPI::UnitType unitType(unitNumber);
					MapData::staticNeutralBuildings.emplace_back(unitType, BWAPI::Position(x, y));
					if (VERBOSE) std::cout << "Doodad " << unitType << " at " << x << "," << y << std::endl;
				} else {
					if (VERBOSE) std::cout << "Ignored Sprite " << unitNumber << " at " << x << "," << y <<  std::endl;
				}
			}

		}
	}

	/*
	This function returns the tilset ID of a StarCraft map from the CHKdata
	*/
	unsigned int getTileset(unsigned char* CHKdata, DWORD size) {
		DWORD chunkSize = 0;
		unsigned char* ERAdata = getChunkPointer("ERA ", CHKdata, size, &chunkSize);

		if (ERAdata != nullptr) {
			// StarCraft masks the tileset indicator's bit value, 
			// so bits after the third place (anything after the value "7") are removed. 
			// Thus, 9 (1001 in binary) is interpreted as 1 (0001), 10 (1010) as 2 (0010), etc. 
			unsigned char tileset = ERAdata[0];
			//std::cout << "ERAdata = " << (std::bitset<8>) tileset << std::endl;
			tileset &= 7;
			//std::cout << "ERAdata = " << (std::bitset<8>) tileset << std::endl;
			return static_cast<unsigned int>(tileset);
		}
		return 8;
	}

	/*
	Finds a given chunk inside CHK data and returns a pointer to it, and its length (in 'desiredChunkLength')
	*/
	unsigned char* getFileBuffer(const char* filename) {
		unsigned char* buffer = nullptr;
		std::ifstream file(filename, std::ios::in | std::ios::binary);
		if (file.is_open()) {
			file.seekg(0, std::ios::end);
			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);

			buffer = new unsigned char[static_cast<unsigned int>(size)];
			if (!file.read(reinterpret_cast<char*>(buffer), size)) {
				printError(filename, "Error reading file", filename, GetLastError());
			}
			file.close();
		} else {
			printError(filename, "Cannot open file", filename, GetLastError());
		}
		return buffer;
	}

	//------------------------------------------------ GET TILE ------------------------------------------------
	TileID getTile(int x, int y)
	{
		if (MapData::TileArray)
			return *(MapData::TileArray + x + y * MapData::mapWidthTileRes);
		return 0;
	}
	//------------------------------------------- GET TILE VARIATION -------------------------------------------
	uint8_t getTileVariation(TileID tileType)
	{
		return tileType & 0xF;
	}
	//--------------------------------------------- GET MINITILE -----------------------------------------------
	uint16_t getMiniTile(int x, int y)
	{
		int tx = x / 4;
		int ty = y / 4;
		int mx = x % 4;
		int my = y % 4;
		TileID tileID = getTile(tx, ty);
		TileType* tile = TileSet::getTileType(tileID);
		if (tile && MapData::MiniTileFlags)
			return MapData::MiniTileFlags->tile[tile->miniTile[getTileVariation(tileID)]].miniTile[mx + my * 4];
		return 0;
	}
	//-------------------------------------------- SET BUILDABILITY --------------------------------------------
	void setOfflineBuildability(RectangleArray<bool> &buildability)
	{
		for (unsigned int y = 0; y < MapData::mapHeightTileRes; ++y)
		for (unsigned int x = 0; x < MapData::mapWidthTileRes; ++x) {
			TileID tileID = getTile(x, y);
//			TileID id = tileID >> 4  & 0x7FF;
//			if (id > 1024) {
//				std::cout << "Doodad " << id << " found at " << x << "," << y << "\n";
//			}
			TileType* tile = TileSet::getTileType(tileID);
			buildability[x][y] = (tile->buildability & (1 << 7)) == 0;
		}
	}
	//-------------------------------------------- SET WALKABILITY ---------------------------------------------
	void setOfflineWalkability(RectangleArray<bool> &walkability)
	{
		uint16_t h = MapData::mapHeightWalkRes;
		uint16_t w = MapData::mapWidthWalkRes;
		for (unsigned int y = 0; y < h; ++y)
			for (unsigned int x = 0; x < w; ++x)
				walkability[x][y] = (getMiniTile(x, y) & MiniTileFlags::Walkable) != 0;
		int y = h - 1;
		for (unsigned int x = 0; x < w; ++x) {
			walkability[x][y] = false;
			walkability[x][y - 1] = false;
			walkability[x][y - 2] = false;
			walkability[x][y - 3] = false;
		}
		y -= 4;
		for (int x = 0; x < 20; ++x) {
			walkability[x][y] = false;
			walkability[x][y - 1] = false;
			walkability[x][y - 2] = false;
			walkability[x][y - 3] = false;
			walkability[w - x - 1][y] = false;
			walkability[w - x - 1][y - 1] = false;
			walkability[w - x - 1][y - 2] = false;
			walkability[w - x - 1][y - 3] = false;
		}
	}

	bool parseMapFile(const char* mapFilePath)
	{
		// check if logs folder exists
		if (!boost::filesystem::exists("logs")) {
			boost::filesystem::path dir("logs");
			boost::filesystem::create_directories(dir);
		}

		// Save map name
		std::string strName = mapFilePath;
		unsigned found = strName.find_last_of("/\\");
		MapData::mapFileName = strName.substr(found + 1);
		std::cout << "START PARSING FILE " << MapData::mapFileName << std::endl;

		DWORD dataSize = 0;
		unsigned char* CHKdata = extractCHKfile(mapFilePath, &dataSize);
		if (CHKdata == nullptr) return false;

		std::cout << "Successfully extracted the CHK file (size " << dataSize << ")" << std::endl;
//		printCHKchunks(CHKdata, dataSize);

		// Calculate hash from MPQ (not only the CHK)
		// ==========================================
		unsigned char hash[20];
		char hexstring[sizeof(hash) * 2 + 1];
		HANDLE hFile = nullptr;

		// Open file
		SFileOpenFileEx(nullptr, mapFilePath, SFILE_OPEN_LOCAL_FILE, &hFile);
		size_t fileSize = SFileGetFileSize(hFile, nullptr);
		std::vector<char> data(fileSize);

		// Read file
		DWORD read = 0;
		SFileReadFile(hFile, data.data(), fileSize, &read, nullptr);

		// Calculate hash
		sha1::calc(data.data(), fileSize, hash);
		sha1::toHexString(hash, hexstring);

		// Save and close
		MapData::hash = std::string(hexstring);
		SFileCloseFile(hFile);

		// Load map dimensions
		// ====================
		unsigned int width = 0, height = 0;
		getDimensions(CHKdata, dataSize, width, height);
		std::cout << "Map is " << width << "x" << height << "\n";
		MapData::mapWidthTileRes = width;
		MapData::mapWidthWalkRes = MapData::mapWidthTileRes * 4;
		MapData::mapWidthPixelRes = MapData::mapWidthTileRes * 32;
		MapData::mapHeightTileRes = height;
		MapData::mapHeightWalkRes = MapData::mapHeightTileRes * 4;
		MapData::mapHeightPixelRes = MapData::mapHeightTileRes * 32;

		// Load neutral units (minerals, gas, and starting locations)
		getUnits(CHKdata, dataSize);

		// Some doodads (buildings) are not walkable
		getDoodads(CHKdata, dataSize);

		// Get map tileset ID
		unsigned int tileset = getTileset(CHKdata, dataSize);
		if (tileset > 7) {
			std::cout << "Tileset Unknown (" << tileset << ")\n";
			return false;
		}
		std::cout << "Map's tilset: " << tileSetName[tileset] << "\n";

		// Load TileSet file (tileSetName[tileset].cv5) into MapData::TileSet
		std::string cv5FileName = "tileset/" + tileSetName[tileset] + ".cv5";
		MapData::TileSet = reinterpret_cast<TileType*>(getFileBuffer(cv5FileName.c_str()));

		// Load MiniTileFlags file (tileSetName[tileset].vf4) into MapData::MiniTileFlags
		std::string vf4FileName = "tileset/" + tileSetName[tileset] + ".vf4";
		MapData::MiniTileFlags = reinterpret_cast<MapData::MiniTileMaps_type*>(getFileBuffer(vf4FileName.c_str()));

		// Load Map Tiles
		DWORD chunkSize = 0;
		unsigned char* mtxm = newMTXM(CHKdata, dataSize, width, height, chunkSize);
		MapData::TileArray = reinterpret_cast<TileID*>(mtxm);

		// Set walkability
		MapData::rawWalkability.resize(MapData::mapWidthWalkRes, MapData::mapHeightWalkRes);
		setOfflineWalkability(MapData::rawWalkability);
//		MapData::rawWalkability.saveToFile("logs/rawWalkability.txt");

		// Set buildability
		MapData::buildability.resize(MapData::mapWidthTileRes, MapData::mapHeightTileRes);
		setOfflineBuildability(MapData::buildability);
//		MapData::buildability.saveToFile("logs/buildable.txt");


		delete CHKdata;
		delete mtxm;
		std::cout << "END PARSING FILE" << std::endl;
		return true;
	}

}