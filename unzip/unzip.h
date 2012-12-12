/*
 * unzip.h
 * -------
 * Purpose: Header file for .zip loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

//===============
class CZipArchive
//===============
{
protected:
	FileReader inFile, outFile;
	void *zipFile;
	const char *extensions;
	
public:

	FileReader GetOutputFile() const { return outFile; }
	bool IsArchive() const;
	bool ExtractFile();
	void *GetComments(bool get);

	CZipArchive(FileReader &file, const char *ext);
	~CZipArchive();
};
