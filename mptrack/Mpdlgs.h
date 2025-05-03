/*
 * MPDlgs.h
 * --------
 * Purpose: Implementation of various player setup dialogs.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "AccessibleControls.h"
#include "TrackerSettings.h"
#include "../sounddsp/EQ.h"
#include "openmpt/sounddevice/SoundDevice.hpp"

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;
class CMainFrame;

#define NUM_CHANNELCOMBOBOXES	4

class COptionsSoundcard: public CPropertyPage
{
protected:
	CComboBoxEx m_CbnDevice;
	CComboBox m_CbnLatencyMS, m_CbnUpdateIntervalMS, m_CbnMixingFreq, m_CbnChannels, m_CbnSampleFormat, m_CbnDither, m_CbnRecordingChannels, m_CbnRecordingSource;
	CEdit m_EditStatistics;
	CButton m_BtnDriverPanel;

	CComboBox m_CbnStoppedMode;

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
	void UpdateRecording();
	void UpdateControls();

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	BOOL OnSetActive() override;
	void DoDataExchange(CDataExchange* pDX) override;
	void UpdateStereoSep();

	afx_msg void OnDeviceChanged();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnExclusiveModeChanged();
	afx_msg void OnChannelsChanged();
	afx_msg void OnSampleFormatChanged();
	afx_msg void OnRecordingChanged();
	afx_msg void OnSoundCardShowAll();
	afx_msg void OnSoundCardRescan();
	afx_msg void OnSoundCardDriverPanel();

	void OnChannelChanged(int channel);
	afx_msg void OnChannel1Changed() { OnChannelChanged(0); };
	afx_msg void OnChannel2Changed() { OnChannelChanged(1); };
	afx_msg void OnChannel3Changed() { OnChannelChanged(2); };
	afx_msg void OnChannel4Changed() { OnChannelChanged(3); };

	DECLARE_MESSAGE_MAP()
};


class COptionsMixer: public CPropertyPage
{
protected:

	CComboBox m_CbnResampling, m_CbnAmigaType;

	AccessibleEdit m_CEditRampUp;
	AccessibleEdit m_CEditRampDown;
	CEdit m_CInfoRampUp;
	CEdit m_CInfoRampDown;

	CSliderCtrl m_SliderStereoSep;

	// check box soft pan

	CSliderCtrl m_SliderPreAmp;

	bool m_initialized = false;

public:
	COptionsMixer();

protected:
	void UpdateRamping();
	void UpdateStereoSep();

	BOOL OnInitDialog() override;
	void OnOK() override;
	BOOL OnSetActive() override;
	void DoDataExchange(CDataExchange* pDX) override;

	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnAmigaChanged();
	afx_msg void OnRampingChanged();
	afx_msg void OnDefaultRampSettings();

	afx_msg void OnHScroll(UINT n, UINT pos, CScrollBar *p);

	DECLARE_MESSAGE_MAP()

};


#ifndef NO_EQ

class CEQSlider: public CSliderCtrl
{
public:
	CWnd *m_pParent;
	UINT m_nSliderNo;
	short int m_x, m_y;
public:
	CEQSlider() = default;
	void Init(UINT nID, UINT n, CWnd *parent);
	BOOL PreTranslateMessage(MSG *pMsg);
};

#endif // !NO_EQ


class COptionsPlayer: public CPropertyPage
{
protected:
	CComboBox m_CbnReverbPreset;
	CSliderCtrl m_SbXBassDepth, m_SbXBassRange;
	CSliderCtrl m_SbSurroundDepth, m_SbSurroundDelay;
	CSliderCtrl m_SbReverbDepth;
	CSliderCtrl m_SbBitCrushBits;

#ifndef NO_EQ
	CEQSlider m_Sliders[MAX_EQ_BANDS];
	EQPreset &m_EQPreset;
	UINT m_nSliderMenu;
#endif // !NO_EQ

public:
	COptionsPlayer();

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	BOOL OnSetActive() override;
	void DoDataExchange(CDataExchange* pDX) override;
	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }

#ifndef NO_EQ
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnEqUser1() { LoadEQPreset(TrackerSettings::Instance().m_EqUserPresets[0]); };
	afx_msg void OnEqUser2() { LoadEQPreset(TrackerSettings::Instance().m_EqUserPresets[1]); };
	afx_msg void OnEqUser3() { LoadEQPreset(TrackerSettings::Instance().m_EqUserPresets[2]); };
	afx_msg void OnEqUser4() { LoadEQPreset(TrackerSettings::Instance().m_EqUserPresets[3]); };
	afx_msg void OnSavePreset();
	afx_msg void OnSliderMenu(UINT);
	afx_msg void OnSliderFreq(UINT);

	void UpdateDialog();
	void UpdateEQ(bool bReset);
	void LoadEQPreset(const EQPreset &preset);
#endif // !NO_EQ

	DECLARE_MESSAGE_MAP()
};


class CMidiSetupDlg: public CPropertyPage
{
public:
	FlagSet<MidiSetup> m_midiSetup;
	UINT m_nMidiDevice;

public:
	CMidiSetupDlg(FlagSet<MidiSetup> flags, UINT device);

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	BOOL OnSetActive() override;
	void DoDataExchange(CDataExchange* pDX) override;
	void RefreshDeviceList(UINT currentDevice);
	afx_msg void OnRenameDevice();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	DECLARE_MESSAGE_MAP()

protected:
	CSpinButtonCtrl m_SpinSpd, m_SpinPat, m_SpinAmp;
	CComboBox m_InputDevice, m_ATBehaviour, m_Quantize, m_ContinueMode, m_RecordPitchBend;
	AccessibleEdit m_editAmp;
};



class COptionsWine: public CPropertyPage
{

protected:
	CComboBox m_CbnPulseAudio;
	CComboBox m_CbnPortAudio;
	CComboBox m_CbnRtAudio;

public:
	COptionsWine();

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	BOOL OnSetActive() override;
	void DoDataExchange(CDataExchange* pDX) override;

	afx_msg void OnSettingsChanged();
	
	DECLARE_MESSAGE_MAP()
};



OPENMPT_NAMESPACE_END
