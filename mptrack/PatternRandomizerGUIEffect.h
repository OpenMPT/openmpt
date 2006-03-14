#pragma once


// CPatternRandomizerGUIEffect dialog

class CPatternRandomizerGUIEffect : public CDialog
{
	DECLARE_DYNAMIC(CPatternRandomizerGUIEffect)

public:
	CPatternRandomizerGUIEffect(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPatternRandomizerGUIEffect();

// Dialog Data
	enum { IDD = IDD_PATTERNRANDOMIZER_EFFECT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
