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

#include "open.h"
#include "arch_raw.h"
#include "arch_gzip.h"
#include "arch_zip.h"
#include "arch_rar.h"

Archive* OpenArchive(const string& aFileName)
{
	string lExt;
	uint32 lPos;

	lPos = aFileName.find_last_of('.');
	if(lPos < 0)
		return NULL;
	lExt = aFileName.substr(lPos);
	for(uint32 i = 0; i < lExt.length(); i++)
		lExt[i] = tolower(lExt[i]);

	if (lExt == ".mdz")
		return new arch_Zip(aFileName);
	if (lExt == ".mdr")
		return new arch_Rar(aFileName);
	if (lExt == ".mdgz")
		return new arch_Gzip(aFileName);

	return new arch_Raw(aFileName);
}