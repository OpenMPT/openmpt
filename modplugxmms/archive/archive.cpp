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

#include "archive.h"

Archive::~Archive()
{
}

bool Archive::IsOurFile(const string& aFileName)
{
	string lExt;
	uint32 lPos;

	lPos = aFileName.find_last_of('.');
	if((int)lPos == -1)
		return false;
	lExt = aFileName.substr(lPos);
	for(uint32 i = 0; i < lExt.length(); i++)
		lExt[i] = tolower(lExt[i]);

	if (lExt == ".669")
		return true;
	if (lExt == ".amf")
		return true;
	if (lExt == ".ams")
		return true;
	if (lExt == ".dbm")
		return true;
	if (lExt == ".dbf")
		return true;
	if (lExt == ".dsm")
		return true;
	if (lExt == ".far")
		return true;
	if (lExt == ".it")
		return true;
	if (lExt == ".mdl")
		return true;
	if (lExt == ".med")
		return true;
	if (lExt == ".mod")
		return true;
	if (lExt == ".mtm")
		return true;
	if (lExt == ".okt")
		return true;
	if (lExt == ".ptm")
		return true;
	if (lExt == ".s3m")
		return true;
	if (lExt == ".stm")
		return true;
	if (lExt == ".ult")
		return true;
	if (lExt == ".umx")      //Unreal rocks!
		return true;
	if (lExt == ".xm")
		return true;
	
	return false;
}
