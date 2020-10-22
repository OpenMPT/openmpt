/*
 * Dither.cpp
 * ----------
 * Purpose: Dithering when converting to lower resolution sample formats.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "Dither.h"

#include "../common/misc_util.h"


OPENMPT_NAMESPACE_BEGIN


mpt::ustring DitherNames::GetModeName(DitherMode mode)
{
	mpt::ustring result;
	switch(mode)
	{
		case DitherNone   : result = U_("no"     ); break;
		case DitherDefault: result = U_("default"); break;
		case DitherModPlug: result = U_("0.5 bit"); break;
		case DitherSimple : result = U_("1 bit"  ); break;
		default           : result = U_(""       ); break;
	}
	return result;
}


OPENMPT_NAMESPACE_END
