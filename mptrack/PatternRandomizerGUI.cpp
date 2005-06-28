// PatternRandomizerGUI.cpp : implementation file
//
#include "stdafx.h"
#include "mptrack.h"
#include ".\patternrandomizergui.h"


// CPatternRandomizerGUI dialog

IMPLEMENT_DYNAMIC(CPatternRandomizerGUI, CDialog)
CPatternRandomizerGUI::CPatternRandomizerGUI(CPatternRandomizer *patRandom, CWnd* pParent /*=NULL*/)
	: CDialog(CPatternRandomizerGUI::IDD, pParent)
{
	m_pParentWindow = pParent;
	m_pPatternRandomizer = patRandom;
	m_pCurrentTab = NULL;
	Create(IDD_PATTERNRANDOMIZER, m_pParentWindow);
}

CPatternRandomizerGUI::~CPatternRandomizerGUI()
{
}

void CPatternRandomizerGUI::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB1, m_RandomizerTabs);
}


BEGIN_MESSAGE_MAP(CPatternRandomizerGUI, CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnRandomiserTabChange)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CPatternRandomizerGUI message handlers

BOOL CPatternRandomizerGUI::OnInitDialog()
//----------------------------------------
{
	CDialog::OnInitDialog();

	CRect rect;
	GetDlgItem(IDC_RANDOM_PLACEHOLDER)->GetWindowRect(rect);
	int hOff = 8, vOff = 86;
	int width = 100, height = 200;

	m_RandomizerNote.Create(CPatternRandomizerGUINote::IDD, (CWnd*)this);
	m_RandomizerNote.MoveWindow(hOff, vOff, rect.Width(), rect.Height());
	m_RandomizerInstrument.Create(CPatternRandomizerGUIInstrument::IDD, (CWnd*)this);
	m_RandomizerInstrument.MoveWindow(hOff, vOff, rect.Width(), rect.Height());
	m_RandomizerVolCmd.Create(CPatternRandomizerGUIVolCmd::IDD, (CWnd*)this);
	m_RandomizerVolCmd.MoveWindow(hOff, vOff, rect.Width(), rect.Height());
	m_RandomizerEffect.Create(CPatternRandomizerGUIEffect::IDD, (CWnd*)this);
	m_RandomizerEffect.MoveWindow(hOff, vOff, rect.Width(), rect.Height());

/*	m_RandomizerNote.Create(CPatternRandomizerGUINote::IDD, (CWnd*)this);
	m_RandomizerNote.MoveWindow(hOff, vOff, width, height);
	m_RandomizerInstrument.Create(CPatternRandomizerGUIInstrument::IDD, (CWnd*)this);
	m_RandomizerInstrument.MoveWindow(hOff, vOff, width, height);
	m_RandomizerVolCmd.Create(CPatternRandomizerGUIVolCmd::IDD, (CWnd*)this);
	m_RandomizerVolCmd.MoveWindow(hOff, vOff, width, height);
	m_RandomizerEffect.Create(CPatternRandomizerGUIEffect::IDD, (CWnd*)this);
	m_RandomizerEffect.MoveWindow(hOff, vOff, width, height);
*/
	m_RandomizerTabs.InsertItem(tab_rand_notes, "Notes");
	m_RandomizerTabs.InsertItem(tab_rand_instruments, "Instruments");
	m_RandomizerTabs.InsertItem(tab_rand_volcmds, "Vol Commands");
	m_RandomizerTabs.InsertItem(tab_rand_effects, "Effects");

	setTab(&m_RandomizerNote);
	return TRUE;
}

bool CPatternRandomizerGUI::openEditor() {
//----------------------------------
	ShowWindow(SW_SHOW);
	return TRUE;
}

bool CPatternRandomizerGUI::isVisible() {
//----------------------------------
	return IsWindowVisible();
}


void CPatternRandomizerGUI::setTab(CWnd *newTab) {
//------------------------------------------------
	if (m_pCurrentTab != NULL) {
		m_pCurrentTab->ShowWindow(SW_HIDE);
	}
	m_pCurrentTab = newTab;
	//m_pCurrentTab->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE);
	m_pCurrentTab->ShowWindow(SW_SHOW);
}


void CPatternRandomizerGUI::OnRandomiserTabChange(NMHDR *pNMHDR, LRESULT *pResult)
//--------------------------------------------------------------------------------
{
	int nActiveTab = m_RandomizerTabs.GetCurSel();

	switch(nActiveTab) {
		case tab_rand_notes:
			setTab(&m_RandomizerNote);
			break;
		case tab_rand_instruments:
			setTab(&m_RandomizerInstrument);
			break;
		case tab_rand_volcmds:
			setTab(&m_RandomizerVolCmd);
			break;
		case tab_rand_effects:
			setTab(&m_RandomizerEffect);
			break;
		default:
			ASSERT(false);
	}

	*pResult = 0;
}

void CPatternRandomizerGUI::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	CDialog::OnClose();
}
