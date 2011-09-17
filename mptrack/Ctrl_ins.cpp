#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "ctrl_ins.h"
#include "view_ins.h"
#include "dlg_misc.h"
#include "tuningDialog.h"
#include "../common/misc_util.h"
#include "../common/StringFixer.h"
#include "SelectPluginDialog.h"

#pragma warning(disable:4244) //conversion from 'type1' to 'type2', possible loss of data

/////////////////////////////////////////////////////////////////////////
// CNoteMapWnd

BEGIN_MESSAGE_MAP(CNoteMapWnd, CStatic)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_NOTEMAP_TRANS_UP,		OnMapTransposeUp)
	ON_COMMAND(ID_NOTEMAP_TRANS_DOWN,	OnMapTransposeDown)
	ON_COMMAND(ID_NOTEMAP_COPY_NOTE,	OnMapCopyNote)
	ON_COMMAND(ID_NOTEMAP_COPY_SMP,		OnMapCopySample)
	ON_COMMAND(ID_NOTEMAP_RESET,		OnMapReset)
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
			CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();
			
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
	if ((pMsg->message == WM_CHAR) && (m_pModDoc)) //key is a character
	{
		UINT nFlags = HIWORD(pMsg->lParam);
		KeyEventType kT = (CMainFrame::GetMainFrame())->GetInputHandler()->GetKeyEventType(nFlags);

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


BOOL CNoteMapWnd::SetCurrentInstrument(CModDoc *pModDoc, UINT nIns)
//-----------------------------------------------------------------
{
	if ((pModDoc != m_pModDoc) || (nIns != m_nInstrument))
	{
		m_pModDoc = pModDoc;
		if (nIns < MAX_INSTRUMENTS) m_nInstrument = nIns;

		// create missing instrument if needed
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		if(m_nInstrument > 0 && pSndFile && m_nInstrument <= pSndFile->GetNumInstruments() && pSndFile->Instruments[m_nInstrument] == nullptr)
		{
			pSndFile->Instruments[m_nInstrument] = new MODINSTRUMENT();
			m_pModDoc->InitializeInstrument(pSndFile->Instruments[m_nInstrument]);
		}

		InvalidateRect(NULL, FALSE);
	}
	return TRUE;
}


BOOL CNoteMapWnd::SetCurrentNote(UINT nNote)
//------------------------------------------
{
	if (nNote == m_nNote) return TRUE;
	if (!NOTE_IS_VALID(nNote + 1)) return FALSE;
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
	if ((m_pModDoc) && (m_cxFont > 0) && (m_cyFont > 0))
	{
		bool bFocus = (::GetFocus() == m_hWnd);
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
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

			string temp = pSndFile->GetNoteName(nPos+1, m_nInstrument);
			temp.resize(4);
			if ((nPos >= 0) && (nPos < NOTE_MAX)) wsprintf(s, "%s", temp.c_str());
			rect.SetRect(0, ypaint, m_cxFont, ypaint+m_cyFont);
			DrawButtonRect(hdc, &rect, s, FALSE, FALSE);
			// Mapped Note
			bHighLight = ((bFocus) && (nPos == (int)m_nNote) /*&& (!m_bIns)*/);
			rect.left = rect.right;
			rect.right = m_cxFont*2-1;
			strcpy(s, "...");
			if ((pIns) && (nPos >= 0) && (nPos < NOTE_MAX) && (pIns->NoteMap[nPos]))
			{
				UINT n = pIns->NoteMap[nPos];
				if(NOTE_IS_VALID(n))
				{
					string temp = pSndFile->GetNoteName(n, m_nInstrument);
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
	CMainFrame::GetMainFrame()->m_pNoteMapHasFocus = (CWnd*) this; //rewbs.customKeys
}


void CNoteMapWnd::OnKillFocus(CWnd *pNewWnd)
//------------------------------------------
{
	CWnd::OnKillFocus(pNewWnd);
	InvalidateRect(NULL, FALSE);
	CMainFrame::GetMainFrame()->m_pNoteMapHasFocus=NULL;	//rewbs.customKeys
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
	if (m_pModDoc)
	{
		CHAR s[64];
		CSoundFile *pSndFile;
		MODINSTRUMENT *pIns;
		CInputHandler* ih = CMainFrame::GetInputHandler();
		
		pSndFile = m_pModDoc->GetSoundFile();
		pIns = pSndFile->Instruments[m_nInstrument];

		if (pIns)
		{
			HMENU hMenu = ::CreatePopupMenu();
			HMENU hSubMenu = ::CreatePopupMenu();

			if (hMenu)
			{
				AppendMenu(hMenu, MF_STRING, ID_INSTRUMENT_SAMPLEMAP, "Edit Sample &Map\t" + ih->GetKeyTextFromCommand(kcInsNoteMapEditSampleMap));
				if (hSubMenu)
				{
					const SAMPLEINDEX numSamps = pSndFile->GetNumSamples();
					vector<bool> smpused(numSamps + 1, false);
					for (UINT i=1; i<NOTE_MAX; i++)
					{
						SAMPLEINDEX nsmp = pIns->Keyboard[i];
						if (nsmp <= numSamps) smpused[nsmp] = true;
					}
					for (SAMPLEINDEX j = 1; j <= numSamps; j++)
					{
						if (smpused[j])
						{
							wsprintf(s, "%d: ", j);
							UINT l = strlen(s);
							memcpy(s + l, pSndFile->m_szNames[j], MAX_SAMPLENAME);
							s[l + MAX_SAMPLENAME] = 0;
							AppendMenu(hSubMenu, MF_STRING, ID_NOTEMAP_EDITSAMPLE+j, s);
						}
					}
					AppendMenu(hMenu, MF_POPUP, (UINT)hSubMenu, "&Edit Sample\t" + ih->GetKeyTextFromCommand(kcInsNoteMapEditSample));
					AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
				}
				wsprintf(s, "Map all notes to &sample %d\t" + ih->GetKeyTextFromCommand(kcInsNoteMapCopyCurrentSample), pIns->Keyboard[m_nNote]);
				AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_COPY_SMP, s);

				if(pSndFile->GetType() != MOD_TYPE_XM)
				{
					if(NOTE_IS_VALID(pIns->NoteMap[m_nNote]))
					{
						wsprintf(s, "Map all &notes to %s\t" + ih->GetKeyTextFromCommand(kcInsNoteMapCopyCurrentNote), pSndFile->GetNoteName(pIns->NoteMap[m_nNote], m_nInstrument).c_str());
						AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_COPY_NOTE, s);
					}
					AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_TRANS_UP, "Transpose map &up\t" + ih->GetKeyTextFromCommand(kcInsNoteMapTransposeUp));
					AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_TRANS_DOWN, "Transpose map &down\t" + ih->GetKeyTextFromCommand(kcInsNoteMapTransposeDown));
				}
				AppendMenu(hMenu, MF_STRING, ID_NOTEMAP_RESET, "&Reset note mapping\t" + ih->GetKeyTextFromCommand(kcInsNoteMapReset));
				AppendMenu(hMenu, MF_STRING, ID_INSTRUMENT_DUPLICATE, "Duplicate &Instrument\t" + ih->GetKeyTextFromCommand(kcInstrumentCtrlDuplicate));
				SetMenuDefaultItem(hMenu, ID_INSTRUMENT_SAMPLEMAP, FALSE);
				ClientToScreen(&pt);
				::TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
				::DestroyMenu(hMenu);
				if (hSubMenu) ::DestroyMenu(hSubMenu);
			}
		}
	}
}


void CNoteMapWnd::OnMapCopyNote()
//-------------------------------
{
	if(m_pModDoc == nullptr) return;
	CSoundFile *pSndFile;
	MODINSTRUMENT *pIns;
	
	pSndFile = m_pModDoc->GetSoundFile();
	pIns = pSndFile->Instruments[m_nInstrument];
	if (pIns)
	{
		bool bModified = false;
		BYTE n = pIns->NoteMap[m_nNote];
		for (NOTEINDEXTYPE i = 0; i < NOTE_MAX; i++) if (pIns->NoteMap[i] != n)
		{
			pIns->NoteMap[i] = n;
			bModified = true;
		}
		if (bModified)
		{
			m_pParent->SetInstrumentModified();
			InvalidateRect(NULL, FALSE);
		}
	}
}

void CNoteMapWnd::OnMapCopySample()
//---------------------------------
{
	if(m_pModDoc == nullptr) return;
	CSoundFile *pSndFile;
	MODINSTRUMENT *pIns;
	
	pSndFile = m_pModDoc->GetSoundFile();
	pIns = pSndFile->Instruments[m_nInstrument];
	if (pIns)
	{
		bool bModified = false;
		SAMPLEINDEX n = pIns->Keyboard[m_nNote];
		for (NOTEINDEXTYPE i = 0; i < NOTE_MAX; i++) if (pIns->Keyboard[i] != n)
		{
			pIns->Keyboard[i] = n;
			bModified = true;
		}
		if (bModified)
		{
			m_pParent->SetInstrumentModified();
			InvalidateRect(NULL, FALSE);
		}
	}
}


void CNoteMapWnd::OnMapReset()
//----------------------------
{
	if(m_pModDoc == nullptr) return;
	CSoundFile *pSndFile;
	MODINSTRUMENT *pIns;
	
	pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return;
	pIns = pSndFile->Instruments[m_nInstrument];
	if (pIns)
	{
		bool bModified = false;
		for (NOTEINDEXTYPE i = 0; i < NOTE_MAX; i++) if (pIns->NoteMap[i] != i + 1)
		{
			pIns->NoteMap[i] = i + 1;
			bModified = true;
		}
		if (bModified)
		{
			m_pParent->SetInstrumentModified();
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
	if(m_pModDoc == nullptr || nAmount == 0) return;
	CSoundFile *pSndFile;
	MODINSTRUMENT *pIns;

	pSndFile = m_pModDoc->GetSoundFile();
	pIns = pSndFile->Instruments[m_nInstrument];
	if (pIns)
	{
		bool bModified = false;
		for(NOTEINDEXTYPE i = 0; i < NOTE_MAX; i++)
		{
			int n = pIns->NoteMap[i];
			if ((n > NOTE_MIN && nAmount < 0) || (n < NOTE_MAX && nAmount > 0))
			{
				n = CLAMP(n + nAmount, NOTE_MIN, NOTE_MAX);
				pIns->NoteMap[i] = (BYTE)n;
				bModified = true;
			}
		}
		if (bModified)
		{
			m_pParent->SetInstrumentModified();
			InvalidateRect(NULL, FALSE);
		}
	}
}


void CNoteMapWnd::OnEditSample(UINT nID)
//--------------------------------------
{
	UINT nSample = nID - ID_NOTEMAP_EDITSAMPLE;
	if (m_pParent) m_pParent->EditSample(nSample);
}


void CNoteMapWnd::OnEditSampleMap()
//---------------------------------
{
	if (m_pParent) m_pParent->PostMessage(WM_COMMAND, ID_INSTRUMENT_SAMPLEMAP);
}


void CNoteMapWnd::OnInstrumentDuplicate()
//---------------------------------------
{
	if (m_pParent) m_pParent->PostMessage(WM_COMMAND, ID_INSTRUMENT_DUPLICATE);
}

//rewbs.customKeys
LRESULT CNoteMapWnd::OnCustomKeyMsg(WPARAM wParam, LPARAM lParam)
//---------------------------------------------------------------
{
	if (wParam == kcNull)
		return NULL;
	
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	MODINSTRUMENT *pIns = nullptr;
	if(pSndFile)
	{
		pIns = pSndFile->Instruments[m_nInstrument];
	}

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

	case kcInsNoteMapEditSample:		if(pIns) OnEditSample(pIns->Keyboard[m_nNote] + ID_NOTEMAP_EDITSAMPLE); return wParam;
	case kcInsNoteMapEditSampleMap:		OnEditSampleMap(); return wParam;

	// Parent shortcuts (also displayed in context menu of this control)
	case kcInstrumentCtrlDuplicate:		OnInstrumentDuplicate(); return wParam;
	}
	
	return NULL;
}

void CNoteMapWnd::EnterNote(UINT note)
//------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
	if ((pIns) && (m_nNote < NOTE_MAX))
	{
		if (!m_bIns && (pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)))
		{
			UINT n = pIns->NoteMap[m_nNote];
			bool bOk = false;
			if ((note >= pSndFile->GetModSpecifications().noteMin) && (note <= pSndFile->GetModSpecifications().noteMax))
			{	
				n = note;
				bOk = true;
			} 
			if (n != pIns->NoteMap[m_nNote])
			{
				pIns->NoteMap[m_nNote] = n;
				m_pParent->SetInstrumentModified();
				InvalidateRect(NULL, FALSE);
			}
			if (bOk) 
			{
				PlayNote(m_nNote + 1);
				//SetCurrentNote(m_nNote+1);
			}
			
		}
	}
}

bool CNoteMapWnd::HandleChar(WPARAM c)
//------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
	if ((pIns) && (m_nNote < NOTE_MAX))
	{

		if ((m_bIns) && (((c >= '0') && (c <= '9')) || (c == ' ')))	//in sample # column
		{
			UINT n = m_nOldIns;
			if (c != ' ')
			{
				n = (10 * pIns->Keyboard[m_nNote] + (c - '0')) % 10000;
				if ((n >= MAX_SAMPLES) || ((pSndFile->m_nSamples < 1000) && (n >= 1000))) n = (n % 1000);
				if ((n >= MAX_SAMPLES) || ((pSndFile->m_nSamples < 100) && (n >= 100))) n = (n % 100); else
				if ((n > 31) && (pSndFile->m_nSamples < 32) && (n % 10)) n = (n % 10);
			}

			if (n != pIns->Keyboard[m_nNote])
			{
				pIns->Keyboard[m_nNote] = n;
				m_pParent->SetInstrumentModified();
				InvalidateRect(NULL, FALSE);
				PlayNote(m_nNote+1);
			}

			if (c == ' ')
			{
				if (m_nNote < NOTE_MAX - 1) m_nNote++;
				InvalidateRect(NULL, FALSE);
				PlayNote(m_nNote);
			}
			return true;
		}

		else if ((!m_bIns) && (pSndFile->m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)))	//in note column
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
				pIns->NoteMap[m_nNote] = n;
				m_pParent->SetInstrumentModified();
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
	case VK_TAB:
		return true;
	case VK_RETURN:
		if (m_pModDoc && m_pModDoc->GetSoundFile())
		{
			MODINSTRUMENT *pIns = m_pModDoc->GetSoundFile()->Instruments[m_nInstrument];
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
	m_pModDoc->PlayNote(note, m_nInstrument, 0, FALSE);
	m_nPlayingNote = note;
}


void CNoteMapWnd::StopNote(int note = -1)
//----------------------------------
{
	if(note < 0) note = m_nPlayingNote;
	if(note < 0) return;

	m_pModDoc->NoteOff(note, TRUE, m_nInstrument);
	m_nPlayingNote = -1;
}

//end rewbs.customKeys


/////////////////////////////////////////////////////////////////////////
// CCtrlInstruments

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
#define MAX_ATTACK_LENGTH	2001
#define MAX_ATTACK_VALUE	(MAX_ATTACK_LENGTH - 1)  // 16 bit unsigned max
// -! NEW_FEATURE#0027

BEGIN_MESSAGE_MAP(CCtrlInstruments, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlInstruments)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
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
	ON_COMMAND(IDC_INSVIEWPLG,			TogglePluginEditor)		//rewbs.instroVSTi
	ON_EN_CHANGE(IDC_EDIT_INSTRUMENT,	OnInstrumentChanged)
	ON_EN_CHANGE(IDC_SAMPLE_NAME,		OnNameChanged)
	ON_EN_CHANGE(IDC_SAMPLE_FILENAME,	OnFileNameChanged)
	ON_EN_CHANGE(IDC_EDIT7,				OnFadeOutVolChanged)
	ON_EN_CHANGE(IDC_EDIT8,				OnGlobalVolChanged)
	ON_EN_CHANGE(IDC_EDIT9,				OnPanningChanged)
	ON_EN_CHANGE(IDC_EDIT10,			OnMPRChanged)
	ON_EN_CHANGE(IDC_EDIT11,			OnMBKChanged)	//rewbs.MidiBank
	ON_EN_CHANGE(IDC_EDIT15,			OnPPSChanged)

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
	ON_EN_CHANGE(IDC_EDIT2,				OnAttackChanged)
// -! NEW_FEATURE#0027

	ON_CBN_SELCHANGE(IDC_COMBO1,		OnNNAChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,		OnDCTChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,		OnDCAChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,		OnPPCChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5,		OnMCHChanged)
	ON_CBN_SELCHANGE(IDC_COMBO6,		OnMixPlugChanged)		//rewbs.instroVSTi
	ON_CBN_SELCHANGE(IDC_COMBO9,		OnResamplingChanged)
	ON_CBN_SELCHANGE(IDC_FILTERMODE,	OnFilterModeChanged)
	ON_CBN_SELCHANGE(IDC_PLUGIN_VELOCITYSTYLE, OnPluginVelocityHandlingChanged)
	ON_CBN_SELCHANGE(IDC_PLUGIN_VOLUMESTYLE, OnPluginVolumeHandlingChanged)
	ON_COMMAND(ID_INSTRUMENT_SAMPLEMAP, OnEditSampleMap)
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_COMBOTUNING, OnCbnSelchangeCombotuning)
	ON_EN_CHANGE(IDC_EDIT_PITCHTEMPOLOCK, OnEnChangeEditPitchtempolock)
	ON_BN_CLICKED(IDC_CHECK_PITCHTEMPOLOCK, OnBnClickedCheckPitchtempolock)
	ON_EN_KILLFOCUS(IDC_EDIT_PITCHTEMPOLOCK, OnEnKillfocusEditPitchtempolock)
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
	DDX_Control(pDX, IDC_COMBO6,				m_CbnMixPlug);	//rewbs.instroVSTi
	DDX_Control(pDX, IDC_COMBO9,				m_CbnResampling);
	DDX_Control(pDX, IDC_FILTERMODE,			m_CbnFilterMode);
	DDX_Control(pDX, IDC_EDIT7,					m_EditFadeOut);
	DDX_Control(pDX, IDC_SPIN7,					m_SpinFadeOut);
	DDX_Control(pDX, IDC_SPIN8,					m_SpinGlobalVol);
	DDX_Control(pDX, IDC_SPIN9,					m_SpinPanning);
	DDX_Control(pDX, IDC_SPIN10,				m_SpinMidiPR);
	DDX_Control(pDX, IDC_SPIN11,				m_SpinMidiBK);	//rewbs.MidiBank
	DDX_Control(pDX, IDC_SPIN12,				m_SpinPPS);
	DDX_Control(pDX, IDC_EDIT8,					m_EditGlobalVol);
	DDX_Control(pDX, IDC_EDIT9,					m_EditPanning);
	DDX_Control(pDX, IDC_EDIT15,				m_EditPPS);
	DDX_Control(pDX, IDC_CHECK1,				m_CheckPanning);
	DDX_Control(pDX, IDC_CHECK2,				m_CheckCutOff);
	DDX_Control(pDX, IDC_CHECK3,				m_CheckResonance);
	DDX_Control(pDX, IDC_SLIDER1,				m_SliderVolSwing);
	DDX_Control(pDX, IDC_SLIDER2,				m_SliderPanSwing);
	DDX_Control(pDX, IDC_SLIDER3,				m_SliderCutOff);
	DDX_Control(pDX, IDC_SLIDER4,				m_SliderResonance);
	DDX_Control(pDX, IDC_SLIDER6,				m_SliderCutSwing);
	DDX_Control(pDX, IDC_SLIDER7,				m_SliderResSwing);
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
	DDX_Control(pDX, IDC_SLIDER5,				m_SliderAttack);
	DDX_Control(pDX, IDC_SPIN1,					m_SpinAttack);
// -! NEW_FEATURE#0027
	DDX_Control(pDX, IDC_COMBOTUNING,			m_ComboTuning);
	DDX_Control(pDX, IDC_CHECK_PITCHTEMPOLOCK,	m_CheckPitchTempoLock);
	DDX_Control(pDX, IDC_EDIT_PITCHTEMPOLOCK,	m_EditPitchTempoLock);
	DDX_Control(pDX, IDC_PLUGIN_VELOCITYSTYLE,		m_CbnPluginVelocityHandling);
	DDX_Control(pDX, IDC_PLUGIN_VOLUMESTYLE,		m_CbnPluginVolumeHandling);
	//}}AFX_DATA_MAP
}


CCtrlInstruments::CCtrlInstruments()
//----------------------------------
{
	m_nInstrument = 1;
	m_nLockCount = 1;
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
	CHAR s[64];
	CModControlDlg::OnInitDialog();
	m_bInitialized = FALSE;
	if ((!m_pModDoc) || (!m_pSndFile)) return TRUE;

	m_NoteMap.Init(this);
	m_ToolBar.Init();
	m_ToolBar.AddButton(IDC_INSTRUMENT_NEW, TIMAGE_INSTR_NEW);
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
	m_ComboDCT.AddString("Plugin");	//rewbs.instroVSTi
	// DCA
	m_ComboDCA.AddString("Note Cut");
	m_ComboDCA.AddString("Note Off");
	m_ComboDCA.AddString("Note Fade");
	// FadeOut Volume
	m_SpinFadeOut.SetRange(0, 8192);
	// Global Volume
	m_SpinGlobalVol.SetRange(0, 64);
	// Panning
	m_SpinPanning.SetRange(0, (m_pModDoc->GetModType() & MOD_TYPE_IT) ? 64 : 256);
	// Midi Program
	m_SpinMidiPR.SetRange(0, 128);
	// rewbs.MidiBank
	m_SpinMidiBK.SetRange(0, 128);
	// Midi Channel
	//rewbs.instroVSTi: we no longer combine midi chan and FX in same cbbox
	for (UINT ich=0; ich<17; ich++)
	{
		UINT n = 0;
		s[0] = 0;
		if (!ich) { strcpy(s, "None"); n=0; } 
		else { wsprintf(s, "%d", ich); n=ich; }
		if (s[0]) m_CbnMidiCh.SetItemData(m_CbnMidiCh.AddString(s), n);
	}
	//end rewbs.instroVSTi
	m_CbnResampling.SetItemData(m_CbnResampling.AddString("Default"), SRCMODE_DEFAULT);
	m_CbnResampling.SetItemData(m_CbnResampling.AddString("None"), SRCMODE_NEAREST);
	m_CbnResampling.SetItemData(m_CbnResampling.AddString("Linear"), SRCMODE_LINEAR);
	m_CbnResampling.SetItemData(m_CbnResampling.AddString("Spline"), SRCMODE_SPLINE);
	m_CbnResampling.SetItemData(m_CbnResampling.AddString("Polyphase"), SRCMODE_POLYPHASE);
	m_CbnResampling.SetItemData(m_CbnResampling.AddString("XMMS"), SRCMODE_FIRFILTER);

	//end rewbs.instroVSTi
	m_CbnFilterMode.SetItemData(m_CbnFilterMode.AddString("Channel default"), FLTMODE_UNCHANGED);
	m_CbnFilterMode.SetItemData(m_CbnFilterMode.AddString("Force lowpass"), FLTMODE_LOWPASS);
	m_CbnFilterMode.SetItemData(m_CbnFilterMode.AddString("Force highpass"), FLTMODE_HIGHPASS);

	//VST velocity/volume handling
	m_CbnPluginVelocityHandling.AddString("Use note volume");
	m_CbnPluginVelocityHandling.AddString("Process as volume");
	m_CbnPluginVolumeHandling.AddString("MIDI volume");
	m_CbnPluginVolumeHandling.AddString("Dry/Wet ratio");
	m_CbnPluginVolumeHandling.AddString("None");

	// Vol/Pan Swing
	m_SliderVolSwing.SetRange(0, 64);
	m_SliderPanSwing.SetRange(0, 64);
	m_SliderCutSwing.SetRange(0, 64);
	m_SliderResSwing.SetRange(0, 64);
	// Filter
	m_SliderCutOff.SetRange(0x00, 0x7F);
	m_SliderResonance.SetRange(0x00, 0x7F);
	// Pitch/Pan Separation
	m_SpinPPS.SetRange(-32, +32);
	// Pitch/Pan Center
	AppendNotesToControl(m_ComboPPC, 0, NOTE_MAX-1);

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
	// Volume ramping (attack)
	m_SliderAttack.SetRange(0,MAX_ATTACK_VALUE);
	m_SpinAttack.SetRange(0,MAX_ATTACK_VALUE);
// -! NEW_FEATURE#0027

	m_SpinInstrument.SetFocus();

	BuildTuningComboBox();
	
	CheckDlgButton(IDC_CHECK_PITCHTEMPOLOCK, MF_UNCHECKED);
	m_EditPitchTempoLock.SetLimitText(4);

	return FALSE;
}


void CCtrlInstruments::RecalcLayout()
//-----------------------------------
{
}


// Set instrument (and moddoc) as modified.
void CCtrlInstruments::SetInstrumentModified(const bool modified)
//---------------------------------------------------------------
{
	if(m_pModDoc == nullptr) return;
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	m_pModDoc->m_bsInstrumentModified.set(m_nInstrument - 1, modified);
	m_pModDoc->UpdateAllViews(NULL, (m_nInstrument << HINT_SHIFT_INS) | HINT_INSNAMES, this);
// -! NEW_FEATURE#0023
	if(modified)
	{
		m_pModDoc->SetModified();
	}
}


BOOL CCtrlInstruments::SetCurrentInstrument(UINT nIns, BOOL bUpdNum)
//------------------------------------------------------------------
{
	if ((!m_pModDoc) || (!m_pSndFile)) return FALSE;
	if (m_pSndFile->m_nInstruments < 1) return FALSE;
	if ((nIns < 1) || (nIns > m_pSndFile->m_nInstruments)) return FALSE;
	LockControls();
	if ((m_nInstrument != nIns) || (!m_bInitialized))
	{
		m_nInstrument = nIns;
		m_NoteMap.SetCurrentInstrument(m_pModDoc, m_nInstrument);
		UpdateView((m_nInstrument << HINT_SHIFT_INS) | HINT_INSTRUMENT | HINT_ENVELOPE, NULL);
	} else
	{
		// Just in case
		m_NoteMap.SetCurrentInstrument(m_pModDoc, m_nInstrument);
	}
	if (bUpdNum)
	{
		SetDlgItemInt(IDC_EDIT_INSTRUMENT, m_nInstrument);
		m_SpinInstrument.SetRange(1, m_pSndFile->GetNumInstruments());
		m_SpinInstrument.EnableWindow((m_pSndFile->GetNumInstruments()) ? TRUE : FALSE);
		// Is this a bug ?
		m_SliderCutOff.InvalidateRect(NULL, FALSE);
		m_SliderResonance.InvalidateRect(NULL, FALSE);
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
		// Volume ramping (attack)
		m_SliderAttack.InvalidateRect(NULL, FALSE);
// -! NEW_FEATURE#0027
	}
	PostViewMessage(VIEWMSG_SETCURRENTINSTRUMENT, m_nInstrument);
	UnlockControls();

	return TRUE;
}


void CCtrlInstruments::OnActivatePage(LPARAM lParam)
//--------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (m_pParent))
	{
		if (lParam < 0)
		{
			int nIns = m_pParent->GetInstrumentChange();
			if (nIns > 0) lParam = nIns;
			m_pParent->InstrumentChanged(-1);
		} else
		if (lParam > 0)
		{
			m_pParent->InstrumentChanged(lParam);
		}
	}

	UpdatePluginList();

	SetCurrentInstrument((lParam > 0) ? lParam : m_nInstrument);

	// Initial Update
	if (!m_bInitialized) UpdateView((m_nInstrument << HINT_SHIFT_INS) | HINT_INSTRUMENT | HINT_ENVELOPE | HINT_MODTYPE, NULL);

	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if (pFrame) PostViewMessage(VIEWMSG_LOADSTATE, (LPARAM)pFrame->GetInstrumentViewState());
	SwitchToView();

	// Combo boxes randomly disappear without this... why?
	Invalidate();
}


void CCtrlInstruments::OnDeactivatePage()
//---------------------------------------
{
	if (m_pModDoc) m_pModDoc->NoteOff(0, TRUE);
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if ((pFrame) && (m_hWndView)) SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)pFrame->GetInstrumentViewState());
}


LRESULT CCtrlInstruments::OnModCtrlMsg(WPARAM wParam, LPARAM lParam)
//------------------------------------------------------------------
{
	switch(wParam)
	{
	case CTRLMSG_INS_PREVINSTRUMENT:
		OnPrevInstrument();
		break;

	case CTRLMSG_INS_NEXTINSTRUMENT:
		OnNextInstrument();
		break;

	case CTRLMSG_INS_OPENFILE:
		if (lParam) return OpenInstrument((LPCSTR)lParam);
		break;

	case CTRLMSG_INS_SONGDROP:
		if (lParam)
		{
			LPDRAGONDROP pDropInfo = (LPDRAGONDROP)lParam;
			CSoundFile *pSndFile = (CSoundFile *)(pDropInfo->lDropParam);
			if (pDropInfo->pModDoc) pSndFile = pDropInfo->pModDoc->GetSoundFile();
			if (pSndFile) return OpenInstrument(pSndFile, pDropInfo->dwDropItem);
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

	//rewbs.customKeys
	case IDC_INSTRUMENT_NEW:
		OnInstrumentNew();
		break;
	case IDC_INSTRUMENT_OPEN:
		OnInstrumentOpen();
		break;
	case IDC_INSTRUMENT_SAVEAS:
		OnInstrumentSave();
		break;
	//end rewbs.customKeys

	default:
		return CModControlDlg::OnModCtrlMsg(wParam, lParam);
	}
	return 0;
}


void CCtrlInstruments::UpdateView(DWORD dwHintMask, CObject *pObj)
//----------------------------------------------------------------
{
	if ((pObj == this) || (!m_pModDoc) || (!m_pSndFile)) return;
	if (dwHintMask & HINT_MPTOPTIONS)
	{
		m_ToolBar.UpdateStyle();
	}
	LockControls();
	if (dwHintMask & HINT_MIXPLUGINS) OnMixPlugChanged();
	UnlockControls();
	if (!(dwHintMask & (HINT_INSTRUMENT|HINT_ENVELOPE|HINT_MODTYPE))) return;
	if (((dwHintMask >> HINT_SHIFT_INS) != m_nInstrument) && (dwHintMask & (HINT_INSTRUMENT|HINT_ENVELOPE)) && (!(dwHintMask & HINT_MODTYPE))) return;
	LockControls();
	if (!m_bInitialized) dwHintMask |= HINT_MODTYPE;
	if (dwHintMask & HINT_MODTYPE)
	{
		const CModSpecifications *specs = &m_pSndFile->GetModSpecifications();

		// Limit text fields
		m_EditName.SetLimitText(specs->instrNameLengthMax);
		m_EditFileName.SetLimitText(specs->instrFilenameLengthMax);

		const BOOL bITandMPT = ((m_pSndFile->m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (m_pSndFile->m_nInstruments)) ? TRUE : FALSE;
		//rewbs.instroVSTi
		const BOOL bITandXM = ((m_pSndFile->m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM))  && (m_pSndFile->m_nInstruments)) ? TRUE : FALSE;
		const BOOL bMPTOnly = ((m_pSndFile->m_nType == MOD_TYPE_MPT) && (m_pSndFile->m_nInstruments)) ? TRUE : FALSE;
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT10), bITandXM);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT11), bITandXM);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT7), bITandXM);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT2), bITandXM);
		m_SliderAttack.EnableWindow(bITandXM);
		m_EditName.EnableWindow(bITandXM);
		m_EditFileName.EnableWindow(bITandMPT);
		m_CbnMidiCh.EnableWindow(bITandXM);
		m_CbnMixPlug.EnableWindow(bITandXM);
		m_SpinMidiPR.EnableWindow(bITandXM);
		m_SpinMidiBK.EnableWindow(bITandXM);	//rewbs.MidiBank
		//end rewbs.instroVSTi

		const bool extendedFadeoutRange = (m_pSndFile->m_nType & MOD_TYPE_XM) != 0;
		m_SpinFadeOut.EnableWindow(bITandXM);
		m_SpinFadeOut.SetRange(0, extendedFadeoutRange ? 32767 : 8192);
		m_EditFadeOut.SetLimitText(extendedFadeoutRange ? 5 : 4);

		// Panning ranges (0...64 for IT, 0...256 for MPTM)
		m_SpinPanning.SetRange(0, (m_pModDoc->GetModType() & MOD_TYPE_IT) ? 64 : 256);

		m_NoteMap.EnableWindow(bITandXM);
		m_CbnResampling.EnableWindow(bITandXM);
		
		m_ComboNNA.EnableWindow(bITandMPT);
		m_SliderVolSwing.EnableWindow(bITandMPT);
		m_SliderPanSwing.EnableWindow(bITandMPT);
		m_SliderCutSwing.EnableWindow(bITandMPT);
		m_SliderResSwing.EnableWindow(bITandMPT);
		m_CbnFilterMode.EnableWindow(bITandMPT);
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
		m_SpinInstrument.SetRange(1, m_pSndFile->m_nInstruments);
		m_SpinInstrument.EnableWindow((m_pSndFile->m_nInstruments) ? TRUE : FALSE);
		m_ComboTuning.EnableWindow(bMPTOnly);
		m_EditPitchTempoLock.EnableWindow(bITandMPT);
		m_CheckPitchTempoLock.EnableWindow(bITandMPT);
	}
	if (dwHintMask & (HINT_INSTRUMENT|HINT_MODTYPE))
	{
		CHAR s[128];
		MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			memcpy(s, pIns->name, 32);
			s[32] = 0;
			m_EditName.SetWindowText(s);
			memcpy(s, pIns->filename, 12);
			s[12] = 0;
			m_EditFileName.SetWindowText(s);
			// Fade Out Volume
			SetDlgItemInt(IDC_EDIT7, pIns->nFadeOut);
			// Global Volume
			SetDlgItemInt(IDC_EDIT8, pIns->nGlobalVol);
			// Panning
			SetDlgItemInt(IDC_EDIT9, (m_pModDoc->GetModType() & MOD_TYPE_IT) ? (pIns->nPan / 4) : pIns->nPan);
			m_CheckPanning.SetCheck((pIns->dwFlags & INS_SETPANNING) ? TRUE : FALSE);
			// Midi
			if (pIns->nMidiProgram>0 && pIns->nMidiProgram<=128)
				SetDlgItemInt(IDC_EDIT10, pIns->nMidiProgram);
			else
				SetDlgItemText(IDC_EDIT10, "---");
			if (pIns->wMidiBank && pIns->wMidiBank<=128)
				SetDlgItemInt(IDC_EDIT11, pIns->wMidiBank);
			else
				SetDlgItemText(IDC_EDIT11, "---");
			//rewbs.instroVSTi
			//was:
			//if (pIns->nMidiChannel < 17) m_CbnMidiCh.SetCurSel(pIns->nMidiChannel); else
			//if (pIns->nMidiChannel & 0x80) m_CbnMidiCh.SetCurSel((pIns->nMidiChannel&0x7f)+16); else
			//	m_CbnMidiCh.SetCurSel(0);
			//now:
			if (pIns->nMidiChannel < 17) {
				m_CbnMidiCh.SetCurSel(pIns->nMidiChannel); 
			} else {
				m_CbnMidiCh.SetCurSel(0);
			}
			if (pIns->nMixPlug <= MAX_MIXPLUGINS) {
				m_CbnMixPlug.SetCurSel(pIns->nMixPlug);
			} else {
				m_CbnMixPlug.SetCurSel(0);
			}
			OnMixPlugChanged();
			//end rewbs.instroVSTi
			for(int nRes = 0; nRes<m_CbnResampling.GetCount(); nRes++) {
                DWORD v = m_CbnResampling.GetItemData(nRes);
		        if (pIns->nResampling == v) {
					m_CbnResampling.SetCurSel(nRes);
					break;
	             }
			}
			for(int nFltMode = 0; nFltMode<m_CbnFilterMode.GetCount(); nFltMode++) {
                DWORD v = m_CbnFilterMode.GetItemData(nFltMode);
		        if (pIns->nFilterMode == v) {
					m_CbnFilterMode.SetCurSel(nFltMode);
					break;
	             }
			}

			// NNA, DCT, DCA
			m_ComboNNA.SetCurSel(pIns->nNNA);
			m_ComboDCT.SetCurSel(pIns->nDCT);
			m_ComboDCA.SetCurSel(pIns->nDNA);
			// Pitch/Pan Separation
			m_ComboPPC.SetCurSel(pIns->nPPC);
			SetDlgItemInt(IDC_EDIT15, pIns->nPPS);
			// Filter
			if (m_pSndFile->m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT))
			{
				m_CheckCutOff.SetCheck((pIns->nIFC & 0x80) ? TRUE : FALSE);
				m_CheckResonance.SetCheck((pIns->nIFR & 0x80) ? TRUE : FALSE);
				m_SliderVolSwing.SetPos(pIns->nVolSwing);
				m_SliderPanSwing.SetPos(pIns->nPanSwing);
				m_SliderResSwing.SetPos(pIns->nResSwing);
				m_SliderCutSwing.SetPos(pIns->nCutSwing);
				m_SliderCutOff.SetPos(pIns->nIFC & 0x7F);
				m_SliderResonance.SetPos(pIns->nIFR & 0x7F);
				UpdateFilterText();
			}
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
			// Volume ramping (attack)
			int n = pIns->nVolRampUp; //? MAX_ATTACK_LENGTH - pIns->nVolRampUp : 0;
			m_SliderAttack.SetPos(n);
			if(n == 0) SetDlgItemText(IDC_EDIT2,"default");
			else SetDlgItemInt(IDC_EDIT2,n);
// -! NEW_FEATURE#0027

			UpdateTuningComboBox();
			if(pIns->wPitchToTempoLock > 0) //Current instrument uses pitchTempoLock.
				CheckDlgButton(IDC_CHECK_PITCHTEMPOLOCK, MF_CHECKED);
			else
				CheckDlgButton(IDC_CHECK_PITCHTEMPOLOCK, MF_UNCHECKED);
			
			OnBnClickedCheckPitchtempolock();

			if(m_pSndFile->GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT))
			{
				if(m_CbnMixPlug.GetCurSel() > 0 && m_pSndFile->GetModFlag(MSF_MIDICC_BUGEMULATION) == false)
				{
					m_CbnPluginVelocityHandling.EnableWindow(TRUE);
					m_CbnPluginVolumeHandling.EnableWindow(TRUE);
				}
				else
				{
					m_CbnPluginVelocityHandling.EnableWindow(FALSE);
					m_CbnPluginVolumeHandling.EnableWindow(FALSE);
				}

			}
		} else
		{
			m_EditName.SetWindowText("");
			m_EditFileName.SetWindowText("");
			m_CbnPluginVelocityHandling.EnableWindow(FALSE);
			m_CbnPluginVolumeHandling.EnableWindow(FALSE);
			m_CbnPluginVelocityHandling.SetCurSel(-1);
			m_CbnPluginVolumeHandling.SetCurSel(-1);
			if(m_nInstrument > m_pSndFile->GetNumInstruments())
				SetCurrentInstrument(m_pSndFile->GetNumInstruments());

		}
		m_NoteMap.InvalidateRect(NULL, FALSE);
	}
	if(dwHintMask & (HINT_MIXPLUGINS|HINT_MODTYPE))
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


VOID CCtrlInstruments::UpdateFilterText()
//---------------------------------------
{
	if ((m_nInstrument) && (m_pModDoc))
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			CHAR s[64];
			if (pIns->nIFC&0x80 && pIns->nIFC<0xFF) {
				wsprintf(s, "%d Hz", pSndFile->CutOffToFrequency(pIns->nIFC & 0x7F));
			} else {
				wsprintf(s, "Off");
			}
			
			SetDlgItemText(IDC_TEXT1, s);
		}
	}
}


BOOL CCtrlInstruments::OpenInstrument(LPCSTR lpszFileName)
//--------------------------------------------------------
{
	CMappedFile f;
	BOOL bFirst, bOk;
	DWORD len;
	LPBYTE lpFile;
	
	BeginWaitCursor();
	if ((!lpszFileName) || (!f.Open(lpszFileName)))
	{
		EndWaitCursor();
		return FALSE;
	}
	bFirst = FALSE;
	len = f.GetLength();
	if (len > CTrackApp::gMemStatus.dwTotalPhys) len = CTrackApp::gMemStatus.dwTotalPhys;
	lpFile = f.Lock(len);
	bOk = FALSE;
	if (lpFile)
	{
		CriticalSection cs;

		if (!m_pSndFile->GetNumInstruments())
		{
			bFirst = TRUE;
			m_pSndFile->m_nInstruments = 1;
			m_NoteMap.SetCurrentInstrument(m_pModDoc, 1);
			m_pModDoc->SetModified();
		}
		if (!m_nInstrument) m_nInstrument = 1;
		if (m_pSndFile->ReadInstrumentFromFile(m_nInstrument, lpFile, len))
		{
			m_pModDoc->UpdateAllViews(NULL, HINT_SAMPLEINFO | HINT_MODTYPE, NULL);
			// -> CODE#0023
			// -> DESC="IT project files (.itp)"
			int n = strlen(lpszFileName);
			if(n >= _MAX_PATH) n = _MAX_PATH-1;
			strncpy(m_pSndFile->m_szInstrumentPath[m_nInstrument-1],lpszFileName,n);
			m_pSndFile->m_szInstrumentPath[m_nInstrument-1][n] = '\0';
			SetInstrumentModified(false);
			// -! NEW_FEATURE#0023
			bOk = TRUE;
		}

		f.Unlock();
	}

	f.Close();
	EndWaitCursor();
	if (bOk)
	{
		MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			TCHAR szName[_MAX_FNAME], szExt[_MAX_EXT];
			_tsplitpath(lpszFileName, nullptr, nullptr, szName, szExt);
			CMainFrame::GetSettings().SetWorkingDirectory(lpszFileName, DIR_INSTRUMENTS, true);
	
			if (!pIns->name[0])
			{
				szName[m_pSndFile->GetModSpecifications().instrNameLengthMax - 1] = 0;
				strcpy(pIns->name, szName);
			}
			if (!pIns->filename[0])
			{
				strcat(szName, szExt);
				szName[m_pSndFile->GetModSpecifications().instrFilenameLengthMax - 1] = 0;
				strcpy(pIns->filename, szName);
			}
			SetCurrentInstrument(m_nInstrument);
			if (m_pModDoc)
			{
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, (m_nInstrument << HINT_SHIFT_INS) | HINT_INSTRUMENT | HINT_ENVELOPE | HINT_INSNAMES | HINT_SMPNAMES);
			}
		} else bOk = FALSE;
	}
	if (bFirst) m_pModDoc->UpdateAllViews(NULL, HINT_MODTYPE | HINT_INSNAMES | HINT_SMPNAMES);
	if (!bOk) ErrorBox(IDS_ERR_FILETYPE, this);
	return TRUE;
}


BOOL CCtrlInstruments::OpenInstrument(CSoundFile *pSndFile, UINT nInstr)
//----------------------------------------------------------------------
{
	if ((!pSndFile) || (!nInstr) || (nInstr > pSndFile->m_nInstruments)) return FALSE;
	BeginWaitCursor();

	CriticalSection cs;

	BOOL bFirst = FALSE;
	if (!m_pSndFile->m_nInstruments)
	{
		bFirst = TRUE;
		m_pSndFile->m_nInstruments = 1;
		m_NoteMap.SetCurrentInstrument(m_pModDoc, 1);
		m_pModDoc->SetModified();
		bFirst = TRUE;
	}
	if (!m_nInstrument)
	{
		m_nInstrument = 1;
		bFirst = TRUE;
	}
	m_pSndFile->ReadInstrumentFromSong(m_nInstrument, pSndFile, nInstr);

	cs.Leave();

	m_pModDoc->SetModified();
	m_pModDoc->UpdateAllViews(NULL, (m_nInstrument << HINT_SHIFT_INS) | HINT_INSTRUMENT | HINT_ENVELOPE | HINT_INSNAMES | HINT_SMPNAMES);
	if (bFirst) m_pModDoc->UpdateAllViews(NULL, HINT_MODTYPE | HINT_INSNAMES | HINT_SMPNAMES);
	m_pModDoc->SetModified();
	EndWaitCursor();
	return TRUE;
}


BOOL CCtrlInstruments::EditSample(UINT nSample)
//---------------------------------------------
{
	if ((nSample > 0) && (nSample < MAX_SAMPLES))
	{
		if (m_pParent)
		{
			m_pParent->PostMessage(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_SAMPLES, nSample);
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CCtrlInstruments::GetToolTipText(UINT uId, LPSTR pszText)
//------------------------------------------------------------
{
	//Note: pszText seems to point to char array of length 256 (Noverber 2006).
	//Note2: If there's problems in getting tooltips showing for certain tools, 
	//		 setting the tab order may have effect.
	const bool hasInstrument = (m_pSndFile) && (m_pSndFile->Instruments[m_nInstrument]);
	if(!hasInstrument) return FALSE;
	if ((pszText) && (uId))
	{
		switch(uId)
		{
		case IDC_EDIT1:
			{
			MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
			wsprintf(pszText, "Z%02X", pIns->nIFC & 0x7f);
			return TRUE;
			break;
			}
		case IDC_EDIT_PITCHTEMPOLOCK:
		case IDC_CHECK_PITCHTEMPOLOCK:
			{
			const CModSpecifications& specs = m_pSndFile->GetModSpecifications();
			string str = string("Tempo range: ") + Stringify(specs.tempoMin) + string(" - ") + Stringify(specs.tempoMax);
			if(str.size() >= 250) str.resize(250);
			wsprintf(pszText, str.c_str());
			return TRUE;
			break;
			}
		case IDC_EDIT7: //Fade out
			wsprintf(pszText, "Higher value <-> Faster fade out");
			return TRUE;
			break;

		case IDC_PLUGIN_VELOCITYSTYLE:
		case IDC_PLUGIN_VOLUMESTYLE:
			if(m_pSndFile->Instruments[m_nInstrument]->nMixPlug < 1) return FALSE;
			if(m_pSndFile->GetModFlag(MSF_MIDICC_BUGEMULATION))
			{
				m_CbnPluginVelocityHandling.EnableWindow(FALSE);
				m_CbnPluginVolumeHandling.EnableWindow(FALSE);
				wsprintf(pszText, "To enable, clear Plugin volume command bug emulation flag from Song Properties");
				return TRUE;
			}
			else
				return FALSE;
			
			break;
		}
	}
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////
// CCtrlInstruments Messages

void CCtrlInstruments::OnInstrumentChanged()
//------------------------------------------
{
	if ((!IsLocked()) && (m_pSndFile))
	{
		UINT n = GetDlgItemInt(IDC_EDIT_INSTRUMENT);
		if ((n > 0) && (n <= m_pSndFile->m_nInstruments) && (n != m_nInstrument))
		{
			SetCurrentInstrument(n, FALSE);
			if (m_pParent) m_pParent->InstrumentChanged(n);
		}
	}
}


void CCtrlInstruments::OnPrevInstrument()
//---------------------------------------
{
	if (m_pSndFile)
	{
		if (m_nInstrument > 1)
			SetCurrentInstrument(m_nInstrument-1);
		else
			SetCurrentInstrument(m_pSndFile->m_nInstruments);
		if (m_pParent) m_pParent->InstrumentChanged(m_nInstrument);
	}
}


void CCtrlInstruments::OnNextInstrument()
//---------------------------------------
{
	if (m_pSndFile)
	{
		if (m_nInstrument < m_pSndFile->m_nInstruments)
			SetCurrentInstrument(m_nInstrument+1);
		else
			SetCurrentInstrument(1);
		if (m_pParent) m_pParent->InstrumentChanged(m_nInstrument);
	}
}


void CCtrlInstruments::OnInstrumentNew()
//--------------------------------------
{
	if (m_pModDoc)
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		if ((pSndFile->m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT))
		 && (pSndFile->m_nInstruments > 0)
		 && (CMainFrame::GetInputHandler()->ShiftPressed())) //rewbs.customKeys
		{
			OnInstrumentDuplicate();
			return;
		}
		bool bFirst = (pSndFile->GetNumInstruments() == 0);
		LONG ins = m_pModDoc->InsertInstrument();
		if (ins != INSTRUMENTINDEX_INVALID)
		{
			SetCurrentInstrument(ins);
			m_pModDoc->UpdateAllViews(NULL, (ins << HINT_SHIFT_INS) | HINT_INSTRUMENT | HINT_INSNAMES | HINT_ENVELOPE);
		}
		if (bFirst) m_pModDoc->UpdateAllViews(NULL, (ins << HINT_SHIFT_INS) | HINT_MODTYPE | HINT_INSTRUMENT | HINT_INSNAMES);
		if (m_pParent) m_pParent->InstrumentChanged(m_nInstrument);
	}
	SwitchToView();
}


void CCtrlInstruments::OnInstrumentDuplicate()
//--------------------------------------------
{
	if (m_pModDoc)
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		if ((pSndFile->m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (pSndFile->m_nInstruments > 0))
		{
			BOOL bFirst = (pSndFile->m_nInstruments) ? FALSE : TRUE;
			LONG ins = m_pModDoc->InsertInstrument(INSTRUMENTINDEX_INVALID, m_nInstrument);
			if (ins != INSTRUMENTINDEX_INVALID)
			{
				SetCurrentInstrument(ins);
				m_pModDoc->UpdateAllViews(NULL, (ins << HINT_SHIFT_INS) | HINT_INSTRUMENT | HINT_INSNAMES | HINT_ENVELOPE);
			}
			if (bFirst) m_pModDoc->UpdateAllViews(NULL, (ins << HINT_SHIFT_INS) | HINT_MODTYPE | HINT_INSTRUMENT | HINT_INSNAMES);
			if (m_pParent) m_pParent->InstrumentChanged(m_nInstrument);
		}
	}
	SwitchToView();
}


void CCtrlInstruments::OnInstrumentOpen()
//---------------------------------------
{
	static int nLastIndex = 0;

	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(true, "", "",
		"All Instruments|*.xi;*.pat;*.iti;*.wav;*.aif;*.aiff|"
		"FastTracker II Instruments (*.xi)|*.xi|"
		"GF1 Patches (*.pat)|*.pat|"
		"Impulse Tracker Instruments (*.iti)|*.iti|"
		"All Files (*.*)|*.*||",
		CMainFrame::GetSettings().GetWorkingDirectory(DIR_INSTRUMENTS),
		true,
		&nLastIndex);
	if(files.abort) return;

	CMainFrame::GetSettings().SetWorkingDirectory(files.workingDirectory.c_str(), DIR_INSTRUMENTS, true);

	for(size_t counter = 0; counter < files.filenames.size(); counter++)
	{
		//If loading multiple instruments, advancing to next instrument and creating
		//new instrument if necessary.
		if(counter > 0)	
		{
			if(m_nInstrument >= MAX_INSTRUMENTS-1)
				break;
			else
				m_nInstrument++;

            if(m_nInstrument > m_pSndFile->GetNumInstruments())
				OnInstrumentNew();
		}

		if(!OpenInstrument(files.filenames[counter].c_str()))
			ErrorBox(IDS_ERR_FILEOPEN, this);
	}

	if (m_pParent) m_pParent->InstrumentChanged(m_nInstrument);
	SwitchToView();
}


void CCtrlInstruments::OnInstrumentSave()
//---------------------------------------
{
	TCHAR szFileName[_MAX_PATH] = "", drive[_MAX_DRIVE], path[_MAX_PATH], ext[_MAX_EXT];
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	
	if (!pIns) return;
	if (pIns->filename[0])
	{
		strncpy(szFileName, pIns->filename, min(CountOf(pIns->filename), CountOf(szFileName) - 1));
	} else
	{
		strncpy(szFileName, pIns->name, min(CountOf(pIns->name), CountOf(szFileName) - 1));
	}
	StringFixer::SetNullTerminator(szFileName);
	SanitizeFilename(szFileName);

	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, (m_pSndFile->GetType() == MOD_TYPE_XM) ? "xi" : "iti", szFileName,
		(m_pSndFile->GetType() == MOD_TYPE_XM) ?
			"FastTracker II Instruments (*.xi)|*.xi|"
			"Impulse Tracker Instruments (*.iti)|*.iti||" :
			"Impulse Tracker Instruments (*.iti)|*.iti|"
			"FastTracker II Instruments (*.xi)|*.xi||",
		CMainFrame::GetSettings().GetWorkingDirectory(DIR_INSTRUMENTS));
	if(files.abort) return;
	
	BeginWaitCursor();

	_splitpath(files.first_file.c_str(), drive, path, NULL, ext);
	BOOL bOk = FALSE;
	if (!lstrcmpi(ext, ".iti"))
		bOk = m_pSndFile->SaveITIInstrument(m_nInstrument, files.first_file.c_str());
	else
		bOk = m_pSndFile->SaveXIInstrument(m_nInstrument, files.first_file.c_str());

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	int n = strlen(files.first_file.c_str());
	if(n > _MAX_PATH) n = _MAX_PATH;
	strncpy(m_pSndFile->m_szInstrumentPath[m_nInstrument-1], files.first_file.c_str(),n);
	SetInstrumentModified(false);
// -! NEW_FEATURE#0023

	EndWaitCursor();
	if (!bOk) ErrorBox(IDS_ERR_SAVEINS, this); else
	{
		strcpy(szFileName, drive);
		strcat(szFileName, path);
		
		CMainFrame::GetSettings().SetWorkingDirectory(files.workingDirectory.c_str(), DIR_INSTRUMENTS);

// -> CODE#0023
// -> DESC="IT project files (.itp)"
//		m_pModDoc->UpdateAllViews(NULL, (m_nInstrument << 24) | HINT_INSTRUMENT);
		m_pModDoc->UpdateAllViews(NULL, HINT_MODTYPE | HINT_INSNAMES | HINT_SMPNAMES);
// -! NEW_FEATURE#0023
	}
	SwitchToView();
}


void CCtrlInstruments::OnInstrumentPlay()
//---------------------------------------
{
	if ((m_pModDoc) && (m_pSndFile))
	{
		if (m_pModDoc->IsNotePlaying(NOTE_MIDDLEC, 0, m_nInstrument))
		{
			m_pModDoc->NoteOff(NOTE_MIDDLEC, TRUE, m_nInstrument);
		} else
		{
			m_pModDoc->PlayNote(NOTE_MIDDLEC, m_nInstrument, 0, FALSE);
		}
	}
	SwitchToView();
}


void CCtrlInstruments::OnNameChanged()
//------------------------------------
{
	if (!IsLocked())
	{
		CHAR s[64];
		s[0] = 0;
		m_EditName.GetWindowText(s, sizeof(s));
		for (UINT i=strlen(s); i<=32; i++) s[i] = 0;
		MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
		if ((pIns) && (strncmp(s, pIns->name, 32)))
		{
			memcpy(pIns->name, s, 32);
			SetInstrumentModified(true);
		}
	}
}


void CCtrlInstruments::OnFileNameChanged()
//----------------------------------------
{
	if (!IsLocked())
	{
		CHAR s[64];
		s[0] = 0;
		m_EditFileName.GetWindowText(s, sizeof(s));
		for (UINT i=strlen(s); i<=12; i++) s[i] = 0;
		MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
		if ((pIns) && (strncmp(s, pIns->filename, 12)))
		{
			memcpy(pIns->filename, s, 12);
			SetInstrumentModified(true);
		}
	}
}


void CCtrlInstruments::OnFadeOutVolChanged()
//------------------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int minval = 0, maxval = 32767;
		m_SpinFadeOut.GetRange(minval, maxval);
		int nVol = GetDlgItemInt(IDC_EDIT7);
		Limit(nVol, minval, maxval);
		
		if(nVol != (int)pIns->nFadeOut)
		{
			pIns->nFadeOut = nVol;
			SetInstrumentModified(true);
		}
	}
}


void CCtrlInstruments::OnGlobalVolChanged()
//-----------------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int nVol = GetDlgItemInt(IDC_EDIT8);
		Limit(nVol, 0, 64);
		if (nVol != (int)pIns->nGlobalVol)
		{
			pIns->nGlobalVol = nVol;
			SetInstrumentModified(true);
		}
	}
}


void CCtrlInstruments::OnSetPanningChanged()
//------------------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		const BOOL b = m_CheckPanning.GetCheck();

		if (b) pIns->dwFlags |= INS_SETPANNING;
		else pIns->dwFlags &= ~INS_SETPANNING;

		if(b && m_pSndFile->GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			bool smpPanningInUse = false;
			for(BYTE i = 0; i < CountOf(pIns->Keyboard); i++)
			{
				const SAMPLEINDEX smp = pIns->Keyboard[i];
				if(smp <= m_pSndFile->GetNumSamples() && m_pSndFile->GetSample(smp).uFlags & CHN_PANNING)
				{
					smpPanningInUse = true;
					break;
				}
			}
			if(smpPanningInUse)
			{
				if(Reporting::Confirm(_T("Some of the samples used in the instrument have \"Set Pan\" enabled. "
						"When instrument is played with such sample, sample pan setting overrides instrument pan. "
						"Do you wish to disable panning from those samples so that instrument pan setting is effective "
						"for the whole instrument?")) == cnfYes)
				{
					for(BYTE i = 0; i < CountOf(pIns->Keyboard); i++)
					{
						const SAMPLEINDEX smp = pIns->Keyboard[i];
						if(smp <= m_pSndFile->GetNumSamples())
							m_pSndFile->GetSample(smp).uFlags &= ~CHN_PANNING;
					}
					m_pModDoc->UpdateAllViews(NULL, HINT_SAMPLEINFO | HINT_MODTYPE);
				}
			}
		}
		SetInstrumentModified(true);
	}
}


void CCtrlInstruments::OnPanningChanged()
//---------------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int nPan = GetDlgItemInt(IDC_EDIT9);
		if(m_pModDoc->GetModType() & MOD_TYPE_IT)	// IT panning ranges from 0 to 64
			nPan *= 4;
		if (nPan < 0) nPan = 0;
		if (nPan > 256) nPan = 256;
		if (nPan != (int)pIns->nPan)
		{
			pIns->nPan = nPan;
			SetInstrumentModified(true);
		}
	}
}


void CCtrlInstruments::OnNNAChanged()
//-----------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		if (pIns->nNNA != m_ComboNNA.GetCurSel())
		{
            pIns->nNNA = m_ComboNNA.GetCurSel();
			SetInstrumentModified(true);
		}		
	}
}
	
	
void CCtrlInstruments::OnDCTChanged()
//-----------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		if (pIns->nDCT != m_ComboDCT.GetCurSel())
		{
			pIns->nDCT = m_ComboDCT.GetCurSel();
			SetInstrumentModified(true);
		}
	}
}
	

void CCtrlInstruments::OnDCAChanged()
//-----------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		if (pIns->nDNA != m_ComboDCA.GetCurSel())
		{
			pIns->nDNA = m_ComboDCA.GetCurSel();
			SetInstrumentModified(true);
		}
	}
}


void CCtrlInstruments::OnMPRChanged()
//-----------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int n = GetDlgItemInt(IDC_EDIT10);
		if ((n >= 0) && (n <= 255))
		{
			if (pIns->nMidiProgram != n)
			{
				pIns->nMidiProgram = n;
				SetInstrumentModified(true);
			}
		}
		//rewbs.MidiBank: we will not set the midi bank/program if it is 0
		if (n==0)
		{	
			LockControls();
			SetDlgItemText(IDC_EDIT10, "---");
			UnlockControls();
		}
		//end rewbs.MidiBank
	}
}

//rewbs.MidiBank
void CCtrlInstruments::OnMBKChanged()
//-----------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		WORD w = GetDlgItemInt(IDC_EDIT11);
		if ((w >= 0) && (w <= 255))
		{
			if (pIns->wMidiBank != w)
			{
				pIns->wMidiBank = w;
				SetInstrumentModified(true);
			}
		}
		//rewbs.MidiBank: we will not set the midi bank/program if it is 0
		if (w==0)
		{	
			LockControls();
			SetDlgItemText(IDC_EDIT11, "---");
			UnlockControls();
		}
		//end rewbs.MidiBank
	}
}
//end rewbs.MidiBank

void CCtrlInstruments::OnMCHChanged()
//-----------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int n = m_CbnMidiCh.GetItemData(m_CbnMidiCh.GetCurSel());
		if (pIns->nMidiChannel != (BYTE)(n & 0xff))
		{
			pIns->nMidiChannel = (BYTE)(n & 0xff);
			SetInstrumentModified(true);
		}
	}
}

void CCtrlInstruments::OnResamplingChanged() 
//------------------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int n = m_CbnResampling.GetItemData(m_CbnResampling.GetCurSel());
		if (pIns->nResampling != (BYTE)(n & 0xff))
		{
			pIns->nResampling = (BYTE)(n & 0xff);
			SetInstrumentModified(true);
			m_pModDoc->UpdateAllViews(NULL, HINT_INSNAMES, this);
		}
	}
}


//rewbs.instroVSTi
void CCtrlInstruments::OnMixPlugChanged()
//---------------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	PLUGINDEX nPlug = static_cast<PLUGINDEX>(m_CbnMixPlug.GetItemData(m_CbnMixPlug.GetCurSel()));

	if (pIns)
	{
		if(nPlug < 1 || m_pSndFile->GetModFlag(MSF_MIDICC_BUGEMULATION))
		{
			m_CbnPluginVelocityHandling.EnableWindow(FALSE);
			m_CbnPluginVolumeHandling.EnableWindow(FALSE);
		}
		else
		{
			m_CbnPluginVelocityHandling.EnableWindow();
			m_CbnPluginVolumeHandling.EnableWindow();
		}

		if (nPlug>=0 && nPlug <= MAX_MIXPLUGINS)
		{
			if ((!IsLocked()) && pIns->nMixPlug != nPlug)
			{ 
				pIns->nMixPlug = nPlug;
				SetInstrumentModified(true);
				m_pModDoc->UpdateAllViews(NULL, HINT_MIXPLUGINS, this);
			}
			m_CbnPluginVelocityHandling.SetCurSel(pIns->nPluginVelocityHandling);
			m_CbnPluginVolumeHandling.SetCurSel(pIns->nPluginVolumeHandling);

			if (pIns->nMixPlug)	//if we have not just set to no plugin
			{
				PSNDMIXPLUGIN pPlug = &(m_pSndFile->m_MixPlugins[pIns->nMixPlug - 1]);
				if ((pPlug == nullptr || pPlug->pMixPlugin == nullptr) && !IsLocked())
				{
					// No plugin in this slot: Ask user to add one.
#ifndef NO_VST
					CSelectPluginDlg dlg(m_pModDoc, nPlug - 1, this); 
					if (dlg.DoModal() == IDOK)
					{
						if(m_pSndFile->GetModSpecifications().supportsPlugins)
						{
							m_pModDoc->SetModified();
						}
						UpdatePluginList();
						m_pModDoc->UpdateAllViews(NULL, HINT_MIXPLUGINS, NULL);
					}
#endif // NO_VST
				}

				if (pPlug && pPlug->pMixPlugin)
				{
					::EnableWindow(::GetDlgItem(m_hWnd, IDC_INSVIEWPLG), true);
					
					// if this plug can recieve MIDI events and we have no MIDI channel
					// selected for this instrument, automatically select MIDI channel 1.
					if (pPlug->pMixPlugin->isInstrument() && pIns->nMidiChannel == 0)
					{
						pIns->nMidiChannel = 1;
						UpdateView((m_nInstrument << HINT_SHIFT_INS) | HINT_INSTRUMENT, NULL);
					}
					return;
				}
			}
		}
		
	}
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_INSVIEWPLG), false);
}
//end rewbs.instroVSTi



void CCtrlInstruments::OnPPSChanged()
//-----------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int n = GetDlgItemInt(IDC_EDIT15);
		if ((n >= -32) && (n <= 32)) {
			if (pIns->nPPS != (signed char)n)
			{
				pIns->nPPS = (signed char)n;
				SetInstrumentModified(true);
			}
		}
	}
}

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
void CCtrlInstruments::OnAttackChanged()
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if(!IsLocked() && pIns){
		int n = GetDlgItemInt(IDC_EDIT2);
		if(n < 0) n = 0;
		if(n > MAX_ATTACK_VALUE) n = MAX_ATTACK_VALUE;
		int newRamp = n; //? MAX_ATTACK_LENGTH - n : 0;

		if(pIns->nVolRampUp != newRamp)
		{
			pIns->nVolRampUp = newRamp;
			SetInstrumentModified(true);
		}

		m_SliderAttack.SetPos(n);
		if( CSpinButtonCtrl *spin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN1) ) spin->SetPos(n);
		LockControls();
		if (n == 0) SetDlgItemText(IDC_EDIT2,"default");
		UnlockControls();
	}
}
// -! NEW_FEATURE#0027


void CCtrlInstruments::OnPPCChanged()
//-----------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int n = m_ComboPPC.GetCurSel();
		if ((n >= 0) && (n <= NOTE_MAX - 1))
		{
			if (pIns->nPPC != n)
			{
				pIns->nPPC = n;
				SetInstrumentModified(true);
			}
		}
	}
}


void CCtrlInstruments::OnEnableCutOff()
//-------------------------------------
{
	BOOL bCutOff = IsDlgButtonChecked(IDC_CHECK2);

	if (m_pModDoc)
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			if (bCutOff)
			{
				pIns->nIFC |= 0x80;
			} else
			{
				pIns->nIFC &= 0x7F;
			}
			for (UINT i=0; i<MAX_CHANNELS; i++)
			{
				if (pSndFile->Chn[i].pModInstrument == pIns)
				{
					if (bCutOff)
					{
						pSndFile->Chn[i].nCutOff = pIns->nIFC & 0x7f;
					} else
					{
						pSndFile->Chn[i].nCutOff = 0x7f;
					}
				}
			}
		}
		SetInstrumentModified(true);
		SwitchToView();
	}
}


void CCtrlInstruments::OnEnableResonance()
//----------------------------------------
{
	BOOL bReso = IsDlgButtonChecked(IDC_CHECK3);

	if (m_pModDoc)
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			if (bReso)
			{
				pIns->nIFR |= 0x80;
			} else
			{
				pIns->nIFR &= 0x7F;
			}
			for (UINT i=0; i<MAX_CHANNELS; i++)
			{
				if (pSndFile->Chn[i].pModInstrument == pIns)
				{
					if (bReso)
					{
						pSndFile->Chn[i].nResonance = pIns->nIFC & 0x7f;
					} else
					{
						pSndFile->Chn[i].nResonance = 0;
					}
				}
			}
		}
		SetInstrumentModified(true);
		SwitchToView();
	}
}

void CCtrlInstruments::OnFilterModeChanged() 
//------------------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		int instFiltermode = m_CbnFilterMode.GetItemData(m_CbnFilterMode.GetCurSel());
		if (!m_pModDoc)
			return;

		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];

		if (pIns)
		{
			
			pIns->nFilterMode = instFiltermode;
			m_pModDoc->SetModified();

			// Translate from mode as stored in instrument to mode as understood by player. 
			// (The reason for the translation is that the player treats 0 as lowpass,
			// but we need to keep 0 as "do not change", so that the instrument setting doesn't
			// override the channel setting by default.)
			/*int playerFilterMode=-1;
			switch (instFilterMode) {
				case INST_FILTERMODE_DEFAULT:  playerFilterMode = FLTMODE_UNCHANGED;break;
				case INST_FILTERMODE_HIGHPASS: playerFilterMode = FLTMODE_HIGHPASS; break;
				case INST_FILTERMODE_LOWPASS:  playerFilterMode = FLTMODE_LOWPASS;  break;
			}*/

            //Update channel settings where this instrument is active, if required.
			if(instFiltermode != FLTMODE_UNCHANGED)
			{
				for (CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
				{
					if (pSndFile->Chn[i].pModInstrument == pIns)
						pSndFile->Chn[i].nFilterMode = instFiltermode;
				}
			}

		}

	}
}


void CCtrlInstruments::OnVScroll(UINT nCode, UINT nPos, CScrollBar *pSB)
//----------------------------------------------------------------------
{
	CModControlDlg::OnVScroll(nCode, nPos, pSB);
	if (nCode == SB_ENDSCROLL) SwitchToView();
}


void CCtrlInstruments::OnHScroll(UINT nCode, UINT nPos, CScrollBar *pSB)
//----------------------------------------------------------------------
{
	CModControlDlg::OnHScroll(nCode, nPos, pSB);
	if ((m_nInstrument) && (m_pModDoc) && (!IsLocked()) && (nCode != SB_ENDSCROLL))
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];

		if (pIns)
		{
		//Various optimisations by rewbs
			CSliderCtrl* pSlider = (CSliderCtrl*) pSB;
			short int n;
			bool filterChanger = false;

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
			// Volume ramping (attack)
			if (pSlider==&m_SliderAttack)
			{
				n = m_SliderAttack.GetPos();
				int newRamp = n; //? MAX_ATTACK_LENGTH - n : 0;
				if(pIns->nVolRampUp != newRamp)
				{
					pIns->nVolRampUp = newRamp;
					SetDlgItemInt(IDC_EDIT2,n);
					SetInstrumentModified(true);
				}
// -! NEW_FEATURE#0027
			} 
			// Volume Swing
			else if (pSlider==&m_SliderVolSwing) 
			{
				n = m_SliderVolSwing.GetPos();
				if ((n >= 0) && (n <= 64) && (n != (int)pIns->nVolSwing))
				{
					pIns->nVolSwing = (BYTE)n;
					SetInstrumentModified(true);
				}
			}
			// Pan Swing
			else if (pSlider==&m_SliderPanSwing) 
			{
				n = m_SliderPanSwing.GetPos();
				if ((n >= 0) && (n <= 64) && (n != (int)pIns->nPanSwing))
				{
					pIns->nPanSwing = (BYTE)n;
					SetInstrumentModified(true);
				}
			}
			//Cutoff swing
			else if (pSlider==&m_SliderCutSwing) 
			{
				n = m_SliderCutSwing.GetPos();
				if ((n >= 0) && (n <= 64) && (n != (int)pIns->nCutSwing))
				{
					pIns->nCutSwing = (BYTE)n;
					SetInstrumentModified(true);
				}
			}
			//Resonance swing
			else if (pSlider==&m_SliderResSwing) 
			{
				n = m_SliderResSwing.GetPos();
				if ((n >= 0) && (n <= 64) && (n != (int)pIns->nResSwing))
				{
					pIns->nResSwing = (BYTE)n;
					SetInstrumentModified(true);
				}
			}
			// Filter CutOff
			else if (pSlider==&m_SliderCutOff)
			{
				n = m_SliderCutOff.GetPos();
				if ((n >= 0) && (n < 0x80) && (n != (int)(pIns->nIFC & 0x7F)))
				{
					pIns->nIFC &= 0x80;
					pIns->nIFC |= (BYTE)n;
					SetInstrumentModified(true);
					UpdateFilterText();
					filterChanger = true;
				}
			}
			else if (pSlider==&m_SliderResonance)
			{
				// Filter Resonance
				n = m_SliderResonance.GetPos();
				if ((n >= 0) && (n < 0x80) && (n != (int)(pIns->nIFR & 0x7F)))
				{
					pIns->nIFR &= 0x80;
					pIns->nIFR |= (BYTE)n;
					SetInstrumentModified(true);
					filterChanger = true;
				}
			}
			
			// Update channels
			if (filterChanger==true)
			{
				for (UINT i=0; i<MAX_CHANNELS; i++)
				{
					if (pSndFile->Chn[i].pModInstrument == pIns)
					{
						if (pIns->nIFC & 0x80) pSndFile->Chn[i].nCutOff = pIns->nIFC & 0x7F;
						if (pIns->nIFR & 0x80) pSndFile->Chn[i].nResonance = pIns->nIFR & 0x7F;
					}
				}
			}
		
// -> CODE#0023
// -> DESC="IT project files (.itp)"
			m_pModDoc->UpdateAllViews(NULL, HINT_INSNAMES, this);
// -! NEW_FEATURE#0023
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
	if ((m_nInstrument) && (m_pModDoc))
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		MODINSTRUMENT *pIns = pSndFile->Instruments[m_nInstrument];
		if (pIns)
		{
			CSampleMapDlg dlg(pSndFile, m_nInstrument, this);
			if (dlg.DoModal() == IDOK)
			{
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, (m_nInstrument << HINT_SHIFT_INS) | HINT_INSTRUMENT, this);
				m_NoteMap.InvalidateRect(NULL, FALSE);
			}
		}
	}
}

//rewbs.instroVSTi
void CCtrlInstruments::TogglePluginEditor()
//----------------------------------------
{
	if ((m_nInstrument) && (m_pModDoc))
	{
		m_pModDoc->TogglePluginEditor(m_CbnMixPlug.GetItemData(m_CbnMixPlug.GetCurSel())-1);
	}
}
//end rewbs.instroVSTi

//rewbs.customKeys
BOOL CCtrlInstruments::PreTranslateMessage(MSG *pMsg)
//-----------------------------------------------
{
	if (pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) || 
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();
			
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
	if (wParam == kcNull)
		return NULL;
	
	switch(wParam)
	{
		case kcInstrumentCtrlLoad: OnInstrumentOpen(); return wParam;
		case kcInstrumentCtrlSave: OnInstrumentSave(); return wParam;
		case kcInstrumentCtrlNew:  OnInstrumentNew();  return wParam;

		case kcInstrumentCtrlDuplicate:	OnInstrumentDuplicate(); return wParam;
	}
	
	return 0;
}

//end rewbs.customKeys

void CCtrlInstruments::OnCbnSelchangeCombotuning()
//------------------------------------------------
{
	if (IsLocked() || m_pModDoc == NULL || m_pSndFile == NULL) return;

	MODINSTRUMENT* pInstH = m_pSndFile->Instruments[m_nInstrument];
	if(pInstH == 0)
		return;

	size_t sel = m_ComboTuning.GetCurSel();
	if(sel == 0) //Setting IT behavior
	{
		CriticalSection cs;
		pInstH->SetTuning(NULL);
		cs.Leave();

		m_pModDoc->SetModified();
		UpdateView((m_nInstrument << HINT_SHIFT_INS) | HINT_INSTRUMENT);
		return;
	}

	sel -= 1;
	CTuningCollection* tc = 0;
	
	if(sel < CSoundFile::GetBuiltInTunings().GetNumTunings())
		tc = &CSoundFile::GetBuiltInTunings();
	else
	{
		sel -= CSoundFile::GetBuiltInTunings().GetNumTunings();
		if(sel < CSoundFile::GetLocalTunings().GetNumTunings())
			tc = &CSoundFile::GetLocalTunings();
		else
		{
			sel -= CSoundFile::GetLocalTunings().GetNumTunings();
			if(sel < m_pSndFile->GetTuneSpecificTunings().GetNumTunings())
				tc = &m_pSndFile->GetTuneSpecificTunings();
		}
	}

	if(tc)
	{
		CriticalSection cs;
		pInstH->SetTuning(&tc->GetTuning(sel));
		cs.Leave();

		m_pModDoc->SetModified();
		UpdateView((m_nInstrument << HINT_SHIFT_INS) | HINT_INSTRUMENT);
		return;
	}

	//Case: Chosen tuning editor to be displayed.
	//Creating vector for the CTuningDialog.
	vector<CTuningCollection*> v;
	v.push_back(&m_pSndFile->GetBuiltInTunings());
	v.push_back(&m_pSndFile->GetLocalTunings());
	v.push_back(&m_pSndFile->GetTuneSpecificTunings());
	CTuningDialog td(this, v, pInstH->pTuning);
	td.DoModal();
	if(td.GetModifiedStatus(&m_pSndFile->GetLocalTunings()))
	{
		if(MsgBox(IDS_APPLY_TUNING_MODIFICATIONS, this, "", MB_OKCANCEL) == IDOK)
			m_pSndFile->SaveStaticTunings();
	}
	if(td.GetModifiedStatus(&m_pSndFile->GetTuneSpecificTunings()))
	{
		m_pModDoc->SetModified();
	}

	//Recreating tuning combobox so that possible
	//new tuning(s) come visible.
	BuildTuningComboBox();
	
	UpdateView((m_nInstrument << HINT_SHIFT_INS) | HINT_INSTRUMENT);
}


void CCtrlInstruments::UpdateTuningComboBox()
//-------------------------------------------
{
	if (m_pModDoc == 0 || m_pSndFile == 0 
		|| m_nInstrument > m_pSndFile->GetNumInstruments()
		|| m_pSndFile->Instruments[m_nInstrument] == nullptr) return;

	MODINSTRUMENT* const pIns = m_pSndFile->Instruments[m_nInstrument];
	if(pIns->pTuning == nullptr)
	{
		m_ComboTuning.SetCurSel(0);
		return;
	}

	for(size_t i = 0; i < CSoundFile::GetBuiltInTunings().GetNumTunings(); i++)
	{
		if(pIns->pTuning == &CSoundFile::GetBuiltInTunings().GetTuning(i))
		{
			m_ComboTuning.SetCurSel(i+1);
			return;
		}
	}

	for(size_t i = 0; i < CSoundFile::GetLocalTunings().GetNumTunings(); i++)
	{
		if(pIns->pTuning == &CSoundFile::GetLocalTunings().GetTuning(i))
		{
			m_ComboTuning.SetCurSel(i+CSoundFile::GetBuiltInTunings().GetNumTunings()+1);
			return;
		}
	}

	for(size_t i = 0; i < m_pSndFile->GetTuneSpecificTunings().GetNumTunings(); i++)
	{
		if(pIns->pTuning == &m_pSndFile->GetTuneSpecificTunings().GetTuning(i))
		{
			m_ComboTuning.SetCurSel(i+CSoundFile::GetBuiltInTunings().GetNumTunings() + CSoundFile::GetLocalTunings().GetNumTunings()+1);
			return;
		}
	}

	CString str;
	str.Format(TEXT("Tuning %s was not found. Setting to default tuning."), m_pSndFile->Instruments[m_nInstrument]->pTuning->GetName().c_str());
	Reporting::Notification(str);

	CriticalSection cs;
	pIns->SetTuning(pIns->s_DefaultTuning);

	m_pModDoc->SetModified();
}

void CCtrlInstruments::OnEnChangeEditPitchtempolock()
//----------------------------------------------------
{
	if(IsLocked() || !m_pModDoc || !m_pSndFile || !m_nInstrument || !m_pSndFile->Instruments[m_nInstrument]) return;

	const TEMPO MINTEMPO = m_pSndFile->GetModSpecifications().tempoMin;
	const TEMPO MAXTEMPO = m_pSndFile->GetModSpecifications().tempoMax;
	char buffer[7];
	m_EditPitchTempoLock.GetWindowText(buffer, 6);
	int ptlTempo = atoi(buffer);
	if(ptlTempo < MINTEMPO)
		ptlTempo = MINTEMPO;
	if(ptlTempo > MAXTEMPO)
		ptlTempo = MAXTEMPO;
	
	CriticalSection cs;
	m_pSndFile->Instruments[m_nInstrument]->wPitchToTempoLock = ptlTempo;

	m_pModDoc->SetModified();
}


void CCtrlInstruments::OnPluginVelocityHandlingChanged()
//------------------------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		BYTE n = static_cast<BYTE>(m_CbnPluginVelocityHandling.GetCurSel());
		if(n != pIns->nPluginVelocityHandling)
		{
			pIns->nPluginVelocityHandling = n;
			SetInstrumentModified(true);
		}
	}
}


void CCtrlInstruments::OnPluginVolumeHandlingChanged()
//----------------------------------------------
{
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((!IsLocked()) && (pIns))
	{
		BYTE n = static_cast<BYTE>(m_CbnPluginVolumeHandling.GetCurSel());
		if(n != pIns->nPluginVolumeHandling)
		{
			pIns->nPluginVolumeHandling = n;
			SetInstrumentModified(true);
		}
	}
}


void CCtrlInstruments::OnBnClickedCheckPitchtempolock()
//-----------------------------------------------------
{
	if(!m_pSndFile || !m_nInstrument || !m_pSndFile->Instruments[m_nInstrument])
		return;

	if(IsDlgButtonChecked(IDC_CHECK_PITCHTEMPOLOCK))
	{
		
		MODINSTRUMENT* pIns = m_pSndFile->Instruments[m_nInstrument];
		if(!pIns)
			return;

		//Checking what value to put for the wPitchToTempoLock.
		m_EditPitchTempoLock.EnableWindow();
		WORD ptl = pIns->wPitchToTempoLock;
		if(ptl == 0)
		{
			if(m_EditPitchTempoLock.GetWindowTextLength() > 0)
			{
				char buffer[7];
				m_EditPitchTempoLock.GetWindowText(buffer, 6);
				ptl = atoi(buffer);
			}
			else
				ptl = m_pSndFile->m_nDefaultTempo;
		}
		m_EditPitchTempoLock.SetWindowText(Stringify(ptl).c_str());
		//SetModified() comes with SetWindowText(.).
	}
	else
	{
		m_EditPitchTempoLock.EnableWindow(FALSE);
		if(m_pSndFile && m_nInstrument && m_pSndFile->Instruments[m_nInstrument] &&
			m_pSndFile->Instruments[m_nInstrument]->wPitchToTempoLock > 0)
		{
			CriticalSection cs;
			m_pSndFile->Instruments[m_nInstrument]->wPitchToTempoLock = 0;

			m_pModDoc->SetModified();
		}
	}
}


void CCtrlInstruments::OnEnKillfocusEditPitchtempolock()
//------------------------------------------------------
{
	//Checking that tempo value is in correct range.

	if(!m_pSndFile || IsLocked()) return;

	char buffer[6];
	m_EditPitchTempoLock.GetWindowText(buffer, 5);
	int ptlTempo = atoi(buffer);
	bool changed = false;
	const CModSpecifications& specs = m_pSndFile->GetModSpecifications();

	if(ptlTempo < specs.tempoMin)
	{
		ptlTempo = specs.tempoMin;
		changed = true;
	}
	if(ptlTempo > specs.tempoMax)
	{
		ptlTempo = specs.tempoMax;
		changed = true;

	}
	if(changed) m_EditPitchTempoLock.SetWindowText(Stringify(ptlTempo).c_str());
}


void CCtrlInstruments::BuildTuningComboBox()
//------------------------------------------
{
	while(m_ComboTuning.GetCount() > 0)
		m_ComboTuning.DeleteString(0);

	m_ComboTuning.AddString("OMPT IT behavior"); //<-> Instrument pTuning pointer == NULL
	for(size_t i = 0; i<CSoundFile::GetBuiltInTunings().GetNumTunings(); i++)
	{
		m_ComboTuning.AddString(CSoundFile::GetBuiltInTunings().GetTuning(i).GetName().c_str());
	}
	for(size_t i = 0; i<CSoundFile::GetLocalTunings().GetNumTunings(); i++)
	{
		m_ComboTuning.AddString(CSoundFile::GetLocalTunings().GetTuning(i).GetName().c_str());
	}
	for(size_t i = 0; i<m_pSndFile->GetTuneSpecificTunings().GetNumTunings(); i++)
	{
		m_ComboTuning.AddString(m_pSndFile->GetTuneSpecificTunings().GetTuning(i).GetName().c_str());
	}
	m_ComboTuning.AddString("Control tunings...");
	m_ComboTuning.SetCurSel(0);
}


void CCtrlInstruments::UpdatePluginList()
//---------------------------------------
{
	//Update plugin list
	m_CbnMixPlug.Clear();
	m_CbnMixPlug.ResetContent();
	CHAR s[64];
	for (PLUGINDEX nPlug = 0; nPlug <= MAX_MIXPLUGINS; nPlug++)
	{
		if (!nPlug) 
		{ 
			strcpy(s, "No plugin");
		} 
		else
		{
			PSNDMIXPLUGIN p = &(m_pSndFile->m_MixPlugins[nPlug - 1]);
			p->Info.szLibraryName[63] = 0;
			if (p->Info.szLibraryName[0])
				wsprintf(s, "FX%d: %s", nPlug, p->Info.szName);
			else
				wsprintf(s, "FX%d: undefined", nPlug);
		}

		m_CbnMixPlug.SetItemData(m_CbnMixPlug.AddString(s), nPlug);
	}
	MODINSTRUMENT *pIns = m_pSndFile->Instruments[m_nInstrument];
	if ((pIns) && (pIns->nMixPlug <= MAX_MIXPLUGINS)) m_CbnMixPlug.SetCurSel(pIns->nMixPlug);
}
