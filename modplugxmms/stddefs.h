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

// stddefs.h: Standard defs to streamline coding style.
//
//////////////////////////////////////////////////////////////////////

#if !defined(__MODPLUGXMMS_STDDEFS_H__INCLUDED__)
#define __MODPLUGXMMS_STDDEFS_H__INCLUDED__

//Invalid pointer
#ifndef NULL
#define NULL 0
#endif

//Standard types. ----------------------------------------
//These data types are provided because the standard types vary across
// platforms.  For example, long is 64-bit on some 64-bit systems.
//u = unsigned
//# = size in bits
typedef unsigned char          uchar;

typedef char                   int8;
typedef short                  int16;
typedef int                    int32;
typedef long long              int64;

typedef unsigned char          uint8;
typedef unsigned short         uint16;
typedef unsigned int           uint32;
typedef unsigned long long     uint64;

typedef float                  float32;
typedef double                 float64;
typedef long double            float128;

#endif // included
