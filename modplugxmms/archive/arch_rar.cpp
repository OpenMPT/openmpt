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

//open()
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

#include "arch_rar.h"
#include <iostream>
#include <procbuf.h>
	
arch_Rar::arch_Rar(const string& aFileName)
{
	//check if file exists
	int lFileDesc = open(aFileName.c_str(), O_RDONLY);

	if(lFileDesc == -1)
	{
		mSize = 0;
		return;
	}
	
	close(lFileDesc);
	
	procbuf lPipeBuf;
	string lCommand = "unrar l " + aFileName;   //get info
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
	lPipe >> lCommand;      //ignore a word.
	lPipe >> mSize;         //this is our size.
	
	lPipeBuf.close();
	
	mMap = new char[mSize];
	if(mMap == NULL)
	{
		mSize = 0;
		return;
	}
	
	lCommand = "unrar p " + aFileName;  //decompress to stdout
	if(!lPipeBuf.open(lCommand.c_str(), ios::in))
	{
		mSize = 0;
		return;
	}
	
	//unrar was obviously written by someone clueless in the ways of unix. :(
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	lPipe.ignore(90, '\n'); //ignore a line.
	
	lPipe.read(mMap, mSize);
	
	lPipeBuf.close();
}

arch_Rar::~arch_Rar()
{
	if(mSize != 0)
		delete [] mMap;
}
