/*
 * draw_pat.cpp
 * ------------
 * Purpose: Code for drawing the pattern data.
 * Notes  : Also used for updating the status bar.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "dlg_misc.h"
#include "globals.h"
#include "view_pat.h"
#include "EffectVis.h"		//rewbs.fxvis
#include "ChannelManagerDlg.h"
#include "../soundlib/tuningbase.h"
#include "../common/StringFixer.h"
#include "EffectInfo.h"
#include <string>

// Headers
#define ROWHDR_WIDTH		32	// Row header
#define COLHDR_HEIGHT		16	// Column header
#define COLUMN_HEIGHT		13
#define	VUMETERS_HEIGHT		13	// Height of vu-meters
#define	PLUGNAME_HEIGHT		16	// Height of plugin names
#define VUMETERS_BMPWIDTH		32
#define VUMETERS_BMPHEIGHT		10
#define VUMETERS_MEDWIDTH		24
#define VUMETERS_LOWIDTH		16


typedef struct PATTERNFONT
{
	int nWidth, nHeight;		// Column Width & Height, including 4-pixels border
	int nClrX, nClrY;			// Clear (empty note) location
	int nSpaceX, nSpaceY;		// White location (must be big enough)
	UINT nEltWidths[5];			// Elements Sizes
	int nNumX, nNumY;			// Vertically-oriented numbers 0x00-0x0F
	int nNum10X, nNum10Y;		// Numbers 10-19
	int nAlphaAM_X,nAlphaAM_Y;	// Letters A-M +#
	int nAlphaNZ_X,nAlphaNZ_Y;	// Letters N-Z +?
	int nNoteX, nNoteY;			// Notes ..., C-, C#, ...
	int nNoteWidth;				// NoteWidth
	int nOctaveWidth;			// Octave Width
	int nVolX, nVolY;			// Volume Column Effects
	int nVolCmdWidth;			// Width of volume effect
	int nVolHiWidth;			// Width of first character in volume parameter
	int nCmdOfs;				// XOffset (-xxx) around the command letter
	int nParamHiWidth;
	int nInstrOfs, nInstr10Ofs, nInstrHiWidth;
} PATTERNFONT;

typedef const PATTERNFONT * PCPATTERNFONT;

//////////////////////////////////////////////
// Font Definitions

// Medium Font (Default)
const PATTERNFONT gDefaultPatternFont = 
{
	92,13,	// Column Width & Height
	0,0,	// Clear location
	130,8,	// Space Location.
	{20, 20, 24, 9, 15},		// Element Widths
	20,13,	// Numbers 0-F (hex)
	30,13,	// Numbers 10-19 (dec)
	64,26,	// A-M#
	78,26,	// N-Z?										// MEDIUM FONT !!!
	0, 0,
	12,8,	// Note & Octave Width
	42,13,	// Volume Column Effects
	8,8,
	-1,
	8,		// 8+7 = 15
	-3, -1, 12,
};


//////////////////////////////////////////////////
// Small Font

const PATTERNFONT gSmallPatternFont = 
{
	70,11,	// Column Width & Height
	92,0,	// Clear location
	130,8,	// Space Location.
	{16, 14, 18, 7, 11},		// Element Widths
	108,13,	// Numbers 0-F (hex)
	120,13,	// Numbers 10-19 (dec)
	142,26,	// A-M#
	150,26,	// N-Z?										// SMALL FONT !!!
	92, 0,	// Notes
	10,6,	// Note & Octave Width
	132,13,	// Volume Column Effects
	6,5,
	-1,
	6,		// 8+7 = 15
	-3,	1, 9,	// InstrOfs + nInstrHiWidth
};

// NOTE: See also CViewPattern::DrawNote() when changing stuff here
// or adding new fonts - The custom tuning note names might require
// some additions there.


/////////////////////////////////////////////////////////////////////////////
// Effect colour codes

// Effect number => Effect colour assignment
const int effectColors[] =
{
	0,					0,					MODCOLOR_PITCH,		MODCOLOR_PITCH,
	MODCOLOR_PITCH,		MODCOLOR_PITCH,		MODCOLOR_VOLUME,	MODCOLOR_VOLUME,
	MODCOLOR_VOLUME,	MODCOLOR_PANNING,	0,					MODCOLOR_VOLUME,
	MODCOLOR_GLOBALS,	MODCOLOR_VOLUME,	MODCOLOR_GLOBALS,	0,
	MODCOLOR_GLOBALS,	MODCOLOR_GLOBALS,	0,					0,					
	0,					MODCOLOR_VOLUME,	MODCOLOR_VOLUME,	MODCOLOR_GLOBALS,	
	MODCOLOR_GLOBALS,	0,					MODCOLOR_PITCH,		MODCOLOR_PANNING,
	MODCOLOR_PITCH,		MODCOLOR_PANNING,	0,					0,
	0,					0,					0,					MODCOLOR_PITCH,
	MODCOLOR_PITCH,		MODCOLOR_PITCH,		MODCOLOR_PITCH,		0,
};

STATIC_ASSERT(CountOf(effectColors) == MAX_EFFECTS);

// Volume effect number => Effect colour assignment
const int volEffectColors[] =
{
	0,					MODCOLOR_VOLUME,	MODCOLOR_PANNING,	MODCOLOR_VOLUME,
	MODCOLOR_VOLUME,	MODCOLOR_VOLUME,	MODCOLOR_VOLUME,	MODCOLOR_PITCH,
	MODCOLOR_PITCH,		MODCOLOR_PANNING,	MODCOLOR_PANNING,	MODCOLOR_PITCH,
	MODCOLOR_PITCH,		MODCOLOR_PITCH,		0,					0,
};

STATIC_ASSERT(CountOf(volEffectColors) == MAX_VOLCMDS);


/////////////////////////////////////////////////////////////////////////////
// CViewPattern Drawing Implementation

inline PCPATTERNFONT GetCurrentPatternFont()
//------------------------------------------
{
	return (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SMALLFONT) ? &gSmallPatternFont : &gDefaultPatternFont;
}


static BYTE hilightcolor(int c0, int c1)
//--------------------------------------
{
	int cf0, cf1;

	cf0 = 0xC0 - (c1>>2) - (c0>>3);
	if (cf0 < 0x40) cf0 = 0x40;
	if (cf0 > 0xC0) cf0 = 0xC0;
	cf1 = 0x100 - cf0;
	return (BYTE)((c0*cf0+c1*cf1)>>8);
}


void CViewPattern::UpdateColors()
//-------------------------------
{
	BYTE r,g,b;

	m_Dib.SetAllColors(0, MAX_MODCOLORS, TrackerSettings::Instance().rgbCustomColors);

	r = hilightcolor(GetRValue(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BACKHILIGHT]),
		GetRValue(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BACKNORMAL]));
	g = hilightcolor(GetGValue(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BACKHILIGHT]),
		GetGValue(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BACKNORMAL]));
	b = hilightcolor(GetBValue(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BACKHILIGHT]),
		GetBValue(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BACKNORMAL]));
	m_Dib.SetColor(MODCOLOR_2NDHIGHLIGHT, RGB(r,g,b));

	r = hilightcolor(GetRValue(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_VOLUME]),
					GetRValue(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BACKNORMAL]));
	g = hilightcolor(GetGValue(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_VOLUME]),
					GetGValue(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BACKNORMAL]));
	b = hilightcolor(GetBValue(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_VOLUME]),
					GetBValue(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BACKNORMAL]));
	m_Dib.SetColor(MODCOLOR_DEFAULTVOLUME, RGB(r,g,b));

	m_Dib.SetBlendColor(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BLENDCOLOR]);
}


bool CViewPattern::UpdateSizes()
//------------------------------
{
	PCPATTERNFONT pfnt = GetCurrentPatternFont();
	int oldx = m_szCell.cx;
	m_szHeader.cx = ROWHDR_WIDTH;
	m_szHeader.cy = COLHDR_HEIGHT;
	if(m_Status[psShowVUMeters]) m_szHeader.cy += VUMETERS_HEIGHT;
	if(m_Status[psShowPluginNames]) m_szHeader.cy += PLUGNAME_HEIGHT;
	m_szCell.cx = 4 + pfnt->nEltWidths[0];
	if (m_nDetailLevel >= PatternCursor::instrColumn) m_szCell.cx += pfnt->nEltWidths[1];
	if (m_nDetailLevel >= PatternCursor::volumeColumn) m_szCell.cx += pfnt->nEltWidths[2];
	if (m_nDetailLevel >= PatternCursor::effectColumn) m_szCell.cx += pfnt->nEltWidths[3] + pfnt->nEltWidths[4];
	m_szCell.cy = pfnt->nHeight;
	return (oldx != m_szCell.cx);
}


UINT CViewPattern::GetColumnOffset(PatternCursor::Columns column) const
//---------------------------------------------------------------------
{
	PCPATTERNFONT pfnt = GetCurrentPatternFont();
	LimitMax(column, PatternCursor::lastColumn);
	UINT offset = 0;
	for(int i = PatternCursor::firstColumn; i < column; i++) offset += pfnt->nEltWidths[i];
	return offset;
}


void CViewPattern::UpdateView(DWORD dwHintMask, CObject *)
//--------------------------------------------------------
{
	if(dwHintMask & HINT_MPTOPTIONS)
	{
		UpdateColors();
		UpdateSizes();
		UpdateScrollSize();
		InvalidatePattern(true);
		return;
	}
	if(dwHintMask & HINT_MODCHANNELS)
	{
		InvalidateChannelsHeaders();
		UpdateScrollSize();
	}

	const PATTERNINDEX updatePat = (dwHintMask >> HINT_SHIFT_PAT);
	if(HintFlagPart(dwHintMask) == HINT_PATTERNDATA && m_nPattern != updatePat && updatePat != 0)
		return;

	if(dwHintMask & (HINT_MODTYPE|HINT_PATTERNDATA))
	{
		InvalidatePattern(false);
	} else
	if(dwHintMask & HINT_PATTERNROW)
	{
		InvalidateRow(dwHintMask >> HINT_SHIFT_ROW);
	}

}


POINT CViewPattern::GetPointFromPosition(PatternCursor cursor) 
//------------------------------------------------------------
{
	PCPATTERNFONT pfnt = GetCurrentPatternFont();
	POINT pt;
	int xofs = GetXScrollPos();
	int yofs = GetYScrollPos();

	PatternCursor::Columns imax = cursor.GetColumnType();
	LimitMax(imax, PatternCursor::lastColumn);
// 	if(imax > m_nDetailLevel)
// 	{
// 		// Extend to next channel
// 		imax = PatternCursor::firstColumn;
// 		cursor.Move(0, 1, 0);
// 	}

	pt.x = (cursor.GetChannel() - xofs) * GetColumnWidth();

	for(int i = 0; i < imax; i++)
	{
		pt.x += pfnt->nEltWidths[i];
	}

	if (pt.x < 0) pt.x = 0;
	pt.x += ROWHDR_WIDTH;
	pt.y = (cursor.GetRow() - yofs + m_nMidRow) * m_szCell.cy;

	if (pt.y < 0) pt.y = 0;
	pt.y += m_szHeader.cy;

	return pt;
}


PatternCursor CViewPattern::GetPositionFromPoint(POINT pt)
//--------------------------------------------------------
{
	PCPATTERNFONT pfnt = GetCurrentPatternFont();
	int xofs = GetXScrollPos();
	int yofs = GetYScrollPos();
	int x = xofs + (pt.x - m_szHeader.cx) / GetColumnWidth();
	if (pt.x < m_szHeader.cx) x = (xofs) ? xofs - 1 : 0;
	int y = yofs - m_nMidRow + (pt.y - m_szHeader.cy) / m_szCell.cy;
	if (y < 0) y = 0;
	int xx = (pt.x - m_szHeader.cx) % GetColumnWidth(), dx = 0;
	int imax = 4;
	if (imax > (int)m_nDetailLevel + 1) imax = m_nDetailLevel + 1;
	int i = 0;
	for (i=0; i<imax; i++)
	{
		dx += pfnt->nEltWidths[i];
		if (xx < dx) break;
	}
	return PatternCursor(static_cast<ROWINDEX>(y), static_cast<CHANNELINDEX>(x), static_cast<PatternCursor::Columns>(i));
}


void CViewPattern::DrawLetter(int x, int y, char letter, int sizex, int ofsx)
//---------------------------------------------------------------------------
{
	PCPATTERNFONT pfnt = GetCurrentPatternFont();
	int srcx = pfnt->nSpaceX, srcy = pfnt->nSpaceY;

	if ((letter >= '0') && (letter <= '9'))
	{
		srcx = pfnt->nNumX;
		srcy = pfnt->nNumY + (letter - '0') * COLUMN_HEIGHT;
	} else
	if ((letter >= 'A') && (letter < 'N'))
	{
		srcx = pfnt->nAlphaAM_X;
		srcy = pfnt->nAlphaAM_Y + (letter - 'A') * COLUMN_HEIGHT;
	} else
	if ((letter >= 'N') && (letter <= 'Z'))
	{
		srcx = pfnt->nAlphaNZ_X;
		srcy = pfnt->nAlphaNZ_Y + (letter - 'N') * COLUMN_HEIGHT;
	} else
	switch(letter)
	{
	case '?':
		srcx = pfnt->nAlphaNZ_X;
		srcy = pfnt->nAlphaNZ_Y + 13 * COLUMN_HEIGHT;
		break;
	case '#':
		srcx = pfnt->nAlphaAM_X;
		srcy = pfnt->nAlphaAM_Y + 13 * COLUMN_HEIGHT;
		break;
	//rewbs.smoothVST
	case '\\':
		srcx = pfnt->nAlphaNZ_X;
		srcy = pfnt->nAlphaNZ_Y + 14 * COLUMN_HEIGHT;
		break;
	//end rewbs.smoothVST
	case ':':
		srcx = pfnt->nAlphaNZ_X;
		srcy = pfnt->nAlphaNZ_Y + 15 * COLUMN_HEIGHT;
		break;
	case ' ':
		srcx = pfnt->nSpaceX;
		srcy = pfnt->nSpaceY;
		break;
	case '-':
		srcx = pfnt->nAlphaAM_X;
		srcy = pfnt->nAlphaAM_Y + 15 * COLUMN_HEIGHT;
		break;
	case 'b':
		srcx = pfnt->nAlphaAM_X;
		srcy = pfnt->nAlphaAM_Y + 14 * COLUMN_HEIGHT;
		break;

	}
	m_Dib.TextBlt(x, y, sizex, COLUMN_HEIGHT, srcx+ofsx, srcy);
}


void CViewPattern::DrawNote(int x, int y, UINT note, CTuning* pTuning)
//--------------------------------------------------------------------
{
	PCPATTERNFONT pfnt = GetCurrentPatternFont();

	UINT xsrc = pfnt->nNoteX, ysrc = pfnt->nNoteY, dx = pfnt->nEltWidths[0];
	if (!note)
	{
		m_Dib.TextBlt(x, y, dx, COLUMN_HEIGHT, xsrc, ysrc);
	} else
	if (note == NOTE_NOTECUT)
	{
		m_Dib.TextBlt(x, y, dx, COLUMN_HEIGHT, xsrc, ysrc + 13*COLUMN_HEIGHT);
	} else
	if (note == NOTE_KEYOFF)
	{
		m_Dib.TextBlt(x, y, dx, COLUMN_HEIGHT, xsrc, ysrc + 14*COLUMN_HEIGHT);
	} else
	if(note == NOTE_FADE)
	{
		m_Dib.TextBlt(x, y, dx, COLUMN_HEIGHT, xsrc, ysrc + 17*COLUMN_HEIGHT);
	} else
	if(note == NOTE_PC)
	{
		m_Dib.TextBlt(x, y, dx, COLUMN_HEIGHT, xsrc, ysrc + 15*COLUMN_HEIGHT);
	} else
	if(note == NOTE_PCS)
	{
		m_Dib.TextBlt(x, y, dx, COLUMN_HEIGHT, xsrc, ysrc + 16*COLUMN_HEIGHT);
	} else
	{
		if(pTuning)
		{   // Drawing custom note names
			std::string noteStr = pTuning->GetNoteName(static_cast<CTuning::NOTEINDEXTYPE>(note-NOTE_MIDDLEC));
			if(noteStr.size() < 3)
				noteStr.resize(3, ' ');
			
			// Hack: Manual tweaking for default/small font displays.
			if(pfnt == &gDefaultPatternFont)
			{
				DrawLetter(x, y, noteStr[0], 7, 0);
				DrawLetter(x + 7, y, noteStr[1], 6, 0);
			}
			else
			{
				DrawLetter(x, y, noteStr[0], 5, 0);
				DrawLetter(x + 5, y, noteStr[1], 5, 0);
			}
			DrawLetter(x + pfnt->nNoteWidth, y, noteStr[2], pfnt->nOctaveWidth, 0);
		}
		else //Original
		{
			UINT o = (note-1) / 12; //Octave
			UINT n = (note-1) % 12; //Note
			m_Dib.TextBlt(x, y, pfnt->nNoteWidth, COLUMN_HEIGHT, xsrc, ysrc+(n+1)*COLUMN_HEIGHT);
			if(o <= 9)
				m_Dib.TextBlt(x+pfnt->nNoteWidth, y, pfnt->nOctaveWidth, COLUMN_HEIGHT,
								pfnt->nNumX, pfnt->nNumY+o*COLUMN_HEIGHT);
			else
				DrawLetter(x+pfnt->nNoteWidth, y, '?', pfnt->nOctaveWidth);
		}
	}
}


void CViewPattern::DrawInstrument(int x, int y, UINT instr)
//---------------------------------------------------------
{
	PCPATTERNFONT pfnt = GetCurrentPatternFont();
	if (instr)
	{
		UINT dx = pfnt->nInstrHiWidth;
		if (instr < 100)
		{
			m_Dib.TextBlt(x, y, dx, COLUMN_HEIGHT, pfnt->nNumX+pfnt->nInstrOfs, pfnt->nNumY+(instr / 10)*COLUMN_HEIGHT);
		} else
		{
			m_Dib.TextBlt(x, y, dx, COLUMN_HEIGHT, pfnt->nNum10X+pfnt->nInstr10Ofs, pfnt->nNum10Y+((instr-100) / 10)*COLUMN_HEIGHT);
		}
		m_Dib.TextBlt(x+dx, y, pfnt->nEltWidths[1]-dx, COLUMN_HEIGHT, pfnt->nNumX+1, pfnt->nNumY+(instr % 10)*COLUMN_HEIGHT);
	} else
	{
		m_Dib.TextBlt(x, y, pfnt->nEltWidths[1], COLUMN_HEIGHT, pfnt->nClrX+pfnt->nEltWidths[0], pfnt->nClrY);
	}
}


void CViewPattern::DrawVolumeCommand(int x, int y, const ModCommand &mc, bool drawDefaultVolume)
//----------------------------------------------------------------------------------------------
{
	PCPATTERNFONT pfnt = GetCurrentPatternFont();

	if(mc.IsPcNote())
	{	//If note is parameter control note, drawing volume command differently.
		const int val = MIN(ModCommand::maxColumnValue, mc.GetValueVolCol());

		m_Dib.TextBlt(x, y, 1, COLUMN_HEIGHT, pfnt->nClrX, pfnt->nClrY);
		m_Dib.TextBlt(x + 1, y, pfnt->nVolCmdWidth, COLUMN_HEIGHT,
							pfnt->nNumX, pfnt->nNumY+(val / 100)*COLUMN_HEIGHT);
		m_Dib.TextBlt(x+pfnt->nVolCmdWidth, y, pfnt->nVolHiWidth, COLUMN_HEIGHT,
							pfnt->nNumX, pfnt->nNumY+((val / 10)%10)*COLUMN_HEIGHT);
		m_Dib.TextBlt(x+pfnt->nVolCmdWidth+pfnt->nVolHiWidth, y, pfnt->nEltWidths[2]-(pfnt->nVolCmdWidth+pfnt->nVolHiWidth), COLUMN_HEIGHT,
							pfnt->nNumX, pfnt->nNumY+(val % 10)*COLUMN_HEIGHT);
	}
	else
	{
		static_assert(MAX_VOLCMDS <= 16, "Pattern draw code assumes <= 16 volume commands");
		ModCommand::VOLCMD volcmd = (mc.volcmd & 0x0F);
		int vol  = (mc.vol & 0x7F);

		if(drawDefaultVolume)
		{
			// Displaying sample default volume if there is no volume command.
			const CSoundFile *pSndFile = GetSoundFile();
			SAMPLEINDEX sample = mc.instr;
			if(pSndFile->GetNumInstruments())
			{
				if(mc.instr <= pSndFile->GetNumInstruments())
				{
					sample = pSndFile->Instruments[mc.instr]->Keyboard[mc.note - NOTE_MIN];
				} else
				{
					sample = 0;
				}
			}
			volcmd = VOLCMD_VOLUME;
			if(sample && sample <= pSndFile->GetNumSamples())
			{
				vol = pSndFile->GetSample(sample).nVolume / 4;
			} else
			{
				vol = 64;
			}
		}

		if(volcmd != VOLCMD_NONE)
		{
			m_Dib.TextBlt(x, y, pfnt->nVolCmdWidth, COLUMN_HEIGHT,
							pfnt->nVolX, pfnt->nVolY + volcmd * COLUMN_HEIGHT);
			m_Dib.TextBlt(x+pfnt->nVolCmdWidth, y, pfnt->nVolHiWidth, COLUMN_HEIGHT,
							pfnt->nNumX, pfnt->nNumY + (vol / 10) * COLUMN_HEIGHT);
			m_Dib.TextBlt(x+pfnt->nVolCmdWidth + pfnt->nVolHiWidth, y, pfnt->nEltWidths[2] - (pfnt->nVolCmdWidth + pfnt->nVolHiWidth), COLUMN_HEIGHT,
							pfnt->nNumX, pfnt->nNumY + (vol % 10) * COLUMN_HEIGHT);
		} else
		{
			int srcx = pfnt->nEltWidths[0] + pfnt->nEltWidths[1];
			m_Dib.TextBlt(x, y, pfnt->nEltWidths[2], COLUMN_HEIGHT, pfnt->nClrX+srcx, pfnt->nClrY);
		}
	}
}


void CViewPattern::OnDraw(CDC *pDC)
//---------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CHAR s[256];
	HGDIOBJ oldpen;
	CRect rcClient, rect, rc;
	const CModDoc *pModDoc;
	const CSoundFile *pSndFile;
	HDC hdc;
	CHANNELINDEX xofs;
	ROWINDEX yofs;
	UINT nColumnWidth, ncols, nrows, ncolhdr;
	int xpaint, ypaint, mixPlug;

	ASSERT(pDC);
	UpdateSizes();
	if ((pModDoc = GetDocument()) == NULL) return;
	GetClientRect(&rcClient);
	hdc = pDC->m_hDC;
	oldpen = ::SelectObject(hdc, CMainFrame::penDarkGray);
	xofs = static_cast<CHANNELINDEX>(GetXScrollPos());
	yofs = static_cast<ROWINDEX>(GetYScrollPos());
	pSndFile = pModDoc->GetSoundFile();
	nColumnWidth = m_szCell.cx;
	nrows = (pSndFile->Patterns[m_nPattern]) ? pSndFile->Patterns[m_nPattern].GetNumRows() : 0;
	ncols = pSndFile->GetNumChannels();
	xpaint = m_szHeader.cx;
	ypaint = rcClient.top;
	ncolhdr = xofs;
	rect.SetRect(0, rcClient.top, rcClient.right, rcClient.top + m_szHeader.cy);
	if (::RectVisible(hdc, &rect))
	{
		wsprintf(s, "#%d", m_nPattern);
		rect.right = m_szHeader.cx;
		DrawButtonRect(hdc, &rect, s, FALSE,
			((m_bInItemRect) && ((m_nDragItem & DRAGITEM_MASK) == DRAGITEM_PATTERNHEADER)) ? TRUE : FALSE);

		// Drawing Channel Headers
		while (xpaint < rcClient.right)
		{
			rect.SetRect(xpaint, ypaint, xpaint+nColumnWidth, ypaint + m_szHeader.cy);
			if (ncolhdr < ncols)
			{
// -> CODE#0012
// -> DESC="midi keyboard split"
				const char *pszfmt = pSndFile->m_bChannelMuteTogglePending[ncolhdr]? "[Channel %d]" : "Channel %d";
//				const char *pszfmt = pModDoc->IsChannelRecord(ncolhdr) ? "Channel %d " : "Channel %d";
// -! NEW_FEATURE#0012
				if ((pSndFile->GetType() & (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT)) && ((BYTE)pSndFile->ChnSettings[ncolhdr].szName[0] >= ' '))
					pszfmt = pSndFile->m_bChannelMuteTogglePending[ncolhdr] ? "%d: [%s]" : "%d: %s";
				else if (m_nDetailLevel < PatternCursor::volumeColumn) pszfmt = pSndFile->m_bChannelMuteTogglePending[ncolhdr] ? "[Ch%d]" : "Ch%d";
				else if (m_nDetailLevel < PatternCursor::effectColumn) pszfmt = pSndFile->m_bChannelMuteTogglePending[ncolhdr] ? "[Chn %d]" : "Chn %d";
				wsprintf(s, pszfmt, ncolhdr + 1, pSndFile->ChnSettings[ncolhdr].szName);
// -> CODE#0012
// -> DESC="midi keyboard split"
//				DrawButtonRect(hdc, &rect, s,
//					(pSndFile->ChnSettings[ncolhdr].dwFlags & CHN_MUTE) ? TRUE : FALSE,
//					((m_bInItemRect) && ((m_nDragItem & DRAGITEM_MASK) == DRAGITEM_CHNHEADER) && ((m_nDragItem & DRAGITEM_VALUEMASK) == ncolhdr)) ? TRUE : FALSE, DT_CENTER);
//				rect.bottom = rect.top + COLHDR_HEIGHT;
				DrawButtonRect(hdc, &rect, s,
					pSndFile->ChnSettings[ncolhdr].dwFlags[CHN_MUTE] ? TRUE : FALSE,
					((m_bInItemRect) && ((m_nDragItem & DRAGITEM_MASK) == DRAGITEM_CHNHEADER) && ((m_nDragItem & DRAGITEM_VALUEMASK) == ncolhdr)) ? TRUE : FALSE,
					pModDoc->IsChannelRecord(static_cast<CHANNELINDEX>(ncolhdr)) ? DT_RIGHT : DT_CENTER);

				// When dragging around channel headers, mark insertion position
				if(m_Status[psDragging] && !m_bInItemRect
					&& (m_nDragItem & DRAGITEM_MASK) == DRAGITEM_CHNHEADER
					&& (m_nDropItem & DRAGITEM_MASK) == DRAGITEM_CHNHEADER
					&& (m_nDropItem & DRAGITEM_VALUEMASK) == ncolhdr)
				{
					RECT r;
					r.top = rect.top;
					r.bottom = rect.bottom;
					// Drop position depends on whether hovered channel is left or right of dragged item.
					r.left = ((m_nDropItem & DRAGITEM_VALUEMASK) < (m_nDragItem & DRAGITEM_VALUEMASK) || m_Status[psShiftDragging]) ? rect.left : rect.right - 2;
					r.right = r.left + 2;
					::FillRect(hdc, &r, CMainFrame::brushText);
				}

				rect.bottom = rect.top + COLHDR_HEIGHT;

				CRect insRect;
				insRect.SetRect(xpaint, ypaint, xpaint+nColumnWidth / 8 + 3, ypaint + 16);
//				if (MultiRecordMask[ncolhdr>>3] & (1 << (ncolhdr&7)))
				if (pModDoc->IsChannelRecord1(static_cast<CHANNELINDEX>(ncolhdr)))
				{
//					rect.DeflateRect(1, 1);
//					InvertRect(hdc, &rect);
//					rect.InflateRect(1, 1);
					FrameRect(hdc,&rect,CMainFrame::brushGray);
					InvertRect(hdc, &rect);
					s[0] = '1';
					s[1] = '\0';
					DrawButtonRect(hdc, &insRect, s, FALSE, FALSE, DT_CENTER);
					FrameRect(hdc,&insRect,CMainFrame::brushBlack);
				}
				else if (pModDoc->IsChannelRecord2(static_cast<CHANNELINDEX>(ncolhdr)))
				{
					FrameRect(hdc,&rect,CMainFrame::brushGray);
					InvertRect(hdc, &rect);
					s[0] = '2';
					s[1] = '\0';
					DrawButtonRect(hdc, &insRect, s, FALSE, FALSE, DT_CENTER);
					FrameRect(hdc,&insRect,CMainFrame::brushBlack);
				}
// -! NEW_FEATURE#0012

				if(m_Status[psShowVUMeters])
				{
					OldVUMeters[ncolhdr] = 0;
					DrawChannelVUMeter(hdc, rect.left + 1, rect.bottom, ncolhdr);
					rect.top+=VUMETERS_HEIGHT;
					rect.bottom+=VUMETERS_HEIGHT;
				}
				if(m_Status[psShowPluginNames])
				{
					rect.top+=PLUGNAME_HEIGHT;
					rect.bottom+=PLUGNAME_HEIGHT;
					mixPlug=pSndFile->ChnSettings[ncolhdr].nMixPlugin;
					if (mixPlug) {
						wsprintf(s, "%d: %s", mixPlug, (pSndFile->m_MixPlugins[mixPlug - 1]).pMixPlugin ? (pSndFile->m_MixPlugins[mixPlug - 1]).GetName() : "[empty]");
					} else {
						wsprintf(s, "---");
					}
					DrawButtonRect(hdc, &rect, s, FALSE, 
						((m_bInItemRect) && ((m_nDragItem & DRAGITEM_MASK) == DRAGITEM_PLUGNAME) && ((m_nDragItem & DRAGITEM_VALUEMASK) == ncolhdr)) ? TRUE : FALSE, DT_CENTER);
				}

			} else break;
			ncolhdr++;
			xpaint += nColumnWidth;
		}
	}
	ypaint += m_szHeader.cy;
	if (m_nMidRow)
	{
		if (yofs >= m_nMidRow)
		{
			yofs -= m_nMidRow;
		} else
		{
			UINT nSkip = m_nMidRow - yofs;
			PATTERNINDEX nPrevPat = PATTERNINDEX_INVALID;

			// Display previous pattern
			if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SHOWPREVIOUS)
			{
				const ORDERINDEX startOrder = static_cast<ORDERINDEX>(SendCtrlMessage(CTRLMSG_GETCURRENTORDER));
				if(startOrder > 0)
				{
					ORDERINDEX prevOrder = pSndFile->Order.GetPreviousOrderIgnoringSkips(startOrder);
					//Skip +++ items

					if(startOrder < pSndFile->Order.size() && pSndFile->Order[startOrder] == m_nPattern)
					{
						nPrevPat = pSndFile->Order[prevOrder];
					}
				}
			}
			if(pSndFile->Patterns.IsValidPat(nPrevPat))
			{
				ROWINDEX nPrevRows = pSndFile->Patterns[nPrevPat].GetNumRows();
				ROWINDEX n = MIN(static_cast<ROWINDEX>(nSkip), nPrevRows);

				ypaint += (nSkip - n) * m_szCell.cy;
				rect.SetRect(0, m_szHeader.cy, nColumnWidth * ncols + m_szHeader.cx, ypaint - 1);
				m_Dib.SetBlendMode(0x80);
				DrawPatternData(hdc, pSndFile, nPrevPat, false, false,
						nPrevRows - n, nPrevRows, xofs, rcClient, &ypaint);
				m_Dib.SetBlendMode(0);
			} else
			{
				ypaint += nSkip * m_szCell.cy;
				rect.SetRect(0, m_szHeader.cy, nColumnWidth * ncols + m_szHeader.cx, ypaint - 1);
			}
			if ((rect.bottom > rect.top) && (rect.right > rect.left))
			{
				::FillRect(hdc, &rect, CMainFrame::brushGray);
				::MoveToEx(hdc, 0, rect.bottom, NULL);
				::LineTo(hdc, rect.right, rect.bottom);
			}
			yofs = 0;
		}
	}
	int ypatternend = ypaint + (nrows-yofs)*m_szCell.cy;
	DrawPatternData(hdc, pSndFile, m_nPattern, TRUE, (pMainFrm->GetModPlaying() == pModDoc),
					yofs, nrows, xofs, rcClient, &ypaint);
	// Display next pattern
	if ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SHOWPREVIOUS) && (ypaint < rcClient.bottom) && (ypaint == ypatternend))
	{
		int nVisRows = (rcClient.bottom - ypaint + m_szCell.cy - 1) / m_szCell.cy;
		if ((nVisRows > 0) && (m_nMidRow))
		{
			PATTERNINDEX nNextPat = PATTERNINDEX_INVALID;
			const ORDERINDEX startOrder= static_cast<ORDERINDEX>(SendCtrlMessage(CTRLMSG_GETCURRENTORDER));

			ORDERINDEX nNextOrder = pSndFile->Order.GetNextOrderIgnoringSkips(startOrder);
			if(nNextOrder == startOrder) nNextOrder = ORDERINDEX_INVALID;
			//Ignore skip items(+++) from sequence.
			const ORDERINDEX ordCount = pSndFile->Order.GetLength();

			if(nNextOrder < ordCount && pSndFile->Order[startOrder] == m_nPattern)
			{
				nNextPat = pSndFile->Order[nNextOrder];
			}
			if(pSndFile->Patterns.IsValidPat(nNextPat))
			{
				ROWINDEX nNextRows = pSndFile->Patterns[nNextPat].GetNumRows();
				ROWINDEX n = MIN(static_cast<ROWINDEX>(nVisRows), nNextRows);

				m_Dib.SetBlendMode(0x80);
				DrawPatternData(hdc, pSndFile, nNextPat, false, false,
						0, n, xofs, rcClient, &ypaint);
				m_Dib.SetBlendMode(0);
			}
		}
	}
	// Drawing outside pattern area
	xpaint = m_szHeader.cx + (ncols - xofs) * nColumnWidth;
	if ((xpaint < rcClient.right) && (ypaint > rcClient.top))
	{
		rc.SetRect(xpaint, rcClient.top, rcClient.right, ypaint);
		::FillRect(hdc, &rc, CMainFrame::brushGray);
	}
	if (ypaint < rcClient.bottom)
	{
		rc.SetRect(0, ypaint, rcClient.right+1, rcClient.bottom+1);
		DrawButtonRect(hdc, &rc, "");
	}
	// Drawing pattern selection
	if(m_Status[psDragnDropping])
	{
		DrawDragSel(hdc);
	}
	if (oldpen) ::SelectObject(hdc, oldpen);

	//rewbs.fxVis
	if (m_pEffectVis)
	{
		//HACK: Update visualizer on every pattern redraw. Cleary there's space for opt here.
		if (m_pEffectVis->m_hWnd) m_pEffectVis->Update();
	}


// -> CODE#0015
// -> DESC="channels management dlg"
	bool activeDoc = pMainFrm ? (pMainFrm->GetActiveDoc() == GetDocument()) : false;

	if(activeDoc && CChannelManagerDlg::sharedInstance(FALSE) && CChannelManagerDlg::sharedInstance()->IsDisplayed())
		CChannelManagerDlg::sharedInstance()->SetDocument((void*)this);
// -! NEW_FEATURE#0015
}


void CViewPattern::DrawPatternData(HDC hdc, const CSoundFile *pSndFile, PATTERNINDEX nPattern, bool selEnable,
	bool isPlaying, ROWINDEX startRow, ROWINDEX numRows, CHANNELINDEX startChan, CRect &rcClient, int *pypaint)
//-------------------------------------------------------------------------------------------------------------
{
	uint8 selectedCols[MAX_BASECHANNELS];	// Bit mask of selected channel components
	static_assert(PatternCursor::lastColumn <= 7, "Columns are used as bitmasks here.");

	PCPATTERNFONT pfnt = GetCurrentPatternFont();
	const ModCommand m0 = ModCommand::Empty();
	const ModCommand *pPattern = pSndFile->Patterns[nPattern];
	CHAR s[256];
	CRect rect;
	int xpaint, ypaint = *pypaint;
	int row_col, row_bkcol;
	UINT bSpeedUp, nColumnWidth;
	
	CHANNELINDEX ncols = pSndFile->GetNumChannels();
	nColumnWidth = m_szCell.cx;
	rect.SetRect(m_szHeader.cx, rcClient.top, m_szHeader.cx+nColumnWidth, rcClient.bottom);
	for(CHANNELINDEX cmk = startChan; cmk < ncols; cmk++)
	{
		selectedCols[cmk] = selEnable ? m_Selection.GetSelectionBits(cmk) : 0;
		if (!::RectVisible(hdc, &rect)) selectedCols[cmk] |= 0x80;
		rect.left += nColumnWidth;
		rect.right += nColumnWidth;
	}
	// Max Visible Column
	CHANNELINDEX maxcol = ncols;
	while ((maxcol > startChan) && (selectedCols[maxcol-1] & 0x80)) maxcol--;
	// Init bitmap border
	{
		UINT maxndx = pSndFile->GetNumChannels() * m_szCell.cx;
		UINT ibmp = 0;
		if (maxndx > FASTBMP_MAXWIDTH) maxndx = FASTBMP_MAXWIDTH;
		do
		{
			ibmp += nColumnWidth;
			m_Dib.TextBlt(ibmp-4, 0, 4, m_szCell.cy, pfnt->nClrX+pfnt->nWidth-4, pfnt->nClrY);
		} while (ibmp + nColumnWidth <= maxndx);
	}
	
	bool bRowSel = false;
	row_col = row_bkcol = -1;
	for (UINT row=startRow; row<numRows; row++)
	{
		UINT col, xbmp, nbmp, oldrowcolor;
		const int compRow = row + TrackerSettings::Instance().rowDisplayOffset;

		if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_HEXDISPLAY))
			wsprintf(s, "%s%02X", compRow < 0 ? "-" : "", abs(compRow));
		else
			wsprintf(s, "%d", compRow);

		rect.left = 0;
		rect.top = ypaint;
		rect.right = rcClient.right;
		rect.bottom = rect.top + m_szCell.cy;
		if (!::RectVisible(hdc, &rect)) 
		{
			// No speedup for these columns next time
			for (CHANNELINDEX iup=startChan; iup<maxcol; iup++) selectedCols[iup] &= ~0x40;
			goto SkipRow;
		}
		rect.right = rect.left + m_szHeader.cx;
		DrawButtonRect(hdc, &rect, s, !selEnable);
		oldrowcolor = (row_bkcol << 16) | (row_col << 8) | (bRowSel ? 1 : 0);
		bRowSel = (m_Selection.ContainsVertical(PatternCursor(row)));
		row_col = MODCOLOR_TEXTNORMAL;
		row_bkcol = MODCOLOR_BACKNORMAL;

		// time signature highlighting
		ROWINDEX nBeat = pSndFile->m_nDefaultRowsPerBeat, nMeasure = pSndFile->m_nDefaultRowsPerMeasure;
		if(pSndFile->Patterns[nPattern].GetOverrideSignature())
		{
			nBeat = pSndFile->Patterns[nPattern].GetRowsPerBeat();
			nMeasure = pSndFile->Patterns[nPattern].GetRowsPerMeasure();
		}
		// secondary highlight (beats)
		if ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_2NDHIGHLIGHT)
		 && (nBeat) && (nBeat < numRows))
		{
			if(!(compRow % nBeat))
			{
				row_bkcol = MODCOLOR_2NDHIGHLIGHT;
			}
		}
		// primary highlight (measures)
		if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_STDHIGHLIGHT)
			&& (nMeasure) && (nMeasure < numRows))
		{
			if(!(compRow % nMeasure))
			{
				row_bkcol = MODCOLOR_BACKHILIGHT;
			}
		}
		if (selEnable)
		{
			if ((row == m_nPlayRow) && (nPattern == m_nPlayPat))
			{
				row_col = MODCOLOR_TEXTPLAYCURSOR;
				row_bkcol = MODCOLOR_BACKPLAYCURSOR;
			}
			if (row == GetCurrentRow())
			{
				if(m_Status[psFocussed])
				{
					row_col = MODCOLOR_TEXTCURROW;
					row_bkcol = MODCOLOR_BACKCURROW;
				} else
				if(m_Status[psFollowSong] && isPlaying)
				{
					row_col = MODCOLOR_TEXTPLAYCURSOR;
					row_bkcol = MODCOLOR_BACKPLAYCURSOR;
				}
			}
		}
		// Eliminate non-visible column
		xpaint = m_szHeader.cx;
		col = startChan;
		while ((selectedCols[col] & 0x80) && (col < maxcol))
		{
			selectedCols[col] &= ~0x40;
			col++;
			xpaint += nColumnWidth;
		}
		// Optimization: same row color ?
		bSpeedUp = (oldrowcolor == (UINT)((row_bkcol << 16) | (row_col << 8) | (bRowSel ? 1 : 0))) ? TRUE : FALSE;
		xbmp = nbmp = 0;
		do
		{
			int x, bk_col, tx_col, col_sel, fx_col;

			const ModCommand *m = (pPattern) ? &pPattern[row*ncols+col] : &m0;

			// Should empty volume commands be replaced with a volume command showing the default volume?
			const bool drawDefaultVolume = DrawDefaultVolume(m);

			DWORD dwSpeedUpMask = 0;
			if ((bSpeedUp) && (selectedCols[col] & 0x40) && (pPattern) && (row))
			{
				const ModCommand *mold = m - ncols;
				const bool drawOldDefaultVolume = DrawDefaultVolume(mold);

				if (m->note == mold->note) dwSpeedUpMask |= 0x01;
				if ((m->instr == mold->instr) || (m_nDetailLevel < PatternCursor::instrColumn)) dwSpeedUpMask |= 0x02;
				if ( m->IsPcNote() || mold->IsPcNote() )
				{   // Handle speedup mask for PC notes.
					if(m->note == mold->note)
					{
						if(m->GetValueVolCol() == mold->GetValueVolCol() || (m_nDetailLevel < PatternCursor::volumeColumn)) dwSpeedUpMask |= 0x04;
						if(m->GetValueEffectCol() == mold->GetValueEffectCol() || (m_nDetailLevel < PatternCursor::effectColumn)) dwSpeedUpMask |= 0x18;
					}		
				}
				else
				{
					if ((m->volcmd == mold->volcmd && (m->volcmd == VOLCMD_NONE || m->vol == mold->vol) && !drawDefaultVolume && !drawOldDefaultVolume) || (m_nDetailLevel < PatternCursor::volumeColumn)) dwSpeedUpMask |= 0x04;
					if ((m->command == mold->command) || (m_nDetailLevel < PatternCursor::effectColumn)) dwSpeedUpMask |= (m->command != CMD_NONE) ? 0x08 : 0x18;
				}
				if (dwSpeedUpMask == 0x1F) goto DoBlit;
			}
			selectedCols[col] |= 0x40;
			col_sel = 0;
			if (bRowSel) col_sel = selectedCols[col] & 0x3F;
			tx_col = row_col;
			bk_col = row_bkcol;
			if (col_sel)
			{
				tx_col = MODCOLOR_TEXTSELECTED;
				bk_col = MODCOLOR_BACKSELECTED;
			}
			// Speedup: Empty command which is either not or fully selected
			if (m->IsEmpty() && ((!col_sel) || (col_sel == 0x1F)))
			{
				m_Dib.SetTextColor(tx_col, bk_col);
				m_Dib.TextBlt(xbmp, 0, nColumnWidth-4, m_szCell.cy, pfnt->nClrX, pfnt->nClrY);
				goto DoBlit;
			}
			x = 0;
			// Note
			if (!(dwSpeedUpMask & 0x01))
			{
				tx_col = row_col;
				bk_col = row_bkcol;
				if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_EFFECTHILIGHT) && m->IsNote())
				{
					tx_col = MODCOLOR_NOTE;

					if(pSndFile->m_SongFlags[SONG_PT1XMODE] && (m->note < NOTE_MIDDLEC - 12 || m->note >= NOTE_MIDDLEC + 2 * 12))
					{
						// MOD "ProTracker 1.x" flag: Highlight notes that are not supported by Amiga trackers.
						tx_col = MODCOLOR_DODGY_COMMANDS;
					} else if(pSndFile->m_SongFlags[SONG_AMIGALIMITS] && m->instr != 0 && m->instr <= pSndFile->GetNumSamples())
					{
						// S3M "Force Amiga Limits": Highlight notes that exceed the Amiga's frequency range.
						UINT period = pSndFile->GetPeriodFromNote(m->note, 0, pSndFile->GetSample(m->instr).nC5Speed);
						if(period < 113 * 4 || period > 856 * 4)
						{
							tx_col = MODCOLOR_DODGY_COMMANDS;
						}
					}
				}
				if (col_sel & 0x01)
				{
					tx_col = MODCOLOR_TEXTSELECTED;
					bk_col = MODCOLOR_BACKSELECTED;
				}
				// Drawing note
				m_Dib.SetTextColor(tx_col, bk_col);
				if(pSndFile->m_nType == MOD_TYPE_MPT && m->instr < MAX_INSTRUMENTS && pSndFile->Instruments[m->instr])
					DrawNote(xbmp+x, 0, m->note, pSndFile->Instruments[m->instr]->pTuning);
				else //Original
					DrawNote(xbmp+x, 0, m->note);
			}
			x += pfnt->nEltWidths[0];
			// Instrument
			if (m_nDetailLevel >= PatternCursor::instrColumn)
			{
				if (!(dwSpeedUpMask & 0x02))
				{
					tx_col = row_col;
					bk_col = row_bkcol;
					if ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_EFFECTHILIGHT) && (m->instr))
					{
						tx_col = MODCOLOR_INSTRUMENT;
					}
					if (col_sel & 0x02)
					{
						tx_col = MODCOLOR_TEXTSELECTED;
						bk_col = MODCOLOR_BACKSELECTED;
					}
					// Drawing instrument
					m_Dib.SetTextColor(tx_col, bk_col);
					DrawInstrument(xbmp+x, 0, m->instr);
				}
				x += pfnt->nEltWidths[1];
			}
			// Volume
			if (m_nDetailLevel >= PatternCursor::volumeColumn)
			{
				if (!(dwSpeedUpMask & 0x04))
				{
					tx_col = row_col;
					bk_col = row_bkcol;
					if (col_sel & 0x04)
					{
						tx_col = MODCOLOR_TEXTSELECTED;
						bk_col = MODCOLOR_BACKSELECTED;
					} else if (!m->IsPcNote() && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_EFFECTHILIGHT))
					{
						if(m->volcmd != VOLCMD_NONE && m->volcmd < MAX_VOLCMDS && volEffectColors[m->volcmd] != 0)
						{
							tx_col = volEffectColors[m->volcmd];
						} else if(drawDefaultVolume)
						{
							tx_col = MODCOLOR_DEFAULTVOLUME;
						}
					}
					// Drawing Volume
					m_Dib.SetTextColor(tx_col, bk_col);
					DrawVolumeCommand(xbmp + x, 0, *m, drawDefaultVolume);
				}
				x += pfnt->nEltWidths[2];
			}
			// Command & param
			if (m_nDetailLevel >= PatternCursor::effectColumn)
			{
				const bool isPCnote = m->IsPcNote();
				uint16 val = m->GetValueEffectCol();
				if(val > ModCommand::maxColumnValue) val = ModCommand::maxColumnValue;
				fx_col = row_col;
				if (!isPCnote && m->command != CMD_NONE && m->command < MAX_EFFECTS && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_EFFECTHILIGHT))
				{
					if(effectColors[m->command] != 0)
						fx_col = effectColors[m->command];
				}
				if (!(dwSpeedUpMask & 0x08))
				{
					tx_col = fx_col;
					bk_col = row_bkcol;
					if (col_sel & 0x08)
					{
						tx_col = MODCOLOR_TEXTSELECTED;
						bk_col = MODCOLOR_BACKSELECTED;
					}

					// Drawing Command
					m_Dib.SetTextColor(tx_col, bk_col);
					if(isPCnote)
					{
						m_Dib.TextBlt(xbmp + x, 0, 2, COLUMN_HEIGHT, pfnt->nClrX+x, pfnt->nClrY);
						m_Dib.TextBlt(xbmp + x + 2, 0, pfnt->nEltWidths[3], m_szCell.cy, pfnt->nNumX, pfnt->nNumY+(val / 100)*COLUMN_HEIGHT);
					} else
					{
						if(m->command != CMD_NONE)
						{
							char n = pSndFile->GetModSpecifications().GetEffectLetter(m->command);
							ASSERT(n > ' ');
							DrawLetter(xbmp+x, 0, n, pfnt->nEltWidths[3], pfnt->nCmdOfs);
						} else
						{
							m_Dib.TextBlt(xbmp+x, 0, pfnt->nEltWidths[3], COLUMN_HEIGHT, pfnt->nClrX+x, pfnt->nClrY);
						}
					}
				}
				x += pfnt->nEltWidths[3];
				// Param
				if (!(dwSpeedUpMask & 0x10))
				{
					tx_col = fx_col;
					bk_col = row_bkcol;
					if (col_sel & 0x10)
					{
						tx_col = MODCOLOR_TEXTSELECTED;
						bk_col = MODCOLOR_BACKSELECTED;
					}

					// Drawing param
					m_Dib.SetTextColor(tx_col, bk_col);
					if(isPCnote)
					{
						m_Dib.TextBlt(xbmp + x, 0, pfnt->nParamHiWidth, m_szCell.cy, pfnt->nNumX, pfnt->nNumY+((val / 10) % 10)*COLUMN_HEIGHT);
						m_Dib.TextBlt(xbmp + x + pfnt->nParamHiWidth, 0, pfnt->nEltWidths[4]-pfnt->nParamHiWidth, m_szCell.cy, pfnt->nNumX+1, pfnt->nNumY+(val % 10)*COLUMN_HEIGHT);
					}
					else
					{
						if (m->command)
						{
							m_Dib.TextBlt(xbmp+x, 0, pfnt->nParamHiWidth, m_szCell.cy, pfnt->nNumX, pfnt->nNumY+(m->param >> 4)*COLUMN_HEIGHT);
							m_Dib.TextBlt(xbmp+x+pfnt->nParamHiWidth, 0, pfnt->nEltWidths[4]-pfnt->nParamHiWidth, m_szCell.cy, pfnt->nNumX+1, pfnt->nNumY+(m->param & 0x0F)*COLUMN_HEIGHT);
						} else
						{
							m_Dib.TextBlt(xbmp+x, 0, pfnt->nEltWidths[4], m_szCell.cy, pfnt->nClrX+x, pfnt->nClrY);
						}
					}
				}
			}
		DoBlit:
			nbmp++;
			xbmp += nColumnWidth;
			xpaint += nColumnWidth;
			if ((xbmp + nColumnWidth >= FASTBMP_MAXWIDTH) || (xpaint >= rcClient.right)) break;
		} while (++col < maxcol);
		m_Dib.Blit(hdc, xpaint-xbmp, ypaint, xbmp, m_szCell.cy);
	SkipRow:
		ypaint += m_szCell.cy;
		if (ypaint >= rcClient.bottom) break;
	}
	*pypaint = ypaint;

}


void CViewPattern::DrawChannelVUMeter(HDC hdc, int x, int y, UINT nChn)
//---------------------------------------------------------------------
{
	if (ChnVUMeters[nChn] != OldVUMeters[nChn])
	{
		UINT vul, vur;
		vul = ChnVUMeters[nChn] & 0xFF;
		vur = (ChnVUMeters[nChn] & 0xFF00) >> 8;
		vul /= 15;
		vur /= 15;
		if (vul > 8) vul = 8;
		if (vur > 8) vur = 8;
		x += (m_szCell.cx / 2);
		if (m_nDetailLevel <= PatternCursor::instrColumn)
		{
			DibBlt(hdc, x-VUMETERS_LOWIDTH-1, y, VUMETERS_LOWIDTH, VUMETERS_BMPHEIGHT,
				VUMETERS_BMPWIDTH*2+VUMETERS_MEDWIDTH*2, vul * VUMETERS_BMPHEIGHT, CMainFrame::bmpVUMeters);
			DibBlt(hdc, x-1, y, VUMETERS_LOWIDTH, VUMETERS_BMPHEIGHT,
				VUMETERS_BMPWIDTH*2+VUMETERS_MEDWIDTH*2+VUMETERS_LOWIDTH, vur * VUMETERS_BMPHEIGHT, CMainFrame::bmpVUMeters);
		} else
		if (m_nDetailLevel <= PatternCursor::volumeColumn)
		{
			DibBlt(hdc, x - VUMETERS_MEDWIDTH-1, y, VUMETERS_MEDWIDTH, VUMETERS_BMPHEIGHT,
				VUMETERS_BMPWIDTH*2, vul * VUMETERS_BMPHEIGHT, CMainFrame::bmpVUMeters);
			DibBlt(hdc, x, y, VUMETERS_MEDWIDTH, VUMETERS_BMPHEIGHT,
				VUMETERS_BMPWIDTH*2+VUMETERS_MEDWIDTH, vur * VUMETERS_BMPHEIGHT, CMainFrame::bmpVUMeters);
		} else
		{
			DibBlt(hdc, x - VUMETERS_BMPWIDTH - 1, y, VUMETERS_BMPWIDTH, VUMETERS_BMPHEIGHT,
				0, vul * VUMETERS_BMPHEIGHT, CMainFrame::bmpVUMeters);
			DibBlt(hdc, x + 1, y, VUMETERS_BMPWIDTH, VUMETERS_BMPHEIGHT,
				VUMETERS_BMPWIDTH, vur * VUMETERS_BMPHEIGHT, CMainFrame::bmpVUMeters);
		}
		OldVUMeters[nChn] = ChnVUMeters[nChn];
	}
}


// Draw an inverted border around the dragged selection.
void CViewPattern::DrawDragSel(HDC hdc)
//-------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	CRect rect;
	int x1, y1, x2, y2;
	int nChannels, nRows;

	if(pSndFile == nullptr) return;

	// Compute relative movement
	int dx = (int)m_DragPos.GetChannel() - (int)m_StartSel.GetChannel();
	int dy = (int)m_DragPos.GetRow() - (int)m_StartSel.GetRow();

	// Compute destination rect
	PatternCursor begin(m_Selection.GetUpperLeft()), end(m_Selection.GetLowerRight());

	// Check which selection lines need to be drawn.
	bool drawLeft = ((int)begin.GetChannel() + dx >= GetXScrollPos());
	bool drawRight = ((int)end.GetChannel() + dx < (int)pSndFile->GetNumChannels());
	bool drawTop = ((int)begin.GetRow() + dy >= GetYScrollPos() - (int)m_nMidRow);
	bool drawBottom = ((int)end.GetRow() + dy < (int)pSndFile->Patterns[m_nPattern].GetNumRows());

	begin.Move(dy, dx, 0);
	if(begin.GetChannel() >= pSndFile->GetNumChannels())
	{
		// Moved outside pattern range.
		return;
	}
	end.Move(dy, dx, 0);
	begin.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());
	end.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());
	// We need to know the first pixel that's not part of our rect anymore, so we extend the selection.
	end.Move(1, 0, 1);
	PatternRect destination(begin, end);

	x1 = m_Selection.GetStartChannel();
	y1 = m_Selection.GetStartRow();
	x2 = m_Selection.GetEndChannel();
	y2 = m_Selection.GetEndRow();
	PatternCursor::Columns c1 = m_Selection.GetStartColumn();
	PatternCursor::Columns c2 = m_Selection.GetEndColumn();
	x1 += dx;
	x2 += dx;
	y1 += dy;
	y2 += dy;
	nChannels = pSndFile->m_nChannels;
	nRows = pSndFile->Patterns[m_nPattern].GetNumRows();
	if (x1 < GetXScrollPos()) drawLeft = false;
	if (x1 >= nChannels) x1 = nChannels - 1;
	if (x1 < 0) { x1 = 0; c1 = PatternCursor::firstColumn; drawLeft = false; }
	if (x2 >= nChannels) { x2 = nChannels - 1; c2 = PatternCursor::lastColumn; drawRight = false; }
	if (x2 < 0) x2 = 0;
	if (y1 < GetYScrollPos() - (int)m_nMidRow) drawTop = false;
	if (y1 >= nRows) y1 = nRows-1;
	if (y1 < 0) { y1 = 0; drawTop = false; }
	if (y2 >= nRows) { y2 = nRows-1; drawBottom = false; }
	if (y2 < 0) y2 = 0;

	POINT ptTopLeft = GetPointFromPosition(begin);
	POINT ptBottomRight = GetPointFromPosition(end);
	if ((ptTopLeft.x >= ptBottomRight.x) || (ptTopLeft.y >= ptBottomRight.y)) return;

	if(end.GetColumnType() == PatternCursor::firstColumn)
	{
		// Special case: If selection ends on the last column of a channel, subtract the channel separator width.
		ptBottomRight.x -= 4;
	}

	// invert the brush pattern (looks just like frame window sizing)
	CBrush* pBrush = CDC::GetHalftoneBrush();
	if (pBrush != NULL)
	{
		HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, pBrush->m_hObject);
		// Top
		if (drawTop)
		{
			rect.SetRect(ptTopLeft.x + 4, ptTopLeft.y, ptBottomRight.x, ptTopLeft.y + 4);
			if (!drawLeft) rect.left -= 4;
			PatBlt(hdc, rect.left, rect.top, rect.Width(), rect.Height(), PATINVERT);
		}
		// Bottom
		if (drawBottom)
		{
			rect.SetRect(ptTopLeft.x, ptBottomRight.y - 4, ptBottomRight.x - 4, ptBottomRight.y);
			if (!drawRight) rect.right += 4;
			PatBlt(hdc, rect.left, rect.top, rect.Width(), rect.Height(), PATINVERT);
		}
		// Left
		if (drawLeft)
		{
			rect.SetRect(ptTopLeft.x, ptTopLeft.y, ptTopLeft.x + 4, ptBottomRight.y - 4);
			if (!drawBottom) rect.bottom += 4;
			PatBlt(hdc, rect.left, rect.top, rect.Width(), rect.Height(), PATINVERT);
		}
		// Right
		if (drawRight)
		{
			rect.SetRect(ptBottomRight.x - 4, ptTopLeft.y + 4, ptBottomRight.x, ptBottomRight.y);
			if (!drawTop) rect.top -= 4;
			PatBlt(hdc, rect.left, rect.top, rect.Width(), rect.Height(), PATINVERT);
		}
		if (hOldBrush != NULL) SelectObject(hdc, hOldBrush);
	}

}


void CViewPattern::OnDrawDragSel()
//--------------------------------
{
	HDC hdc = ::GetDC(m_hWnd);
	if (hdc != NULL)
	{
		DrawDragSel(hdc);
		::ReleaseDC(m_hWnd, hdc);
	}
}


///////////////////////////////////////////////////////////////////////////////
// CViewPattern Scrolling Functions


void CViewPattern::UpdateScrollSize()
//-----------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile)
	{
		CRect rect;
		SIZE sizeTotal, sizePage, sizeLine;
		sizeTotal.cx = m_szHeader.cx + pSndFile->GetNumChannels() * m_szCell.cx;
		sizeTotal.cy = m_szHeader.cy + pSndFile->Patterns[m_nPattern].GetNumRows() * m_szCell.cy;
		sizeLine.cx = m_szCell.cx;
		sizeLine.cy = m_szCell.cy;
		sizePage.cx = sizeLine.cx * 2;
		sizePage.cy = sizeLine.cy * 8;
		GetClientRect(&rect);
		m_nMidRow = 0;
		if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CENTERROW) m_nMidRow = (rect.Height() - m_szHeader.cy) / (m_szCell.cy << 1);
		if (m_nMidRow) sizeTotal.cy += m_nMidRow * m_szCell.cy * 2;
		SetScrollSizes(MM_TEXT, sizeTotal, sizePage, sizeLine);
		//UpdateScrollPos(); //rewbs.FixLPsOddScrollingIssue
		if (rect.Height() >= sizeTotal.cy)
		{
			m_bWholePatternFitsOnScreen = true;
			m_nYScroll = 0;  //rewbs.fix2977
		} else
		{
			m_bWholePatternFitsOnScreen = false;
		}
	}
}


void CViewPattern::UpdateScrollPos()
//----------------------------------
{
	CRect rect;
	GetClientRect(&rect);
	
	int x = GetScrollPos(SB_HORZ);
	if (x < 0) x = 0;
	m_nXScroll = (x + m_szCell.cx - 1) / m_szCell.cx;
	int y = GetScrollPos(SB_VERT);
	if (y < 0) y = 0;
	m_nYScroll = (y + m_szCell.cy - 1) / m_szCell.cy;

}


BOOL CViewPattern::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
//-------------------------------------------------------------
{
	int xOrig, xNew, x;
	int yOrig, yNew, y;

	// don't scroll if there is no valid scroll range (ie. no scroll bar)
	CScrollBar* pBar;
	DWORD dwStyle = GetStyle();
	pBar = GetScrollBarCtrl(SB_VERT);
	if ((pBar != NULL && !pBar->IsWindowEnabled()) ||
		(pBar == NULL && !(dwStyle & WS_VSCROLL)))
	{
		// vertical scroll bar not enabled
		sizeScroll.cy = 0;
	}
	pBar = GetScrollBarCtrl(SB_HORZ);
	if ((pBar != NULL && !pBar->IsWindowEnabled()) ||
		(pBar == NULL && !(dwStyle & WS_HSCROLL)))
	{
		// horizontal scroll bar not enabled
		sizeScroll.cx = 0;
	}

	// adjust current x position
	xOrig = x = GetScrollPos(SB_HORZ);
	int xMax = GetScrollLimit(SB_HORZ);
	x += sizeScroll.cx;
	if (x < 0) x = 0; else if (x > xMax) x = xMax;

	// adjust current y position
	yOrig = y = GetScrollPos(SB_VERT);
	int yMax = GetScrollLimit(SB_VERT);
	y += sizeScroll.cy;
	if (y < 0) y = 0; else if (y > yMax) y = yMax;

	// did anything change?
	if (x == xOrig && y == yOrig) return FALSE;

	if (!bDoScroll) return TRUE;
	xNew = x;
	yNew = y;

	if (x > 0) x = (x + m_szCell.cx - 1) / m_szCell.cx; else x = 0;
	if (y > 0) y = (y + m_szCell.cy - 1) / m_szCell.cy; else y = 0;
	if ((x != m_nXScroll) || (y != m_nYScroll))
	{
		CRect rect;
		GetClientRect(&rect);
		if (x != m_nXScroll)
		{
			rect.left = m_szHeader.cx;
			rect.top = 0;
			ScrollWindow((m_nXScroll-x)*GetColumnWidth(), 0, &rect, &rect);
			m_nXScroll = x;
		}
		if (y != m_nYScroll)
		{
			rect.left = 0;
			rect.top = m_szHeader.cy;
			ScrollWindow(0, (m_nYScroll-y)*GetColumnHeight(), &rect, &rect);
			m_nYScroll = y;
		}
	}
	if (xNew != xOrig) SetScrollPos(SB_HORZ, xNew);
	if (yNew != yOrig) SetScrollPos(SB_VERT, yNew);
	return TRUE;
}


void CViewPattern::OnSize(UINT nType, int cx, int cy)
//---------------------------------------------------
{
	// TODO: Switching between modules (when MDI childs are maximized) first calls this with the windowed size, then with the maximized size, which makes the scrollbars move. Eww!
	CScrollView::OnSize(nType, cx, cy);
	if (((nType == SIZE_RESTORED) || (nType == SIZE_MAXIMIZED)) && (cx > 0) && (cy > 0))
	{
		UpdateSizes();
		UpdateScrollSize();
		OnScroll(0,0,TRUE);
	}
}


void CViewPattern::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//---------------------------------------------------------------------------
{
	if (nSBCode == (SB_THUMBTRACK|SB_THUMBPOSITION)) m_Status.set(psDragVScroll);
	CModScrollView::OnVScroll(nSBCode, nPos, pScrollBar);
	if (nSBCode == SB_ENDSCROLL) m_Status.reset(psDragVScroll);
}


void CViewPattern::SetCurSel(const PatternCursor &beginSel, const PatternCursor &endSel)
//--------------------------------------------------------------------------------------
{
	RECT rect1, rect2, rect, rcInt, rcUni;
	POINT pt;

	// Get current selection area
	PatternCursor endSel2(m_Selection.GetLowerRight());
	endSel2.Move(1, 0, 1);

	pt = GetPointFromPosition(m_Selection.GetUpperLeft());
	rect1.left = pt.x;
	rect1.top = pt.y;
	pt = GetPointFromPosition(endSel2);
	rect1.right = pt.x;
	rect1.bottom = pt.y;
	if(rect1.left < m_szHeader.cx) rect1.left = m_szHeader.cx;
	if(rect1.top < m_szHeader.cy) rect1.top = m_szHeader.cy;

	// Get new selection area
	m_Selection = PatternRect(beginSel, endSel);
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr)
	{
		m_Selection.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());
	}

	pt = GetPointFromPosition(m_Selection.GetUpperLeft());
	rect2.left = pt.x;
	rect2.top = pt.y;
	endSel2.Set(m_Selection.GetLowerRight());
	endSel2.Move(1, 0, 1);
	pt = GetPointFromPosition(endSel2);
	rect2.right = pt.x;
	rect2.bottom = pt.y;
	if (rect2.left < m_szHeader.cx) rect2.left = m_szHeader.cx;
	if (rect2.top < m_szHeader.cy) rect2.top = m_szHeader.cy;

	// Compute area for invalidation
	IntersectRect(&rcInt, &rect1, &rect2);
	UnionRect(&rcUni, &rect1, &rect2);
	SubtractRect(&rect, &rcUni, &rcInt);
	if ((rect.left == rcUni.left) && (rect.top == rcUni.top)
		&& (rect.right == rcUni.right) && (rect.bottom == rcUni.bottom))
	{
		InvalidateRect(&rect1, FALSE);
		InvalidateRect(&rect2, FALSE);
	} else
	{
		InvalidateRect(&rect, FALSE);
	}
}


void CViewPattern::InvalidatePattern(bool invalidateHeader)
//---------------------------------------------------------
{
	CRect rect;
	GetClientRect(&rect);
	if(!invalidateHeader)
	{
		rect.left += m_szHeader.cx;
		rect.top += m_szHeader.cy;
	}
	InvalidateRect(&rect, FALSE);
}


void CViewPattern::InvalidateRow(ROWINDEX n)
//------------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile)
	{
		int yofs = GetYScrollPos() - m_nMidRow;
		if (n == ROWINDEX_INVALID) n = GetCurrentRow();
		if (((int)n < yofs) || (n >= pSndFile->Patterns[m_nPattern].GetNumRows())) return;
		CRect rect;
		GetClientRect(&rect);
		rect.left = m_szHeader.cx;
		rect.top = m_szHeader.cy;
		rect.top += (n - yofs) * m_szCell.cy;
		rect.bottom = rect.top + m_szCell.cy;
		InvalidateRect(&rect, FALSE);
	}

}


void CViewPattern::InvalidateArea(PatternCursor begin, PatternCursor end)
//-----------------------------------------------------------------------
{
	RECT rect;
	POINT pt;
	pt = GetPointFromPosition(begin);
	rect.left = pt.x;
	rect.top = pt.y;
	end.Move(1, 0, 1);
	pt = GetPointFromPosition(end);
	rect.right = pt.x;
	rect.bottom = pt.y;
	InvalidateRect(&rect, FALSE);
}


void CViewPattern::InvalidateCell(PatternCursor cursor)
//-----------------------------------------------------
{
	cursor.RemoveColType();
	InvalidateArea(cursor, PatternCursor(cursor.GetRow(), cursor.GetChannel(), PatternCursor::lastColumn));
}


void CViewPattern::InvalidateChannelsHeaders()
//--------------------------------------------
{
	CRect rect;
	GetClientRect(&rect);
	rect.bottom = rect.top + m_szHeader.cy;
	InvalidateRect(&rect, FALSE);
}


void CViewPattern::UpdateIndicator()
//----------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm != nullptr && pSndFile != nullptr)
	{
		EffectInfo effectInfo(*pSndFile);

		CHAR s[128];
		CHANNELINDEX nChn;
		wsprintf(s, "Row %d, Col %d", GetCurrentRow(), GetCurrentChannel() + 1);
		pMainFrm->SetUserText(s);
		if (::GetFocus() == m_hWnd)
		{
			nChn = m_Cursor.GetChannel();
			s[0] = 0;
			if(!m_Status[psKeyboardDragSelect]
				&& (m_Selection.GetUpperLeft() == m_Selection.GetLowerRight()) && (pSndFile->Patterns[m_nPattern])
				&& (GetCurrentRow() < pSndFile->Patterns[m_nPattern].GetNumRows()) && (nChn < pSndFile->m_nChannels))
			{
				const ModCommand *m = pSndFile->Patterns[m_nPattern].GetpModCommand(GetCurrentRow(), nChn);

				switch(m_Cursor.GetColumnType())
				{
				case PatternCursor::noteColumn:
					// display note
					if(m->note >= NOTE_MIN_SPECIAL)
						strcpy(s, szSpecialNoteShortDesc[m->note - NOTE_MIN_SPECIAL]);
					break;

				case PatternCursor::instrColumn:
					// display instrument
					if (m->instr)
					{
						CHAR sztmp[128] = "";
						if(m->IsPcNote())
						{
							// display plugin name.
							if(m->instr <= MAX_MIXPLUGINS)
							{
								mpt::String::CopyN(sztmp, pSndFile->m_MixPlugins[m->instr - 1].GetName());
							}
						} else
						{
							// "normal" instrument
							if (pSndFile->GetNumInstruments())
							{
								if ((m->instr <= pSndFile->GetNumInstruments()) && (pSndFile->Instruments[m->instr]))
								{
									ModInstrument *pIns = pSndFile->Instruments[m->instr];
									mpt::String::Copy(sztmp, pIns->name);
									if ((m->note) && (m->note <= NOTE_MAX))
									{
										const SAMPLEINDEX nsmp = pIns->Keyboard[m->note - 1];
										if ((nsmp) && (nsmp <= pSndFile->GetNumSamples()))
										{
											if (pSndFile->m_szNames[nsmp][0])
											{
												wsprintf(sztmp + strlen(sztmp), " (%d: %s)", nsmp, pSndFile->m_szNames[nsmp]);
											}
										}
									}
								}
							} else
							{
								if (m->instr <= pSndFile->GetNumSamples())
								{
									mpt::String::Copy(sztmp, pSndFile->m_szNames[m->instr]);
								}
							}

						}
						mpt::String::SetNullTerminator(sztmp);
						if (sztmp[0]) wsprintf(s, "%d: %s", m->instr, sztmp);
					}
					break;

				case PatternCursor::volumeColumn:
					// display volume command
					if(m->IsPcNote())
					{
						// display plugin param name.
						if(m->instr > 0 && m->instr <= MAX_MIXPLUGINS)
						{
							CHAR sztmp[128] = "";
							mpt::String::CopyN(sztmp, pSndFile->m_MixPlugins[m->instr - 1].GetParamName(m->GetValueVolCol()).c_str());
							if (sztmp[0]) wsprintf(s, "%d: %s", m->GetValueVolCol(), sztmp);
						}
					} else
					{
						// "normal" volume command
						if (!effectInfo.GetVolCmdInfo(effectInfo.GetIndexFromVolCmd(m->volcmd), s)) s[0] = 0;
					}
					break;

				case PatternCursor::effectColumn:
				case PatternCursor::paramColumn:
					// display effect command
					if(!m->IsPcNote())
					{
						if (!effectInfo.GetEffectName(s, m->command, m->param, false, nChn)) s[0] = 0;
					}
					break;
				}
			}
			pMainFrm->SetInfoText(s);
			UpdateXInfoText();		//rewbs.xinfo
		}
	}

}

//rewbs.xinfo
void CViewPattern::UpdateXInfoText()
//----------------------------------
{
	UINT nChn = GetCurrentChannel();
	CString xtraInfo;

	const CSoundFile *pSndFile = GetSoundFile();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm != nullptr && pSndFile != nullptr)
	{
		//xtraInfo.Format("Chan: %d; macro: %X; cutoff: %X; reso: %X; pan: %X",
		xtraInfo.Format("Chn:%d; Vol:%X; Mac:%X; Cut:%X%s; Res:%X; Pan:%X%s",
			nChn + 1,
			pSndFile->Chn[nChn].nGlobalVol,
			pSndFile->Chn[nChn].nActiveMacro,
			pSndFile->Chn[nChn].nCutOff,
			(pSndFile->Chn[nChn].nFilterMode == FLTMODE_HIGHPASS) ? "-Hi" : "",
			pSndFile->Chn[nChn].nResonance,
			pSndFile->Chn[nChn].nPan,
			pSndFile->Chn[nChn].dwFlags[CHN_SURROUND] ? "-S" : "");

		pMainFrm->SetXInfoText(xtraInfo);
	}

	return;
}
//end rewbs.xinfo


void CViewPattern::UpdateAllVUMeters(Notification *pnotify)
//---------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	const CModDoc *pModDoc = GetDocument();
	const CSoundFile *pSndFile;
	CRect rcClient;
	HDC hdc;
	BOOL bPlaying;
	UINT nChn;
	int x, xofs;
	
	if ((!pModDoc) || (!pMainFrm)) return;
	GetClientRect(&rcClient);
	xofs = GetXScrollPos();
	pSndFile = pModDoc->GetSoundFile();
	hdc = ::GetDC(m_hWnd);
	bPlaying = (pMainFrm->GetFollowSong(pModDoc) == m_hWnd) ? TRUE : FALSE;
	x = m_szHeader.cx;
	nChn = xofs;
	while ((nChn < pSndFile->m_nChannels) && (x < rcClient.right))
	{
		ChnVUMeters[nChn] = (WORD)pnotify->pos[nChn];
		if ((!bPlaying) || pnotify->type[Notification::Stop]) ChnVUMeters[nChn] = 0;
		DrawChannelVUMeter(hdc, x + 1, rcClient.top + COLHDR_HEIGHT, nChn);
		nChn++;
		x += m_szCell.cx;
	}
	::ReleaseDC(m_hWnd, hdc);
}

