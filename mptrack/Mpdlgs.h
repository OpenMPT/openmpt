/*
 * MPDlgs.h
 * --------
 * Purpose: Implementation of various player setup dialogs.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

class CSoundFile;
class CMainFrame;

#define NUM_CHANNELCOMBOBOXES	4

//===========================================
class COptionsSoundcard: public CPropertyPage
//===========================================
{
protected:
	CComboBoxEx m_CbnDevice;
	CComboBox m_CbnLatencyMS, m_CbnUpdateIntervalMS, m_CbnMixingFreq, m_CbnChannels, m_CbnSampleFormat, m_CbnDither;
	CEdit m_EditStatistics;
	CButton m_BtnDriverPanel;

	CStatic m_StaticChannelMapping[NUM_CHANNELCOMBOBOXES];
	CComboBox m_CbnChannelMapping[NUM_CHANNELCOMBOBOXES];

	void SetDevice(SoundDeviceID dev, bool forceReload=false);
	SoundDeviceInfo m_CurrentDeviceInfo;
	SoundDeviceCaps m_CurrentDeviceCaps;
	SoundDeviceSettings m_Settings;

public:
	COptionsSoundcard(SoundDeviceID sd);

	void UpdateStatistics();

private:
	void UpdateEverything();
	void UpdateDevice();
	void UpdateLatency();
	void UpdateUpdateInterval();
	void UpdateSampleRates();
	void UpdateChannels();
	void UpdateSampleFormat();
	void UpdateDither();
	void UpdateChannelMapping();
	void UpdateControls();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);
	void UpdateStereoSep();

	afx_msg void OnDeviceChanged();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnChannelsChanged();
	afx_msg void OnSampleFormatChanged();
	afx_msg void OnSoundCardRescan();
	afx_msg void OnSoundCardDriverPanel();

	void OnChannelChanged(int channel);
	afx_msg void OnChannel1Changed() { OnChannelChanged(0); };
	afx_msg void OnChannel2Changed() { OnChannelChanged(1); };
	afx_msg void OnChannel3Changed() { OnChannelChanged(2); };
	afx_msg void OnChannel4Changed() { OnChannelChanged(3); };

	DECLARE_MESSAGE_MAP()
};


//=======================================
class COptionsMixer: public CPropertyPage
//=======================================
{
protected:

	CComboBox m_CbnResampling;
	CEdit m_CEditWFIRCutoff;
	CComboBox m_CbnWFIRType;

	CEdit m_CEditRampUp;
	CEdit m_CEditRampDown;
	CEdit m_CInfoRampUp;
	CEdit m_CInfoRampDown;

	CComboBox m_CbnPolyphony;

	CSliderCtrl m_SliderStereoSep;

	// check box soft pan

	CSliderCtrl m_SliderPreAmp;

public:
	COptionsMixer():CPropertyPage(IDD_OPTIONS_MIXER) {}

protected:
	void UpdateRamping();
	void UpdateStereoSep();

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnResamplerChanged();
	afx_msg void OnRampingChanged();

	afx_msg void OnHScroll(UINT n, UINT pos, CScrollBar *p) { CPropertyPage::OnHScroll(n, pos, p); OnScroll(n, pos, p); }
	afx_msg void OnVScroll(UINT n, UINT pos, CScrollBar *p) { CPropertyPage::OnVScroll(n, pos, p); OnScroll(n, pos, p); }
	void OnScroll(UINT n, UINT pos, CScrollBar *p);

	DECLARE_MESSAGE_MAP()

};


//========================================
class COptionsPlayer: public CPropertyPage
//========================================
{
protected:
	CComboBox m_CbnReverbPreset;
	CSliderCtrl m_SbXBassDepth, m_SbXBassRange;
	CSliderCtrl m_SbSurroundDepth, m_SbSurroundDelay;
	CSliderCtrl m_SbReverbDepth;

public:
	COptionsPlayer():CPropertyPage(IDD_OPTIONS_PLAYER) {}

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }

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
