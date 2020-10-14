/*
 * CImageListEx.h
 * --------------
 * Purpose: A class that extends MFC's CImageList to handle alpha-blended images properly. Also provided 1-bit transparency fallback when needed.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"
#include "../common/mptSpan.h"

OPENMPT_NAMESPACE_BEGIN

class CImageListEx : public CImageList
{
public:
	bool Create(UINT resourceID, int cx, int cy, int nInitial, int nGrow, CDC *dc, double scaling, bool disabled, const mpt::span<const int> invertImages = {});
};

OPENMPT_NAMESPACE_END
