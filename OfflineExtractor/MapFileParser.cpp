#include "MapFileParser.h"

#include <fstream>
#include <direct.h>
#include <StormLib.h>
#include "../BWTA/Source/filesystem/path.h"
#include "sha1.h"
#include "TileSet.h"
#include "MiniTileFlags.h"
#include "ChkDataTypes.h"

namespace BWTA
{
	bool VERBOSE = false;

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
	Goes over the CHK data printing the different chunk types and their lengths
	(this function is for debugging only)
	*/
	void printCHKchunks(unsigned char *CHKdata, DWORD CHKdataSize) {
		DWORD position = 0;

		while (position < CHKdataSize) {
			ChkSection* section = reinterpret_cast<ChkSection*>(CHKdata+position);
			position += sizeof(ChkSection);
			// copy the name to end it with 0
			char chunkName[5] = {0};
			memcpy(chunkName,&section[0].name,4);

			std::cout << "Chunk '" << chunkName << "', size: " << section[0].size << '\n';
			position += section[0].size;
		}
	}

	/*
	Finds a given chunk inside CHK data and returns a pointer to it, and its length (in 'desiredChunkLength')
	*/
	unsigned char* getChunkPointer(const char* desiredChunk, DWORD& chunkSize, unsigned char* CHKdata, DWORD CHKdataSize, DWORD& offset) {
		while (offset < CHKdataSize) {
			ChkSection* section = reinterpret_cast<ChkSection*>(CHKdata+offset);
			offset += sizeof(ChkSection);
			// copy the name to end it with 0
			char chunkName[5] = {0};
			memcpy(chunkName,&section[0].name,4);

			if (strcmp(desiredChunk, reinterpret_cast<char*>(chunkName)) == 0) {
				unsigned char* chunkData = CHKdata + offset;
				chunkSize = section[0].size;
				offset += section[0].size;
				return chunkData;
			}
			offset += section[0].size;
		}
		return nullptr;
	}

	unsigned char* newMTXM(unsigned char* CHKdata, DWORD CHKdataSize, uint32_t width, uint32_t height) {
		uint32_t expectedSize = width * height * 2;
		unsigned char* chunkData = nullptr;

		DWORD chunkSize = 0;
		DWORD offset = 0;
		unsigned char* MTXMdata = getChunkPointer("MTXM", chunkSize, CHKdata, CHKdataSize, offset);

		while (MTXMdata) {
			std::cout << "MTXM found with size " << chunkSize << " (" << expectedSize << " expected)";
			if (chunkSize == expectedSize) {
				chunkData = new unsigned char[chunkSize];
				memcpy(chunkData, MTXMdata, chunkSize);
			} else if (chunkData && chunkSize < expectedSize) {
				memcpy(chunkData, MTXMdata, chunkSize); // overwrite from the begining
				std::cout << " overwrite to previous one from the begining";
			} else {
				std::cout << " omitted";
			}
			std::cout << '\n';

			// search if there is another MTXM chunk
			MTXMdata = getChunkPointer("MTXM", chunkSize, CHKdata, CHKdataSize, offset);
		}
		return chunkData;
	}


	/*
	This function gets the dimensions of a StarCraft map from the CHKdata, and returns them in 'width', and 'height'
	*/
	void getDimensions(unsigned char* CHKdata, DWORD CHKdataSize, unsigned int& width, unsigned int& height) {
		DWORD chunkSize = 0;
		DWORD offset = 0;
		unsigned char* DIMdata = getChunkPointer("DIM ", chunkSize, CHKdata, CHKdataSize, offset);

		if (DIMdata) {
			ChkDim* dim = reinterpret_cast<ChkDim*>(DIMdata);
			width = dim[0].width;
			height = dim[0].height;
		} else {
			std::cout << "[ERROR] Map dimensions not found\n";
		}
	}


	/*
	This function will decode the unit information from a CHK map, and return a list of all the units with their
	types, players and coordinates.
	*/
	void getUnits(unsigned char* CHKdata, DWORD CHKdataSize) {
		DWORD chunkSize = 0;
		DWORD offset = 0;
		unsigned char* UNITdata = getChunkPointer("UNIT", chunkSize, CHKdata, CHKdataSize, offset);

		while (UNITdata) {
			int nUnits = chunkSize / sizeof(ChkUnit);
			std::cout << "UNIT found with " << nUnits << " units\n";

			ChkUnit* chkUnit = reinterpret_cast<ChkUnit*>(UNITdata);
			for (int i = 0; i < nUnits; i++) {
				BWAPI::UnitType unitType(chkUnit[i].ID);
				BWAPI::Position pos(chkUnit[i].x, chkUnit[i].y);
//				BWAPI::WalkPosition unitWalkPosition(unitPosition);
				BWAPI::TilePosition unitTilePosition((pos.x - unitType.dimensionLeft()) / TILE_SIZE, (pos.y - unitType.dimensionUp()) / TILE_SIZE);
				if (VERBOSE)
					std::cout << "Unit " << unitType << " at " << pos.x << "," << pos.y << "\n";

				if (unitType.isMineralField() || unitType == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
					MapData::resources.emplace_back(unitType, unitTilePosition, chkUnit[i].resourceAmount);
				} else if (unitType == BWAPI::UnitTypes::Special_Start_Location) {
					MapData::startLocations.push_back(unitTilePosition);
				} else {
					if (VERBOSE) std::cout << "Ignored unit: " << unitType << " at " << pos.x << "," << pos.y << std::endl;
				}
			}

			// search if there is another UNIT chunk
			UNITdata = getChunkPointer("UNIT", chunkSize, CHKdata, CHKdataSize, offset);
		}
	}

	/*
	This function decode the doodads information from a CHK map, 
	and store the neutral buildings positions in MapData::staticNeutralBuildings
	*/
	void getDoodads(unsigned char* CHKdata, DWORD CHKdataSize) {
		DWORD chunkSize = 0;
		DWORD offset = 0;
		unsigned char* THG2data = getChunkPointer("THG2", chunkSize, CHKdata, CHKdataSize, offset);

		while (THG2data) {
			int nDoodads = chunkSize / sizeof(ChkDoodad);
			std::cout << "THG2 found with " << nDoodads << " doodads\n";

			ChkDoodad* chkDoodad = reinterpret_cast<ChkDoodad*>(THG2data);
			for (int i = 0; i < nDoodads; ++i) {
				bool isSprite = chkDoodad[i].flags >> 12 & 1;

				if (!isSprite) {  // if it is a sprite, the terrain tile info should take care of walkability
					BWAPI::UnitType unitType(chkDoodad[i].ID);
					MapData::staticNeutralBuildings.emplace_back(unitType, BWAPI::Position(chkDoodad[i].x, chkDoodad[i].y));
					if (VERBOSE) 
						std::cout << "Doodad " << unitType << " at " << chkDoodad[i].x << "," << chkDoodad[i].y << std::endl;
				} else {
					if (VERBOSE) 
						std::cout << "Ignored Sprite " << chkDoodad[i].ID << " at " << chkDoodad[i].x << "," << chkDoodad[i].y <<  std::endl;
				}
			}

			// search if there is another THG2 chunk
			THG2data = getChunkPointer("THG2", chunkSize, CHKdata, CHKdataSize, offset);
		}
	}

	/*
	This function returns the tilset ID of a StarCraft map from the CHKdata
	*/
	unsigned int getTileset(unsigned char* CHKdata, DWORD size) {
		DWORD chunkSize = 0;
		DWORD offset = 0;
		unsigned char* ERAdata = getChunkPointer("ERA ", chunkSize, CHKdata, size, offset);

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
//			if (id > 1024) std::cout << "Doodad " << id << " found at " << x << "," << y << "\n";
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
		// make sure log folder exists
		filesystem::path logFolder(filesystem::path::get_cwd() / "logs");
		if (!logFolder.exists()) logFolder.mkdirp();

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
		std::cout << "Map size: " << width << "x" << height << "\n";
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
		unsigned char* mtxm = newMTXM(CHKdata, dataSize, width, height);
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