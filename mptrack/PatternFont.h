/*
 * PatternFont.h
 * -------------
 * Purpose: Code for creating pattern font bitmaps
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

struct MODPLUGDIB;

struct PATTERNFONT
{
	MODPLUGDIB *dib;
	MODPLUGDIB *dibASCII;  // optional
	int nWidth, nHeight;		// Column Width & Height, including 4-pixels border
	int nClrX, nClrY;			// Clear (empty note) location
	int nSpaceX, nSpaceY;		// White location (must be big enough)
	UINT nEltWidths[5];			// Elements Sizes
	UINT padding[5];			// Padding pixels contained in element width
	int nNumX, nNumY;			// Vertically-oriented numbers 0x00-0x0F
	int nNum10X, nNum10Y;		// Numbers 10-29
	int nAlphaAM_X,nAlphaAM_Y;	// Letters A-M +#
	int nAlphaNZ_X,nAlphaNZ_Y;	// Letters N-Z +?
	int nNoteX, nNoteY;			// Notes ..., C-, C#, ...
	int nNoteWidth[2];			// Total width of note (C#)
	int nOctaveWidth;			// Octave Width
	int nVolX, nVolY;			// Volume Column Effects
	int nVolCmdWidth;			// Width of volume effect
	int nVolHiWidth;			// Width of first character in volume parameter
	int nCmdOfs;				// XOffset (-xxx) around the command letter
	int nParamHiWidth;
	int nInstrOfs, nInstr10Ofs, nInstrHiWidth;
	int pcParamMargin;			// Margin for first digit of PC event parameter number
	int pcValMargin;			// Margin for first digit of PC event parameter value
	int paramLoMargin;			// Margin for second digit of parameter
	int spacingY;				// Vertical spacing between letters in the bitmap
};

class PatternFont
{
public:
	static const PATTERNFONT *currentFont;

	static void UpdateFont(HWND hwnd);
	static void DeleteFontData();
};

OPENMPT_NAMESPACE_END