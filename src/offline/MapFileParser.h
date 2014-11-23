#pragma once

#include <windows.h>
#include <direct.h>
#include <bitset>
#include <StormLib.h>
#include <iostream>
#include <fstream>

#include "../MapData.h"
#include "TileSet.h"
#include "TileType.h"
#include "MiniTileFlags.h"

namespace BWTA
{
	const std::string tileSetName[9] = {
		"badlands",
		"platform",
		"install",
		"ashworld",
		"jungle",
		"desert",
		"ice",
		"twilight",
		"Unknown"
	};

	unsigned char *extractCHKfile(const char *archive, DWORD *dataSize);
	void printCHKchunks(unsigned char *CHKdata, DWORD size);
	unsigned char *getChunkPointer(unsigned char *desiredChunk, unsigned char *CHKdata, DWORD size, DWORD *desiredChunkLength);

	void getDimensions(unsigned char *CHKdata, DWORD size, unsigned int *width, unsigned int *height);
	void getUnits(unsigned char *CHKdata, DWORD size);
	void getDoodads(unsigned char *CHKdata, DWORD size);
	unsigned int getTileset(unsigned char *CHKdata, DWORD size);
	unsigned char *getFileBuffer(const char *filename);

	void setOfflineWalkability(RectangleArray<bool> &walkability);
	void setOfflineBuildability(RectangleArray<bool> &buildability);
}
