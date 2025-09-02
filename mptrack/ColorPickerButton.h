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
	std::optional<COLORREF> PickChannelColor(const CSoundFile &sndFile, CHANNELINDEX chn);
	std::optional<COLORREF> PickPatternColor(const mpt::span<COLORREF> patternColors, PATTERNINDEX pat);

protected:
	void DrawItem(DRAWITEMSTRUCT *dis) override;

	std::optional<COLORREF> PickColor(mpt::span<COLORREF> colors, std::map<COLORREF, int> usedColors);

	COLORREF m_color = 0;
};

OPENMPT_NAMESPACE_END
