/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

//open()
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

#include "arch_rar.h"
#include <iostream>
#include <procbuf.h>
#include <vector>
	
arch_Rar::arch_Rar(const string& aFileName)
{
	//check if file exists
	int lFileDesc = open(aFileName.c_str(), O_RDONLY);
	char lBuffer[350];
	uint32 lLength;
	uint32 lCount;
	uint32 lPos = 0;
	vector<uint32> lSizes;
	bool lFound = false;
	uint32 lFileNum = 0;
	string lName;

	if(lFileDesc == -1)
	{
		mSize = 0;
		return;
	}
	
	close(lFileDesc);
	
	procbuf lPipeBuf;
	string lCommand = "unrar l \"" + aFileName + '\"';   //get info
	iostream lPipe(&lPipeBuf);
	if(!lPipeBuf.open(lCommand.c_str(), ios::in))
	{
		mSize = 0;
		return;
	}
	
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	
	while(lPipe)
	{
		lPipe.getline(lBuffer, 350);
		if(lBuffer[0] == '-')
			break;
		
		lLength = strlen(lBuffer);
		lCount = 0;
		for(uint32 i = lLength - 1; i > 0; i--)
		{
			if(lBuffer[i] == ' ')
			{
				lBuffer[i] = 0;
				if(lBuffer[i - 1] != ' ')
				{
					lCount++;
					if(lCount == 9)
					{
						lPos = i;
						break;
					}
				}
			}
		}
		
		while(lBuffer[lPos] == '\0')
			lPos++;
		
		lName = lBuffer;
		mSize = strtol(lBuffer + lPos, NULL, 10);
		
		if(IsOurFile(lName))
		{
			lFound = true;
			break;
		}
		
		lSizes.insert(lSizes.end(), mSize);
		lFileNum++;
	}
	
	if(!lFound)
	{
		mSize = 0;
		return;
	}
	
	lPipeBuf.close();
	
	mMap = new char[mSize];
	if(mMap == NULL)
	{
		mSize = 0;
		return;
	}
	
	lCommand = "unrar p -inul \"" + aFileName + '\"';  //decompress to stdout
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

arch_Rar::~arch_Rar()
{
	if(mSize != 0)
		delete [] (char*)mMap;
}

bool arch_Rar::ContainsMod(const string& aFileName)
{
	//check if file exists
	string lName;
	int lFileDesc = open(aFileName.c_str(), O_RDONLY);
	char lBuffer[350];
	uint32 lLength;
	uint32 lCount;

	if(lFileDesc == -1)
		return false;
	
	close(lFileDesc);
	
	procbuf lPipeBuf;
	string lCommand = "unrar l \"" + aFileName + '\"';   //get info
	iostream lPipe(&lPipeBuf);
	if(!lPipeBuf.open(lCommand.c_str(), ios::in))
		return false;
	
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	
	while(lPipe)
	{
		lPipe.getline(lBuffer, 350);
		if(lBuffer[0] == '-')
			break;
		
		lLength = strlen(lBuffer);
		lCount = 0;
		for(uint32 i = lLength - 1; i > 0; i--)
		{
			if(lBuffer[i] == ' ')
			{
				lBuffer[i] = 0;
				if(lBuffer[i - 1] != ' ')
				{
					lCount++;
					if(lCount == 9)
						break;
				}
			}
		}
		
		lName = lBuffer;
		
		if(IsOurFile(lName))
			return true;
	}
	
	return false;
}
