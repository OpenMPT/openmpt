// PatternRandomizerGUIInstrument.cpp : implementation file
//

#include "stdafx.h"
#include "mptrack.h"
#include "PatternRandomizerGUIInstrument.h"
#include ".\patternrandomizerguiinstrument.h"


// CPatternRandomizerGUIInstrument dialog

IMPLEMENT_DYNAMIC(CPatternRandomizerGUIInstrument, CDialog)
CPatternRandomizerGUIInstrument::CPatternRandomizerGUIInstrument(CWnd* pParent /*=NULL*/)
	: CDialog(CPatternRandomizerGUIInstrument::IDD, pParent)
{
}

CPatternRandomizerGUIInstrument::~CPatternRandomizerGUIInstrument()
{
}

void CPatternRandomizerGUIInstrument::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPatternRandomizerGUIInstrument, CDialog)
	ON_BN_CLICKED(65535, OnBnClicked65535)
END_MESSAGE_MAP()


// CPatternRandomizerGUIInstrument message handlers

void CPatternRandomizerGUIInstrument::OnBnClicked65535()
{
	// TODO: Add your control notification handler code here
}
