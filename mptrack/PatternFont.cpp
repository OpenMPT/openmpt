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
#include "../soundlib/Tables.h"

OPENMPT_NAMESPACE_BEGIN

//////////////////////////////////////////////
// Font Definitions

// Medium Font (Default)
static constexpr PATTERNFONT gDefaultPatternFont = 
{
	nullptr,
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
	{ 7, 5 }, 8,	// Note & Octave Width
	42,13,			// Volume Column Effects
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

static constexpr PATTERNFONT gSmallPatternFont =
{
	nullptr,
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
	{ 5, 5 }, 6,	// Note & Octave Width
	132,13,		// Volume Column Effects
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
static MODPLUGDIB customFontBitmapASCII;

static void DrawChar(HDC hDC, WCHAR ch, int x, int y, int w, int h)
{
	CRect rect(x, y, x + w, y + h);
	::DrawTextW(hDC, &ch, 1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

static void DrawChar(HDC hDC, CHAR ch, int x, int y, int w, int h)
{
	CRect rect(x, y, x + w, y + h);
	::DrawTextA(hDC, &ch, 1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

template<typename char_t>
static void DrawString(HDC hDC, const char_t *text, int len, int x, int y, int w, int h)
{
	for(int i = 0; i < len; i++)
	{
		DrawChar(hDC, text[i], x, y, w, h);
		x += w;
	}
}

void PatternFont::UpdateFont(HWND hwnd)
{
	FontSetting font = TrackerSettings::Instance().patternFont;
	const PATTERNFONT *builtinFont = nullptr;
	if(font.name == PATTERNFONT_SMALL || font.name.empty())
	{
		builtinFont = &gSmallPatternFont;
	} else if(font.name == PATTERNFONT_LARGE)
	{
		builtinFont = &gDefaultPatternFont;
	}

	if(builtinFont != nullptr && font.size < 1)
	{
		currentFont = builtinFont;
		return;
	}

	static PATTERNFONT pf = { 0 };
	currentFont = &pf;

	static FontSetting previousFont;
	if(previousFont == font)
	{
		// Nothing to do
		return;
	}
	previousFont = font;
	DeleteFontData();
	pf.dib = &customFontBitmap;
	pf.dibASCII = nullptr;

	// Upscale built-in font?
	if(builtinFont != nullptr)
	{
		// Copy and scale original 4-bit bitmap
		LimitMax(font.size, 10);
		font.size++;
		customFontBitmap.bmiHeader = CMainFrame::bmpNotes->bmiHeader;
		customFontBitmap.bmiHeader.biWidth *= font.size;
		customFontBitmap.bmiHeader.biHeight *= font.size;
		customFontBitmap.bmiHeader.biSizeImage = customFontBitmap.bmiHeader.biWidth * customFontBitmap.bmiHeader.biHeight / 2;
		customFontBitmap.lpDibBits = new uint8[customFontBitmap.bmiHeader.biSizeImage];

		// Upscale the image (ugly code ahead)
		const uint8 *origPixels = CMainFrame::bmpNotes->lpDibBits;
		uint8 *scaledPixels = customFontBitmap.lpDibBits;
		const int bytesPerLine = customFontBitmap.bmiHeader.biWidth / 2, scaleBytes = bytesPerLine * font.size;
		bool outPos = false;
		for(int y = 0; y < CMainFrame::bmpNotes->bmiHeader.biHeight; y++, scaledPixels += scaleBytes - bytesPerLine)
		{
			for(int x = 0; x < CMainFrame::bmpNotes->bmiHeader.biWidth; x++)
			{
				uint8 pixel = *origPixels;
				if(x % 2u == 0)
				{
					pixel >>= 4;
				} else
				{
					pixel &= 0x0F;
					origPixels++;
				}
				for(int scaleX = 0; scaleX < font.size; scaleX++)
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
		pf.nWidth = (builtinFont->nWidth - 4) * font.size + 4;
		pf.nHeight = builtinFont->nHeight * font.size;
		pf.nClrX = builtinFont->nClrX * font.size;
		pf.nClrY = builtinFont->nClrY * font.size;
		pf.nSpaceX = builtinFont->nSpaceX * font.size;
		pf.nSpaceY = builtinFont->nSpaceY * font.size;
		for(std::size_t i = 0; i < std::size(pf.nEltWidths); i++)
		{
			pf.nEltWidths[i] = builtinFont->nEltWidths[i] * font.size;
			pf.padding[i] = builtinFont->padding[i] * font.size;
		}
		pf.nNumX = builtinFont->nNumX * font.size;
		pf.nNumY = builtinFont->nNumY * font.size;
		pf.nNum10X = builtinFont->nNum10X * font.size;
		pf.nNum10Y = builtinFont->nNum10Y * font.size;
		pf.nAlphaAM_X = builtinFont->nAlphaAM_X * font.size;
		pf.nAlphaAM_Y = builtinFont->nAlphaAM_Y * font.size;
		pf.nAlphaNZ_X = builtinFont->nAlphaNZ_X * font.size;
		pf.nAlphaNZ_Y = builtinFont->nAlphaNZ_Y * font.size;
		pf.nNoteX = builtinFont->nNoteX * font.size;
		pf.nNoteY = builtinFont->nNoteY * font.size;
		pf.nNoteWidth[0] = builtinFont->nNoteWidth[0] * font.size;
		pf.nNoteWidth[1] = builtinFont->nNoteWidth[1] * font.size;
		pf.nOctaveWidth = builtinFont->nOctaveWidth * font.size;
		pf.nVolX = builtinFont->nVolX * font.size;
		pf.nVolY = builtinFont->nVolY * font.size;
		pf.nVolCmdWidth = builtinFont->nVolCmdWidth * font.size;
		pf.nVolHiWidth = builtinFont->nVolHiWidth * font.size;
		pf.nCmdOfs = builtinFont->nCmdOfs * font.size;
		pf.nParamHiWidth = builtinFont->nParamHiWidth * font.size;
		pf.nInstrOfs = builtinFont->nInstrOfs * font.size;
		pf.nInstr10Ofs = builtinFont->nInstr10Ofs * font.size;
		pf.nInstrHiWidth = builtinFont->nInstrHiWidth * font.size;
		pf.pcParamMargin = builtinFont->pcParamMargin * font.size;
		pf.pcValMargin = builtinFont->pcValMargin * font.size;
		pf.paramLoMargin = builtinFont->paramLoMargin * font.size;
		pf.spacingY = builtinFont->spacingY * font.size;

		// Create 4-pixel border
		const int bmWidth2 = pf.dib->bmiHeader.biWidth / 2;
		for(int y = 0, x = (customFontBitmap.bmiHeader.biHeight - pf.nClrY - pf.nHeight) * bmWidth2 + (pf.nClrX + pf.nWidth - 4) / 2; y < pf.nHeight; y++, x += bmWidth2)
		{
			pf.dib->lpDibBits[x] = 0xEC;
			pf.dib->lpDibBits[x + 1] = 0xC4;
		}

		return;
	}

	// Create our own font!

	CDC hDC;
	hDC.CreateCompatibleDC(CDC::FromHandle(::GetDC(hwnd)));
	// Point size to pixels
	font.size = -MulDiv(font.size, Util::GetDPIy(hwnd), 720);

	CFont gdiFont;
	gdiFont.CreateFont(font.size, 0, 0, 0, font.flags[FontSetting::Bold] ? FW_BOLD : FW_NORMAL, font.flags[FontSetting::Italic] ? TRUE : FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, FIXED_PITCH | FF_DONTCARE, mpt::ToCString(font.name));
	CFont *oldFont = hDC.SelectObject(&gdiFont);

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
	pf.nNoteWidth[0] = charWidth;		// Total width of note (C#)
	pf.nNoteWidth[1] = charWidth;		// Total width of note (C#)
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

	{

	pf.dib->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pf.dib->bmiHeader.biWidth = ((width + 7) & ~7);	// 4-byte alignment
	pf.dib->bmiHeader.biHeight = -(int32)height;
	pf.dib->bmiHeader.biSizeImage = pf.dib->bmiHeader.biWidth * height / 2;
	pf.dib->bmiHeader.biPlanes = 1;
	pf.dib->bmiHeader.biBitCount = 4;
	pf.dib->bmiHeader.biCompression = BI_RGB;
	pf.dib->lpDibBits = new uint8[pf.dib->bmiHeader.biSizeImage];
	pf.dib->bmiColors[0] = rgb2quad(RGB(0x00, 0x00, 0x00));
	pf.dib->bmiColors[15] = rgb2quad(RGB(0xFF, 0xFF, 0xFF));

	uint8 *data = nullptr;
	HBITMAP bitmap = ::CreateDIBSection(hDC, (BITMAPINFO *)&pf.dib->bmiHeader, DIB_RGB_COLORS, (void **)&data, nullptr, 0);
	if(!bitmap)
	{
		hDC.SelectObject(oldFont);
		gdiFont.DeleteObject();
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
	const uint8 dots[5] = { 1 | 2 | 4, 2 | 4, 2 | 4, 1, 1 | 2 };
	const auto dotStr = TrackerSettings::Instance().patternFontDot.Get();
	auto dotChar = dotStr.empty() ? UC_(' ') : dotStr[0];
	for(int cell = 0, offset = 0; cell < static_cast<int>(std::size(dots)); cell++)
	{
		uint8 dot = dots[cell];
		for(int i = 0; dot != 0; i++)
		{
			if(dot & 1) DrawChar(hDC, dotChar, pf.nClrX + offset + i * charWidth, pf.nClrY, charWidth, charHeight);
			dot >>= 1;
		}
		offset += pf.nEltWidths[cell];
	}

	// Notes
	for(int i = 0; i < 12; i++)
	{
		DrawString(hDC, NoteNamesSharp[i], 2, pf.nNoteX, pf.nNoteY + (i + 1) * charHeight, charWidth, charHeight);
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
	static_assert(mpt::array_size<decltype(volEffects)>::size - 1 == MAX_VOLCMDS);
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
	DrawChar(hDC, '+', pf.nAlphaAM_X, pf.nAlphaAM_Y + 16 * charHeight, charWidth, charHeight);
	DrawChar(hDC, 'd', pf.nAlphaAM_X, pf.nAlphaAM_Y + 17 * charHeight, charWidth, charHeight);

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
	DeleteBitmap(bitmap);

	}

	{

		pf.dibASCII = &customFontBitmapASCII;

		pf.dibASCII->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		pf.dibASCII->bmiHeader.biWidth = ((charWidth * 128 + 7) & ~7);	// 4-byte alignment
		pf.dibASCII->bmiHeader.biHeight = -(int32)charHeight;
		pf.dibASCII->bmiHeader.biSizeImage = pf.dibASCII->bmiHeader.biWidth * charHeight / 2;
		pf.dibASCII->bmiHeader.biPlanes = 1;
		pf.dibASCII->bmiHeader.biBitCount = 4;
		pf.dibASCII->bmiHeader.biCompression = BI_RGB;
		pf.dibASCII->lpDibBits = new uint8[pf.dibASCII->bmiHeader.biSizeImage];
		pf.dibASCII->bmiColors[0] = rgb2quad(RGB(0x00, 0x00, 0x00));
		pf.dibASCII->bmiColors[15] = rgb2quad(RGB(0xFF, 0xFF, 0xFF));

		uint8 *data = nullptr;
		HBITMAP bitmap = ::CreateDIBSection(hDC, (BITMAPINFO *)&pf.dibASCII->bmiHeader, DIB_RGB_COLORS, (void **)&data, nullptr, 0);
		if(!bitmap)
		{
			hDC.SelectObject(oldFont);
			gdiFont.DeleteObject();
			hDC.DeleteDC();
			currentFont = &gDefaultPatternFont;
			return;
		}
		HGDIOBJ oldBitmap = hDC.SelectObject(bitmap);

		hDC.FillSolidRect(0, 0, pf.dibASCII->bmiHeader.biWidth, pf.dibASCII->bmiHeader.biHeight, RGB(0xFF, 0xFF, 0xFF));
		hDC.SetTextColor(RGB(0x00, 0x00, 0x00));
		hDC.SetBkColor(RGB(0xFF, 0xFF, 0xFF));
		hDC.SetTextAlign(TA_TOP | TA_LEFT);

		for(uint32 c = 32; c < 128; ++c)
		{
			DrawChar(hDC, (char)(unsigned char)c, charWidth * c, 0, charWidth, charHeight);
		}
		::GdiFlush();

		std::memcpy(pf.dibASCII->lpDibBits, data, pf.dibASCII->bmiHeader.biSizeImage);

		hDC.SelectObject(oldBitmap);
		DeleteBitmap(bitmap);

	}

	hDC.SelectObject(oldFont);
	gdiFont.DeleteObject();

	hDC.DeleteDC();
}


void PatternFont::DeleteFontData()
{
	delete[] customFontBitmap.lpDibBits;
	customFontBitmap.lpDibBits = nullptr;
	delete[] customFontBitmapASCII.lpDibBits;
	customFontBitmapASCII.lpDibBits = nullptr;
}

OPENMPT_NAMESPACE_END
