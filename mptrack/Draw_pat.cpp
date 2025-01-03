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
#include "View_pat.h"
#include "dlg_misc.h"
#include "EffectVis.h"
#include "Globals.h"
#include "HighDPISupport.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Mptrack.h"
#include "../soundlib/tuning.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/Tables.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "../common/mptStringBuffer.h"
#include "EffectInfo.h"
#include "PatternFont.h"


OPENMPT_NAMESPACE_BEGIN


// Headers
enum
{
	ROWHDR_WIDTH           = 32,      // Row header
	COLHDR_HEIGHT          = 16 + 4,  // Column header (name + color)
	PLUGNAME_HEIGHT        = 16,      // Height of plugin names
	VUMETERS_HEIGHT        = 13,      // Height of vu-meters (including padding)
	VUMETERS_HEIGHT_LED    = 10,      // Height of vu-meters (without padding)
	VUMETERS_LEDS_PER_SIDE = 8,
	SEPARATOR_WIDTH        = 4,
};

enum
{
	COLUMN_BITS_NONE          = 0x00,
	COLUMN_BITS_NOTE          = 0x01,
	COLUMN_BITS_INSTRUMENT    = 0x02,
	COLUMN_BITS_VOLUME        = 0x04,
	COLUMN_BITS_FXCMD         = 0x08,
	COLUMN_BITS_FXPARAM       = 0x10,
	COLUMN_BITS_FXCMDANDPARAM = 0x18,
	COLUMN_BITS_ALLCOLUMNS    = 0x1F,
	COLUMN_BITS_SKIP          = 0x40,
	COLUMN_BITS_INVISIBLE     = 0x80,
};


/////////////////////////////////////////////////////////////////////////////
// Effect colour codes

// EffectType => ModColor mapping
static constexpr int effectColors[] =
{
	0,
	MODCOLOR_GLOBALS,
	MODCOLOR_VOLUME,
	MODCOLOR_PANNING,
	MODCOLOR_PITCH,
};

static_assert(std::size(effectColors) == static_cast<size_t>(EffectType::NumTypes));

/////////////////////////////////////////////////////////////////////////////
// CViewPattern Drawing Implementation

static uint8 HighlightColor(int c0, int c1)
{
	int cf0 = 0xC0 - (c1 >> 2) - (c0 >> 3);
	Limit(cf0, 0x40, 0xC0);
	int cf1 = 0x100 - cf0;
	return static_cast<uint8>((c0 * cf0 + c1 * cf1) >> 8);
}


static void MixColors(CFastBitmap &dib, ModColor target, ModColor src1, ModColor src2)
{
	const auto c1 = TrackerSettings::Instance().rgbCustomColors[src1], c2 = TrackerSettings::Instance().rgbCustomColors[src2];
	auto r = HighlightColor(GetRValue(c1), GetRValue(c2));
	auto g = HighlightColor(GetGValue(c1), GetGValue(c2));
	auto b = HighlightColor(GetBValue(c1), GetBValue(c2));
	dib.SetColor(target, RGB(r, g, b));
}


void CViewPattern::UpdateColors()
{
	m_Dib.SetAllColors(0, MAX_MODCOLORS, TrackerSettings::Instance().rgbCustomColors.data());
	MixColors(m_Dib, MODCOLOR_2NDHIGHLIGHT, MODCOLOR_BACKHILIGHT, MODCOLOR_BACKNORMAL);
	MixColors(m_Dib, MODCOLOR_DEFAULTVOLUME, MODCOLOR_VOLUME, MODCOLOR_BACKNORMAL);
	MixColors(m_Dib, MODCOLOR_DUMMYCOMMAND, MODCOLOR_TEXTNORMAL, MODCOLOR_BACKNORMAL);
	m_Dib.SetBlendColor(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BLENDCOLOR]);

	CreateVUMeterBitmap();
}


void CViewPattern::UpdateVisibileColumns(std::bitset<PatternCursor::numColumns> visibleColumns)
{
	m_visibleColumns = visibleColumns;
	m_visibleColumns.set(PatternCursor::noteColumn);  // Cannot be disabled at the moment
	m_visibleColumns.set(PatternCursor::paramColumn, m_visibleColumns[PatternCursor::effectColumn]);
	UpdateSizes();
	UpdateScrollSize();
	SetCurrentColumn(m_Cursor);
	InvalidatePattern(true, true);
}


bool CViewPattern::UpdateSizes()
{
	const PATTERNFONT *pfnt = PatternFont::currentFont;
	int oldx = m_szCell.cx, oldy = m_szCell.cy;
	m_szHeader.cx = ROWHDR_WIDTH;
	m_szHeader.cy = COLHDR_HEIGHT;
	m_szPluginHeader.cx = 0;
	m_szPluginHeader.cy = m_Status[psShowPluginNames] ? MulDiv(PLUGNAME_HEIGHT, m_dpi, 96) : 0;
	if(m_Status[psShowVUMeters]) m_szHeader.cy += VUMETERS_HEIGHT;
	m_szCell.cx = SEPARATOR_WIDTH;
	for(size_t i = 0; i <= PatternCursor::lastColumn; i++)
	{
		if(m_visibleColumns[static_cast<PatternCursor::Columns>(i)])
			m_szCell.cx += pfnt->nEltWidths[i];
	}
	m_szCell.cy = pfnt->nHeight;

	m_szHeader.cx = MulDiv(m_szHeader.cx, m_dpi, 96);
	m_szHeader.cy = MulDiv(m_szHeader.cy, m_dpi, 96);
	m_szHeader.cy += m_szPluginHeader.cy;

	if(oldy != m_szCell.cy)
	{
		m_Dib.SetSize(m_Dib.GetWidth(), m_szCell.cy);
	}

	if(oldx != m_szCell.cx || oldy != m_szCell.cy)
	{
		CreateVUMeterBitmap();
	}

	return (oldx != m_szCell.cx || oldy != m_szCell.cy);
}


UINT CViewPattern::GetColumnOffset(PatternCursor::Columns column) const
{
	const PATTERNFONT *pfnt = PatternFont::currentFont;
	LimitMax(column, PatternCursor::lastColumn);
	UINT offset = 0;
	for(int i = PatternCursor::firstColumn; i < column; i++)
		offset += pfnt->nEltWidths[i];
	return offset;
}


int CViewPattern::GetSmoothScrollOffset() const
{
	if((TrackerSettings::Instance().patternSetup & PatternSetup::SmoothScrolling)	// Actually using the smooth scroll feature
		&& (m_Status & (psFollowSong | psDragActive)) == psFollowSong	// Not drawing a selection during playback
		&& (m_nMidRow != 0 || GetYScrollPos() > 0)	// If active row is not centered, only scroll when display position is actually not at the top
		&& IsLiveRecord()	// Actually playing live (not paused or stepping)
		&& m_nNextPlayRow != m_nPlayRow)	// Don't scroll if we stay on the same row
	{
		uint32 tick = m_nPlayTick;
		// Avoid jerky animation with backwards-going patterns
		if(m_nNextPlayRow == m_nPlayRow - 1) tick = m_nTicksOnRow - m_nPlayTick - 1;
		return Util::muldivr_unsigned(m_szCell.cy, tick, std::max(1u, m_nTicksOnRow));
	}
	return 0;
}


void CViewPattern::UpdateView(UpdateHint hint, CObject *pObj)
{
	if(pObj == this)
	{
		return;
	}
	if(hint.GetType()[HINT_MPTOPTIONS])
	{
		PatternFont::UpdateFont(m_hWnd);
		UpdateColors();
		UpdateSizes();
		UpdateScrollSize();
		InvalidatePattern(true, true);
		return;
	}
	const auto generalHint = hint.ToType<GeneralHint>();
	if(generalHint.GetType()[HINT_MODTYPE | HINT_MODCHANNELS])
	{
		InvalidateChannelsHeaders();
		UpdateScrollSize();
	}
	if(generalHint.GetType()[HINT_MODTYPE])
	{
		// If sequence and pattern view became inconsistent (e.g. due to rearranging patterns during cleanup), synchronize to order list again
		const auto &order = Order();
		const ORDERINDEX ord = GetCurrentOrder();
		if(order.IsValidPat(ord) && order.at(ord) != m_nPattern)
			SetCurrentPattern(order.at(ord));
	}
	if(generalHint.GetType()[HINT_MODCHANNELS]
	   && m_quickChannelProperties.m_hWnd
	   && pObj != &m_quickChannelProperties
	   && (generalHint.GetChannel() >= GetDocument()->GetNumChannels() || generalHint.GetChannel() == m_quickChannelProperties.GetChannel()))
	{
		m_quickChannelProperties.UpdateDisplay();
	}

	const PatternHint patternHint = hint.ToType<PatternHint>();
	const PATTERNINDEX updatePat = patternHint.GetPattern();
	if(hint.GetType() == HINT_PATTERNDATA
		&& m_nPattern != updatePat
		&& updatePat != 0
		&& updatePat != GetNextPattern()
		&& updatePat != GetPrevPattern())
		return;

	if(patternHint.GetType()[HINT_MODTYPE | HINT_PATTERNDATA])
	{
		InvalidatePattern(false, true);
	} else if(patternHint.GetType()[HINT_PATTERNROW])
	{
		InvalidateRow(static_cast<const RowHint &>(hint).GetRow());
	}
}


POINT CViewPattern::GetPointFromPosition(PatternCursor cursor) const
{
	const PATTERNFONT *pfnt = PatternFont::currentFont;
	POINT pt;
	int xofs = GetXScrollPos();
	int yofs = GetYScrollPos();

	PatternCursor::Columns imax = cursor.GetColumnType();
	LimitMax(imax, PatternCursor::lastColumn);
	pt.x = (cursor.GetChannel() - xofs) * GetChannelWidth();

	for(int i = 0; i < imax; i++)
	{
		if(m_visibleColumns[static_cast<PatternCursor::Columns>(i)])
			pt.x += pfnt->nEltWidths[i];
	}

	if(pt.x < 0)
		pt.x = 0;
	pt.x += HighDPISupport::ScalePixels(ROWHDR_WIDTH, m_hWnd);
	pt.y = (cursor.GetRow() - yofs + m_nMidRow) * m_szCell.cy;

	if (pt.y < 0) pt.y = 0;
	pt.y += m_szHeader.cy;

	return pt;
}


PatternCursor CViewPattern::GetPositionFromPoint(POINT pt) const
{
	const PATTERNFONT *pfnt = PatternFont::currentFont;
	int xofs = GetXScrollPos();
	int yofs = GetYScrollPos();
	int x = xofs + (pt.x - m_szHeader.cx) / GetChannelWidth();
	if (pt.x < m_szHeader.cx) x = (xofs) ? xofs - 1 : 0;

	int y = yofs - m_nMidRow + (pt.y - m_szHeader.cy + GetSmoothScrollOffset()) / m_szCell.cy;
	if (y < 0) y = 0;
	int xx = (pt.x - m_szHeader.cx) % GetChannelWidth(), dx = 0;
	size_t i = 0;
	for(; i < PatternCursor::lastColumn; i++)
	{
		if(m_visibleColumns[static_cast<PatternCursor::Columns>(i)])
		dx += pfnt->nEltWidths[i];
		if(xx < dx)
			break;
	}
	return PatternCursor(static_cast<ROWINDEX>(y), static_cast<CHANNELINDEX>(x), static_cast<PatternCursor::Columns>(i));
}


void CViewPattern::DrawLetter(int x, int y, char letter, int sizex, int ofsx)
{
	const PATTERNFONT *pfnt = PatternFont::currentFont;

	if(pfnt->dibASCII)
	{
		if(32 <= letter && letter <= 127)
		{
			m_Dib.TextBlt(x, y, sizex, pfnt->spacingY, (((unsigned char)letter) * pfnt->nNoteWidth[0]) + ofsx, 0, pfnt->dibASCII);
			return;
		}
	}

	int srcx = pfnt->nSpaceX, srcy = pfnt->nSpaceY;

	if ((letter >= '0') && (letter <= '9'))
	{
		srcx = pfnt->nNumX;
		srcy = pfnt->nNumY + (letter - '0') * pfnt->spacingY;
	} else
	if ((letter >= 'A') && (letter < 'N'))
	{
		srcx = pfnt->nAlphaAM_X;
		srcy = pfnt->nAlphaAM_Y + (letter - 'A') * pfnt->spacingY;
	} else
	if ((letter >= 'N') && (letter <= 'Z'))
	{
		srcx = pfnt->nAlphaNZ_X;
		srcy = pfnt->nAlphaNZ_Y + (letter - 'N') * pfnt->spacingY;
	} else
	switch(letter)
	{
	case '?':
		srcx = pfnt->nAlphaNZ_X;
		srcy = pfnt->nAlphaNZ_Y + 13 * pfnt->spacingY;
		break;
	case '#':
		srcx = pfnt->nAlphaAM_X;
		srcy = pfnt->nAlphaAM_Y + 13 * pfnt->spacingY;
		break;
	case '\\':
		srcx = pfnt->nAlphaNZ_X;
		srcy = pfnt->nAlphaNZ_Y + 14 * pfnt->spacingY;
		break;
	case ':':
		srcx = pfnt->nAlphaNZ_X;
		srcy = pfnt->nAlphaNZ_Y + 15 * pfnt->spacingY;
		break;
	case '*':
		srcx = pfnt->nAlphaNZ_X;
		srcy = pfnt->nAlphaNZ_Y + 16 * pfnt->spacingY;
		break;
	case ' ':
		srcx = pfnt->nSpaceX;
		srcy = pfnt->nSpaceY;
		break;
	case 'b':
		srcx = pfnt->nAlphaAM_X;
		srcy = pfnt->nAlphaAM_Y + 14 * pfnt->spacingY;
		break;
	case '-':
		srcx = pfnt->nAlphaAM_X;
		srcy = pfnt->nAlphaAM_Y + 15 * pfnt->spacingY;
		break;
	case '+':
		srcx = pfnt->nAlphaAM_X;
		srcy = pfnt->nAlphaAM_Y + 16 * pfnt->spacingY;
		break;
	case 'd':
		srcx = pfnt->nAlphaAM_X;
		srcy = pfnt->nAlphaAM_Y + 17 * pfnt->spacingY;
		break;
	case '.':
		srcx = pfnt->nNoteX;
		srcy = pfnt->nNoteY;
		break;
	}
	m_Dib.TextBlt(x, y, sizex, pfnt->spacingY, srcx+ofsx, srcy, pfnt->dib);
}

void CViewPattern::DrawLetter(int x, int y, wchar_t letter, int sizex, int ofsx)
{
	DrawLetter(x, y, mpt::unsafe_char_convert<char>(letter), sizex, ofsx);
}

#if MPT_CXX_AT_LEAST(20)
void CViewPattern::DrawLetter(int x, int y, char8_t letter, int sizex, int ofsx)
{
	DrawLetter(x, y, mpt::unsafe_char_convert<char>(letter), sizex, ofsx);
}
#endif

static MPT_FORCEINLINE void DrawPadding(CFastBitmap &dib, const PATTERNFONT *pfnt, int x, int y, PatternCursor::Columns col)
{
	if(pfnt->padding[col])
		dib.TextBlt(x + pfnt->nEltWidths[col] - pfnt->padding[col], y, pfnt->padding[col], pfnt->spacingY, pfnt->nClrX + pfnt->nEltWidths[col] - pfnt->padding[col], pfnt->nClrY, pfnt->dib);
}

void CViewPattern::DrawNote(int x, int y, UINT note, CTuning* pTuning)
{
	const PATTERNFONT *pfnt = PatternFont::currentFont;

	UINT xsrc = pfnt->nNoteX, ysrc = pfnt->nNoteY, dx = pfnt->nEltWidths[0];
	if (!note)
	{
		m_Dib.TextBlt(x, y, dx, pfnt->spacingY, xsrc, ysrc, pfnt->dib);
	} else
	if (note == NOTE_NOTECUT)
	{
		m_Dib.TextBlt(x, y, dx, pfnt->spacingY, xsrc, ysrc + 13*pfnt->spacingY, pfnt->dib);
	} else
	if (note == NOTE_KEYOFF)
	{
		m_Dib.TextBlt(x, y, dx, pfnt->spacingY, xsrc, ysrc + 14*pfnt->spacingY, pfnt->dib);
	} else
	if(note == NOTE_FADE)
	{
		m_Dib.TextBlt(x, y, dx, pfnt->spacingY, xsrc, ysrc + 17*pfnt->spacingY, pfnt->dib);
	} else
	if(note == NOTE_PC)
	{
		m_Dib.TextBlt(x, y, dx, pfnt->spacingY, xsrc, ysrc + 15*pfnt->spacingY, pfnt->dib);
	} else
	if(note == NOTE_PCS)
	{
		m_Dib.TextBlt(x, y, dx, pfnt->spacingY, xsrc, ysrc + 16*pfnt->spacingY, pfnt->dib);
	} else
	{
		if(pTuning)
		{
			// Drawing custom note names
			std::wstring noteStr = mpt::ToWide(pTuning->GetNoteName(static_cast<Tuning::NOTEINDEXTYPE>(note - NOTE_MIDDLEC)));
			if(noteStr.size() < 3)
				noteStr.resize(3, L' ');
			
			DrawLetter(x, y, noteStr[0], pfnt->nNoteWidth[0], 0);
			DrawLetter(x + pfnt->nNoteWidth[0], y, noteStr[1], pfnt->nNoteWidth[1], 0);
			DrawLetter(x + pfnt->nNoteWidth[0] + pfnt->nNoteWidth[1], y, noteStr[2], pfnt->nOctaveWidth, 0);
		} else
		{
			// Original
			UINT o = (note - NOTE_MIN) / 12; //Octave
			UINT n = (note - NOTE_MIN) % 12; //Note

			// Hack for default pattern font, allowing for sharps
			if(TrackerSettings::Instance().accidentalFlats)
			{
				DrawLetter(x, y, NoteNamesFlat[n][0], pfnt->nNoteWidth[0], 0);
				DrawLetter(x + pfnt->nNoteWidth[0], y, NoteNamesFlat[n][1], pfnt->nNoteWidth[1], 0);
			} else
			{
				m_Dib.TextBlt(x, y, pfnt->nNoteWidth[0] + pfnt->nNoteWidth[1], pfnt->spacingY, xsrc, ysrc+(n+1)*pfnt->spacingY, pfnt->dib);
			}

			if(o <= 15)
				m_Dib.TextBlt(x + pfnt->nNoteWidth[0] + pfnt->nNoteWidth[1], y, pfnt->nOctaveWidth, pfnt->spacingY,
								pfnt->nNumX, pfnt->nNumY+o*pfnt->spacingY, pfnt->dib);
			else
				DrawLetter(x + pfnt->nNoteWidth[0] + pfnt->nNoteWidth[1], y, '?', pfnt->nOctaveWidth);
		}
	}
	DrawPadding(m_Dib, pfnt, x, y, PatternCursor::noteColumn);
}


void CViewPattern::DrawInstrument(int x, int y, UINT instr)
{
	const PATTERNFONT *pfnt = PatternFont::currentFont;
	if (instr)
	{
		UINT dx = pfnt->nInstrHiWidth;
		if (instr < 100)
		{
			m_Dib.TextBlt(x, y, dx, pfnt->spacingY, pfnt->nNumX+pfnt->nInstrOfs, pfnt->nNumY+(instr / 10)*pfnt->spacingY, pfnt->dib);
		} else
		{
			m_Dib.TextBlt(x, y, dx, pfnt->spacingY, pfnt->nNum10X+pfnt->nInstr10Ofs, pfnt->nNum10Y+((instr-100) / 10)*pfnt->spacingY, pfnt->dib);
		}
		m_Dib.TextBlt(x+dx, y, pfnt->nEltWidths[1]-dx, pfnt->spacingY, pfnt->nNumX+pfnt->paramLoMargin, pfnt->nNumY+(instr % 10)*pfnt->spacingY, pfnt->dib);
	} else
	{
		m_Dib.TextBlt(x, y, pfnt->nEltWidths[1], pfnt->spacingY, pfnt->nClrX+pfnt->nEltWidths[0], pfnt->nClrY, pfnt->dib);
	}
	DrawPadding(m_Dib, pfnt, x, y, PatternCursor::instrColumn);
}


void CViewPattern::DrawVolumeCommand(int x, int y, const ModCommand &mc, std::optional<int> defaultVolume, bool hex)
{
	const PATTERNFONT *pfnt = PatternFont::currentFont;

	if(mc.IsPcNote())
	{
		//If note is parameter control note, drawing volume command differently.
		const int val = std::min(ModCommand::maxColumnValue, static_cast<int>(mc.GetValueVolCol()));

		if(pfnt->pcParamMargin) m_Dib.TextBlt(x, y, pfnt->pcParamMargin, pfnt->spacingY, pfnt->nClrX, pfnt->nClrY, pfnt->dib);
		m_Dib.TextBlt(x + pfnt->pcParamMargin, y, pfnt->nVolCmdWidth, pfnt->spacingY,
							pfnt->nNumX, pfnt->nNumY+(val / 100)*pfnt->spacingY, pfnt->dib);
		m_Dib.TextBlt(x+pfnt->nVolCmdWidth, y, pfnt->nVolHiWidth, pfnt->spacingY,
							pfnt->nNumX, pfnt->nNumY+((val / 10)%10)*pfnt->spacingY, pfnt->dib);
		m_Dib.TextBlt(x+pfnt->nVolCmdWidth+pfnt->nVolHiWidth, y, pfnt->nEltWidths[2]-(pfnt->nVolCmdWidth+pfnt->nVolHiWidth), pfnt->spacingY,
							pfnt->nNumX, pfnt->nNumY+(val % 10)*pfnt->spacingY, pfnt->dib);
	} else
	{
		ModCommand::VOLCMD volcmd = mc.volcmd;
		int vol = (mc.vol & 0x7F);

		if(defaultVolume)
		{
			// Displaying sample default volume if there is no volume command.
			volcmd = VOLCMD_VOLUME;
			vol = *defaultVolume;
		}

		if(volcmd != VOLCMD_NONE && volcmd < MAX_VOLCMDS)
		{
			m_Dib.TextBlt(x, y, pfnt->nVolCmdWidth, pfnt->spacingY,
							pfnt->nVolX, pfnt->nVolY + volcmd * pfnt->spacingY, pfnt->dib);
			const int digit1 = vol / (hex ? 16 : 10);
			const int digit2 = vol % (hex ? 16 : 10);
			m_Dib.TextBlt(x+pfnt->nVolCmdWidth, y, pfnt->nVolHiWidth, pfnt->spacingY,
							pfnt->nNumX, pfnt->nNumY + digit1 * pfnt->spacingY, pfnt->dib);
			m_Dib.TextBlt(x+pfnt->nVolCmdWidth + pfnt->nVolHiWidth, y, pfnt->nEltWidths[2] - (pfnt->nVolCmdWidth + pfnt->nVolHiWidth), pfnt->spacingY,
							pfnt->nNumX, pfnt->nNumY + digit2 * pfnt->spacingY, pfnt->dib);
		} else
		{
			int srcx = pfnt->nEltWidths[0];
			if(m_visibleColumns[PatternCursor::instrColumn])
				srcx += pfnt->nEltWidths[1];
			m_Dib.TextBlt(x, y, pfnt->nEltWidths[2], pfnt->spacingY, pfnt->nClrX+srcx, pfnt->nClrY, pfnt->dib);
		}
	}
	DrawPadding(m_Dib, pfnt, x, y, PatternCursor::volumeColumn);
}


void CViewPattern::OnDraw(CDC *pDC)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CHAR s[256];
	CRect rcClient, rect, rc;
	const CModDoc *pModDoc;

	MPT_ASSERT(pDC);
	UpdateSizes();
	if ((pModDoc = GetDocument()) == nullptr)
		return;
	m_chnState.resize(pModDoc->GetNumChannels());

	FlagSet<PatternSetup> patternSetup = TrackerSettings::Instance().patternSetup;
	const int vuHeight = MulDiv(VUMETERS_HEIGHT, m_dpi, 96);
	const int colHeight = MulDiv(COLHDR_HEIGHT, m_dpi, 96);
	const int chanColorHeight = MulDiv(4, m_dpi, 96);
	const int chanColorOffset = MulDiv(2, m_dpi, 96);
	const int recordInsX = MulDiv(3, m_dpi, 96);
	const bool doSmoothScroll = patternSetup[PatternSetup::SmoothScrolling];
	const int lineWidth = HighDPISupport::ScalePixels(1, *this);

	GetClientRect(&rcClient);
	CRect clipRect;
	pDC->GetClipBox(clipRect);

	HDC hdc;
	if(doSmoothScroll)
	{
		if(rcClient != m_oldClient)
		{
			m_offScreenBitmap.DeleteObject();
			m_offScreenDC.DeleteDC();
			m_offScreenDC.CreateCompatibleDC(pDC);
			m_offScreenBitmap.CreateCompatibleBitmap(pDC, rcClient.Width(), rcClient.Height());
			m_oldClient = rcClient;
		}
		hdc = m_offScreenDC;
	} else
	{
		// Off-screen DC for drawing horizontal (channel buttons) and vertical (row number buttons) headers to avoid flicker
		CRect buttonRect{0, 0, rcClient.Width(), std::max(m_szHeader.cy, m_szCell.cy)};
		if(buttonRect.Width() > m_oldClient.Width() || buttonRect.Height() > m_oldClient.Height())
		{
			m_offScreenBitmap.DeleteObject();
			m_offScreenDC.DeleteDC();
			m_offScreenDC.CreateCompatibleDC(pDC);
			m_offScreenBitmap.CreateCompatibleBitmap(pDC, buttonRect.Width(), buttonRect.Height());
			m_oldClient = buttonRect;
		}
		hdc = pDC->m_hDC;
	}
	HBITMAP oldBitmap = SelectBitmap(m_offScreenDC, m_offScreenBitmap);

	const auto dcBrush = GetStockBrush(DC_BRUSH);
	const auto faceColor = GetSysColor(COLOR_BTNFACE);
	const auto shadowColor = GetSysColor(COLOR_BTNSHADOW);
	const auto textColor = GetSysColor(COLOR_BTNTEXT);

	CHANNELINDEX xofs = static_cast<CHANNELINDEX>(GetXScrollPos());
	ROWINDEX yofs = static_cast<ROWINDEX>(GetYScrollPos());
	const CSoundFile &sndFile = pModDoc->GetSoundFile();
	const UINT nColumnWidth = m_szCell.cx;
	const UINT numChannels = sndFile.GetNumChannels();
	int ypaint = rcClient.top + m_szHeader.cy - GetSmoothScrollOffset();
	const auto &order = Order();
	const ORDERINDEX ordCount = Order().GetLength();

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
			if(patternSetup[PatternSetup::ShowPrevNextPattern])
			{
				if(m_nOrder > 0 && m_nOrder < ordCount)
				{
					ORDERINDEX prevOrder = order.GetPreviousOrderIgnoringSkips(m_nOrder);
					//Skip +++ items

					if(m_nOrder < order.size() && order[m_nOrder] == m_nPattern)
					{
						nPrevPat = order[prevOrder];
					}
				}
			}
			if(sndFile.Patterns.IsValidPat(nPrevPat))
			{
				ROWINDEX nPrevRows = sndFile.Patterns[nPrevPat].GetNumRows();
				ROWINDEX n = std::min(static_cast<ROWINDEX>(nSkip), nPrevRows);

				ypaint += (nSkip - n) * m_szCell.cy;
				rect.SetRect(0, m_szHeader.cy, nColumnWidth * numChannels + m_szHeader.cx, ypaint - 1);
				m_Dib.SetBlendMode(true);
				DrawPatternData(hdc, lineWidth, nPrevPat, false, false,
						nPrevRows - n, nPrevRows, xofs, rcClient, &ypaint);
				m_Dib.SetBlendMode(false);
			} else
			{
				ypaint += nSkip * m_szCell.cy;
				rect.SetRect(0, m_szHeader.cy, nColumnWidth * numChannels + m_szHeader.cx, ypaint - 1);
			}
			if ((rect.bottom > rect.top) && (rect.right > rect.left))
			{
				::SetDCBrushColor(hdc, faceColor);
				::FillRect(hdc, &rect, dcBrush);
				auto shadowRect = rect;
				shadowRect.top = shadowRect.bottom++;
				::SetDCBrushColor(hdc, shadowColor);
				::FillRect(hdc, &shadowRect, dcBrush);
			}
			yofs = 0;
		}
	}

	UINT nrows = sndFile.Patterns.IsValidPat(m_nPattern) ? sndFile.Patterns[m_nPattern].GetNumRows() : 0;
	int ypatternend = ypaint + (nrows-yofs)*m_szCell.cy;
	DrawPatternData(hdc, lineWidth, m_nPattern, true, (pMainFrm->GetModPlaying() == pModDoc),
					yofs, nrows, xofs, rcClient, &ypaint);
	// Display next pattern
	if(patternSetup[PatternSetup::ShowPrevNextPattern] && (ypaint < rcClient.bottom) && (ypaint == ypatternend))
	{
		int nVisRows = (rcClient.bottom - ypaint + m_szCell.cy - 1) / m_szCell.cy;
		if(nVisRows > 0)
		{
			PATTERNINDEX nNextPat = PATTERNINDEX_INVALID;
			ORDERINDEX nNextOrder = order.GetNextOrderIgnoringSkips(m_nOrder);
			if(nNextOrder == m_nOrder) nNextOrder = ORDERINDEX_INVALID;
			//Ignore skip items(+++) from sequence.

			if(m_nOrder < ordCount && nNextOrder < ordCount && order[m_nOrder] == m_nPattern)
			{
				nNextPat = order[nNextOrder];
			}
			if(sndFile.Patterns.IsValidPat(nNextPat))
			{
				ROWINDEX nNextRows = sndFile.Patterns[nNextPat].GetNumRows();
				ROWINDEX n = std::min(static_cast<ROWINDEX>(nVisRows), nNextRows);

				m_Dib.SetBlendMode(true);
				DrawPatternData(hdc, lineWidth, nNextPat, false, false,
						0, n, xofs, rcClient, &ypaint);
				m_Dib.SetBlendMode(false);
			}
		}
	}
	// Drawing outside pattern area
	int xpaint = m_szHeader.cx + (numChannels - xofs) * nColumnWidth;
	if ((xpaint < rcClient.right) && (ypaint > rcClient.top))
	{
		rc.SetRect(xpaint, rcClient.top, rcClient.right, ypaint);
		::SetDCBrushColor(hdc, faceColor);
		::FillRect(hdc, &rc, dcBrush);
	}
	if (ypaint < rcClient.bottom)
	{
		int width = HighDPISupport::ScalePixels(1, m_hWnd);
		rc.SetRect(0, ypaint, rcClient.right + 1, rcClient.bottom + 1);
		if(width == 1)
			DrawButtonRect(hdc, lineWidth, rc, _T(""));
		else
			DrawEdge(hdc, rc, EDGE_RAISED, BF_TOPLEFT | BF_MIDDLE);  // Prevent lower edge from being drawn
	}
	// Drawing pattern selection
	if(m_Status[psDragnDropping])
	{
		DrawDragSel(hdc);
	}

	const auto buttonBrush = GetSysColorBrush(COLOR_BTNFACE), blackBrush = GetStockBrush(BLACK_BRUSH);
	rect.SetRect(0, rcClient.top, rcClient.right, rcClient.top + m_szHeader.cy);
	if(pDC->RectVisible(rect))
	{
		sprintf(s, "#%u", m_nPattern);
		rect.right = m_szHeader.cx;
		if(pDC->RectVisible(rect))
		{
			DrawButtonRect(m_offScreenDC, lineWidth, rect, s, false,
				m_bInItemRect && m_nDragItem.Type() == DragItem::PatternHeader);
		}

		// Drawing Channel Headers
		const int dropWidth = HighDPISupport::ScalePixels(2, m_hWnd);
		UINT chn = xofs;
		ypaint = rcClient.top;
		for(xpaint = m_szHeader.cx; xpaint < clipRect.right; xpaint += nColumnWidth, chn++)
		{
			rect.SetRect(xpaint, ypaint, xpaint + nColumnWidth, ypaint + m_szHeader.cy);
			if(chn < numChannels)
			{
				if(!pDC->RectVisible(rect))
					continue;
				const auto &channel = sndFile.ChnSettings[chn];
				const auto recordGroup = pModDoc->GetChannelRecordGroup(static_cast<CHANNELINDEX>(chn));
				const char *pszfmt = sndFile.m_bChannelMuteTogglePending[chn]? "[Channel %u]" : "Channel %u";
				if(channel.szName[0] != 0)
					pszfmt = sndFile.m_bChannelMuteTogglePending[chn] ? "%u: [%s]" : "%u: %s";
				else if(const auto numVisibleColums = m_visibleColumns.count(); numVisibleColums < 2)
					pszfmt = sndFile.m_bChannelMuteTogglePending[chn] ? "[%u]" : "%u";
				else if(numVisibleColums < 3)
					pszfmt = sndFile.m_bChannelMuteTogglePending[chn] ? "[Ch%u]" : "Ch%u";
				else if(numVisibleColums < 5)
					pszfmt = sndFile.m_bChannelMuteTogglePending[chn] ? "[Chn %u]" : "Chn %u";
				sprintf(s, pszfmt, chn + 1, channel.szName.buf);
				DrawButtonRect(m_offScreenDC, lineWidth, rect, s,
					channel.dwFlags[CHN_MUTE],
					m_bInItemRect && m_nDragItem.Type() == DragItem::ChannelHeader && m_nDragItem.Value() == chn,
					recordGroup != RecordGroup::NoGroup ? DT_RIGHT : DT_CENTER, chanColorHeight);

				if(channel.color != ModChannelSettings::INVALID_COLOR)
				{
					// Channel color
					CRect r;
					r.top = rect.top + chanColorOffset;
					r.bottom = r.top + chanColorHeight;
					r.left = rect.left + chanColorOffset;
					r.right = rect.right - chanColorOffset;

					::SetDCBrushColor(m_offScreenDC, channel.color);
					::FillRect(m_offScreenDC, r, dcBrush);
				}

				// When dragging around channel headers, mark insertion position
				if(m_Status[psDragging] && !m_bInItemRect
				   && m_nDragItem.Type() == DragItem::ChannelHeader
				   && m_nDropItem.Type() == DragItem::ChannelHeader
				   && m_nDropItem.Value() == chn)
				{
					CRect r;
					r.top = rect.top;
					r.bottom = rect.bottom;
					// Drop position depends on whether hovered channel is left or right of dragged item.
					r.left = (m_nDropItem.Value() < m_nDragItem.Value() || m_Status[psShiftDragging]) ? rect.left : rect.right - dropWidth;
					r.right = r.left + dropWidth;

					::SetDCBrushColor(m_offScreenDC, textColor);
					::FillRect(m_offScreenDC, r, dcBrush);
				}

				rect.bottom = rect.top + colHeight;
				rect.top += chanColorHeight;

				if(recordGroup != RecordGroup::NoGroup)
				{
					CRect insRect;
					insRect.SetRect(xpaint, ypaint + chanColorHeight, xpaint + nColumnWidth / 8 + recordInsX, ypaint + colHeight);
					FrameRect(m_offScreenDC, &rect, buttonBrush);
					InvertRect(m_offScreenDC, &rect);
					s[0] = (recordGroup == RecordGroup::Group1) ? '1' : '2';
					s[1] = '\0';
					DrawButtonRect(m_offScreenDC, lineWidth, insRect, s, false, false, DT_CENTER);
					FrameRect(m_offScreenDC, &insRect, blackBrush);
				}

				if(m_Status[psShowVUMeters])
				{
					m_chnState[chn].vuMeterOld = 0;
					DrawChannelVUMeter(m_offScreenDC, rect.left, rect.bottom, chn);
					rect.top += vuHeight;
					rect.bottom += vuHeight;
				}
				if(m_Status[psShowPluginNames])
				{
					rect.top = rect.bottom;
					rect.bottom = rect.top + m_szPluginHeader.cy;
					if(PLUGINDEX mixPlug = channel.nMixPlugin; mixPlug != 0)
						sprintf(s, "%u: %s", mixPlug, (sndFile.m_MixPlugins[mixPlug - 1]).pMixPlugin ? sndFile.m_MixPlugins[mixPlug - 1].GetNameLocale() : "[empty]");
					else
						sprintf(s, "---");
					DrawButtonRect(m_offScreenDC, lineWidth, rect, s, channel.dwFlags[CHN_NOFX],
						m_bInItemRect && (m_nDragItem.Type() == DragItem::PluginName) && (m_nDragItem.Value() == chn), DT_CENTER);
				}
			} else
			{
				break;
			}
		}
		if(!doSmoothScroll)
		{
			pDC->BitBlt(clipRect.left, clipRect.top, xpaint - clipRect.left, m_szHeader.cy - clipRect.top, &m_offScreenDC, clipRect.left, clipRect.top, SRCCOPY);
		}
	}

	if(doSmoothScroll)
	{
		pDC->BitBlt(clipRect.left, clipRect.top, clipRect.Width(), clipRect.Height(), &m_offScreenDC, clipRect.left, clipRect.top, SRCCOPY);
	}
	SelectBitmap(m_offScreenDC, oldBitmap);

	if (m_pEffectVis)
	{
		//HACK: Update visualizer on every pattern redraw. Cleary there's space for opt here.
		if (m_pEffectVis->m_hWnd) m_pEffectVis->Update();
	}
}


static constexpr UINT EncodeRowColor(int rowBkCol, int rowCol, bool rowSelected)
{
	return (rowBkCol << 16) | (rowCol << 8) | (rowSelected ? 1 : 0);
}


void CViewPattern::DrawPatternData(HDC hdc, const int lineWidth, PATTERNINDEX nPattern, bool selEnable,
	bool isPlaying, ROWINDEX startRow, ROWINDEX numRows, CHANNELINDEX startChan, CRect &rcClient, int *pypaint)
{
	static_assert(1 << PatternCursor::lastColumn <= Util::MaxValueOfType(ChannelState{}.selectedCols), "Columns are used as bitmasks");
	static_assert(!((1 << PatternCursor::lastColumn) & (COLUMN_BITS_INVISIBLE | COLUMN_BITS_SKIP)), "Column bits and special bits overlap");

	const CSoundFile &sndFile = GetDocument()->GetSoundFile();
	if(!sndFile.Patterns.IsValidPat(nPattern))
		return;
	const CPattern &pattern = sndFile.Patterns[nPattern];
	const FlagSet<PatternSetup> patternSetupFlags = TrackerSettings::Instance().patternSetup;
	const bool volumeColumnIsHex = TrackerSettings::Instance().patternVolColHex;

	const PATTERNFONT *pfnt = PatternFont::currentFont;
	CRect rect;
	int xpaint, ypaint = *pypaint;
	
	const CHANNELINDEX ncols = sndFile.GetNumChannels();
	const UINT nColumnWidth = m_szCell.cx;
	rect.SetRect(m_szHeader.cx, rcClient.top, m_szHeader.cx+nColumnWidth, rcClient.bottom);
	for(CHANNELINDEX cmk = startChan; cmk < ncols; cmk++)
	{
		m_chnState[cmk].selectedCols = selEnable ? m_Selection.GetSelectionBits(cmk) : 0;
		if(!::RectVisible(hdc, &rect))
			m_chnState[cmk].selectedCols |= COLUMN_BITS_INVISIBLE;
		rect.left += nColumnWidth;
		rect.right += nColumnWidth;
	}
	// Max Visible Column
	CHANNELINDEX maxcol = ncols;
	while((maxcol > startChan) && (m_chnState[maxcol -1].selectedCols & COLUMN_BITS_INVISIBLE))
		maxcol--;

	// Check if there's no "hole" in the visible columns (to speed up empty pattern cell drawing)
	bool allColumnsConsecutive = true;
	for(int i = LastVisibleColumn(); i >= 0; i--)
	{
		if(!m_visibleColumns[static_cast<PatternCursor::Columns>(i)])
		{
			allColumnsConsecutive = false;
			break;
		}
	}

	// Init bitmap border
	{
		UINT maxndx = sndFile.GetNumChannels() * m_szCell.cx;
		UINT ibmp = 0;
		if (maxndx > (UINT)m_Dib.GetWidth()) maxndx = m_Dib.GetWidth();
		do
		{
			ibmp += nColumnWidth;
			m_Dib.TextBlt(ibmp - SEPARATOR_WIDTH, 0, SEPARATOR_WIDTH, m_szCell.cy, pfnt->nClrX + pfnt->nWidth - SEPARATOR_WIDTH, pfnt->nClrY, pfnt->dib);
		} while (ibmp + nColumnWidth <= maxndx);
	}

	// Time signature highlighting
	ROWINDEX nBeat = sndFile.m_nDefaultRowsPerBeat, nMeasure = sndFile.m_nDefaultRowsPerMeasure;
	if(TrackerSettings::Instance().patternIgnoreSongTimeSignature)
	{
		nBeat = TrackerSettings::Instance().m_nRowHighlightBeats;
		nMeasure = TrackerSettings::Instance().m_nRowHighlightMeasures;
	} else if(sndFile.Patterns[nPattern].GetOverrideSignature())
	{
		nBeat = sndFile.Patterns[nPattern].GetRowsPerBeat();
		nMeasure = sndFile.Patterns[nPattern].GetRowsPerMeasure();
	}

	const bool hexNumbers = patternSetupFlags[PatternSetup::RowAndOrderNumbersHex];
	bool bRowSel = false;
	int row_col = -1, row_bkcol = -1;
	for(ROWINDEX row = startRow; row < numRows; row++)
	{
		UINT col, xbmp, nbmp, oldrowcolor;
		const int compRow = row + TrackerSettings::Instance().rowDisplayOffset;

		rect.left = 0;
		rect.top = ypaint;
		rect.right = rcClient.right;
		rect.bottom = rect.top + m_szCell.cy;
		if (!::RectVisible(hdc, &rect))
		{
			// No speedup for these columns next time
			for(CHANNELINDEX iup = startChan; iup < maxcol; iup++)
				m_chnState[iup].selectedCols &= ~COLUMN_BITS_SKIP;
			// skip row
			ypaint += m_szCell.cy;
			if(ypaint >= rcClient.bottom)
				break;
			continue;
		}
		rect.right = rect.left + m_szHeader.cx;

		const bool rowDisabled = sndFile.m_lockRowStart != ROWINDEX_INVALID && (row < sndFile.m_lockRowStart || row > sndFile.m_lockRowEnd);
		// Draw button with row number
		{
			TCHAR s[32];
			if(hexNumbers)
				wsprintf(s, _T("%s%02X"), compRow < 0 ? _T("-") : _T(""), std::abs(compRow));
			else
				wsprintf(s, _T("%d"), compRow);

			// We already draw to the off-screen buffer in case of smooth scrolling
			const bool drawOffscreen = !patternSetupFlags[PatternSetup::SmoothScrolling];
			const CRect drawRect = drawOffscreen ? CRect{0, 0, rect.Width(), rect.Height()} : rect;
			DrawButtonRect(drawOffscreen ? m_offScreenDC : hdc, lineWidth, drawRect, s, !selEnable || rowDisabled);
			if(drawOffscreen)
				::BitBlt(hdc, rect.left, rect.top, rect.Width(), rect.Height(), m_offScreenDC, 0, 0, SRCCOPY);
		}

		oldrowcolor = EncodeRowColor(row_bkcol, row_col, bRowSel);
		bRowSel = (m_Selection.ContainsVertical(PatternCursor(row)));
		row_col = MODCOLOR_TEXTNORMAL;
		row_bkcol = MODCOLOR_BACKNORMAL;

		// secondary highlight (beats)
		ROWINDEX highlightRow = compRow;
		if(nMeasure > 0)
			highlightRow %= nMeasure;
		if(patternSetupFlags[PatternSetup::HighlightBeats] && nBeat > 0)
		{
			if((highlightRow % nBeat) == 0)
			{
				row_bkcol = MODCOLOR_2NDHIGHLIGHT;
			}
		}
		// primary highlight (measures)
		if(patternSetupFlags[PatternSetup::HighlightMeasures] && nMeasure > 0)
		{
			if(highlightRow == 0)
			{
				row_bkcol = MODCOLOR_BACKHILIGHT;
			}
		}
		bool blendModeChanged = false;
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
					row_bkcol = m_Status[psRecordingEnabled] ? MODCOLOR_BACKRECORDROW : MODCOLOR_BACKCURROW;
				} else
				if(m_Status[psFollowSong] && isPlaying)
				{
					row_col = MODCOLOR_TEXTPLAYCURSOR;
					row_bkcol = MODCOLOR_BACKPLAYCURSOR;
				}
			}
			blendModeChanged = (rowDisabled != m_Dib.GetBlendMode());
			m_Dib.SetBlendMode(rowDisabled);
		}
		// Eliminate non-visible column
		xpaint = m_szHeader.cx;
		col = startChan;
		while((m_chnState[col].selectedCols & COLUMN_BITS_INVISIBLE) && (col < maxcol))
		{
			m_chnState[col].selectedCols &= ~COLUMN_BITS_SKIP;
			col++;
			xpaint += nColumnWidth;
		}
		// Optimization: same row color ?
		const bool useSpeedUpMask = (oldrowcolor == EncodeRowColor(row_bkcol, row_col, bRowSel)) && !blendModeChanged;
		xbmp = nbmp = 0;
		do
		{
			int x = 0, xClear = 0, bk_col, tx_col, col_sel, fx_col;

			const ModCommand *m = pattern.GetpModCommand(row, static_cast<CHANNELINDEX>(col));

			// Should empty volume commands be replaced with a volume command showing the default volume?
			const auto defaultVolume = patternSetupFlags[PatternSetup::ShowDefaultVolume] ? DrawDefaultVolume(*m) : std::nullopt;

			DWORD dwSpeedUpMask = 0;
			if(useSpeedUpMask && (m_chnState[col].selectedCols & COLUMN_BITS_SKIP) && (row))
			{
				const ModCommand *mold = m - ncols;
				const auto oldDefaultVolume = patternSetupFlags[PatternSetup::ShowDefaultVolume] ? DrawDefaultVolume(*mold) : std::nullopt;

				if(m->note == mold->note || !m_visibleColumns[PatternCursor::noteColumn])
					dwSpeedUpMask |= COLUMN_BITS_NOTE;
				if((m->instr == mold->instr) || !m_visibleColumns[PatternCursor::instrColumn])
					dwSpeedUpMask |= COLUMN_BITS_INSTRUMENT;
				if (m->IsPcNote() || mold->IsPcNote())
				{
					// Handle speedup mask for PC notes.
					if(m->note == mold->note)
					{
						if(m->GetValueVolCol() == mold->GetValueVolCol() || !m_visibleColumns[PatternCursor::volumeColumn])
							dwSpeedUpMask |= COLUMN_BITS_VOLUME;
						if(m->GetValueEffectCol() == mold->GetValueEffectCol() || !m_visibleColumns[PatternCursor::effectColumn])
							dwSpeedUpMask |= COLUMN_BITS_FXCMDANDPARAM;
					}
				} else
				{
					if ((m->volcmd == mold->volcmd && (m->volcmd == VOLCMD_NONE || m->vol == mold->vol) && !defaultVolume && !oldDefaultVolume) || !m_visibleColumns[PatternCursor::volumeColumn])
						dwSpeedUpMask |= COLUMN_BITS_VOLUME;
					if ((m->command == mold->command) || !m_visibleColumns[PatternCursor::effectColumn])
						dwSpeedUpMask |= (m->command != CMD_NONE) ? COLUMN_BITS_FXCMD : COLUMN_BITS_FXCMDANDPARAM;
				}
				if (dwSpeedUpMask == COLUMN_BITS_ALLCOLUMNS) goto DoBlit;
			}
			m_chnState[col].selectedCols |= COLUMN_BITS_SKIP;
			col_sel = 0;
			if(bRowSel) col_sel = m_chnState[col].selectedCols & COLUMN_BITS_ALLCOLUMNS;
			tx_col = row_col;
			bk_col = row_bkcol;
			if (col_sel)
			{
				tx_col = MODCOLOR_TEXTSELECTED;
				bk_col = MODCOLOR_BACKSELECTED;
			}
			// Speedup: Empty command which is either not or fully selected
			if (m->IsEmpty() && ((!col_sel) || (col_sel == COLUMN_BITS_ALLCOLUMNS)) && allColumnsConsecutive)
			{
				m_Dib.SetTextColor(tx_col, bk_col);
				m_Dib.TextBlt(xbmp, 0, nColumnWidth - SEPARATOR_WIDTH, m_szCell.cy, pfnt->nClrX, pfnt->nClrY, pfnt->dib);
				goto DoBlit;
			}

			// Note
			if (!(dwSpeedUpMask & COLUMN_BITS_NOTE))
			{
				tx_col = row_col;
				bk_col = row_bkcol;
				if(patternSetupFlags[PatternSetup::EffectHighlight] && m->IsNote())
				{
					tx_col = MODCOLOR_NOTE;

					if(sndFile.m_SongFlags[SONG_AMIGALIMITS | SONG_PT_MODE])
					{
						// Highlight notes that exceed the Amiga's frequency range.
						if(sndFile.GetType() == MOD_TYPE_MOD && (m->note < NOTE_MIDDLEC - 12 || m->note >= NOTE_MIDDLEC + 2 * 12))
						{
							tx_col = MODCOLOR_DODGY_COMMANDS;
						} else if(sndFile.GetType() == MOD_TYPE_S3M && m->instr != 0 && m->instr <= sndFile.GetNumSamples())
						{
							uint32 period = sndFile.GetPeriodFromNote(m->note, 0, sndFile.GetSample(m->instr).nC5Speed);
							if(period < 113 * 4 || period > 856 * 4)
							{
								tx_col = MODCOLOR_DODGY_COMMANDS;
							}
						}
					}
				}
				if (col_sel & COLUMN_BITS_NOTE)
				{
					tx_col = MODCOLOR_TEXTSELECTED;
					bk_col = MODCOLOR_BACKSELECTED;
				}
				// Drawing note
				m_Dib.SetTextColor(tx_col, bk_col);
				MPT_MAYBE_CONSTANT_IF(sndFile.GetType() == MOD_TYPE_MPT && m->instr < MAX_INSTRUMENTS && sndFile.Instruments[m->instr])
					DrawNote(xbmp+x, 0, m->note, sndFile.Instruments[m->instr]->pTuning);
				else //Original
					DrawNote(xbmp+x, 0, m->note);
			}
			x += pfnt->nEltWidths[0];
			xClear += pfnt->nEltWidths[0];
			// Instrument
			if (m_visibleColumns[PatternCursor::instrColumn])
			{
				if (!(dwSpeedUpMask & COLUMN_BITS_INSTRUMENT))
				{
					tx_col = row_col;
					bk_col = row_bkcol;
					if(patternSetupFlags[PatternSetup::EffectHighlight] && (m->instr))
					{
						tx_col = MODCOLOR_INSTRUMENT;
					}
					if (col_sel & COLUMN_BITS_INSTRUMENT)
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
			xClear += pfnt->nEltWidths[1];
			// Volume
			if (m_visibleColumns[PatternCursor::volumeColumn])
			{
				if (!(dwSpeedUpMask & COLUMN_BITS_VOLUME))
				{
					tx_col = row_col;
					bk_col = row_bkcol;
					if (col_sel & COLUMN_BITS_VOLUME)
					{
						tx_col = MODCOLOR_TEXTSELECTED;
						bk_col = MODCOLOR_BACKSELECTED;
					} else if (!m->IsPcNote() && patternSetupFlags[PatternSetup::EffectHighlight])
					{
						auto fxColor = effectColors[static_cast<size_t>(m->GetVolumeEffectType())];
						if(m->volcmd != VOLCMD_NONE && m->volcmd < MAX_VOLCMDS && fxColor != 0)
						{
							tx_col = fxColor;
						} else if(defaultVolume)
						{
							tx_col = MODCOLOR_DEFAULTVOLUME;
						}
					}
					// Drawing Volume
					m_Dib.SetTextColor(tx_col, bk_col);
					DrawVolumeCommand(xbmp + x, 0, *m, defaultVolume, volumeColumnIsHex);
				}
				x += pfnt->nEltWidths[2];
			}
			xClear += pfnt->nEltWidths[2];
			// Command & param
			if (m_visibleColumns[PatternCursor::effectColumn])
			{
				const bool isPCnote = m->IsPcNote();
				uint16 val = m->GetValueEffectCol();
				if(val > ModCommand::maxColumnValue) val = ModCommand::maxColumnValue;
				fx_col = row_col;
				if (!isPCnote && m->command != CMD_NONE && m->command < MAX_EFFECTS && patternSetupFlags[PatternSetup::EffectHighlight])
				{
					if(auto fxColor = effectColors[static_cast<size_t>(m->GetEffectType())]; fxColor != 0)
						fx_col = fxColor;
					else if(m->command == CMD_DUMMY)
						fx_col = MODCOLOR_DUMMYCOMMAND;
				}
				if (!(dwSpeedUpMask & COLUMN_BITS_FXCMD))
				{
					tx_col = fx_col;
					bk_col = row_bkcol;
					if (col_sel & COLUMN_BITS_FXCMD)
					{
						tx_col = MODCOLOR_TEXTSELECTED;
						bk_col = MODCOLOR_BACKSELECTED;
					}

					// Drawing Command
					m_Dib.SetTextColor(tx_col, bk_col);
					if(isPCnote)
					{
						m_Dib.TextBlt(xbmp + x, 0, 2, pfnt->spacingY, pfnt->nClrX + xClear, pfnt->nClrY, pfnt->dib);
						m_Dib.TextBlt(xbmp + x + pfnt->pcValMargin, 0, pfnt->nEltWidths[3], m_szCell.cy, pfnt->nNumX, pfnt->nNumY+(val / 100)*pfnt->spacingY, pfnt->dib);
					} else
					{
						if(m->command != CMD_NONE)
						{
							char n = sndFile.GetModSpecifications().GetEffectLetter(m->command);
							MPT_ASSERT(n >= ' ');
							DrawLetter(xbmp+x, 0, n, pfnt->nEltWidths[3], pfnt->nCmdOfs);
						} else
						{
							m_Dib.TextBlt(xbmp+x, 0, pfnt->nEltWidths[3], pfnt->spacingY, pfnt->nClrX + xClear, pfnt->nClrY, pfnt->dib);
						}
					}
					DrawPadding(m_Dib, pfnt, xbmp + x, 0, PatternCursor::effectColumn);
				}
				x += pfnt->nEltWidths[3];
				xClear += pfnt->nEltWidths[3];
				// Param
				if (!(dwSpeedUpMask & COLUMN_BITS_FXPARAM))
				{
					tx_col = fx_col;
					bk_col = row_bkcol;
					if (col_sel & COLUMN_BITS_FXPARAM)
					{
						tx_col = MODCOLOR_TEXTSELECTED;
						bk_col = MODCOLOR_BACKSELECTED;
					}

					// Drawing param
					m_Dib.SetTextColor(tx_col, bk_col);
					if(isPCnote)
					{
						m_Dib.TextBlt(xbmp + x, 0, pfnt->nParamHiWidth, m_szCell.cy, pfnt->nNumX, pfnt->nNumY+((val / 10) % 10)*pfnt->spacingY, pfnt->dib);
						m_Dib.TextBlt(xbmp + x + pfnt->nParamHiWidth, 0, pfnt->nEltWidths[4] - pfnt->padding[4] - pfnt->nParamHiWidth, m_szCell.cy, pfnt->nNumX+pfnt->paramLoMargin, pfnt->nNumY+(val % 10)*pfnt->spacingY, pfnt->dib);
					}
					else
					{
						if (m->command)
						{
							m_Dib.TextBlt(xbmp + x, 0, pfnt->nParamHiWidth, m_szCell.cy, pfnt->nNumX, pfnt->nNumY+(m->param >> 4)*pfnt->spacingY, pfnt->dib);
							m_Dib.TextBlt(xbmp + x + pfnt->nParamHiWidth, 0, pfnt->nEltWidths[4] - pfnt->padding[4] - pfnt->nParamHiWidth, m_szCell.cy, pfnt->nNumX+pfnt->paramLoMargin, pfnt->nNumY+(m->param & 0x0F)*pfnt->spacingY, pfnt->dib);
						} else
						{
							m_Dib.TextBlt(xbmp+x, 0, pfnt->nEltWidths[4], m_szCell.cy, pfnt->nClrX + xClear, pfnt->nClrY, pfnt->dib);
						}
					}
					DrawPadding(m_Dib, pfnt, xbmp + x, 0, PatternCursor::paramColumn);
				}
			}
		DoBlit:
			nbmp++;
			xbmp += nColumnWidth;
			xpaint += nColumnWidth;
			if ((xbmp + nColumnWidth >= (UINT)m_Dib.GetWidth()) || (xpaint >= rcClient.right)) break;
		} while (++col < maxcol);
		m_Dib.Blit(hdc, xpaint-xbmp, ypaint, xbmp, m_szCell.cy);
		// skip row
		ypaint += m_szCell.cy;
		if (ypaint >= rcClient.bottom) break;
	}
	*pypaint = ypaint;
}


std::optional<int> CViewPattern::DrawDefaultVolume(const ModCommand &m) const
{
	if(m.instr == 0 || m.volcmd != VOLCMD_NONE || m.command == CMD_VOLUME || m.command == CMD_VOLUME8)
		return std::nullopt;
	return GetDefaultVolume(m, 0);
}


void CViewPattern::DrawChannelVUMeter(HDC hdc, int x, int y, UINT nChn)
{
	if(m_chnState[nChn].vuMeter == m_chnState[nChn].vuMeterOld)
		return;
	
	uint8 vuL = static_cast<uint8>((m_chnState[nChn].vuMeter & 0xFF00) >> 8);
	uint8 vuR = static_cast<uint8>(m_chnState[nChn].vuMeter & 0xFF);
	vuL /= 15;
	vuR /= 15;
	LimitMax(vuL, uint8(8));
	LimitMax(vuR, uint8(8));

	const int barWidth = m_ledWidth * VUMETERS_LEDS_PER_SIDE;
	const int midSpacer = (m_ledWidth > 2) ? m_ledWidth / 2 : 0;

	const auto &channel = GetSoundFile()->m_PlayState.Chn[nChn];
	const bool isSynth =
		channel.dwFlags[CHN_ADLIB]
		|| (channel.pModSample != nullptr && channel.pModSample->uFlags[CHN_ADLIB])
		|| ((channel.pModSample == nullptr || !channel.pModSample->HasSampleData()) && channel.HasMIDIOutput());

	const auto oldBitmap = m_vuMeterDC.SelectObject(m_vuMeterBitmap);
	const int srcOffsetX = isSynth ? barWidth * 2 : 0;
	x += (m_szCell.cx / 2);
	BitBlt(hdc, x - barWidth - (midSpacer - midSpacer / 2), y, barWidth, m_ledHeight, m_vuMeterDC, srcOffsetX, vuL * m_ledHeight, SRCCOPY);
	BitBlt(hdc, x + midSpacer / 2, y, barWidth, m_ledHeight, m_vuMeterDC, srcOffsetX + barWidth, vuR * m_ledHeight, SRCCOPY);
	m_vuMeterDC.SelectObject(oldBitmap);

	m_chnState[nChn].vuMeterOld = m_chnState[nChn].vuMeter;
}


// Draw an inverted border around the dragged selection.
void CViewPattern::DrawDragSel(HDC hdc)
{
	const CSoundFile *pSndFile = GetSoundFile();
	CRect rect;
	int x1, y1, x2, y2;
	int nChannels, nRows;

	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern)) return;

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
	x1 += dx;
	x2 += dx;
	y1 += dy;
	y2 += dy;
	nChannels = pSndFile->GetNumChannels();
	nRows = pSndFile->Patterns[m_nPattern].GetNumRows();
	if (x1 < GetXScrollPos()) drawLeft = false;
	if (x1 >= nChannels) x1 = nChannels - 1;
	if (x1 < 0) { x1 = 0; drawLeft = false; }
	if (x2 >= nChannels) { x2 = nChannels - 1; drawRight = false; }
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
		ptBottomRight.x -= SEPARATOR_WIDTH;
	}

	// invert the brush pattern (looks just like frame window sizing)
	::SetTextColor(hdc, RGB(255, 255, 255));
	::SetBkColor(hdc, RGB(0, 0, 0));
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
{
	const CSoundFile *pSndFile = GetSoundFile();
	const CHANNELINDEX numChannels = pSndFile ? pSndFile->GetNumChannels() : 0;
	const ROWINDEX numRows = (pSndFile && pSndFile->Patterns.IsValidPat(m_nPattern)) ? pSndFile->Patterns[m_nPattern].GetNumRows() : 0;

	CRect rect;
	SIZE sizeTotal, sizePage, sizeLine;
	sizeTotal.cx = m_szHeader.cx + numChannels * m_szCell.cx;
	sizeTotal.cy = m_szHeader.cy + numRows * m_szCell.cy;
	sizeLine.cx = m_szCell.cx;
	sizeLine.cy = m_szCell.cy;
	sizePage.cx = sizeLine.cx * 2;
	sizePage.cy = sizeLine.cy * 8;
	GetClientRect(&rect);
	m_nMidRow = 0;
	if(TrackerSettings::Instance().patternSetup & PatternSetup::CenterActiveRow)
		m_nMidRow = (rect.Height() - m_szHeader.cy) / (m_szCell.cy * 2);
	if(m_nMidRow)
		sizeTotal.cy += m_nMidRow * m_szCell.cy * 2;
	SetScrollSizes(MM_TEXT, sizeTotal, sizePage, sizeLine);
	m_bWholePatternFitsOnScreen = (rect.Height() >= sizeTotal.cy);
	if(m_bWholePatternFitsOnScreen)
		m_nYScroll = 0;
}


void CViewPattern::UpdateScrollPos()
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

	if (!bDoScroll) return TRUE;
	xNew = x;
	yNew = y;

	if (x > 0) x = (x + m_szCell.cx - 1) / m_szCell.cx; else x = 0;
	if (y > 0) y = (y + m_szCell.cy - 1) / m_szCell.cy; else y = 0;
	if ((x != m_nXScroll) || (y != m_nYScroll))
	{
		CRect rect;
		GetClientRect(&rect);
		// HACK:
		// Wine handles ScrollWindow completely synchronously (using RedrawWindow).
		// This causes the window update region to be repainted immediately
		// before and immediately after the actual copying of the scrolled rect.
		// Async and sync window painting generally do not mix well at all
		// (not even on native Windows) and this causes inevitable flickering
		// on Wine.
		// Instead, just invalidate the whole scrolled window area and let
		// WM_PAINT handle the whole mess without ever scrolling any already
		// painted contents. This causes additional CPU usage (on Wine) but 
		// avoids totally annoying and distracting flickering of the current-row-
		// highlight.
		if (x != m_nXScroll)
		{
			rect.left = m_szHeader.cx;
			rect.top = 0;
			if(TrackerSettings::Instance().patternAlwaysDrawWholePatternOnScrollSlow || mpt::OS::Windows::IsWine())
			{
				InvalidateRect(&rect, FALSE);
			} else
			{
				ScrollWindow((m_nXScroll - x) * GetChannelWidth(), 0, &rect, &rect);
			}
			m_nXScroll = x;
		}
		if (y != m_nYScroll)
		{
			rect.left = 0;
			rect.top = m_szHeader.cy;
			if(TrackerSettings::Instance().patternAlwaysDrawWholePatternOnScrollSlow || mpt::OS::Windows::IsWine())
			{
				InvalidateRect(&rect, FALSE);
			} else
			{
				ScrollWindow(0, (m_nYScroll - y) * GetRowHeight(), &rect, &rect);
			}
			m_nYScroll = y;
		}
	}
	if (xNew != xOrig) SetScrollPos(SB_HORZ, xNew);
	if (yNew != yOrig) SetScrollPos(SB_VERT, yNew);
	return TRUE;
}


void CViewPattern::OnSize(UINT nType, int cx, int cy)
{
	// Note: Switching between modules (when MDI childs are maximized) first calls this with the windowed size, then with the maximized size.
	// Watch out for this odd behaviour when debugging this function.
	CScrollView::OnSize(nType, cx, cy);
	if (((nType == SIZE_RESTORED) || (nType == SIZE_MAXIMIZED)) && (cx > 0) && (cy > 0))
	{
		UpdateSizes();
		UpdateScrollSize();
		UpdateScrollPos();
		m_Dib.SetSize(cx + m_szCell.cx, m_szCell.cy);
		InvalidatePattern();
	}
}


void CViewPattern::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (nSBCode == SB_THUMBTRACK) m_Status.set(psDragVScroll);
	CModScrollView::OnVScroll(nSBCode, nPos, pScrollBar);
	if (nSBCode == SB_ENDSCROLL) m_Status.reset(psDragVScroll);
}


void CViewPattern::SetCurSel(PatternCursor beginSel, PatternCursor endSel)
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
	if(const CSoundFile *sndFile = GetSoundFile(); sndFile != nullptr && sndFile->Patterns.IsValidPat(m_nPattern))
	{
		m_Selection.Sanitize(sndFile->Patterns[m_nPattern].GetNumRows(), sndFile->GetNumChannels(), LastVisibleColumn());
	}
	UpdateIndicator();

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


void CViewPattern::InvalidatePattern(bool invalidateChannelHeaders, bool invalidateRowHeaders)
{
	CRect rect;
	GetClientRect(&rect);
	if(!invalidateChannelHeaders)
	{
		rect.top += m_szHeader.cy;
	}
	if(!invalidateRowHeaders)
	{
		rect.left += m_szHeader.cx;
	}
	InvalidateRect(&rect, FALSE);
	SanitizeCursor();
}


void CViewPattern::InvalidateRow(ROWINDEX n)
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile && pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		int yofs = GetYScrollPos() - m_nMidRow;
		if (n == ROWINDEX_INVALID) n = GetCurrentRow();
		if (((int)n < yofs) || (n >= pSndFile->Patterns[m_nPattern].GetNumRows())) return;
		CRect rect;
		GetClientRect(&rect);
		rect.left = m_szHeader.cx;
		rect.top = m_szHeader.cy - GetSmoothScrollOffset();
		rect.top += (n - yofs) * m_szCell.cy;
		rect.bottom = rect.top + m_szCell.cy;
		InvalidateRect(&rect, FALSE);
	}

}


void CViewPattern::InvalidateArea(PatternCursor begin, PatternCursor end)
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
{
	cursor.RemoveColType();
	InvalidateArea(cursor, PatternCursor(cursor.GetRow(), cursor.GetChannel(), PatternCursor::lastColumn));
}


void CViewPattern::InvalidateChannelsHeaders(CHANNELINDEX chn)
{
	CRect rect;
	GetClientRect(&rect);
	rect.bottom = rect.top + m_szHeader.cy;
	if(chn != CHANNELINDEX_INVALID)
	{
		rect.left = GetPointFromPosition(PatternCursor{ 0u, chn }).x;
		rect.right = rect.left + GetChannelWidth();
	}
	InvalidateRect(&rect, FALSE);
}


void CViewPattern::UpdateIndicator(bool updateAccessibility)
{
	const CSoundFile *sndFile = GetSoundFile();
	CMainFrame *mainFrm = CMainFrame::GetMainFrame();
	if(mainFrm == nullptr || sndFile == nullptr || !sndFile->Patterns.IsValidPat(m_nPattern))
		return;

	mainFrm->SetUserText(MPT_CFORMAT("Row {}, Col {}")(GetCurrentRow(), GetCurrentChannel() + 1));
	if(::GetFocus() == m_hWnd)
	{
		const bool hasSelection = m_Selection.GetUpperLeft() != m_Selection.GetLowerRight();
		if(hasSelection)
			mainFrm->SetInfoText(MPT_CFORMAT("Selection: {} row{}, {} channel{}")(m_Selection.GetNumRows(), CString(m_Selection.GetNumRows() != 1 ? _T("s") : _T("")), m_Selection.GetNumChannels(), CString(m_Selection.GetNumChannels() != 1 ? _T("s") : _T(""))));
		if(GetCurrentRow() < sndFile->Patterns[m_nPattern].GetNumRows() && m_Cursor.GetChannel() < sndFile->GetNumChannels())
		{
			if(!hasSelection)
				mainFrm->SetInfoText(GetCursorDescription());
			UpdateXInfoText();
		}
		if(updateAccessibility)
			mainFrm->NotifyAccessibilityUpdate(*this);
	}
}


CString CViewPattern::GetCursorDescription() const
{
	const CSoundFile &sndFile = *GetSoundFile();
	CString s;
	if(!sndFile.Patterns.IsValidPat(m_nPattern))
	{
		return s;
	}
	ROWINDEX row = m_Cursor.GetRow();
	CHANNELINDEX channel = m_Cursor.GetChannel();
	const ModCommand *m = sndFile.Patterns[m_nPattern].GetpModCommand(row, channel);

	switch(m_Cursor.GetColumnType())
	{
	case PatternCursor::noteColumn:
		// display note
		if(m->IsSpecialNote())
			s = szSpecialNoteShortDesc[m->note - NOTE_MIN_SPECIAL];
		else if(m->IsNote())
			s = mpt::ToCString(sndFile.GetNoteName(m->note, m->instr));
		break;

	case PatternCursor::instrColumn:
		// display instrument
		if(m->instr)
		{
			s.Format(_T("%u: "), m->instr);
			if(m->IsPcNote())
			{
				// display plugin name.
				if(m->instr <= MAX_MIXPLUGINS)
				{
					s += mpt::ToCString(sndFile.m_MixPlugins[m->instr - 1].GetName());
				}
			} else
			{
				// "normal" instrument
				if(sndFile.GetNumInstruments())
				{
					if((m->instr <= sndFile.GetNumInstruments()) && (sndFile.Instruments[m->instr]))
					{
						ModInstrument *pIns = sndFile.Instruments[m->instr];
						s += mpt::ToCString(sndFile.GetCharsetInternal(), pIns->name);
						if(m->IsNote())
						{
							const SAMPLEINDEX nsmp = pIns->Keyboard[m->note - 1];
							if((nsmp) && (nsmp <= sndFile.GetNumSamples()))
							{
								if(sndFile.m_szNames[nsmp][0])
								{
									s.AppendFormat(_T(" (%d: "), nsmp);
									s += mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.m_szNames[nsmp]);
									s.AppendChar(_T(')'));
								}
							}
						}
					}
				} else if(m->instr <= sndFile.GetNumSamples())
				{
					s += mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.m_szNames[m->instr]);
				}

			}
		}
		break;

	case PatternCursor::volumeColumn:
		// display volume command
		if(m->IsPcNote())
		{
			// display plugin param name.
			if(m->instr > 0 && m->instr <= MAX_MIXPLUGINS)
			{
				const SNDMIXPLUGIN &plug = sndFile.m_MixPlugins[m->instr - 1];
				if(plug.pMixPlugin != nullptr)
				{
					s = plug.pMixPlugin->GetFormattedParamName(m->GetValueVolCol());
				}
			}
		} else if(m->volcmd != VOLCMD_NONE)
		{
			// "normal" volume command
			EffectInfo effectInfo(sndFile);
			effectInfo.GetVolCmdInfo(effectInfo.GetIndexFromVolCmd(m->volcmd), &s);
			s += _T(": ");
			CString tmp;
			effectInfo.GetVolCmdParamInfo(*m, &tmp, TrackerSettings::Instance().patternVolColHex);
			s += tmp;
		}
		break;

	case PatternCursor::effectColumn:
	case PatternCursor::paramColumn:
		// display effect command
		if(m->IsPcNote())
		{
			s.Format(_T("Parameter value: %u"), m->GetValueEffectCol());
		} else if(m->command != CMD_NONE)
		{
			EffectInfo effectInfo(sndFile);
			CString sztmp;
			if(effectInfo.GetIndexFromEffect(m->command, m->param) >= 0)
			{
				UINT xParam = 0, xMultiplier = 1;
				getXParam(m->command, m_nPattern, row, channel, sndFile, xParam, xMultiplier);

				effectInfo.GetEffectNameEx(sztmp, *m, m->param * xMultiplier + xParam, channel);
			}
			//effectInfo.GetEffectName(sztmp, m->command, m->param, false, nChn);
			if(!sztmp.IsEmpty())
			{
				s.Format(_T("%c%02X: "), sndFile.GetModSpecifications().GetEffectLetter(m->command), m->param);
				s += sztmp;
			}
		}
		break;

	case PatternCursor::numColumns:
		MPT_ASSERT_NOTREACHED();
		break;
	}
	return s;
}


void CViewPattern::UpdateXInfoText()
{
	const CSoundFile *sndFile = GetSoundFile();
	CMainFrame *mainFrm = CMainFrame::GetMainFrame();
	if(mainFrm == nullptr || sndFile == nullptr)
		return;

	CHANNELINDEX chn = GetCurrentChannel();
	const auto &channel = sndFile->m_PlayState.Chn[chn];
	CString xtraInfo;

	xtraInfo.Format(_T("Chn:%d; Vol:%X; Mac:%X; Cut:%X%s; Res:%X; Pan:%X%s"),
	                chn + 1,
	                channel.nGlobalVol,
	                channel.nActiveMacro,
	                channel.nCutOff,
	                (channel.nFilterMode == FilterMode::HighPass) ? _T("-HP") : _T(""),
	                channel.nResonance,
	                channel.nPan,
	                channel.dwFlags[CHN_SURROUND] ? _T("-S") : _T(""));

	mainFrm->SetXInfoText(xtraInfo);
}


void CViewPattern::UpdateAllVUMeters(Notification *pnotify)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	const CModDoc *pModDoc = GetDocument();
	
	if(!pModDoc || !pMainFrm)
		return;
	CRect rcClient;
	GetClientRect(&rcClient);
	int xofs = GetXScrollPos();
	HDC hdc = ::GetDC(m_hWnd);
	const bool isPlaying = (pMainFrm->GetFollowSong(pModDoc) == m_hWnd);
	int x = m_szHeader.cx;
	CHANNELINDEX nChn = static_cast<CHANNELINDEX>(xofs);
	const CHANNELINDEX numChannels = std::min(pModDoc->GetNumChannels(), static_cast<CHANNELINDEX>(m_chnState.size()));
	const int yPos = rcClient.top + MulDiv(COLHDR_HEIGHT, m_dpi, 96);
	while(nChn < numChannels && x < rcClient.right)
	{
		m_chnState[nChn].vuMeter = static_cast<uint16>(pnotify->pos[nChn]);
		if(!isPlaying || pnotify->type[Notification::Stop])
			m_chnState[nChn].vuMeter = 0;
		DrawChannelVUMeter(hdc, x, rcClient.top + yPos, nChn);
		nChn++;
		x += m_szCell.cx;
	}
	::ReleaseDC(m_hWnd, hdc);
}


// This creates the VU meter LED bitmap, which looks somewhat like this:
// Row 0
// Row 1               [][]                             [][]              
// Row 2             [][][][]                         [][][][]            
// Row 3           [][][][][][]                     [][][][][][]          
// Row 4         [][][][][][][][]                 [][][][][][][][]        
// Row 5       [][][][][][][][][][]             [][][][][][][][][][]      
// Row 6     [][][][][][][][][][][][]         [][][][][][][][][][][][]    
// Row 7   [][][][][][][][][][][][][][]     [][][][][][][][][][][][][][]  
// Row 8 [][][][][][][][][][][][][][][][] [][][][][][][][][][][][][][][][]
// The left stack of LEDs uses the colors for sample-based instruments, the right stack is for synthesized / plugin instruments.
void CViewPattern::CreateVUMeterBitmap()
{
	if(!m_hWnd)
		return;
	
	const auto dc = GetDC();

	m_vuMeterBitmap.DeleteObject();
	m_vuMeterDC.DeleteDC();
	m_vuMeterDC.CreateCompatibleDC(dc);

	const int availableWidth = m_szCell.cx - HighDPISupport::ScalePixels(2, m_hWnd);
	m_ledWidth = static_cast<int>(availableWidth / (VUMETERS_LEDS_PER_SIDE * 2 + 0.5f));
	if(m_ledWidth < 2)
	{
		// The half-width spacer between left and right channel is not added if the LED width is below 2 pixels, so check if we can increase the width to 2 pixels
		m_ledWidth = std::max(availableWidth / (VUMETERS_LEDS_PER_SIDE * 2), 1);
	}

	m_ledHeight = HighDPISupport::ScalePixels(VUMETERS_HEIGHT_LED, m_hWnd);
	const int barWidth = m_ledWidth * VUMETERS_LEDS_PER_SIDE;
	const int ledBlockHeight = m_ledHeight - 2;
	
	const int bmpWidth = barWidth * 4;
	const int bmpHeight = m_ledHeight * (VUMETERS_LEDS_PER_SIDE + 1);

	m_vuMeterBitmap.CreateCompatibleBitmap(dc, bmpWidth, bmpHeight);
	const auto oldBitmap = m_vuMeterDC.SelectObject(m_vuMeterBitmap);
	m_vuMeterDC.FillSolidRect(0, 0, bmpWidth, bmpHeight, GetSysColor(COLOR_BTNFACE));

	const auto &Colors = TrackerSettings::Instance().rgbCustomColors;
	const COLORREF shadowColor = (m_ledWidth > 2) ? RGB(0, 0, 0) : GetSysColor(COLOR_BTNSHADOW);

	// 0: sample-based, 1: synthesized / plugin
	for(int colorScheme = 0; colorScheme < 2; colorScheme++)
	{
		const int x = colorScheme * barWidth * 2 + barWidth;
		for (int led = 0; led < VUMETERS_LEDS_PER_SIDE; led++)
		{
			COLORREF color;
			if(led < 4)
				color = Colors[colorScheme ? MODCOLOR_VUMETER_LO_VST : MODCOLOR_VUMETER_LO];
			else if(led < 6)
				color = Colors[colorScheme ? MODCOLOR_VUMETER_MED_VST : MODCOLOR_VUMETER_MED];
			else
				color = Colors[colorScheme ? MODCOLOR_VUMETER_HI_VST : MODCOLOR_VUMETER_HI];

			const int ledOffsetX = led * m_ledWidth;
			for(int y = (led + 1) * m_ledHeight; y < bmpHeight; y += m_ledHeight)
			{
				if(m_ledWidth > 1)
				{
					// With shadow
					m_vuMeterDC.FillSolidRect(x + ledOffsetX + 1, y + 2, m_ledWidth - 1, ledBlockHeight, shadowColor);
					m_vuMeterDC.FillSolidRect(x - m_ledWidth - ledOffsetX + 1, y + 2, m_ledWidth - 1, ledBlockHeight, shadowColor);
					m_vuMeterDC.FillSolidRect(x + ledOffsetX, y + 1, m_ledWidth - 1, ledBlockHeight, color);
					m_vuMeterDC.FillSolidRect(x - m_ledWidth - ledOffsetX, y + 1, m_ledWidth - 1, ledBlockHeight, color);
				} else
				{
					// Solid
					m_vuMeterDC.FillSolidRect(x + ledOffsetX, y + 1, m_ledWidth, ledBlockHeight, color);
					m_vuMeterDC.FillSolidRect(x - m_ledWidth - ledOffsetX, y + 1, m_ledWidth, ledBlockHeight, color);
				}
			}
		}
	}

	m_vuMeterDC.SelectObject(oldBitmap);
	ReleaseDC(dc);
}


OPENMPT_NAMESPACE_END
