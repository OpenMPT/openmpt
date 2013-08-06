/*
 * MPDlgs.h
 * --------
 * Purpose: Implementation of various player setup dialogs.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#define NUMMIXRATE	16

class CSoundFile;
class CMainFrame;

//===========================================
class COptionsSoundcard: public CPropertyPage
//===========================================
{
protected:
	CComboBoxEx m_CbnDevice;
	CComboBox m_CbnLatencyMS, m_CbnUpdateIntervalMS, m_CbnMixingFreq, m_CbnPolyphony, m_CbnQuality;
	CSliderCtrl m_SliderStereoSep, m_SliderPreAmp;
	CEdit m_EditStatistics;
	DWORD m_dwRate, m_SoundDeviceFlags;
	SampleFormat m_SampleFormat;
	DWORD m_nChannels;
	DWORD m_LatencyMS;
	DWORD m_UpdateIntervalMS;
	DWORD m_nSoundDevice;
	bool m_PreAmpNoteShowed;

public:
	COptionsSoundcard(DWORD rate, DWORD flags, SampleFormat sampleformat, DWORD chns, DWORD latency_ms, DWORD updateinterval_ms, DWORD sd):CPropertyPage(IDD_OPTIONS_SOUNDCARD)
		{ m_dwRate = rate; m_SoundDeviceFlags = flags; m_SampleFormat = sampleformat; m_nChannels = chns;
		  m_LatencyMS = latency_ms; m_UpdateIntervalMS = updateinterval_ms; m_nSoundDevice = sd; m_PreAmpNoteShowed = false; }

	void UpdateStatistics();

private:
	void UpdateSampleRates(int dev);
	void UpdateChannelsFormat(int dev);
	void UpdateControls(int dev);
	void SetPreAmpSliderPosition();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);
	void UpdateStereoSep();
	afx_msg void OnDeviceChanged();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	afx_msg void OnVScroll(UINT, UINT, CScrollBar *);
	DECLARE_MESSAGE_MAP()
};


//========================================
class COptionsPlayer: public CPropertyPage
//========================================
{
protected:
	CComboBox m_CbnResampling, m_CbnReverbPreset, m_CbnWFIRType; //rewbs.resamplerConf: added m_CbnWFIRType
	CSliderCtrl m_SbXBassDepth, m_SbXBassRange;
	CSliderCtrl m_SbSurroundDepth, m_SbSurroundDelay;
	CSliderCtrl m_SbReverbDepth;
	CEdit m_CEditWFIRCutoff;
	CEdit m_CEditRampUp, m_CEditRampDown;

public:
	COptionsPlayer():CPropertyPage(IDD_OPTIONS_PLAYER) {}

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	//rewbs.resamplerConf
	afx_msg void OnWFIRTypeChanged();
	afx_msg void OnResamplerChanged();
	afx_msg void OnDefaultResampling();
	//end rewbs.resamplerConf

	DECLARE_MESSAGE_MAP()
};



//=================================
class CEQSlider: public CSliderCtrl
//=================================
{
public:
	CWnd *m_pParent;
	UINT m_nSliderNo;
	short int m_x, m_y;
public:
	CEQSlider() {}
	VOID Init(UINT nID, UINT n, CWnd *parent);
	BOOL PreTranslateMessage(MSG *pMsg);
};


//=====================================
class CEQSetupDlg: public CPropertyPage
//=====================================
{
protected:
	CEQSlider m_Sliders[MAX_EQ_BANDS];
	EQPreset *m_pEqPreset;
	UINT m_nSliderMenu;

public:
	CEQSetupDlg(EQPreset *pEq):CPropertyPage(IDD_SETUP_EQ) { m_pEqPreset = pEq; }
	void UpdateDialog();
	void UpdateEQ(BOOL bReset);

public:
	static const EQPreset gEQPresets[];
	static EQPreset gUserPresets[];

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	enum
	{
		EQPRESET_FLAT = 0,
		EQPRESET_JAZZ,
		EQPRESET_POP,
		EQPRESET_ROCK,
		EQPRESET_CONCERT,
		EQPRESET_CLEAR,
	};

	afx_msg void OnEqFlat()		{ LoadEQPreset(gEQPresets[EQPRESET_FLAT]); };
	afx_msg void OnEqJazz()		{ LoadEQPreset(gEQPresets[EQPRESET_JAZZ]); };
	afx_msg void OnEqPop()		{ LoadEQPreset(gEQPresets[EQPRESET_POP]); };
	afx_msg void OnEqRock()		{ LoadEQPreset(gEQPresets[EQPRESET_ROCK]); };
	afx_msg void OnEqConcert()	{ LoadEQPreset(gEQPresets[EQPRESET_CONCERT]); };
	afx_msg void OnEqClear()	{ LoadEQPreset(gEQPresets[EQPRESET_CLEAR]); };

	afx_msg void OnEqUser1()	{ LoadEQPreset(gUserPresets[0]); };
	afx_msg void OnEqUser2()	{ LoadEQPreset(gUserPresets[1]); };
	afx_msg void OnEqUser3()	{ LoadEQPreset(gUserPresets[2]); };
	afx_msg void OnEqUser4()	{ LoadEQPreset(gUserPresets[3]); };

	afx_msg void OnSavePreset();
	afx_msg void OnSliderMenu(UINT);
	afx_msg void OnSliderFreq(UINT);
	DECLARE_MESSAGE_MAP()

	void LoadEQPreset(const EQPreset &preset);
};


//=======================================
class CMidiSetupDlg: public CPropertyPage
//=======================================
{
public:
	DWORD m_dwMidiSetup;
	LONG m_nMidiDevice;

protected:
	CSpinButtonCtrl m_SpinSpd, m_SpinPat, m_SpinAmp;
	CComboBox m_ATBehaviour;

public:
	CMidiSetupDlg(DWORD d, LONG n):CPropertyPage(IDD_OPTIONS_MIDI)
		{ m_dwMidiSetup = d; m_nMidiDevice = n; }

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	DECLARE_MESSAGE_MAP()
};
