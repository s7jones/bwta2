#include <iostream>
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

int main ()
{
	std::cout << "Testing standalone BWTA\n";

	HANDLE hMpq       = NULL;          // Open archive handle
	HANDLE hFileFind  = NULL;          // Archived file handle
	SFILE_FIND_DATA SFileFindData;     // Data of the archived file

	const char * archive = (char *)"path01.scx";

	// Open an archive
	if(!SFileOpenArchive(archive, 0, 0, &hMpq)) {
		printError(archive, "Cannot open archive", archive, GetLastError());
		return -1;
	}

	// List all files in archive
	hFileFind = SFileFindFirstFile(hMpq, "*", &SFileFindData, NULL);
	while ( hFileFind ) {
		std::cout << SFileFindData.cFileName << "\n";
		//TODO look for the .chk file
		if ( ! SFileFindNextFile(hFileFind, &SFileFindData) )
			break;
	}

	// Closing handles
	if ( hFileFind != (HANDLE)0xFFFFFFFF ) 
		SFileFindClose(hFileFind);
	if ( hMpq != (HANDLE)0xFFFFFFFF ) 
		SFileCloseArchive(hMpq);

	return 0;

}