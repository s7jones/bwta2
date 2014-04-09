#include <StormLib.h>
#include <direct.h>

// TODO we should instantiate a new Game
namespace BWAPI { Game* Broodwar; }


void printError(const char * archive, const char * message, const char * file, int errnum) {

	char * error = NULL;
	char cCurrentPath[FILENAME_MAX];
	_getcwd(cCurrentPath, sizeof(cCurrentPath));

	switch(errnum) {
		case 105:
			error = (char *)"Bad format"; break;
		case 106:
			error = (char *)"No more files"; break;
		case 107:
			error = (char *)"Handle EOF"; break;
		case 108:
			error = (char *)"Cannot compile"; break;
		case 109:
			error = (char *)"File corrupted"; break;
		default:
			char msg[128];
			strerror_s(msg, sizeof(msg), errnum);
			error = msg;
	}

	std::cout << archive << ": Error: " << message << " \"" << cCurrentPath  << "\\" << file << "\": " << error << "\n";
}


/*
	Check whether the string 'fullString' ends with the string 'ending'
*/
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


/*
	Looks for a file with the .chk extension inside of a .scx map, decompresses it, and returns:
		- a pointer to the data in the .chk file
		- the size of the .chk file (in 'dataSize')
*/
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


/*
	Decodes a 4 byte integer from a string of characters
*/
unsigned long decode4ByteUnsigned(unsigned char *ptr) {
	unsigned long ul = 0;
	ul +=((unsigned long) ptr[0]) << 0;
	ul +=((unsigned long) ptr[1]) << 8;
	ul +=((unsigned long) ptr[2]) << 16;
	ul +=((unsigned long) ptr[3]) << 24;
	return ul;
}


/*
	Decodes a 2 byte integer from a string of characters
*/
unsigned int decode2ByteUnsigned(unsigned char *ptr) {
	unsigned int ui = 0;
	ui +=((unsigned int) ptr[0]) << 0;
	ui +=((unsigned int) ptr[1]) << 8;
	return ui;
}



/*
	Goes over the CHK data printing the different chunk types and their lengths
	(this function is for debugging only)
*/
void printCHKchunks(unsigned char *CHKdata, DWORD size) {
	DWORD position = 0;

	while(position<size) {
		char chunkName[5];
		chunkName[0] = CHKdata[position++];
		chunkName[1] = CHKdata[position++];
		chunkName[2] = CHKdata[position++];
		chunkName[3] = CHKdata[position++];
		chunkName[4] = 0;
		unsigned long chunkLength = decode4ByteUnsigned(CHKdata+position);
		position+=4;

		std::cout << "Chunk '" << chunkName << "', size: " << chunkLength << "\n";

		position+=chunkLength;
	}
}


/*
	Finds a given chunk inside CHK data and returns a pointer to it, and its length (in 'desiredChunkLength')
*/
unsigned char *getChunkPointer(unsigned char *desiredChunk, unsigned char *CHKdata, DWORD size, DWORD *desiredChunkLength) {
	DWORD position = 0;

	while(position<size) {
		char chunkName[5];
		chunkName[0] = CHKdata[position++];
		chunkName[1] = CHKdata[position++];
		chunkName[2] = CHKdata[position++];
		chunkName[3] = CHKdata[position++];
		chunkName[4] = 0;
		unsigned long chunkLength = decode4ByteUnsigned(CHKdata+position);
		position+=4;

		if (strcmp((char *)desiredChunk, (char *)chunkName)==0) {
			*desiredChunkLength = chunkLength;
			return CHKdata + position;
		}
		position+=chunkLength;
	}
	return NULL;
}


/*
	This function gets the dimensions of a StarCraft map from the CHKdata, and returns them in 'width', and 'height'
*/
void getDimensions(unsigned char *CHKdata, DWORD size, unsigned int *width, unsigned int *height) {
	DWORD chunkSize = 0;
	unsigned char *DIMdata = getChunkPointer((unsigned char *)"DIM ", CHKdata, size, &chunkSize);
	
	if (DIMdata!=NULL) {
		*width = decode2ByteUnsigned(DIMdata);
		*height = decode2ByteUnsigned(DIMdata+2);
	}
}


/*
	This function decodes the terrain information from a CHK map:
*/
unsigned int *getTerrain(unsigned char *CHKdata, DWORD size, int width, int height) {
	DWORD chunkSize = 0;
//	unsigned char *TILEdata = getChunkPointer((unsigned char *)"TILE", CHKdata, size, &chunkSize);
	unsigned char *TILEdata = getChunkPointer((unsigned char *)"MTXM", CHKdata, size, &chunkSize);
	
	if (TILEdata!=NULL) {
		std::cout<< "Decoding terrain information...\n";
		unsigned int *terrain = new unsigned int[width*height];
		for(int i = 0, offset = 0;i<height;i++) {
			for(int j = 0;j<width;j++,offset+=2) {
				terrain[i] = decode2ByteUnsigned(TILEdata+offset);
//				std::cout << terrain[i] << " ";
			}
//				std::cout << "\n";
		}
		return terrain;
	}
	return NULL;
}


/*
	*** This function is not finished ***

	This function will decode the unit information from a CHK map, and return a list of all the units with their
	types, players and coordinates.
*/
void getUnits(unsigned char *CHKdata, DWORD size) {
	DWORD chunkSize = 0;
	unsigned char *UNITdata = getChunkPointer((unsigned char *)"UNIT", CHKdata, size, &chunkSize);
	
	if (UNITdata!=NULL) {
		int bytesPerUnit = 36;
		int neutralPlayer = 16;

		int nUnits = chunkSize/bytesPerUnit;
		std::cout << "UNIT chunk successfully found, with information about " << nUnits << " units\n";
		for(int i = 0;i<nUnits;i++) {
			int position = (i*bytesPerUnit);
			unsigned long unitClass = decode4ByteUnsigned(UNITdata+position);
			position+=4;
			unsigned int x = decode2ByteUnsigned(UNITdata+position);
			position+=2;
			unsigned int y = decode2ByteUnsigned(UNITdata+position);
			position+=2;
			unsigned int ID = decode2ByteUnsigned(UNITdata+position);
			position+=2;
			position+=2;	//skip relation to another building
			position+=2;	// skip special property flags
			unsigned int mapEditorFlags = decode2ByteUnsigned(UNITdata+position);
			int playerIsValid = mapEditorFlags & 0x0001;	// If this is 0, it is a neutral unit/critter/start location/etc.
			position+=2;	
			int player = (playerIsValid==1 ? UNITdata[position] : neutralPlayer);
			std::cout << "Unit(" << unitClass << ") ID=" << ID << " at " << x << "," << y << " player " << player << "\n";

			// TODO: how do we translate from the unitClass to a unit type? I have not been able to find information online...
			// ...
		}

	}
}


int main (int argc, char * argv[])
{
	// Check the number of parameters
    if (argc < 2) {
        // Tell the user how to run the program
        std::cerr << "Usage: " << argv[0] << " [mapFile]" << std::endl;
        return 1;
    }

	std::cout << "Testing standalone BWTA\n";
	DWORD dataSize = 0;

	unsigned char *CHKdata = extractCHKfile(argv[1], &dataSize);
	if (CHKdata==NULL) return 0;

	std::cout << "Successfully extracted the CHK file, of size " << dataSize << "\n";
	printCHKchunks(CHKdata, dataSize);

	unsigned int width = 0, height = 0;
	getDimensions(CHKdata, dataSize, &width, &height);
	std::cout << "Map is " << width << "x" << height << "\n";

	unsigned int *terrain = getTerrain(CHKdata, dataSize, width, height);
	getUnits(CHKdata, dataSize);

	// TODO: implement an alternative method to "load_map()" in "load_data.cpp" that uses this instead of 
	//       taking it from the global Broodwar singleton.
	//		 Something that we need to figure out is how to translate the tile information to what is "walkable" or not
	//       and what is "buildable" or not...

	delete CHKdata;
	return 0;
}
