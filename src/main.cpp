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
		if (hasEnding(SFileFindData.cFileName, ".chk")) {
			std::cout << "CHK file found!\n";
			// TODO: do something with it
			// ...
			break;
		}
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

