#pragma once
#include "afxcmn.h"
#include "PatternRandomizerGUINote.h"
#include "PatternRandomizerGUIInstrument.h"
#include "PatternRandomizerGUIVolCmd.h"
#include "PatternRandomizerGUIEffect.h"

class CPatternRandomizer;


// CPatternRandomizerGUI dialog
enum {
	tab_rand_notes = 0,
	tab_rand_instruments,
	tab_rand_volcmds,
	tab_rand_effects,
};

class CPatternRandomizerGUI : public CDialog
{
	DECLARE_DYNAMIC(CPatternRandomizerGUI)

public:
	CPatternRandomizerGUI(CPatternRandomizer* patRandom, CWnd* pParent = NULL);   // standard constructor
	virtual ~CPatternRandomizerGUI();

	bool openEditor();
	bool isVisible();

// Dialog Data
	enum { IDD = IDD_PATTERNRANDOMIZER };

protected:
	CWnd* m_pParentWindow;
	CPatternRandomizer* m_pPatternRandomizer;
	
	CWnd* m_pCurrentTab;
	CPatternRandomizerGUINote m_RandomizerNote;
	CPatternRandomizerGUIInstrument m_RandomizerInstrument;
	CPatternRandomizerGUIEffect m_RandomizerEffect;
	CPatternRandomizerGUIVolCmd m_RandomizerVolCmd;
	CTabCtrl m_RandomizerTabs;

	void setTab(CWnd *newTab);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	

public:
	afx_msg void OnRandomiserTabChange(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnClose();
};
