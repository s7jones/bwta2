#include "MapFileParser.h"

namespace BWTA
{

	void printError(const char* archive, const char* message, const char* file, int errnum) {
		char* error = NULL;
		char cCurrentPath[FILENAME_MAX];
		_getcwd(cCurrentPath, sizeof(cCurrentPath));

		switch (errnum) {
		case 105:
			error = (char*)"Bad format"; break;
		case 106:
			error = (char*)"No more files"; break;
		case 107:
			error = (char*)"Handle EOF"; break;
		case 108:
			error = (char*)"Cannot compile"; break;
		case 109:
			error = (char*)"File corrupted"; break;
		default:
			char msg[128];
			strerror_s(msg, sizeof(msg), errnum);
			error = msg;
		}

		std::cout << archive << ": Error: " << message << " \"" << cCurrentPath << "\\" << file << "\": " << error << "\n";
	}

	/*
	Check whether the string 'fullString' ends with the string 'ending'
	*/
	bool hasEnding(const char* fullString, const char* ending)
	{
		size_t l1 = strlen(fullString);
		size_t l2 = strlen(ending);
		if (l1 >= l2) {
			int start = l1 - l2;
			for (size_t idx = 0; idx<l2; idx++) {
				if (fullString[start + idx] != ending[idx]) return false;
			}
			return true;
		} else {
			return false;
		}
	}


	/*
	Looks for a file with the .chk extension inside of a .scx map, decompresses it, and returns:
	- a pointer to the data in the .chk file
	- the size of the .chk file (in 'dataSize')
	*/
	unsigned char* extractCHKfile(const char* archive, DWORD* dataSize)
	{
		HANDLE hMpq = NULL;            // Open archive handle
		HANDLE hFileFind = NULL;       // Archived file handle
		SFILE_FIND_DATA SFileFindData; // Data of the archived file
		bool chkFilefound = false;     // Whether the chk file was found in the archive
		unsigned char* CHKdata = NULL;

		// Open an archive
		if (!SFileOpenArchive(archive, 0, SFILE_OPEN_FROM_MPQ, &hMpq)) {
			printError(archive, "Cannot open archive", archive, GetLastError());
			return NULL;
		}

		// List all files in archive
		hFileFind = SFileFindFirstFile(hMpq, "*", &SFileFindData, NULL);
		while (hFileFind) {
			std::cout << SFileFindData.cFileName << "\n";
			if (hasEnding(SFileFindData.cFileName, ".chk")) {
				chkFilefound = true;
				break;
			}
			if (!SFileFindNextFile(hFileFind, &SFileFindData))
				break;
		}

		if (chkFilefound) {
// 			 std::cout << "CHK file found: " << SFileFindData.cFileName << ", size: " << SFileFindData.dwFileSize << "\n";

			// Closing previous file handle
			if (hFileFind != (HANDLE)0xFFFFFFFF)
				SFileFindClose(hFileFind);

			// Open (extract) CHK file
			if (!SFileOpenFileEx(hMpq, SFileFindData.cFileName, 0, &hFileFind)) {
				printError(archive, "Cannot extract CHK file", archive, GetLastError());
				return NULL;
			}

			// Read CHK
			CHKdata = new unsigned char[SFileFindData.dwFileSize];
			DWORD dwBytes = 0;
			SFileReadFile(hFileFind, CHKdata, SFileFindData.dwFileSize, &dwBytes, NULL);
			// std::cout << "Read " << dwBytes << " of " << SFileFindData.dwFileSize << " bytes\n";
			*dataSize = SFileFindData.dwFileSize;
		}

		// Closing handles
		if (hFileFind != (HANDLE)0xFFFFFFFF)
			SFileFindClose(hFileFind);
		if (hMpq != (HANDLE)0xFFFFFFFF)
			SFileCloseArchive(hMpq);

		return CHKdata;
	}


	/*
	Decodes a 4 byte integer from a string of characters
	*/
	unsigned long decode4ByteUnsigned(unsigned char* ptr) {
		unsigned long ul = 0;
		ul += ((unsigned long)ptr[0]) << 0;
		ul += ((unsigned long)ptr[1]) << 8;
		ul += ((unsigned long)ptr[2]) << 16;
		ul += ((unsigned long)ptr[3]) << 24;
		return ul;
	}


	/*
	Decodes a 2 byte integer from a string of characters
	*/
	unsigned int decode2ByteUnsigned(unsigned char* ptr) {
		unsigned int ui = 0;
		ui += ((unsigned int)ptr[0]) << 0;
		ui += ((unsigned int)ptr[1]) << 8;
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
			unsigned long chunkLength = decode4ByteUnsigned(CHKdata + position);
			position += 4;

			std::cout << "Chunk '" << chunkName << "', size: " << chunkLength << "\n";

			position += chunkLength;
		}
	}


	/*
	Finds a given chunk inside CHK data and returns a pointer to it, and its length (in 'desiredChunkLength')
	*/
	unsigned char* getChunkPointer(unsigned char* desiredChunk, unsigned char* CHKdata, DWORD size, DWORD* desiredChunkLength) {
		DWORD position = 0;

		while (position<size) {
			char chunkName[5];
			chunkName[0] = CHKdata[position++];
			chunkName[1] = CHKdata[position++];
			chunkName[2] = CHKdata[position++];
			chunkName[3] = CHKdata[position++];
			chunkName[4] = 0;
			unsigned long chunkLength = decode4ByteUnsigned(CHKdata + position);
			position += 4;

			if (strcmp((char *)desiredChunk, (char *)chunkName) == 0) {
				*desiredChunkLength = chunkLength;
				return CHKdata + position;
			}
			position += chunkLength;
		}
		return NULL;
	}


	/*
	This function gets the dimensions of a StarCraft map from the CHKdata, and returns them in 'width', and 'height'
	*/
	void getDimensions(unsigned char* CHKdata, DWORD size, unsigned int& width, unsigned int& height) {
		DWORD chunkSize = 0;
		unsigned char* DIMdata = getChunkPointer((unsigned char*)"DIM ", CHKdata, size, &chunkSize);

		if (DIMdata != NULL) {
			width = decode2ByteUnsigned(DIMdata);
			height = decode2ByteUnsigned(DIMdata + 2);
		}
	}


	/*
	This function will decode the unit information from a CHK map, and return a list of all the units with their
	types, players and coordinates.
	*/
	void getUnits(unsigned char* CHKdata, DWORD size) {
		DWORD chunkSize = 0;
		unsigned char* UNITdata = getChunkPointer((unsigned char*)"UNIT", CHKdata, size, &chunkSize);

		if (UNITdata != NULL) {
			int bytesPerUnit = 36;
			int neutralPlayer = 16;

			int nUnits = chunkSize / bytesPerUnit;
			std::cout << "UNIT chunk successfully found, with information about " << nUnits << " units\n";
			for (int i = 0; i<nUnits; i++) {
				int position = (i*bytesPerUnit);
				unsigned long unitClass = decode4ByteUnsigned(UNITdata + position);
				position += 4;
				unsigned int x = decode2ByteUnsigned(UNITdata + position);
				position += 2;
				unsigned int y = decode2ByteUnsigned(UNITdata + position);
				position += 2;
				unsigned int ID = decode2ByteUnsigned(UNITdata + position);
				position += 2;
				position += 2;	// skip relation to another building
				position += 2;	// skip special property flags
				unsigned int mapEditorFlags = decode2ByteUnsigned(UNITdata + position);
				int playerIsValid = mapEditorFlags & 0x0001;	// If this is 0, it is a neutral unit/critter/start location/etc.
				position += 2;
				int player = (playerIsValid == 1 ? UNITdata[position] : neutralPlayer);
				
				BWAPI::UnitType unitType(ID);
				BWAPI::Position unitPosition(x, y);
				BWAPI::WalkPosition unitWalkPosition(unitPosition);
				BWAPI::TilePosition unitTilePosition((x - unitType.dimensionLeft()) / TILE_SIZE, (y - unitType.dimensionUp()) / TILE_SIZE);
// 				std::cout << "Unit(" << unitClass << ") type=" << unitType.c_str() << " at " << x << "," << y << " player " << player << "\n";
// 				std::cout << unitType.c_str() << " Tile " << unitTilePosition.x << "," << unitTilePosition.y << std::endl;

				if (unitType.isMineralField() || unitType == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
					MapData::resourcesWalkPositions.push_back(std::make_pair(unitType, unitWalkPosition));
				}
				if (unitType == BWAPI::UnitTypes::Special_Start_Location) {
// 					std::cout << unitType.c_str() << " Tile " << unitTilePosition.x << "," << unitTilePosition.y << std::endl;
					MapData::startLocations.push_back(unitTilePosition);
				}
			}

		}
	}

	/*
	This function decode the doodads information from a CHK map, 
	and store the neutral buildings positions in BWTA::MapData::staticNeutralBuildings
	*/
	void getDoodads(unsigned char* CHKdata, DWORD size) {
		DWORD chunkSize = 0;
		unsigned char* UNITdata = getChunkPointer((unsigned char*)"THG2", CHKdata, size, &chunkSize);

		if (UNITdata != NULL) {
			int bytesPerUnit = 8;

			int nDoodads = chunkSize / bytesPerUnit;
			std::cout << "THG2 chunk successfully found, with information about " << nDoodads << " doodads\n";
			for (int i = 0; i<nDoodads; i++) {
				int position = (i*bytesPerUnit);
				unsigned long unitNumber = decode2ByteUnsigned(UNITdata + position);
				if (unitNumber > 227) continue; // ignore units out of range
				position += 2;
				unsigned int x = decode2ByteUnsigned(UNITdata + position);
				position += 2;
				unsigned int y = decode2ByteUnsigned(UNITdata + position);
				position += 2;
				unsigned int ID = decode2ByteUnsigned(UNITdata + position);
				position += 2;
				int player = UNITdata[position];
				
				BWAPI::UnitType unitType(unitNumber);
				//std::cout << "Doodad (" << unitNumber << ":" << unitType.c_str() << ") at " << x << "," << y << " player " << player << "\n";
				
				if (unitType.isBuilding()) {
					//std::cout << "Doodad " << unitType.c_str() << " at " << x << "," << y << std::endl;
					BWAPI::Position unitPosition(x, y);
					BWTA::UnitTypePosition unitTypePosition = std::make_pair(unitType, unitPosition);
					BWTA::MapData::staticNeutralBuildings.push_back(unitTypePosition);
				}
			}

		}
	}

	/*
	This function returns the tilset ID of a StarCraft map from the CHKdata
	*/
	unsigned int getTileset(unsigned char* CHKdata, DWORD size) {
		DWORD chunkSize = 0;
		unsigned char* ERAdata = getChunkPointer((unsigned char*)"ERA ", CHKdata, size, &chunkSize);

		if (ERAdata != NULL) {
			// StarCraft masks the tileset indicator's bit value, 
			// so bits after the third place (anything after the value "7") are removed. 
			// Thus, 9 (1001 in binary) is interpreted as 1 (0001), 10 (1010) as 2 (0010), etc. 
			unsigned char tileset = ERAdata[0];
			//std::cout << "ERAdata = " << (std::bitset<8>) tileset << std::endl;
			tileset &= 7;
			//std::cout << "ERAdata = " << (std::bitset<8>) tileset << std::endl;
			return (unsigned int)tileset;
		}
		return 8;
	}

	/*
	Finds a given chunk inside CHK data and returns a pointer to it, and its length (in 'desiredChunkLength')
	*/
	unsigned char* getFileBuffer(const char* filename) {
		unsigned char* buffer = NULL;
		std::ifstream file(filename, std::ios::in | std::ios::binary);
		if (file.is_open()) {
			file.seekg(0, std::ios::end);
			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);

			buffer = new unsigned char[static_cast<unsigned int>(size)];
			if (!file.read((char*)buffer, size)) {
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
		if (BWTA::MapData::TileArray)
			return *(BWTA::MapData::TileArray + x + y * BWTA::MapData::mapWidth);
		return 0;
	}
	//------------------------------------------- GET TILE VARIATION -------------------------------------------
	u8 getTileVariation(TileID tileType)
	{
		return tileType & 0xF;
	}
	//--------------------------------------------- GET MINITILE -----------------------------------------------
	u16 getMiniTile(int x, int y)
	{
		int tx = x / 4;
		int ty = y / 4;
		int mx = x % 4;
		int my = y % 4;
		TileID tileID = getTile(tx, ty);
		TileType* tile = TileSet::getTileType(tileID);
		if (tile && BWTA::MapData::MiniTileFlags)
			return BWTA::MapData::MiniTileFlags->tile[tile->miniTile[getTileVariation(tileID)]].miniTile[mx + my * 4];
		return 0;
	}
	//-------------------------------------------- SET BUILDABILITY --------------------------------------------
	void setOfflineBuildability(BWTA::RectangleArray<bool> &buildability)
	{
		u16 h = BWTA::MapData::mapHeight;
		u16 w = BWTA::MapData::mapWidth;
		for (unsigned int y = 0; y < h; ++y)
			for (unsigned int x = 0; x < w; ++x) {
			TileID tileID = getTile(x, y);
			TileType* tile = TileSet::getTileType(tileID);
			buildability[x][y] = (tile->buildability & (1 << 7)) == 0;
			}
	}
	//-------------------------------------------- SET WALKABILITY ---------------------------------------------
	void setOfflineWalkability(BWTA::RectangleArray<bool> &walkability)
	{
		u16 h = BWTA::MapData::mapHeight * 4;
		u16 w = BWTA::MapData::mapWidth * 4;
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
		BWTA::MapData::mapFileName = strName.substr(found + 1);
		std::cout << "START PARSING FILE " << BWTA::MapData::mapFileName << std::endl;

		DWORD dataSize = 0;
		unsigned char* CHKdata = BWTA::extractCHKfile(mapFilePath, &dataSize);
		if (CHKdata == NULL) return 0;

		std::cout << "Successfully extracted the CHK file (size " << dataSize << ")" << std::endl;
		//BWTA::printCHKchunks(CHKdata, dataSize);

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
		SFileReadFile(hFile, data.data(), fileSize, &read, 0);

		// Calculate hash
		sha1::calc(data.data(), fileSize, hash);
		sha1::toHexString(hash, hexstring);

		// Save and close
		BWTA::MapData::hash = std::string(hexstring);
		SFileCloseFile(hFile);

		// Load map dimensions
		// ====================
		unsigned int width = 0, height = 0;
		BWTA::getDimensions(CHKdata, dataSize, width, height);
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
		std::string cv5FileName = "tileset/" + BWTA::tileSetName[tileset] + ".cv5";
		BWTA::MapData::TileSet = (TileType*)BWTA::getFileBuffer(cv5FileName.c_str());


		// Load MiniTileFlags file (tileSetName[tileset].vf4) into BWTA::MapData::MiniTileFlags
		std::string vf4FileName = "tileset/" + BWTA::tileSetName[tileset] + ".vf4";
		BWTA::MapData::MiniTileFlags = (BWTA::MapData::MiniTileMaps_type*)BWTA::getFileBuffer(vf4FileName.c_str());


		// Load Map Tiles
		DWORD chunkSize = 0;
		BWTA::MapData::TileArray = (TileID*)BWTA::getChunkPointer((unsigned char *)"MTXM", CHKdata, dataSize, &chunkSize);


		// Set walkability
		BWTA::MapData::rawWalkability.resize(BWTA::MapData::mapWidth * 4, BWTA::MapData::mapHeight * 4);
		BWTA::setOfflineWalkability(BWTA::MapData::rawWalkability);
		// Test walkability data
		BWTA::MapData::rawWalkability.saveToFile("logs/walkable.txt");


		// Set buildability
		BWTA::MapData::buildability.resize(BWTA::MapData::mapWidth, BWTA::MapData::mapHeight);
		BWTA::setOfflineBuildability(BWTA::MapData::buildability);
		// Test buildability data
		BWTA::MapData::buildability.saveToFile("logs/buildable.txt");


		delete CHKdata;
		std::cout << "END PARSING FILE" << std::endl;
		return 1;
	}

}