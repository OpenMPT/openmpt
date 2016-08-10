/*
 * MPDlgs.h
 * --------
 * Purpose: Implementation of various player setup dialogs.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

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

	CComboBox m_CbnStoppedMode;

	CStatic m_StaticChannelMapping[NUM_CHANNELCOMBOBOXES];
	CComboBox m_CbnChannelMapping[NUM_CHANNELCOMBOBOXES];

	SoundDevice::Identifier m_InitialDeviceIdentifier;

	void SetInitialDevice();

	void SetDevice(SoundDevice::Identifier dev, bool forceReload=false);
	SoundDevice::Info m_CurrentDeviceInfo;
	SoundDevice::Caps m_CurrentDeviceCaps;
	SoundDevice::DynamicCaps m_CurrentDeviceDynamicCaps;
	SoundDevice::Settings m_Settings;

public:
	COptionsSoundcard(SoundDevice::Identifier deviceIdentifier);

	void UpdateStatistics();

private:
	void UpdateEverything();
	void UpdateDevice();
	void UpdateGeneral();
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
	afx_msg void OnExclusiveModeChanged();
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

	bool m_initialized : 1;

public:
	COptionsMixer()
		: CPropertyPage(IDD_OPTIONS_MIXER)
		, m_initialized(false)
	{}

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


#ifndef NO_EQ

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
	void Init(UINT nID, UINT n, CWnd *parent);
	BOOL PreTranslateMessage(MSG *pMsg);
};

#endif // !NO_EQ


//========================================
class COptionsPlayer: public CPropertyPage
//========================================
{
protected:
	CComboBox m_CbnReverbPreset;
	CSliderCtrl m_SbXBassDepth, m_SbXBassRange;
	CSliderCtrl m_SbSurroundDepth, m_SbSurroundDelay;
	CSliderCtrl m_SbReverbDepth;

#ifndef NO_EQ
	CEQSlider m_Sliders[MAX_EQ_BANDS];
	EQPreset &m_EQPreset;
	UINT m_nSliderMenu;
#endif // !NO_EQ

public:
	COptionsPlayer() : CPropertyPage(IDD_OPTIONS_PLAYER)
#ifndef NO_EQ
		, m_EQPreset(TrackerSettings::Instance().m_EqSettings)
#endif
	{ }

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }

#ifndef NO_EQ
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnEqUser1()	{ LoadEQPreset(TrackerSettings::Instance().m_EqUserPresets[0]); };
	afx_msg void OnEqUser2()	{ LoadEQPreset(TrackerSettings::Instance().m_EqUserPresets[1]); };
	afx_msg void OnEqUser3()	{ LoadEQPreset(TrackerSettings::Instance().m_EqUserPresets[2]); };
	afx_msg void OnEqUser4()	{ LoadEQPreset(TrackerSettings::Instance().m_EqUserPresets[3]); };
	afx_msg void OnSavePreset();
	afx_msg void OnSliderMenu(UINT);
	afx_msg void OnSliderFreq(UINT);

	void UpdateDialog();
	void UpdateEQ(bool bReset);
	void LoadEQPreset(const EQPreset &preset);
#endif // !NO_EQ

	DECLARE_MESSAGE_MAP()
};


//=======================================
class CMidiSetupDlg: public CPropertyPage
//=======================================
{
public:
	DWORD m_dwMidiSetup;
	UINT m_nMidiDevice;

protected:
	CSpinButtonCtrl m_SpinSpd, m_SpinPat, m_SpinAmp;
	CComboBox m_ATBehaviour, m_Quantize;

public:
	CMidiSetupDlg(DWORD d, UINT n)
		: CPropertyPage(IDD_OPTIONS_MIDI)
		, m_dwMidiSetup(d)
		, m_nMidiDevice(n)
		{ }

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
