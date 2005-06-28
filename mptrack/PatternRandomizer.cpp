#include "stdafx.h"
#include "mptrack.h"
#include ".\patternrandomizer.h"
#include ".\patternrandomizerGUI.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "dlg_misc.h"
#include "globals.h"
#include ".\view_pat.h"

CPatternRandomizer::CPatternRandomizer(CViewPattern* viewPat)
{
	m_pViewPattern = viewPat;
	m_pRandomizerGUI = new CPatternRandomizerGUI(this, (CWnd*) m_pViewPattern);
}

CPatternRandomizer::~CPatternRandomizer(void)
{
	delete m_pRandomizerGUI;
}

bool CPatternRandomizer::showGUI()
{
	return m_pRandomizerGUI->openEditor();
}

bool CPatternRandomizer::isGUIVisible()
{
	return m_pRandomizerGUI->isVisible();
}