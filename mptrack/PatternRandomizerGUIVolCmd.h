#pragma once


// CPatternRandomizerGUIVolCmd dialog

class CPatternRandomizerGUIVolCmd : public CDialog
{
	DECLARE_DYNAMIC(CPatternRandomizerGUIVolCmd)

public:
	CPatternRandomizerGUIVolCmd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPatternRandomizerGUIVolCmd();

// Dialog Data
	enum { IDD = IDD_PATTERNRANDOMIZER_VOLCMD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
