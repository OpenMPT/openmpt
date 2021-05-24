/*
 * ColorPickerButton.h
 * -------------------
 * Purpose: A button for picking UI colors
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "../soundlib/Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;

class ColorPickerButton : public CButton
{
public:
	void SetColor(COLORREF color);
	std::optional<COLORREF> PickColor(const CSoundFile &sndFile, CHANNELINDEX chn);

protected:
	COLORREF m_color = 0;

	void DrawItem(DRAWITEMSTRUCT *dis) override;
};

OPENMPT_NAMESPACE_END
