#ifndef _CONTROL_GENERAL_H_
#define _CONTROL_GENERAL_H_

//=========================
class CVuMeter: public CWnd
//=========================
{
protected:
	LONG m_nDisplayedVu, m_nVuMeter;

public:
	CVuMeter() { m_nDisplayedVu = -1; m_nVuMeter = 0; }
	VOID SetVuMeter(LONG lVuMeter);

protected:
	VOID DrawVuMeter(HDC hdc);

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP();
};

enum {
	MAX_SLIDER_GLOBAL_VOL=256,
	MAX_SLIDER_VSTI_VOL=256,
	MAX_SLIDER_SAMPLE_VOL=256
};


//=======================================
class CCtrlGeneral: public CModControlDlg
//=======================================
{
public:
	CCtrlGeneral();
	LONG* GetSplitPosRef() {return &CMainFrame::glGeneralWindowHeight;} 	//rewbs.varWindowSize

private:
	void setAsDecibels(LPSTR stringToSet, double value, double valueAtZeroDB);

public:
	bool m_bEditsLocked;
	//{{AFX_DATA(CCtrlGeneral)
	CEdit m_EditTitle;
	CStatic m_EditModType;
	CEdit m_EditTempo, m_EditSpeed, m_EditGlobalVol, m_EditRestartPos,
		  m_EditSamplePA, m_EditVSTiVol;
	CSpinButtonCtrl m_SpinTempo, m_SpinSpeed, m_SpinGlobalVol, m_SpinRestartPos, 
				    m_SpinSamplePA, m_SpinVSTiVol;

	CSliderCtrl m_SliderTempo, m_SliderSamplePreAmp, m_SliderGlobalVol, m_SliderVSTiVol;
	CComboBox m_ComboResampling;
	CVuMeter m_VuMeterLeft, m_VuMeterRight;
	//}}AFX_DATA
	//{{AFX_VIRTUAL(CCtrlGeneral)
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void RecalcLayout();
	virtual void UpdateView(DWORD dwHintMask=0, CObject *pObj=NULL);
	virtual CRuntimeClass *GetAssociatedViewClass();
	virtual void OnActivatePage(LPARAM);
	virtual void OnDeactivatePage();
	virtual BOOL GetToolTipText(UINT uId, LPSTR pszText);
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CCtrlGeneral)
	afx_msg LRESULT OnUpdatePosition(WPARAM, LPARAM);
	afx_msg void OnVScroll(UINT, UINT, CScrollBar *);
	afx_msg void OnTitleChanged();
	afx_msg void OnTempoChanged();
	afx_msg void OnSpeedChanged();
	afx_msg void OnGlobalVolChanged();
	afx_msg void OnVSTiVolChanged();
	afx_msg void OnSamplePAChanged();
	afx_msg void OnRestartPosChanged();
	afx_msg void OnSongProperties();
	afx_msg void OnPlayerProperties();
	afx_msg void OnResamplingChanged();
	afx_msg void OnLoopSongChanged();
	afx_msg void OnAGCChanged();
	afx_msg void OnXBassChanged();
	afx_msg void OnReverbChanged();
	afx_msg void OnSurroundChanged();
	afx_msg void OnEqualizerChanged();
	afx_msg void OnEnSetfocusEditSongtitle();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif