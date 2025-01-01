/*
 * Ctrl_smp.h
 * ----------
 * Purpose: Sample tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "AccessibleControls.h"
#include "CDecimalSupport.h"
#include "Globals.h"
#include "Undo.h"
#include "UpdateHints.h"
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
	CNumberEdit m_EditTimeStretchRatio;
	CSpinButtonCtrl m_SpinVolume, m_SpinGlobalVol, m_SpinPanning, m_SpinVibSweep, m_SpinVibDepth, m_SpinVibRate;
	CSpinButtonCtrl m_SpinLoopStart, m_SpinLoopEnd, m_SpinSustainStart, m_SpinSustainEnd;
	CSpinButtonCtrl m_SpinFineTune, m_SpinSample, m_SpinTimeStretchRatio;
	CComboBox m_ComboAutoVib, m_ComboLoopType, m_ComboSustainType, m_ComboZoom, m_CbnBaseNote, m_ComboGrainSize;
	CButton m_CheckPanning;
	SAMPLEINDEX m_nSample = 1;
	INSTRUMENTINDEX m_editInstrumentName = INSTRUMENTINDEX_INVALID;
	bool m_rememberRawFormat = false;
	bool m_startedEdit = false;

	CComboBox m_ComboPitch;

public:
	CCtrlSamples(CModControlView &parent, CModDoc &document);
	~CCtrlSamples();

protected:
	bool IsOPLInstrument() const;
	
	bool SetCurrentSample(SAMPLEINDEX nSmp, LONG lZoom = -1, bool bUpdNum = true);
	bool InsertSample(bool duplicate, int8 *confirm = nullptr);
	bool OpenSample(const mpt::PathString &fileName, FlagSet<OpenSampleTypes> types = OpenSampleKnown | OpenSampleRaw);
	bool OpenSample(const CSoundFile &sndFile, SAMPLEINDEX nSample);
	void OpenSamples(const std::vector<mpt::PathString> &files, FlagSet<OpenSampleTypes> types);
	void SaveSample(bool doBatchSave);

	void Normalize(bool allSamples);
	void RemoveDCOffset(bool allSamples);

	void ApplyAmplify(const double amp, const double fadeInStart, const double fadeOutEnd, const bool fadeIn, const bool fadeOut, const Fade::Law fadeLaw);
	void ApplyResample(SAMPLEINDEX smp, uint32 newRate, ResamplingMode mode, bool ignoreSelection = false, bool updatePatternCommands = false, bool updatePatternNotes = false);

	SampleSelectionPoints GetSelectionPoints();
	void SetSelectionPoints(SmpLength nStart, SmpLength nEnd);

	void PropagateAutoVibratoChanges();
	void SetFinetune(int step);

public:
	//{{AFX_VIRTUAL(CCtrlSamples)
	Setting<LONG> &GetSplitPosRef() override;
	BOOL OnInitDialog() override;
	void DoDataExchange(CDataExchange* pDX) override;	// DDX/DDV support
	CRuntimeClass *GetAssociatedViewClass() override;
	void RecalcLayout() override;
	void OnActivatePage(LPARAM) override;
	void OnDeactivatePage() override;
	void UpdateView(UpdateHint hint, CObject *pObj = nullptr) override;
	LRESULT OnModCtrlMsg(WPARAM wParam, LPARAM lParam) override;
	CString GetToolTipText(UINT uId, HWND hwnd) const override;
	BOOL PreTranslateMessage(MSG* pMsg) override;
	void OnDPIChanged() override;
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
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM);
	afx_msg void OnXButtonUp(UINT nFlags, UINT nButton, CPoint point);

	afx_msg void OnPitchShiftTimeStretch();
	afx_msg void OnToggleTimestretchQuality();
	afx_msg void OnEstimateSampleSize();

	afx_msg void OnInitOPLInstrument();

	MPT_NOINLINE void SetModified(SAMPLEINDEX smp, SampleHint hint, bool updateAll, bool waveformModified);
	void SetModified(SampleHint hint, bool updateAll, bool waveformModified) { SetModified(m_nSample, hint, updateAll, waveformModified); }
	void PrepareUndo(const char *description, sampleUndoTypes type = sundo_none, SmpLength start = 0, SmpLength end = 0);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
