/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

//open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "arch_zip.h"
#include <iostream>
#include <procbuf.h>
#include <vector>
	
arch_Zip::arch_Zip(const string& aFileName)
{
	//check if file exists
	int lFileDesc = open(aFileName.c_str(), O_RDONLY);
	uint32 lFileNum = 0;
	vector<uint32> lSizes;
	string lName;
	char lBuffer[257];
	bool bFound = false;

	if(lFileDesc == -1)
	{
		mSize = 0;
		return;
	}
	
	close(lFileDesc);
	
	procbuf lPipeBuf;
	string lCommand = "unzip -l -qq \"" + aFileName + '\"';   //get info
	iostream lPipe(&lPipeBuf);
	if(!lPipeBuf.open(lCommand.c_str(), ios::in))
	{
		mSize = 0;
		return;
	}
	
	while(lPipe)
	{
		//damnit, procbufs throw exceptions.  NO!  Exceptions are EVIL!
		lPipe >> lBuffer;             //this is our size.
		if(lBuffer[0] == '-')
			break;                     //no more files
		mSize = strtol(lBuffer, NULL, 10);
		lPipe >> lName;               //ignore date.
		lPipe >> lName;               //ignore time.
		lPipe.getline(lBuffer, 257);  //this is the name.
		lName = lBuffer;
		
		if(IsOurFile(lName))    //is it a mod?
		{
			bFound = true;
			break;
		}
		
		lSizes.insert(lSizes.end(), mSize);
		lFileNum++;
	}
	if(!bFound)
	{
		mSize = 0;
		return;
	}
	
	lPipeBuf.close();
	
	mMap = new char[mSize];
	
	lCommand = "unzip -p \"" + aFileName + '\"';  //decompress to stdout
	if(!lPipeBuf.open(lCommand.c_str(), ios::in))
	{
		mSize = 0;
		return;
	}
	
	for(uint32 i = 0; i < lFileNum; i++)
	{
		lPipe.ignore(lSizes[i]);
	}
	
	lPipe.read(mMap, mSize);
	lPipeBuf.close();
}

arch_Zip::~arch_Zip()
{
	if(mSize != 0)
		delete [] (char*)mMap;
}
	
bool arch_Zip::ContainsMod(const string& aFileName)
{
	int lFileDesc = open(aFileName.c_str(), O_RDONLY);
	string lName;
	char lBuffer[257];

	if(lFileDesc == -1)
		return false;
	
	close(lFileDesc);
	
	procbuf lPipeBuf;
	string lCommand = "unzip -l -qq \"" + aFileName + '\"';   //get info
	iostream lPipe(&lPipeBuf);
	if(!lPipeBuf.open(lCommand.c_str(), ios::in))
		return false;
	
	while(lPipe)
	{
		lPipe >> lName;               //ignore size
		if(lName[0] == '-')
			return false;              //no more files
		lPipe >> lName;               //ignore date.
		lPipe >> lName;               //ignore time.
		lPipe.getline(lBuffer, 257);  //this is the name.
		lName = lBuffer;
		
		if(IsOurFile(lName))    //is it a mod?
			return true;
	}
	
	return false;
}
