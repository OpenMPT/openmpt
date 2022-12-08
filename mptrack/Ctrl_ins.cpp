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
#include "TuningDialog.h"
#include "../common/misc_util.h"
#include "../common/mptStringBuffer.h"
#include "SelectPluginDialog.h"
#include "mpt/io_file/inputfile.hpp"
#include "mpt/io_file_read/inputfile_filecursor.hpp"
#include "mpt/io_file/outputfile.hpp"
#include "../common/mptFileIO.h"
#include "../common/FileReader.h"
#include "FileDialog.h"


OPENMPT_NAMESPACE_BEGIN


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
	ON_COMMAND(ID_NOTEMAP_TRANS_UP,          &CNoteMapWnd::OnMapTransposeUp)
	ON_COMMAND(ID_NOTEMAP_TRANS_DOWN,        &CNoteMapWnd::OnMapTransposeDown)
	ON_COMMAND(ID_NOTEMAP_COPY_NOTE,         &CNoteMapWnd::OnMapCopyNote)
	ON_COMMAND(ID_NOTEMAP_COPY_SMP,          &CNoteMapWnd::OnMapCopySample)
	ON_COMMAND(ID_NOTEMAP_RESET,             &CNoteMapWnd::OnMapReset)
	ON_COMMAND(ID_NOTEMAP_TRANSPOSE_SAMPLES, &CNoteMapWnd::OnTransposeSamples)
	ON_COMMAND(ID_NOTEMAP_REMOVE,            &CNoteMapWnd::OnMapRemove)
	ON_COMMAND(ID_INSTRUMENT_SAMPLEMAP,      &CNoteMapWnd::OnEditSampleMap)
	ON_COMMAND(ID_INSTRUMENT_DUPLICATE,      &CNoteMapWnd::OnInstrumentDuplicate)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,            &CNoteMapWnd::OnCustomKeyMsg)
	ON_COMMAND_RANGE(ID_NOTEMAP_EDITSAMPLE, ID_NOTEMAP_EDITSAMPLE + MAX_SAMPLES, &CNoteMapWnd::OnEditSample)
END_MESSAGE_MAP()


BOOL CNoteMapWnd::PreTranslateMessage(MSG *pMsg)
{
	if(!pMsg)
		return TRUE;
	uint32 wParam = static_cast<uint32>(pMsg->wParam);

	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) ||
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler *ih = CMainFrame::GetInputHandler();
			const auto event = ih->Translate(*pMsg);

			if (ih->KeyEvent(kCtxInsNoteMap, event) != kcNull)
				return true; // Mapped to a command, no need to pass message on.

			// a bit of a hack...
			if (ih->KeyEvent(kCtxCtrlInstruments, event) != kcNull)
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
		if(HandleNav(wParam))
			return true;

		// Handle Application (menu) key
		if(wParam == VK_APPS)
		{
			CRect clientRect;
			GetClientRect(clientRect);
			clientRect.bottom = clientRect.top + mpt::align_up(clientRect.Height(), m_cyFont);
			OnRButtonDown(0, clientRect.CenterPoint());
		}
	}
	else if (pMsg->message == WM_KEYUP) //stop notes on key release
	{
		if (((pMsg->wParam >= '0') && (pMsg->wParam <= '9')) || (pMsg->wParam == ' ') ||
			((pMsg->wParam >= VK_NUMPAD0) && (pMsg->wParam <= VK_NUMPAD9)))
		{
			StopNote();
			return true;
		}
	}

	return CStatic::PreTranslateMessage(pMsg);
}


void CNoteMapWnd::PrepareUndo(const char *description)
{
	m_modDoc.GetInstrumentUndo().PrepareUndo(m_nInstrument, description);
}


void CNoteMapWnd::SetCurrentInstrument(INSTRUMENTINDEX nIns)
{
	if (nIns != m_nInstrument)
	{
		if (nIns < MAX_INSTRUMENTS) m_nInstrument = nIns;

		// create missing instrument if needed
		CSoundFile &sndFile = m_modDoc.GetSoundFile();
		if(m_nInstrument > 0 && m_nInstrument <= sndFile.GetNumInstruments() && sndFile.Instruments[m_nInstrument] == nullptr)
		{
			ModInstrument *instrument = sndFile.AllocateInstrument(m_nInstrument);
			if(instrument == nullptr)
				return;
			m_modDoc.InitializeInstrument(instrument);
		}

		Invalidate(FALSE);
		UpdateAccessibleTitle();
	}
}


void CNoteMapWnd::SetCurrentNote(UINT nNote)
{
	if(nNote != m_nNote && ModCommand::IsNote(static_cast<ModCommand::NOTE>(nNote + NOTE_MIN)))
	{
		m_nNote = nNote;
		Invalidate(FALSE);
		UpdateAccessibleTitle();
	}
}


void CNoteMapWnd::OnPaint()
{
	CPaintDC dc(this);

	CRect rcClient;
	GetClientRect(&rcClient);
	const auto highlightBrush = GetSysColorBrush(COLOR_HIGHLIGHT), windowBrush = GetSysColorBrush(COLOR_WINDOW);
	const auto colorText = GetSysColor(COLOR_WINDOWTEXT);
	const auto colorTextSel = GetSysColor(COLOR_HIGHLIGHTTEXT);
	auto oldFont = dc.SelectObject(CMainFrame::GetGUIFont());
	dc.SetBkMode(TRANSPARENT);
	if ((m_cxFont <= 0) || (m_cyFont <= 0))
	{
		CSize sz;
		sz = dc.GetTextExtent(_T("C#0."), 4);
		m_cyFont = sz.cy + 2;
		m_cxFont = rcClient.right / 3;
	}
	dc.IntersectClipRect(&rcClient);

	const CSoundFile &sndFile = m_modDoc.GetSoundFile();
	if (m_cxFont > 0 && m_cyFont > 0)
	{
		const bool focus = (::GetFocus() == m_hWnd);
		const ModInstrument *pIns = sndFile.Instruments[m_nInstrument];
		CRect rect;

		int nNotes = (rcClient.bottom + m_cyFont - 1) / m_cyFont;
		int nPos = m_nNote - (nNotes/2);
		int ypaint = 0;
		mpt::winstring s;
		for (int ynote=0; ynote<nNotes; ynote++, ypaint+=m_cyFont, nPos++)
		{
			// Note
			bool isValidPos = (nPos >= 0) && (nPos < NOTE_MAX - NOTE_MIN + 1);
			if (isValidPos)
			{
				s = mpt::ToWin(sndFile.GetNoteName(static_cast<ModCommand::NOTE>(nPos + 1), m_nInstrument));
				s.resize(4);
			} else
			{
				s.clear();
			}
			rect.SetRect(0, ypaint, m_cxFont, ypaint+m_cyFont);
			DrawButtonRect(dc, &rect, s.c_str(), FALSE, FALSE);
			// Mapped Note
			bool highlight = ((focus) && (nPos == (int)m_nNote));
			rect.left = rect.right;
			rect.right = m_cxFont*2-1;
			s = _T("...");
			if(pIns != nullptr && isValidPos && (pIns->NoteMap[nPos] != NOTE_NONE))
			{
				ModCommand::NOTE n = pIns->NoteMap[nPos];
				if(ModCommand::IsNote(n))
				{
					s = mpt::ToWin(sndFile.GetNoteName(n, m_nInstrument));
					s.resize(4);
				} else
				{
					s = _T("???");
				}
			}
			FillRect(dc, &rect, highlight ? highlightBrush : windowBrush);
			if(nPos == (int)m_nNote && !m_bIns)
			{
				rect.InflateRect(-1, -1);
				dc.DrawFocusRect(&rect);
				rect.InflateRect(1, 1);
			}
			dc.SetTextColor(highlight ? colorTextSel : colorText);
			dc.DrawText(s.c_str(), -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
			// Sample
			highlight = (focus && nPos == (int)m_nNote);
			rect.left = rcClient.left + m_cxFont * 2 + 3;
			rect.right = rcClient.right;
			s = _T(" ..");
			if(pIns && nPos >= 0 && nPos < NOTE_MAX && pIns->Keyboard[nPos])
			{
				s = mpt::tfmt::right(3, mpt::tfmt::dec(pIns->Keyboard[nPos]));
			}
			FillRect(dc, &rect, highlight ? highlightBrush : windowBrush);
			if((nPos == (int)m_nNote) && (m_bIns))
			{
				rect.InflateRect(-1, -1);
				dc.DrawFocusRect(&rect);
				rect.InflateRect(1, 1);
			}
			dc.SetTextColor((highlight) ? colorTextSel : colorText);
			dc.DrawText(s.c_str(), -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
		}
		rect.SetRect(rcClient.left + m_cxFont * 2 - 1, rcClient.top, rcClient.left + m_cxFont * 2 + 3, ypaint);
		DrawButtonRect(dc, &rect, _T(""), FALSE, FALSE);
		if (ypaint < rcClient.bottom)
		{
			rect.SetRect(rcClient.left, ypaint, rcClient.right, rcClient.bottom);
			FillRect(dc, &rect, GetSysColorBrush(COLOR_BTNFACE));
		}
	}
	dc.SelectObject(oldFont);
}


void CNoteMapWnd::OnSetFocus(CWnd *pOldWnd)
{
	CStatic::OnSetFocus(pOldWnd);
	Invalidate(FALSE);
	CMainFrame::GetMainFrame()->m_pNoteMapHasFocus = this;
	m_undo = true;
}


void CNoteMapWnd::OnKillFocus(CWnd *pNewWnd)
{
	CStatic::OnKillFocus(pNewWnd);
	Invalidate(FALSE);
	CMainFrame::GetMainFrame()->m_pNoteMapHasFocus = nullptr;
}


void CNoteMapWnd::OnLButtonDown(UINT, CPoint pt)
{
	if ((pt.x >= m_cxFont) && (pt.x < m_cxFont*2) && (m_bIns))
	{
		m_bIns = false;
		Invalidate(FALSE);
	}
	if ((pt.x > m_cxFont*2) && (pt.x <= m_cxFont*3) && (!m_bIns))
	{
		m_bIns = true;
		Invalidate(FALSE);
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
{
	// Double-click edits sample map
	OnEditSampleMap();
}


void CNoteMapWnd::OnRButtonDown(UINT, CPoint pt)
{
	CInputHandler* ih = CMainFrame::GetInputHandler();

	CSoundFile &sndFile = m_modDoc.GetSoundFile();
	ModInstrument *pIns = sndFile.Instruments[m_nInstrument];
	if (pIns)
	{
		HMENU hMenu = ::CreatePopupMenu();
		HMENU hSubMenu = ::CreatePopupMenu();

		if (hMenu)
		{
			AppendMenu(hMenu, MF_STRING, ID_INSTRUMENT_SAMPLEMAP, ih->GetKeyTextFromCommand(kcInsNoteMapEditSampleMap, _T("Edit Sample &Map")));
			if (hSubMenu)
			{
				// Create sub menu with a list of all samples that are referenced by this instrument.
				for(auto sample : pIns->GetSamples())
				{
					if(sample <= sndFile.GetNumSamples())
					{
						AppendMenu(hSubMenu, MF_STRING, ID_NOTEMAP_EDITSAMPLE + sample, MPT_CFORMAT("{}: {}")(sample, mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.m_szNames[sample])));
					}
				}

				AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), ih->GetKeyTextFromCommand(kcInsNoteMapEditSample, _T("&Edit Sample")));
				AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
			}
			AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_COPY_SMP, ih->GetKeyTextFromCommand(kcInsNoteMapCopyCurrentSample, MPT_CFORMAT("Map All Notes to &Sample {}")(pIns->Keyboard[m_nNote])));

			if(sndFile.GetType() != MOD_TYPE_XM)
			{
				if(ModCommand::IsNote(pIns->NoteMap[m_nNote]))
				{
					AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_COPY_NOTE, ih->GetKeyTextFromCommand(kcInsNoteMapCopyCurrentNote, MPT_CFORMAT("Map All &Notes to {}")(mpt::ToCString(sndFile.GetNoteName(pIns->NoteMap[m_nNote], m_nInstrument)))));
				}
				AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_TRANS_UP, ih->GetKeyTextFromCommand(kcInsNoteMapTransposeUp, _T("Transpose Map &Up")));
				AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_TRANS_DOWN, ih->GetKeyTextFromCommand(kcInsNoteMapTransposeDown, _T("Transpose Map &Down")));
			}
			AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_RESET, ih->GetKeyTextFromCommand(kcInsNoteMapReset, _T("&Reset Note Mapping")));
			AppendMenu(hMenu, MF_STRING | (pIns->CanConvertToDefaultNoteMap().empty() ? MF_GRAYED : 0), ID_NOTEMAP_TRANSPOSE_SAMPLES, ih->GetKeyTextFromCommand(kcInsNoteMapTransposeSamples, _T("&Transpose Samples / Reset Map")));
			AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_REMOVE, ih->GetKeyTextFromCommand(kcInsNoteMapRemove, _T("Remo&ve All Samples")));
			AppendMenu(hMenu, MF_STRING, ID_INSTRUMENT_DUPLICATE, ih->GetKeyTextFromCommand(kcInstrumentCtrlDuplicate, _T("Duplicate &Instrument")));
			SetMenuDefaultItem(hMenu, ID_INSTRUMENT_SAMPLEMAP, FALSE);
			ClientToScreen(&pt);
			::TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
			::DestroyMenu(hMenu);
			if (hSubMenu) ::DestroyMenu(hSubMenu);
		}
	}
}


BOOL CNoteMapWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	SetCurrentNote(m_nNote - mpt::signum(zDelta));
	return CStatic::OnMouseWheel(nFlags, zDelta, pt);
}


void CNoteMapWnd::OnMapCopyNote()
{
	ModInstrument *pIns = m_modDoc.GetSoundFile().Instruments[m_nInstrument];
	if (pIns)
	{
		m_undo = true;
		bool bModified = false;
		auto n = pIns->NoteMap[m_nNote];
		for (auto &key : pIns->NoteMap) if (key != n)
		{
			if(!bModified)
			{
				PrepareUndo("Map Notes");
			}
			key = n;
			bModified = true;
		}
		if (bModified)
		{
			m_pParent.SetModified(InstrumentHint().Info(), false);
			Invalidate(FALSE);
		}
	}
}

void CNoteMapWnd::OnMapCopySample()
{
	ModInstrument *pIns = m_modDoc.GetSoundFile().Instruments[m_nInstrument];
	if (pIns)
	{
		m_undo = true;
		bool bModified = false;
		auto n = pIns->Keyboard[m_nNote];
		for (auto &sample : pIns->Keyboard) if (sample != n)
		{
			if(!bModified)
			{
				PrepareUndo("Map Samples");
			}
			sample = n;
			bModified = true;
		}
		if (bModified)
		{
			m_pParent.SetModified(InstrumentHint().Info(), false);
			Invalidate(FALSE);
			UpdateAccessibleTitle();
		}
	}
}


void CNoteMapWnd::OnMapReset()
{
	ModInstrument *pIns = m_modDoc.GetSoundFile().Instruments[m_nInstrument];
	if (pIns)
	{
		m_undo = true;
		bool modified = false;
		for (size_t i = 0; i < std::size(pIns->NoteMap); i++) if (pIns->NoteMap[i] != i + 1)
		{
			if(!modified)
			{
				PrepareUndo("Reset Note Map");
			}
			pIns->NoteMap[i] = static_cast<ModCommand::NOTE>(i + 1);
			modified = true;
		}
		if(modified)
		{
			m_pParent.SetModified(InstrumentHint().Info(), false);
			Invalidate(FALSE);
			UpdateAccessibleTitle();
		}
	}
}


void CNoteMapWnd::OnTransposeSamples()
{
	auto &sndFile = m_modDoc.GetSoundFile();
	ModInstrument *pIns = sndFile.Instruments[m_nInstrument];
	if(!pIns)
		return;
	const auto samples = pIns->CanConvertToDefaultNoteMap();
	if(samples.empty())
		return;

	PrepareUndo("Transpose Samples");
	for(const auto &[smp, transpose] : samples)
	{
		if(smp > sndFile.GetNumSamples())
			continue;
		m_modDoc.GetSampleUndo().PrepareUndo(smp, sundo_none, "Transpose");
		auto &sample = sndFile.GetSample(smp);
		if(sndFile.UseFinetuneAndTranspose())
			sample.RelativeTone += transpose;
		else
			sample.Transpose(transpose / 12.0);
		m_modDoc.UpdateAllViews(nullptr, SampleHint(smp).Info(), &m_pParent);
	}
	pIns->ResetNoteMap();
	m_pParent.SetModified(InstrumentHint().Info(), false);
	Invalidate(FALSE);
}


void CNoteMapWnd::OnMapRemove()
{
	ModInstrument *pIns = m_modDoc.GetSoundFile().Instruments[m_nInstrument];
	if (pIns)
	{
		m_undo = true;
		bool modified = false;
		for (auto &sample: pIns->Keyboard) if (sample != 0)
		{
			if(!modified)
			{
				PrepareUndo("Remove Sample Assocations");
			}
			sample = 0;
			modified = true;
		}
		if(modified)
		{
			m_pParent.SetModified(InstrumentHint().Info(), false);
			Invalidate(FALSE);
			UpdateAccessibleTitle();
		}
	}
}


void CNoteMapWnd::OnMapTransposeUp()
{
	MapTranspose(1);
}


void CNoteMapWnd::OnMapTransposeDown()
{
	MapTranspose(-1);
}


void CNoteMapWnd::MapTranspose(int nAmount)
{
	if(nAmount == 0 || m_modDoc.GetModType() == MOD_TYPE_XM) return;

	ModInstrument *pIns = m_modDoc.GetSoundFile().Instruments[m_nInstrument];
	if((nAmount == 12 || nAmount == -12))
	{
		// Special case for instrument-specific tunings
		nAmount = m_modDoc.GetInstrumentGroupSize(m_nInstrument) * mpt::signum(nAmount);
	}

	m_undo = true;
	if (pIns)
	{
		bool modified = false;
		for(NOTEINDEXTYPE i = 0; i < NOTE_MAX; i++)
		{
			int n = pIns->NoteMap[i];
			if ((n > NOTE_MIN && nAmount < 0) || (n < NOTE_MAX && nAmount > 0))
			{
				n = Clamp(n + nAmount, NOTE_MIN, NOTE_MAX);
				if(n != pIns->NoteMap[i])
				{
					if(!modified)
					{
						PrepareUndo("Transpose Map");
					}
					pIns->NoteMap[i] = static_cast<uint8>(n);
					modified = true;
				}
			}
		}
		if(modified)
		{
			m_pParent.SetModified(InstrumentHint().Info(), false);
			Invalidate(FALSE);
			UpdateAccessibleTitle();
		}
	}
}


void CNoteMapWnd::OnEditSample(UINT nID)
{
	UINT nSample = nID - ID_NOTEMAP_EDITSAMPLE;
	m_pParent.EditSample(nSample);
}


void CNoteMapWnd::OnEditSampleMap()
{
	m_undo = true;
	m_pParent.PostMessage(WM_COMMAND, ID_INSTRUMENT_SAMPLEMAP);
}


void CNoteMapWnd::OnInstrumentDuplicate()
{
	m_undo = true;
	m_pParent.PostMessage(WM_COMMAND, ID_INSTRUMENT_DUPLICATE);
}


LRESULT CNoteMapWnd::OnCustomKeyMsg(WPARAM wParam, LPARAM lParam)
{
	ModInstrument *pIns = m_modDoc.GetSoundFile().Instruments[m_nInstrument];

	// Handle notes

	if (wParam >= kcInsNoteMapStartNotes && wParam <= kcInsNoteMapEndNotes)
	{
		// Special case: number keys override notes if we're in the sample # column.
		const auto key = KeyCombination::FromLPARAM(lParam).KeyCode();
		if(m_bIns && ((key >= '0' && key <= '9') || (key == ' ')))
			HandleChar(key);
		else
			EnterNote(m_modDoc.GetNoteWithBaseOctave(static_cast<int>(wParam - kcInsNoteMapStartNotes), m_nInstrument));

		return wParam;
	}

	if (wParam >= kcInsNoteMapStartNoteStops && wParam <= kcInsNoteMapEndNoteStops)
	{
		StopNote();
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
	case kcInsNoteMapTransposeSamples:	OnTransposeSamples(); return wParam;
	case kcInsNoteMapRemove:			OnMapRemove(); return wParam;

	case kcInsNoteMapEditSample:		if(pIns) OnEditSample(pIns->Keyboard[m_nNote] + ID_NOTEMAP_EDITSAMPLE); return wParam;
	case kcInsNoteMapEditSampleMap:		OnEditSampleMap(); return wParam;

	// Parent shortcuts (also displayed in context menu of this control)
	case kcInstrumentCtrlDuplicate:		OnInstrumentDuplicate(); return wParam;
	case kcNextInstrument:				m_pParent.PostMessage(WM_COMMAND, ID_NEXTINSTRUMENT); return wParam;
	case kcPrevInstrument:				m_pParent.PostMessage(WM_COMMAND, ID_PREVINSTRUMENT); return wParam;
	}

	return kcNull;
}

void CNoteMapWnd::EnterNote(UINT note)
{
	CSoundFile &sndFile = m_modDoc.GetSoundFile();
	ModInstrument *pIns = sndFile.Instruments[m_nInstrument];
	if ((pIns) && (m_nNote < NOTE_MAX))
	{
		if (!m_bIns && (sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)))
		{
			UINT n = pIns->NoteMap[m_nNote];
			bool ok = false;
			if ((note >= sndFile.GetModSpecifications().noteMin) && (note <= sndFile.GetModSpecifications().noteMax))
			{
				n = note;
				ok = true;
			}
			if (n != pIns->NoteMap[m_nNote])
			{
				StopNote(); // Stop old note according to current instrument settings
				pIns->NoteMap[m_nNote] = static_cast<ModCommand::NOTE>(n);
				m_pParent.SetModified(InstrumentHint().Info(), false);
				Invalidate(FALSE);
				UpdateAccessibleTitle();
			}
			if(ok)
			{
				PlayNote(m_nNote);
			}
		}
	}
}

bool CNoteMapWnd::HandleChar(WPARAM c)
{
	CSoundFile &sndFile = m_modDoc.GetSoundFile();
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
				if(m_undo)
				{
					PrepareUndo("Enter Instrument");
					m_undo = false;
				}
				StopNote(); // Stop old note according to current instrument settings
				pIns->Keyboard[m_nNote] = static_cast<SAMPLEINDEX>(n);
				m_pParent.SetModified(InstrumentHint().Info(), false);
				Invalidate(FALSE);
				UpdateAccessibleTitle();
				PlayNote(m_nNote);
			}

			if (c == ' ')
			{
				SetCurrentNote(m_nNote + 1);
				PlayNote(m_nNote);
			}
			return true;
		}

		else if ((!m_bIns) && (sndFile.m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)))	//in note column
		{
			uint32 n = pIns->NoteMap[m_nNote];

			if ((c >= '0') && (c <= '9'))
			{
				if (n)
					n = static_cast<uint32>(((n - 1) % 12) + (c - '0') * 12 + 1);
				else
					n = static_cast<uint32>((m_nNote % 12) + (c - '0') * 12 + 1);
			} else if (c == ' ')
			{
				n = (m_nOldNote) ? m_nOldNote : m_nNote+1;
			}

			if (n != pIns->NoteMap[m_nNote])
			{
				if(m_undo)
				{
					PrepareUndo("Enter Note");
					m_undo = false;
				}

				StopNote(); // Stop old note according to current instrument settings
				pIns->NoteMap[m_nNote] = static_cast<ModCommand::NOTE>(n);
				m_pParent.SetModified(InstrumentHint().Info(), false);
				Invalidate(FALSE);
				UpdateAccessibleTitle();
			}

			if(c == ' ')
			{
				SetCurrentNote(m_nNote + 1);
			}

			PlayNote(m_nNote);

			return true;
		}
	}
	return false;
}

bool CNoteMapWnd::HandleNav(WPARAM k)
{
	bool redraw = false;

	//HACK: handle numpad (convert numpad number key to normal number key)
	if ((k >= VK_NUMPAD0) && (k <= VK_NUMPAD9)) return HandleChar(k-VK_NUMPAD0+'0');

	switch(k)
	{
	case VK_RIGHT:
		if (!m_bIns) { m_bIns = true; redraw = true; } else
		if (m_nNote < NOTE_MAX - NOTE_MIN) { m_nNote++; m_bIns = false; redraw = true; }
		break;
	case VK_LEFT:
		if (m_bIns) { m_bIns = false; redraw = true; } else
		if (m_nNote) { m_nNote--; m_bIns = true; redraw = true; }
		break;
	case VK_UP:
		if (m_nNote > 0) { m_nNote--; redraw = true; }
		break;
	case VK_DOWN:
		if (m_nNote < NOTE_MAX - 1) { m_nNote++; redraw = true; }
		break;
	case VK_PRIOR:
		if (m_nNote > 3) { m_nNote -= 3; redraw = true; } else
		if (m_nNote > 0) { m_nNote = 0; redraw = true; }
		break;
	case VK_NEXT:
		if (m_nNote+3 < NOTE_MAX) { m_nNote += 3; redraw = true; } else
		if (m_nNote < NOTE_MAX - NOTE_MIN) { m_nNote = NOTE_MAX - NOTE_MIN; redraw = true; }
		break;
	case VK_HOME:
		if(m_nNote > 0) { m_nNote = 0; redraw = true; }
		break;
	case VK_END:
		if(m_nNote < NOTE_MAX - NOTE_MIN) { m_nNote = NOTE_MAX - NOTE_MIN; redraw = true; }
		break;
// 	case VK_TAB:
// 		return true;
	case VK_RETURN:
		{
			ModInstrument *pIns = m_modDoc.GetSoundFile().Instruments[m_nInstrument];
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
	if(redraw)
	{
		m_undo = true;
		Invalidate(FALSE);
		UpdateAccessibleTitle();
	}

	return true;
}


void CNoteMapWnd::PlayNote(UINT note)
{
	if(m_nPlayingNote != NOTE_NONE)
	{
		// No polyphony in notemap window
		StopNote();
	}
	m_nPlayingNote = static_cast<ModCommand::NOTE>(note + NOTE_MIN);
	m_noteChannel = m_modDoc.PlayNote(PlayNoteParam(m_nPlayingNote).Instrument(m_nInstrument));
}


void CNoteMapWnd::StopNote()
{
	if(!ModCommand::IsNote(m_nPlayingNote)) return;

	m_modDoc.NoteOff(m_nPlayingNote, true, m_nInstrument, m_noteChannel);
	m_nPlayingNote = NOTE_NONE;
}


void CNoteMapWnd::UpdateAccessibleTitle()
{
	CMainFrame::GetMainFrame()->NotifyAccessibilityUpdate(*this);
}


// Accessible description for screen readers
HRESULT CNoteMapWnd::get_accName(VARIANT varChild, BSTR *pszName)
{
	const auto *ins = m_modDoc.GetSoundFile().Instruments[m_nInstrument];
	if(!ins || m_nNote >= std::size(ins->NoteMap))
		return CStatic::get_accName(varChild, pszName);

	const auto &sndFile = m_modDoc.GetSoundFile();
	CString str = mpt::ToCString(sndFile.GetNoteName(static_cast<ModCommand::NOTE>(m_nNote + NOTE_MIN), m_nInstrument)) + _T(": ");
	if(ins->Keyboard[m_nNote] == 0)
	{
		str += _T("no sample");
	} else
	{
		auto mappedNote = ins->NoteMap[m_nNote];
		str += MPT_CFORMAT("sample {} at {}")(ins->Keyboard[m_nNote], mpt::ToCString(sndFile.GetNoteName(mappedNote, m_nInstrument)));
	}

	*pszName = str.AllocSysString();
	return S_OK;
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
	ON_NOTIFY(TBN_DROPDOWN, IDC_TOOLBAR1, &CCtrlInstruments::OnTbnDropDownToolBar)
	ON_COMMAND(IDC_INSTRUMENT_NEW,		&CCtrlInstruments::OnInstrumentNew)
	ON_COMMAND(IDC_INSTRUMENT_OPEN,		&CCtrlInstruments::OnInstrumentOpen)
	ON_COMMAND(IDC_INSTRUMENT_SAVEAS,	&CCtrlInstruments::OnInstrumentSave)
	ON_COMMAND(IDC_SAVE_ONE,			&CCtrlInstruments::OnInstrumentSaveOne)
	ON_COMMAND(IDC_SAVE_ALL,			&CCtrlInstruments::OnInstrumentSaveAll)
	ON_COMMAND(IDC_INSTRUMENT_PLAY,		&CCtrlInstruments::OnInstrumentPlay)
	ON_COMMAND(ID_PREVINSTRUMENT,		&CCtrlInstruments::OnPrevInstrument)
	ON_COMMAND(ID_NEXTINSTRUMENT,		&CCtrlInstruments::OnNextInstrument)
	ON_COMMAND(ID_INSTRUMENT_DUPLICATE, &CCtrlInstruments::OnInstrumentDuplicate)
	ON_COMMAND(IDC_CHECK1,				&CCtrlInstruments::OnSetPanningChanged)
	ON_COMMAND(IDC_CHECK2,				&CCtrlInstruments::OnEnableCutOff)
	ON_COMMAND(IDC_CHECK3,				&CCtrlInstruments::OnEnableResonance)
	ON_COMMAND(IDC_INSVIEWPLG,			&CCtrlInstruments::TogglePluginEditor)
	ON_EN_CHANGE(IDC_EDIT_INSTRUMENT,	&CCtrlInstruments::OnInstrumentChanged)
	ON_EN_CHANGE(IDC_SAMPLE_NAME,		&CCtrlInstruments::OnNameChanged)
	ON_EN_CHANGE(IDC_SAMPLE_FILENAME,	&CCtrlInstruments::OnFileNameChanged)
	ON_EN_CHANGE(IDC_EDIT7,				&CCtrlInstruments::OnFadeOutVolChanged)
	ON_EN_CHANGE(IDC_EDIT8,				&CCtrlInstruments::OnGlobalVolChanged)
	ON_EN_CHANGE(IDC_EDIT9,				&CCtrlInstruments::OnPanningChanged)
	ON_EN_CHANGE(IDC_EDIT10,			&CCtrlInstruments::OnMPRChanged)
	ON_EN_KILLFOCUS(IDC_EDIT10,			&CCtrlInstruments::OnMPRKillFocus)
	ON_EN_CHANGE(IDC_EDIT11,			&CCtrlInstruments::OnMBKChanged)
	ON_EN_CHANGE(IDC_EDIT15,			&CCtrlInstruments::OnPPSChanged)
	ON_EN_CHANGE(IDC_PITCHWHEELDEPTH,	&CCtrlInstruments::OnPitchWheelDepthChanged)
	ON_EN_CHANGE(IDC_EDIT2,				&CCtrlInstruments::OnAttackChanged)

	ON_EN_SETFOCUS(IDC_SAMPLE_NAME,		&CCtrlInstruments::OnEditFocus)
	ON_EN_SETFOCUS(IDC_SAMPLE_FILENAME,	&CCtrlInstruments::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT7,			&CCtrlInstruments::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT8,			&CCtrlInstruments::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT9,			&CCtrlInstruments::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT10,			&CCtrlInstruments::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT11,			&CCtrlInstruments::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT15,			&CCtrlInstruments::OnEditFocus)
	ON_EN_SETFOCUS(IDC_PITCHWHEELDEPTH,	&CCtrlInstruments::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT2,			&CCtrlInstruments::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT_PITCHTEMPOLOCK, &CCtrlInstruments::OnEditFocus)

	ON_CBN_SELCHANGE(IDC_COMBO1,		&CCtrlInstruments::OnNNAChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,		&CCtrlInstruments::OnDCTChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,		&CCtrlInstruments::OnDCAChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,		&CCtrlInstruments::OnPPCChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5,		&CCtrlInstruments::OnMCHChanged)
	ON_CBN_SELCHANGE(IDC_COMBO6,		&CCtrlInstruments::OnMixPlugChanged)
	ON_CBN_DROPDOWN(IDC_COMBO6,			&CCtrlInstruments::OnOpenPluginList)
	ON_CBN_SELCHANGE(IDC_COMBO9,		&CCtrlInstruments::OnResamplingChanged)
	ON_CBN_SELCHANGE(IDC_FILTERMODE,	&CCtrlInstruments::OnFilterModeChanged)
	ON_CBN_SELCHANGE(IDC_PLUGIN_VOLUMESTYLE,	&CCtrlInstruments::OnPluginVolumeHandlingChanged)
	ON_COMMAND(IDC_PLUGIN_VELOCITYSTYLE,		&CCtrlInstruments::OnPluginVelocityHandlingChanged)
	ON_COMMAND(ID_INSTRUMENT_SAMPLEMAP,			&CCtrlInstruments::OnEditSampleMap)
	ON_CBN_SELCHANGE(IDC_COMBOTUNING, &CCtrlInstruments::OnCbnSelchangeCombotuning)
	ON_EN_CHANGE(IDC_EDIT_PITCHTEMPOLOCK, &CCtrlInstruments::OnEnChangeEditPitchTempoLock)
	ON_BN_CLICKED(IDC_CHECK_PITCHTEMPOLOCK, &CCtrlInstruments::OnBnClickedCheckPitchtempolock)
	ON_EN_KILLFOCUS(IDC_EDIT_PITCHTEMPOLOCK, &CCtrlInstruments::OnEnKillFocusEditPitchTempoLock)
	ON_EN_KILLFOCUS(IDC_EDIT7, &CCtrlInstruments::OnEnKillFocusEditFadeOut)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CCtrlInstruments::DoDataExchange(CDataExchange* pDX)
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


CCtrlInstruments::CCtrlInstruments(CModControlView &parent, CModDoc &document)
	: CModControlDlg(parent, document)
	, m_NoteMap(*this, document)
{
	m_nLockCount = 1;
}


CRuntimeClass *CCtrlInstruments::GetAssociatedViewClass()
{
	return RUNTIME_CLASS(CViewInstrument);
}


void CCtrlInstruments::OnEditFocus()
{
	m_startedEdit = false;
}


BOOL CCtrlInstruments::OnInitDialog()
{
	CModControlDlg::OnInitDialog();
	m_bInitialized = FALSE;
	SetRedraw(FALSE);

	m_ToolBar.SetExtendedStyle(m_ToolBar.GetExtendedStyle() | TBSTYLE_EX_DRAWDDARROWS);
	m_ToolBar.Init(CMainFrame::GetMainFrame()->m_PatternIcons,CMainFrame::GetMainFrame()->m_PatternIconsDisabled);
	m_ToolBar.AddButton(IDC_INSTRUMENT_NEW, TIMAGE_INSTR_NEW, TBSTYLE_BUTTON | TBSTYLE_DROPDOWN);
	m_ToolBar.AddButton(IDC_INSTRUMENT_OPEN, TIMAGE_OPEN);
	m_ToolBar.AddButton(IDC_INSTRUMENT_SAVEAS, TIMAGE_SAVE, TBSTYLE_BUTTON | TBSTYLE_DROPDOWN);
	m_ToolBar.AddButton(IDC_INSTRUMENT_PLAY, TIMAGE_PREVIEW);
	m_SpinInstrument.SetRange(0, 0);
	m_SpinInstrument.EnableWindow(FALSE);
	// NNA
	m_ComboNNA.AddString(_T("Note Cut"));
	m_ComboNNA.AddString(_T("Continue"));
	m_ComboNNA.AddString(_T("Note Off"));
	m_ComboNNA.AddString(_T("Note Fade"));
	// DCT
	m_ComboDCT.AddString(_T("Disabled"));
	m_ComboDCT.AddString(_T("Note"));
	m_ComboDCT.AddString(_T("Sample"));
	m_ComboDCT.AddString(_T("Instrument"));
	m_ComboDCT.AddString(_T("Plugin"));
	// DCA
	m_ComboDCA.AddString(_T("Note Cut"));
	m_ComboDCA.AddString(_T("Note Off"));
	m_ComboDCA.AddString(_T("Note Fade"));
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

	const auto resamplingModes = Resampling::AllModes();
	m_CbnResampling.SetItemData(m_CbnResampling.AddString(_T("Default")), SRCMODE_DEFAULT);
	for(auto mode : resamplingModes)
	{
		m_CbnResampling.SetItemData(m_CbnResampling.AddString(CTrackApp::GetResamplingModeName(mode, 1, false)), mode);
	}

	m_CbnFilterMode.SetItemData(m_CbnFilterMode.AddString(_T("Channel default")), static_cast<DWORD_PTR>(FilterMode::Unchanged));
	m_CbnFilterMode.SetItemData(m_CbnFilterMode.AddString(_T("Force lowpass")), static_cast<DWORD_PTR>(FilterMode::LowPass));
	m_CbnFilterMode.SetItemData(m_CbnFilterMode.AddString(_T("Force highpass")), static_cast<DWORD_PTR>(FilterMode::HighPass));

	//VST velocity/volume handling
	m_CbnPluginVolumeHandling.AddString(_T("MIDI volume"));
	m_CbnPluginVolumeHandling.AddString(_T("Dry/Wet ratio"));
	m_CbnPluginVolumeHandling.AddString(_T("None"));

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
{
}


void CCtrlInstruments::OnTbnDropDownToolBar(NMHDR *pNMHDR, LRESULT *pResult)
{
	CInputHandler *ih = CMainFrame::GetInputHandler();
	NMTOOLBAR *pToolBar = reinterpret_cast<NMTOOLBAR *>(pNMHDR);
	ClientToScreen(&(pToolBar->rcButton)); // TrackPopupMenu uses screen coords
	const int offset = Util::ScalePixels(4, m_hWnd);	// Compared to the main toolbar, the offset seems to be a bit wrong here...?
	int x = pToolBar->rcButton.left + offset, y = pToolBar->rcButton.bottom + offset;
	CMenu menu;
	switch(pToolBar->iItem)
	{
	case IDC_INSTRUMENT_NEW:
		{
			menu.CreatePopupMenu();
			menu.AppendMenu(MF_STRING, ID_INSTRUMENT_DUPLICATE, ih->GetKeyTextFromCommand(kcInstrumentCtrlDuplicate, _T("Duplicate &Instrument")));
			menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, this);
			menu.DestroyMenu();
		}
		break;
	case IDC_INSTRUMENT_SAVEAS:
		{
			menu.CreatePopupMenu();
			menu.AppendMenu(MF_STRING, IDC_SAVE_ALL, _T("Save &All..."));
			menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, this);
			menu.DestroyMenu();
		}
		break;
	}
	*pResult = 0;
}


void CCtrlInstruments::PrepareUndo(const char *description)
{
	m_startedEdit = true;
	m_modDoc.GetInstrumentUndo().PrepareUndo(m_nInstrument, description);
}


// Set document as modified and update other views.
// updateAll: Update all views including this one. Otherwise, only update update other views.
void CCtrlInstruments::SetModified(InstrumentHint hint, bool updateAll)
{
	m_modDoc.SetModified();
	m_modDoc.UpdateAllViews(nullptr, hint.SetData(m_nInstrument), updateAll ? nullptr : this);
}


BOOL CCtrlInstruments::SetCurrentInstrument(UINT nIns, BOOL bUpdNum)
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
		m_SliderCutOff.Invalidate(FALSE);
		m_SliderResonance.Invalidate(FALSE);
		// Volume ramping (attack)
		m_SliderAttack.Invalidate(FALSE);
	}
	SendViewMessage(VIEWMSG_SETCURRENTINSTRUMENT, m_nInstrument);
	UnlockControls();

	return TRUE;
}


void CCtrlInstruments::OnActivatePage(LPARAM lParam)
{
	CModControlDlg::OnActivatePage(lParam);
	if (lParam < 0)
	{
		int nIns = m_parent.GetInstrumentChange();
		if (nIns > 0) lParam = nIns;
	} else if(lParam > 0)
	{
		m_parent.InstrumentChanged(static_cast<INSTRUMENTINDEX>(lParam));
	}

	UpdatePluginList();

	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	INSTRUMENTVIEWSTATE &instrumentState = pFrame->GetInstrumentViewState();
	if(instrumentState.initialInstrument != 0)
	{
		m_nInstrument = instrumentState.initialInstrument;
		instrumentState.initialInstrument = 0;
	}

	SetCurrentInstrument(static_cast<INSTRUMENTINDEX>((lParam > 0) ? lParam : m_nInstrument));

	// Initial Update
	if (!m_bInitialized) UpdateView(InstrumentHint(m_nInstrument).Info().Envelope().ModType(), NULL);

	PostViewMessage(VIEWMSG_LOADSTATE, (LPARAM)&instrumentState);
	SwitchToView();

	// Combo boxes randomly disappear without this... why?
	Invalidate();
}


void CCtrlInstruments::OnDeactivatePage()
{
	m_modDoc.NoteOff(0, true);
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if ((pFrame) && (m_hWndView)) SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&pFrame->GetInstrumentViewState());
	CModControlDlg::OnDeactivatePage();
}


LRESULT CCtrlInstruments::OnModCtrlMsg(WPARAM wParam, LPARAM lParam)
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

	case CTRLMSG_INS_OPENFILE:
		if(lParam)
			return OpenInstrument(*reinterpret_cast<const mpt::PathString *>(lParam));
		break;

	case CTRLMSG_INS_SONGDROP:
		if(lParam)
		{
			const auto &dropInfo = *reinterpret_cast<const DRAGONDROP *>(lParam);
			if(dropInfo.sndFile)
				return OpenInstrument(*dropInfo.sndFile, static_cast<INSTRUMENTINDEX>(dropInfo.dropItem));
		}
		break;

	case CTRLMSG_INS_NEWINSTRUMENT:
		return InsertInstrument(false) ? 1 : 0;

	case CTRLMSG_SETCURRENTINSTRUMENT:
		SetCurrentInstrument(static_cast<INSTRUMENTINDEX>(lParam));
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
{
	if(pObj == this)
		return;
	if (hint.GetType()[HINT_MPTOPTIONS])
	{
		m_ToolBar.UpdateStyle();
		hint.ModType(); // For possibly updating note names in Pitch/Pan Separation dropdown
	}
	LockControls();
	if(hint.ToType<PluginHint>().GetType()[HINT_MIXPLUGINS | HINT_PLUGINNAMES])
	{
		OnMixPlugChanged();
	}
	if(hint.ToType<GeneralHint>().GetType()[HINT_TUNINGS])
	{
		BuildTuningComboBox();
	}
	UnlockControls();

	const InstrumentHint instrHint = hint.ToType<InstrumentHint>();
	FlagSet<HintType> hintType = instrHint.GetType();
	if(!m_bInitialized)
		hintType.set(HINT_MODTYPE);
	if(!hintType[HINT_MODTYPE | HINT_INSTRUMENT | HINT_ENVELOPE | HINT_INSNAMES])
		return;

	const INSTRUMENTINDEX updateIns = instrHint.GetInstrument();
	if(updateIns != m_nInstrument && updateIns != 0 && !hintType[HINT_MODTYPE])
		return;

	LockControls();
	const ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];

	if(hintType[HINT_MODTYPE])
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
		m_SpinFadeOut.SetAccel(mpt::saturate_cast<int>(std::size(accell)), accell);

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
				s.Format(_T("%i"), ich);
			m_CbnMidiCh.SetItemData(m_CbnMidiCh.AddString(s), ich);
		}
	}
	if(hintType[HINT_MODTYPE | HINT_INSTRUMENT | HINT_INSNAMES])
	{
		if(pIns)
			m_EditName.SetWindowText(mpt::ToCString(m_sndFile.GetCharsetInternal(), pIns->name));
		else
			m_EditName.SetWindowText(_T(""));
	}
	if(hintType[HINT_MODTYPE | HINT_INSTRUMENT])
	{
		m_SpinInstrument.SetRange(1, m_sndFile.m_nInstruments);
		m_SpinInstrument.EnableWindow((m_sndFile.m_nInstruments) ? TRUE : FALSE);

		// Backwards compatibility with legacy IT/XM modules that use now deprecated hack features.
		m_SliderCutSwing.EnableWindow(pIns != nullptr && (m_sndFile.GetType() == MOD_TYPE_MPT || pIns->nCutSwing != 0));
		m_SliderResSwing.EnableWindow(pIns != nullptr && (m_sndFile.GetType() == MOD_TYPE_MPT || pIns->nResSwing != 0));
		m_CbnFilterMode.EnableWindow (pIns != nullptr && (m_sndFile.GetType() == MOD_TYPE_MPT || pIns->filterMode != FilterMode::Unchanged));
		m_CbnResampling.EnableWindow (pIns != nullptr && (m_sndFile.GetType() == MOD_TYPE_MPT || pIns->resampling != SRCMODE_DEFAULT));
		m_SliderAttack.EnableWindow  (pIns != nullptr && (m_sndFile.GetType() == MOD_TYPE_MPT || pIns->nVolRampUp));
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT2), pIns != nullptr && (m_sndFile.GetType() == MOD_TYPE_MPT || pIns->nVolRampUp));

		if (pIns)
		{
			m_EditFileName.SetWindowText(mpt::ToCString(m_sndFile.GetCharsetInternal(), pIns->filename));
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
			for(int resMode = 0; resMode<m_CbnResampling.GetCount(); resMode++)
			{
				if(pIns->resampling == m_CbnResampling.GetItemData(resMode))
				{
					m_CbnResampling.SetCurSel(resMode);
					break;
				}
			}
			for(int fltMode = 0; fltMode<m_CbnFilterMode.GetCount(); fltMode++)
			{
				if(pIns->filterMode == static_cast<FilterMode>(m_CbnFilterMode.GetItemData(fltMode)))
				{
					m_CbnFilterMode.SetCurSel(fltMode);
					break;
				}
			}

			// NNA, DCT, DCA
			m_ComboNNA.SetCurSel(static_cast<int>(pIns->nNNA));
			m_ComboDCT.SetCurSel(static_cast<int>(pIns->nDCT));
			m_ComboDCA.SetCurSel(static_cast<int>(pIns->nDNA));
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
			m_EditFileName.SetWindowText(_T(""));
			velocityStyle.EnableWindow(FALSE);
			m_CbnPluginVolumeHandling.EnableWindow(FALSE);
			if(m_nInstrument > m_sndFile.GetNumInstruments())
				SetCurrentInstrument(m_sndFile.GetNumInstruments());

		}
		m_NoteMap.Invalidate(FALSE);

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

	UnlockControls();
}


void CCtrlInstruments::UpdateFilterText()
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
{
	BeginWaitCursor();
	mpt::IO::InputFile f(fileName, TrackerSettings::Instance().MiscCacheCompleteFileBeforeLoading);
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
		ScopedLogCapturer log(m_modDoc, _T("Instrument Import"), this);
		PrepareUndo("Replace Instrument");
		if (m_sndFile.ReadInstrumentFromFile(m_nInstrument, file, TrackerSettings::Instance().m_MayNormalizeSamplesOnLoad))
		{
			ok = true;
		} else
		{
			m_modDoc.GetInstrumentUndo().RemoveLastUndoStep(m_nInstrument);
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
			fileName.SplitPath(nullptr, nullptr, nullptr, &name, &ext);

			if (!pIns->name[0] && m_sndFile.GetModSpecifications().instrNameLengthMax > 0)
			{
				pIns->name = mpt::truncate(name.ToLocale(), m_sndFile.GetModSpecifications().instrNameLengthMax);
			}
			if (!pIns->filename[0] && m_sndFile.GetModSpecifications().instrFilenameLengthMax > 0)
			{
				name += ext;
				pIns->filename = mpt::truncate(name.ToLocale(), m_sndFile.GetModSpecifications().instrFilenameLengthMax);
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


bool CCtrlInstruments::OpenInstrument(const CSoundFile &sndFile, INSTRUMENTINDEX nInstr)
{
	if((!nInstr) || (nInstr > sndFile.GetNumInstruments())) return false;
	BeginWaitCursor();

	CriticalSection cs;

	bool first = false;
	if (!m_sndFile.GetNumInstruments())
	{
		first = true;
		m_sndFile.m_nInstruments = 1;
		SetCurrentInstrument(1);
		first = true;
	}
	PrepareUndo("Replace Instrument");
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
{
	if ((nSample > 0) && (nSample < MAX_SAMPLES))
	{
		m_parent.PostMessage(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_SAMPLES, nSample);
		return TRUE;
	}
	return FALSE;
}


BOOL CCtrlInstruments::GetToolTipText(UINT uId, LPTSTR pszText)
{
	//Note: pszText points to a TCHAR array of length 256 (see CChildFrame::OnToolTipText).
	//Note2: If there's problems in getting tooltips showing for certain tools,
	//		 setting the tab order may have effect.
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];

	if(pIns == nullptr) return FALSE;
	if ((pszText) && (uId))
	{
		CWnd *wnd = GetDlgItem(uId);
		bool isEnabled = wnd != nullptr && wnd->IsWindowEnabled() != FALSE;
		const auto plusMinus = mpt::ToWin(mpt::Charset::UTF8, "\xC2\xB1");
		const TCHAR *s = nullptr;
		CommandID cmd = kcNull;
		switch(uId)
		{
		case IDC_INSTRUMENT_NEW: s = _T("Insert Instrument (Hold Shift to duplicate)"); cmd = kcInstrumentNew; break;
		case IDC_INSTRUMENT_OPEN: s = _T("Import Instrument"); cmd = kcInstrumentLoad; break;
		case IDC_INSTRUMENT_SAVEAS: s = _T("Save Instrument"); cmd = kcInstrumentSave; break;
		case IDC_INSTRUMENT_PLAY: s = _T("Play Instrument"); break;

		case IDC_EDIT_PITCHTEMPOLOCK:
		case IDC_CHECK_PITCHTEMPOLOCK:
			// Pitch/Tempo lock
			if(isEnabled)
			{
				const CModSpecifications& specs = m_sndFile.GetModSpecifications();
				wsprintf(pszText, _T("Tempo Range: %u - %u"), specs.GetTempoMin().GetInt(), specs.GetTempoMax().GetInt());
			} else
			{
				_tcscpy(pszText, _T("Only available in MPTM format"));
			}
			return TRUE;

		case IDC_EDIT7:
			// Fade Out
			if(!pIns->nFadeOut)
				_tcscpy(pszText, _T("Fade disabled"));
			else
				wsprintf(pszText, _T("%u ticks (Higher value <-> Faster fade out)"), 0x8000 / pIns->nFadeOut);
			return TRUE;

		case IDC_EDIT8:
			// Global volume
			if(isEnabled)
				_tcscpy(pszText, CModDoc::LinearToDecibels(GetDlgItemInt(IDC_EDIT8), 64.0));
			else
				_tcscpy(pszText, _T("Only available in IT / MPTM format"));
			return TRUE;

		case IDC_EDIT9:
			// Panning
			if(isEnabled)
				_tcscpy(pszText, CModDoc::PanningToString(pIns->nPan, 128));
			else
				_tcscpy(pszText, _T("Only available in IT / MPTM format"));
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
			s = _T("Mapped: MIDI channel corresponds to pattern channel modulo 16");
			break;

		case IDC_SLIDER1:
			if(isEnabled)
				wsprintf(pszText, _T("%s%d%% volume variation"), plusMinus.c_str(), pIns->nVolSwing);
			else
				_tcscpy(pszText, _T("Only available in IT / MPTM format"));
			return TRUE;

		case IDC_SLIDER2:
			if(isEnabled)
				wsprintf(pszText, _T("%s%d panning variation"), plusMinus.c_str(), pIns->nPanSwing);
			else
				_tcscpy(pszText, _T("Only available in IT / MPTM format"));
			return TRUE;

		case IDC_SLIDER3:
			if(isEnabled)
				wsprintf(pszText, _T("%u"), pIns->GetCutoff());
			else
				_tcscpy(pszText, _T("Only available in IT / MPTM format"));
			return TRUE;

		case IDC_SLIDER4:
			if(isEnabled)
				wsprintf(pszText, _T("%u (%i dB)"), pIns->GetResonance(), Util::muldivr(pIns->GetResonance(), 24, 128));
			else
				_tcscpy(pszText, _T("Only available in IT / MPTM format"));
			return TRUE;

		case IDC_SLIDER6:
			if(isEnabled)
				wsprintf(pszText, _T("%s%d cutoff variation"), plusMinus.c_str(), pIns->nCutSwing);
			else
				_tcscpy(pszText, _T("Only available in MPTM format"));
			return TRUE;

		case IDC_SLIDER7:
			if(isEnabled)
				wsprintf(pszText, _T("%s%d resonance variation"), plusMinus.c_str(), pIns->nResSwing);
			else
				_tcscpy(pszText, _T("Only available in MPTM format"));
			return TRUE;

		case IDC_PITCHWHEELDEPTH:
			s = _T("Set this to the actual Pitch Wheel Depth used in your plugin on this channel.");
			break;

		case IDC_INSVIEWPLG:	// Open Editor
			if(!isEnabled)
				s = _T("No Plugin Loaded");
			break;

		case IDC_SPIN9:		// Pan
		case IDC_CHECK1:	// Pan
		case IDC_COMBO1:	// NNA
		case IDC_COMBO2:	// DCT
		case IDC_COMBO3:	// DNA
		case IDC_COMBO4:	// PPC
		case IDC_SPIN12:	// PPS
		case IDC_EDIT15:	// PPS
			if(!isEnabled)
				s = _T("Only available in IT / MPTM format");
			break;

		case IDC_COMBOTUNING:	// Tuning
		case IDC_COMBO9:		// Resampling:
		case IDC_SLIDER5:		// Ramping
		case IDC_SPIN1:			// Ramping
		case IDC_EDIT2:			// Ramping
			if(!isEnabled)
				s = _T("Only available in MPTM format");
			break;

		}

		if(s != nullptr)
		{
			_tcscpy(pszText, s);
			if(cmd != kcNull)
			{
				auto keyText = CMainFrame::GetInputHandler()->m_activeCommandSet->GetKeyTextFromCommand(cmd, 0);
				if (!keyText.IsEmpty())
					_tcscat(pszText, MPT_TFORMAT(" ({})")(keyText).c_str());
			}
			return TRUE;
		}
	}
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////
// CCtrlInstruments Messages

void CCtrlInstruments::OnInstrumentChanged()
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
{
	if(m_nInstrument > 1)
		SetCurrentInstrument(m_nInstrument - 1);
	else
		SetCurrentInstrument(m_sndFile.GetNumInstruments());
	m_parent.InstrumentChanged(m_nInstrument);
}


void CCtrlInstruments::OnNextInstrument()
{
	if(m_nInstrument < m_sndFile.GetNumInstruments())
		SetCurrentInstrument(m_nInstrument + 1);
	else
		SetCurrentInstrument(1);
	m_parent.InstrumentChanged(m_nInstrument);
}


void CCtrlInstruments::OnInstrumentNew()
{
	InsertInstrument(m_sndFile.GetNumInstruments() > 0 && CMainFrame::GetInputHandler()->ShiftPressed());
	SwitchToView();
}


bool CCtrlInstruments::InsertInstrument(bool duplicate)
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
{
	static int nLastIndex = 0;

	std::vector<FileType> mediaFoundationTypes = CSoundFile::GetMediaFoundationFileTypes();
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.EnableAudioPreview()
		.ExtensionFilter(
			"All Instruments (*.xi,*.pat,*.iti,*.sfz,...)|*.xi;*.pat;*.iti;*.sfz;*.flac;*.wav;*.w64;*.caf;*.aif;*.aiff;*.au;*.snd;*.sbk;*.sf2;*.sf3;*.sf4;*.dls;*.oga;*.ogg;*.opus;*.s3i;*.sb0;*.sb2;*.sbi;*.brr" + ToFilterOnlyString(mediaFoundationTypes, true).ToLocale() + "|"
			"FastTracker II Instruments (*.xi)|*.xi|"
			"GF1 Patches (*.pat)|*.pat|"
			"Impulse Tracker Instruments (*.iti)|*.iti|"
			"SFZ Instruments (*.sfz)|*.sfz|"
			"SoundFont 2.0 Banks (*.sf2)|*.sbk;*.sf2;*.sf3;*.sf4|"
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
{
	SaveInstrument(CMainFrame::GetInputHandler()->ShiftPressed());
}


void CCtrlInstruments::SaveInstrument(bool doBatchSave)
{
	if(!doBatchSave && m_sndFile.Instruments[m_nInstrument] == nullptr)
	{
		SwitchToView();
		return;
	}

	mpt::PathString fileName;
	if(!doBatchSave)
	{
		const ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
		if(pIns->filename[0])
			fileName = mpt::PathString::FromLocale(pIns->filename);
		else
			fileName = mpt::PathString::FromLocale(pIns->name);
	} else
	{
		// Save all samples
		fileName = m_sndFile.GetpModDoc()->GetPathNameMpt().GetFilenameBase();
		if(fileName.empty()) fileName = P_("untitled");

		fileName += P_(" - %instrument_number% - ");
		if(m_sndFile.GetModSpecifications().sampleFilenameLengthMax == 0)
			fileName += P_("%instrument_name%");
		else
			fileName += P_("%instrument_filename%");

	}
	fileName = fileName.AsSanitizedComponent();

	int index;
	if(TrackerSettings::Instance().compressITI)
		index = 2;
	else if(m_sndFile.GetType() == MOD_TYPE_XM)
		index = 4;
	else
		index = 1;

	FileDialog dlg = SaveFileDialog()
		.DefaultExtension(m_sndFile.GetType() == MOD_TYPE_XM ? U_("xi") : U_("iti"))
		.DefaultFilename(fileName)
		.ExtensionFilter(
			"Impulse Tracker Instruments (*.iti)|*.iti|"
			"Compressed Impulse Tracker Instruments (*.iti)|*.iti|"
			"Impulse Tracker Instruments with external Samples (*.iti)|*.iti|"
			"FastTracker II Instruments (*.xi)|*.xi|"
			"SFZ Instruments with WAV (*.sfz)|*.sfz|"
			"SFZ Instruments with FLAC (*.sfz)|*.sfz||")
		.WorkingDirectory(TrackerSettings::Instance().PathInstruments.GetWorkingDir())
		.FilterIndex(&index);
	if(!dlg.Show(this)) return;

	BeginWaitCursor();

	INSTRUMENTINDEX minIns = m_nInstrument, maxIns = m_nInstrument;
	if(doBatchSave)
	{
		minIns = 1;
		maxIns = m_sndFile.GetNumInstruments();
	}
	auto numberFmt = mpt::FormatSpec<mpt::ustring>().Dec().FillNul().Width(1 + static_cast<int>(std::log10(maxIns)));
	CString instrName, instrFilename;

	bool ok = true;
	const bool saveXI = !mpt::PathCompareNoCase(dlg.GetExtension(), P_("xi"));
	const bool saveSFZ = !mpt::PathCompareNoCase(dlg.GetExtension(), P_("sfz"));
	const bool doCompress = index == 2 || index == 6;
	const bool allowExternal = index == 3;

	for(INSTRUMENTINDEX ins = minIns; ins <= maxIns; ins++)
	{
		const ModInstrument *pIns = m_sndFile.Instruments[ins];
		if(pIns != nullptr)
		{
			fileName = dlg.GetFirstFile();
			if(doBatchSave)
			{
				instrName = mpt::ToCString(m_sndFile.GetCharsetInternal(), pIns->name[0] ? pIns->GetName() : "untitled");
				instrFilename = mpt::ToCString(m_sndFile.GetCharsetInternal(), pIns->filename[0] ? pIns->GetFilename() : pIns->GetName());
				instrName = SanitizePathComponent(instrName);
				instrFilename = SanitizePathComponent(instrFilename);

				mpt::ustring fileNameW = fileName.ToUnicode();
				fileNameW = mpt::String::Replace(fileNameW, U_("%instrument_number%"), mpt::ufmt::fmt(ins, numberFmt));
				fileNameW = mpt::String::Replace(fileNameW, U_("%instrument_filename%"), mpt::ToUnicode(instrFilename));
				fileNameW = mpt::String::Replace(fileNameW, U_("%instrument_name%"), mpt::ToUnicode(instrName));
				fileName = mpt::PathString::FromUnicode(fileNameW);
			}

			try
			{
				ScopedLogCapturer logcapturer(m_modDoc);
				mpt::IO::SafeOutputFile sf(fileName, std::ios::binary, mpt::IO::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
				mpt::IO::ofstream &f = sf;
				if(!f)
				{
					ok = false;
					continue;
				}
				f.exceptions(f.exceptions() | std::ios::badbit | std::ios::failbit);

				if (saveXI)
					ok &= m_sndFile.SaveXIInstrument(ins, f);
				else if (saveSFZ)
					ok &= m_sndFile.SaveSFZInstrument(ins, f, fileName, doCompress);
				else
					ok &= m_sndFile.SaveITIInstrument(ins, f, fileName, doCompress, allowExternal);
			} catch(const std::exception &)
			{
				ok = false;
			}
		}
	}

	EndWaitCursor();
	if (!ok)
		ErrorBox(IDS_ERR_SAVEINS, this);
	else
		TrackerSettings::Instance().PathInstruments.SetWorkingDir(dlg.GetWorkingDirectory());
	SwitchToView();
}


void CCtrlInstruments::OnInstrumentPlay()
{
	if (m_modDoc.IsNotePlaying(NOTE_MIDDLEC, 0, m_nInstrument))
	{
		m_modDoc.NoteOff(NOTE_MIDDLEC, true, m_nInstrument);
	} else
	{
		m_modDoc.PlayNote(PlayNoteParam(NOTE_MIDDLEC).Instrument(m_nInstrument));
	}
	SwitchToView();
}


void CCtrlInstruments::OnNameChanged()
{
	if (!IsLocked())
	{
		CString tmp;
		m_EditName.GetWindowText(tmp);
		const std::string s = mpt::ToCharset(m_sndFile.GetCharsetInternal(), tmp);
		ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
		if ((pIns) && (s != pIns->name))
		{
			if(!m_startedEdit) PrepareUndo("Set Name");
			pIns->name = s;
			SetModified(InstrumentHint().Names(), false);
		}
	}
}


void CCtrlInstruments::OnFileNameChanged()
{
	if (!IsLocked())
	{
		CString tmp;
		m_EditFileName.GetWindowText(tmp);
		const std::string s = mpt::ToCharset(m_sndFile.GetCharsetInternal(), tmp);
		ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
		if ((pIns) && (s != pIns->filename))
		{
			if(!m_startedEdit) PrepareUndo("Set Filename");
			pIns->filename = s;
			SetModified(InstrumentHint().Names(), false);
		}
	}
}


void CCtrlInstruments::OnFadeOutVolChanged()
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
			if(!m_startedEdit) PrepareUndo("Set Fade Out");
			pIns->nFadeOut = nVol;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnGlobalVolChanged()
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int nVol = GetDlgItemInt(IDC_EDIT8);
		Limit(nVol, 0, 64);
		if (nVol != (int)pIns->nGlobalVol)
		{
			if(!m_startedEdit) PrepareUndo("Set Global Volume");
			// Live-adjust volume
			pIns->nGlobalVol = nVol;
			for(auto &chn : m_sndFile.m_PlayState.Chn)
			{
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
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		const bool b = m_CheckPanning.GetCheck() != BST_UNCHECKED;

		PrepareUndo("Toggle Panning");
		pIns->dwFlags.set(INS_SETPANNING, b);

		if(b && m_sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			bool smpPanningInUse = false;

			const std::set<SAMPLEINDEX> referencedSamples = pIns->GetSamples();

			for(auto sample : referencedSamples)
			{
				if(sample <= m_sndFile.GetNumSamples() && m_sndFile.GetSample(sample).uFlags[CHN_PANNING])
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
					for (auto sample : referencedSamples)
					{
						if(sample <= m_sndFile.GetNumSamples())
							m_sndFile.GetSample(sample).uFlags.reset(CHN_PANNING);
					}
					m_modDoc.UpdateAllViews(nullptr, SampleHint().Info().ModType(), this);
				}
			}
		}
		SetModified(InstrumentHint().Info(), false);
	}
}


void CCtrlInstruments::OnPanningChanged()
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
			if(!m_startedEdit) PrepareUndo("Set Panning");
			pIns->nPan = nPan;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnNNAChanged()
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		const auto nna = static_cast<NewNoteAction>(m_ComboNNA.GetCurSel());
		if(pIns->nNNA != nna)
		{
			PrepareUndo("Set New Note Action");
			pIns->nNNA = nna;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnDCTChanged()
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		const auto dct = static_cast<DuplicateCheckType>(m_ComboDCT.GetCurSel());
		if(pIns->nDCT != dct)
		{
			PrepareUndo("Set Duplicate Check Type");
			pIns->nDCT = dct;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnDCAChanged()
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		const auto dna = static_cast<DuplicateNoteAction>(m_ComboDCA.GetCurSel());
		if (pIns->nDNA != dna)
		{
			PrepareUndo("Set Duplicate Check Action");
			pIns->nDNA = dna;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnMPRChanged()
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int n = GetDlgItemInt(IDC_EDIT10);
		if ((n >= 0) && (n <= 128))
		{
			if (pIns->nMidiProgram != n)
			{
				if(!m_startedEdit) PrepareUndo("Set MIDI Program");
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
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		uint16 w = static_cast<uint16>(GetDlgItemInt(IDC_EDIT11));
		if(w >= 0 && w <= 16384 && pIns->wMidiBank != w)
		{
			if(!m_startedEdit) PrepareUndo("Set MIDI Bank");
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
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if(!IsLocked() && pIns)
	{
		uint8 ch = static_cast<uint8>(m_CbnMidiCh.GetItemData(m_CbnMidiCh.GetCurSel()));
		if(pIns->nMidiChannel != ch)
		{
			PrepareUndo("Set MIDI Channel");
			pIns->nMidiChannel = ch;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}

void CCtrlInstruments::OnResamplingChanged()
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		ResamplingMode n = static_cast<ResamplingMode>(m_CbnResampling.GetItemData(m_CbnResampling.GetCurSel()));
		if (pIns->resampling != n)
		{
			PrepareUndo("Set Resampling");
			pIns->resampling = n;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnMixPlugChanged()
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	PLUGINDEX nPlug = static_cast<PLUGINDEX>(m_CbnMixPlug.GetItemData(m_CbnMixPlug.GetCurSel()));

	bool wasOpenedWithMouse = m_openendPluginListWithMouse;
	m_openendPluginListWithMouse = false;

	if (pIns)
	{
		BOOL enableVol = (nPlug < 1 || m_sndFile.m_playBehaviour[kMIDICCBugEmulation]) ? FALSE : TRUE;
		velocityStyle.EnableWindow(enableVol);
		m_CbnPluginVolumeHandling.EnableWindow(enableVol);

		if(nPlug >= 0 && nPlug <= MAX_MIXPLUGINS)
		{
			bool active = !IsLocked();
			if (active && pIns->nMixPlug != nPlug)
			{
				PrepareUndo("Set Plugin");
				pIns->nMixPlug = nPlug;
				SetModified(InstrumentHint().Info(), false);
			}

			velocityStyle.SetCheck(pIns->pluginVelocityHandling == PLUGIN_VELOCITYHANDLING_CHANNEL ? BST_CHECKED : BST_UNCHECKED);
			m_CbnPluginVolumeHandling.SetCurSel(pIns->pluginVolumeHandling);

#ifndef NO_PLUGINS
			if(pIns->nMixPlug)
			{
				// we have selected a plugin that's not "no plugin"
				const SNDMIXPLUGIN &plugin = m_sndFile.m_MixPlugins[pIns->nMixPlug - 1];

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
						for(auto sample : referencedSamples)
						{
							if(sample > 0 && sample <= m_sndFile.GetNumSamples() && m_sndFile.GetSample(sample).HasSampleData())
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
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int n = GetDlgItemInt(IDC_EDIT15);
		if ((n >= -32) && (n <= 32))
		{
			if (pIns->nPPS != (signed char)n)
			{
				if(!m_startedEdit) PrepareUndo("Set Pitch/Pan Separation");
				pIns->nPPS = (signed char)n;
				SetModified(InstrumentHint().Info(), false);
			}
		}
	}
}


void CCtrlInstruments::OnPPCChanged()
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int n = m_ComboPPC.GetCurSel();
		if(n >= 0 && n <= NOTE_MAX - NOTE_MIN)
		{
			if (pIns->nPPC != n)
			{
				PrepareUndo("Set Pitch/Pan Center");
				pIns->nPPC = static_cast<decltype(pIns->nPPC)>(n);
				SetModified(InstrumentHint().Info(), false);
			}
		}
	}
}


void CCtrlInstruments::OnAttackChanged()
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if(!IsLocked() && pIns)
	{
		int n = Clamp(static_cast<int>(GetDlgItemInt(IDC_EDIT2)), 0, MAX_ATTACK_VALUE);
		auto newRamp = static_cast<decltype(pIns->nVolRampUp)>(n);
		if(pIns->nVolRampUp != newRamp)
		{
			if(!m_startedEdit)
				PrepareUndo("Set Ramping");
			pIns->nVolRampUp = newRamp;
			SetModified(InstrumentHint().Info(), false);
		}

		m_SliderAttack.SetPos(n);
		if(CSpinButtonCtrl *spin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN1))
			spin->SetPos(n);
		LockControls();
		if (n == 0) SetDlgItemText(IDC_EDIT2, _T("default"));
		UnlockControls();
	}
}


void CCtrlInstruments::OnEnableCutOff()
{
	const bool enableCutOff = IsDlgButtonChecked(IDC_CHECK2) != BST_UNCHECKED;

	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if (pIns)
	{
		PrepareUndo("Toggle Cutoff");
		pIns->SetCutoff(pIns->GetCutoff(), enableCutOff);
		for(auto &chn : m_sndFile.m_PlayState.Chn)
		{
			if (chn.pModInstrument == pIns)
			{
				if(enableCutOff)
					chn.nCutOff = pIns->GetCutoff();
				else
					chn.nCutOff = 0x7F;
			}
		}
	}
	UpdateFilterText();
	SetModified(InstrumentHint().Info(), false);
	SwitchToView();
}


void CCtrlInstruments::OnEnableResonance()
{
	const bool enableReso = IsDlgButtonChecked(IDC_CHECK3) != BST_UNCHECKED;

	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if (pIns)
	{
		PrepareUndo("Toggle Resonance");
		pIns->SetResonance(pIns->GetResonance(), enableReso);
		for(auto &chn : m_sndFile.m_PlayState.Chn)
		{
			if (chn.pModInstrument == pIns)
			{
				if (enableReso)
					chn.nResonance = pIns->GetResonance();
				else
					chn.nResonance = 0;
			}
		}
	}
	UpdateFilterText();
	SetModified(InstrumentHint().Info(), false);
	SwitchToView();
}

void CCtrlInstruments::OnFilterModeChanged()
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		FilterMode instFiltermode = static_cast<FilterMode>(m_CbnFilterMode.GetItemData(m_CbnFilterMode.GetCurSel()));

		if(pIns->filterMode != instFiltermode)
		{
			PrepareUndo("Set Filter Mode");
			pIns->filterMode = instFiltermode;
			SetModified(InstrumentHint().Info(), false);

			//Update channel settings where this instrument is active, if required.
			if(instFiltermode != FilterMode::Unchanged)
			{
				for(auto &chn : m_sndFile.m_PlayState.Chn)
				{
					if(chn.pModInstrument == pIns)
						chn.nFilterMode = instFiltermode;
				}
			}
		}

	}
}


void CCtrlInstruments::OnVScroll(UINT nCode, UINT nPos, CScrollBar *pSB)
{
	// Give focus back to envelope editor when stopping to scroll spin buttons (for instrument preview keyboard focus)
	CModControlDlg::OnVScroll(nCode, nPos, pSB);
	if (nCode == SB_ENDSCROLL) SwitchToView();
}


void CCtrlInstruments::OnHScroll(UINT nCode, UINT nPos, CScrollBar *pSB)
{
	CModControlDlg::OnHScroll(nCode, nPos, pSB);
	if ((m_nInstrument) && (!IsLocked()) && (nCode != SB_ENDSCROLL))
	{
		ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
		if(!pIns)
			return;
			
		auto *pSlider = reinterpret_cast<const CSliderCtrl *>(pSB);
		int32 n = pSlider->GetPos();
		bool filterChanged = false;

		if(pSlider == &m_SliderAttack)
		{
			// Volume ramping (attack)
			if(pIns->nVolRampUp != n)
			{
				if(!m_startedHScroll)
				{
					PrepareUndo("Set Ramping");
					m_startedHScroll = true;
				}
				pIns->nVolRampUp = static_cast<decltype(pIns->nVolRampUp)>(n);
				SetDlgItemInt(IDC_EDIT2, n);
				SetModified(InstrumentHint().Info(), false);
			}
		} else if(pSlider == &m_SliderVolSwing)
		{
			// Volume Swing
			if((n >= 0) && (n <= 100) && (n != pIns->nVolSwing))
			{
				if(!m_startedHScroll)
				{
					PrepareUndo("Set Volume Random Variation");
					m_startedHScroll = true;
				}
				pIns->nVolSwing = static_cast<uint8>(n);
				SetModified(InstrumentHint().Info(), false);
			}
		} else if(pSlider == &m_SliderPanSwing)
		{
			// Pan Swing
			if((n >= 0) && (n <= 64) && (n != pIns->nPanSwing))
			{
				if(!m_startedHScroll)
				{
					PrepareUndo("Set Panning Random Variation");
					m_startedHScroll = true;
				}
				pIns->nPanSwing = static_cast<uint8>(n);
				SetModified(InstrumentHint().Info(), false);
			}
		} else if(pSlider == &m_SliderCutSwing)
		{
			// Cutoff swing
			if((n >= 0) && (n <= 64) && (n != pIns->nCutSwing))
			{
				if(!m_startedHScroll)
				{
					PrepareUndo("Set Cutoff Random Variation");
					m_startedHScroll = true;
				}
				pIns->nCutSwing = static_cast<uint8>(n);
				SetModified(InstrumentHint().Info(), false);
			}
		} else if(pSlider == &m_SliderResSwing)
		{
			// Resonance swing
			if((n >= 0) && (n <= 64) && (n != pIns->nResSwing))
			{
				if(!m_startedHScroll)
				{
					PrepareUndo("Set Resonance Random Variation");
					m_startedHScroll = true;
				}
				pIns->nResSwing = static_cast<uint8>(n);
				SetModified(InstrumentHint().Info(), false);
			}
		} else if(pSlider == &m_SliderCutOff)
		{
			// Filter Cutoff
			if((n >= 0) && (n < 0x80) && (n != (int)(pIns->GetCutoff())))
			{
				if(!m_startedHScroll)
				{
					PrepareUndo("Set Cutoff");
					m_startedHScroll = true;
				}
				pIns->SetCutoff(static_cast<uint8>(n), pIns->IsCutoffEnabled());
				SetModified(InstrumentHint().Info(), false);
				UpdateFilterText();
				filterChanged = true;
			}
		} else if(pSlider == &m_SliderResonance)
		{
			// Filter Resonance
			if((n >= 0) && (n < 0x80) && (n != (int)(pIns->GetResonance())))
			{
				if(!m_startedHScroll)
				{
					PrepareUndo("Set Resonance");
					m_startedHScroll = true;
				}
				pIns->SetResonance(static_cast<uint8>(n), pIns->IsResonanceEnabled());
				SetModified(InstrumentHint().Info(), false);
				UpdateFilterText();
				filterChanged = true;
			}
		}

		// Update channels
		if(filterChanged)
		{
			for(auto &chn : m_sndFile.m_PlayState.Chn)
			{
				if(chn.pModInstrument == pIns)
				{
					if(pIns->IsCutoffEnabled())
						chn.nCutOff = pIns->GetCutoff();
					if(pIns->IsResonanceEnabled())
						chn.nResonance = pIns->GetResonance();
				}
			}
		}
	} else if(nCode == SB_ENDSCROLL)
	{
		m_startedHScroll = false;
	}
	if ((nCode == SB_ENDSCROLL) || (nCode == SB_THUMBPOSITION))
	{
		SwitchToView();
	}

}


void CCtrlInstruments::OnEditSampleMap()
{
	if(m_nInstrument)
	{
		ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
		if (pIns)
		{
			PrepareUndo("Edit Sample Map");
			CSampleMapDlg dlg(m_sndFile, m_nInstrument, this);
			if (dlg.DoModal() == IDOK)
			{
				SetModified(InstrumentHint().Info(), true);
				m_NoteMap.Invalidate(FALSE);
			} else
			{
				m_modDoc.GetInstrumentUndo().RemoveLastUndoStep(m_nInstrument);
			}
		}
	}
}


void CCtrlInstruments::TogglePluginEditor()
{
	if(m_nInstrument)
	{
		m_modDoc.TogglePluginEditor(static_cast<PLUGINDEX>(m_CbnMixPlug.GetItemData(m_CbnMixPlug.GetCurSel()) - 1), CMainFrame::GetInputHandler()->ShiftPressed());
	}
}


BOOL CCtrlInstruments::PreTranslateMessage(MSG *pMsg)
{
	if(pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) ||
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler *ih = CMainFrame::GetInputHandler();
			if (ih->KeyEvent(kCtxCtrlInstruments, ih->Translate(*pMsg)) != kcNull)
				return true; // Mapped to a command, no need to pass message on.
		}

	}

	return CModControlDlg::PreTranslateMessage(pMsg);
}

LRESULT CCtrlInstruments::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
{
	switch(wParam)
	{
		case kcInstrumentCtrlLoad: OnInstrumentOpen(); return wParam;
		case kcInstrumentCtrlSave: OnInstrumentSaveOne(); return wParam;
		case kcInstrumentCtrlNew:  InsertInstrument(false); return wParam;
		case kcInstrumentCtrlDuplicate:	InsertInstrument(true); return wParam;
	}

	return kcNull;
}


void CCtrlInstruments::OnCbnSelchangeCombotuning()
{
	if (IsLocked()) return;

	ModInstrument *instr = m_sndFile.Instruments[m_nInstrument];
	if(instr == nullptr)
		return;

	size_t sel = m_ComboTuning.GetCurSel();
	if(sel == 0) //Setting IT behavior
	{
		CriticalSection cs;
		PrepareUndo("Reset Tuning");
		instr->SetTuning(nullptr);
		cs.Leave();

		SetModified(InstrumentHint().Info(), true);
		return;
	}

	sel -= 1;

	if(sel < m_sndFile.GetTuneSpecificTunings().GetNumTunings())
	{
		CriticalSection cs;
		PrepareUndo("Set Tuning");
		instr->SetTuning(m_sndFile.GetTuneSpecificTunings().GetTuning(sel));
		cs.Leave();

		SetModified(InstrumentHint().Info(), true);
		return;
	}

	//Case: Chosen tuning editor to be displayed.
	//Creating vector for the CTuningDialog.
	CTuningDialog td(this, m_nInstrument, m_sndFile);
	td.DoModal();
	if(td.GetModifiedStatus(&m_sndFile.GetTuneSpecificTunings()))
	{
		m_modDoc.SetModified();
	}

	//Recreating tuning combobox so that possible
	//new tuning(s) come visible.
	BuildTuningComboBox();

	m_modDoc.UpdateAllViews(nullptr, GeneralHint().Tunings());
	m_modDoc.UpdateAllViews(nullptr, InstrumentHint().Info());
}


void CCtrlInstruments::UpdateTuningComboBox()
{
	if(m_nInstrument > m_sndFile.GetNumInstruments()
		|| m_sndFile.Instruments[m_nInstrument] == nullptr) return;

	ModInstrument* const pIns = m_sndFile.Instruments[m_nInstrument];
	if(pIns->pTuning == nullptr)
	{
		m_ComboTuning.SetCurSel(0);
		return;
	}

	for(size_t i = 0; i < m_sndFile.GetTuneSpecificTunings().GetNumTunings(); i++)
	{
		if(pIns->pTuning == m_sndFile.GetTuneSpecificTunings().GetTuning(i))
		{
			m_ComboTuning.SetCurSel((int)(i + 1));
			return;
		}
	}

	Reporting::Notification(MPT_CFORMAT("Tuning {} was not found. Setting to default tuning.")(mpt::ToCString(m_sndFile.Instruments[m_nInstrument]->pTuning->GetName())));

	CriticalSection cs;
	pIns->SetTuning(m_sndFile.GetDefaultTuning());

	m_modDoc.SetModified();
}


void CCtrlInstruments::OnPluginVelocityHandlingChanged()
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if(!IsLocked() && pIns != nullptr)
	{
		PlugVelocityHandling n = velocityStyle.GetCheck() != BST_UNCHECKED ? PLUGIN_VELOCITYHANDLING_CHANNEL : PLUGIN_VELOCITYHANDLING_VOLUME;
		if(n != pIns->pluginVelocityHandling)
		{
			PrepareUndo("Set Velocity Handling");
			if(n == PLUGIN_VELOCITYHANDLING_VOLUME && m_CbnPluginVolumeHandling.GetCurSel() == PLUGIN_VOLUMEHANDLING_IGNORE)
			{
				// This combination doesn't make sense.
				m_CbnPluginVolumeHandling.SetCurSel(PLUGIN_VOLUMEHANDLING_MIDI);
			}

			pIns->pluginVelocityHandling = n;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnPluginVolumeHandlingChanged()
{
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if(!IsLocked() && pIns != nullptr)
	{
		PlugVolumeHandling n = static_cast<PlugVolumeHandling>(m_CbnPluginVolumeHandling.GetCurSel());
		if(n != pIns->pluginVolumeHandling)
		{
			PrepareUndo("Set Volume Handling");

			if(velocityStyle.GetCheck() == BST_UNCHECKED && n == PLUGIN_VOLUMEHANDLING_IGNORE)
			{
				// This combination doesn't make sense.
				velocityStyle.SetCheck(BST_CHECKED);
			}

			pIns->pluginVolumeHandling = n;
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnPitchWheelDepthChanged()
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
			if(!m_startedEdit) PrepareUndo("Set Pitch Wheel Depth");
			pIns->midiPWD = static_cast<int8>(pwd);
			SetModified(InstrumentHint().Info(), false);
		}
	}
}


void CCtrlInstruments::OnBnClickedCheckPitchtempolock()
{
	if(IsLocked() || !m_nInstrument) return;

	INSTRUMENTINDEX firstIns = m_nInstrument, lastIns = m_nInstrument;
	if(CMainFrame::GetInputHandler()->ShiftPressed())
	{
		firstIns = 1;
		lastIns = m_sndFile.GetNumInstruments();
	}

	m_EditPitchTempoLock.EnableWindow(IsDlgButtonChecked(IDC_CHECK_PITCHTEMPOLOCK));
	TEMPO ptl(0, 0);
	bool isZero = false;
	if(IsDlgButtonChecked(IDC_CHECK_PITCHTEMPOLOCK))
	{
		//Checking what value to put for the wPitchToTempoLock.
		if(m_EditPitchTempoLock.GetWindowTextLength() > 0)
		{
			ptl = m_EditPitchTempoLock.GetTempoValue();
		}
		if(!ptl.GetRaw())
		{
			ptl = m_sndFile.m_nDefaultTempo;
		}
		m_EditPitchTempoLock.SetTempoValue(ptl);
		isZero = true;
	}

	for(INSTRUMENTINDEX i = firstIns; i <= lastIns; i++)
	{
		if(m_sndFile.Instruments[i] != nullptr && (m_sndFile.Instruments[i]->pitchToTempoLock.GetRaw() == 0) == isZero)
		{
			m_modDoc.GetInstrumentUndo().PrepareUndo(i, "Set Pitch/Tempo Lock");
			m_sndFile.Instruments[i]->pitchToTempoLock = ptl;
			m_modDoc.SetModified();
		}
	}

	m_modDoc.UpdateAllViews(nullptr, InstrumentHint().Info(), this);
}


void CCtrlInstruments::OnEnChangeEditPitchTempoLock()
{
	if(IsLocked() || !m_nInstrument || !m_sndFile.Instruments[m_nInstrument]) return;

	TEMPO ptlTempo = m_EditPitchTempoLock.GetTempoValue();
	Limit(ptlTempo, m_sndFile.GetModSpecifications().GetTempoMin(), m_sndFile.GetModSpecifications().GetTempoMax());

	if(m_sndFile.Instruments[m_nInstrument]->pitchToTempoLock != ptlTempo)
	{
		if(!m_startedEdit) PrepareUndo("Set Pitch/Tempo Lock");
		m_sndFile.Instruments[m_nInstrument]->pitchToTempoLock = ptlTempo;
		m_modDoc.SetModified();	// Only update other views after killing focus
	}
}


void CCtrlInstruments::OnEnKillFocusEditPitchTempoLock()
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


void CCtrlInstruments::OnEnKillFocusEditFadeOut()
{
	if(IsLocked() || !m_nInstrument || !m_sndFile.Instruments[m_nInstrument]) return;

	if(m_modDoc.GetModType() == MOD_TYPE_IT)
	{
		// Coarse fade-out in IT files
		BOOL success;
		uint32 fadeout = (GetDlgItemInt(IDC_EDIT7, &success, FALSE) + 16) & ~31;
		if(success && fadeout != m_sndFile.Instruments[m_nInstrument]->nFadeOut)
		{
			SetDlgItemInt(IDC_EDIT7, fadeout, FALSE);
		}
	}
}


void CCtrlInstruments::BuildTuningComboBox()
{
	m_ComboTuning.SetRedraw(FALSE);
	m_ComboTuning.ResetContent();

	m_ComboTuning.AddString(_T("OpenMPT IT behaviour")); //<-> Instrument pTuning pointer == NULL
	for(const auto &tuning : m_sndFile.GetTuneSpecificTunings())
	{
		m_ComboTuning.AddString(mpt::ToCString(tuning->GetName()));
	}
	m_ComboTuning.AddString(_T("Control Tunings..."));
	UpdateTuningComboBox();
	m_ComboTuning.SetRedraw(TRUE);
}


void CCtrlInstruments::UpdatePluginList()
{
	m_CbnMixPlug.SetRedraw(FALSE);
	m_CbnMixPlug.Clear();
	m_CbnMixPlug.ResetContent();
#ifndef NO_PLUGINS
	m_CbnMixPlug.SetItemData(m_CbnMixPlug.AddString(_T("No plugin")), 0);
	AddPluginNamesToCombobox(m_CbnMixPlug, m_sndFile.m_MixPlugins, false);
#endif // NO_PLUGINS
	m_CbnMixPlug.Invalidate(FALSE);
	m_CbnMixPlug.SetRedraw(TRUE);
	ModInstrument *pIns = m_sndFile.Instruments[m_nInstrument];
	if ((pIns) && (pIns->nMixPlug <= MAX_MIXPLUGINS)) m_CbnMixPlug.SetCurSel(pIns->nMixPlug);
}


void CCtrlInstruments::OnXButtonUp(UINT nFlags, UINT nButton, CPoint point)
{
	if(nButton == XBUTTON1) OnPrevInstrument();
	else if(nButton == XBUTTON2) OnNextInstrument();
	CModControlDlg::OnXButtonUp(nFlags, nButton, point);
}


OPENMPT_NAMESPACE_END
