/*
 * Mainbar.cpp
 * -----------
 * Purpose: Implementation of OpenMPT's window toolbar.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "View_tre.h"
#include "ImageLists.h"
#include "Moddoc.h"
#include "../soundlib/mod_specifications.h"
#include "../common/mptStringBuffer.h"


OPENMPT_NAMESPACE_BEGIN


/////////////////////////////////////////////////////////////////////
// CToolBarEx: custom toolbar base class

void CToolBarEx::SetHorizontal()
{
	m_bVertical = false;
	SetBarStyle(GetBarStyle() | CBRS_ALIGN_TOP);
}


void CToolBarEx::SetVertical()
{
	m_bVertical = true;
}


CSize CToolBarEx::CalcDynamicLayout(int nLength, DWORD dwMode)
{
	CSize sizeResult;
	// if we're committing set the buttons appropriately
	if(dwMode & LM_COMMIT)
	{
		if(dwMode & LM_VERTDOCK)
		{
			if(!m_bVertical)
				SetVertical();
		} else
		{
			if(m_bVertical)
				SetHorizontal();
		}
		sizeResult = CToolBar::CalcDynamicLayout(nLength, dwMode);
	} else
	{
		const bool wasVertical = m_bVertical;
		const bool doSwitch = (dwMode & LM_HORZ) ? wasVertical : !wasVertical;

		if(doSwitch)
		{
			if(wasVertical)
				SetHorizontal();
			else
				SetVertical();
		}

		sizeResult = CToolBar::CalcDynamicLayout(nLength, dwMode);

		if(doSwitch)
		{
			if(wasVertical)
				SetHorizontal();
			else
				SetVertical();
		}
	}

	return sizeResult;
}


BOOL CToolBarEx::EnableControl(CWnd &wnd, UINT nIndex, UINT nHeight)
{
	if(wnd.m_hWnd != NULL)
	{
		CRect rect;
		GetItemRect(nIndex, rect);
		if(nHeight)
		{
			int n = (rect.bottom + rect.top - nHeight) / 2;
			if(n > rect.top) rect.top = n;
		}
		wnd.SetWindowPos(NULL, rect.left, rect.top, 0, 0, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOCOPYBITS);
		wnd.ShowWindow(SW_SHOW);
	}
	return TRUE;
}


void CToolBarEx::ChangeCtrlStyle(LONG lStyle, BOOL bSetStyle)
{
	if(m_hWnd)
	{
		LONG lStyleOld = GetWindowLong(m_hWnd, GWL_STYLE);
		if(bSetStyle)
			lStyleOld |= lStyle;
		else
			lStyleOld &= ~lStyle;
		SetWindowLong(m_hWnd, GWL_STYLE, lStyleOld);
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE);
		Invalidate();
	}
}


void CToolBarEx::EnableFlatButtons(BOOL bFlat)
{
	m_bFlatButtons = bFlat ? true : false;
	ChangeCtrlStyle(TBSTYLE_FLAT, bFlat);
}


/////////////////////////////////////////////////////////////////////
// CMainToolBar

#define SCALEWIDTH(x) (Util::ScalePixels(x, m_hWnd))
#define SCALEHEIGHT(x) (Util::ScalePixels(x, m_hWnd))

// Play Command
#define PLAYCMD_INDEX		10
#define TOOLBAR_IMAGE_PAUSE	8
#define TOOLBAR_IMAGE_PLAY	13
// Base octave
#define EDITOCTAVE_INDEX	13
#define EDITOCTAVE_WIDTH	SCALEWIDTH(55)
#define EDITOCTAVE_HEIGHT	SCALEHEIGHT(20)
#define SPINOCTAVE_INDEX	(EDITOCTAVE_INDEX+1)
#define SPINOCTAVE_WIDTH	SCALEWIDTH(16)
#define SPINOCTAVE_HEIGHT	(EDITOCTAVE_HEIGHT)
// Static "Tempo:"
#define TEMPOTEXT_INDEX		16
#define TEMPOTEXT_WIDTH		SCALEWIDTH(45)
#define TEMPOTEXT_HEIGHT	SCALEHEIGHT(20)
// Edit Tempo
#define EDITTEMPO_INDEX		(TEMPOTEXT_INDEX+1)
#define EDITTEMPO_WIDTH		SCALEWIDTH(48)
#define EDITTEMPO_HEIGHT	SCALEHEIGHT(20)
// Spin Tempo
#define SPINTEMPO_INDEX		(EDITTEMPO_INDEX+1)
#define SPINTEMPO_WIDTH		SCALEWIDTH(16)
#define SPINTEMPO_HEIGHT	(EDITTEMPO_HEIGHT)
// Static "Speed:"
#define SPEEDTEXT_INDEX		20
#define SPEEDTEXT_WIDTH		SCALEWIDTH(57)
#define SPEEDTEXT_HEIGHT	(TEMPOTEXT_HEIGHT)
// Edit Speed
#define EDITSPEED_INDEX		(SPEEDTEXT_INDEX+1)
#define EDITSPEED_WIDTH		SCALEWIDTH(28)
#define EDITSPEED_HEIGHT	(EDITTEMPO_HEIGHT)
// Spin Speed
#define SPINSPEED_INDEX		(EDITSPEED_INDEX+1)
#define SPINSPEED_WIDTH		SCALEWIDTH(16)
#define SPINSPEED_HEIGHT	(EDITSPEED_HEIGHT)
// Static "Rows/Beat:"
#define RPBTEXT_INDEX		24
#define RPBTEXT_WIDTH		SCALEWIDTH(63)
#define RPBTEXT_HEIGHT		(TEMPOTEXT_HEIGHT)
// Edit Speed
#define EDITRPB_INDEX		(RPBTEXT_INDEX+1)
#define EDITRPB_WIDTH		SCALEWIDTH(28)
#define EDITRPB_HEIGHT		(EDITTEMPO_HEIGHT)
// Spin Speed
#define SPINRPB_INDEX		(EDITRPB_INDEX+1)
#define SPINRPB_WIDTH		SCALEWIDTH(16)
#define SPINRPB_HEIGHT		(EDITRPB_HEIGHT)
// VU Meters
#define VUMETER_INDEX		(SPINRPB_INDEX+6)
#define VUMETER_WIDTH		SCALEWIDTH(255)
#define VUMETER_HEIGHT		SCALEHEIGHT(19)

static UINT MainButtons[] =
{
	// same order as in the bitmap 'mainbar.bmp'
	ID_FILE_NEW,
	ID_FILE_OPEN,
	ID_FILE_SAVE,
		ID_SEPARATOR,
	ID_EDIT_CUT,
	ID_EDIT_COPY,
	ID_EDIT_PASTE,
		ID_SEPARATOR,
	ID_MIDI_RECORD,
	ID_PLAYER_STOP,
	ID_PLAYER_PAUSE,
	ID_PLAYER_PLAYFROMSTART,
		ID_SEPARATOR,
		ID_SEPARATOR,
		ID_SEPARATOR,
		ID_SEPARATOR,
		ID_SEPARATOR,
		ID_SEPARATOR,
		ID_SEPARATOR,
		ID_SEPARATOR,
		ID_SEPARATOR,
		ID_SEPARATOR,
		ID_SEPARATOR,
		ID_SEPARATOR,

		ID_SEPARATOR,
		ID_SEPARATOR,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_VIEW_OPTIONS,
	ID_PANIC,
	ID_UPDATE_AVAILABLE,
	ID_SEPARATOR,
		ID_SEPARATOR,	// VU Meter
};


enum { MAX_MIDI_DEVICES = 256 };

BEGIN_MESSAGE_MAP(CMainToolBar, CToolBarEx)
	ON_WM_VSCROLL()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xFFFF, &CMainToolBar::OnToolTipText)
	ON_NOTIFY_REFLECT(TBN_DROPDOWN, &CMainToolBar::OnTbnDropDownToolBar)
	ON_COMMAND_RANGE(ID_SELECT_MIDI_DEVICE, ID_SELECT_MIDI_DEVICE + MAX_MIDI_DEVICES, &CMainToolBar::OnSelectMIDIDevice)
END_MESSAGE_MAP()


template<typename TWnd>
static bool CreateTextWnd(TWnd &wnd, const TCHAR *text, DWORD style, CWnd *parent, UINT id)
{
	auto dc = parent->GetDC();
	auto oldFont = dc->SelectObject(CMainFrame::GetGUIFont());
	const auto size = dc->GetTextExtent(text);
	dc->SelectObject(oldFont);
	parent->ReleaseDC(dc);
	CRect rect{0, 0, size.cx + Util::ScalePixels(10, *parent), std::max(static_cast<int>(size.cy) + Util::ScalePixels(4, *parent), Util::ScalePixels(20, *parent))};
	return wnd.Create(text, style, rect, parent, id) != FALSE;
}

BOOL CMainToolBar::Create(CWnd *parent)
{
	CRect rect;
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY;

	if(!CToolBar::Create(parent, dwStyle))
		return FALSE;

	CDC *dc = GetDC();
	const auto hFont     = reinterpret_cast<WPARAM>(CMainFrame::GetGUIFont());
	const double scaling = Util::GetDPIx(m_hWnd) / 96.0;
	const int imgSize = mpt::saturate_round<int>(16 * scaling), btnSizeX = mpt::saturate_round<int>(23 * scaling), btnSizeY = mpt::saturate_round<int>(22 * scaling);
	m_ImageList.Create(IDB_MAINBAR, 16, 16, IMGLIST_NUMIMAGES, 1, dc, scaling, false);
	m_ImageListDisabled.Create(IDB_MAINBAR, 16, 16, IMGLIST_NUMIMAGES, 1, dc, scaling, true);
	ReleaseDC(dc);
	GetToolBarCtrl().SetBitmapSize(CSize(imgSize, imgSize));
	GetToolBarCtrl().SetButtonSize(CSize(btnSizeX, btnSizeY));
	GetToolBarCtrl().SetImageList(&m_ImageList);
	GetToolBarCtrl().SetDisabledImageList(&m_ImageListDisabled);
	SendMessage(WM_SETFONT, hFont, TRUE);

	if(!SetButtons(MainButtons, mpt::saturate_cast<int>(std::size(MainButtons)))) return FALSE;

	CRect temp;
	GetItemRect(0, temp);
	SetSizes(CSize(temp.Width(), temp.Height()), CSize(imgSize, imgSize));

	// Dropdown menus for New and MIDI buttons
	LPARAM dwExStyle = GetToolBarCtrl().SendMessage(TB_GETEXTENDEDSTYLE) | TBSTYLE_EX_DRAWDDARROWS;
	GetToolBarCtrl().SendMessage(TB_SETEXTENDEDSTYLE, 0, dwExStyle);
	SetButtonStyle(CommandToIndex(ID_FILE_NEW), GetButtonStyle(CommandToIndex(ID_FILE_NEW)) | TBSTYLE_DROPDOWN);
	SetButtonStyle(CommandToIndex(ID_MIDI_RECORD), GetButtonStyle(CommandToIndex(ID_MIDI_RECORD)) | TBSTYLE_DROPDOWN);

	nCurrentSpeed = 6;
	nCurrentTempo.Set(125);
	nCurrentRowsPerBeat = 4;
	nCurrentOctave = -1;

	// Octave Edit Box
	if(!CreateTextWnd(m_EditOctave, _T("Octave 9"), WS_CHILD | WS_BORDER | SS_LEFT | SS_CENTERIMAGE, this, IDC_EDIT_BASEOCTAVE)) return FALSE;
	rect.SetRect(0, 0, SPINOCTAVE_WIDTH, SPINOCTAVE_HEIGHT);
	m_SpinOctave.Create(WS_CHILD | UDS_ALIGNRIGHT, rect, this, IDC_SPIN_BASEOCTAVE);

	// Tempo Text
	if(!CreateTextWnd(m_StaticTempo, _T("Tempo:"), WS_CHILD | SS_CENTER | SS_CENTERIMAGE, this, IDC_TEXT_CURRENTTEMPO)) return FALSE;
	// Tempo EditBox
	if(!CreateTextWnd(m_EditTempo, _T("999.999"), WS_CHILD | WS_BORDER | SS_LEFT | SS_CENTERIMAGE , this, IDC_EDIT_CURRENTTEMPO)) return FALSE;
	// Tempo Spin
	rect.SetRect(0, 0, SPINTEMPO_WIDTH, SPINTEMPO_HEIGHT);
	m_SpinTempo.Create(WS_CHILD | UDS_ALIGNRIGHT, rect, this, IDC_SPIN_CURRENTTEMPO);

	// Speed Text
	if(!CreateTextWnd(m_StaticSpeed, _T("Ticks/Row:"), WS_CHILD | SS_CENTER | SS_CENTERIMAGE, this, IDC_TEXT_CURRENTSPEED)) return FALSE;
	// Speed EditBox
	if(!CreateTextWnd(m_EditSpeed, _T("999"), WS_CHILD | WS_BORDER | SS_LEFT | SS_CENTERIMAGE , this, IDC_EDIT_CURRENTSPEED)) return FALSE;
	// Speed Spin
	rect.SetRect(0, 0, SPINSPEED_WIDTH, SPINSPEED_HEIGHT);
	m_SpinSpeed.Create(WS_CHILD | UDS_ALIGNRIGHT, rect, this, IDC_SPIN_CURRENTSPEED);

	// Rows per Beat Text
	if(!CreateTextWnd(m_StaticRowsPerBeat, _T("Rows/Beat:"), WS_CHILD | SS_CENTER | SS_CENTERIMAGE, this, IDC_TEXT_RPB)) return FALSE;
	// Rows per Beat EditBox
	if(!CreateTextWnd(m_EditRowsPerBeat, _T("9999"), WS_CHILD | WS_BORDER | SS_LEFT | SS_CENTERIMAGE , this, IDC_EDIT_RPB)) return FALSE;
	// Rows per Beat Spin
	rect.SetRect(0, 0, SPINRPB_WIDTH, SPINRPB_HEIGHT);
	m_SpinRowsPerBeat.Create(WS_CHILD | UDS_ALIGNRIGHT, rect, this, IDC_SPIN_RPB);

	// VU Meter
	rect.SetRect(0, 0, VUMETER_WIDTH, VUMETER_HEIGHT);
	//m_VuMeter.CreateEx(WS_EX_STATICEDGE, "STATIC", "", WS_CHILD | WS_BORDER | SS_NOTIFY, rect, this, IDC_VUMETER);
	m_VuMeter.Create(_T(""), WS_CHILD | WS_BORDER | SS_NOTIFY, rect, this, IDC_VUMETER);

	// Adjust control styles
	m_EditOctave.SendMessage(WM_SETFONT, hFont);
	m_EditOctave.ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_NOACTIVATE);
	m_StaticTempo.SendMessage(WM_SETFONT, hFont);
	m_EditTempo.SendMessage(WM_SETFONT, hFont);
	m_EditTempo.ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_NOACTIVATE);
	m_StaticSpeed.SendMessage(WM_SETFONT, hFont);
	m_EditSpeed.SendMessage(WM_SETFONT, hFont);
	m_EditSpeed.ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_NOACTIVATE);
	m_StaticRowsPerBeat.SendMessage(WM_SETFONT, hFont);
	m_EditRowsPerBeat.SendMessage(WM_SETFONT, hFont);
	m_EditRowsPerBeat.ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_NOACTIVATE);
	m_SpinOctave.SetRange(MIN_BASEOCTAVE, MAX_BASEOCTAVE);
	m_SpinOctave.SetPos(4);
	m_SpinTempo.SetRange(-1, 1);
	m_SpinTempo.SetPos(0);
	m_SpinSpeed.SetRange(-1, 1);
	m_SpinSpeed.SetPos(0);
	m_SpinRowsPerBeat.SetRange(-1, 1);
	m_SpinRowsPerBeat.SetPos(0);
	// Display everything
	SetWindowText(_T("Main"));
	SetBaseOctave(4);
	SetCurrentSong(nullptr);
	EnableDocking(CBRS_ALIGN_ANY);

	GetToolBarCtrl().SetState(ID_UPDATE_AVAILABLE, TBSTATE_HIDDEN);

	return TRUE;
}


void CMainToolBar::Init(CMainFrame *pMainFrm)
{
	EnableFlatButtons(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FLATBUTTONS);
	SetHorizontal();
	pMainFrm->DockControlBar(this);
}


static int GetWndWidth(const CWnd &wnd)
{
	CRect rect;
	wnd.GetClientRect(rect);
	return rect.right;
}


void CMainToolBar::SetHorizontal()
{
	CToolBarEx::SetHorizontal();
	m_VuMeter.SetOrientation(true);
	SetButtonInfo(EDITOCTAVE_INDEX, IDC_EDIT_BASEOCTAVE, TBBS_SEPARATOR, GetWndWidth(m_EditOctave));
	SetButtonInfo(SPINOCTAVE_INDEX, IDC_SPIN_BASEOCTAVE, TBBS_SEPARATOR, SPINOCTAVE_WIDTH);
	SetButtonInfo(TEMPOTEXT_INDEX, IDC_TEXT_CURRENTTEMPO, TBBS_SEPARATOR, GetWndWidth(m_StaticTempo));
	SetButtonInfo(EDITTEMPO_INDEX, IDC_EDIT_CURRENTTEMPO, TBBS_SEPARATOR, GetWndWidth(m_EditTempo));
	SetButtonInfo(SPINTEMPO_INDEX, IDC_SPIN_CURRENTTEMPO, TBBS_SEPARATOR, SPINTEMPO_WIDTH);
	SetButtonInfo(SPEEDTEXT_INDEX, IDC_TEXT_CURRENTSPEED, TBBS_SEPARATOR, GetWndWidth(m_StaticSpeed));
	SetButtonInfo(EDITSPEED_INDEX, IDC_EDIT_CURRENTSPEED, TBBS_SEPARATOR, GetWndWidth(m_EditSpeed));
	SetButtonInfo(SPINSPEED_INDEX, IDC_SPIN_CURRENTSPEED, TBBS_SEPARATOR, SPINSPEED_WIDTH);
	SetButtonInfo(RPBTEXT_INDEX, IDC_TEXT_RPB, TBBS_SEPARATOR, GetWndWidth(m_StaticRowsPerBeat));
	SetButtonInfo(EDITRPB_INDEX, IDC_EDIT_RPB, TBBS_SEPARATOR, GetWndWidth(m_EditRowsPerBeat));
	SetButtonInfo(SPINRPB_INDEX, IDC_SPIN_RPB, TBBS_SEPARATOR, SPINRPB_WIDTH);
	SetButtonInfo(VUMETER_INDEX, IDC_VUMETER, TBBS_SEPARATOR, VUMETER_WIDTH);

	//SetButtonInfo(SPINSPEED_INDEX+1, IDC_TEXT_BPM, TBBS_SEPARATOR, SPEEDTEXT_WIDTH);
	// Octave Box
	EnableControl(m_EditOctave, EDITOCTAVE_INDEX);
	EnableControl(m_SpinOctave, SPINOCTAVE_INDEX);
	// Tempo
	EnableControl(m_StaticTempo, TEMPOTEXT_INDEX, TEMPOTEXT_HEIGHT);
	EnableControl(m_EditTempo, EDITTEMPO_INDEX, EDITTEMPO_HEIGHT);
	EnableControl(m_SpinTempo, SPINTEMPO_INDEX, SPINTEMPO_HEIGHT);
	// Speed
	EnableControl(m_StaticSpeed, SPEEDTEXT_INDEX, SPEEDTEXT_HEIGHT);
	EnableControl(m_EditSpeed, EDITSPEED_INDEX, EDITSPEED_HEIGHT);
	EnableControl(m_SpinSpeed, SPINSPEED_INDEX, SPINSPEED_HEIGHT);
	// Rows per Beat
	EnableControl(m_StaticRowsPerBeat, RPBTEXT_INDEX, RPBTEXT_HEIGHT);
	EnableControl(m_EditRowsPerBeat, EDITRPB_INDEX, EDITRPB_HEIGHT);
	EnableControl(m_SpinRowsPerBeat, SPINRPB_INDEX, SPINRPB_HEIGHT);
	EnableControl(m_VuMeter, VUMETER_INDEX, VUMETER_HEIGHT);
}


void CMainToolBar::SetVertical()
{
	CToolBarEx::SetVertical();
	m_VuMeter.SetOrientation(false);
	// Change Buttons
	SetButtonInfo(EDITOCTAVE_INDEX, ID_SEPARATOR, TBBS_SEPARATOR, 1);
	SetButtonInfo(SPINOCTAVE_INDEX, ID_SEPARATOR, TBBS_SEPARATOR, 1);
	SetButtonInfo(TEMPOTEXT_INDEX, ID_SEPARATOR, TBBS_SEPARATOR, 1);
	SetButtonInfo(EDITTEMPO_INDEX, ID_SEPARATOR, TBBS_SEPARATOR, 1);
	SetButtonInfo(SPINTEMPO_INDEX, ID_SEPARATOR, TBBS_SEPARATOR, 1);
	SetButtonInfo(SPEEDTEXT_INDEX, ID_SEPARATOR, TBBS_SEPARATOR, 1);
	SetButtonInfo(EDITSPEED_INDEX, ID_SEPARATOR, TBBS_SEPARATOR, 1);
	SetButtonInfo(SPINSPEED_INDEX, ID_SEPARATOR, TBBS_SEPARATOR, 1);
	SetButtonInfo(RPBTEXT_INDEX, ID_SEPARATOR, TBBS_SEPARATOR, 1);
	SetButtonInfo(EDITRPB_INDEX, ID_SEPARATOR, TBBS_SEPARATOR, 1);
	SetButtonInfo(SPINRPB_INDEX, ID_SEPARATOR, TBBS_SEPARATOR, 1);
	SetButtonInfo(VUMETER_INDEX, IDC_VUMETER, TBBS_SEPARATOR, VUMETER_HEIGHT);

	// Hide Controls
	if(m_EditOctave.m_hWnd) m_EditOctave.ShowWindow(SW_HIDE);
	if(m_SpinOctave.m_hWnd) m_SpinOctave.ShowWindow(SW_HIDE);
	if(m_StaticTempo.m_hWnd) m_StaticTempo.ShowWindow(SW_HIDE);
	if(m_EditTempo.m_hWnd) m_EditTempo.ShowWindow(SW_HIDE);
	if(m_SpinTempo.m_hWnd) m_SpinTempo.ShowWindow(SW_HIDE);
	if(m_StaticSpeed.m_hWnd) m_StaticSpeed.ShowWindow(SW_HIDE);
	if(m_EditSpeed.m_hWnd) m_EditSpeed.ShowWindow(SW_HIDE);
	if(m_SpinSpeed.m_hWnd) m_SpinSpeed.ShowWindow(SW_HIDE);
	if(m_StaticRowsPerBeat.m_hWnd) m_StaticRowsPerBeat.ShowWindow(SW_HIDE);
	if(m_EditRowsPerBeat.m_hWnd) m_EditRowsPerBeat.ShowWindow(SW_HIDE);
	if(m_SpinRowsPerBeat.m_hWnd) m_SpinRowsPerBeat.ShowWindow(SW_HIDE);
	EnableControl(m_VuMeter, VUMETER_INDEX, VUMETER_HEIGHT);
	//if(m_StaticBPM.m_hWnd) m_StaticBPM.ShowWindow(SW_HIDE);
}


UINT CMainToolBar::GetBaseOctave() const
{
	if(nCurrentOctave >= MIN_BASEOCTAVE) return (UINT)nCurrentOctave;
	return 4;
}


BOOL CMainToolBar::SetBaseOctave(UINT nOctave)
{
	TCHAR s[64];

	if((nOctave < MIN_BASEOCTAVE) || (nOctave > MAX_BASEOCTAVE)) return FALSE;
	if(nOctave != (UINT)nCurrentOctave)
	{
		nCurrentOctave = nOctave;
		wsprintf(s, _T(" Octave %d"), nOctave);
		m_EditOctave.SetWindowText(s);
		m_SpinOctave.SetPos(nOctave);
	}
	return TRUE;
}


bool CMainToolBar::ShowUpdateInfo(const CString &newVersion, const CString &infoURL, bool showHighLight)
{
	GetToolBarCtrl().SetState(ID_UPDATE_AVAILABLE, TBSTATE_ENABLED);
	if(m_bVertical)
		SetVertical();
	else
		SetHorizontal();

	CRect rect;
	GetToolBarCtrl().GetRect(ID_UPDATE_AVAILABLE, &rect);
	CPoint pt = rect.CenterPoint();
	ClientToScreen(&pt);
	CMainFrame::GetMainFrame()->GetWindowRect(rect);
	LimitMax(pt.x, rect.right);

	if(showHighLight)
	{
		return m_tooltip.ShowUpdate(*this, newVersion, infoURL, rect, pt, ID_UPDATE_AVAILABLE);
	} else
	{
		return true;
	}
}


void CMainToolBar::RemoveUpdateInfo()
{
	if(m_tooltip)
		m_tooltip.Pop();
	GetToolBarCtrl().SetState(ID_UPDATE_AVAILABLE, TBSTATE_HIDDEN);
}


BOOL CMainToolBar::SetCurrentSong(CSoundFile *pSndFile)
{
	static CSoundFile *sndFile = nullptr;
	if(pSndFile != sndFile)
	{
		sndFile = pSndFile;
	}

	// Update Info
	if(pSndFile)
	{
		TCHAR s[32];
		// Update play/pause button
		if(nCurrentTempo == TEMPO(0, 0)) SetButtonInfo(PLAYCMD_INDEX, ID_PLAYER_PAUSE, TBBS_BUTTON, TOOLBAR_IMAGE_PAUSE);
		// Update Speed
		int nSpeed = pSndFile->m_PlayState.m_nMusicSpeed;
		if(nSpeed != nCurrentSpeed)
		{
			CModDoc *modDoc = pSndFile->GetpModDoc();
			if(modDoc != nullptr)
			{
				// Update envelope views if speed has changed
				modDoc->UpdateAllViews(InstrumentHint().Envelope());
			}

			if(nCurrentSpeed < 0) m_SpinSpeed.EnableWindow(TRUE);
			nCurrentSpeed = nSpeed;
			wsprintf(s, _T("%u"), static_cast<unsigned int>(nCurrentSpeed));
			m_EditSpeed.SetWindowText(s);
		}
		TEMPO nTempo = pSndFile->m_PlayState.m_nMusicTempo;
		if(nTempo != nCurrentTempo)
		{
			if(nCurrentTempo <= TEMPO(0, 0)) m_SpinTempo.EnableWindow(TRUE);
			nCurrentTempo = nTempo;
			if(nCurrentTempo.GetFract() == 0)
				_stprintf(s, _T("%u"), nCurrentTempo.GetInt());
			else
				_stprintf(s, _T("%.4f"), nCurrentTempo.ToDouble());
			m_EditTempo.SetWindowText(s);
		}
		int nRowsPerBeat = pSndFile->m_PlayState.m_nCurrentRowsPerBeat;
		if(nRowsPerBeat != nCurrentRowsPerBeat)
		{
			if(nCurrentRowsPerBeat < 0) m_SpinRowsPerBeat.EnableWindow(TRUE);
			nCurrentRowsPerBeat = nRowsPerBeat;
			wsprintf(s, _T("%u"), static_cast<unsigned int>(nCurrentRowsPerBeat));
			m_EditRowsPerBeat.SetWindowText(s);
		}
	} else
	{
		if(nCurrentTempo > TEMPO(0, 0))
		{
			nCurrentTempo.Set(0);
			m_EditTempo.SetWindowText(_T("---"));
			m_SpinTempo.EnableWindow(FALSE);
			SetButtonInfo(PLAYCMD_INDEX, ID_PLAYER_PLAY, TBBS_BUTTON, TOOLBAR_IMAGE_PLAY);
		}
		if(nCurrentSpeed != -1)
		{
			nCurrentSpeed = -1;
			m_EditSpeed.SetWindowText(_T("---"));
			m_SpinSpeed.EnableWindow(FALSE);
		}
		if(nCurrentRowsPerBeat != -1)
		{
			nCurrentRowsPerBeat = -1;
			m_EditRowsPerBeat.SetWindowText(_T("---"));
			m_SpinRowsPerBeat.EnableWindow(FALSE);
		}
	}
	return TRUE;
}


void CMainToolBar::OnVScroll(UINT nCode, UINT nPos, CScrollBar *pScrollBar)
{
	CMainFrame *pMainFrm;

	CToolBarEx::OnVScroll(nCode, nPos, pScrollBar);
	short int oct = (short int)m_SpinOctave.GetPos();
	if((oct >= MIN_BASEOCTAVE) && ((int)oct != nCurrentOctave))
	{
		SetBaseOctave(oct);
	}
	if((nCurrentSpeed < 0) || (nCurrentTempo <= TEMPO(0, 0))) return;
	if((pMainFrm = CMainFrame::GetMainFrame()) != nullptr)
	{
		CSoundFile *pSndFile = pMainFrm->GetSoundFilePlaying();
		if(pSndFile)
		{
			const auto &specs = pSndFile->GetModSpecifications();
			int n;
			if((n = mpt::signum(m_SpinTempo.GetPos32())) != 0)
			{
				TEMPO newTempo;
				if(specs.hasFractionalTempo)
				{
					n *= TEMPO::fractFact;
					if(CMainFrame::GetMainFrame()->GetInputHandler()->CtrlPressed())
						n /= 100;
					else
						n /= 10;
					newTempo.SetRaw(n);
				} else
				{
					newTempo = TEMPO(n, 0);
				}
				newTempo += nCurrentTempo;
				pSndFile->SetTempo(Clamp(newTempo, specs.GetTempoMin(), specs.GetTempoMax()), true);
				m_SpinTempo.SetPos(0);
			}
			if((n = mpt::signum(m_SpinSpeed.GetPos32())) != 0)
			{
				pSndFile->m_PlayState.m_nMusicSpeed = Clamp(uint32(nCurrentSpeed + n), specs.speedMin, specs.speedMax);
				m_SpinSpeed.SetPos(0);
			}
			if((n = m_SpinRowsPerBeat.GetPos32()) != 0)
			{
				if(n < 0)
				{
					if(nCurrentRowsPerBeat > 1)
						SetRowsPerBeat(nCurrentRowsPerBeat - 1);
				} else if(static_cast<ROWINDEX>(nCurrentRowsPerBeat) < pSndFile->m_PlayState.m_nCurrentRowsPerMeasure)
				{
						SetRowsPerBeat(nCurrentRowsPerBeat + 1);
				}
				m_SpinRowsPerBeat.SetPos(0);

				// Update pattern editor
				pMainFrm->PostMessage(WM_MOD_INVALIDATEPATTERNS, HINT_MPTOPTIONS);
			}

			SetCurrentSong(pSndFile);
		}
	}
}


void CMainToolBar::OnTbnDropDownToolBar(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMTOOLBAR *pToolBar = reinterpret_cast<NMTOOLBAR *>(pNMHDR);
	ClientToScreen(&(pToolBar->rcButton));

	switch(pToolBar->iItem)
	{
	case ID_FILE_NEW:
		CMainFrame::GetMainFrame()->GetFileMenu()->GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pToolBar->rcButton.left, pToolBar->rcButton.bottom, this);
		break;
	case ID_MIDI_RECORD:
		// Show a list of MIDI devices
		{
			HMENU hMenu = ::CreatePopupMenu();
			MIDIINCAPS mic;
			UINT numDevs = midiInGetNumDevs();
			if(numDevs > MAX_MIDI_DEVICES) numDevs = MAX_MIDI_DEVICES;
			UINT current = TrackerSettings::Instance().GetCurrentMIDIDevice();
			for(UINT i = 0; i < numDevs; i++)
			{
				mic.szPname[0] = 0;
				if(midiInGetDevCaps(i, &mic, sizeof(mic)) == MMSYSERR_NOERROR)
				{
					::AppendMenu(hMenu, MF_STRING | (i == current ? MF_CHECKED : 0), ID_SELECT_MIDI_DEVICE + i, theApp.GetFriendlyMIDIPortName(mpt::String::ReadCStringBuf(mic.szPname), true));
				}
			}
			if(!numDevs)
			{
				::AppendMenu(hMenu, MF_STRING | MF_GRAYED, 0, _T("No MIDI input devices found"));
			}
			::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pToolBar->rcButton.left, pToolBar->rcButton.bottom, 0, m_hWnd, NULL);
			::DestroyMenu(hMenu);
		}
		break;
	}

	*pResult = 0;
}


void CMainToolBar::OnSelectMIDIDevice(UINT id)
{
	CMainFrame::GetMainFrame()->midiCloseDevice();
	TrackerSettings::Instance().SetMIDIDevice(id - ID_SELECT_MIDI_DEVICE);
	CMainFrame::GetMainFrame()->midiOpenDevice();
}


void CMainToolBar::SetRowsPerBeat(ROWINDEX nNewRPB)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm == nullptr)
		return;
	CModDoc *pModDoc = pMainFrm->GetModPlaying();
	CSoundFile *pSndFile = pMainFrm->GetSoundFilePlaying();
	if(pModDoc == nullptr || pSndFile == nullptr)
		return;

	pSndFile->m_PlayState.m_nCurrentRowsPerBeat = nNewRPB;
	PATTERNINDEX nPat = pSndFile->GetCurrentPattern();
	if(pSndFile->Patterns[nPat].GetOverrideSignature())
	{
		if(nNewRPB <= pSndFile->Patterns[nPat].GetRowsPerMeasure())
		{
			pSndFile->Patterns[nPat].SetSignature(nNewRPB, pSndFile->Patterns[nPat].GetRowsPerMeasure());
			TempoSwing swing = pSndFile->Patterns[nPat].GetTempoSwing();
			if(!swing.empty())
			{
				swing.resize(nNewRPB);
				pSndFile->Patterns[nPat].SetTempoSwing(swing);
			}
			pModDoc->SetModified();
		}
	} else
	{
		if(nNewRPB <= pSndFile->m_nDefaultRowsPerMeasure)
		{
			pSndFile->m_nDefaultRowsPerBeat = nNewRPB;
			if(!pSndFile->m_tempoSwing.empty()) pSndFile->m_tempoSwing.resize(nNewRPB);
			pModDoc->SetModified();
		}
	}
}


BOOL CMainToolBar::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
	auto pTTT = reinterpret_cast<TOOLTIPTEXT *>(pNMHDR);
	UINT_PTR id = pNMHDR->idFrom;
	if(pTTT->uFlags & TTF_IDISHWND)
	{
		// idFrom is actually the HWND of the tool
		id = (UINT_PTR)::GetDlgCtrlID((HWND)id);
	}

	const TCHAR *s = nullptr;
	CommandID cmd = kcNull;
	switch(id)
	{
	case ID_FILE_NEW: s = _T("New"); cmd = kcFileNew; break;
	case ID_FILE_OPEN: s = _T("Open"); cmd = kcFileOpen; break;
	case ID_FILE_SAVE: s = _T("Save"); cmd = kcFileSave; break;
	case ID_EDIT_CUT: s = _T("Cut"); cmd = kcEditCut; break;
	case ID_EDIT_COPY: s = _T("Copy"); cmd = kcEditCopy; break;
	case ID_EDIT_PASTE: s = _T("Paste"); cmd = kcEditPaste; break;
	case ID_MIDI_RECORD: s = _T("MIDI Record"); cmd = kcMidiRecord; break;
	case ID_PLAYER_STOP: s = _T("Stop"); cmd = kcStopSong; break;
	case ID_PLAYER_PLAY: s = _T("Play"); cmd = kcPlayPauseSong; break;
	case ID_PLAYER_PAUSE: s = _T("Pause"); cmd = kcPlayPauseSong; break;
	case ID_PLAYER_PLAYFROMSTART: s = _T("Play From Start"); cmd = kcPlaySongFromStart; break;
	case ID_VIEW_OPTIONS: s = _T("Setup"); cmd = kcViewOptions; break;
	case ID_PANIC: s = _T("Stop all hanging plugin and sample voices"); cmd = kcPanic; break;
	case ID_UPDATE_AVAILABLE: s = _T("A new update is available."); break;
	}

	if(s == nullptr)
		return FALSE;
	
	mpt::tstring fmt = s;
	if(cmd != kcNull)
	{
		auto keyText = CMainFrame::GetInputHandler()->m_activeCommandSet->GetKeyTextFromCommand(cmd, 0);
		if(!keyText.IsEmpty())
			fmt += MPT_TFORMAT(" ({})")(keyText);
	}
	mpt::String::WriteWinBuf(pTTT->szText) = fmt;
	*pResult = 0;

	// bring the tooltip window above other popup windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
		SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER);

	return TRUE;    // message was handled
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// CModTreeBar

BEGIN_MESSAGE_MAP(CModTreeBar, CDialogBar)
	//{{AFX_MSG_MAP(CModTreeBar)
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_NCHITTEST()
	ON_WM_SIZE()
	ON_WM_NCMOUSEMOVE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_NCLBUTTONUP()
	ON_WM_TIMER()
	ON_MESSAGE(WM_INITDIALOG, &CModTreeBar::OnInitDialog)

	ON_EN_CHANGE(IDC_EDIT1,             &CModTreeBar::OnFilterChanged)
	ON_EN_KILLFOCUS(IDC_EDIT1,          &CModTreeBar::OnFilterLostFocus)
	ON_COMMAND(ID_CLOSE_LIBRARY_FILTER, &CModTreeBar::CloseTreeFilter)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CModTreeBar::CModTreeBar()
{
	m_nTreeSplitRatio = TrackerSettings::Instance().glTreeSplitRatio;
}


LRESULT CModTreeBar::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	LRESULT l = CDialogBar::HandleInitDialog(wParam, lParam);
	m_pModTreeData = new CModTree(nullptr);
	if(m_pModTreeData)	m_pModTreeData->SubclassDlgItem(IDC_TREEDATA, this);
	m_pModTree = new CModTree(m_pModTreeData);
	if(m_pModTree)	m_pModTree->SubclassDlgItem(IDC_TREEVIEW, this);
	m_dwStatus = 0;
	m_sizeDefault.cx = Util::ScalePixels(TrackerSettings::Instance().glTreeWindowWidth, m_hWnd) + 3;
	m_sizeDefault.cy = 32767;
	return l;
}


CModTreeBar::~CModTreeBar()
{
	if(m_pModTree)
	{
		delete m_pModTree;
		m_pModTree = nullptr;
	}
	if(m_pModTreeData)
	{
		delete m_pModTreeData;
		m_pModTreeData = nullptr;
	}
}


void CModTreeBar::Init()
{
	m_nTreeSplitRatio = TrackerSettings::Instance().glTreeSplitRatio;
	if(m_pModTree)
	{
		m_pModTreeData->Init();
		m_pModTree->Init();
	}
}


BOOL CModTreeBar::PreTranslateMessage(MSG *pMsg)
{
	if(m_filterEdit && pMsg->hwnd == m_filterEdit.m_hWnd && pMsg->message == WM_KEYDOWN && m_filterSource != nullptr)
	{
		switch(pMsg->wParam)
		{
		case VK_RETURN:
			if(const auto modItem = m_filterSource->GetModItem(m_filterSource->GetSelectedItem()); (modItem.type == CModTree::MODITEM_INSLIB_FOLDER || modItem.type == CModTree::MODITEM_INSLIB_SONG))
			{
				m_filterSource->PostMessage(WM_COMMAND, ID_MODTREE_EXECUTE);
			}
			[[fallthrough]];
		case VK_ESCAPE:
			CloseTreeFilter();
			return TRUE;

		case VK_TAB:
			if(m_filterSource)
			{
				m_filterSource->SetFocus();
				return TRUE;
			}
			break;

		case VK_UP:
		case VK_DOWN:
			if(const auto selectedItem = m_filterSource->GetSelectedItem(); selectedItem != nullptr)
			{
				const auto item = m_filterSource->GetNextItem(selectedItem, (pMsg->wParam == VK_UP) ? TVGN_PREVIOUS : TVGN_NEXT);
				if(item)
					m_filterSource->SelectItem(item);
				return TRUE;
			}
			break;
		}
	}
	return CDialogBar::PreTranslateMessage(pMsg);
}


void CModTreeBar::RefreshDlsBanks()
{
	if(m_pModTree) m_pModTree->RefreshDlsBanks();
}


void CModTreeBar::RefreshMidiLibrary()
{
	if(m_pModTree) m_pModTree->RefreshMidiLibrary();
}


void CModTreeBar::OnOptionsChanged()
{
	if(m_pModTree) m_pModTree->OnOptionsChanged();
}


void CModTreeBar::RecalcLayout()
{
	CRect rect;

	if((m_pModTree) && (m_pModTreeData))
	{
		int cytree, cydata, cyavail;

		GetClientRect(&rect);
		cyavail = rect.Height() - 3;
		if(cyavail < 0) cyavail = 0;
		cytree = (cyavail * m_nTreeSplitRatio) >> 8;
		cydata = cyavail - cytree;
		if(m_filterSource == m_pModTree)
		{
			int editHeight = Util::ScalePixels(20, m_hWnd);
			m_pModTree->SetWindowPos(nullptr, 0, 0, rect.Width(), cytree - editHeight, SWP_NOZORDER | SWP_NOACTIVATE);
			m_pModTreeData->SetWindowPos(nullptr, 0, cytree + 3, rect.Width(), cydata, SWP_NOZORDER | SWP_NOACTIVATE);
			m_filterEdit.SetWindowPos(m_pModTree, 0, cytree - editHeight, rect.Width(), editHeight, SWP_NOACTIVATE);
		} else if(m_filterSource == m_pModTreeData)
		{
			int editHeight = Util::ScalePixels(20, m_hWnd);
			m_pModTree->SetWindowPos(nullptr, 0, 0, rect.Width(), cytree, SWP_NOZORDER | SWP_NOACTIVATE);
			m_pModTreeData->SetWindowPos(nullptr, 0, cytree + 3, rect.Width(), cydata - editHeight, SWP_NOZORDER | SWP_NOACTIVATE);
			m_filterEdit.SetWindowPos(m_pModTreeData, 0, cytree + 3 + cydata - editHeight, rect.Width(), editHeight, SWP_NOACTIVATE);
		} else
		{
			m_pModTree->SetWindowPos(nullptr, 0, 0, rect.Width(), cytree, SWP_NOZORDER | SWP_NOACTIVATE);
			m_pModTreeData->SetWindowPos(nullptr, 0, cytree + 3, rect.Width(), cydata, SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}
}


CSize CModTreeBar::CalcFixedLayout(BOOL, BOOL)
{
	int width = Util::ScalePixels(TrackerSettings::Instance().glTreeWindowWidth, m_hWnd);
	CSize sz;
	m_sizeDefault.cx = width;
	m_sizeDefault.cy = 32767;
	sz.cx = width + 3;
	if(sz.cx < 4) sz.cx = 4;
	sz.cy = 32767;
	return sz;
}


void CModTreeBar::DoMouseMove(CPoint pt)
{
	CRect rect;

	if((m_dwStatus & (MTB_CAPTURE|MTB_DRAGGING)) && (::GetCapture() != m_hWnd))
	{
		CancelTracking();
	}
	if(m_dwStatus & MTB_DRAGGING)
	{
		if(m_dwStatus & MTB_VERTICAL)
		{
			if(m_pModTree)
			{
				m_pModTree->GetWindowRect(&rect);
				pt.y += rect.Height();
			}
			GetClientRect(&rect);
			pt.y -= ptDragging.y;
			if(pt.y < 0) pt.y = 0;
			if(pt.y > rect.Height()) pt.y = rect.Height();
			if((!(m_dwStatus & MTB_TRACKER)) || (pt.y != (int)m_nTrackPos))
			{
				if(m_dwStatus & MTB_TRACKER) OnInvertTracker(m_nTrackPos);
				m_nTrackPos = pt.y;
				OnInvertTracker(m_nTrackPos);
				m_dwStatus |= MTB_TRACKER;
			}
		} else
		{
			pt.x -= ptDragging.x - m_cxOriginal + 3;
			if(pt.x < 0) pt.x = 0;
			if((!(m_dwStatus & MTB_TRACKER)) || (pt.x != (int)m_nTrackPos))
			{
				if(m_dwStatus & MTB_TRACKER) OnInvertTracker(m_nTrackPos);
				m_nTrackPos = pt.x;
				OnInvertTracker(m_nTrackPos);
				m_dwStatus |= MTB_TRACKER;
			}
		}
	} else
	{
		UINT nCursor = 0;

		GetClientRect(&rect);
		rect.left = rect.right - 2;
		rect.right = rect.left + 5;
		if(rect.PtInRect(pt))
		{
			nCursor = AFX_IDC_HSPLITBAR;
		} else
		if(m_pModTree)
		{
			m_pModTree->GetWindowRect(&rect);
			rect.right = rect.Width();
			rect.left = 0;
			rect.top = rect.Height()-1;
			rect.bottom = rect.top + 5;
			if(rect.PtInRect(pt))
			{
				nCursor = AFX_IDC_VSPLITBAR;
			}
		}
		if(nCursor)
		{
			UINT nDir = (nCursor == AFX_IDC_VSPLITBAR) ? MTB_VERTICAL : 0;
			BOOL bLoad = FALSE;
			if(!(m_dwStatus & MTB_CAPTURE))
			{
				m_dwStatus |= MTB_CAPTURE;
				SetCapture();
				bLoad = TRUE;
			} else
			{
				if(nDir != (m_dwStatus & MTB_VERTICAL)) bLoad = TRUE;
			}
			m_dwStatus &= ~MTB_VERTICAL;
			m_dwStatus |= nDir;
			if(bLoad) SetCursor(theApp.LoadCursor(nCursor));
		} else
		{
			if(m_dwStatus & MTB_CAPTURE)
			{
				m_dwStatus &= ~MTB_CAPTURE;
				ReleaseCapture();
				SetCursor(LoadCursor(NULL, IDC_ARROW));
			}
		}
	}
}


void CModTreeBar::DoLButtonDown(CPoint pt)
{
	if((m_dwStatus & MTB_CAPTURE) && (!(m_dwStatus & MTB_DRAGGING)))
	{
		CRect rect;
		GetWindowRect(&rect);
		m_cxOriginal = rect.Width();
		m_cyOriginal = rect.Height();
		ptDragging = pt;
		m_dwStatus |= MTB_DRAGGING;
		DoMouseMove(pt);
	}
}


void CModTreeBar::DoLButtonUp()
{
	if(m_dwStatus & MTB_DRAGGING)
	{
		CRect rect;

		m_dwStatus &= ~MTB_DRAGGING;
		if(m_dwStatus & MTB_TRACKER)
		{
			OnInvertTracker(m_nTrackPos);
			m_dwStatus &= ~MTB_TRACKER;
		}
		if(m_dwStatus & MTB_VERTICAL)
		{
			GetClientRect(&rect);
			int cyavail = rect.Height() - 3;
			if(cyavail < 4) cyavail = 4;
			int ratio = (m_nTrackPos << 8) / cyavail;
			if(ratio < 0) ratio = 0;
			if(ratio > 256) ratio = 256;
			m_nTreeSplitRatio = ratio;
			TrackerSettings::Instance().glTreeSplitRatio = ratio;
			RecalcLayout();
		} else
		{
			GetWindowRect(&rect);
			m_nTrackPos += 3;
			if(m_nTrackPos < 4) m_nTrackPos = 4;
			CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
			if((m_nTrackPos != (UINT)rect.Width()) && (pMainFrm))
			{
				TrackerSettings::Instance().glTreeWindowWidth = Util::ScalePixelsInv(m_nTrackPos - 3, m_hWnd);
				m_sizeDefault.cx = m_nTrackPos;
				m_sizeDefault.cy = 32767;
				pMainFrm->RecalcLayout();
			}
		}
	}
}


void CModTreeBar::CancelTracking()
{
	if(m_dwStatus & MTB_TRACKER)
	{
		OnInvertTracker(m_nTrackPos);
		m_dwStatus &= ~MTB_TRACKER;
	}
	m_dwStatus &= ~MTB_DRAGGING;
	if(m_dwStatus & MTB_CAPTURE)
	{
		m_dwStatus &= ~MTB_CAPTURE;
		ReleaseCapture();
	}
}


void CModTreeBar::OnInvertTracker(UINT x)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	if(pMainFrm)
	{
		CRect rect;

		GetClientRect(&rect);
		if(m_dwStatus & MTB_VERTICAL)
		{
			rect.top = x;
			rect.bottom = rect.top + 4;
		} else
		{
			rect.left = x;
			rect.right = rect.left + 4;
		}
		ClientToScreen(&rect);
		pMainFrm->ScreenToClient(&rect);

		// pat-blt without clip children on
		CDC* pDC = pMainFrm->GetDC();
		// invert the brush pattern (looks just like frame window sizing)
		CBrush* pBrush = CDC::GetHalftoneBrush();
		HBRUSH hOldBrush = NULL;
		if(pBrush != NULL)
			hOldBrush = (HBRUSH)SelectObject(pDC->m_hDC, pBrush->m_hObject);
		pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATINVERT);
		if(hOldBrush != NULL)
			SelectObject(pDC->m_hDC, hOldBrush);
		ReleaseDC(pDC);
	}
}


void CModTreeBar::OnDocumentCreated(CModDoc *pModDoc)
{
	if(m_pModTree && pModDoc) m_pModTree->AddDocument(*pModDoc);
}


void CModTreeBar::OnDocumentClosed(CModDoc *pModDoc)
{
	if(m_pModTree && pModDoc) m_pModTree->RemoveDocument(*pModDoc);
}


void CModTreeBar::OnUpdate(CModDoc *pModDoc, UpdateHint hint, CObject *pHint)
{
	if(m_pModTree) m_pModTree->OnUpdate(pModDoc, hint, pHint);
}


void CModTreeBar::UpdatePlayPos(CModDoc *pModDoc, Notification *pNotify)
{
	if(m_pModTree && pModDoc) m_pModTree->UpdatePlayPos(*pModDoc, pNotify);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// CModTreeBar message handlers

void CModTreeBar::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	CDialogBar::OnNcCalcSize(bCalcValidRects, lpncsp);
	if(lpncsp)
	{
		lpncsp->rgrc[0].right -= 3;
		if(lpncsp->rgrc[0].right < lpncsp->rgrc[0].left) lpncsp->rgrc[0].right = lpncsp->rgrc[0].left;
	}
}


LRESULT CModTreeBar::OnNcHitTest(CPoint point)
{
	CRect rect;

	GetWindowRect(&rect);
	rect.DeflateRect(1,1);
	rect.right -= 3;
	if(!rect.PtInRect(point)) return HTBORDER;
	return CDialogBar::OnNcHitTest(point);
}


void CModTreeBar::OnNcPaint()
{
	RECT rect;
	CDialogBar::OnNcPaint();

	GetWindowRect(&rect);
	// Assumes there is no other non-client items
	rect.right -= rect.left;
	rect.bottom -= rect.top;
	rect.top = 0;
	rect.left = rect.right - 3;
	if((rect.left < rect.right) && (rect.top < rect.bottom))
	{
		CDC *pDC = GetWindowDC();
		HDC hdc = pDC->m_hDC;
		FillRect(hdc, &rect, GetSysColorBrush(COLOR_BTNFACE));
		ReleaseDC(pDC);
	}
}


void CModTreeBar::OnSize(UINT nType, int cx, int cy)
{
	CDialogBar::OnSize(nType, cx, cy);
	RecalcLayout();
}


void CModTreeBar::OnNcMouseMove(UINT, CPoint point)
{
	CRect rect;
	CPoint pt = point;

	GetWindowRect(&rect);
	pt.x -= rect.left;
	pt.y -= rect.top;
	DoMouseMove(pt);
}


void CModTreeBar::OnMouseMove(UINT, CPoint point)
{
	DoMouseMove(point);
}


void CModTreeBar::OnNcLButtonDown(UINT, CPoint point)
{
	CRect rect;
	CPoint pt = point;

	GetWindowRect(&rect);
	pt.x -= rect.left;
	pt.y -= rect.top;
	DoLButtonDown(pt);
}


void CModTreeBar::OnLButtonDown(UINT, CPoint point)
{
	DoLButtonDown(point);
}


void CModTreeBar::OnNcLButtonUp(UINT, CPoint)
{
	DoLButtonUp();
}


void CModTreeBar::OnLButtonUp(UINT, CPoint)
{
	DoLButtonUp();
}


HWND CModTreeBar::GetModTreeHWND()
{
	return m_pModTree->m_hWnd;
}


LRESULT CModTreeBar::SendMessageToModTree(UINT cmdID, WPARAM wParam, LPARAM lParam)
{
	if(::GetFocus() == m_pModTree->m_hWnd)
		return m_pModTree->SendMessage(cmdID, wParam, lParam);
	if(::GetFocus() == m_pModTreeData->m_hWnd)
		return m_pModTreeData->SendMessage(cmdID, wParam, lParam);
	return 0;
}


bool CModTreeBar::SetTreeSoundfile(FileReader &file)
{
	return m_pModTree->SetSoundFile(file);
}


void CModTreeBar::StartTreeFilter(CModTree &source)
{
	if(!m_filterEdit)
	{
		CRect rect;
		GetClientRect(rect);
		rect.bottom = Util::ScalePixels(20, m_hWnd);

		m_pModTree->GetItemRect(m_pModTree->GetFirstVisibleItem(), rect, FALSE);
		m_filterEdit.Create(WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, rect, this, IDC_EDIT1);
		m_filterEdit.SetFont(GetFont());
	} else if(m_filterSource != &source)
	{
		m_filterEdit.SetWindowText(_T(""));
	}
	m_filterEdit.SetFocus();
	m_filterSource = &source;
	RecalcLayout();
}


void CModTreeBar::OnFilterChanged()
{
	if(!m_filterSource)
		return;

	CString filter;
	m_filterEdit.GetWindowText(filter);
	if (filter.GetLength() < 1 || filter.GetLength() > 2)
	{
		CancelTimer();
		m_filterSource->SetInstrumentLibraryFilter(mpt::ToWin(filter));
	} else
	{
		if(!m_filterTimer)
			m_filterTimer = SetTimer(1, 360 - filter.GetLength() * 120, nullptr);
	}
}


void CModTreeBar::OnTimer(UINT_PTR id)
{
	if(id != m_filterTimer)
		return;

	if(m_filterSource)
	{
		CString filter;
		m_filterEdit.GetWindowText(filter);
		m_filterSource->SetInstrumentLibraryFilter(mpt::ToWin(filter));
	}
	CancelTimer();
}


void CModTreeBar::OnFilterLostFocus()
{
	if(m_filterEdit && !m_filterEdit.GetWindowTextLength())
		CloseTreeFilter();
}


void CModTreeBar::CloseTreeFilter()
{
	CancelTimer();
	if(m_filterSource)
	{
		m_filterSource->SetInstrumentLibraryFilter({});
		if(GetFocus() == &m_filterEdit)
			m_filterSource->SetFocus();
		m_filterSource = nullptr;
	}
	if(m_filterEdit)
	{
		m_filterEdit.DestroyWindow();
		RecalcLayout();
	}
}


void CModTreeBar::CancelTimer()
{
	if(m_filterTimer)
	{
		KillTimer(m_filterTimer);
		m_filterTimer = 0;
	}
}


////////////////////////////////////////////////////////////////////////////////
//
// Stereo VU Meter for toolbar
//

BEGIN_MESSAGE_MAP(CStereoVU, CStatic)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


void CStereoVU::OnPaint()
{
	CRect rect;
	CPaintDC dc(this);
	DrawVuMeters(dc, true);
}


void CStereoVU::SetVuMeter(uint8 validChannels, const uint32 channels[4], bool force)
{
	bool changed = false;
	if(validChannels == 0)
	{
		// reset
		validChannels = numChannels;
	} else if(validChannels != numChannels)
	{
		changed = true;
		force = true;
		numChannels = validChannels;
		allowRightToLeft = (numChannels > 2);
	}
	for(uint8 c = 0; c < validChannels; ++c)
	{
		if(vuMeter[c] != channels[c])
		{
			changed = true;
		}
	}
	if(changed)
	{
		DWORD curTime = timeGetTime();
		if(curTime - lastVuUpdateTime >= TrackerSettings::Instance().VuMeterUpdateInterval || force)
		{
			for(uint8 c = 0; c < validChannels; ++c)
			{
				vuMeter[c] = channels[c];
			}
			CClientDC dc(this);
			DrawVuMeters(dc, force);
			lastVuUpdateTime = curTime;
		}
	}
}


// Draw stereo VU
void CStereoVU::DrawVuMeters(CDC &dc, bool redraw)
{
	CRect rect;
	GetClientRect(&rect);

	if(redraw)
	{
		dc.FillSolidRect(rect.left, rect.top, rect.Width(), rect.Height(), RGB(0,0,0));
	}

	for(uint8 channel = 0; channel < numChannels; ++channel)
	{
		CRect chanrect = rect;
		if(horizontal)
		{
			if(allowRightToLeft)
			{
				const int col = channel % 2;
				const int row = channel / 2;

				float width = (rect.Width() - 2.0f) / 2.0f;
				float height = rect.Height() / float(numChannels/2);

				chanrect.top = mpt::saturate_round<int32>(rect.top + height * row);
				chanrect.bottom = mpt::saturate_round<int32>(chanrect.top + height) - 1;
				
				chanrect.left = mpt::saturate_round<int32>(rect.left + width * col) + ((col == 1) ? 2 : 0);
				chanrect.right = mpt::saturate_round<int32>(chanrect.left + width) - 1;

			} else
			{
				float height = rect.Height() / float(numChannels);
				chanrect.top = mpt::saturate_round<int32>(rect.top + height * channel);
				chanrect.bottom = mpt::saturate_round<int32>(chanrect.top + height) - 1;
			}
		} else
		{
			float width = rect.Width() / float(numChannels);
			chanrect.left = mpt::saturate_round<int32>(rect.left + width * channel);
			chanrect.right = mpt::saturate_round<int32>(chanrect.left + width) - 1;
		}
		DrawVuMeter(dc, chanrect, channel, redraw);
	}

}


// Draw a single VU Meter
void CStereoVU::DrawVuMeter(CDC &dc, const CRect &rect, int index, bool redraw)
{
	uint32 vu = vuMeter[index];

	if(CMainFrame::GetMainFrame()->GetSoundFilePlaying() == nullptr)
	{
		vu = 0;
	}

	const bool clip = (vu & Notification::ClipVU) != 0;
	vu = (vu & (~Notification::ClipVU)) >> 8;

	if(horizontal)
	{
		const bool rtl = allowRightToLeft && ((index % 2) == 0);

		const int cx = std::max(1, rect.Width());
		int v = (vu * cx) >> 8;

		for(int x = 0; x <= cx; x += 2)
		{
			int pen = Clamp((x * NUM_VUMETER_PENS) / cx, 0, NUM_VUMETER_PENS - 1);
			const bool last = (x == (cx & ~0x1));

			// Darken everything above volume, unless it's the clip indicator
			if(v <= x && (!last || !clip))
				pen += NUM_VUMETER_PENS;

			bool draw = redraw || (v < lastV[index] && v<=x && x<=lastV[index]) || (lastV[index] < v && lastV[index]<=x && x<=v);
			draw = draw || (last && clip != lastClip[index]);
			if(draw) dc.FillSolidRect(
				((!rtl) ? (rect.left + x) : (rect.right - x)),
				rect.top, 1, rect.Height(), CMainFrame::gcolrefVuMeter[pen]);
			if(last) lastClip[index] = clip;
		}
		lastV[index] = v;
	} else
	{
		const int cy = std::max(1, rect.Height());
		int v = (vu * cy) >> 8;

		for(int ry = rect.bottom - 1; ry > rect.top; ry -= 2)
		{
			const int y0 = rect.bottom - ry;
			int pen = Clamp((y0 * NUM_VUMETER_PENS) / cy, 0, NUM_VUMETER_PENS - 1);
			const bool last = (ry == rect.top + 1);

			// Darken everything above volume, unless it's the clip indicator
			if(v <= y0 && (!last || !clip))
				pen += NUM_VUMETER_PENS;

			bool draw = redraw || (v < lastV[index] && v<=ry && ry<=lastV[index]) || (lastV[index] < v && lastV[index]<=ry && ry<=v);
			draw = draw || (last && clip != lastClip[index]);
			if(draw) dc.FillSolidRect(rect.left, ry, rect.Width(), 1, CMainFrame::gcolrefVuMeter[pen]);
			if(last) lastClip[index] = clip;
		}
		lastV[index] = v;
	}
}


void CStereoVU::OnLButtonDown(UINT, CPoint)
{
	// Reset clip indicator.
	CMainFrame::GetMainFrame()->m_VUMeterInput.ResetClipped();
	CMainFrame::GetMainFrame()->m_VUMeterOutput.ResetClipped();
}


OPENMPT_NAMESPACE_END
