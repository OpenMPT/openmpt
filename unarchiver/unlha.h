
#ifndef UNLHA32_LIB_H
#define UNLHA32_LIB_H

#include "../soundlib/FileReader.h"

typedef struct _LHAInputStream LHAInputStream;
typedef struct _LHAReader LHAReader;
typedef struct _LHAFileHeader LHAFileHeader;

//===============
class CLhaArchive
//===============
{
private:
	FileReader file;
	LHAInputStream *inputstream;
	LHAReader *reader;
	LHAFileHeader *firstfile;
	std::vector<char> data;
public:
	CLhaArchive(FileReader file_);
	~CLhaArchive();
public:
	bool IsArchive();
	FileReader GetOutputFile() const;
	bool ExtractFile();
	bool ExtractFile(const std::vector<const char *> &extensions);
};

#endif // UNLHA32_LIB_H
