/*
 * mainbar.cpp
 * -----------
 * Purpose: Implementation of OpenMPT's window toolbar.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "view_tre.h"
#include "moddoc.h"

/////////////////////////////////////////////////////////////////////
// CToolBarEx: custom toolbar base class

BOOL CToolBarEx::SetHorizontal()
//------------------------------
{
	m_bVertical = FALSE;
	SetBarStyle(GetBarStyle() | CBRS_ALIGN_TOP);
	return TRUE;
}


BOOL CToolBarEx::SetVertical()
//----------------------------
{
	m_bVertical = TRUE;
	return TRUE;
}


CSize CToolBarEx::CalcDynamicLayout(int nLength, DWORD dwMode)
//------------------------------------------------------------
{
	CSize sizeResult;
	// if we're committing set the buttons appropriately
	if (dwMode & LM_COMMIT)
	{
		if (dwMode & LM_VERTDOCK)
		{
			if (!m_bVertical)
				SetVertical();
		} else
		{
			if (m_bVertical)
				SetHorizontal();
		}
		sizeResult = CToolBar::CalcDynamicLayout(nLength, dwMode);
	} else
	{
		BOOL bOld = m_bVertical;
		BOOL bSwitch = (dwMode & LM_HORZ) ? bOld : !bOld;

		if (bSwitch)
		{
			if (bOld)
				SetHorizontal();
			else
				SetVertical();
		}

		sizeResult = CToolBar::CalcDynamicLayout(nLength, dwMode);
		
		if (bSwitch)
		{
			if (bOld)
				SetHorizontal();
			else
				SetVertical();
		}
	}

	return sizeResult;
}


BOOL CToolBarEx::EnableControl(CWnd &wnd, UINT nIndex, UINT nHeight)
//------------------------------------------------------------------
{
	if (wnd.m_hWnd != NULL)
	{
		CRect rect;
		GetItemRect(nIndex, rect);
		if (nHeight)
		{
			int n = (rect.bottom + rect.top - nHeight) / 2;
			if (n > rect.top) rect.top = n;
		}
		wnd.SetWindowPos(NULL, rect.left, rect.top, 0, 0, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOCOPYBITS);
		wnd.ShowWindow(SW_SHOW);
	}
	return TRUE;
}


void CToolBarEx::ChangeCtrlStyle(long lStyle, BOOL bSetStyle)
//-----------------------------------------------------------
{
	if (m_hWnd)
	{
		LONG lStyleOld = GetWindowLong(m_hWnd, GWL_STYLE);
		if (bSetStyle)
			lStyleOld |= lStyle;
		else
			lStyleOld &= ~lStyle;
		SetWindowLong(m_hWnd, GWL_STYLE, lStyleOld);
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE);
		Invalidate();
	}
}


void CToolBarEx::EnableFlatButtons(BOOL bFlat)
//--------------------------------------------
{
	m_bFlatButtons = bFlat;
	ChangeCtrlStyle(TBSTYLE_FLAT, bFlat);
}


/////////////////////////////////////////////////////////////////////
// CMainToolBar

// Play Command
#define PLAYCMD_INDEX		10
#define TOOLBAR_IMAGE_PAUSE	8
#define TOOLBAR_IMAGE_PLAY	12
// Base octave
#define EDITOCTAVE_INDEX	13
#define EDITOCTAVE_WIDTH	55
#define EDITOCTAVE_HEIGHT	20
#define SPINOCTAVE_INDEX	(EDITOCTAVE_INDEX+1)
#define SPINOCTAVE_WIDTH	16
#define SPINOCTAVE_HEIGHT	(EDITOCTAVE_HEIGHT)
// Static "Tempo:"
#define TEMPOTEXT_INDEX		16
#define TEMPOTEXT_WIDTH		45
#define TEMPOTEXT_HEIGHT	20
// Edit Tempo
#define EDITTEMPO_INDEX		(TEMPOTEXT_INDEX+1)
#define EDITTEMPO_WIDTH		32
#define EDITTEMPO_HEIGHT	20
// Spin Tempo
#define SPINTEMPO_INDEX		(EDITTEMPO_INDEX+1)
#define SPINTEMPO_WIDTH		16
#define SPINTEMPO_HEIGHT	(EDITTEMPO_HEIGHT)
// Static "Speed:"
#define SPEEDTEXT_INDEX		20
#define SPEEDTEXT_WIDTH		57
#define SPEEDTEXT_HEIGHT	(TEMPOTEXT_HEIGHT)
// Edit Speed
#define EDITSPEED_INDEX		(SPEEDTEXT_INDEX+1)
#define EDITSPEED_WIDTH		28
#define EDITSPEED_HEIGHT	(EDITTEMPO_HEIGHT)
// Spin Speed
#define SPINSPEED_INDEX		(EDITSPEED_INDEX+1)
#define SPINSPEED_WIDTH		16
#define SPINSPEED_HEIGHT	(EDITSPEED_HEIGHT)
// Static "Rows/Beat:"
#define RPBTEXT_INDEX		24
#define RPBTEXT_WIDTH		63
#define RPBTEXT_HEIGHT		(TEMPOTEXT_HEIGHT)
// Edit Speed
#define EDITRPB_INDEX		(RPBTEXT_INDEX+1)
#define EDITRPB_WIDTH		28
#define EDITRPB_HEIGHT		(EDITTEMPO_HEIGHT)
// Spin Speed
#define SPINRPB_INDEX		(EDITRPB_INDEX+1)
#define SPINRPB_WIDTH		16
#define SPINRPB_HEIGHT		(EDITRPB_HEIGHT)
// VU Meters
#define VUMETER_INDEX		(SPINRPB_INDEX+5)
#define VUMETER_WIDTH		255
#define VUMETER_HEIGHT		19

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
	ID_SEPARATOR,
		ID_SEPARATOR,	// VU Meter
};


BEGIN_MESSAGE_MAP(CMainToolBar, CToolBarEx)
	ON_WM_VSCROLL()
END_MESSAGE_MAP()



BOOL CMainToolBar::Create(CWnd *parent)
//-------------------------------------
{
	CRect rect;
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY;

	if (!CToolBar::Create(parent, dwStyle)) return FALSE;
	if (!LoadBitmap(IDB_MAINBAR)) return FALSE;
	if (!SetButtons(MainButtons, CountOf(MainButtons))) return FALSE;

	nCurrentSpeed = 6;
	nCurrentTempo = 125;
	nCurrentRowsPerBeat = 4;
	nCurrentOctave = -1;

	// Octave Edit Box
	rect.SetRect(-EDITOCTAVE_WIDTH, -EDITOCTAVE_HEIGHT, 0, 0);
	if (!m_EditOctave.Create("", WS_CHILD|WS_BORDER|SS_LEFT|SS_CENTERIMAGE, rect, this, IDC_EDIT_BASEOCTAVE)) return FALSE;
	rect.SetRect(-SPINOCTAVE_WIDTH, -SPINOCTAVE_HEIGHT, 0, 0);
	m_SpinOctave.Create(WS_CHILD|UDS_ALIGNRIGHT, rect, this, IDC_SPIN_BASEOCTAVE);

	// Tempo Text
	rect.SetRect(-TEMPOTEXT_WIDTH, -TEMPOTEXT_HEIGHT, 0, 0);
	if (!m_StaticTempo.Create("Tempo:", WS_CHILD|SS_CENTER|SS_CENTERIMAGE, rect, this, IDC_TEXT_CURRENTTEMPO)) return FALSE;
	// Tempo EditBox
	rect.SetRect(-EDITTEMPO_WIDTH, -EDITTEMPO_HEIGHT, 0, 0);
	if (!m_EditTempo.Create("---", WS_CHILD|WS_BORDER|SS_LEFT|SS_CENTERIMAGE , rect, this, IDC_EDIT_CURRENTTEMPO)) return FALSE;
	// Tempo Spin
	rect.SetRect(-SPINTEMPO_WIDTH, -SPINTEMPO_HEIGHT, 0, 0);
	m_SpinTempo.Create(WS_CHILD|UDS_ALIGNRIGHT, rect, this, IDC_SPIN_CURRENTTEMPO);

	// Speed Text
	rect.SetRect(-SPEEDTEXT_WIDTH, -SPEEDTEXT_HEIGHT, 0, 0);
	if (!m_StaticSpeed.Create("Ticks/Row:", WS_CHILD|SS_CENTER|SS_CENTERIMAGE, rect, this, IDC_TEXT_CURRENTSPEED)) return FALSE;
	// Speed EditBox
	rect.SetRect(-EDITSPEED_WIDTH, -EDITSPEED_HEIGHT, 0, 0);
	if (!m_EditSpeed.Create("---", WS_CHILD|WS_BORDER|SS_LEFT|SS_CENTERIMAGE , rect, this, IDC_EDIT_CURRENTSPEED)) return FALSE;
	// Speed Spin
	rect.SetRect(-SPINSPEED_WIDTH, -SPINSPEED_HEIGHT, 0, 0);
	m_SpinSpeed.Create(WS_CHILD|UDS_ALIGNRIGHT, rect, this, IDC_SPIN_CURRENTSPEED);

	// Rows per Beat Text
	rect.SetRect(-RPBTEXT_WIDTH, -RPBTEXT_HEIGHT, 0, 0);
	if (!m_StaticRowsPerBeat.Create("Rows/Beat:", WS_CHILD|SS_CENTER|SS_CENTERIMAGE, rect, this, IDC_TEXT_RPB)) return FALSE;
	// Rows per Beat EditBox
	rect.SetRect(-EDITRPB_WIDTH, -EDITRPB_HEIGHT, 0, 0);
	if (!m_EditRowsPerBeat.Create("---", WS_CHILD|WS_BORDER|SS_LEFT|SS_CENTERIMAGE , rect, this, IDC_EDIT_RPB)) return FALSE;
	// Rows per Beat Spin
	rect.SetRect(-SPINRPB_WIDTH, -SPINRPB_HEIGHT, 0, 0);
	m_SpinRowsPerBeat.Create(WS_CHILD|UDS_ALIGNRIGHT, rect, this, IDC_SPIN_RPB);

	// VU Meter
	rect.SetRect(-VUMETER_WIDTH, -VUMETER_HEIGHT, 0, 0);
	//m_VuMeter.CreateEx(WS_EX_STATICEDGE, "STATIC", "", WS_CHILD | WS_BORDER | SS_NOTIFY, rect, this, IDC_VUMETER);
	m_VuMeter.Create("", WS_CHILD | WS_BORDER | SS_NOTIFY, rect, this, IDC_VUMETER);

	// Adjust control styles
	HFONT hFont = CMainFrame::GetGUIFont();
	m_EditOctave.SendMessage(WM_SETFONT, (WPARAM)hFont);
	m_EditOctave.ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_NOACTIVATE);
	m_StaticTempo.SendMessage(WM_SETFONT, (WPARAM)hFont);
	m_EditTempo.SendMessage(WM_SETFONT, (WPARAM)hFont);
	m_EditTempo.ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_NOACTIVATE);
	m_StaticSpeed.SendMessage(WM_SETFONT, (WPARAM)hFont);
	m_EditSpeed.SendMessage(WM_SETFONT, (WPARAM)hFont);
	m_EditSpeed.ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_NOACTIVATE);
	m_StaticRowsPerBeat.SendMessage(WM_SETFONT, (WPARAM)hFont);
	m_EditRowsPerBeat.SendMessage(WM_SETFONT, (WPARAM)hFont);
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
	SetCurrentSong(NULL);
	EnableDocking(CBRS_ALIGN_ANY);
	return TRUE;
}


void CMainToolBar::Init(CMainFrame *pMainFrm)
//-------------------------------------------
{
	EnableFlatButtons(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FLATBUTTONS);
	SetHorizontal();
	pMainFrm->DockControlBar(this);
}


BOOL CMainToolBar::SetHorizontal()
//--------------------------------
{
	CToolBarEx::SetHorizontal();
	m_VuMeter.SetOrientation(true);
	SetButtonInfo(EDITOCTAVE_INDEX, IDC_EDIT_BASEOCTAVE, TBBS_SEPARATOR, EDITOCTAVE_WIDTH);
	SetButtonInfo(SPINOCTAVE_INDEX, IDC_SPIN_BASEOCTAVE, TBBS_SEPARATOR, SPINOCTAVE_WIDTH);
	SetButtonInfo(TEMPOTEXT_INDEX, IDC_TEXT_CURRENTTEMPO, TBBS_SEPARATOR, TEMPOTEXT_WIDTH);
	SetButtonInfo(EDITTEMPO_INDEX, IDC_EDIT_CURRENTTEMPO, TBBS_SEPARATOR, EDITTEMPO_WIDTH);
	SetButtonInfo(SPINTEMPO_INDEX, IDC_SPIN_CURRENTTEMPO, TBBS_SEPARATOR, SPINTEMPO_WIDTH);
	SetButtonInfo(SPEEDTEXT_INDEX, IDC_TEXT_CURRENTSPEED, TBBS_SEPARATOR, SPEEDTEXT_WIDTH);
	SetButtonInfo(EDITSPEED_INDEX, IDC_EDIT_CURRENTSPEED, TBBS_SEPARATOR, EDITSPEED_WIDTH);
	SetButtonInfo(SPINSPEED_INDEX, IDC_SPIN_CURRENTSPEED, TBBS_SEPARATOR, SPINSPEED_WIDTH);
	SetButtonInfo(RPBTEXT_INDEX, IDC_TEXT_RPB, TBBS_SEPARATOR, RPBTEXT_WIDTH);
	SetButtonInfo(EDITRPB_INDEX, IDC_EDIT_RPB, TBBS_SEPARATOR, EDITRPB_WIDTH);
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

	return TRUE;
}


BOOL CMainToolBar::SetVertical()
//------------------------------
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
	if (m_EditOctave.m_hWnd != NULL) m_EditOctave.ShowWindow(SW_HIDE);
	if (m_SpinOctave.m_hWnd != NULL) m_SpinOctave.ShowWindow(SW_HIDE);
	if (m_StaticTempo.m_hWnd != NULL) m_StaticTempo.ShowWindow(SW_HIDE);
	if (m_EditTempo.m_hWnd != NULL) m_EditTempo.ShowWindow(SW_HIDE);
	if (m_SpinTempo.m_hWnd != NULL) m_SpinTempo.ShowWindow(SW_HIDE);
	if (m_StaticSpeed.m_hWnd != NULL) m_StaticSpeed.ShowWindow(SW_HIDE);
	if (m_EditSpeed.m_hWnd != NULL) m_EditSpeed.ShowWindow(SW_HIDE);
	if (m_SpinSpeed.m_hWnd != NULL) m_SpinSpeed.ShowWindow(SW_HIDE);
	if (m_StaticRowsPerBeat.m_hWnd != NULL) m_StaticRowsPerBeat.ShowWindow(SW_HIDE);
	if (m_EditRowsPerBeat.m_hWnd != NULL) m_EditRowsPerBeat.ShowWindow(SW_HIDE);
	if (m_SpinRowsPerBeat.m_hWnd != NULL) m_SpinRowsPerBeat.ShowWindow(SW_HIDE);
	EnableControl(m_VuMeter, VUMETER_INDEX, VUMETER_HEIGHT);
	//if (m_StaticBPM.m_hWnd != NULL) m_StaticBPM.ShowWindow(SW_HIDE);
	return TRUE;
}


UINT CMainToolBar::GetBaseOctave() const
//--------------------------------------
{
	if (nCurrentOctave >= MIN_BASEOCTAVE) return (UINT)nCurrentOctave;
	return 4;
}


BOOL CMainToolBar::SetBaseOctave(UINT nOctave)
//--------------------------------------------
{
	CHAR s[64];

	if ((nOctave < MIN_BASEOCTAVE) || (nOctave > MAX_BASEOCTAVE)) return FALSE;
	if (nOctave != (UINT)nCurrentOctave)
	{
		nCurrentOctave = nOctave;
		wsprintf(s, " Octave %d", nOctave);
		m_EditOctave.SetWindowText(s);
		m_SpinOctave.SetPos(nOctave);
	}
	return TRUE;
}


BOOL CMainToolBar::SetCurrentSong(CSoundFile *pSndFile)
//-----------------------------------------------------
{
	// Update Info
	if (pSndFile)
	{
		CHAR s[256];
		// Update play/pause button
		if (nCurrentTempo == -1) SetButtonInfo(PLAYCMD_INDEX, ID_PLAYER_PAUSE, TBBS_BUTTON, TOOLBAR_IMAGE_PAUSE);
		// Update Speed
		int nSpeed = pSndFile->m_nMusicSpeed;
		if (nSpeed != nCurrentSpeed)
		{
			//rewbs.envRowGrid
			CModDoc *pModDoc = CMainFrame::GetMainFrame()->GetActiveDoc();
			if (pModDoc) {
				pModDoc->UpdateAllViews(NULL, HINT_SPEEDCHANGE);
			}
			//end rewbs.envRowGrid
				
			if (nCurrentSpeed < 0) m_SpinSpeed.EnableWindow(TRUE);
			nCurrentSpeed = nSpeed;
			wsprintf(s, "%d", nCurrentSpeed);
			m_EditSpeed.SetWindowText(s);
		}
		int nTempo = pSndFile->m_nMusicTempo;
		if (nTempo != nCurrentTempo)
		{
			if (nCurrentTempo < 0) m_SpinTempo.EnableWindow(TRUE);
			nCurrentTempo = nTempo;
			wsprintf(s, "%d", nCurrentTempo);
			m_EditTempo.SetWindowText(s);
		}
		int nRowsPerBeat = pSndFile->m_nCurrentRowsPerBeat;
		if (nRowsPerBeat != nCurrentRowsPerBeat)
		{	
			if (nCurrentRowsPerBeat < 0) m_SpinRowsPerBeat.EnableWindow(TRUE);
			nCurrentRowsPerBeat = nRowsPerBeat;
			wsprintf(s, "%d", nCurrentRowsPerBeat);
			m_EditRowsPerBeat.SetWindowText(s);			
		}
	} else
	{
		if (nCurrentTempo != -1)
		{
			nCurrentTempo = -1;
			m_EditTempo.SetWindowText("---");
			m_SpinTempo.EnableWindow(FALSE);
			SetButtonInfo(PLAYCMD_INDEX, ID_PLAYER_PLAY, TBBS_BUTTON, TOOLBAR_IMAGE_PLAY);
		}
		if (nCurrentSpeed != -1)
		{
			nCurrentSpeed = -1;
			m_EditSpeed.SetWindowText("---");
			m_SpinSpeed.EnableWindow(FALSE);
		}
		if (nCurrentRowsPerBeat != -1)
		{
			nCurrentRowsPerBeat = -1;
			m_EditRowsPerBeat.SetWindowText("---");
			m_SpinRowsPerBeat.EnableWindow(FALSE);
		}
	}
	return TRUE;
}


void CMainToolBar::OnVScroll(UINT nCode, UINT nPos, CScrollBar *pScrollBar)
//-------------------------------------------------------------------------
{
	CMainFrame *pMainFrm;
	
	CToolBarEx::OnVScroll(nCode, nPos, pScrollBar);
	short int oct = (short int)m_SpinOctave.GetPos();
	if ((oct >= MIN_BASEOCTAVE) && ((int)oct != nCurrentOctave))
	{
		SetBaseOctave(oct);
	}
	if ((nCurrentSpeed < 0) || (nCurrentTempo < 0)) return;
	if ((pMainFrm = CMainFrame::GetMainFrame()) != NULL)
	{
		CSoundFile *pSndFile = pMainFrm->GetSoundFilePlaying();
		if (pSndFile)
		{
			short int n;
			if ((n = (short int)m_SpinTempo.GetPos()) != 0)
			{
				if (n < 0)
					pSndFile->SetTempo(max(nCurrentTempo - 1, pSndFile->GetModSpecifications().tempoMin), true);
				else
					pSndFile->SetTempo(min(nCurrentTempo + 1, pSndFile->GetModSpecifications().tempoMax), true);
		
				m_SpinTempo.SetPos(0);
			}
			if ((n = (short int)m_SpinSpeed.GetPos()) != 0)
			{
				if (n < 0)
				{
					pSndFile->m_nMusicSpeed = Util::Max(UINT(nCurrentSpeed - 1), pSndFile->GetModSpecifications().speedMin);
				} else
				{
					pSndFile->m_nMusicSpeed = Util::Min(UINT(nCurrentSpeed + 1), pSndFile->GetModSpecifications().speedMax);
				}
				m_SpinSpeed.SetPos(0);
			}
			if ((n = (short int)m_SpinRowsPerBeat.GetPos()) != 0)
			{
				if (n < 0)
				{
					if (nCurrentRowsPerBeat > 1)
					{
						SetRowsPerBeat(nCurrentRowsPerBeat - 1);
					}
				} else
				{
					if (static_cast<ROWINDEX>(nCurrentRowsPerBeat) < pSndFile->m_nCurrentRowsPerMeasure)
					{
						SetRowsPerBeat(nCurrentRowsPerBeat + 1);
					}
				}
				m_SpinRowsPerBeat.SetPos(0);

				//update pattern editor
				
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				if (pMainFrm)
				{
					pMainFrm->PostMessage(WM_MOD_INVALIDATEPATTERNS, HINT_MPTOPTIONS);
				}
			}

			SetCurrentSong(pSndFile);
		}
	}
}


void CMainToolBar::SetRowsPerBeat(ROWINDEX nNewRPB)
//-------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm == nullptr)
		return;
	CModDoc *pModDoc = pMainFrm->GetModPlaying();
	CSoundFile *pSndFile = pMainFrm->GetSoundFilePlaying();
	if(pModDoc == nullptr || pSndFile == nullptr)
		return;

	pSndFile->m_nCurrentRowsPerBeat = nNewRPB;
	PATTERNINDEX nPat = pSndFile->GetCurrentPattern();
	if(pSndFile->Patterns[nPat].GetOverrideSignature())
	{
		if(nNewRPB <= pSndFile->Patterns[nPat].GetRowsPerMeasure())
		{
			pSndFile->Patterns[nPat].SetSignature(nNewRPB, pSndFile->Patterns[nPat].GetRowsPerMeasure());
			pModDoc->SetModified();
		}
	} else
	{
		if(nNewRPB <= pSndFile->m_nDefaultRowsPerMeasure)
		{
			pSndFile->m_nDefaultRowsPerBeat = nNewRPB;
			pModDoc->SetModified();
		}
	}
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
	ON_MESSAGE(WM_INITDIALOG,	OnInitDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CModTreeBar::CModTreeBar()
//------------------------
{
	m_pModTree = m_pModTreeData = NULL;
	m_nTreeSplitRatio = TrackerSettings::Instance().glTreeSplitRatio;
}


LRESULT CModTreeBar::OnInitDialog(WPARAM wParam, LPARAM lParam)
//-------------------------------------------------------------
{
	LRESULT l = CDialogBar::HandleInitDialog(wParam, lParam);
	m_pModTreeData = new CModTree(nullptr);
	if (m_pModTreeData)	m_pModTreeData->SubclassDlgItem(IDC_TREEDATA, this);
	m_pModTree = new CModTree(m_pModTreeData);
	if (m_pModTree)	m_pModTree->SubclassDlgItem(IDC_TREEVIEW, this);
	m_dwStatus = 0;
	m_sizeDefault.cx = TrackerSettings::Instance().glTreeWindowWidth + 3;
	m_sizeDefault.cy = 32767;
	return l;
}


CModTreeBar::~CModTreeBar()
//-------------------------
{
	if (m_pModTree)
	{
		delete m_pModTree;
		m_pModTree = nullptr;
	}
	if (m_pModTreeData)
	{
		delete m_pModTreeData;
		m_pModTreeData = nullptr;
	}
}


void CModTreeBar::Init()
//----------------------
{
	m_nTreeSplitRatio = TrackerSettings::Instance().glTreeSplitRatio;
	if (m_pModTree)
	{
		m_pModTreeData->Init();
		m_pModTree->Init();
	}
}


VOID CModTreeBar::RefreshDlsBanks()
//---------------------------------
{
	if (m_pModTree) m_pModTree->RefreshDlsBanks();
}


VOID CModTreeBar::RefreshMidiLibrary()
//------------------------------------
{
	if (m_pModTree) m_pModTree->RefreshMidiLibrary();
}


VOID CModTreeBar::OnOptionsChanged()
//----------------------------------
{
	if (m_pModTree) m_pModTree->OnOptionsChanged();
}


VOID CModTreeBar::RecalcLayout()
//------------------------------
{
	CRect rect;

	if ((m_pModTree) && (m_pModTreeData))
	{
		int cytree, cydata, cyavail;

		GetClientRect(&rect);
		cyavail = rect.Height() - 3;
		if (cyavail < 0) cyavail = 0;
		cytree = (cyavail * m_nTreeSplitRatio) >> 8;
		cydata = cyavail - cytree;
		m_pModTree->SetWindowPos(NULL, 0,0, rect.Width(), cytree, SWP_NOZORDER|SWP_NOACTIVATE);
		m_pModTreeData->SetWindowPos(NULL, 0,cytree+3, rect.Width(), cydata, SWP_NOZORDER|SWP_NOACTIVATE);
	}
}


CSize CModTreeBar::CalcFixedLayout(BOOL, BOOL)
//--------------------------------------------
{
	CSize sz;
	m_sizeDefault.cx = TrackerSettings::Instance().glTreeWindowWidth;
	m_sizeDefault.cy = 32767;
	sz.cx = TrackerSettings::Instance().glTreeWindowWidth + 3;
	if (sz.cx < 4) sz.cx = 4;
	sz.cy = 32767;
	return sz;
}


VOID CModTreeBar::DoMouseMove(CPoint pt)
//--------------------------------------
{
	CRect rect;

	if ((m_dwStatus & (MTB_CAPTURE|MTB_DRAGGING)) && (::GetCapture() != m_hWnd))
	{
		CancelTracking();
	}
	if (m_dwStatus & MTB_DRAGGING)
	{
		if (m_dwStatus & MTB_VERTICAL)
		{
			if (m_pModTree)
			{
				m_pModTree->GetWindowRect(&rect);
				pt.y += rect.Height();
			}
			GetClientRect(&rect);
			pt.y -= ptDragging.y;
			if (pt.y < 0) pt.y = 0;
			if (pt.y > rect.Height()) pt.y = rect.Height();
			if ((!(m_dwStatus & MTB_TRACKER)) || (pt.y != (int)m_nTrackPos))
			{
				if (m_dwStatus & MTB_TRACKER) OnInvertTracker(m_nTrackPos);
				m_nTrackPos = pt.y;
				OnInvertTracker(m_nTrackPos);
				m_dwStatus |= MTB_TRACKER;
			}
		} else
		{
			pt.x -= ptDragging.x - m_cxOriginal + 3;
			if (pt.x < 0) pt.x = 0;
			if ((!(m_dwStatus & MTB_TRACKER)) || (pt.x != (int)m_nTrackPos))
			{
				if (m_dwStatus & MTB_TRACKER) OnInvertTracker(m_nTrackPos);
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
		if (rect.PtInRect(pt))
		{
			nCursor = AFX_IDC_HSPLITBAR;
		} else
		if (m_pModTree)
		{
			m_pModTree->GetWindowRect(&rect);
			rect.right = rect.Width();
			rect.left = 0;
			rect.top = rect.Height()-1;
			rect.bottom = rect.top + 5;
			if (rect.PtInRect(pt))
			{
				nCursor = AFX_IDC_VSPLITBAR;
			}
		}
		if (nCursor)
		{
			UINT nDir = (nCursor == AFX_IDC_VSPLITBAR) ? MTB_VERTICAL : 0;
			BOOL bLoad = FALSE;
			if (!(m_dwStatus & MTB_CAPTURE))
			{
				m_dwStatus |= MTB_CAPTURE;
				SetCapture();
				bLoad = TRUE;
			} else
			{
				if (nDir != (m_dwStatus & MTB_VERTICAL)) bLoad = TRUE;
			}
			m_dwStatus &= ~MTB_VERTICAL;
			m_dwStatus |= nDir;
			if (bLoad) SetCursor(theApp.LoadCursor(nCursor));
		} else
		{
			if (m_dwStatus & MTB_CAPTURE)
			{
				m_dwStatus &= ~MTB_CAPTURE;
				ReleaseCapture();
				SetCursor(LoadCursor(NULL, IDC_ARROW));
			}
		}
	}
}


VOID CModTreeBar::DoLButtonDown(CPoint pt)
//----------------------------------------
{
	if ((m_dwStatus & MTB_CAPTURE) && (!(m_dwStatus & MTB_DRAGGING)))
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


VOID CModTreeBar::DoLButtonUp()
//-----------------------------
{
	if (m_dwStatus & MTB_DRAGGING)
	{
		CRect rect;
		
		m_dwStatus &= ~MTB_DRAGGING;
		if (m_dwStatus & MTB_TRACKER)
		{
			OnInvertTracker(m_nTrackPos);
			m_dwStatus &= ~MTB_TRACKER;
		}
		if (m_dwStatus & MTB_VERTICAL)
		{
			GetClientRect(&rect);
			int cyavail = rect.Height() - 3;
			if (cyavail < 4) cyavail = 4;
			int ratio = (m_nTrackPos << 8) / cyavail;
			if (ratio < 0) ratio = 0;
			if (ratio > 256) ratio = 256;
			m_nTreeSplitRatio = ratio;
			TrackerSettings::Instance().glTreeSplitRatio = ratio;
			RecalcLayout();
		} else
		{
			GetWindowRect(&rect);
			m_nTrackPos += 3;
			if (m_nTrackPos < 4) m_nTrackPos = 4;
			CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
			if ((m_nTrackPos != (UINT)rect.Width()) && (pMainFrm))
			{
				TrackerSettings::Instance().glTreeWindowWidth = m_nTrackPos - 3;
				m_sizeDefault.cx = m_nTrackPos;
				m_sizeDefault.cy = 32767;
				pMainFrm->RecalcLayout();
			}
		}
	}
}


VOID CModTreeBar::CancelTracking()
//--------------------------------
{
	if (m_dwStatus & MTB_TRACKER)
	{
		OnInvertTracker(m_nTrackPos);
		m_dwStatus &= ~MTB_TRACKER;
	}
	m_dwStatus &= ~MTB_DRAGGING;
	if (m_dwStatus & MTB_CAPTURE)
	{
		m_dwStatus &= ~MTB_CAPTURE;
		ReleaseCapture();
	}
}


void CModTreeBar::OnInvertTracker(UINT x)
//---------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	if (pMainFrm)
	{
		CRect rect;

		GetClientRect(&rect);
		if (m_dwStatus & MTB_VERTICAL)
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
		if (pBrush != NULL)
			hOldBrush = (HBRUSH)SelectObject(pDC->m_hDC, pBrush->m_hObject);
		pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATINVERT);
		if (hOldBrush != NULL)
			SelectObject(pDC->m_hDC, hOldBrush);
		ReleaseDC(pDC);
	}
}


VOID CModTreeBar::OnDocumentCreated(CModDoc *pModDoc)
//---------------------------------------------------
{
	if (m_pModTree) m_pModTree->AddDocument(pModDoc);
}


VOID CModTreeBar::OnDocumentClosed(CModDoc *pModDoc)
//--------------------------------------------------
{
	if (m_pModTree) m_pModTree->RemoveDocument(pModDoc);
}


VOID CModTreeBar::OnUpdate(CModDoc *pModDoc, DWORD lHint, CObject *pHint)
//-----------------------------------------------------------------------
{
	if (m_pModTree) m_pModTree->OnUpdate(pModDoc, lHint, pHint);
}


VOID CModTreeBar::UpdatePlayPos(CModDoc *pModDoc, Notification *pNotify)
//----------------------------------------------------------------------
{
	if (m_pModTree) m_pModTree->UpdatePlayPos(pModDoc, pNotify);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// CModTreeBar message handlers

void CModTreeBar::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
//-----------------------------------------------------------------------------
{
	CDialogBar::OnNcCalcSize(bCalcValidRects, lpncsp);
	if (lpncsp)
	{
		lpncsp->rgrc[0].right -= 3;
		if (lpncsp->rgrc[0].right < lpncsp->rgrc[0].left) lpncsp->rgrc[0].right = lpncsp->rgrc[0].left;
	}
}


#if _MFC_VER > 0x0710
LRESULT CModTreeBar::OnNcHitTest(CPoint point)
#else
UINT CModTreeBar::OnNcHitTest(CPoint point)
#endif 
//-----------------------------------------
{
	CRect rect;

	GetWindowRect(&rect);
	rect.DeflateRect(1,1);
	rect.right -= 3;
	if (!rect.PtInRect(point)) return HTBORDER;
	return CDialogBar::OnNcHitTest(point);
}


void CModTreeBar::OnNcPaint()
//---------------------------
{
	RECT rect;
	CDialogBar::OnNcPaint();

	GetWindowRect(&rect);
	// Assumes there is no other non-client items
	rect.right -= rect.left;
	rect.bottom -= rect.top;
	rect.top = 0;
	rect.left = rect.right - 3;
	if ((rect.left < rect.right) && (rect.top < rect.bottom))
	{
		CDC *pDC = GetWindowDC();
		HDC hdc = pDC->m_hDC;
		if (rect.left < rect.right) FillRect(hdc, &rect, CMainFrame::brushGray);
		ReleaseDC(pDC);
	}
}


void CModTreeBar::OnSize(UINT nType, int cx, int cy)
//--------------------------------------------------
{
	CDialogBar::OnSize(nType, cx, cy);
	RecalcLayout();
}


void CModTreeBar::OnNcMouseMove(UINT, CPoint point)
//-------------------------------------------------
{
	CRect rect;
	CPoint pt = point;
	
	GetWindowRect(&rect);
	pt.x -= rect.left;
	pt.y -= rect.top;
	DoMouseMove(pt);
}


void CModTreeBar::OnMouseMove(UINT, CPoint point)
//-----------------------------------------------
{
	DoMouseMove(point);
}


void CModTreeBar::OnNcLButtonDown(UINT, CPoint point)
//---------------------------------------------------
{
	CRect rect;
	CPoint pt = point;
	
	GetWindowRect(&rect);
	pt.x -= rect.left;
	pt.y -= rect.top;
	DoLButtonDown(pt);
}


void CModTreeBar::OnLButtonDown(UINT, CPoint point)
//-------------------------------------------------
{
	DoLButtonDown(point);
}


void CModTreeBar::OnNcLButtonUp(UINT, CPoint)
//-------------------------------------------
{
	DoLButtonUp();
}


void CModTreeBar::OnLButtonUp(UINT, CPoint)
//-----------------------------------------
{
	DoLButtonUp();
}

//rewbs.customKeys
HWND CModTreeBar::GetModTreeHWND()
{
	return m_pModTree->m_hWnd;
}

BOOL CModTreeBar::PostMessageToModTree(UINT cmdID, WPARAM wParam, LPARAM lParam)
{
	if (::GetFocus() == m_pModTree->m_hWnd)
		return m_pModTree->SendMessage(cmdID, wParam, lParam);
	if (::GetFocus() == m_pModTreeData->m_hWnd)
		return m_pModTreeData->SendMessage(cmdID, wParam, lParam);

	return 0;
}
//end rewbs.customKeys


////////////////////////////////////////////////////////////////////////////////
//
// Stereo VU Meter for toolbar
//

BEGIN_MESSAGE_MAP(CStereoVU, CStatic)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


void CStereoVU::OnPaint()
//-----------------------
{
	CRect rect;
	CPaintDC dc(this);
	GetClientRect(&rect);
	dc.FillSolidRect(rect.left, rect.top, rect.Width(), rect.Height(), RGB(0,0,0));
	DrawVuMeters(dc, true);
}


void CStereoVU::SetVuMeter(uint32 left, uint32 right, bool force)
//---------------------------------------------------------------
{
	if(left != vuMeter[0] || right != vuMeter[1])
	{
		vuMeter[0] = left;
		vuMeter[1] = right;
		DWORD curTime = timeGetTime();
		if(curTime - lastVuUpdateTime >= TrackerSettings::Instance().VuMeterUpdateInterval || force)
		{
			CClientDC dc(this);
			DrawVuMeters(dc);
			lastVuUpdateTime = curTime;
		}
	}
}


// Draw stereo VU
void CStereoVU::DrawVuMeters(CDC &dc, bool redraw)
//------------------------------------------------
{
	CRect rect;
	GetClientRect(&rect);
	CRect rectL = rect, rectR = rect;

	if(horizontal)
	{
		const int mid = rect.top + rect.Height() / 2;
		rectL.bottom = mid;

		rectR.top = mid + 1;
	} else
	{
		const int mid = rect.left + rect.Width() / 2;
		rectL.right = mid;

		rectR.left = mid + 1;
	}

	DrawVuMeter(dc, rectL, 0, redraw);
	DrawVuMeter(dc, rectR, 1, redraw);
}


// Draw a single VU Meter
void CStereoVU::DrawVuMeter(CDC &dc, const CRect &rect, int index, bool redraw)
//-----------------------------------------------------------------------------
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
		const int cx = Util::Max(1, rect.Width());
		int v = (vu * cx) >> 8;

		for(int rx = rect.left; rx <= rect.right; rx += 2)
		{
			int pen = Clamp((rx * NUM_VUMETER_PENS) / cx, 0, NUM_VUMETER_PENS - 1);
			const bool last = (rx == rect.right - 1);

			// Darken everything above volume, unless it's the clip indicator
			if(v <= rx && (!last || !clip))
				pen += NUM_VUMETER_PENS;

			bool draw = redraw || (v < lastV[index] && v<=rx && rx<=lastV[index]) || (lastV[index] < v && lastV[index]<=rx && rx<=v);
			draw = draw || (last && clip != lastClip[index]);
			if(draw) dc.FillSolidRect(rx, rect. top, 1, rect.Height(), CMainFrame::gcolrefVuMeter[pen]);
			if(last) lastClip[index] = clip;
		}
		lastV[index] = v;
	} else
	{
		const int cy = Util::Max(1, rect.Height());
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
//-----------------------------------------
{
	// Reset clip indicator.
	CMainFrame::gnClipLeft = CMainFrame::gnClipRight = false;
}