/*
 * mod2wave.h
 * ----------
 * Purpose: Module to WAV conversion (dialog + conversion code).
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "StreamEncoder.h"
#include "Settings.h"
#include "ProgressDialog.h"


OPENMPT_NAMESPACE_BEGIN


///////////////////////////////////////////////////////////////////////////////////////////////////
// Direct To Disk Recording


struct StoredTags
{
	Setting<mpt::ustring> artist;
	Setting<mpt::ustring> album;
	Setting<mpt::ustring> trackno;
	Setting<mpt::ustring> year;
	Setting<mpt::ustring> url;

	Setting<mpt::ustring> genre;

	StoredTags(SettingsContainer &conf);

};


struct CWaveConvertSettings
{
	std::vector<EncoderFactoryBase*> EncoderFactories;
	std::vector<MPT_SHARED_PTR<Encoder::Settings> > EncoderSettings;

	Setting<mpt::ustring> EncoderName;
	std::size_t EncoderIndex;

	SampleFormat FinalSampleFormat;

	StoredTags storedTags;
	FileTags Tags;

	int repeatCount;
	ORDERINDEX minOrder, maxOrder;
	SAMPLEINDEX sampleSlot;

	bool normalize : 1;
	bool silencePlugBuffers : 1;
	bool outputToSample : 1;

	std::size_t FindEncoder(const mpt::ustring &name) const;
	void SelectEncoder(std::size_t index);
	EncoderFactoryBase *GetEncoderFactory() const;
	const Encoder::Traits *GetTraits() const;
	Encoder::Settings &GetEncoderSettings() const;
	CWaveConvertSettings(SettingsContainer &conf, const std::vector<EncoderFactoryBase*> &encFactories);
};


//================================
class CWaveConvert: public CDialog
//================================
{
public:
	CWaveConvertSettings m_Settings;
	const Encoder::Traits *encTraits;
	CSoundFile &m_SndFile;
	uint64 m_dwFileLimit, m_dwSongLimit;
	ORDERINDEX m_nNumOrders;

	CComboBox m_CbnFileType, m_CbnSampleRate, m_CbnChannels, m_CbnDither, m_CbnSampleFormat, m_CbnSampleSlot;
	CSpinButtonCtrl m_SpinLoopCount, m_SpinMinOrder, m_SpinMaxOrder;

	bool m_bGivePlugsIdleTime;
	bool m_bChannelMode;		// Render by channel
	bool m_bInstrumentMode;		// Render by instrument

	CEdit m_EditTitle, m_EditAuthor, m_EditURL, m_EditAlbum, m_EditYear;
	CComboBox m_CbnGenre;
	CEdit m_EditGenre;

private:
	void OnShowEncoderInfo();
	void FillFileTypes();
	void FillSamplerates();
	void FillChannels();
	void FillFormats();
	void FillDither();
	void FillTags();

	void LoadTags();

	void SaveEncoderSettings();
	void SaveTags();

public:
	CWaveConvert(CWnd *parent, ORDERINDEX minOrder, ORDERINDEX maxOrder, ORDERINDEX numOrders, CSoundFile &sndFile, const std::vector<EncoderFactoryBase*> &encFactories);

public:
	void UpdateDialog();
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange *pDX);
	virtual void OnOK();
	afx_msg void OnCheck1();
	afx_msg void OnCheck2();
	afx_msg void OnCheckChannelMode();
	afx_msg void OnCheckInstrMode();
	afx_msg void OnFileTypeChanged();
	afx_msg void OnSamplerateChanged();
	afx_msg void OnChannelsChanged();
	afx_msg void OnDitherChanged();
	afx_msg void OnFormatChanged();
	afx_msg void OnPlayerOptions();
	afx_msg void OnExportModeChanged();
	afx_msg void OnSampleSlotChanged();
	DECLARE_MESSAGE_MAP()
};


//==========================================
class CDoWaveConvert: public CProgressDialog
//==========================================
{
public:
	const CWaveConvertSettings &m_Settings;
	CSoundFile &m_SndFile;
	const mpt::PathString &m_lpszFileName;
	uint64 m_dwFileLimit, m_dwSongLimit;
	bool m_bGivePlugsIdleTime;

public:
	CDoWaveConvert(CSoundFile &sndFile, const mpt::PathString &filename, const CWaveConvertSettings &settings, CWnd *parent = NULL)
		: CProgressDialog(parent)
		, m_SndFile(sndFile)
		, m_Settings(settings)
		, m_lpszFileName(filename)
		, m_dwFileLimit(0), m_dwSongLimit(0)
	{ }
	void Run();
};


OPENMPT_NAMESPACE_END
