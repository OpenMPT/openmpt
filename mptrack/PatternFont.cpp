/*
 * PatternFont.cpp
 * ---------------
 * Purpose: Code for creating pattern font bitmaps
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "PatternFont.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "TrackerSettings.h"

OPENMPT_NAMESPACE_BEGIN

//////////////////////////////////////////////
// Font Definitions

// Medium Font (Default)
const PATTERNFONT gDefaultPatternFont = 
{
	nullptr,
	92,13,	// Column Width & Height
	0,0,	// Clear location
	130,8,	// Space Location.
	{20, 20, 24, 9, 15},		// Element Widths
	{0, 0, 0, 0, 0},			// Padding pixels contained in element width
	20,13,	// Numbers 0-F (hex)
	30,13,	// Numbers 10-29 (dec)
	64,26,	// A-M#
	78,26,	// N-Z?										// MEDIUM FONT !!!
	0, 0,
	12,8,	// Note & Octave Width
	42,13,	// Volume Column Effects
	8,8,
	-1,
	8,		// 8+7 = 15
	-3, -1, 12,
	1,		// Margin for first digit of PC event parameter number
	2,		// Margin for first digit of PC event parameter value
	1,		// Margin for second digit of parameter
	13,		// Vertical spacing between letters in the bitmap
};


//////////////////////////////////////////////////
// Small Font

const PATTERNFONT gSmallPatternFont = 
{
	nullptr,
	70,11,	// Column Width & Height
	92,0,	// Clear location
	130,8,	// Space Location.
	{16, 14, 18, 7, 11},		// Element Widths
	{0, 0, 0, 0, 0},			// Padding pixels contained in element width
	108,13,	// Numbers 0-F (hex)
	120,13,	// Numbers 10-29 (dec)
	142,26,	// A-M#
	150,26,	// N-Z?										// SMALL FONT !!!
	92, 0,	// Notes
	10,6,	// Note & Octave Width
	132,13,	// Volume Column Effects
	6,5,
	-1,
	6,		// 8+7 = 15
	-3,	1, 9,	// InstrOfs + nInstrHiWidth
	1,		// Margin for first digit of PC event parameter number
	2,		// Margin for first digit of PC event parameter value
	1,		// Margin for second digit of parameter
	13,		// // Vertical spacing between letters in the bitmap
};

// NOTE: See also CViewPattern::DrawNote() when changing stuff here
// or adding new fonts - The custom tuning note names might require
// some additions there.

const PATTERNFONT *PatternFont::currentFont = nullptr;

static MODPLUGDIB customFontBitmap;

static void DrawChar(HDC hDC, WCHAR ch, int x, int y, int w, int h)
//-----------------------------------------------------------------
{
	CRect rect(x, y, x + w, y + h);
	::DrawTextW(hDC, &ch, 1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void DrawChar(HDC hDC, CHAR ch, int x, int y, int w, int h)
//----------------------------------------------------------------
{
	CRect rect(x, y, x + w, y + h);
	::DrawTextA(hDC, &ch, 1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

template<typename char_t>
static void DrawString(HDC hDC, const char_t *text, int len, int x, int y, int w, int h)
//--------------------------------------------------------------------------------------
{
	for(int i = 0; i < len; i++)
	{
		DrawChar(hDC, text[i], x, y, w, h);
		x += w;
	}
}

void PatternFont::UpdateFont(CDC *dc)
//-----------------------------------
{
	const std::string name = TrackerSettings::Instance().patternFont;
	int32_t fontSize = TrackerSettings::Instance().patternFontSize;
	int32_t flags = TrackerSettings::Instance().patternFontFlags;
	const PATTERNFONT *builtinFont = nullptr;
	if(name == PATTERNFONT_SMALL || name.empty())
	{
		builtinFont = &gSmallPatternFont;
	} else if(name == PATTERNFONT_LARGE)
	{
		builtinFont = &gDefaultPatternFont;
	}

	if(builtinFont != nullptr && fontSize < 1)
	{
		currentFont = builtinFont;
		return;
	}

	static PATTERNFONT pf = { 0 };
	currentFont = &pf;

	static std::string previousFont;
	static int32_t previousSize, previousFlags;
	if(previousFont == name && previousSize == fontSize && previousFlags == flags)
	{
		// Nothing to do
		return;
	}
	previousFont = name;
	previousSize = fontSize;
	previousFlags = flags;
	DeleteFontData();
	pf.dib = &customFontBitmap;

	// Upscale built-in font?
	if(builtinFont != nullptr)
	{
		// Copy and scale original 4-bit bitmap
		LimitMax(fontSize, 10);
		fontSize++;
		MemCopy(customFontBitmap.bmiHeader, CMainFrame::bmpNotes->bmiHeader);
		customFontBitmap.bmiHeader.biWidth *= fontSize;
		customFontBitmap.bmiHeader.biHeight *= fontSize;
		customFontBitmap.bmiHeader.biSizeImage = customFontBitmap.bmiHeader.biWidth * customFontBitmap.bmiHeader.biHeight / 2;
		customFontBitmap.lpDibBits = new uint8_t[customFontBitmap.bmiHeader.biSizeImage];

		// Upscale the image (ugly code ahead)
		const uint8_t *origPixels = CMainFrame::bmpNotes->lpDibBits;
		uint8_t *scaledPixels = customFontBitmap.lpDibBits;
		const int bytesPerLine = customFontBitmap.bmiHeader.biWidth / 2, scaleBytes = bytesPerLine * fontSize;
		bool outPos = false;
		for(int y = 0; y < CMainFrame::bmpNotes->bmiHeader.biHeight; y++, scaledPixels += scaleBytes - bytesPerLine)
		{
			for(int x = 0; x < CMainFrame::bmpNotes->bmiHeader.biWidth; x++)
			{
				uint8_t pixel = *origPixels;
				if(x % 2u == 0)
				{
					pixel >>= 4;
				} else
				{
					pixel &= 0x0F;
					origPixels++;
				}
				for(int scaleX = 0; scaleX < fontSize; scaleX++)
				{
					if(!outPos)
					{
						for(int scaleY = 0; scaleY < scaleBytes; scaleY += bytesPerLine)
						{
							scaledPixels[scaleY] = pixel << 4;
						}
					} else
					{
						for(int scaleY = 0; scaleY < scaleBytes; scaleY += bytesPerLine)
						{
							scaledPixels[scaleY] |= pixel;
						}
						scaledPixels++;
					}
					outPos = !outPos;
				}
			}
		}
		pf.nWidth = (builtinFont->nWidth - 4) * fontSize + 4;
		pf.nHeight = builtinFont->nHeight * fontSize;
		pf.nClrX = builtinFont->nClrX * fontSize;
		pf.nClrY = builtinFont->nClrY * fontSize;
		pf.nSpaceX = builtinFont->nSpaceX * fontSize;
		pf.nSpaceY = builtinFont->nSpaceY * fontSize;
		for(size_t i = 0; i < CountOf(pf.nEltWidths); i++)
		{
			pf.nEltWidths[i] = builtinFont->nEltWidths[i] * fontSize;
			pf.padding[i] = builtinFont->padding[i] * fontSize;
		}
		pf.nNumX = builtinFont->nNumX * fontSize;
		pf.nNumY = builtinFont->nNumY * fontSize;
		pf.nNum10X = builtinFont->nNum10X * fontSize;
		pf.nNum10Y = builtinFont->nNum10Y * fontSize;
		pf.nAlphaAM_X = builtinFont->nAlphaAM_X * fontSize;
		pf.nAlphaAM_Y = builtinFont->nAlphaAM_Y * fontSize;
		pf.nAlphaNZ_X = builtinFont->nAlphaNZ_X * fontSize;
		pf.nAlphaNZ_Y = builtinFont->nAlphaNZ_Y * fontSize;
		pf.nNoteX = builtinFont->nNoteX * fontSize;
		pf.nNoteY = builtinFont->nNoteY * fontSize;
		pf.nNoteWidth = builtinFont->nNoteWidth * fontSize;
		pf.nOctaveWidth = builtinFont->nOctaveWidth * fontSize;
		pf.nVolX = builtinFont->nVolX * fontSize;
		pf.nVolY = builtinFont->nVolY * fontSize;
		pf.nVolCmdWidth = builtinFont->nVolCmdWidth * fontSize;
		pf.nVolHiWidth = builtinFont->nVolHiWidth * fontSize;
		pf.nCmdOfs = builtinFont->nCmdOfs * fontSize;
		pf.nParamHiWidth = builtinFont->nParamHiWidth * fontSize;
		pf.nInstrOfs = builtinFont->nInstrOfs * fontSize;
		pf.nInstr10Ofs = builtinFont->nInstr10Ofs * fontSize;
		pf.nInstrHiWidth = builtinFont->nInstrHiWidth * fontSize;
		pf.pcParamMargin = builtinFont->pcParamMargin * fontSize;
		pf.pcValMargin = builtinFont->pcValMargin * fontSize;
		pf.paramLoMargin = builtinFont->paramLoMargin * fontSize;
		pf.spacingY = builtinFont->spacingY * fontSize;

		// Create 4-pixel border
		const int bmWidth2 = pf.dib->bmiHeader.biWidth / 2;
		for(int y = 0, x = pf.nClrY * bmWidth2 + (pf.nClrX + pf.nWidth - 4) / 2; y < pf.nHeight; y++, x += bmWidth2)
		{
			pf.dib->lpDibBits[x] = 0xEC;
			pf.dib->lpDibBits[x + 1] = 0xC4;
		}

		return;
	}

	// Create our own font!
	// Point size to pixels
	fontSize = -MulDiv(fontSize, GetDeviceCaps(dc->m_hDC, LOGPIXELSY), 720);

	CDC hDC;
	hDC.CreateCompatibleDC(dc);

	CFont font;
	font.CreateFont(fontSize, 0, 0, 0, (flags & PatternFontBold) ? FW_BOLD : FW_NORMAL, (flags & PatternFontItalic) ? TRUE : FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, FIXED_PITCH | FF_DONTCARE, name.c_str());
	CFont *oldFont = hDC.SelectObject(&font);

	CPoint pt = hDC.GetTextExtent(_T("W"));
	const int charWidth = pt.x, charHeight = pt.y;
	const int spacing = charWidth / 4;
	const int width = charWidth * 12 + spacing * 2 + 4, height = charHeight * 21;

	pf.nWidth = width;					// Column Width & Height, including 4-pixels border
	pf.nHeight = charHeight;
	pf.nClrX = pf.nClrY = 0;			// Clear (empty note) location
	pf.nSpaceX = charWidth * 7;			// White location (must be big enough)
	pf.nSpaceY = charHeight;
	pf.nEltWidths[0] = charWidth * 3;	// Note
	pf.padding[0] = 0;
	pf.nEltWidths[1] = charWidth * 3 + spacing;	// Instr
	pf.padding[1] = spacing;
	pf.nEltWidths[2] = charWidth * 3 + spacing;	// Volume
	pf.padding[2] = spacing;
	pf.nEltWidths[3] = charWidth;		// Command letter
	pf.padding[3] = 0;
	pf.nEltWidths[4] = charWidth * 2;	// Command param
	pf.padding[4] = 0;
	pf.nNumX = charWidth * 3;			// Vertically-oriented numbers 0x00-0x0F
	pf.nNumY = charHeight;
	pf.nNum10X = charWidth * 4;			// Numbers 10-29
	pf.nNum10Y = charHeight;
	pf.nAlphaAM_X = charWidth * 6;		// Letters A-M +#
	pf.nAlphaAM_Y = charHeight * 2;
	pf.nAlphaNZ_X = charWidth * 7;		// Letters N-Z +?
	pf.nAlphaNZ_Y = charHeight * 2;
	pf.nNoteX = 0;						// Notes ..., C-, C#, ...
	pf.nNoteY = 0;
	pf.nNoteWidth = charWidth * 2;		// Total width of note (C#)
	pf.nOctaveWidth = charWidth;		// Octave Width
	pf.nVolX = charWidth * 8;			// Volume Column Effects
	pf.nVolY = charHeight;
	pf.nVolCmdWidth = charWidth;		// Width of volume effect
	pf.nVolHiWidth = charWidth;			// Width of first character in volume parameter
	pf.nCmdOfs = 0;						// XOffset (-xxx) around the command letter
	pf.nParamHiWidth = charWidth;
	pf.nInstrOfs = -charWidth;
	pf.nInstr10Ofs = 0;
	pf.nInstrHiWidth = charWidth * 2;
	pf.pcParamMargin = 0;
	pf.pcValMargin = 0;
	pf.paramLoMargin = 0;				// Margin for second digit of parameter
	pf.spacingY = charHeight;

	pf.dib->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pf.dib->bmiHeader.biWidth = ((width + 7) & ~7);	// 4-byte alignment
	pf.dib->bmiHeader.biHeight = -(int32_t)height;
	pf.dib->bmiHeader.biSizeImage = pf.dib->bmiHeader.biWidth * height / 2;
	pf.dib->bmiHeader.biPlanes = 1;
	pf.dib->bmiHeader.biBitCount = 4;
	pf.dib->bmiHeader.biCompression = BI_RGB;
	pf.dib->lpDibBits = new uint8_t[pf.dib->bmiHeader.biSizeImage];
	pf.dib->bmiColors[0] = rgb2quad(RGB(0x00, 0x00, 0x00));
	pf.dib->bmiColors[15] = rgb2quad(RGB(0xFF, 0xFF, 0xFF));

	uint8_t *data = nullptr;
	HBITMAP bitmap = ::CreateDIBSection(hDC, (BITMAPINFO *)&pf.dib->bmiHeader, DIB_RGB_COLORS, (void **)&data, nullptr, 0);
	if(data == nullptr)
	{
		hDC.SelectObject(oldFont);
		font.DeleteObject();
		hDC.DeleteDC();
		currentFont = &gDefaultPatternFont;
		return;
	}
	HGDIOBJ oldBitmap = hDC.SelectObject(bitmap);

	hDC.FillSolidRect(0, 0, width - 4, height, RGB(0xFF, 0xFF, 0xFF));
	hDC.SetTextColor(RGB(0x00, 0x00, 0x00));
	hDC.SetBkColor(RGB(0xFF, 0xFF, 0xFF));
	hDC.SetTextAlign(TA_TOP | TA_LEFT);

	// Empty cells (dots - i-th bit set = dot in the i-th column of a cell)
	const uint8_t dots[5] = { 1 | 2 | 4, 2 | 4, 2 | 4, 1, 1 | 2 };
	for(size_t cell = 0, offset = 0; cell < CountOf(dots); cell++)
	{
		uint8_t dot = dots[cell];
		for(int i = 0; dot != 0; i++)
		{
			if(dot & 1) DrawChar(hDC, L'\u00B7', pf.nClrX + offset + i * charWidth, pf.nClrY, charWidth, charHeight);
			dot >>= 1;
		}
		offset += pf.nEltWidths[cell];
	}

	// Notes
	for(int i = 0; i < 12; i++)
	{
		DrawString(hDC, szNoteNames[i], 2, pf.nNoteX, pf.nNoteY + (i + 1) * charHeight, charWidth, charHeight);
	}
	DrawString(hDC, "^^", 2, pf.nNoteX, pf.nNoteY + 13 * charHeight, charWidth, charHeight);
	DrawString(hDC, "==", 2, pf.nNoteX, pf.nNoteY + 14 * charHeight, charWidth, charHeight);
	DrawString(hDC, "PC", 2, pf.nNoteX, pf.nNoteY + 15 * charHeight, charWidth, charHeight);
	DrawString(hDC, "PCs", 3, pf.nNoteX, pf.nNoteY + 16 * charHeight, charWidth, charHeight);
	DrawString(hDC, "~~", 2, pf.nNoteX, pf.nNoteY + 17 * charHeight, charWidth, charHeight);

	// Hex chars
	const char hexChars[] = "0123456789ABCDEF";
	for(int i = 0; i < 16; i++)
	{
		DrawChar(hDC, hexChars[i], pf.nNumX, pf.nNumY + i * charHeight, charWidth, charHeight);
	}
	// Double-digit numbers
	for(int i = 0; i < 20; i++)
	{
		char s[2];
		s[0] = char('1' + i / 10);
		s[1] = char('0' + i % 10);
		DrawString(hDC, s, 2, pf.nNum10X, pf.nNum10Y + i * charHeight, charWidth, charHeight);
	}

	// Volume commands
	const char volEffects[]= " vpcdabuhlrgfe:o";
	STATIC_ASSERT(CountOf(volEffects) - 1 == MAX_VOLCMDS);
	for(int i = 0; i < MAX_VOLCMDS; i++)
	{
		DrawChar(hDC, volEffects[i], pf.nVolX, pf.nVolY + i * charHeight, charWidth, charHeight);
	}

	// Letters A-Z
	for(int i = 0; i < 13; i++)
	{
		DrawChar(hDC, char('A' + i), pf.nAlphaAM_X, pf.nAlphaAM_Y + i * charHeight, charWidth, charHeight);
		DrawChar(hDC, char('N' + i), pf.nAlphaNZ_X, pf.nAlphaNZ_Y + i * charHeight, charWidth, charHeight);
	}
	// Special chars
	DrawChar(hDC, '#', pf.nAlphaAM_X, pf.nAlphaAM_Y + 13 * charHeight, charWidth, charHeight);
	DrawChar(hDC, '?', pf.nAlphaNZ_X, pf.nAlphaNZ_Y + 13 * charHeight, charWidth, charHeight);
	DrawChar(hDC, 'b', pf.nAlphaAM_X, pf.nAlphaAM_Y + 14 * charHeight, charWidth, charHeight);
	DrawChar(hDC, '\\', pf.nAlphaNZ_X, pf.nAlphaNZ_Y + 14 * charHeight, charWidth, charHeight);
	DrawChar(hDC, '-', pf.nAlphaAM_X, pf.nAlphaAM_Y + 15 * charHeight, charWidth, charHeight);
	DrawChar(hDC, ':', pf.nAlphaNZ_X, pf.nAlphaNZ_Y + 15 * charHeight, charWidth, charHeight);

	::GdiFlush();
	std::memcpy(pf.dib->lpDibBits, data, pf.dib->bmiHeader.biSizeImage);
	// Create 4-pixel border
	const int bmWidth2 = pf.dib->bmiHeader.biWidth / 2;
	for(int y = 0, x = width / 2 - 2; y < height; y++, x += bmWidth2)
	{
		pf.dib->lpDibBits[x] = 0xEC;
		pf.dib->lpDibBits[x + 1] = 0xC4;
	}

	hDC.SelectObject(oldBitmap);
	hDC.SelectObject(oldFont);
	font.DeleteObject();
	DeleteBitmap(bitmap);
	hDC.DeleteDC();
}


void PatternFont::DeleteFontData()
//--------------------------------
{
	delete[] customFontBitmap.lpDibBits;
	customFontBitmap.lpDibBits = nullptr;
}

OPENMPT_NAMESPACE_END
