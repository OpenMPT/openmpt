/* Modplug XMMS Plugin
 * Copyright (C) 1999 Kenton Varda and Olivier Lapicque
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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