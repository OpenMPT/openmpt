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

#include "arch_gzip.h"
#include <iostream>
#include <procbuf.h>
	
arch_Gzip::arch_Gzip(const string& aFileName)
{
	//check if file exists
	int lFileDesc = open(aFileName.c_str(), O_RDONLY);

	if(lFileDesc == -1)
	{
		mSize = 0;
		return;
	}
	
	close(lFileDesc);
	
	//ok, continue
	procbuf lPipeBuf;
	string lCommand = "gunzip -l \"" + aFileName + '\"';   //get info
	iostream lPipe(&lPipeBuf);
	if(!lPipeBuf.open(lCommand.c_str(), ios::in))
	{
		mSize = 0;
		return;
	}
	
	lPipe.ignore(80, '\n'); //ignore a line.
	lPipe >> mSize;         //ignore a number.
	lPipe >> mSize;         //this is our size.
	
	lPipeBuf.close();
	
	mMap = new char[mSize];
	if(mMap == NULL)
	{
		mSize = 0;
		return;
	}
	
	lCommand = "gunzip -c \"" + aFileName + '\"';  //decompress to stdout
	if(!lPipeBuf.open(lCommand.c_str(), ios::in))
	{
		mSize = 0;
		return;
	}
	
	lPipe.read(mMap, mSize);
	
	lPipeBuf.close();
}

arch_Gzip::~arch_Gzip()
{
	if(mSize != 0)
		delete [] (char*)mMap;
}

bool arch_Gzip::ContainsMod(const string& aFileName)
{
	string lName;
	int lFileDesc = open(aFileName.c_str(), O_RDONLY);
	char lBuffer[257];

	if(lFileDesc == -1)
		return false;
	
	close(lFileDesc);
	
	procbuf lPipeBuf;
	string lCommand = "gunzip -l \"" + aFileName + '\"';   //get info
	iostream lPipe(&lPipeBuf);
	if(!lPipeBuf.open(lCommand.c_str(), ios::in))
		return false;
	
	lPipe.ignore(80, '\n'); //ignore a line.
	lPipe >> lName;         //ignore a number.
	lPipe >> lName;         //ignore size.
	lPipe >> lName;         //ignore ratio.
	lPipe.getline(lBuffer, 257);
	lName = lBuffer;
	
	return IsOurFile(lName);
}
