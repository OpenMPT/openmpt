/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *          Colin DeVilbiss <crdevilb@mtu.edu>
 *
 * This source code is public domain.
 */

// BZ2 support added by Colin DeVilbiss <crdevilb@mtu.edu>

//open()
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

#include "arch_bz2.h"
#include <iostream>
#include <procbuf.h>
 	
arch_Bzip2::arch_Bzip2(const string& aFileName)
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
	string lCommand = "bzcat \'" + aFileName + "\' | wc -c";   //get info
	iostream lPipe(&lPipeBuf);
	if(!lPipeBuf.open(lCommand.c_str(), ios::in))
	{
		mSize = 0;
		return;
	}
	
	lPipe >> mSize;         //this is our size.
	
	lPipeBuf.close();
	
	mMap = new char[mSize];
	if(mMap == NULL)
	{
		mSize = 0;
		return;
	}
	
	lCommand = "bzcat \'" + aFileName + '\'';  //decompress to stdout
	if(!lPipeBuf.open(lCommand.c_str(), ios::in))
	{
		mSize = 0;
		return;
	}
	
	lPipe.read(mMap, mSize);
	
	lPipeBuf.close();
}

arch_Bzip2::~arch_Bzip2()
{
	if(mSize != 0)
		delete [] (char*)mMap;
}

bool arch_Bzip2::ContainsMod(const string& aFileName)
{
	string lName;
	int lFileDesc = open(aFileName.c_str(), O_RDONLY);
 	if(lFileDesc == -1)
		return false;
	
	close(lFileDesc);
	
	lName = aFileName.substr(0, aFileName.find_last_of('.'));
	return IsOurFile(lName);
}
