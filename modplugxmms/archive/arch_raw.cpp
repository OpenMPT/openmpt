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
//mmap()
#include<unistd.h>
#include<sys/mman.h>

#include "arch_raw.h"

arch_Raw::arch_Raw(const string& aFileName)
{
	mFileDesc = open(aFileName.c_str(), O_RDONLY);

	struct stat lStat;

	//open and mmap the file
	if(mFileDesc == -1)
	{
		mSize = 0;
		return;
	}
	fstat(mFileDesc, &lStat);
	mSize = lStat.st_size;

	mMap =
		(uchar*)mmap(0, mSize, PROT_READ,
		MAP_PRIVATE, mFileDesc, 0);
	if(!mMap)
	{
		close(mFileDesc);
		mSize = 0;
		return;
	}
}

arch_Raw::~arch_Raw()
{
	if(mSize != 0)
	{
		munmap(mMap, mSize);
		close(mFileDesc);
	}
}
