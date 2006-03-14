// PatternRandomizerGUIEffect.cpp : implementation file
//

#include "stdafx.h"
#include "mptrack.h"
#include "PatternRandomizerGUIEffect.h"


// CPatternRandomizerGUIEffect dialog

IMPLEMENT_DYNAMIC(CPatternRandomizerGUIEffect, CDialog)
CPatternRandomizerGUIEffect::CPatternRandomizerGUIEffect(CWnd* pParent /*=NULL*/)
	: CDialog(CPatternRandomizerGUIEffect::IDD, pParent)
{
}

CPatternRandomizerGUIEffect::~CPatternRandomizerGUIEffect()
{
}

void CPatternRandomizerGUIEffect::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPatternRandomizerGUIEffect, CDialog)
END_MESSAGE_MAP()


// CPatternRandomizerGUIEffect message handlers
