#include <iostream>
#include <fstream>
#include <StormLib.h>

// TODO we should instantiate a new Game
namespace BWAPI { Game* Broodwar; }


void printError(const char * archive, const char * message, const char * file, int errnum) {

	char * error = NULL;

	if ( errnum < 105 )
		error = strerror(errnum); //TODO strerror deprecated?
	else if ( errnum == 105 )
		error = (char *)"Bad format";
	else if ( errnum == 106 )
		error = (char *)"No more files";
	else if ( errnum == 107 )
		error = (char *)"Handle EOF";
	else if ( errnum == 108 )
		error = (char *)"Cannot compile";
	else if ( errnum == 109 )
		error = (char *)"File corrupted";

	std::cout << archive << ": Error: " << message << "(" << file << "): " << error << "\n";
}


bool hasEnding (const char *fullString, const char *ending)
{
	int l1 = strlen(fullString);
	int l2 = strlen(ending);
    if (l1 >= l2) {
		int start = l1-l2;
		for(int idx = 0;idx<l2;idx++) {
			if (fullString[start+idx] != ending[idx]) return false;
		}
		return true;
    } else {
        return false;
    }
}

unsigned char *extractCHKfile(const char *archive, DWORD *dataSize) 
{
	HANDLE hMpq       = NULL;          // Open archive handle
	HANDLE hFileFind  = NULL;          // Archived file handle
	SFILE_FIND_DATA SFileFindData;     // Data of the archived file
	bool chkFilefound = false;		   // Whether the chk file was found in the archive
	unsigned char *CHKdata = NULL;

	// Open an archive
	if(!SFileOpenArchive(archive, 0, 0, &hMpq)) {
		printError(archive, "Cannot open archive", archive, GetLastError());
		return NULL;
	}

	// List all files in archive
	hFileFind = SFileFindFirstFile(hMpq, "*", &SFileFindData, NULL);
	while ( hFileFind ) {
		std::cout << SFileFindData.cFileName << "\n";
		if (hasEnding(SFileFindData.cFileName, ".chk")) {
			chkFilefound = true;
			break;
		}
		if ( ! SFileFindNextFile(hFileFind, &SFileFindData) )
			break;
	}

	// extract the chk file to a temporary file:
	// Santi: perhaps there is a better way to extract the CHK file directly form the MPQ
	// into an array of bytes, but I haven't found it. So, for now, I just use a temporary file...
	if (chkFilefound) {
		// std::cout << "CHK file found: " << SFileFindData.cFileName << ", size: " << SFileFindData.dwFileSize << "\n";
		bool success = SFileExtractFile(hMpq, SFileFindData.cFileName, "tmp.chk", SFILE_OPEN_FROM_MPQ);

		if (success) {
			// read the CHK file for the required information:
			CHKdata = new unsigned char[SFileFindData.dwFileSize];
			std::ifstream file ("tmp.chk", std::ios::in | std::ios::binary);
			if (file.is_open()) {
				file.seekg (0, std::ios::beg);
				file.read ((char *)CHKdata, SFileFindData.dwFileSize);
				file.close();
				*dataSize = SFileFindData.dwFileSize;
			}
		}
	}

	// Closing handles
	if ( hFileFind != (HANDLE)0xFFFFFFFF ) 
		SFileFindClose(hFileFind);
	if ( hMpq != (HANDLE)0xFFFFFFFF ) 
		SFileCloseArchive(hMpq);

	return CHKdata;
}


void printCHKchunks(unsigned char *CHKdata, DWORD size) {
	DWORD position = 0;

	while(position<size) {
		char chunkName[5];
		chunkName[0] = CHKdata[position++];
		chunkName[1] = CHKdata[position++];
		chunkName[2] = CHKdata[position++];
		chunkName[3] = CHKdata[position++];
		chunkName[4] = 0;
		unsigned long chunkLength = 0;
		chunkLength +=((unsigned long) CHKdata[position++]) << 0;
		chunkLength +=((unsigned long) CHKdata[position++]) << 8;
		chunkLength +=((unsigned long) CHKdata[position++]) << 16;
		chunkLength +=((unsigned long) CHKdata[position++]) << 24;

		std::cout << "Chunk '" << chunkName << "', size: " << chunkLength << "\n";

		position+=chunkLength;
	}
}




int main ()
{
	std::cout << "Testing standalone BWTA\n";
	DWORD dataSize = 0;

	unsigned char *CHKdata = extractCHKfile("path01.scx", &dataSize);

	std::cout << "Successfully extracted the CHK file, of size " << dataSize << "\n";

	printCHKchunks(CHKdata, dataSize);

	// TODO: read the "MTXM" chunk to get the tile information
	// TODO: read the "UNIT" chunk to get the unit information (e.g. minerals)
	// TODO: implement an alternative method to "load_map()" in "load_data.cpp" that uses this instead of 
	//       taking it from the global Broodwar singleton.

	delete CHKdata;
	return 0;
}
