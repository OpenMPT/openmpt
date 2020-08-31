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

#include "BuildSettings.h"

#include "../soundlib/SampleIO.h"
#include "../tracklib/FadeLaws.h"

OPENMPT_NAMESPACE_BEGIN

enum OpenSampleTypes
{
	OpenSampleKnown = (1<<0),
	OpenSampleRaw   = (1<<1),
};
MPT_DECLARE_ENUM(OpenSampleTypes)

class CCtrlSamples: public CModControlDlg
{
protected:
	friend class DoPitchShiftTimeStretch;

	struct SampleSelectionPoints
	{
		SmpLength nStart;
		SmpLength nEnd;
		bool selectionActive;	// does sample selection exist or not?
	};

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
	double m_dTimeStretchRatio = 100;
	uint32 m_nSequenceMs = 0;
	uint32 m_nSeekWindowMs = 0;
	uint32 m_nOverlapMs = 0;
	SampleIO m_nPreviousRawFormat;
	SAMPLEINDEX m_nSample = 1;
	bool m_rememberRawFormat = false;
	bool m_startedEdit = false;

	CComboBox m_ComboPitch, m_ComboQuality, m_ComboFFT;

	void UpdateTimeStretchParameterString();
	void ReadTimeStretchParameters();

	void ApplyAmplify(const double amp, const double fadeInStart, const double fadeOutEnd, const bool fadeIn, const bool fadeOut, const Fade::Law fadeLaw);
	void ApplyResample(uint32 newRate, ResamplingMode mode);

	SampleSelectionPoints GetSelectionPoints();
	void SetSelectionPoints(SmpLength nStart, SmpLength nEnd);

	void PropagateAutoVibratoChanges();

	bool IsOPLInstrument() const;

public:
	CCtrlSamples(CModControlView &parent, CModDoc &document);
	~CCtrlSamples();

	bool SetCurrentSample(SAMPLEINDEX nSmp, LONG lZoom = -1, bool bUpdNum = true);
	bool InsertSample(bool duplicate, int8 *confirm = nullptr);
	bool OpenSample(const mpt::PathString &fileName, FlagSet<OpenSampleTypes> types = OpenSampleKnown | OpenSampleRaw);
	bool OpenSample(const CSoundFile &sndFile, SAMPLEINDEX nSample);
	void OpenSamples(const std::vector<mpt::PathString> &files, FlagSet<OpenSampleTypes> types);
	void SaveSample(bool doBatchSave);

	void Normalize(bool allSamples);
	void RemoveDCOffset(bool allSamples);

	Setting<LONG> &GetSplitPosRef() override {return TrackerSettings::Instance().glSampleWindowHeight;}

public:
	//{{AFX_VIRTUAL(CCtrlSamples)
	BOOL OnInitDialog() override;
	void DoDataExchange(CDataExchange* pDX) override;	// DDX/DDV support
	CRuntimeClass *GetAssociatedViewClass() override;
	void RecalcLayout() override;
	void OnActivatePage(LPARAM) override;
	void OnDeactivatePage() override;
	void UpdateView(UpdateHint hint, CObject *pObj = nullptr) override;
	LRESULT OnModCtrlMsg(WPARAM wParam, LPARAM lParam) override;
	BOOL GetToolTipText(UINT uId, LPTSTR pszText) override;
	BOOL PreTranslateMessage(MSG* pMsg) override;
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CCtrlSamples)
	afx_msg void OnEditFocus();
	afx_msg void OnSampleChanged();
	afx_msg void OnZoomChanged();
	afx_msg void OnPrevInstrument();
	afx_msg void OnNextInstrument();
	afx_msg void OnTbnDropDownToolBar(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSampleNew();
	afx_msg void OnSampleDuplicate() { InsertSample(true); }
	afx_msg void OnSampleOpen();
	afx_msg void OnSampleOpenKnown();
	afx_msg void OnSampleOpenRaw();
	afx_msg void OnSampleSave();
	afx_msg void OnSampleSaveOne() { SaveSample(false); }
	afx_msg void OnSampleSaveAll() { SaveSample(true); }
	afx_msg void OnSamplePlay();
	afx_msg void OnNormalize();
	afx_msg void OnAmplify();
	afx_msg void OnQuickFade();
	afx_msg void OnRemoveDCOffset();
	afx_msg void OnResample();
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
	afx_msg void OnFineTuneChangedDone();
	afx_msg void OnBaseNoteChanged();
	afx_msg void OnLoopTypeChanged();
	afx_msg void OnLoopPointsChanged();
	afx_msg void OnSustainTypeChanged();
	afx_msg void OnSustainPointsChanged();
	afx_msg void OnVibTypeChanged();
	afx_msg void OnVibDepthChanged();
	afx_msg void OnVibSweepChanged();
	afx_msg void OnVibRateChanged();
	afx_msg void OnXFade();
	afx_msg void OnStereoSeparation();
	afx_msg void OnKeepSampleOnDisk();
	afx_msg void OnVScroll(UINT, UINT, CScrollBar *);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM); //rewbs.customKeys
	afx_msg void OnXButtonUp(UINT nFlags, UINT nButton, CPoint point);

	afx_msg void OnPitchShiftTimeStretch();
	afx_msg void OnEnableStretchToSize();
	afx_msg void OnEstimateSampleSize();

	afx_msg void OnInitOPLInstrument();

	MPT_NOINLINE void SetModified(SampleHint hint, bool updateAll, bool waveformModified);
	void PrepareUndo(const char *description, sampleUndoTypes type = sundo_none, SmpLength start = 0, SmpLength end = 0);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
