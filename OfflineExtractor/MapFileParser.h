#pragma once

#include "../BWTA/Source/MapData.h"

namespace BWTA
{
	const std::string tileSetName[9] = { "badlands", "platform", "install", "ashworld",
										 "jungle", "desert", "ice", "twilight", "Unknown" };

	unsigned char* extractCHKfile(const char* archive, DWORD* dataSize);
	void printCHKchunks(unsigned char* CHKdata, DWORD CHKdataSize);
	unsigned char* getChunkPointer(const char* desiredChunk, DWORD& chunkSize, unsigned char* CHKdata, DWORD CHKdataSize, DWORD& offset);

	void getDimensions(unsigned char* CHKdata, DWORD CHKdataSize, unsigned int& width, unsigned int& height);
	void getUnits(unsigned char* CHKdata, DWORD CHKdataSize);
	void getDoodads(unsigned char* CHKdata, DWORD CHKdataSize);
	unsigned int getTileset(unsigned char* CHKdata, DWORD CHKdataSize);
	unsigned char* getFileBuffer(const char* filename);

	void setOfflineWalkability(RectangleArray<bool> &walkability);
	void setOfflineBuildability(RectangleArray<bool> &buildability);

	bool parseMapFile(const char* mapFilePath);
}
