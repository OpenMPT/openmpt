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

#ifndef __MODPLUG_ARCH_BZIP2_H__INCLUDED__
#define __MODPLUG_ARCH_BZIP2_H__INCLUDED__

#include "archive.h"
#include <string>

class arch_Bzip2: public Archive
{
public:
	arch_Bzip2(const string& aFileName);
	virtual ~arch_Bzip2();
	
	static bool ContainsMod(const string& aFileName);
};

#endif
