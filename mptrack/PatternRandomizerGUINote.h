#pragma once


// CPatternRandomizerGUINote dialog

class CPatternRandomizerGUINote : public CDialog
{
	DECLARE_DYNAMIC(CPatternRandomizerGUINote)

public:
	CPatternRandomizerGUINote(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPatternRandomizerGUINote();

// Dialog Data
	enum { IDD = IDD_PATTERNRANDOMIZER_NOTE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
