// PatternRandomizerGUINote.cpp : implementation file
//

#include "stdafx.h"
#include "mptrack.h"
#include "PatternRandomizerGUINote.h"


// CPatternRandomizerGUINote dialog

IMPLEMENT_DYNAMIC(CPatternRandomizerGUINote, CDialog)
CPatternRandomizerGUINote::CPatternRandomizerGUINote(CWnd* pParent /*=NULL*/)
	: CDialog(CPatternRandomizerGUINote::IDD, pParent)
{
}

CPatternRandomizerGUINote::~CPatternRandomizerGUINote()
{
}

void CPatternRandomizerGUINote::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPatternRandomizerGUINote, CDialog)
END_MESSAGE_MAP()


// CPatternRandomizerGUINote message handlers
