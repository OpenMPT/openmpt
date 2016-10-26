/*
 * Ctrl_ins.cpp
 * ------------
 * Purpose: Instrument tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "Childfrm.h"
#include "ImageLists.h"
#include "Moddoc.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "Globals.h"
#include "Ctrl_ins.h"
#include "View_ins.h"
#include "dlg_misc.h"
#include "tuningDialog.h"
#include "../common/misc_util.h"
#include "../common/StringFixer.h"
#include "SelectPluginDialog.h"
#include "../common/mptFileIO.h"
#include "../common/FileReader.h"
#include "FileDialog.h"


OPENMPT_NAMESPACE_BEGIN


#pragma warning(disable:4244) //conversion from 'type1' to 'type2', possible loss of data

/////////////////////////////////////////////////////////////////////////
// CNoteMapWnd

BEGIN_MESSAGE_MAP(CNoteMapWnd, CStatic)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_NOTEMAP_TRANS_UP,		OnMapTransposeUp)
	ON_COMMAND(ID_NOTEMAP_TRANS_DOWN,	OnMapTransposeDown)
	ON_COMMAND(ID_NOTEMAP_COPY_NOTE,	OnMapCopyNote)
	ON_COMMAND(ID_NOTEMAP_COPY_SMP,		OnMapCopySample)
	ON_COMMAND(ID_NOTEMAP_RESET,		OnMapReset)
	ON_COMMAND(ID_NOTEMAP_REMOVE,		OnMapRemove)
	ON_COMMAND(ID_INSTRUMENT_SAMPLEMAP, OnEditSampleMap)
	ON_COMMAND(ID_INSTRUMENT_DUPLICATE, OnInstrumentDuplicate)
	ON_COMMAND_RANGE(ID_NOTEMAP_EDITSAMPLE, ID_NOTEMAP_EDITSAMPLE+MAX_SAMPLES, OnEditSample)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,		OnCustomKeyMsg)
END_MESSAGE_MAP()


BOOL CNoteMapWnd::PreTranslateMessage(MSG* pMsg)
//----------------------------------------------
{
	UINT wParam;
	if (!pMsg) return TRUE;
	wParam = pMsg->wParam;

	if (pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) ||
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler* ih = CMainFrame::GetInputHandler();

			//Translate message manually
			UINT nChar = pMsg->wParam;
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);
			InputTargetContext ctx = (InputTargetContext)(kCtxInsNoteMap);

			if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT) != kcNull)
				return true; // Mapped to a command, no need to pass message on.

			// a bit of a hack...
			ctx = (InputTargetContext)(kCtxCtrlInstruments);

			if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT) != kcNull)
				return true; // Mapped to a command, no need to pass message on.
		}
	}

	//The key was not handled by a command, but it might still be useful
	if (pMsg->message == WM_CHAR) //key is a character
	{
		UINT nFlags = HIWORD(pMsg->lParam);
		KeyEventType kT = CMainFrame::GetInputHandler()->GetKeyEventType(nFlags);

		if (kT == kKeyEventDown)
			if (HandleChar(wParam))
				return true;
	}
	else if (pMsg->message == WM_KEYDOWN) //key is not a character
	{
		if (HandleNav(wParam))
			return true;
	}
	else if (pMsg->message == WM_KEYUP) //stop notes on key release
	{
		if (((pMsg->wParam >= '0') && (pMsg->wParam <= '9')) || (pMsg->wParam == ' ') ||
			((pMsg->wParam >= VK_NUMPAD0) && (pMsg->wParam <= VK_NUMPAD9)))
		{
			StopNote(m_nPlayingNote);
			return true;
		}
	}

	return CStatic::PreTranslateMessage(pMsg);
}


BOOL CNoteMapWnd::SetCurrentInstrument(INSTRUMENTINDEX nIns)
//----------------------------------------------------------
{
	if (nIns != m_nInstrument)
	{
		if (nIns < MAX_INSTRUMENTS) m_nInstrument = nIns;

		// create missing instrument if needed
		CSoundFile &sndFile = m_modDoc.GetrSoundFile();
		if(m_nInstrument > 0 && m_nInstrument <= sndFile.GetNumInstruments() && sndFile.Instruments[m_nInstrument] == nullptr)
		{
			ModInstrument *instrument = sndFile.AllocateInstrument(m_nInstrument);
			if(instrument == nullptr)
			{
				return FALSE;
			}
			m_modDoc.InitializeInstrument(instrument);
		}

		InvalidateRect(NULL, FALSE);
	}
	return TRUE;
}


BOOL CNoteMapWnd::SetCurrentNote(UINT nNote)
//------------------------------------------
{
	if (nNote == m_nNote) return TRUE;
	if (!ModCommand::IsNote(nNote + 1)) return FALSE;
	m_nNote = nNote;
	InvalidateRect(NULL, FALSE);
	return TRUE;
}


void CNoteMapWnd::OnPaint()
//-------------------------
{
	HGDIOBJ oldfont = NULL;
	CRect rcClient;
	CPaintDC dc(this);
	HDC hdc;
	CSoundFile &sndFile = m_modDoc.GetrSoundFile();

	GetClientRect(&rcClient);
	if (!m_hFont)
	{
		m_hFont = CMainFrame::GetGUIFont();
		colorText = GetSysColor(COLOR_WINDOWTEXT);
		colorTextSel = GetSysColor(COLOR_HIGHLIGHTTEXT);
	}
	hdc = dc.m_hDC;
	oldfont = ::SelectObject(hdc, m_hFont);
	dc.SetBkMode(TRANSPARENT);
	if ((m_cxFont <= 0) || (m_cyFont <= 0))
	{
		CSize sz;
		sz = dc.GetTextExtent("C#0.", 4);
		m_cyFont = sz.cy + 2;
		m_cxFont = rcClient.right / 3;
	}
	dc.IntersectClipRect(&rcClient);
	if (m_cxFont > 0 && m_cyFont > 0)
	{
		bool bFocus = (::GetFocus() == m_hWnd);
		ModInstrument *pIns = sndFile.Instruments[m_nInstrument];
		CHAR s[64];
		CRect rect;

		int nNotes = (rcClient.bottom + m_cyFont - 1) / m_cyFont;
		int nPos = m_nNote - (nNotes/2);
		int ypaint = 0;
		for (int ynote=0; ynote<nNotes; ynote++, ypaint+=m_cyFont, nPos++)
		{
			bool bHighLight;

			// Note
			s[0] = 0;

			std::string temp = sndFile.GetNoteName(static_cast<ModCommand::NOTE>(nPos + 1), m_nInstrument);
			temp.resize(4);
			if ((nPos >= 0) && (nPos < NOTE_MAX)) wsprintf(s, "%s", temp.c_str());
			rect.SetRect(0, ypaint, m_cxFont, ypaint+m_cyFont);
			DrawButtonRect(hdc, &rect, s, FALSE, FALSE);
			// Mapped Note
			bHighLight = ((bFocus) && (nPos == (int)m_nNote) /*&& (!m_bIns)*/);
			rect.left = rect.right;
			rect.right = m_cxFont*2-1;
			strcpy(s, "...");
			if ((pIns) && (nPos >= 0) && (nPos < NOTE_MAX) && (pIns->NoteMap[nPos] != NOTE_NONE))
			{
				ModCommand::NOTE n = pIns->NoteMap[nPos];
				if(ModCommand::IsNote(n))
				{
					std::string temp = sndFile.GetNoteName(n, m_nInstrument);
					temp.resize(4);
					wsprintf(s, "%s", temp.c_str());
				} else
				{
					strcpy(s, "???");
				}
			}
			FillRect(hdc, &rect, (bHighLight) ? CMainFrame::brushHighLight : CMainFrame::brushWindow);
			if ((nPos == (int)m_nNote) && (!m_bIns))
			{
				rect.InflateRect(-1, -1);
				dc.DrawFocusRect(&rect);
				rect.InflateRect(1, 1);
			}
			dc.SetTextColor((bHighLight) ? colorTextSel : colorText);
			dc.DrawText(s, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			// Sample
			bHighLight = ((bFocus) && (nPos == (int)m_nNote) /*&& (m_bIns)*/);
			rect.left = rcClient.left + m_cxFont*2+3;
			rect.right = rcClient.right;
			strcpy(s, " ..");
			if ((pIns) && (nPos >= 0) && (nPos < NOTE_MAX) && (pIns->Keyboard[nPos]))
			{
				wsprintf(s, "%3d", pIns->Keyboard[nPos]);
			}
			FillRect(hdc, &rect, (bHighLight) ? CMainFrame::brushHighLight : CMainFrame::brushWindow);
			if ((nPos == (int)m_nNote) && (m_bIns))
			{
				rect.InflateRect(-1, -1);
				dc.DrawFocusRect(&rect);
				rect.InflateRect(1, 1);
			}
			dc.SetTextColor((bHighLight) ? colorTextSel : colorText);
			dc.DrawText(s, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		}
		rect.SetRect(rcClient.left+m_cxFont*2-1, rcClient.top, rcClient.left+m_cxFont*2+3, ypaint);
		DrawButtonRect(hdc, &rect, "", FALSE, FALSE);
		if (ypaint < rcClient.bottom)
		{
			rect.SetRect(rcClient.left, ypaint, rcClient.right, rcClient.bottom);
			FillRect(hdc, &rect, CMainFrame::brushGray);
		}
	}
	if (oldfont) ::SelectObject(hdc, oldfont);
}


void CNoteMapWnd::OnSetFocus(CWnd *pOldWnd)
//-----------------------------------------
{
	CWnd::OnSetFocus(pOldWnd);
	InvalidateRect(NULL, FALSE);
	CMainFrame::GetMainFrame()->m_pNoteMapHasFocus = (CWnd*)this;
}


void CNoteMapWnd::OnKillFocus(CWnd *pNewWnd)
//------------------------------------------
{
	CWnd::OnKillFocus(pNewWnd);
	InvalidateRect(NULL, FALSE);
	CMainFrame::GetMainFrame()->m_pNoteMapHasFocus = nullptr;
}


void CNoteMapWnd::OnLButtonDown(UINT, CPoint pt)
//----------------------------------------------
{
	if ((pt.x >= m_cxFont) && (pt.x < m_cxFont*2) && (m_bIns))
	{
		m_bIns = false;
		InvalidateRect(NULL, FALSE);
	}
	if ((pt.x > m_cxFont*2) && (pt.x <= m_cxFont*3) && (!m_bIns))
	{
		m_bIns = true;
		InvalidateRect(NULL, FALSE);
	}
	if ((pt.x >= 0) && (m_cyFont))
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		int nNotes = (rcClient.bottom + m_cyFont - 1) / m_cyFont;
		int n = (pt.y / m_cyFont) + m_nNote - (nNotes/2);
		if(n >= 0)
		{
			SetCurrentNote(n);
		}
	}
	SetFocus();
}


void CNoteMapWnd::OnLButtonDblClk(UINT, CPoint)
//---------------------------------------------
{
	// Double-click edits sample map
	OnEditSampleMap();
}


void CNoteMapWnd::OnRButtonDown(UINT, CPoint pt)
//----------------------------------------------
{
	CHAR s[64];
	CInputHandler* ih = CMainFrame::GetInputHandler();

	CSoundFile &sndFile = m_modDoc.GetrSoundFile();
	ModInstrument *pIns = sndFile.Instruments[m_nInstrument];
	if (pIns)
	{
		HMENU hMenu = ::CreatePopupMenu();
		HMENU hSubMenu = ::CreatePopupMenu();

		if (hMenu)
		{
			AppendMenu(hMenu, MF_STRING, ID_INSTRUMENT_SAMPLEMAP, _T("Edit Sample &Map\t") + ih->GetKeyTextFromCommand(kcInsNoteMapEditSampleMap));
			if (hSubMenu)
			{
				// Create sub menu with a list of all samples that are referenced by this instrument.
				const std::set<SAMPLEINDEX> referencedSamples = pIns->GetSamples();

				for(auto sample = referencedSamples.cbegin(); sample != referencedSamples.cend(); sample++)
				{
					if(*sample <= sndFile.GetNumSamples())
					{
						wsprintf(s, _T("%u: "), *sample);
						size_t l = strlen(s);
						memcpy(s + l, sndFile.m_szNames[*sample], MAX_SAMPLENAME);
						s[l + MAX_SAMPLENAME] = '\0';
						AppendMenu(hSubMenu, MF_STRING, ID_NOTEMAP_EDITSAMPLE + *sample, s);
					}
				}

				AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), _T("&Edit Sample\t") + ih->GetKeyTextFromCommand(kcInsNoteMapEditSample));
				AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
			}
			wsprintf(s, _T("Map all notes to &sample %u\t") + ih->GetKeyTextFromCommand(kcInsNoteMapCopyCurrentSample), pIns->Keyboard[m_nNote]);
			AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_COPY_SMP, s);

			if(sndFile.GetType() != MOD_TYPE_XM)
			{
				if(ModCommand::IsNote(pIns->NoteMap[m_nNote]))
				{
					wsprintf(s, _T("Map all &notes to %s\t") + ih->GetKeyTextFromCommand(kcInsNoteMapCopyCurrentNote), sndFile.GetNoteName(pIns->NoteMap[m_nNote], m_nInstrument).c_str());
					AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_COPY_NOTE, s);
				}
				AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_TRANS_UP, _T("Transpose map &up\t") + ih->GetKeyTextFromCommand(kcInsNoteMapTransposeUp));
				AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_TRANS_DOWN, _T("Transpose map &down\t") + ih->GetKeyTextFromCommand(kcInsNoteMapTransposeDown));
			}
			AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_RESET, _T("&Reset note mapping\t") + ih->GetKeyTextFromCommand(kcInsNoteMapReset));
			AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_REMOVE, _T("Remo&ve all samples\t") + ih->GetKeyTextFromCommand(kcInsNoteMapRemove));
			AppendMenu(hMenu, MF_STRING, ID_INSTRUMENT_DUPLICATE, _T("Duplicate &Instrument\t") + ih->GetKeyTextFromCommand(kcInstrumentCtrlDuplicate));
			SetMenuDefaultItem(hMenu, ID_INSTRUMENT_SAMPLEMAP, FALSE);
			ClientToScreen(&pt);
			::TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
			::DestroyMenu(hMenu);
			if (hSubMenu) ::DestroyMenu(hSubMenu);
		}
	}
}


BOOL CNoteMapWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
//------------------------------------------------------------------
{
	SetCurrentNote(m_nNote - sgn(zDelta));
	return CStatic::OnMouseWheel(nFlags, zDelta, pt);
}


void CNoteMapWnd::OnMapCopyNote()
//-------------------------------
{
	ModInstrument *pIns = m_modDoc.GetrSoundFile().Instruments[m_nInstrument];
	if (pIns)
	{
		bool bModified = false;
		uint8 n = pIns->NoteMap[m_nNote];
		for (size_t i = 0; i < CountOf(pIns->NoteMap); i++) if (pIns->NoteMap[i] != n)
		{
			pIns->NoteMap[i] = n;
			bModified = true;
		}
		if (bModified)
		{
			m_pParent.SetModified(InstrumentHint().Info(), false);
			InvalidateRect(NULL, FALSE);
		}
	}
}

void CNoteMapWnd::OnMapCopySample()
//---------------------------------
{
	ModInstrument *pIns = m_modDoc.GetrSoundFile().Instruments[m_nInstrument];
	if (pIns)
	{
		bool bModified = false;
		SAMPLEINDEX n = pIns->Keyboard[m_nNote];
		for (NOTEINDEXTYPE i = 0; i < CountOf(pIns->Keyboard); i++) if (pIns->Keyboard[i] != n)
		{
			pIns->Keyboard[i] = n;
			bModified = true;
		}
		if (bModified)
		{
			m_pParent.SetModified(InstrumentHint().Info(), false);
			InvalidateRect(NULL, FALSE);
		}
	}
}


void CNoteMapWnd::OnMapReset()
//----------------------------
{
	ModInstrument *pIns = m_modDoc.GetrSoundFile().Instruments[m_nInstrument];
	if (pIns)
	{
		bool bModified = false;
		for (size_t i = 0; i < CountOf(pIns->NoteMap); i++) if (pIns->NoteMap[i] != i + 1)
		{
			pIns->NoteMap[i] = static_cast<ModCommand::NOTE>(i + 1);
			bModified = true;
		}
		if (bModified)
		{
			m_pParent.SetModified(InstrumentHint().Info(), false);
			InvalidateRect(NULL, FALSE);
		}
	}
}


void CNoteMapWnd::OnMapRemove()
//-----------------------------
{
	ModInstrument *pIns = m_modDoc.GetrSoundFile().Instruments[m_nInstrument];
	if (pIns)
	{
		bool bModified = false;
		for (size_t i = 0; i < CountOf(pIns->Keyboard); i++) if (pIns->Keyboard[i] != 0)
		{
			pIns->Keyboard[i] = 0;
			bModified = true;
		}
		if (bModified)
		{
			m_pParent.SetModified(InstrumentHint().Info(), false);
			InvalidateRect(NULL, FALSE);
		}
	}
}


void CNoteMapWnd::OnMapTransposeUp()
//----------------------------------
{
	MapTranspose(1);
}


void CNoteMapWnd::OnMapTransposeDown()
//------------------------------------
{
	MapTranspose(-1);
}


void CNoteMapWnd::MapTranspose(int nAmount)
//-----------------------------------------
{
	if(nAmount == 0 || m_modDoc.GetModType() == MOD_TYPE_XM) return;

	ModInstrument *pIns = m_modDoc.GetrSoundFile().Instruments[m_nInstrument];
	if((nAmount == 12 || nAmount == -12) && pIns->pTuning != nullptr)
	{
		// Special case for instrument-specific tunings
		nAmount = pIns->pTuning->GetGroupSize() * sgn(nAmount);
	}

	if (pIns)
	{
		bool bModified = false;
		for(NOTEINDEXTYPE i = 0; i < NOTE_MAX; i++)
		{
			int n = pIns->NoteMap[i];
			if ((n > NOTE_MIN && nAmount < 0) || (n < NOTE_MAX && nAmount > 0))
			{
				n = Clamp(n + nAmount, NOTE_MIN, NOTE_MAX);
				pIns->NoteMap[i] = (BYTE)n;
				bModified = true;
			}
		}
		if (bModified)
		{
			m_pParent.SetModified(InstrumentHint().Info(), false);
			InvalidateRect(NULL, FALSE);
		}
	}
}


void CNoteMapWnd::OnEditSample(UINT nID)
//--------------------------------------
{
	UINT nSample = nID - ID_NOTEMAP_EDITSAMPLE;
	m_pParent.EditSample(nSample);
}


void CNoteMapWnd::OnEditSampleMap()
//---------------------------------
{
	m_pParent.PostMessage(WM_COMMAND, ID_INSTRUMENT_SAMPLEMAP);
}


void CNoteMapWnd::OnInstrumentDuplicate()
//---------------------------------------
{
	m_pParent.PostMessage(WM_COMMAND, ID_INSTRUMENT_DUPLICATE);
}


LRESULT CNoteMapWnd::OnCustomKeyMsg(WPARAM wParam, LPARAM lParam)
//---------------------------------------------------------------
{
	if (wParam == kcNull)
		return NULL;

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	ModInstrument *pIns = m_modDoc.GetrSoundFile().Instruments[m_nInstrument];

	// Handle notes

	if (wParam>=kcInsNoteMapStartNotes && wParam<=kcInsNoteMapEndNotes)
	{
		//Special case: number keys override notes if we're in the sample # column.
		if ((m_bIns) && (((lParam >= '0') && (lParam <= '9')) || (lParam == ' ')))
			HandleChar(lParam);
		else
			EnterNote(wParam-kcInsNoteMapStartNotes+1+pMainFrm->GetBaseOctave()*12);

		return wParam;
	}

	if (wParam>=kcInsNoteMapStartNoteStops && wParam<=kcInsNoteMapEndNoteStops)
	{
		StopNote(m_nPlayingNote);
		return wParam;
	}

	// Other shortcuts

	switch(wParam)
	{
	case kcInsNoteMapTransposeDown:		MapTranspose(-1); return wParam;
	case kcInsNoteMapTransposeUp:		MapTranspose(1); return wParam;
	case kcInsNoteMapTransposeOctDown:	MapTranspose(-12); return wParam;
	case kcInsNoteMapTransposeOctUp:	MapTranspose(12); return wParam;

	case kcInsNoteMapCopyCurrentSample:	OnMapCopySample(); return wParam;
	case kcInsNoteMapCopyCurrentNote:	OnMapCopyNote(); return wParam;
	case kcInsNoteMapReset:				OnMapReset(); return wParam;
	case kcInsNoteMapRemove:			OnMapRemove(); return wParam;

	case kcInsNoteMapEditSample:		if(pIns) OnEditSample(pIns->Keyboard[m_nNote] + ID_NOTEMAP_EDITSAMPLE); return wParam;
	case kcInsNoteMapEditSampleMap:		OnEditSampleMap(); return wParam;

	// Parent shortcuts (also displayed in context menu of this control)
	case kcInstrumentCtrlDuplicate:		OnInstrumentDuplicate(); return wParam;
	case kcNextInstrument:				m_pParent.PostMessage(WM_COMMAND, ID_NEXTINSTRUMENT); return wParam;
	case kcPrevInstrument:				m_pParent.PostMessage(WM_COMMAND, ID_PREVINSTRUMENT); return wParam;
	}

	return NULL;
}

void CNoteMapWnd::EnterNote(UINT note)
//------------------------------------
{
	CSoundFile &sndFile = m_modDoc.GetrSoundFile();
	ModInstrument *pIns = sndFile.Instruments[m_nInstrument];
	if ((pIns) && (m_nNote < NOTE_MAX))
	{
		if (!m_bIns && (sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)))
		{
			UINT n = pIns->NoteMap[m_nNote];
			bool bOk = false;
			if ((note >= sndFile.GetModSpecifications().noteMin) && (note <= sndFile.GetModSpecifications().noteMax))
			{
				n = note;
				bOk = true;
			}
			if (n != pIns->NoteMap[m_nNote])
			{
				pIns->NoteMap[m_nNote] = static_cast<ModCommand::NOTE>(n);
				m_pParent.SetModified(InstrumentHint().Info(), false);
				InvalidateRect(NULL, FALSE);
			}
			if (bOk)
			{
				PlayNote(m_nNote);
				//SetCurrentNote(m_nNote+1);
			}

		}
	}
}

bool CNoteMapWnd::HandleChar(WPARAM c)
//------------------------------------
{
	CSoundFile &sndFile = m_modDoc.GetrSoundFile();
	ModInstrument *pIns = sndFile.Instruments[m_nInstrument];
	if ((pIns) && (m_nNote < NOTE_MAX))
	{

		if ((m_bIns) && (((c >= '0') && (c <= '9')) || (c == ' ')))	//in sample # column
		{
			UINT n = m_nOldIns;
			if (c != ' ')
			{
				n = (10 * pIns->Keyboard[m_nNote] + (c - '0')) % 10000;
				if ((n >= MAX_SAMPLES) || ((sndFile.m_nSamples < 1000) && (n >= 1000))) n = (n % 1000);
				if ((n >= MAX_SAMPLES) || ((sndFile.m_nSamples < 100) && (n >= 100))) n = (n % 100); else
				if ((n > 31) && (sndFile.m_nSamples < 32) && (n % 10)) n = (n % 10);
			}

			if (n != pIns->Keyboard[m_nNote])
			{
				pIns->Keyboard[m_nNote] = static_cast<SAMPLEINDEX>(n);
				m_pParent.SetModified(InstrumentHint().Info(), false);
				InvalidateRect(NULL, FALSE);
				PlayNote(m_nNote);
			}

			if (c == ' ')
			{
				if (m_nNote < NOTE_MAX - 1) m_nNote++;
				InvalidateRect(NULL, FALSE);
				PlayNote(m_nNote);
			}
			return true;
		}

		else if ((!m_bIns) && (sndFile.m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)))	//in note column
		{
			UINT n = pIns->NoteMap[m_nNote];

			if ((c >= '0') && (c <= '9'))
			{
				if (n)
					n = ((n-1) % 12) + (c-'0')*12 + 1;
				else
					n = (m_nNote % 12) + (c-'0')*12 + 1;
			} else if (c == ' ')
			{
				n = (m_nOldNote) ? m_nOldNote : m_nNote+1;
			}

			if (n != pIns->NoteMap[m_nNote])
			{
				pIns->NoteMap[m_nNote] = static_cast<ModCommand::NOTE>(n);
				m_pParent.SetModified(InstrumentHint().Info(), false);
				InvalidateRect(NULL, FALSE);
			}

			if (c == ' ')
			{
				SetCurrentNote(m_nNote+1);
			}

			PlayNote(m_nNote);

			return true;
		}
	}
	return false;
}

bool CNoteMapWnd::HandleNav(WPARAM k)
//------------------------------------
{
	bool bRedraw = false;

	//HACK: handle numpad (convert numpad number key to normal number key)
	if ((k >= VK_NUMPAD0) && (k <= VK_NUMPAD9)) return HandleChar(k-VK_NUMPAD0+'0');

	switch(k)
	{
	case VK_RIGHT:
		if (!m_bIns) { m_bIns = true; bRedraw = true; } else
		if (m_nNote < NOTE_MAX - 1) { m_nNote++; m_bIns = false; bRedraw = true; }
		break;
	case VK_LEFT:
		if (m_bIns) { m_bIns = false; bRedraw = true; } else
		if (m_nNote) { m_nNote--; m_bIns = true; bRedraw = true; }
		break;
	case VK_UP:
		if (m_nNote > 0) { m_nNote--; bRedraw = true; }
		break;
	case VK_DOWN:
		if (m_nNote < NOTE_MAX - 1) { m_nNote++; bRedraw = true; }
		break;
	case VK_PRIOR:
		if (m_nNote > 3) { m_nNote -= 3; bRedraw = true; } else
		if (m_nNote > 0) { m_nNote = 0; bRedraw = true; }
		break;
	case VK_NEXT:
		if (m_nNote+3 < NOTE_MAX) { m_nNote += 3; bRedraw = true; } else
		if (m_nNote < NOTE_MAX - 1) { m_nNote = NOTE_MAX - 1; bRedraw = true; }
		break;
// 	case VK_TAB:
// 		return true;
	case VK_RETURN:
		{
			ModInstrument *pIns = m_modDoc.GetrSoundFile().Instruments[m_nInstrument];
			if(pIns)
			{
				if (m_bIns)
					m_nOldIns = pIns->Keyboard[m_nNote];
				else
					m_nOldNote = pIns->NoteMap[m_nNote];
			}
		}

		return true;
	default:
		return false;
	}
	if (bRedraw)
	{
		InvalidateRect(NULL, FALSE);
	}

	return true;
}


void CNoteMapWnd::PlayNote(int note)
//----------------------------------
{
	if(m_nPlayingNote >= 0) return; //no polyphony in notemap window
	m_modDoc.PlayNote(note + NOTE_MIN, m_nInstrument, 0, false);
	m_nPlayingNote = note;
}


void CNoteMapWnd::StopNote(int note = -1)
//----------------------------------
{
	if(note < 0) note = m_nPlayingNote;
	if(note < 0) return;

	m_modDoc.NoteOff(note, true, m_nInstrument);
	m_nPlayingNote = -1;
}


/////////////////////////////////////////////////////////////////////////
// CCtrlInstruments

#define MAX_ATTACK_LENGTH	2001
#define MAX_ATTACK_VALUE	(MAX_ATTACK_LENGTH - 1)  // 16 bit unsigned max

BEGIN_MESSAGE_MAP(CCtrlInstruments, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlInstruments)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_XBUTTONUP()
	ON_NOTIFY(TBN_DROPDOWN, IDC_TOOLBAR1, OnTbnDropDownToolBar)
	ON_COMMAND(IDC_INSTRUMENT_NEW,		OnInstrumentNew)
	ON_COMMAND(IDC_INSTRUMENT_OPEN,		OnInstrumentOpen)
	ON_COMMAND(IDC_INSTRUMENT_SAVEAS,	OnInstrumentSave)
	ON_COMMAND(IDC_INSTRUMENT_PLAY,		OnInstrumentPlay)
	ON_COMMAND(ID_PREVINSTRUMENT,		OnPrevInstrument)
	ON_COMMAND(ID_NEXTINSTRUMENT,		OnNextInstrument)
	ON_COMMAND(ID_INSTRUMENT_DUPLICATE, OnInstrumentDuplicate)
	ON_COMMAND(IDC_CHECK1,				OnSetPanningChanged)
	ON_COMMAND(IDC_CHECK2,				OnEnableCutOff)
	ON_COMMAND(IDC_CHECK3,				OnEnableResonance)
	ON_COMMAND(IDC_INSVIEWPLG,			TogglePluginEditor)
	ON_EN_CHANGE(IDC_EDIT_INSTRUMENT,	OnInstrumentChanged)
	ON_EN_CHANGE(IDC_SAMPLE_NAME,		OnNameChanged)
	ON_EN_CHANGE(IDC_SAMPLE_FILENAME,	OnFileNameChanged)
	ON_EN_CHANGE(IDC_EDIT7,				OnFadeOutVolChanged)
	ON_EN_CHANGE(IDC_EDIT8,				OnGlobalVolChanged)
	ON_EN_CHANGE(IDC_EDIT9,				OnPanningChanged)
	ON_EN_CHANGE(IDC_EDIT10,			OnMPRChanged)
	ON_EN_KILLFOCUS(IDC_EDIT10,			OnMPRKillFocus)
	ON_EN_CHANGE(IDC_EDIT11,			OnMBKChanged)
	ON_EN_CHANGE(IDC_EDIT15,			OnPPSChanged)
	ON_EN_CHANGE(IDC_PITCHWHEELDEPTH,	OnPitchWheelDepthChanged)
	ON_EN_CHANGE(IDC_EDIT2,				OnAttackChanged)

	ON_CBN_SELCHANGE(IDC_COMBO1,		OnNNAChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,		OnDCTChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,		OnDCAChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,		OnPPCChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5,		OnMCHChanged)
	ON_CBN_SELCHANGE(IDC_COMBO6,		OnMixPlugChanged)
	ON_CBN_DROPDOWN(IDC_COMBO6,			OnOpenPluginList)
	ON_CBN_SELCHANGE(IDC_COMBO9,		OnResamplingChanged)
	ON_CBN_SELCHANGE(IDC_FILTERMODE,	OnFilterModeChanged)
	ON_CBN_SELCHANGE(IDC_PLUGIN_VOLUMESTYLE,	OnPluginVolumeHandlingChanged)
	ON_COMMAND(IDC_PLUGIN_VELOCITYSTYLE,		OnPluginVelocityHandlingChanged)
	ON_COMMAND(ID_INSTRUMENT_SAMPLEMAP,			OnEditSampleMap)
	ON_CBN_SELCHANGE(IDC_COMBOTUNING, OnCbnSelchangeCombotuning)
	ON_EN_CHANGE(IDC_EDIT_PITCHTEMPOLOCK, OnEnChangeEditPitchtempolock)
	ON_BN_CLICKED(IDC_CHECK_PITCHTEMPOLOCK, OnBnClickedCheckPitchtempolock)
	ON_EN_KILLFOCUS(IDC_EDIT_PITCHTEMPOLOCK, OnEnKillfocusEditPitchtempolock)
	ON_EN_KILLFOCUS(IDC_EDIT7, OnEnKillfocusEditFadeOut)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CCtrlInstruments::DoDataExchange(CDataExchange* pDX)
//-------------------------------------------------------
{
	CModControlDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCtrlInstruments)
	DDX_Control(pDX, IDC_TOOLBAR1,				m_ToolBar);
	DDX_Control(pDX, IDC_NOTEMAP,				m_NoteMap);
	DDX_Control(pDX, IDC_SAMPLE_NAME,			m_EditName);
	DDX_Control(pDX, IDC_SAMPLE_FILENAME,		m_EditFileName);
	DDX_Control(pDX, IDC_SPIN_INSTRUMENT,		m_SpinInstrument);
	DDX_Control(pDX, IDC_COMBO1,				m_ComboNNA);
	DDX_Control(pDX, IDC_COMBO2,				m_ComboDCT);
	DDX_Control(pDX, IDC_COMBO3,				m_ComboDCA);
	DDX_Control(pDX, IDC_COMBO4,				m_ComboPPC);
	DDX_Control(pDX, IDC_COMBO5,				m_CbnMidiCh);
	DDX_Control(pDX, IDC_COMBO6,				m_CbnMixPlug);
	DDX_Control(pDX, IDC_COMBO9,				m_CbnResampling);
	DDX_Control(pDX, IDC_FILTERMODE,			m_CbnFilterMode);
	DDX_Control(pDX, IDC_EDIT7,					m_EditFadeOut);
	DDX_Control(pDX, IDC_SPIN7,					m_SpinFadeOut);
	DDX_Control(pDX, IDC_SPIN8,					m_SpinGlobalVol);
	DDX_Control(pDX, IDC_SPIN9,					m_SpinPanning);
	DDX_Control(pDX, IDC_SPIN10,				m_SpinMidiPR);
	DDX_Control(pDX, IDC_SPIN11,				m_SpinMidiBK);
	DDX_Control(pDX, IDC_SPIN12,				m_SpinPPS);
	DDX_Control(pDX, IDC_EDIT8,					m_EditGlobalVol);
	DDX_Control(pDX, IDC_EDIT9,					m_EditPanning);
	DDX_Control(pDX, IDC_CHECK1,				m_CheckPanning);
	DDX_Control(pDX, IDC_CHECK2,				m_CheckCutOff);
	DDX_Control(pDX, IDC_CHECK3,				m_CheckResonance);
	DDX_Control(pDX, IDC_SLIDER1,				m_SliderVolSwing);
	DDX_Control(pDX, IDC_SLIDER2,				m_SliderPanSwing);
	DDX_Control(pDX, IDC_SLIDER3,				m_SliderCutOff);
	DDX_Control(pDX, IDC_SLIDER4,				m_SliderResonance);
	DDX_Control(pDX, IDC_SLIDER6,				m_SliderCutSwing);
	DDX_Control(pDX, IDC_SLIDER7,				m_SliderResSwing);
	DDX_Control(pDX, IDC_SLIDER5,				m_SliderAttack);
	DDX_Control(pDX, IDC_SPIN1,					m_SpinAttack);
	DDX_Control(pDX, IDC_COMBOTUNING,			m_ComboTuning);
	DDX_Control(pDX, IDC_CHECK_PITCHTEMPOLOCK,	m_CheckPitchTempoLock);
	DDX_Control(pDX, IDC_PLUGIN_VOLUMESTYLE,	m_CbnPluginVolumeHandling);
	DDX_Control(pDX, IDC_PLUGIN_VELOCITYSTYLE,	velocityStyle);
	DDX_Control(pDX, IDC_SPIN2,					m_SpinPWD);
	//}}AFX_DATA_MAP
}


CCtrlInstruments::CCtrlInstruments(CModControlView &parent, CModDoc &document) : CModControlDlg(parent, document), m_NoteMap(*this, document)
//-------------------------------------------------------------------------------------------------------------------------------------------
{
	m_nInstrument = 1;
	m_nLockCount = 1;
	openendPluginListWithMouse = false;
}


CCtrlInstruments::~CCtrlInstruments()
//-----------------------------------
{
}


CRuntimeClass *CCtrlInstruments::GetAssociatedViewClass()
//-------------------------------------------------------
{
	return RUNTIME_CLASS(CViewInstrument);
}


BOOL CCtrlInstruments::OnInitDialog()
//-----------------------------------
{
	CModControlDlg::OnInitDialog();
	m_bInitialized = FALSE;
	SetRedraw(FALSE);

	m_ToolBar.SetExtendedStyle(m_ToolBar.GetExtendedStyle() | TBSTYLE_EX_DRAWDDARROWS);
	m_ToolBar.Init(CMainFrame::GetMainFrame()->m_PatternIcons,CMainFrame::GetMainFrame()->m_PatternIconsDisabled);
	m_ToolBar.AddButton(IDC_INSTRUMENT_NEW, TIMAGE_INSTR_NEW, TBSTYLE_BUTTON | TBSTYLE_DROPDOWN);
	m_ToolBar.AddButton(IDC_INSTRUMENT_OPEN, TIMAGE_OPEN);
	m_ToolBar.AddButton(IDC_INSTRUMENT_SAVEAS, TIMAGE_SAVE);
	m_ToolBar.AddButton(IDC_INSTRUMENT_PLAY, TIMAGE_PREVIEW);
	m_SpinInstrument.SetRange(0, 0);
	m_SpinInstrument.EnableWindow(FALSE);
	// NNA
	m_ComboNNA.AddString("Note Cut");
	m_ComboNNA.AddString("Continue");
	m_ComboNNA.AddString("Note Off");
	m_ComboNNA.AddString("Note Fade");
	// DCT
	m_ComboDCT.AddString("Disabled");
	m_ComboDCT.AddString("Note");
	m_ComboDCT.AddString("Sample");
	m_ComboDCT.AddString("Instrument");
	m_ComboDCT.AddString("Plugin");
	// DCA
	m_ComboDCA.AddString("Note Cut");
	m_ComboDCA.AddString("Note Off");
	m_ComboDCA.AddString("Note Fade");
	// FadeOut Volume
	m_SpinFadeOut.SetRange(0, 8192);
	// Global Volume
	m_SpinGlobalVol.SetRange(0, 64);
	// Panning
	m_SpinPanning.SetRange(0, (m_modDoc.GetModType() & MOD_TYPE_IT) ? 64 : 256);
	// Midi Program
	m_SpinMidiPR.SetRange(0, 128);
	// Midi Bank
	m_SpinMidiBK.SetRange(0, 16384);
	// MIDI Pitch Wheel Depth
	m_EditPWD.SubclassDlgItem(IDC_PITCHWHEELDEPTH, this);
	m_EditPWD.AllowFractions(false);

	m_CbnResampling.SetItemData(m_CbnResampling.AddString("Default"), SRCMODE_DEFAULT);
	m_CbnResampling.SetItemData(m_CbnResampling.AddString("None"), SRCMODE_NEAREST);
	m_CbnResampling.SetItemData(m_CbnResampling.AddString("Linear"), SRCMODE_LINEAR);
	m_CbnResampling.SetItemData(m_CbnResampling.AddString("Spline"), SRCMODE_SPLINE);
	m_CbnResampling.SetItemData(m_CbnResampling.AddString("Polyphase"), SRCMODE_POLYPHASE);
	m_CbnResampling.SetItemData(m_CbnResampling.AddString("XMMS"), SRCMODE_FIRFILTER);

	m_CbnFilterMode.SetItemData(m_CbnFilterMode.AddString("Channel default"), FLTMODE_UNCHANGED);
	m_CbnFilterMode.SetItemData(m_CbnFilterMode.AddString("Force lowpass"), FLTMODE_LOWPASS);
	m_CbnFilterMode.SetItemData(m_CbnFilterMode.AddString("Force highpass"), FLTMODE_HIGHPASS);

	//VST velocity/volume handling
	m_CbnPluginVolumeHandling.AddString("MIDI volume");
	m_CbnPluginVolumeHandling.AddString("Dry/Wet ratio");
	m_CbnPluginVolumeHandling.AddString("None");

	// Vol/Pan Swing
	m_SliderVolSwing.SetRange(0, 100);
	m_SliderPanSwing.SetRange(0, 64);
	m_SliderCutSwing.SetRange(0, 64);
	m_SliderResSwing.SetRange(0, 64);
	// Filter
	m_SliderCutOff.SetRange(0x00, 0x7F);
	m_SliderResonance.SetRange(0x00, 0x7F);
	// Pitch/Pan Separation
	m_EditPPS.SubclassDlgItem(IDC_EDIT15, this);
	m_EditPPS.AllowFractions(false);
	m_SpinPPS.SetRange(-32, +32);
	// Pitch/Pan Center
	SetWindowLongPtr(m_ComboPPC.m_hWnd, GWLP_USERDATA, 0);

	// Volume ramping (attack)
	m_SliderAttack.SetRange(0,MAX_ATTACK_VALUE);
	m_SpinAttack.SetRange(0,MAX_ATTACK_VALUE);

	m_SpinInstrument.SetFocus();

	m_EditPWD.EnableWindow(FALSE);

	BuildTuningComboBox();

	CheckDlgButton(IDC_CHECK_PITCHTEMPOLOCK, BST_UNCHECKED);
	m_EditPitchTempoLock.SubclassDlgItem(IDC_EDIT_PITCHTEMPOLOCK, this);
	m_EditPitchTempoLock.AllowNegative(false);
	m_EditPitchTempoLock.SetLimitText(9);

	SetRedraw(TRUE);
	return FALSE;
}


void CCtrlInstruments::RecalcLayout()
//-----------------------------------
{
}


void CCtrlInstruments::OnTbnDropDownToolBar(NMHDR* pNMHDR, LRESULT* pResult)
//--------------------------------------------------------------------------
{
	CInputHandler *ih = CMainFrame::GetInputHandler();
	LPNMTOOLBAR pToolBar = reinterpret_cast<LPNMTOOLBAR>(pNMHDR);
	ClientToScreen(&(pToolBar->rcButton)); // TrackPopupMenu uses screen coords
	const int offset = Util::ScalePixels(4, m_hWnd);	// Compared to the main toolbar, the offset seems to be a bit wrong here...?
	switch(pToolBar->iItem)
	{
	case IDC_INSTRUMENT_NEW:
		{
			CMenu menu;
			menu.CreatePopupMenu();
			menu.AppendMenu(MF_STRING, ID_INSTRUMENT_DUPLICATE, _T("Duplicate &Instrument\t") + ih->GetKeyTextFromCommand(kcInstrumentCtrlDuplicate));
			menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pToolBar->rcButton.left + offset, pToolBar->rcButton.bottom + offset, this);
			menu.DestroyMenu();
		}
		break;
	}
	*pResult = 0;
}


// Set document as modified and update other views.
// updateAll: Update all views including this one. Otherwise, only update update other views.
void CCtrlInstruments::SetModified(InstrumentHint hint, bool updateAll)
//---------------------------------------------------------------------
{
	m_modDoc.SetModified();
	m_modDoc.UpdateAllViews(nullptr, hint.SetData(m_nInstrument), updateAll ? nullptr : this);
}


BOOL CCtrlInstruments::SetCurrentInstrument(UINT nIns, BOOL bUpdNum)
//------------------------------------------------------------------
{
	if (m_sndFile.m_nInstruments < 1) return FALSE;
	if ((nIns < 1) || (nIns > m_sndFile.m_nInstruments)) return FALSE;
	LockControls();
	if ((m_nInstrument != nIns) || (!m_bInitialized))
	{
		m_nInstrument = static_cast<INSTRUMENTINDEX>(nIns);
		m_NoteMap.SetCurrentInstrument(m_nInstrument);
		UpdateView(InstrumentHint(m_nInstrument).Info().Envelope(), NULL);
	} else
	{
		// Just in case
		m_NoteMap.SetCurrentInstrument(m_nInstrument);
	}
	if (bUpdNum)
	{
		SetDlgItemInt(IDC_EDIT_INSTRUMENT, m_nInstrument);
		m_SpinInstrument.SetRange(1, m_sndFile.GetNumInstruments());
		m_SpinInstrument.EnableWindow((m_sndFile.GetNumInstruments()) ? TRUE : FALSE);
		// Is this a bug ?
		m_SliderCutOff.InvalidateRect(NULL, FALSE);
		m_SliderResonance.InvalidateRect(NULL, FALSE);
		// Volume ramping (attack)
		m_SliderAttack.InvalidateRect(NULL, FALSE);
	}
	PostViewMessage(VIEWMSG_SETCURRENTINSTRUMENT, m_nInstrument);
	UnlockControls();

	return TRUE;
}


void CCtrlInstruments::OnActivatePage(LPARAM lParam)
//--------------------------------------------------
{
	if (lParam < 0)
	{
		int nIns = m_parent.GetInstrumentChange();
		if (nIns > 0) lParam = nIns;
		m_parent.InstrumentChanged(-1);
	} else
	if (lParam > 0)
	{
		m_parent.InstrumentChanged(lParam);
	}

	UpdatePluginList();

	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	INSTRUMENTVIEWSTATE &instrumentState = pFrame->GetInstrumentViewState();
	if(instrumentState.initialInstrument != 0)
	{
		m_nInstrument = instrumentState.initialInstrument;
		instrumentState.initialInstrument = 0;
	}

	SetCurrentInstrument((lParam > 0) ? lParam : m_nInstrument);

	// Initial Update
	if (!m_bInitialized) UpdateView(InstrumentHint(m_nInstrument).Info().Envelope().ModType(), NULL);

	if (pFrame) PostViewMessage(VIEWMSG_LOADSTATE, (LPARAM)&instrumentState);
	SwitchToView();

	// Combo boxes randomly disappear without this... why?
	Invalidate();
}


void CCtrlInstruments::OnDeactivatePage()
//---------------------------------------
{
	m_modDoc.NoteOff(0, true);
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if ((pFrame) && (m_hWndView)) SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&pFrame->GetInstrumentViewState());
}


LRESULT CCtrlInstruments::OnModCtrlMsg(WPARAM wParam, LPARAM lParam)
//------------------------------------------------------------------
{
	switch(wParam)
	{
	case CTRLMSG_GETCURRENTINSTRUMENT:
		return m_nInstrument;
		break;

	case CTRLMSG_INS_PREVINSTRUMENT:
		OnPrevInstrument();
		break;

	case CTRLMSG_INS_NEXTINSTRUMENT:
		OnNextInstrument();
		break;

	case CTRLMSG_INS_OPENFILE_NEW:
		if(!InsertInstrument(false))
			break;
		MPT_FALLTHROUGH;
	case CTRLMSG_INS_OPENFILE:
		if (lParam) return OpenInstrument(*reinterpret_cast<const mpt::PathString *>(lParam));
		break;

	case CTRLMSG_INS_SONGDROP:
		if (lParam)
		{
			const DRAGONDROP *pDropInfo = (const DRAGONDROP *)lParam;
			CSoundFile *pSndFile = (CSoundFile *)(pDropInfo->lDropParam);
			if (pDropInfo->pModDoc) pSndFile = pDropInfo->pModDoc->GetSoundFile();
			if (pSndFile) return OpenInstrument(*pSndFile, static_cast<INSTRUMENTINDEX>(pDropInfo->dwDropItem));
		}
		break;

	case CTRLMSG_INS_NEWINSTRUMENT:
		OnInstrumentNew();
		break;

	case CTRLMSG_SETCURRENTINSTRUMENT:
		SetCurrentInstrument(lParam);
		break;

	case CTRLMSG_INS_SAMPLEMAP:
		OnEditSampleMap();
		break;

	case IDC_INSTRUMENT_NEW:
		OnInstrumentNew();
		break;
	case IDC_INSTRUMENT_OPEN:
		OnInstrumentOpen();
		break;
	case IDC_INSTRUMENT_SAVEAS:
		OnInstrumentSave();
		break;

	default:
		return CModControlDlg::OnModCtrlMsg(wParam, lParam);
	}
	return 0;
}


void CCtrlInstruments::UpdateView(UpdateHint hint, CObject *pObj)
//---------------------------------------------------------------
{
	if(pObj == this) return;
	if (hint.GetType()[HINT_MPTOPTIONS])
	{
		m_ToolBar.UpdateStyle();
		hint.ModType(); // For possibly updating note names in Pitch/Pan Separation dropdown
	}
	LockControls();
	if (hint.ToType<PluginHint>().GetType()[HINT_MIXPLUGINS | HINT_PLUGINNAMES]) OnMixPlugChanged();
	UnlockControls();

	const InstrumentHint instrHint = hint.ToType<InstrumentHint>();
	FlagSet<HintType> hintType = instrHint.GetType();
	if (!m_bInitialized) hintType.set(HINT_MODTYPE);
	if(!hintType[HINT_MODTYPE | HINT_INSTRUMENT | HINT_ENVELOPE]) return;

	const INSTRUMENTINDEX updateIns = instrHint.GetInstrument();
	if(updateIns != m_nInstrument && updateIns != 0 && !hintType[HINT_MODTYPE]) return;

	LockControls();
	const ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];

	if (hintType[HINT_MODTYPE])
	{
		auto &specs = m_sndFile.GetModSpecifications();

		// Limit text fields
		m_EditName.SetLimitText(specs.instrNameLengthMax);
		m_EditFileName.SetLimitText(specs.instrFilenameLengthMax);

		const BOOL bITandMPT = ((m_sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (m_sndFile.GetNumInstruments())) ? TRUE : FALSE;
		const BOOL bITandXM = ((m_sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM))  && (m_sndFile.GetNumInstruments())) ? TRUE : FALSE;
		const BOOL bMPTOnly = ((m_sndFile.GetType() == MOD_TYPE_MPT) && (m_sndFile.GetNumInstruments())) ? TRUE : FALSE;
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT10), bITandXM);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT11), bITandXM);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT7), bITandXM);
		m_EditName.EnableWindow(bITandXM);
		m_EditFileName.EnableWindow(bITandMPT);
		m_CbnMidiCh.EnableWindow(bITandXM);
		m_CbnMixPlug.EnableWindow(bITandXM);
		m_SpinMidiPR.EnableWindow(bITandXM);
		m_SpinMidiBK.EnableWindow(bITandXM);

		const bool extendedFadeoutRange = !(m_sndFile.GetType() & MOD_TYPE_IT);
		m_SpinFadeOut.EnableWindow(bITandXM);
		m_SpinFadeOut.SetRange(0, extendedFadeoutRange ? 32767 : 8192);
		m_EditFadeOut.SetLimitText(extendedFadeoutRange ? 5 : 4);
		// XM-style fade-out is 32 times more precise than IT
		UDACCEL accell[2];
		accell[0].nSec = 0;
		accell[0].nInc = (m_sndFile.GetType() == MOD_TYPE_IT ? 32 : 1);
		accell[1].nSec = 2;
		accell[1].nInc = 5 * accell[0].nInc;
		m_SpinFadeOut.SetAccel(CountOf(accell), accell);

		// Panning ranges (0...64 for IT, 0...256 for MPTM)
		m_SpinPanning.SetRange(0, (m_sndFile.GetType() & MOD_TYPE_IT) ? 64 : 256);

		// Pitch Wheel Depth
		if(m_sndFile.GetType() == MOD_TYPE_XM)
			m_SpinPWD.SetRange(0, 36);
		else
			m_SpinPWD.SetRange(-128, 127);
		m_EditPWD.EnableWindow(bITandXM);
		m_SpinPWD.EnableWindow(bITandXM);

		m_NoteMap.EnableWindow(bITandXM);

		m_ComboNNA.EnableWindow(bITandMPT);
		m_SliderVolSwing.EnableWindow(bITandMPT);
		m_SliderPanSwing.EnableWindow(bITandMPT);
		m_ComboDCT.EnableWindow(bITandMPT);
		m_ComboDCA.EnableWindow(bITandMPT);
		m_ComboPPC.EnableWindow(bITandMPT);
		m_SpinPPS.EnableWindow(bITandMPT);
		m_EditGlobalVol.EnableWindow(bITandMPT);
		m_SpinGlobalVol.EnableWindow(bITandMPT);
		m_EditPanning.EnableWindow(bITandMPT);
		m_SpinPanning.EnableWindow(bITandMPT);
		m_CheckPanning.EnableWindow(bITandMPT);
		m_EditPPS.EnableWindow(bITandMPT);
		m_CheckCutOff.EnableWindow(bITandMPT);
		m_CheckResonance.EnableWindow(bITandMPT);
		m_SliderCutOff.EnableWindow(bITandMPT);
		m_SliderResonance.EnableWindow(bITandMPT);
		m_ComboTuning.EnableWindow(bMPTOnly);
		m_EditPitchTempoLock.EnableWindow(bMPTOnly);
		m_CheckPitchTempoLock.EnableWindow(bMPTOnly);

		// MIDI Channel
		// XM has no "mapped" MIDI channels.
		m_CbnMidiCh.ResetContent();
		for(int ich = MidiNoChannel; ich <= (bITandMPT ? MidiMappedChannel : MidiLastChannel); ich++)
		{
			CString s;
			if (ich == MidiNoChannel)
				s = _T("None");
			else if (ich == MidiMappedChannel)
				s = _T("Mapped");
			else
				s.Format(_T("%u"), ich);
			m_CbnMidiCh.SetItemData(m_CbnMidiCh.AddString(s), ich);
		}
	}
	if (hintType[HINT_MODTYPE | HINT_INSTRUMENT])
	{
		m_SpinInstrument.SetRange(1, m_sndFile.m_nInstruments);
		m_SpinInstrument.EnableWindow((m_sndFile.m_nInstruments) ? TRUE : FALSE);

		// Backwards compatibility with legacy IT/XM modules that use now deprecated hack features.
		m_SliderCutSwing.EnableWindow(pIns != nullptr && (m_sndFile.GetType() == MOD_TYPE_MPT || pIns->nCutSwing != 0));
		m_SliderResSwing.EnableWindow(pIns != nullptr && (m_sndFile.GetType() == MOD_TYPE_MPT || pIns->nResSwing != 0));
		m_CbnFilterMode.EnableWindow (pIns != nullptr && (m_sndFile.GetType() == MOD_TYPE_MPT || pIns->nFilterMode != FLTMODE_UNCHANGED));
		m_CbnResampling.EnableWindow (pIns != nullptr && (m_sndFile.GetType() == MOD_TYPE_MPT || pIns->nResampling != SRCMODE_DEFAULT));
		m_SliderAttack.EnableWindow  (pIns != nullptr && (m_sndFile.GetType() == MOD_TYPE_MPT || pIns->nVolRampUp));
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT2), pIns != nullptr && (m_sndFile.GetType() == MOD_TYPE_MPT || pIns->nVolRampUp));

		if (pIns)
		{
			m_EditName.SetWindowText(pIns->name);
			m_EditFileName.SetWindowText(pIns->filename);
			// Fade Out Volume
			SetDlgItemInt(IDC_EDIT7, pIns->nFadeOut);
			// Global Volume
			SetDlgItemInt(IDC_EDIT8, pIns->nGlobalVol);
			// Panning
			SetDlgItemInt(IDC_EDIT9, (m_modDoc.GetModType() & MOD_TYPE_IT) ? (pIns->nPan / 4) : pIns->nPan);
			m_CheckPanning.SetCheck(pIns->dwFlags[INS_SETPANNING] ? TRUE : FALSE);
			// Midi
			if (pIns->nMidiProgram>0 && pIns->nMidiProgram<=128)
				SetDlgItemInt(IDC_EDIT10, pIns->nMidiProgram);
			else
				SetDlgItemText(IDC_EDIT10, _T("---"));
			if (pIns->wMidiBank && pIns->wMidiBank <= 16384)
				SetDlgItemInt(IDC_EDIT11, pIns->wMidiBank);
			else
				SetDlgItemText(IDC_EDIT11, _T("---"));

			if (pIns->nMidiChannel < 18)
			{
				m_CbnMidiCh.SetCurSel(pIns->nMidiChannel);
			} else
			{
				m_CbnMidiCh.SetCurSel(0);
			}
			if (pIns->nMixPlug <= MAX_MIXPLUGINS)
			{
				m_CbnMixPlug.SetCurSel(pIns->nMixPlug);
			} else
			{
				m_CbnMixPlug.SetCurSel(0);
			}
			OnMixPlugChanged();
			for(int nRes = 0; nRes<m_CbnResampling.GetCount(); nRes++)
			{
				DWORD v = m_CbnResampling.GetItemData(nRes);
				if (pIns->nResampling == v)
				{
					m_CbnResampling.SetCurSel(nRes);
					break;
				}
			}
			for(int nFltMode = 0; nFltMode<m_CbnFilterMode.GetCount(); nFltMode++)
			{
				DWORD v = m_CbnFilterMode.GetItemData(nFltMode);
				if (pIns->nFilterMode == v)
				{
					m_CbnFilterMode.SetCurSel(nFltMode);
					break;
				}
			}

			// NNA, DCT, DCA
			m_ComboNNA.SetCurSel(pIns->nNNA);
			m_ComboDCT.SetCurSel(pIns->nDCT);
			m_ComboDCA.SetCurSel(pIns->nDNA);
			// Pitch/Pan Separation
			if(hintType[HINT_MODTYPE] || pIns->pTuning != (CTuning *)GetWindowLongPtr(m_ComboPPC.m_hWnd, GWLP_USERDATA))
			{
				// Tuning may have changed, and thus the note names need to be updated
				m_ComboPPC.SetRedraw(FALSE);
				m_ComboPPC.ResetContent();
				AppendNotesToControlEx(m_ComboPPC, m_sndFile, m_nInstrument, NOTE_MIN, NOTE_MAX);
				SetWindowLongPtr(m_ComboPPC.m_hWnd, GWLP_USERDATA, (LONG_PTR)pIns->pTuning);
				m_ComboPPC.SetRedraw(TRUE);
			}
			m_ComboPPC.SetCurSel(pIns->nPPC);
			ASSERT((uint8)m_ComboPPC.GetItemData(m_ComboPPC.GetCurSel()) == pIns->nPPC + NOTE_MIN);
			SetDlgItemInt(IDC_EDIT15, pIns->nPPS);
			// Filter
			if (m_sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
			{
				m_CheckCutOff.SetCheck((pIns->IsCutoffEnabled()) ? TRUE : FALSE);
				m_CheckResonance.SetCheck((pIns->IsResonanceEnabled()) ? TRUE : FALSE);
				m_SliderVolSwing.SetPos(pIns->nVolSwing);
				m_SliderPanSwing.SetPos(pIns->nPanSwing);
				m_SliderResSwing.SetPos(pIns->nResSwing);
				m_SliderCutSwing.SetPos(pIns->nCutSwing);
				m_SliderCutOff.SetPos(pIns->GetCutoff());
				m_SliderResonance.SetPos(pIns->GetResonance());
				UpdateFilterText();
			}
			// Volume ramping (attack)
			int n = pIns->nVolRampUp; //? MAX_ATTACK_LENGTH - pIns->nVolRampUp : 0;
			m_SliderAttack.SetPos(n);
			if(n == 0) SetDlgItemText(IDC_EDIT2, _T("default"));
			else SetDlgItemInt(IDC_EDIT2,n);

			UpdateTuningComboBox();

			// Only enable Pitch/Tempo Lock for MPTM files or legacy files that have this property enabled.
			m_CheckPitchTempoLock.EnableWindow((m_sndFile.GetType() == MOD_TYPE_MPT || pIns->pitchToTempoLock.GetRaw() > 0) ? TRUE : FALSE);
			CheckDlgButton(IDC_CHECK_PITCHTEMPOLOCK, pIns->pitchToTempoLock.GetRaw() > 0 ? BST_CHECKED : BST_UNCHECKED);
			m_EditPitchTempoLock.EnableWindow(pIns->pitchToTempoLock.GetRaw() > 0 ? TRUE : FALSE);
			if(pIns->pitchToTempoLock.GetRaw() > 0)
			{
				m_EditPitchTempoLock.SetTempoValue(pIns->pitchToTempoLock);
			}

			// Pitch Wheel Depth
			SetDlgItemInt(IDC_PITCHWHEELDEPTH, pIns->midiPWD, TRUE);

			if(m_sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT))
			{
				BOOL enableVol = (m_CbnMixPlug.GetCurSel() > 0 && !m_sndFile.m_playBehaviour[kMIDICCBugEmulation]) ? TRUE : FALSE;
				velocityStyle.EnableWindow(enableVol);
				m_CbnPluginVolumeHandling.EnableWindow(enableVol);
			}
		} else
		{
			m_EditName.SetWindowText("");
			m_EditFileName.SetWindowText("");
			velocityStyle.EnableWindow(FALSE);
			m_CbnPluginVolumeHandling.EnableWindow(FALSE);
			if(m_nInstrument > m_sndFile.GetNumInstruments())
				SetCurrentInstrument(m_sndFile.GetNumInstruments());

		}
		m_NoteMap.InvalidateRect(NULL, FALSE);
	}
	if(hint.ToType<PluginHint>().GetType()[HINT_MIXPLUGINS | HINT_PLUGINNAMES | HINT_MODTYPE])
	{
		UpdatePluginList();
	}

	if (!m_bInitialized)
	{
		// First update
		m_bInitialized = TRUE;
		UnlockControls();
	}

	m_ComboNNA.Invalidate(FALSE);
	m_ComboDCT.Invalidate(FALSE);
	m_ComboDCA.Invalidate(FALSE);
	m_ComboPPC.Invalidate(FALSE);
	m_CbnMidiCh.Invalidate(FALSE);
	m_CbnMixPlug.Invalidate(FALSE);
	m_CbnResampling.Invalidate(FALSE);
	m_CbnFilterMode.Invalidate(FALSE);
	m_CbnPluginVolumeHandling.Invalidate(FALSE);
	m_ComboTuning.Invalidate(FALSE);
	UnlockControls();
}


void CCtrlInstruments::UpdateFilterText()
//---------------------------------------
{
	if(m_nInstrument)
	{
		ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
		if(pIns)
		{
			TCHAR s[32];
			// In IT Compatible mode, it is enough to just have resonance enabled to turn on the filter.
			const bool resEnabled = (pIns->IsResonanceEnabled() && pIns->GetResonance() > 0 && m_sndFile.m_playBehaviour[kITFilterBehaviour]);

			if((pIns->IsCutoffEnabled() && pIns->GetCutoff() < 0x7F) || resEnabled)
			{
				const BYTE cutoff = (resEnabled && !pIns->IsCutoffEnabled()) ? 0x7F : pIns->GetCutoff();
				wsprintf(s, _T("Z%02X (%u Hz)"), cutoff, m_sndFile.CutOffToFrequency(cutoff));
			} else if(pIns->IsCutoffEnabled())
			{
				_tcscpy(s, _T("Z7F (Off)"));
			} else
			{
				_tcscpy(s, _T("No Change"));
			}

			SetDlgItemText(IDC_FILTERTEXT, s);
		}
	}
}


bool CCtrlInstruments::OpenInstrument(const mpt::PathString &fileName)
//--------------------------------------------------------------------
{
	BeginWaitCursor();
	InputFile f(fileName);
	if(!f.IsValid())
	{
		EndWaitCursor();
		return false;
	}

	FileReader file = GetFileReader(f);

	bool first = false, ok = false;
	if (file.IsValid())
	{
		if (!m_sndFile.GetNumInstruments())
		{
			first = true;
			m_sndFile.m_nInstruments = 1;
			m_modDoc.SetModified();
		}
		if (!m_nInstrument) m_nInstrument = 1;
		if (m_sndFile.ReadInstrumentFromFile(m_nInstrument, file, TrackerSettings::Instance().m_MayNormalizeSamplesOnLoad))
		{
			ok = true;
		}
	}

	if(!ok && first)
	{
		// Undo adding the instrument
		delete m_sndFile.Instruments[1];
		m_sndFile.m_nInstruments = 0;
	} else if(ok && first)
	{
		m_NoteMap.SetCurrentInstrument(1);
	}

	EndWaitCursor();
	if(ok)
	{
		TrackerSettings::Instance().PathInstruments.SetWorkingDir(fileName, true);
		ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
		if (pIns)
		{
			mpt::PathString name, ext;
			fileName.SplitPath(nullptr, nullptr, &name, &ext);

			if (!pIns->name[0] && m_sndFile.GetModSpecifications().instrNameLengthMax > 0)
			{
				mpt::String::CopyN(pIns->name, name.ToLocale().c_str(), m_sndFile.GetModSpecifications().instrNameLengthMax);
			}
			if (!pIns->filename[0] && m_sndFile.GetModSpecifications().instrFilenameLengthMax > 0)
			{
				name += ext;
				mpt::String::CopyN(pIns->filename, name.ToLocale().c_str(), m_sndFile.GetModSpecifications().instrFilenameLengthMax);
			}

			SetCurrentInstrument(m_nInstrument);
			InstrumentHint hint = InstrumentHint().Info().Envelope().Names();
			if(first) hint.ModType();
			SetModified(hint, true);
		} else ok = FALSE;
	} else
	{
		// Try loading as module
		ok = CMainFrame::GetMainFrame()->SetTreeSoundfile(file);
		if(ok) return true;
	}
	SampleHint hint = SampleHint().Info().Data().Names();
	if (first) hint.ModType();
	m_modDoc.UpdateAllViews(nullptr, hint);
	if (!ok) ErrorBox(IDS_ERR_FILETYPE, this);
	return ok;
}


bool CCtrlInstruments::OpenInstrument(CSoundFile &sndFile, INSTRUMENTINDEX nInstr)
//--------------------------------------------------------------------------------
{
	if((!nInstr) || (nInstr > sndFile.GetNumInstruments())) return false;
	BeginWaitCursor();

	CriticalSection cs;

	bool first = false;
	if (!m_sndFile.GetNumInstruments())
	{
		first = true;
		m_sndFile.m_nInstruments = 1;
		m_NoteMap.SetCurrentInstrument(1);
		first = true;
	}
	if (!m_nInstrument)
	{
		m_nInstrument = 1;
		first = true;
	}
	m_sndFile.ReadInstrumentFromSong(m_nInstrument, sndFile, nInstr);

	cs.Leave();

	{
		InstrumentHint hint = InstrumentHint().Info().Envelope().Names();
		if (first) hint.ModType();
		SetModified(hint, true);
	}
	{
		SampleHint hint = SampleHint().Info().Data().Names();
		if (first) hint.ModType();
		m_modDoc.UpdateAllViews(nullptr, hint, this);
	}
	EndWaitCursor();
	return true;
}


BOOL CCtrlInstruments::EditSample(UINT nSample)
//---------------------------------------------
{
	if ((nSample > 0) && (nSample < MAX_SAMPLES))
	{
		m_parent.PostMessage(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_SAMPLES, nSample);
		return TRUE;
	}
	return FALSE;
}


BOOL CCtrlInstruments::GetToolTipText(UINT uId, LPSTR pszText)
//------------------------------------------------------------
{
	//Note: pszText seems to point to char array of length 256 (Noverber 2006).
	//Note2: If there's problems in getting tooltips showing for certain tools,
	//		 setting the tab order may have effect.
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];

	if(pIns == nullptr) return FALSE;
	if ((pszText) && (uId))
	{
		switch(uId)
		{
		case IDC_EDIT_PITCHTEMPOLOCK:
		case IDC_CHECK_PITCHTEMPOLOCK:
			// Pitch/Tempo lock
			{
				const CModSpecifications& specs = m_sndFile.GetModSpecifications();
				wsprintf(pszText, _T("Tempo range: %u - %u"), specs.GetTempoMin().GetInt(), specs.GetTempoMax().GetInt());
				return TRUE;
			}

		case IDC_EDIT7:
			// Fade Out
			if(!pIns->nFadeOut)
				_tcscpy(pszText, _T("Fade disabled"));
			else
				wsprintf(pszText, _T("%u ticks (Higher value <-> Faster fade out)"), 0x8000 / pIns->nFadeOut);
			return TRUE;

		case IDC_EDIT8:
			// Global volume
			_tcscpy(pszText, CModDoc::LinearToDecibels(GetDlgItemInt(IDC_EDIT8), 64.0));
			return TRUE;

		case IDC_EDIT9:
			// Panning
			_tcscpy(pszText, CModDoc::PanningToString(pIns->nPan, 128));
			return TRUE;

#ifndef NO_PLUGINS
		case IDC_EDIT10:
		case IDC_EDIT11:
			// Show plugin program name when hovering program or bank edits
			if(pIns->nMixPlug > 0 && pIns->nMidiProgram != 0)
			{
				const SNDMIXPLUGIN &plugin = m_sndFile.m_MixPlugins[pIns->nMixPlug - 1];
				if(plugin.pMixPlugin != nullptr)
				{
					int32 prog = pIns->nMidiProgram - 1;
					if(pIns->wMidiBank > 1) prog += 128 * (pIns->wMidiBank - 1);
					_tcscpy(pszText, plugin.pMixPlugin->GetFormattedProgramName(prog));
				}
			}
			return TRUE;
#endif // NO_PLUGINS

		case IDC_PLUGIN_VELOCITYSTYLE:
		case IDC_PLUGIN_VOLUMESTYLE:
			// Plugin volume handling
			if(pIns->nMixPlug < 1) return FALSE;
			if(m_sndFile.m_playBehaviour[kMIDICCBugEmulation])
			{
				velocityStyle.EnableWindow(FALSE);
				m_CbnPluginVolumeHandling.EnableWindow(FALSE);
				_tcscpy(pszText, _T("To enable, clear Plugin volume command bug emulation flag from Song Properties"));
				return TRUE;
			} else
			{
				if(uId == IDC_PLUGIN_VELOCITYSTYLE)
				{
					_tcscpy(pszText, _T("Volume commands (vxx) next to a note are sent as note velocity instead."));
					return TRUE;
				}
				return FALSE;
			}

		case IDC_COMBO5:
			// MIDI Channel
			_tcscpy(pszText, _T("Mapped: MIDI channel corresponds to pattern channel modulo 16"));
			return TRUE;

		case IDC_SLIDER1:
			wsprintf(pszText, "±%d%% volume variation", pIns->nVolSwing);
			return TRUE;

		case IDC_SLIDER2:
			wsprintf(pszText, "±%d panning variation", pIns->nPanSwing);
			return TRUE;

		case IDC_SLIDER3:
			wsprintf(pszText, _T("%u"), pIns->GetCutoff());
			return TRUE;

		case IDC_SLIDER4:
			wsprintf(pszText, _T("%u (%u dB)"), pIns->GetResonance(), Util::muldivr(pIns->GetResonance(), 24, 128));
			return TRUE;

		case IDC_SLIDER6:
			wsprintf(pszText, "±%d cutoff variation", pIns->nCutSwing);
			return TRUE;

		case IDC_SLIDER7:
			wsprintf(pszText, "±%d resonance variation", pIns->nResSwing);
			return TRUE;

		case IDC_PITCHWHEELDEPTH:
			_tcscpy(pszText, _T("Set this to the actual Pitch Wheel Depth used in your plugin on this channel."));
			return TRUE;

		}
	}
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////
// CCtrlInstruments Messages

void CCtrlInstruments::OnInstrumentChanged()
//------------------------------------------
{
	if(!IsLocked())
	{
		UINT n = GetDlgItemInt(IDC_EDIT_INSTRUMENT);
		if ((n > 0) && (n <= m_sndFile.GetNumInstruments()) && (n != m_nInstrument))
		{
			SetCurrentInstrument(n, FALSE);
			m_parent.InstrumentChanged(n);
		}
	}
}


void CCtrlInstruments::OnPrevInstrument()
//---------------------------------------
{
	if(m_nInstrument > 1)
		SetCurrentInstrument(m_nInstrument - 1);
	else
		SetCurrentInstrument(m_sndFile.GetNumInstruments());
	m_parent.InstrumentChanged(m_nInstrument);
}


void CCtrlInstruments::OnNextInstrument()
//---------------------------------------
{
	if(m_nInstrument < m_sndFile.GetNumInstruments())
		SetCurrentInstrument(m_nInstrument + 1);
	else
		SetCurrentInstrument(1);
	m_parent.InstrumentChanged(m_nInstrument);
}


void CCtrlInstruments::OnInstrumentNew()
//--------------------------------------
{
	InsertInstrument(m_sndFile.GetNumInstruments() > 0 && CMainFrame::GetInputHandler()->ShiftPressed());
	SwitchToView();
}


bool CCtrlInstruments::InsertInstrument(bool duplicate)
//-----------------------------------------------------
{
	const bool hasInstruments = m_sndFile.GetNumInstruments() > 0;

	INSTRUMENTINDEX ins = m_modDoc.InsertInstrument(SAMPLEINDEX_INVALID, (duplicate && hasInstruments) ? m_nInstrument : INSTRUMENTINDEX_INVALID);
	if (ins == INSTRUMENTINDEX_INVALID)
		return false;

	if (!hasInstruments) m_modDoc.UpdateAllViews(nullptr, InstrumentHint().Info().Names().ModType());

	SetCurrentInstrument(ins);
	m_modDoc.UpdateAllViews(nullptr, InstrumentHint(ins).Info().Envelope().Names());
	m_parent.InstrumentChanged(m_nInstrument);
	return true;
}


void CCtrlInstruments::OnInstrumentOpen()
//---------------------------------------
{
	static int nLastIndex = 0;

	std::vector<FileType> mediaFoundationTypes = CSoundFile::GetMediaFoundationFileTypes();
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.EnableAudioPreview()
		.ExtensionFilter(
			"All Instruments|*.xi;*.pat;*.iti;*.sfz;*.flac;*.wav;*.aif;*.aiff;*.au;*.snd;*.sf2;*.sbk;*.dls;*.oga;*.ogg;*.opus" + ToFilterOnlyString(mediaFoundationTypes, true).ToLocale() + "|"
			"FastTracker II Instruments (*.xi)|*.xi|"
			"GF1 Patches (*.pat)|*.pat|"
			"Impulse Tracker Instruments (*.iti)|*.iti|"
			"SFZ Instruments (*.sfz)|*.sfz|"
			"SoundFont 2.0 Banks (*.sf2)|*.sf2;*.sbk|"
			"DLS Sound Banks (*.dls)|*.dls|"
			"All Files (*.*)|*.*||")
		.WorkingDirectory(TrackerSettings::Instance().PathInstruments.GetWorkingDir())
		.FilterIndex(&nLastIndex);
	if(!dlg.Show(this)) return;

	TrackerSettings::Instance().PathInstruments.SetWorkingDir(dlg.GetWorkingDirectory());

	const FileDialog::PathList &files = dlg.GetFilenames();
	for(size_t counter = 0; counter < files.size(); counter++)
	{
		//If loading multiple instruments, advancing to next instrument and creating
		//new instrument if necessary.
		if(counter > 0)
		{
			if(m_nInstrument >= MAX_INSTRUMENTS - 1)
				break;
			else
				m_nInstrument++;

			if(m_nInstrument > m_sndFile.GetNumInstruments())
				OnInstrumentNew();
		}

		if(!OpenInstrument(files[counter]))
			ErrorBox(IDS_ERR_FILEOPEN, this);
	}

	m_parent.InstrumentChanged(m_nInstrument);
	SwitchToView();
}


void CCtrlInstruments::OnInstrumentSave()
//---------------------------------------
{
	TCHAR szFileName[_MAX_PATH] = _T("");
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];

	if (!pIns) return;
	if (pIns->filename[0])
	{
		mpt::String::Copy(szFileName, pIns->filename);
	} else
	{
		mpt::String::Copy(szFileName, pIns->name);
	}
	SanitizeFilename(szFileName);

	int index = (m_sndFile.GetType() == MOD_TYPE_XM || !TrackerSettings::Instance().compressITI) ? 1 : 2;
	FileDialog dlg = SaveFileDialog()
		.DefaultExtension(m_sndFile.GetType() == MOD_TYPE_XM ? "xi" : "iti")
		.DefaultFilename(szFileName)
		.ExtensionFilter((m_sndFile.GetType() == MOD_TYPE_XM) ?
		"FastTracker II Instruments (*.xi)|*.xi|"
		"Impulse Tracker Instruments (*.iti)|*.iti|"
		"Compressed Impulse Tracker Instruments (*.iti)|*.iti||"
		: "Impulse Tracker Instruments (*.iti)|*.iti|"
		"Compressed Impulse Tracker Instruments (*.iti)|*.iti|"
		"Impulse Tracker Instruments with external Samples (*.iti)|*.iti|"
		"FastTracker II Instruments (*.xi)|*.xi||")
		.WorkingDirectory(TrackerSettings::Instance().PathInstruments.GetWorkingDir())
		.FilterIndex(&index);
	if(!dlg.Show(this)) return;

	BeginWaitCursor();

	bool ok = false;
	if(!mpt::PathString::CompareNoCase(dlg.GetExtension(), MPT_PATHSTRING("xi")))
		ok = m_sndFile.SaveXIInstrument(m_nInstrument, dlg.GetFirstFile());
	else
		ok = m_sndFile.SaveITIInstrument(m_nInstrument, dlg.GetFirstFile(), index == (m_sndFile.GetType() == MOD_TYPE_XM ? 3 : 2), m_sndFile.GetType() != MOD_TYPE_XM && index == 3);

	EndWaitCursor();
	if (!ok)
		ErrorBox(IDS_ERR_SAVEINS, this);
	else
		TrackerSettings::Instance().PathInstruments.SetWorkingDir(dlg.GetWorkingDirectory());
	SwitchToView();
}


void CCtrlInstruments::OnInstrumentPlay()
//---------------------------------------
{
	if (m_modDoc.IsNotePlaying(NOTE_MIDDLEC, 0, m_nInstrument))
	{
		m_modDoc.NoteOff(NOTE_MIDDLEC, true, m_nInstrument);
	} else
	{
		m_modDoc.PlayNote(NOTE_MIDDLEC, m_nInstrument, 0, false);
	}
	SwitchToView();
}


void CCtrlInstruments::OnNameChanged()
//------------------------------------
{
	if (!IsLocked())
	{
		TCHAR s[64];
		s[0] = 0;
		m_EditName.GetWindowText(s, sizeof(s));
		ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
		if ((pIns) && (strncmp(s, pIns->name, MAX_INSTRUMENTNAME)))
		{
			mpt::String::Copy(pIns->name, s);
			SetModified(InstrumentHint().Names(), false);
		}
	}
}


void CCtrlInstruments::OnFileNameChanged()
//----------------------------------------
{
	if (!IsLocked())
	{
		TCHAR s[64];
		s[0] = 0;
		m_EditFileName.GetWindowText(s, sizeof(s));
		ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
		if ((pIns) && (strncmp(s, pIns->filename, 12)))
		{
			mpt::String::Copy(pIns->filename, s);
			SetModified(InstrumentHint().Names(), false);
		}
	}
}


void CCtrlInstruments::OnFadeOutVolChanged()
//------------------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int minval = 0, maxval = 32767;
		m_SpinFadeOut.GetRange(minval, maxval);
		int nVol = GetDlgItemInt(IDC_EDIT7);
		Limit(nVol, minval, maxval);

		if(nVol != (int)pIns->nFadeOut)
		{
			pIns->nFadeOut = nVol;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnGlobalVolChanged()
//-----------------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int nVol = GetDlgItemInt(IDC_EDIT8);
		Limit(nVol, 0, 64);
		if (nVol != (int)pIns->nGlobalVol)
		{
			// Live-adjust volume
			pIns->nGlobalVol = nVol;
			for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
			{
				auto &chn = m_sndFile.m_PlayState.Chn[i];
				if(chn.pModInstrument == pIns)
				{
					chn.UpdateInstrumentVolume(chn.pModSample, pIns);
				}
			}
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnSetPanningChanged()
//------------------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		const bool b = m_CheckPanning.GetCheck() != BST_UNCHECKED;

		pIns->dwFlags.set(INS_SETPANNING, b);

		if(b && m_sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			bool smpPanningInUse = false;

			const std::set<SAMPLEINDEX> referencedSamples = pIns->GetSamples();

			for(auto sample = referencedSamples.cbegin(); sample != referencedSamples.cend(); sample++)
			{
				if(*sample <= m_sndFile.GetNumSamples() && m_sndFile.GetSample(*sample).uFlags[CHN_PANNING])
				{
					smpPanningInUse = true;
					break;
				}
			}

			if(smpPanningInUse)
			{
				if(Reporting::Confirm(_T("Some of the samples used in the instrument have \"Set Pan\" enabled. "
						"Sample panning overrides instrument panning for the notes associated with such samples. "
						"Do you wish to disable panning from those samples so that the instrument pan setting is effective "
						"for the whole instrument?")) == cnfYes)
				{
					for(size_t i = 0; i < CountOf(pIns->Keyboard); i++)
					{
						const SAMPLEINDEX smp = pIns->Keyboard[i];
						if(smp > 0 && smp <= m_sndFile.GetNumSamples())
							m_sndFile.GetSample(smp).uFlags.reset(CHN_PANNING);
					}
					m_modDoc.UpdateAllViews(nullptr, SampleHint().Info().ModType(), this);
				}
			}
		}
		SetModified(InstrumentHint().Info(), false);
	}
}


void CCtrlInstruments::OnPanningChanged()
//---------------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int nPan = GetDlgItemInt(IDC_EDIT9);
		if(m_modDoc.GetModType() & MOD_TYPE_IT)	// IT panning ranges from 0 to 64
			nPan *= 4;
		if (nPan < 0) nPan = 0;
		if (nPan > 256) nPan = 256;
		if (nPan != (int)pIns->nPan)
		{
			pIns->nPan = nPan;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnNNAChanged()
//-----------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		if (pIns->nNNA != m_ComboNNA.GetCurSel())
		{
			pIns->nNNA = static_cast<uint8>(m_ComboNNA.GetCurSel());
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnDCTChanged()
//-----------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		if (pIns->nDCT != m_ComboDCT.GetCurSel())
		{
			pIns->nDCT = static_cast<uint8>(m_ComboDCT.GetCurSel());
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnDCAChanged()
//-----------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		if (pIns->nDNA != m_ComboDCA.GetCurSel())
		{
			pIns->nDNA = static_cast<uint8>(m_ComboDCA.GetCurSel());
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnMPRChanged()
//-----------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int n = GetDlgItemInt(IDC_EDIT10);
		if ((n >= 0) && (n <= 128))
		{
			if (pIns->nMidiProgram != n)
			{
				pIns->nMidiProgram = static_cast<uint8>(n);
				SetModified(InstrumentHint().Info(), false);
			}
		}
		// we will not set the midi bank/program if it is 0
		if(n == 0)
		{
			LockControls();
			SetDlgItemText(IDC_EDIT10, _T("---"));
			UnlockControls();
		}
	}
}


void CCtrlInstruments::OnMPRKillFocus()
//-------------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int n = GetDlgItemInt(IDC_EDIT10);
		if (n > 128)
		{
			n--;
			pIns->nMidiProgram = static_cast<uint8>(n % 128 + 1);
			pIns->wMidiBank = static_cast<uint16>(n / 128 + 1);
			SetModified(InstrumentHint().Info(), false);

			LockControls();
			SetDlgItemInt(IDC_EDIT10, pIns->nMidiProgram);
			SetDlgItemInt(IDC_EDIT11, pIns->wMidiBank);
			UnlockControls();
		}
	}
}


void CCtrlInstruments::OnMBKChanged()
//-----------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		uint16 w = static_cast<uint16>(GetDlgItemInt(IDC_EDIT11));
		if(w >= 0 && w <= 16384 && pIns->wMidiBank != w)
		{
			pIns->wMidiBank = w;
			SetModified(InstrumentHint().Info(), false);
		}
		// we will not set the midi bank/program if it is 0
		if(w == 0)
		{
			LockControls();
			SetDlgItemText(IDC_EDIT11, _T("---"));
			UnlockControls();
		}
	}
}


void CCtrlInstruments::OnMCHChanged()
//-----------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if(!IsLocked() && pIns)
	{
		uint8 ch = static_cast<uint8>(m_CbnMidiCh.GetItemData(m_CbnMidiCh.GetCurSel()));
		if(pIns->nMidiChannel != ch)
		{
			pIns->nMidiChannel = ch;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}

void CCtrlInstruments::OnResamplingChanged()
//------------------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		uint32 n = m_CbnResampling.GetItemData(m_CbnResampling.GetCurSel());
		if (pIns->nResampling != n)
		{
			pIns->nResampling = n;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnMixPlugChanged()
//---------------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	PLUGINDEX nPlug = static_cast<PLUGINDEX>(m_CbnMixPlug.GetItemData(m_CbnMixPlug.GetCurSel()));

	bool wasOpenedWithMouse = openendPluginListWithMouse;
	openendPluginListWithMouse = false;

	if (pIns)
	{
		BOOL enableVol = (nPlug < 1 || m_sndFile.m_playBehaviour[kMIDICCBugEmulation]) ? FALSE : TRUE;
		velocityStyle.EnableWindow(enableVol);
		m_CbnPluginVolumeHandling.EnableWindow(enableVol);

		if(nPlug >= 0 && nPlug <= MAX_MIXPLUGINS)
		{
			if ((!IsLocked()) && pIns->nMixPlug != nPlug)
			{
				pIns->nMixPlug = nPlug;
				SetModified(InstrumentHint().Info(), false);
			}

			velocityStyle.SetCheck(pIns->nPluginVelocityHandling == PLUGIN_VELOCITYHANDLING_CHANNEL ? BST_CHECKED : BST_UNCHECKED);
			m_CbnPluginVolumeHandling.SetCurSel(pIns->nPluginVolumeHandling);

#ifndef NO_PLUGINS
			if(pIns->nMixPlug)
			{
				// we have selected a plugin that's not "no plugin"
				const SNDMIXPLUGIN &plugin = m_sndFile.m_MixPlugins[pIns->nMixPlug - 1];
				bool active = !IsLocked();

				if(!plugin.IsValidPlugin() && active && wasOpenedWithMouse)
				{
					// No plugin in this slot yet: Ask user to add one.
					CSelectPluginDlg dlg(&m_modDoc, nPlug - 1, this);
					if (dlg.DoModal() == IDOK)
					{
						if(m_sndFile.GetModSpecifications().supportsPlugins)
						{
							m_modDoc.SetModified();
						}
						UpdatePluginList();

						m_modDoc.UpdateAllViews(nullptr, PluginHint(nPlug).Info().Names());
					}
				}

				if(plugin.pMixPlugin != nullptr)
				{
					GetDlgItem(IDC_INSVIEWPLG)->EnableWindow(true);

					if(active && plugin.pMixPlugin->IsInstrument())
					{
						if(pIns->nMidiChannel == MidiNoChannel)
						{
							// If this plugin can recieve MIDI events and we have no MIDI channel
							// selected for this instrument, automatically select MIDI channel 1.
							pIns->nMidiChannel = MidiFirstChannel;
							UpdateView(InstrumentHint(m_nInstrument).Info());
						}
						if(pIns->midiPWD == 0)
						{
							pIns->midiPWD = 2;
						}

						// If we just dialled up an instrument plugin, zap the sample assignments.
						const std::set<SAMPLEINDEX> referencedSamples = pIns->GetSamples();
						bool hasSamples = false;
						for(auto sample = referencedSamples.cbegin(); sample != referencedSamples.cend(); sample++)
						{
							if(*sample > 0 && *sample <= m_sndFile.GetNumSamples() && m_sndFile.GetSample(*sample).pSample != nullptr)
							{
								hasSamples = true;
								break;
							}
						}

						if(!hasSamples || Reporting::Confirm("Remove sample associations of this instrument?") == cnfYes)
						{
							pIns->AssignSample(0);
							m_NoteMap.Invalidate();
						}
					}
					return;
				}
			}
#endif // NO_PLUGINS
		}

	}
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_INSVIEWPLG), false);
}


void CCtrlInstruments::OnPPSChanged()
//-----------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int n = GetDlgItemInt(IDC_EDIT15);
		if ((n >= -32) && (n <= 32))
		{
			if (pIns->nPPS != (signed char)n)
			{
				pIns->nPPS = (signed char)n;
				SetModified(InstrumentHint().Info(), false);
			}
		}
	}
}


void CCtrlInstruments::OnAttackChanged()
//--------------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if(!IsLocked() && pIns)
	{
		int n = GetDlgItemInt(IDC_EDIT2);
		if(n < 0) n = 0;
		if(n > MAX_ATTACK_VALUE) n = MAX_ATTACK_VALUE;
		int newRamp = n; //? MAX_ATTACK_LENGTH - n : 0;

		if(pIns->nVolRampUp != newRamp)
		{
			pIns->nVolRampUp = newRamp;
			SetModified(InstrumentHint().Info(), false);
		}

		m_SliderAttack.SetPos(n);
		if( CSpinButtonCtrl *spin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN1) ) spin->SetPos(n);
		LockControls();
		if (n == 0) SetDlgItemText(IDC_EDIT2,"default");
		UnlockControls();
	}
}


void CCtrlInstruments::OnPPCChanged()
//-----------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int n = m_ComboPPC.GetCurSel();
		if(n >= 0 && n <= NOTE_MAX - NOTE_MIN)
		{
			if (pIns->nPPC != n)
			{
				pIns->nPPC = n;
				SetModified(InstrumentHint().Info(), false);
			}
		}
	}
}


void CCtrlInstruments::OnEnableCutOff()
//-------------------------------------
{
	const bool bCutOff = IsDlgButtonChecked(IDC_CHECK2) != BST_UNCHECKED;

	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if (pIns)
	{
		pIns->SetCutoff(pIns->GetCutoff(), bCutOff);
		for (CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
		{
			if (m_sndFile.m_PlayState.Chn[i].pModInstrument == pIns)
			{
				if (bCutOff)
				{
					m_sndFile.m_PlayState.Chn[i].nCutOff = pIns->GetCutoff();
				} else
				{
					m_sndFile.m_PlayState.Chn[i].nCutOff = 0x7F;
				}
			}
		}
	}
	UpdateFilterText();
	SetModified(InstrumentHint().Info(), false);
	SwitchToView();
}


void CCtrlInstruments::OnEnableResonance()
//----------------------------------------
{
	const bool bReso = IsDlgButtonChecked(IDC_CHECK3) != BST_UNCHECKED;

	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if (pIns)
	{
		pIns->SetResonance(pIns->GetResonance(), bReso);
		for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
		{
			if (m_sndFile.m_PlayState.Chn[i].pModInstrument == pIns)
			{
				if (bReso)
				{
					m_sndFile.m_PlayState.Chn[i].nResonance = pIns->GetResonance();
				} else
				{
					m_sndFile.m_PlayState.Chn[i].nResonance = 0;
				}
			}
		}
	}
	UpdateFilterText();
	SetModified(InstrumentHint().Info(), false);
	SwitchToView();
}

void CCtrlInstruments::OnFilterModeChanged()
//------------------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int instFiltermode = m_CbnFilterMode.GetItemData(m_CbnFilterMode.GetCurSel());

		if(pIns->nFilterMode != instFiltermode)
		{
			pIns->nFilterMode = instFiltermode;
			SetModified(InstrumentHint().Info(), false);

			//Update channel settings where this instrument is active, if required.
			if(instFiltermode != FLTMODE_UNCHANGED)
			{
				for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
				{
					if(m_sndFile.m_PlayState.Chn[i].pModInstrument == pIns)
						m_sndFile.m_PlayState.Chn[i].nFilterMode = instFiltermode;
				}
			}
		}

	}
}


void CCtrlInstruments::OnVScroll(UINT nCode, UINT nPos, CScrollBar *pSB)
//----------------------------------------------------------------------
{
	// Give focus back to envelope editor when stopping to scroll spin buttons (for instrument preview keyboard focus)
	CModControlDlg::OnVScroll(nCode, nPos, pSB);
	if (nCode == SB_ENDSCROLL) SwitchToView();
}


void CCtrlInstruments::OnHScroll(UINT nCode, UINT nPos, CScrollBar *pSB)
//----------------------------------------------------------------------
{
	CModControlDlg::OnHScroll(nCode, nPos, pSB);
	if ((m_nInstrument) && (!IsLocked()) && (nCode != SB_ENDSCROLL))
	{
		ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];

		if (pIns)
		{
			CSliderCtrl* pSlider = (CSliderCtrl*) pSB;
			short int n;
			bool filterChanger = false;

			// Volume ramping (attack)
			if (pSlider == &m_SliderAttack)
			{
				n = m_SliderAttack.GetPos();
				if(pIns->nVolRampUp != n)
				{
					pIns->nVolRampUp = n;
					SetDlgItemInt(IDC_EDIT2,n);
					SetModified(InstrumentHint().Info(), false);
				}
			}
			// Volume Swing
			else if (pSlider == &m_SliderVolSwing)
			{
				n = m_SliderVolSwing.GetPos();
				if ((n >= 0) && (n <= 100) && (n != (int)pIns->nVolSwing))
				{
					pIns->nVolSwing = (uint8)n;
					SetModified(InstrumentHint().Info(), false);
				}
			}
			// Pan Swing
			else if (pSlider == &m_SliderPanSwing)
			{
				n = m_SliderPanSwing.GetPos();
				if ((n >= 0) && (n <= 64) && (n != (int)pIns->nPanSwing))
				{
					pIns->nPanSwing = (uint8)n;
					SetModified(InstrumentHint().Info(), false);
				}
			}
			//Cutoff swing
			else if (pSlider == &m_SliderCutSwing)
			{
				n = m_SliderCutSwing.GetPos();
				if ((n >= 0) && (n <= 64) && (n != (int)pIns->nCutSwing))
				{
					pIns->nCutSwing = (uint8)n;
					SetModified(InstrumentHint().Info(), false);
				}
			}
			//Resonance swing
			else if (pSlider == &m_SliderResSwing)
			{
				n = m_SliderResSwing.GetPos();
				if ((n >= 0) && (n <= 64) && (n != (int)pIns->nResSwing))
				{
					pIns->nResSwing = (uint8)n;
					SetModified(InstrumentHint().Info(), false);
				}
			}
			// Filter CutOff
			else if (pSlider == &m_SliderCutOff)
			{
				n = m_SliderCutOff.GetPos();
				if ((n >= 0) && (n < 0x80) && (n != (int)(pIns->GetCutoff())))
				{
					pIns->SetCutoff(static_cast<uint8>(n), pIns->IsCutoffEnabled());
					SetModified(InstrumentHint().Info(), false);
					UpdateFilterText();
					filterChanger = true;
				}
			}
			else if (pSlider == &m_SliderResonance)
			{
				// Filter Resonance
				n = m_SliderResonance.GetPos();
				if ((n >= 0) && (n < 0x80) && (n != (int)(pIns->GetResonance())))
				{
					pIns->SetResonance(static_cast<uint8>(n), pIns->IsResonanceEnabled());
					SetModified(InstrumentHint().Info(), false);
					UpdateFilterText();
					filterChanger = true;
				}
			}

			// Update channels
			if (filterChanger)
			{
				for (CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
				{
					if (m_sndFile.m_PlayState.Chn[i].pModInstrument == pIns)
					{
						if (pIns->IsCutoffEnabled()) m_sndFile.m_PlayState.Chn[i].nCutOff = pIns->GetCutoff();
						if (pIns->IsResonanceEnabled()) m_sndFile.m_PlayState.Chn[i].nResonance = pIns->GetResonance();
					}
				}
			}
		}
	}
	if ((nCode == SB_ENDSCROLL) || (nCode == SB_THUMBPOSITION))
	{
		SwitchToView();
	}

}


void CCtrlInstruments::OnEditSampleMap()
//--------------------------------------
{
	if(m_nInstrument)
	{
		ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
		if (pIns)
		{
			CSampleMapDlg dlg(m_sndFile, m_nInstrument, this);
			if (dlg.DoModal() == IDOK)
			{
				SetModified(InstrumentHint().Info(), true);
				m_NoteMap.InvalidateRect(NULL, FALSE);
			}
		}
	}
}


void CCtrlInstruments::TogglePluginEditor()
//----------------------------------------
{
	if(m_nInstrument)
	{
		m_modDoc.TogglePluginEditor(m_CbnMixPlug.GetItemData(m_CbnMixPlug.GetCurSel()) - 1, CMainFrame::GetInputHandler()->ShiftPressed());
	}
}


BOOL CCtrlInstruments::PreTranslateMessage(MSG *pMsg)
//-----------------------------------------------
{
	if (pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) ||
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler* ih = CMainFrame::GetInputHandler();

			//Translate message manually
			UINT nChar = pMsg->wParam;
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);
			InputTargetContext ctx = (InputTargetContext)(kCtxCtrlInstruments);

			if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT) != kcNull)
				return true; // Mapped to a command, no need to pass message on.
		}

	}

	return CModControlDlg::PreTranslateMessage(pMsg);
}

LRESULT CCtrlInstruments::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
//--------------------------------------------------------------------
{
	switch(wParam)
	{
		case kcInstrumentCtrlLoad: OnInstrumentOpen(); return wParam;
		case kcInstrumentCtrlSave: OnInstrumentSave(); return wParam;
		case kcInstrumentCtrlNew:  InsertInstrument(false); return wParam;
		case kcInstrumentCtrlDuplicate:	InsertInstrument(true); return wParam;
	}

	return 0;
}


void CCtrlInstruments::OnCbnSelchangeCombotuning()
//------------------------------------------------
{
	if (IsLocked()) return;

	ModInstrument* pInstH = m_sndFile.Instruments[m_nInstrument];
	if(pInstH == 0)
		return;

	size_t sel = m_ComboTuning.GetCurSel();
	if(sel == 0) //Setting IT behavior
	{
		CriticalSection cs;
		pInstH->SetTuning(NULL);
		cs.Leave();

		SetModified(InstrumentHint(m_nInstrument).Info(), true);
		return;
	}

	sel -= 1;
	CTuningCollection* tc = 0;

	if(sel < m_sndFile.GetBuiltInTunings().GetNumTunings())
		tc = &m_sndFile.GetBuiltInTunings();
	else
	{
		sel -= m_sndFile.GetBuiltInTunings().GetNumTunings();
		if(sel < CSoundFile::GetLocalTunings().GetNumTunings())
			tc = &CSoundFile::GetLocalTunings();
		else
		{
			sel -= CSoundFile::GetLocalTunings().GetNumTunings();
			if(sel < m_sndFile.GetTuneSpecificTunings().GetNumTunings())
				tc = &m_sndFile.GetTuneSpecificTunings();
		}
	}

	if(tc)
	{
		CriticalSection cs;
		pInstH->SetTuning(&tc->GetTuning(sel));
		cs.Leave();

		SetModified(InstrumentHint(m_nInstrument).Info(), true);
		return;
	}

	//Case: Chosen tuning editor to be displayed.
	//Creating vector for the CTuningDialog.
	std::vector<CTuningCollection*> v;
	v.push_back(&m_sndFile.GetBuiltInTunings());
	v.push_back(&m_sndFile.GetLocalTunings());
	v.push_back(&m_sndFile.GetTuneSpecificTunings());
	CTuningDialog td(this, v, pInstH->pTuning);
	td.DoModal();
	if(td.GetModifiedStatus(&m_sndFile.GetLocalTunings()))
	{
		if(MsgBox(IDS_APPLY_TUNING_MODIFICATIONS, this, "", MB_OKCANCEL) == IDOK)
			m_sndFile.SaveStaticTunings();
	}
	if(td.GetModifiedStatus(&m_sndFile.GetTuneSpecificTunings()))
	{
		m_modDoc.SetModified();
	}

	//Recreating tuning combobox so that possible
	//new tuning(s) come visible.
	BuildTuningComboBox();

	m_modDoc.UpdateAllViews(nullptr, InstrumentHint(m_nInstrument).Info());
}


void CCtrlInstruments::UpdateTuningComboBox()
//-------------------------------------------
{
	if(m_nInstrument > m_sndFile.GetNumInstruments()
		|| m_sndFile.Instruments[m_nInstrument] == nullptr) return;

	ModInstrument* const pIns = m_sndFile.Instruments[m_nInstrument];
	if(pIns->pTuning == nullptr)
	{
		m_ComboTuning.SetCurSel(0);
		return;
	}

	for(size_t i = 0; i < m_sndFile.GetBuiltInTunings().GetNumTunings(); i++)
	{
		if(pIns->pTuning == &m_sndFile.GetBuiltInTunings().GetTuning(i))
		{
			m_ComboTuning.SetCurSel((int)(i + 1));
			return;
		}
	}

	for(size_t i = 0; i < CSoundFile::GetLocalTunings().GetNumTunings(); i++)
	{
		if(pIns->pTuning == &CSoundFile::GetLocalTunings().GetTuning(i))
		{
			m_ComboTuning.SetCurSel((int)(i + m_sndFile.GetBuiltInTunings().GetNumTunings() + 1));
			return;
		}
	}

	for(size_t i = 0; i < m_sndFile.GetTuneSpecificTunings().GetNumTunings(); i++)
	{
		if(pIns->pTuning == &m_sndFile.GetTuneSpecificTunings().GetTuning(i))
		{
			m_ComboTuning.SetCurSel((int)(i + m_sndFile.GetBuiltInTunings().GetNumTunings() + CSoundFile::GetLocalTunings().GetNumTunings() + 1));
			return;
		}
	}

	CString str;
	str.Format(_T("Tuning %s was not found. Setting to default tuning."), m_sndFile.Instruments[m_nInstrument]->pTuning->GetName().c_str());
	Reporting::Notification(str);

	CriticalSection cs;
	pIns->SetTuning(m_sndFile.GetDefaultTuning());

	m_modDoc.SetModified();
}


void CCtrlInstruments::OnPluginVelocityHandlingChanged()
//------------------------------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if(!IsLocked() && pIns != nullptr)
	{
		uint8 n = static_cast<uint8>(velocityStyle.GetCheck() != BST_UNCHECKED ? PLUGIN_VELOCITYHANDLING_CHANNEL : PLUGIN_VELOCITYHANDLING_VOLUME);
		if(n != pIns->nPluginVelocityHandling)
		{
			if(n == PLUGIN_VELOCITYHANDLING_VOLUME && m_CbnPluginVolumeHandling.GetCurSel() == PLUGIN_VOLUMEHANDLING_IGNORE)
			{
				// This combination doesn't make sense.
				m_CbnPluginVolumeHandling.SetCurSel(PLUGIN_VOLUMEHANDLING_MIDI);
			}

			pIns->nPluginVelocityHandling = n;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnPluginVolumeHandlingChanged()
//----------------------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if(!IsLocked() && pIns != nullptr)
	{
		uint8 n = static_cast<uint8>(m_CbnPluginVolumeHandling.GetCurSel());
		if(n != pIns->nPluginVolumeHandling)
		{

			if(velocityStyle.GetCheck() == BST_UNCHECKED && n == PLUGIN_VOLUMEHANDLING_IGNORE)
			{
				// This combination doesn't make sense.
				velocityStyle.SetCheck(BST_CHECKED);
			}

			pIns->nPluginVolumeHandling = n;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnPitchWheelDepthChanged()
//-----------------------------------------------
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if(!IsLocked() && pIns != nullptr)
	{
		int pwd = GetDlgItemInt(IDC_PITCHWHEELDEPTH, NULL, TRUE);
		int lower = -128, upper = 127;
		m_SpinPWD.GetRange32(lower, upper);
		Limit(pwd, lower, upper);
		if(pwd != pIns->midiPWD)
		{
			pIns->midiPWD = static_cast<int8>(pwd);
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnBnClickedCheckPitchtempolock()
//-----------------------------------------------------
{
	if(IsLocked() || !m_nInstrument) return;

	INSTRUMENTINDEX firstIns = m_nInstrument, lastIns = m_nInstrument;
	if(CMainFrame::GetInputHandler()->ShiftPressed())
	{
		firstIns = 1;
		lastIns = m_sndFile.GetNumInstruments();
	}

	m_EditPitchTempoLock.EnableWindow(IsDlgButtonChecked(IDC_CHECK_PITCHTEMPOLOCK));
	if(IsDlgButtonChecked(IDC_CHECK_PITCHTEMPOLOCK))
	{
		//Checking what value to put for the wPitchToTempoLock.
		TEMPO ptl;
		if(m_EditPitchTempoLock.GetWindowTextLength() > 0)
		{
			ptl = m_EditPitchTempoLock.GetTempoValue();
		}
		if(!ptl.GetRaw())
		{
			ptl = m_sndFile.m_nDefaultTempo;
		}
		m_EditPitchTempoLock.SetTempoValue(ptl);

		for(INSTRUMENTINDEX i = firstIns; i <= lastIns; i++)
		{
			if(m_sndFile.Instruments[i] != nullptr && m_sndFile.Instruments[i]->pitchToTempoLock.GetRaw() == 0)
			{
				m_sndFile.Instruments[i]->pitchToTempoLock = ptl;
				m_modDoc.SetModified();
			}
		}
	} else
	{
		for(INSTRUMENTINDEX i = firstIns; i <= lastIns; i++)
		{
			if(m_sndFile.Instruments[i] != nullptr && m_sndFile.Instruments[i]->pitchToTempoLock.GetRaw() != 0)
			{
				m_sndFile.Instruments[i]->pitchToTempoLock.Set(0);
				m_modDoc.SetModified();
			}
		}
	}
	m_modDoc.UpdateAllViews(nullptr, InstrumentHint().Info(), this);
}


void CCtrlInstruments::OnEnChangeEditPitchtempolock()
//----------------------------------------------------
{
	if(IsLocked() || !m_nInstrument || !m_sndFile.Instruments[m_nInstrument]) return;

	TEMPO ptlTempo = m_EditPitchTempoLock.GetTempoValue();
	Limit(ptlTempo, m_sndFile.GetModSpecifications().GetTempoMin(), m_sndFile.GetModSpecifications().GetTempoMax());

	m_sndFile.Instruments[m_nInstrument]->pitchToTempoLock = ptlTempo;
	m_modDoc.SetModified();	// Only update other views after killing focus
}


void CCtrlInstruments::OnEnKillfocusEditPitchtempolock()
//------------------------------------------------------
{
	//Checking that tempo value is in correct range.
	if(IsLocked()) return;

	TEMPO ptlTempo = m_EditPitchTempoLock.GetTempoValue();
	bool changed = false;
	const CModSpecifications& specs = m_sndFile.GetModSpecifications();

	if(ptlTempo < specs.GetTempoMin())
	{
		ptlTempo = specs.GetTempoMin();
		changed = true;
	} else if(ptlTempo > specs.GetTempoMax())
	{
		ptlTempo = specs.GetTempoMax();
		changed = true;
	}
	if(changed)
	{
		m_EditPitchTempoLock.SetTempoValue(ptlTempo);
		m_modDoc.SetModified();
	}
	m_modDoc.UpdateAllViews(nullptr, InstrumentHint().Info(), this);
}


void CCtrlInstruments::OnEnKillfocusEditFadeOut()
//-----------------------------------------------
{
	if(IsLocked() || !m_nInstrument || !m_sndFile.Instruments[m_nInstrument]) return;

	if(m_modDoc.GetModType() == MOD_TYPE_IT)
	{
		BOOL success;
		uint32 fadeout = (GetDlgItemInt(IDC_EDIT7, &success, FALSE) + 16) & ~31;
		if(success && fadeout != m_sndFile.Instruments[m_nInstrument]->nFadeOut)
		{
			SetDlgItemInt(IDC_EDIT7, fadeout, FALSE);
		}
	}
}


void CCtrlInstruments::BuildTuningComboBox()
//------------------------------------------
{
	m_ComboTuning.ResetContent();

	m_ComboTuning.AddString("OpenMPT IT behaviour"); //<-> Instrument pTuning pointer == NULL
	for(size_t i = 0; i<m_sndFile.GetBuiltInTunings().GetNumTunings(); i++)
	{
		m_ComboTuning.AddString(m_sndFile.GetBuiltInTunings().GetTuning(i).GetName().c_str());
	}
	for(size_t i = 0; i<CSoundFile::GetLocalTunings().GetNumTunings(); i++)
	{
		m_ComboTuning.AddString(CSoundFile::GetLocalTunings().GetTuning(i).GetName().c_str());
	}
	for(size_t i = 0; i<m_sndFile.GetTuneSpecificTunings().GetNumTunings(); i++)
	{
		m_ComboTuning.AddString(m_sndFile.GetTuneSpecificTunings().GetTuning(i).GetName().c_str());
	}
	m_ComboTuning.AddString("Control Tunings...");
	m_ComboTuning.SetCurSel(0);
}


void CCtrlInstruments::UpdatePluginList()
//---------------------------------------
{
	m_CbnMixPlug.SetRedraw(FALSE);
	m_CbnMixPlug.Clear();
	m_CbnMixPlug.ResetContent();
#ifndef NO_PLUGINS
	CHAR s[64];
	for (PLUGINDEX nPlug = 0; nPlug <= MAX_MIXPLUGINS; nPlug++)
	{
		if(!nPlug)
		{
			strcpy(s, "No plugin");
		}
		else
		{
			const SNDMIXPLUGIN &plugin = m_sndFile.m_MixPlugins[nPlug - 1];
			if(plugin.IsValidPlugin())
				wsprintf(s, "FX%d: %s", nPlug, plugin.GetName());
			else
				wsprintf(s, "FX%d: undefined", nPlug);
		}

		m_CbnMixPlug.SetItemData(m_CbnMixPlug.AddString(s), nPlug);
	}
#endif // NO_PLUGINS
	m_CbnMixPlug.SetRedraw(TRUE);
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((pIns) && (pIns->nMixPlug <= MAX_MIXPLUGINS)) m_CbnMixPlug.SetCurSel(pIns->nMixPlug);
}


void CCtrlInstruments::OnXButtonUp(UINT nFlags, UINT nButton, CPoint point)
//-------------------------------------------------------------------------
{
	if(nButton == XBUTTON1) OnPrevInstrument();
	else if(nButton == XBUTTON2) OnNextInstrument();
	CModControlDlg::OnXButtonUp(nFlags, nButton, point);
}


OPENMPT_NAMESPACE_END
