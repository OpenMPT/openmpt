#pragma once
#include "mptrack.h"
#include "MainFrm.h"
#include "VstPlug.h"
#include "abstractvsteditor.h"

enum {
	PARAM_RESOLUTION=1000,
};

class CDefaultVstEditor :
	public CAbstractVstEditor
{
public:
	CListBox m_lbParameters;
	CSliderCtrl m_slParam;
	CEdit m_editParam;
	CStatic m_statParamLabel;
	int m_nControlLock;

	long m_nCurrentParam;

	CDefaultVstEditor(CVstPlugin *pPlugin);
	virtual ~CDefaultVstEditor(void);
	virtual VOID OnOK();
	virtual VOID OnCancel();
	BOOL OpenEditor(CWnd *parent);
	VOID DoClose();
	
	void OnParamChanged();

	afx_msg void OnClose();
	afx_msg void OnLoadPreset();
	afx_msg void OnSavePreset();
	afx_msg void OnRandomizePreset();
	afx_msg void OnParamTextboxChanged();
	afx_msg void OnParamSliderChanged();

	virtual void DoDataExchange(CDataExchange* pDX);
	
	DECLARE_MESSAGE_MAP()

	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
private:

	void UpdateParamDisplays();
};
