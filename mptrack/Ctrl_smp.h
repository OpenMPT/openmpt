/*
 * ctrl_smp.h
 * ----------
 * Purpose: Sample tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

struct SampleSelectionPoints
{
	UINT nStart;
	UINT nEnd;
	bool selectionActive;	// does sample selection exist or not?
};

//=======================================
class CCtrlSamples: public CModControlDlg
//=======================================
{
protected:
	CModControlBar m_ToolBar1, m_ToolBar2;
	CEdit m_EditSample, m_EditName, m_EditFileName, m_EditFineTune;
	CEdit m_EditLoopStart, m_EditLoopEnd, m_EditSustainStart, m_EditSustainEnd;
	CEdit m_EditVibSweep, m_EditVibDepth, m_EditVibRate;
	CEdit m_EditVolume, m_EditGlobalVol, m_EditPanning;
	CSpinButtonCtrl m_SpinVolume, m_SpinGlobalVol, m_SpinPanning, m_SpinVibSweep, m_SpinVibDepth, m_SpinVibRate;
	CSpinButtonCtrl m_SpinLoopStart, m_SpinLoopEnd, m_SpinSustainStart, m_SpinSustainEnd;
	CSpinButtonCtrl m_SpinFineTune, m_SpinSample;
	CComboBox m_ComboAutoVib, m_ComboLoopType, m_ComboSustainType, m_ComboZoom, m_CbnBaseNote;
	CButton m_CheckPanning;
	SAMPLEINDEX m_nSample;
	double m_dTimeStretchRatio; //rewbs.timeStretchMods
	uint32 m_nStretchProcessStepLength;
	uint32 m_nSequenceMs;
	uint32 m_nSeekWindowMs;
	uint32 m_nOverlapMs;
	uint16 m_nFinetuneStep; // Increment finetune by x when using spin control. Default = 25
	enum {nDefaultStretchChunkSize = 8192};
	SampleIO m_nPreviousRawFormat;
	bool rememberRawFormat;

	CComboBox m_ComboPitch, m_ComboQuality, m_ComboFFT;

	int PitchShift(float pitch);
	int TimeStretch(float ratio);
	void UpdateTimeStretchParameterString();
	void ReadTimeStretchParameters();

	// Applies amplification to sample. Negative values
	// can be used to invert phase.
	void ApplyAmplify(LONG nAmp, bool bFadeIn = false, bool bFadeOut = false);

	SampleSelectionPoints GetSelectionPoints();
	void SetSelectionPoints(UINT nStart, UINT nEnd);

	void PropagateAutoVibratoChanges() const;

public:
	CCtrlSamples();
	~CCtrlSamples();

	bool SetCurrentSample(SAMPLEINDEX nSmp, LONG lZoom = -1, bool bUpdNum = true);
	bool OpenSample(LPCSTR lpszFileName);
	bool OpenSample(const CSoundFile *pSndFile, SAMPLEINDEX nSample);
	LONG* GetSplitPosRef() {return &TrackerSettings::Instance().glSampleWindowHeight;} 	//rewbs.varWindowSize

public:
	//{{AFX_VIRTUAL(CCtrlSamples)
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual CRuntimeClass *GetAssociatedViewClass();
	virtual void RecalcLayout();
	virtual void OnActivatePage(LPARAM);
	virtual void OnDeactivatePage();
	virtual void UpdateView(DWORD dwHintMask=0, CObject *pObj=NULL);
	virtual LRESULT OnModCtrlMsg(WPARAM wParam, LPARAM lParam);
	virtual BOOL GetToolTipText(UINT uId, LPSTR pszText);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CCtrlSamples)
	afx_msg void OnSampleChanged();
	afx_msg void OnZoomChanged();
	afx_msg void OnPrevInstrument();
	afx_msg void OnNextInstrument();
	afx_msg void OnSampleNew();
	afx_msg void OnSampleOpen();
	afx_msg void OnSampleSave();
	afx_msg void OnSamplePlay();
	afx_msg void OnNormalize();
	afx_msg void OnAmplify();
	afx_msg void OnQuickFade();
	afx_msg void OnRemoveDCOffset();
	afx_msg void OnUpsample();
	afx_msg void OnDownsample();
	afx_msg void OnReverse();
	afx_msg void OnSilence();
	afx_msg void OnInvert();
	afx_msg void OnSignUnSign();
	afx_msg void OnAutotune();
	afx_msg void OnNameChanged();
	afx_msg void OnFileNameChanged();
	afx_msg void OnVolumeChanged();
	afx_msg void OnGlobalVolChanged();
	afx_msg void OnSetPanningChanged();
	afx_msg void OnPanningChanged();
	afx_msg void OnFineTuneChanged();
	afx_msg void OnBaseNoteChanged();
	afx_msg void OnLoopTypeChanged();
	afx_msg void OnLoopStartChanged();
	afx_msg void OnLoopEndChanged();
	afx_msg void OnSustainTypeChanged();
	afx_msg void OnSustainStartChanged();
	afx_msg void OnSustainEndChanged();
	afx_msg void OnVibTypeChanged();
	afx_msg void OnVibDepthChanged();
	afx_msg void OnVibSweepChanged();
	afx_msg void OnVibRateChanged();
	afx_msg void OnXFade();
	afx_msg void OnVScroll(UINT, UINT, CScrollBar *);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM); //rewbs.customKeys

	afx_msg void OnPitchShiftTimeStretch();
	afx_msg void OnEnableStretchToSize();
	afx_msg void OnEstimateSampleSize();

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
