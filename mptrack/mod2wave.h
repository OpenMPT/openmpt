/*
 * mod2wave.h
 * ----------
 * Purpose: Module to steaming audio (WAV, MP3, etc.) conversion (dialog + conversion code).
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "DialogBase.h"
#include "ProgressDialog.h"
#include "Settings.h"
#include "openmpt/streamencoder/StreamEncoder.hpp"
#include "StreamEncoderSettings.h"
#include "../soundlib/Snd_defs.h"


OPENMPT_NAMESPACE_BEGIN

class CSoundFile;
struct SubSong;

struct CWaveConvertSettings
{
	std::vector<EncoderFactoryBase*> EncoderFactories;
	std::vector<std::unique_ptr<EncoderSettingsConf>> EncoderSettings;

	Setting<mpt::ustring> EncoderName;
	std::size_t EncoderIndex;

	StoredTags storedTags;
	FileTags Tags;

	int repeatCount = 1;
	ORDERINDEX minOrder = ORDERINDEX_INVALID, maxOrder = ORDERINDEX_INVALID;
	SAMPLEINDEX sampleSlot = 0;

	bool normalize = false;
	bool silencePlugBuffers = false;
	bool outputToSample = false;

	std::size_t FindEncoder(const mpt::ustring &name) const;
	void SelectEncoder(std::size_t index);
	EncoderFactoryBase *GetEncoderFactory() const;
	const Encoder::Traits *GetTraits() const;
	EncoderSettingsConf &GetEncoderSettings() const;
	Encoder::Settings GetEncoderSettingsWithDetails() const;
	CWaveConvertSettings(SettingsContainer &conf, const std::vector<EncoderFactoryBase*> &encFactories);
};


class CWaveConvert : public DialogBase
{
public:
	CWaveConvertSettings m_Settings;
	CSoundFile &m_SndFile;
	uint64 m_dwSongLimit = 0;
	std::vector<SubSong> m_subSongs;

	bool m_bGivePlugsIdleTime = false;
	bool m_bChannelMode = false;     // Render by channel
	bool m_bInstrumentMode = false;  // Render by instrument

private:
	const Encoder::Traits *encTraits = nullptr;

	CComboBox m_CbnFileType, m_CbnSampleRate, m_CbnChannels, m_CbnDither, m_CbnSampleFormat, m_CbnSampleSlot;
	CSpinButtonCtrl m_SpinLoopCount, m_SpinMinOrder, m_SpinMaxOrder, m_SpinSubsongIndex;

	CEdit m_EditTitle, m_EditAuthor, m_EditURL, m_EditAlbum, m_EditYear;
	CComboBox m_CbnGenre;
	CEdit m_EditGenre;
	size_t m_selectedSong = 0;
	const ORDERINDEX m_nNumOrders;

public:
	CWaveConvert(CWnd *parent, ORDERINDEX minOrder, ORDERINDEX maxOrder, ORDERINDEX numOrders, CSoundFile &sndFile, const std::vector<EncoderFactoryBase *> &encFactories);
	~CWaveConvert();

private:
	void FillFileTypes();
	void FillSamplerates();
	void FillChannels();
	void FillFormats();
	void FillDither();
	void FillTags();

	void LoadTags();

	void SaveEncoderSettings();
	void SaveTags();

	void UpdateSubsongName();
	void UpdateDialog();

	BOOL OnInitDialog() override;
	void DoDataExchange(CDataExchange *pDX) override;
	void OnOK() override;

	afx_msg void OnCheckTimeLimit();
	afx_msg void OnCheckChannelMode();
	afx_msg void OnCheckInstrMode();
	afx_msg void OnFileTypeChanged();
	afx_msg void OnSamplerateChanged();
	afx_msg void OnChannelsChanged();
	afx_msg void OnDitherChanged();
	afx_msg void OnFormatChanged();
	afx_msg void OnSubsongChanged();
	afx_msg void OnPlayerOptions();
	afx_msg void OnExportModeChanged();
	afx_msg void OnSampleSlotChanged();

	DECLARE_MESSAGE_MAP()
};


class CDoWaveConvert: public CProgressDialog
{
public:
	uint64 m_dwSongLimit = 0;
	bool m_bGivePlugsIdleTime = false;

public:
	CDoWaveConvert(CSoundFile &sndFile, std::ostream &f, const CString &caption, const CWaveConvertSettings &settings, const SubSong &subSong, CWnd *parent = nullptr)
		: CProgressDialog(parent)
		, m_Settings(settings)
		, m_SndFile(sndFile)
		, fileStream(f)
		, caption(caption)
		, m_subSong(subSong)
	{ }

	void Run() override;

private:
	const CWaveConvertSettings &m_Settings;
	CSoundFile &m_SndFile;
	std::ostream &fileStream;
	const CString &caption;
	const SubSong &m_subSong;
};


OPENMPT_NAMESPACE_END
