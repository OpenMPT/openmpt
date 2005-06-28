#pragma once


// CPatternRandomizerGUIInstrument dialog

class CPatternRandomizerGUIInstrument : public CDialog
{
	DECLARE_DYNAMIC(CPatternRandomizerGUIInstrument)

public:
	CPatternRandomizerGUIInstrument(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPatternRandomizerGUIInstrument();

// Dialog Data
	enum { IDD = IDD_PATTERNRANDOMIZER_INSTRUMENT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClicked65535();
};
