/*
 * mod2wave.cpp
 * ------------
 * Purpose: Module to WAV conversion (dialog + conversion code).
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Sndfile.h"
#include "Dlsbank.h"
#include "Mainfrm.h"
#include "Mpdlgs.h"
#include "mod2wave.h"
#include "WAVTools.h"
#include "../common/mptFileTemporary.h"
#include "../common/mptString.h"
#include "../common/version.h"
#include "../soundlib/MixerLoops.h"
#include "openmpt/soundbase/Dither.hpp"
#include "../common/Dither.h"
#include "../soundlib/AudioReadTarget.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "mpt/io_file/fstream.hpp"
#include "../common/mptFileIO.h"
#include "mpt/audio/span.hpp"
#include <variant>
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"


OPENMPT_NAMESPACE_BEGIN


extern const TCHAR *gszChnCfgNames[3];


template <typename Tsample>
static CSoundFile::samplecount_t ReadInterleaved(CSoundFile &sndFile, Tsample *outputBuffer, std::size_t channels, CSoundFile::samplecount_t count, DithersOpenMPT &dithers)
{
	sndFile.ResetMixStat();
	MPT_ASSERT(sndFile.m_MixerSettings.gnChannels == channels);
	AudioTargetBuffer<mpt::audio_span_interleaved<Tsample>, DithersOpenMPT> target(mpt::audio_span_interleaved<Tsample>(outputBuffer, channels, count), dithers);
	return sndFile.Read(count, target);
}


///////////////////////////////////////////////////
// CWaveConvert - setup for converting a wave file

BEGIN_MESSAGE_MAP(CWaveConvert, CDialog)
	ON_COMMAND(IDC_CHECK2,			&CWaveConvert::OnCheckTimeLimit)
	ON_COMMAND(IDC_CHECK4,			&CWaveConvert::OnCheckChannelMode)
	ON_COMMAND(IDC_CHECK6,			&CWaveConvert::OnCheckInstrMode)
	ON_COMMAND(IDC_RADIO1,			&CWaveConvert::UpdateDialog)
	ON_COMMAND(IDC_RADIO2,			&CWaveConvert::UpdateDialog)
	ON_COMMAND(IDC_RADIO3,			&CWaveConvert::UpdateDialog)
	ON_COMMAND(IDC_RADIO4,			&CWaveConvert::OnExportModeChanged)
	ON_COMMAND(IDC_RADIO5,			&CWaveConvert::OnExportModeChanged)
	ON_COMMAND(IDC_PLAYEROPTIONS,	&CWaveConvert::OnPlayerOptions)
	ON_CBN_SELCHANGE(IDC_COMBO5,	&CWaveConvert::OnFileTypeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1,	&CWaveConvert::OnSamplerateChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,	&CWaveConvert::OnChannelsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO6,	&CWaveConvert::OnDitherChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	&CWaveConvert::OnFormatChanged)
	ON_CBN_SELCHANGE(IDC_COMBO9,	&CWaveConvert::OnSampleSlotChanged)
END_MESSAGE_MAP()


CWaveConvert::CWaveConvert(CWnd *parent, ORDERINDEX minOrder, ORDERINDEX maxOrder, ORDERINDEX numOrders, CSoundFile &sndFile, const std::vector<EncoderFactoryBase*> &encFactories)
	: CDialog(IDD_WAVECONVERT, parent)
	, m_Settings(theApp.GetSettings(), encFactories)
	, m_SndFile(sndFile)
{
	ASSERT(!encFactories.empty());
	encTraits = m_Settings.GetTraits();
	m_bGivePlugsIdleTime = false;
	if(minOrder != ORDERINDEX_INVALID && maxOrder != ORDERINDEX_INVALID)
	{
		// render selection
		m_Settings.minOrder = minOrder;
		m_Settings.maxOrder = maxOrder;
	}
	m_Settings.repeatCount = 1;
	m_Settings.minSequence = m_Settings.maxSequence = m_SndFile.Order.GetCurrentSequenceIndex();
	m_nNumOrders = numOrders;

	m_dwSongLimit = 0;
}


void CWaveConvert::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO5,	m_CbnFileType);
	DDX_Control(pDX, IDC_COMBO1,	m_CbnSampleRate);
	DDX_Control(pDX, IDC_COMBO4,	m_CbnChannels);
	DDX_Control(pDX, IDC_COMBO6,	m_CbnDither);
	DDX_Control(pDX, IDC_COMBO2,	m_CbnSampleFormat);
	DDX_Control(pDX, IDC_SPIN3,		m_SpinMinOrder);
	DDX_Control(pDX, IDC_SPIN4,		m_SpinMaxOrder);
	DDX_Control(pDX, IDC_SPIN5,		m_SpinLoopCount);
	DDX_Control(pDX, IDC_SPIN6,		m_SpinMinSequence);
	DDX_Control(pDX, IDC_SPIN7,		m_SpinMaxSequence);
	DDX_Control(pDX, IDC_COMBO9,	m_CbnSampleSlot);

	DDX_Control(pDX, IDC_COMBO3,	m_CbnGenre);
	DDX_Control(pDX, IDC_EDIT10,	m_EditGenre);
	DDX_Control(pDX, IDC_EDIT11,	m_EditTitle);
	DDX_Control(pDX, IDC_EDIT6,		m_EditAuthor);
	DDX_Control(pDX, IDC_EDIT7,		m_EditAlbum);
	DDX_Control(pDX, IDC_EDIT8,		m_EditURL);
	DDX_Control(pDX, IDC_EDIT9,		m_EditYear);
}


BOOL CWaveConvert::OnInitDialog()
{
	CDialog::OnInitDialog();

	CheckDlgButton(IDC_CHECK5, BST_UNCHECKED);	// Normalize
	CheckDlgButton(IDC_CHECK3, BST_CHECKED);	// Cue points

	CheckDlgButton(IDC_CHECK4, BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK6, BST_UNCHECKED);

	const bool selection = (m_Settings.minOrder != ORDERINDEX_INVALID && m_Settings.maxOrder != ORDERINDEX_INVALID);
	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, selection ? IDC_RADIO2 : IDC_RADIO1);
	if(selection)
	{
		SetDlgItemInt(IDC_EDIT3, m_Settings.minOrder);
		SetDlgItemInt(IDC_EDIT4, m_Settings.maxOrder);
	}
	m_SpinMinOrder.SetRange32(0, m_nNumOrders);
	m_SpinMaxOrder.SetRange32(0, m_nNumOrders);

	const SEQUENCEINDEX numSequences = m_SndFile.Order.GetNumSequences();
	const BOOL enableSeq = numSequences > 1 ? TRUE : FALSE;
	GetDlgItem(IDC_RADIO3)->EnableWindow(enableSeq);
	m_SpinMinSequence.SetRange32(1, numSequences);
	m_SpinMaxSequence.SetRange32(1, numSequences);
	SetDlgItemInt(IDC_EDIT12, m_Settings.minSequence + 1);
	SetDlgItemInt(IDC_EDIT13, m_Settings.maxSequence + 1);

	SetDlgItemInt(IDC_EDIT5, m_Settings.repeatCount, FALSE);
	m_SpinLoopCount.SetRange32(1, int16_max);

	FillFileTypes();
	FillSamplerates();
	FillChannels();
	FillFormats();
	FillDither();

	LoadTags();

	m_EditYear.SetLimitText(4);

	m_EditTitle.SetWindowText(mpt::ToCString(m_Settings.Tags.title));
	m_EditAuthor.SetWindowText(mpt::ToCString(m_Settings.Tags.artist));
	m_EditURL.SetWindowText(mpt::ToCString(m_Settings.Tags.url));
	m_EditAlbum.SetWindowText(mpt::ToCString(m_Settings.Tags.album));
	m_EditYear.SetWindowText(mpt::ToCString(m_Settings.Tags.year));
	m_EditGenre.SetWindowText(mpt::ToCString(m_Settings.Tags.genre));

	FillTags();

	// Plugin quirk options are only available if there are any plugins loaded.
	GetDlgItem(IDC_GIVEPLUGSIDLETIME)->EnableWindow(FALSE);
	GetDlgItem(IDC_RENDERSILENCE)->EnableWindow(FALSE);
#ifndef NO_PLUGINS
	for(const auto &plug : m_SndFile.m_MixPlugins)
	{
		if(plug.pMixPlugin != nullptr)
		{
			GetDlgItem(IDC_GIVEPLUGSIDLETIME)->EnableWindow(TRUE);
			GetDlgItem(IDC_RENDERSILENCE)->EnableWindow(TRUE);
			break;
		}
	}
#endif // NO_PLUGINS

	// Fill list of sample slots to render into
	if(m_SndFile.GetNextFreeSample() != SAMPLEINDEX_INVALID)
	{
		m_CbnSampleSlot.SetItemData(m_CbnSampleSlot.AddString(_T("<empty slot>")), 0);
	}
	CString s;
	for(SAMPLEINDEX smp = 1; smp <= m_SndFile.GetNumSamples(); smp++)
	{
		s.Format(_T("%02u: %s%s"), smp, m_SndFile.GetSample(smp).HasSampleData() ? _T("*") : _T(""), mpt::ToCString(m_SndFile.GetCharsetInternal(), m_SndFile.GetSampleName(smp)).GetString());
		m_CbnSampleSlot.SetItemData(m_CbnSampleSlot.AddString(s), smp);
	}
	if(m_Settings.sampleSlot > m_SndFile.GetNumSamples()) m_Settings.sampleSlot = 0;
	m_CbnSampleSlot.SetCurSel(m_Settings.sampleSlot);

	CheckRadioButton(IDC_RADIO4, IDC_RADIO5, m_Settings.outputToSample ? IDC_RADIO5 : IDC_RADIO4);

	UpdateDialog();
	return TRUE;
}


void CWaveConvert::LoadTags()
{
	m_Settings.Tags.title = mpt::ToUnicode(mpt::Charset::Locale, m_SndFile.GetTitle());
	m_Settings.Tags.comments = mpt::ToUnicode(mpt::Charset::Locale, m_SndFile.m_songMessage.GetFormatted(SongMessage::leLF));
	m_Settings.Tags.artist = m_SndFile.m_songArtist;
	m_Settings.Tags.album = m_Settings.storedTags.album;
	m_Settings.Tags.trackno = m_Settings.storedTags.trackno;
	m_Settings.Tags.year = m_Settings.storedTags.year;
	m_Settings.Tags.url = m_Settings.storedTags.url;
	m_Settings.Tags.genre = m_Settings.storedTags.genre;
}


void CWaveConvert::SaveTags()
{
	m_Settings.storedTags.artist = m_Settings.Tags.artist;
	m_Settings.storedTags.album = m_Settings.Tags.album;
	m_Settings.storedTags.trackno = m_Settings.Tags.trackno;
	m_Settings.storedTags.year = m_Settings.Tags.year;
	m_Settings.storedTags.url = m_Settings.Tags.url;
	m_Settings.storedTags.genre = m_Settings.Tags.genre;
}


void CWaveConvert::FillTags()
{
	EncoderSettingsConf &encSettings = m_Settings.GetEncoderSettings();

	DWORD_PTR dwFormat = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());
	Encoder::Mode mode = (Encoder::Mode)((dwFormat >> 24) & 0xff);

	CheckDlgButton(IDC_CHECK3, encTraits->canCues?encSettings.Cues?TRUE:FALSE:FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK3), encTraits->canCues?TRUE:FALSE);

	const BOOL canTags = encTraits->canTags ? TRUE : FALSE;
	CheckDlgButton(IDC_CHECK7, encSettings.Tags ? canTags : FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK7), canTags);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_COMBO3), canTags);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT11), canTags);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT6), canTags);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT7), canTags);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT8), canTags);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT9), canTags);
	m_CbnGenre.EnableWindow(canTags?TRUE:FALSE);
	m_EditGenre.EnableWindow(canTags?TRUE:FALSE);

	if((encTraits->modesWithFixedGenres & mode) && !encTraits->genres.empty())
	{
		m_EditGenre.ShowWindow(SW_HIDE);
		m_CbnGenre.ShowWindow(SW_SHOW);
		m_EditGenre.Clear();
		m_CbnGenre.ResetContent();
		m_CbnGenre.AddString(_T(""));
		for(const auto &genre : encTraits->genres)
		{
			m_CbnGenre.AddString(mpt::ToCString(genre));
		}
	} else
	{
		m_CbnGenre.ShowWindow(SW_HIDE);
		m_EditGenre.ShowWindow(SW_SHOW);
		m_CbnGenre.ResetContent();
		m_EditGenre.Clear();
	}

}


void CWaveConvert::FillFileTypes()
{
	m_CbnFileType.ResetContent();
	int sel = 0;
	for(std::size_t i = 0; i < m_Settings.EncoderFactories.size(); ++i)
	{
		int ndx = m_CbnFileType.AddString(MPT_CFORMAT("{} ({})")(mpt::ToCString(m_Settings.EncoderFactories[i]->GetTraits().fileShortDescription), mpt::ToCString(m_Settings.EncoderFactories[i]->GetTraits().fileDescription)));
		m_CbnFileType.SetItemData(ndx, i);
		if(m_Settings.EncoderIndex == i)
		{
			sel = ndx;
		}
	}
	m_CbnFileType.SetCurSel(sel);
}


void CWaveConvert::FillSamplerates()
{
	EncoderSettingsConf &encSettings = m_Settings.GetEncoderSettings();
	m_CbnSampleRate.CComboBox::ResetContent();
	int sel = -1;
	std::vector<uint32> samplerates = (!encTraits->samplerates.empty() ? encTraits->samplerates : TrackerSettings::Instance().GetSampleRates());
	if(TrackerSettings::Instance().ExportDefaultToSoundcardSamplerate)
	{
		for(auto samplerate : samplerates)
		{
			if(samplerate == TrackerSettings::Instance().MixerSamplerate)
			{
				encSettings.Samplerate = samplerate;
			}
		}
	}
	for(auto samplerate : samplerates)
	{
		int ndx = m_CbnSampleRate.AddString(MPT_CFORMAT("{} Hz")(samplerate));
		m_CbnSampleRate.SetItemData(ndx, samplerate);
		if(samplerate == encSettings.Samplerate)
		{
			sel = ndx;
		}
	}
	if(sel == -1)
	{
		sel = 0;
	}
	m_CbnSampleRate.SetCurSel(sel);
}


void CWaveConvert::FillChannels()
{
	EncoderSettingsConf &encSettings = m_Settings.GetEncoderSettings();
	m_CbnChannels.CComboBox::ResetContent();
	int sel = 0;
	for(int channels = 4; channels >= 1; channels /= 2)
	{
		if(channels > encTraits->maxChannels)
		{
			continue;
		}
		if(IsDlgButtonChecked(IDC_RADIO5) != BST_UNCHECKED)
		{
			if(channels > 2)
			{
				// sample export only supports 2 channels max
				continue;
			}
		}
		int ndx = m_CbnChannels.AddString(gszChnCfgNames[(channels+2)/2-1]);
		m_CbnChannels.SetItemData(ndx, channels);
		if(channels == encSettings.Channels)
		{
			sel = ndx;
		}
	}
	m_CbnChannels.SetCurSel(sel);
}


void CWaveConvert::FillFormats()
{
	EncoderSettingsConf &encSettings = m_Settings.GetEncoderSettings();
	m_CbnSampleFormat.CComboBox::ResetContent();
	int sel = -1;
	int samplerate = static_cast<int>(m_CbnSampleRate.GetItemData(m_CbnSampleRate.GetCurSel()));
	int channels = static_cast<int>(m_CbnChannels.GetItemData(m_CbnChannels.GetCurSel()));
	if(encTraits->modes & Encoder::ModeQuality)
	{
		for(int quality = 100; quality >= 0; quality -= 10)
		{
			int ndx = m_CbnSampleFormat.AddString(mpt::ToCString(m_Settings.GetEncoderFactory()->DescribeQuality(quality * 0.01f)));
			m_CbnSampleFormat.SetItemData(ndx, (Encoder::ModeQuality<<24) | (quality<<0));
			if(encSettings.Mode == Encoder::ModeQuality && mpt::saturate_round<int>(encSettings.Quality*100.0f) == quality)
			{
				sel = ndx;
			}
		}
	}
	if(encTraits->modes & Encoder::ModeVBR)
	{
		for(int bitrate = static_cast<int>(encTraits->bitrates.size()-1); bitrate >= 0; --bitrate)
		{
			if(!m_Settings.GetEncoderFactory()->IsBitrateSupported(samplerate, channels, encTraits->bitrates[bitrate]))
			{
				continue;
			}
			int ndx = m_CbnSampleFormat.AddString(mpt::ToCString(m_Settings.GetEncoderFactory()->DescribeBitrateVBR(encTraits->bitrates[bitrate])));
			m_CbnSampleFormat.SetItemData(ndx, (Encoder::ModeVBR<<24) | (encTraits->bitrates[bitrate]<<0));
			if(encSettings.Mode == Encoder::ModeVBR && static_cast<int>(encSettings.Bitrate) == encTraits->bitrates[bitrate])
			{
				sel = ndx;
			}
		}
	}
	if(encTraits->modes & Encoder::ModeABR)
	{
		for(int bitrate = static_cast<int>(encTraits->bitrates.size()-1); bitrate >= 0; --bitrate)
		{
			if(!m_Settings.GetEncoderFactory()->IsBitrateSupported(samplerate, channels, encTraits->bitrates[bitrate]))
			{
				continue;
			}
			int ndx = m_CbnSampleFormat.AddString(mpt::ToCString(m_Settings.GetEncoderFactory()->DescribeBitrateABR(encTraits->bitrates[bitrate])));
			m_CbnSampleFormat.SetItemData(ndx, (Encoder::ModeABR<<24) | (encTraits->bitrates[bitrate]<<0));
			if(encSettings.Mode == Encoder::ModeABR && static_cast<int>(encSettings.Bitrate) == encTraits->bitrates[bitrate])
			{
				sel = ndx;
			}
		}
	}
	if(encTraits->modes & Encoder::ModeCBR)
	{
		for(int bitrate = static_cast<int>(encTraits->bitrates.size()-1); bitrate >= 0; --bitrate)
		{
			if(!m_Settings.GetEncoderFactory()->IsBitrateSupported(samplerate, channels, encTraits->bitrates[bitrate]))
			{
				continue;
			}
			int ndx = m_CbnSampleFormat.AddString(mpt::ToCString(m_Settings.GetEncoderFactory()->DescribeBitrateCBR(encTraits->bitrates[bitrate])));
			m_CbnSampleFormat.SetItemData(ndx, (Encoder::ModeCBR<<24) | (encTraits->bitrates[bitrate]<<0));
			if(encSettings.Mode == Encoder::ModeCBR && static_cast<int>(encSettings.Bitrate) == encTraits->bitrates[bitrate])
			{
				sel = ndx;
			}
		}
	}
	if(encTraits->modes & Encoder::ModeLossless)
	{
		bool allBig = true;
		bool allLittle = true;
		for(const auto &format : encTraits->formats)
		{
			if(format.endian != mpt::endian::little)
			{
				allLittle = false;
			}
			if(format.endian != mpt::endian::big)
			{
				allBig = false;
			}
		}
		bool showEndian = !(allBig || allLittle);
		for(std::size_t i = 0; i < encTraits->formats.size(); ++i)
		{
			const Encoder::Format &format = encTraits->formats[i];
			mpt::ustring description;
			switch(format.encoding)
			{
			case Encoder::Format::Encoding::Float:
				description = MPT_UFORMAT("{} Bit Floating Point")(format.bits);
				break;
			case Encoder::Format::Encoding::Integer:
				description = MPT_UFORMAT("{} Bit")(format.bits);
				break;
			case Encoder::Format::Encoding::Alaw:
				description = U_("A-law");
				break;
			case Encoder::Format::Encoding::ulaw:
				description = MPT_UTF8("\xce\xbc-law");
				break;
			case Encoder::Format::Encoding::Unsigned:
				description = MPT_UFORMAT("{} Bit (unsigned)")(format.bits);
				break;
			}
			if(showEndian && format.bits != 8 && format.encoding != Encoder::Format::Encoding::Alaw && format.encoding != Encoder::Format::Encoding::ulaw)
			{
				switch(format.endian)
				{
				case mpt::endian::big:
					description += U_(" Big-Endian");
					break;
				case mpt::endian::little:
					description += U_(" Little-Endian");
					break;
				}
			}
			int ndx = m_CbnSampleFormat.AddString(mpt::ToCString(description));
			m_CbnSampleFormat.SetItemData(ndx, format.AsInt());
			if(encSettings.Mode & Encoder::ModeLossless && format == encSettings.Format2)
			{
				sel = ndx;
			}
		}
	}
	if(sel == -1)
	{
		sel = 0;
	}
	m_CbnSampleFormat.SetCurSel(sel);
}


void CWaveConvert::FillDither()
{
	EncoderSettingsConf &encSettings = m_Settings.GetEncoderSettings();
	m_CbnDither.CComboBox::ResetContent();
	int format = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel()) & 0xffffff;
	if((encTraits->modes & Encoder::ModeLossless) && !Encoder::Format::FromInt(format).GetSampleFormat().IsFloat())
	{
		m_CbnDither.EnableWindow(TRUE);
		for(std::size_t dither = 0; dither < DithersOpenMPT::GetNumDithers(); ++dither)
		{
			int ndx = m_CbnDither.AddString(mpt::ToCString(DithersOpenMPT::GetModeName(dither) + U_(" dither")));
			m_CbnDither.SetItemData(ndx, dither);
		}
	} else
	{
		m_CbnDither.EnableWindow(FALSE);
		for(std::size_t dither = 0; dither < DithersOpenMPT::GetNumDithers(); ++dither)
		{
			int ndx = m_CbnDither.AddString(mpt::ToCString(DithersOpenMPT::GetModeName(DithersOpenMPT::GetNoDither()) + U_(" dither")));
			m_CbnDither.SetItemData(ndx, dither);
		}
	}
	m_CbnDither.SetCurSel(encSettings.Dither);
}


void CWaveConvert::OnFileTypeChanged()
{
	SaveEncoderSettings();
	DWORD_PTR dwFileType = m_CbnFileType.GetItemData(m_CbnFileType.GetCurSel());
	m_Settings.SelectEncoder(dwFileType);
	encTraits = m_Settings.GetTraits();
	FillSamplerates();
	FillChannels();
	FillFormats();
	FillDither();
	FillTags();
}


void CWaveConvert::OnSamplerateChanged()
{
	SaveEncoderSettings();
	FillFormats();
	FillDither();
}


void CWaveConvert::OnChannelsChanged()
{
	SaveEncoderSettings();
	FillFormats();
	FillDither();
}


void CWaveConvert::OnDitherChanged()
{
	SaveEncoderSettings();
}


void CWaveConvert::OnFormatChanged()
{
	SaveEncoderSettings();
	FillDither();
	FillTags();
}


void CWaveConvert::UpdateDialog()
{
	CheckDlgButton(IDC_CHECK2, (m_dwSongLimit) ? BST_CHECKED : 0);
	GetDlgItem(IDC_EDIT2)->EnableWindow(m_dwSongLimit ? TRUE : FALSE);

	// Repeat / selection play
	int sel = GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO3);

	GetDlgItem(IDC_EDIT3)->EnableWindow(sel == IDC_RADIO2);
	GetDlgItem(IDC_EDIT4)->EnableWindow(sel == IDC_RADIO2);
	m_SpinMinOrder.EnableWindow(sel == IDC_RADIO2);
	m_SpinMaxOrder.EnableWindow(sel == IDC_RADIO2);

	GetDlgItem(IDC_EDIT5)->EnableWindow(sel == IDC_RADIO1);
	m_SpinLoopCount.EnableWindow(sel == IDC_RADIO1);

	const SEQUENCEINDEX numSequences = m_SndFile.Order.GetNumSequences();
	const BOOL enableSeq = (numSequences > 1 && sel == IDC_RADIO3) ? TRUE : FALSE;
	GetDlgItem(IDC_EDIT12)->EnableWindow(enableSeq);
	GetDlgItem(IDC_EDIT13)->EnableWindow(enableSeq);
	m_SpinMinSequence.EnableWindow(enableSeq);
	m_SpinMaxSequence.EnableWindow(enableSeq);

	// No free slots => Cannot do instrument- or channel-based export to sample
	BOOL canDoMultiExport = (IsDlgButtonChecked(IDC_RADIO4) != BST_UNCHECKED /* normal export */ || m_CbnSampleSlot.GetItemData(0) == 0 /* "free slot" is in list */) ? TRUE : FALSE;
	GetDlgItem(IDC_CHECK4)->EnableWindow(canDoMultiExport);
	GetDlgItem(IDC_CHECK6)->EnableWindow(canDoMultiExport);
}


void CWaveConvert::OnExportModeChanged()
{
	SaveEncoderSettings();
	bool sampleExport = (IsDlgButtonChecked(IDC_RADIO5) != BST_UNCHECKED);
	m_CbnFileType.EnableWindow(sampleExport ? FALSE : TRUE);
	m_CbnSampleSlot.EnableWindow(sampleExport && !IsDlgButtonChecked(IDC_CHECK4) && !IsDlgButtonChecked(IDC_CHECK6));
	if(sampleExport)
	{
		// Render to sample: Always use WAV
		if(m_CbnFileType.GetCurSel() != 0)
		{
			m_CbnFileType.SetCurSel(0);
			OnFileTypeChanged();
		}
	}
	FillChannels();
	FillFormats();
	FillDither();
	FillTags();
}


void CWaveConvert::OnSampleSlotChanged()
{
	CheckRadioButton(IDC_RADIO4, IDC_RADIO5, IDC_RADIO5);
	// When choosing a specific sample slot, we cannot use per-channel or per-instrument export
	int sel = m_CbnSampleSlot.GetCurSel();
	if(sel >= 0 && m_CbnSampleSlot.GetItemData(sel) > 0)
	{
		CheckDlgButton(IDC_CHECK4, BST_UNCHECKED);
		CheckDlgButton(IDC_CHECK6, BST_UNCHECKED);
	}
	UpdateDialog();
}


void CWaveConvert::OnPlayerOptions()
{
	CPropertySheet dlg(_T("Mixer Settings"), this);
	COptionsMixer mixerpage;
	dlg.AddPage(&mixerpage);
#if !defined(NO_REVERB) || !defined(NO_DSP) || !defined(NO_EQ) || !defined(NO_AGC)
	COptionsPlayer dsppage;
	dlg.AddPage(&dsppage);
#endif
	dlg.DoModal();
}


void CWaveConvert::OnCheckTimeLimit()
{
	if (IsDlgButtonChecked(IDC_CHECK2))
	{
		m_dwSongLimit = GetDlgItemInt(IDC_EDIT2, NULL, FALSE);
		if (!m_dwSongLimit)
		{
			m_dwSongLimit = 600;
			SetDlgItemText(IDC_EDIT2, _T("600"));
		}
	} else m_dwSongLimit = 0;
	UpdateDialog();
}


// Channel render is mutually exclusive with instrument render
void CWaveConvert::OnCheckChannelMode()
{
	if(IsDlgButtonChecked(IDC_CHECK4) != BST_UNCHECKED)
	{
		CheckDlgButton(IDC_CHECK6, BST_UNCHECKED);
		m_CbnSampleSlot.SetCurSel(0);
	}
	UpdateDialog();
}


// Channel render is mutually exclusive with instrument render
void CWaveConvert::OnCheckInstrMode()
{
	if(IsDlgButtonChecked(IDC_CHECK6) != BST_UNCHECKED)
	{
		CheckDlgButton(IDC_CHECK4, BST_UNCHECKED);
		m_CbnSampleSlot.SetCurSel(0);
	}
	UpdateDialog();
}


void CWaveConvert::OnOK()
{
	if (m_dwSongLimit) m_dwSongLimit = GetDlgItemInt(IDC_EDIT2, NULL, FALSE);

	const bool selection = IsDlgButtonChecked(IDC_RADIO2) != BST_UNCHECKED;
	if(selection)
	{
		// Play selection
		m_Settings.minOrder = static_cast<ORDERINDEX>(GetDlgItemInt(IDC_EDIT3, NULL, FALSE));
		m_Settings.maxOrder = static_cast<ORDERINDEX>(GetDlgItemInt(IDC_EDIT4, NULL, FALSE));
		if(m_Settings.minOrder > m_Settings.maxOrder)
			std::swap(m_Settings.minOrder, m_Settings.maxOrder);
	} else
	{
		m_Settings.minOrder = m_Settings.maxOrder = ORDERINDEX_INVALID;
	}
	if(IsDlgButtonChecked(IDC_RADIO3))
	{
		const UINT maxSequence = m_SndFile.Order.GetNumSequences();
		m_Settings.minSequence = static_cast<SEQUENCEINDEX>(std::clamp(GetDlgItemInt(IDC_EDIT12, NULL, FALSE), 1u, maxSequence) - 1u);
		m_Settings.maxSequence = static_cast<SEQUENCEINDEX>(std::clamp(GetDlgItemInt(IDC_EDIT13, NULL, FALSE), 1u, maxSequence) - 1u);
		if(m_Settings.minSequence > m_Settings.maxSequence)
			std::swap(m_Settings.minSequence, m_Settings.maxSequence);
	} else
	{
		m_Settings.minSequence = m_Settings.maxSequence = m_SndFile.Order.GetCurrentSequenceIndex();
	}

	m_Settings.repeatCount = static_cast<uint16>(GetDlgItemInt(IDC_EDIT5, NULL, FALSE));
	m_Settings.normalize = IsDlgButtonChecked(IDC_CHECK5) != BST_UNCHECKED;
	m_Settings.silencePlugBuffers = IsDlgButtonChecked(IDC_RENDERSILENCE) != BST_UNCHECKED;
	m_Settings.outputToSample = IsDlgButtonChecked(IDC_RADIO5) != BST_UNCHECKED;
	m_bGivePlugsIdleTime = IsDlgButtonChecked(IDC_GIVEPLUGSIDLETIME) != BST_UNCHECKED;
	if (m_bGivePlugsIdleTime)
	{
		static bool showWarning = true;
		if(showWarning && Reporting::Confirm("You only need slow render if you are experiencing dropped notes with a Kontakt based sampler with Direct-From-Disk enabled, or buggy plugins that use the system time for parameter automation.\nIt will make rendering *very* slow.\n\nAre you sure you want to enable slow render?",
			"Really enable slow render?") == cnfNo)
		{
			m_bGivePlugsIdleTime = false;
		} else
		{
			showWarning = false;
		}
	}

	m_bChannelMode = IsDlgButtonChecked(IDC_CHECK4) != BST_UNCHECKED;
	m_bInstrumentMode= IsDlgButtonChecked(IDC_CHECK6) != BST_UNCHECKED;

	m_Settings.sampleSlot = static_cast<SAMPLEINDEX>(m_CbnSampleSlot.GetItemData(m_CbnSampleSlot.GetCurSel()));

	SaveEncoderSettings();

	EncoderSettingsConf &encSettings = m_Settings.GetEncoderSettings();

	m_Settings.Tags = FileTags();

	m_Settings.Tags.SetEncoder();

	if(encSettings.Tags)
	{
		CString tmp;

		m_EditTitle.GetWindowText(tmp);
		m_Settings.Tags.title = mpt::ToUnicode(tmp);

		m_EditAuthor.GetWindowText(tmp);
		m_Settings.Tags.artist = mpt::ToUnicode(tmp);

		m_EditAlbum.GetWindowText(tmp);
		m_Settings.Tags.album = mpt::ToUnicode(tmp);

		m_EditURL.GetWindowText(tmp);
		m_Settings.Tags.url = mpt::ToUnicode(tmp);

		if((encTraits->modesWithFixedGenres & encSettings.Mode) && !encTraits->genres.empty())
		{
			m_CbnGenre.GetWindowText(tmp);
			m_Settings.Tags.genre = mpt::ToUnicode(tmp);
		} else
		{
			m_EditGenre.GetWindowText(tmp);
			m_Settings.Tags.genre = mpt::ToUnicode(tmp);
		}

		m_EditYear.GetWindowText(tmp);
		m_Settings.Tags.year = mpt::ToUnicode(tmp);
		if(m_Settings.Tags.year == U_("0"))
		{
			m_Settings.Tags.year = mpt::ustring();
		}

		if(!m_SndFile.m_songMessage.empty())
		{
			m_Settings.Tags.comments = mpt::ToUnicode(mpt::Charset::Locale, m_SndFile.m_songMessage.GetFormatted(SongMessage::leLF));
		}

		m_Settings.Tags.bpm = mpt::ufmt::val(m_SndFile.GetCurrentBPM());

		SaveTags();

	}

	CDialog::OnOK();

}


void CWaveConvert::SaveEncoderSettings()
{
	EncoderSettingsConf &encSettings = m_Settings.GetEncoderSettings();

	encSettings.Samplerate = static_cast<uint32>(m_CbnSampleRate.GetItemData(m_CbnSampleRate.GetCurSel()));
	encSettings.Channels = static_cast<uint16>(m_CbnChannels.GetItemData(m_CbnChannels.GetCurSel()));
	DWORD_PTR dwFormat = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());

	if(encTraits->modes & Encoder::ModeLossless)
	{
		int format = (int)((dwFormat >> 0) & 0xffffff);
		encSettings.Dither = static_cast<int>(m_CbnDither.GetItemData(m_CbnDither.GetCurSel()));
		encSettings.Format2 = Encoder::Format::FromInt(format);
		encSettings.Mode = Encoder::ModeLossless;
		encSettings.Bitrate = 0;
		encSettings.Quality = encTraits->defaultQuality;
	} else
	{
		encSettings.Dither = static_cast<int>(m_CbnDither.GetItemData(m_CbnDither.GetCurSel()));
		Encoder::Mode mode = (Encoder::Mode)((dwFormat >> 24) & 0xff);
		int quality = (int)((dwFormat >> 0) & 0xff);
		int bitrate = (int)((dwFormat >> 0) & 0xffff);
		encSettings.Mode = mode;
		encSettings.Bitrate = bitrate;
		encSettings.Quality = static_cast<float>(quality) * 0.01f;
		encSettings.Format2 = { Encoder::Format::Encoding::Float, 32, mpt::get_endian() };
	}
	
	encSettings.Cues = IsDlgButtonChecked(IDC_CHECK3) ? true : false;

	encSettings.Tags = IsDlgButtonChecked(IDC_CHECK7) ? true : false;

}


std::size_t CWaveConvertSettings::FindEncoder(const mpt::ustring &name) const
{
	for(std::size_t i = 0; i < EncoderFactories.size(); ++i)
	{
		if(EncoderFactories[i]->GetTraits().encoderSettingsName == name)
		{
			return i;
		}
	}
	return 0;
}


void CWaveConvertSettings::SelectEncoder(std::size_t index)
{
	MPT_ASSERT(!EncoderFactories.empty());
	MPT_ASSERT(index < EncoderFactories.size());
	EncoderIndex = index;
	EncoderName = EncoderFactories[EncoderIndex]->GetTraits().encoderSettingsName;
}


EncoderFactoryBase *CWaveConvertSettings::GetEncoderFactory() const
{
	MPT_ASSERT(!EncoderFactories.empty());
	return EncoderFactories[EncoderIndex];
}


const Encoder::Traits *CWaveConvertSettings::GetTraits() const
{
	MPT_ASSERT(!EncoderFactories.empty());
	return &EncoderFactories[EncoderIndex]->GetTraits();
}


EncoderSettingsConf &CWaveConvertSettings::GetEncoderSettings() const
{
	MPT_ASSERT(!EncoderSettings.empty());
	return *(EncoderSettings[EncoderIndex]);
}


Encoder::Settings CWaveConvertSettings::GetEncoderSettingsWithDetails() const
{
	MPT_ASSERT(!EncoderSettings.empty());
	Encoder::Settings settings = static_cast<Encoder::Settings>(*(EncoderSettings[EncoderIndex]));
	settings.Details = static_cast<Encoder::StreamSettings>(TrackerSettings::Instance().ExportStreamEncoderSettings);
	return settings;
}


CWaveConvertSettings::CWaveConvertSettings(SettingsContainer &conf, const std::vector<EncoderFactoryBase*> &encFactories)
	: EncoderFactories(encFactories)
	, EncoderName(conf, U_("Export"), U_("Encoder"), U_(""))
	, EncoderIndex(FindEncoder(EncoderName))
	, storedTags(conf)
	, repeatCount(0)
	, minOrder(ORDERINDEX_INVALID), maxOrder(ORDERINDEX_INVALID)
	, sampleSlot(0)
	, normalize(false)
	, silencePlugBuffers(false)
	, outputToSample(false)
{
	Tags.SetEncoder();
	for(const auto & factory : EncoderFactories)
	{
		const Encoder::Traits &encTraits = factory->GetTraits();
		EncoderSettings.push_back(
			std::make_unique<EncoderSettingsConf>(
				conf,
				encTraits.encoderSettingsName,
				encTraits.canCues,
				encTraits.canTags,
				encTraits.defaultSamplerate,
				encTraits.defaultChannels,
				encTraits.defaultMode,
				encTraits.defaultBitrate,
				encTraits.defaultQuality,
				encTraits.defaultFormat,
				encTraits.defaultDitherType
			)
		);
	}
	SelectEncoder(EncoderIndex);
}


/////////////////////////////////////////////////////////////////////////////////////////
// CDoWaveConvert: save a mod as a wave file

void CDoWaveConvert::Run()
{

	UINT ok = IDOK;
	uint64 ullSamples = 0;

	std::vector<float> normalizeBufferData;
	float *normalizeBuffer = nullptr;
	float normalizePeak = 0.0f;
	const mpt::PathString normalizeFileName = mpt::TemporaryPathname{}.GetPathname();
	std::optional<mpt::IO::fstream> normalizeFile;
	if(m_Settings.normalize)
	{
		normalizeBufferData.resize(MIXBUFFERSIZE * 4);
		normalizeBuffer = normalizeBufferData.data();
		// Ensure this temporary file is marked as temporary in the file system, to increase the chance it will never be written to disk
		if(HANDLE hFile = ::CreateFile(normalizeFileName.AsNative().c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL); hFile != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(hFile);
		}
		normalizeFile.emplace(normalizeFileName, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
	}

	const Encoder::Settings encSettings = m_Settings.GetEncoderSettingsWithDetails();
	const uint32 samplerate = encSettings.Samplerate;
	const uint16 channels = encSettings.Channels;

	ASSERT(m_Settings.GetEncoderFactory() && m_Settings.GetEncoderFactory()->IsAvailable());

	// Silence mix buffer of plugins, for plugins that don't clear their reverb buffers and similar stuff when they are reset
#ifndef NO_PLUGINS
	if(m_Settings.silencePlugBuffers)
	{
		SetText(_T("Clearing plugin buffers"));
		for(auto &plug : m_SndFile.m_MixPlugins)
		{
			if(plug.pMixPlugin != nullptr)
			{
				// Render up to 20 seconds per plugin
				for(int j = 0; j < 20; j++)
				{
					const float maxVal = plug.pMixPlugin->RenderSilence(samplerate);
					if(maxVal <= FLT_EPSILON)
					{
						break;
					}
				}

				ProcessMessages();
				if(m_abort)
				{
					m_abort = false;
					break;
				}
			}
		}
	}
#endif // NO_PLUGINS

	MixerSettings oldmixersettings = m_SndFile.m_MixerSettings;
	MixerSettings mixersettings = TrackerSettings::Instance().GetMixerSettings();
	mixersettings.m_nMaxMixChannels = MAX_CHANNELS; // always use max mixing channels when rendering
	mixersettings.gdwMixingFreq = samplerate;
	mixersettings.gnChannels = channels;
	m_SndFile.m_SongFlags.reset(SONG_PAUSED | SONG_STEP);
	if(m_Settings.normalize)
	{
#ifndef NO_AGC
		mixersettings.DSPMask &= ~SNDDSP_AGC;
#endif
	}

	DithersOpenMPT dithers(theApp.PRNG(), encSettings.Dither, encSettings.Channels);

	m_SndFile.ResetChannels();
	m_SndFile.SetMixerSettings(mixersettings);
	m_SndFile.SetResamplerSettings(TrackerSettings::Instance().GetResamplerSettings());
	m_SndFile.InitPlayer(true);

	// Tags must be known at the stream start,
	// so that the encoder class could write them before audio data if mandated by the format,
	// otherwise they should just be cached by the encoder.
	std::unique_ptr<IAudioStreamEncoder> fileEnc = m_Settings.GetEncoderFactory()->ConstructStreamEncoder(fileStream, encSettings, m_Settings.Tags);

	std::variant<
		std::unique_ptr<std::array<double, MIXBUFFERSIZE * 4>>,
		std::unique_ptr<std::array<float, MIXBUFFERSIZE * 4>>,
		std::unique_ptr<std::array<int32, MIXBUFFERSIZE * 4>>,
		std::unique_ptr<std::array<int24, MIXBUFFERSIZE * 4>>,
		std::unique_ptr<std::array<int16, MIXBUFFERSIZE * 4>>,
		std::unique_ptr<std::array<int8, MIXBUFFERSIZE * 4>>,
		std::unique_ptr<std::array<uint8, MIXBUFFERSIZE * 4>>> bufferData;
	union AnyBufferSamplePointer
	{
		double *float64;
		float *float32;
		int32 *int32;
		int24 *int24;
		int16 *int16;
		int8 *int8;
		uint8 *uint8;
		void *any;
	};
	AnyBufferSamplePointer buffer;
	buffer.any = nullptr;
	switch(fileEnc->GetSampleFormat())
	{
	case SampleFormat::Float64:
		bufferData = std::make_unique<std::array<double, MIXBUFFERSIZE * 4>>();
		buffer.float64 = std::get<0>(bufferData)->data();
		break;
	case SampleFormat::Float32:
		bufferData = std::make_unique<std::array<float, MIXBUFFERSIZE * 4>>();
		buffer.float32 = std::get<1>(bufferData)->data();
		break;
	case SampleFormat::Int32:
		bufferData = std::make_unique<std::array<int32, MIXBUFFERSIZE * 4>>();
		buffer.int32 = std::get<2>(bufferData)->data();
		break;
	case SampleFormat::Int24:
		bufferData = std::make_unique<std::array<int24, MIXBUFFERSIZE * 4>>();
		buffer.int24 = std::get<3>(bufferData)->data();
		break;
	case SampleFormat::Int16:
		bufferData = std::make_unique<std::array<int16, MIXBUFFERSIZE * 4>>();
		buffer.int16 = std::get<4>(bufferData)->data();
		break;
	case SampleFormat::Int8:
		bufferData = std::make_unique<std::array<int8, MIXBUFFERSIZE * 4>>();
		buffer.int8 = std::get<5>(bufferData)->data();
		break;
	case SampleFormat::Unsigned8:
		bufferData = std::make_unique<std::array<uint8, MIXBUFFERSIZE * 4>>();
		buffer.uint8 = std::get<6>(bufferData)->data();
		break;
	}

	uint64 ullMaxSamples = uint64_max / (channels * ((fileEnc->GetSampleFormat().GetBitsPerSample()+7) / 8));
	if (m_dwSongLimit)
	{
		LimitMax(ullMaxSamples, m_dwSongLimit * samplerate);
	}

	// Calculate maximum samples
	uint64 max = m_dwSongLimit ? ullMaxSamples : uint64_max;

	// Reset song position tracking
	m_SndFile.ResetPlayPos();
	m_SndFile.m_SongFlags.reset(SONG_PATTERNLOOP);
	ORDERINDEX startOrder = 0;
	GetLengthTarget target;
	if(m_Settings.minOrder != ORDERINDEX_INVALID && m_Settings.maxOrder != ORDERINDEX_INVALID)
	{
		m_SndFile.SetRepeatCount(0);
		startOrder = m_Settings.minOrder;
		ORDERINDEX endOrder = m_Settings.maxOrder;
		while(!m_SndFile.Order().IsValidPat(endOrder) && endOrder > startOrder)
		{
			endOrder--;
		}
		if(m_SndFile.Order().IsValidPat(endOrder))
		{
			target = GetLengthTarget(endOrder, m_SndFile.Patterns[m_SndFile.Order()[endOrder]].GetNumRows() - 1);
		}
		target.StartPos(m_SndFile.Order.GetCurrentSequenceIndex(), startOrder, 0);
		m_SndFile.m_nMaxOrderPosition = endOrder + 1;
	} else
	{
		m_SndFile.SetRepeatCount(std::max(0, m_Settings.repeatCount - 1));
	}
	uint64 l = mpt::saturate_round<uint64>(m_SndFile.GetLength(eNoAdjust, target).front().duration * samplerate * (1 + m_SndFile.GetRepeatCount()));

	m_SndFile.SetCurrentOrder(startOrder);
	m_SndFile.GetLength(eAdjust, GetLengthTarget(startOrder, 0));	// adjust playback variables / visited rows vector
	m_SndFile.m_PlayState.m_nCurrentOrder = startOrder;

	if (l < max) max = l;

	SetRange(0, max);
	EnableTaskbarProgress();

	// No pattern cue points yet
	std::vector<PatternCuePoint> patternCuePoints;
	patternCuePoints.reserve(m_SndFile.Order().size());
	m_SndFile.m_PatternCuePoints = &patternCuePoints;

	// Process the conversion

	// For calculating the remaining time
	auto dwStartTime = timeGetTime(), prevTime = dwStartTime;
	uint32 timeRemaining = 0;

	uint64 bytesWritten = 0;

	auto mainFrame = CMainFrame::GetMainFrame();
	mainFrame->PauseMod();
	m_SndFile.m_SongFlags.reset(SONG_STEP | SONG_PATTERNLOOP);
	mainFrame->InitRenderer(&m_SndFile);

	for (UINT n = 0; ; n++)
	{
		UINT lRead = 0;
		if(m_Settings.normalize)
		{
			lRead = ReadInterleaved(m_SndFile, normalizeBuffer, channels, MIXBUFFERSIZE, dithers);
		} else
		{
			switch(fileEnc->GetSampleFormat())
			{
			case SampleFormat::Float64:
				lRead = ReadInterleaved(m_SndFile, buffer.float64, channels, MIXBUFFERSIZE, dithers);
				break;
			case SampleFormat::Float32:
				lRead = ReadInterleaved(m_SndFile, buffer.float32, channels, MIXBUFFERSIZE, dithers);
				break;
			case SampleFormat::Int32:
				lRead = ReadInterleaved(m_SndFile, buffer.int32, channels, MIXBUFFERSIZE, dithers);
				break;
			case SampleFormat::Int24:
				lRead = ReadInterleaved(m_SndFile, buffer.int24, channels, MIXBUFFERSIZE, dithers);
				break;
			case SampleFormat::Int16:
				lRead = ReadInterleaved(m_SndFile, buffer.int16, channels, MIXBUFFERSIZE, dithers);
				break;
			case SampleFormat::Int8:
				lRead = ReadInterleaved(m_SndFile, buffer.int8, channels, MIXBUFFERSIZE, dithers);
				break;
			case SampleFormat::Unsigned8:
				lRead = ReadInterleaved(m_SndFile, buffer.uint8, channels, MIXBUFFERSIZE, dithers);
				break;
			}
		}

		// Process cue points (add base offset), if there are any to process.
		for(auto iter = patternCuePoints.rbegin(); iter != patternCuePoints.rend(); ++iter)
		{
			if(iter->processed)
			{
				// From this point, all cues have already been processed.
				break;
			}
			iter->offset += ullSamples;
			iter->processed = true;
		}

		if (m_bGivePlugsIdleTime)
		{
			Sleep(20);
		}

		if (!lRead)
			break;
		ullSamples += lRead;

		if(m_Settings.normalize)
		{

			std::size_t countSamples = lRead * m_SndFile.m_MixerSettings.gnChannels;
			const float *src = normalizeBuffer;
			while(countSamples--)
			{
				const float val = *src;
				if(val > normalizePeak) normalizePeak = val;
				else if(0.0f - val >= normalizePeak) normalizePeak = 0.0f - val;
				src++;
			}

			if(!mpt::IO::WriteRaw(*normalizeFile, mpt::as_span(reinterpret_cast<const std::byte*>(normalizeBuffer), lRead * m_SndFile.m_MixerSettings.gnChannels * sizeof(float))))
			{
				break;
			}

		} else
		{

			const std::streampos oldPos = fileStream.tellp();
			switch(fileEnc->GetSampleFormat())
			{
			case SampleFormat::Float64:
				fileEnc->WriteInterleaved(lRead, buffer.float64);
				break;
			case SampleFormat::Float32:
				fileEnc->WriteInterleaved(lRead, buffer.float32);
				break;
			case SampleFormat::Int32:
				fileEnc->WriteInterleaved(lRead, buffer.int32);
				break;
			case SampleFormat::Int24:
				fileEnc->WriteInterleaved(lRead, buffer.int24);
				break;
			case SampleFormat::Int16:
				fileEnc->WriteInterleaved(lRead, buffer.int16);
				break;
			case SampleFormat::Int8:
				fileEnc->WriteInterleaved(lRead, buffer.int8);
				break;
			case SampleFormat::Unsigned8:
				fileEnc->WriteInterleaved(lRead, buffer.uint8);
				break;
			}
			const std::streampos newPos = fileStream.tellp();
			bytesWritten += static_cast<uint64>(newPos - oldPos);

			if(!fileStream)
			{
				break;
			}

		}
		if(m_dwSongLimit && (ullSamples >= ullMaxSamples))
		{
			break;
		}

		auto currentTime = timeGetTime();
		if((currentTime - prevTime) >= 16)
		{
			prevTime = currentTime;

			DWORD seconds = (DWORD)(ullSamples / m_SndFile.m_MixerSettings.gdwMixingFreq);

			if((ullSamples > 0) && (ullSamples < max))
			{
				timeRemaining = static_cast<uint32>((timeRemaining + ((currentTime - dwStartTime) * (max - ullSamples) / ullSamples) / 1000) / 2);
			}

			if(m_Settings.normalize)
			{
				SetText(MPT_CFORMAT("Rendering {}... ({}mn{}s, {}mn{}s remaining)")(caption, seconds / 60, mpt::ufmt::dec0<2>(seconds % 60u), timeRemaining / 60, mpt::ufmt::dec0<2>(timeRemaining % 60u)));
			} else
			{
				SetText(MPT_CFORMAT("Writing {}... ({}kB, {}mn{}s, {}mn{}s remaining)")(caption, bytesWritten >> 10, seconds / 60, mpt::ufmt::dec0<2>(seconds % 60u), timeRemaining / 60, mpt::ufmt::dec0<2>(timeRemaining % 60u)));
			}

			SetProgress(ullSamples);
		}
		ProcessMessages();

		if (m_abort)
		{
			ok = IDCANCEL;
			break;
		}
	}

	m_SndFile.m_nMaxOrderPosition = 0;

	mainFrame->StopRenderer(&m_SndFile);

	if(m_Settings.normalize)
	{
		const float normalizeFactor = (normalizePeak != 0.0f) ? (1.0f / normalizePeak) : 1.0f;

		const uint64 framesTotal = ullSamples;
		int lastPercent = -1;

		mpt::IO::SeekAbsolute(*normalizeFile, 0);

		uint64 framesProcessed = 0;
		uint64 framesToProcess = framesTotal;

		SetRange(0, framesTotal);

		while(framesToProcess)
		{
			const std::size_t framesChunk = std::min(mpt::saturate_cast<std::size_t>(framesToProcess), std::size_t(MIXBUFFERSIZE));
			const uint32 samplesChunk = static_cast<uint32>(framesChunk * channels);
			
			const std::size_t bytes = samplesChunk * sizeof(float);
			if(mpt::IO::ReadRaw(*normalizeFile, mpt::as_span(reinterpret_cast<std::byte*>(normalizeBuffer), bytes)).size() != bytes)
			{
				break;
			}

			for(std::size_t i = 0; i < samplesChunk; ++i)
			{
				normalizeBuffer[i] *= normalizeFactor;
			}

			const std::streampos oldPos = fileStream.tellp();
			std::visit(
				[&](auto& ditherInstance)
				{
					switch(fileEnc->GetSampleFormat())
					{
					case SampleFormat::Unsigned8:
						ConvertBufferMixInternalToBuffer<false>(mpt::audio_span_interleaved<uint8>(buffer.uint8, channels, framesChunk), mpt::audio_span_interleaved<const MixSampleFloat>(normalizeBuffer, channels, framesChunk), ditherInstance, channels, framesChunk);
						break;
					case SampleFormat::Int8:
						ConvertBufferMixInternalToBuffer<false>(mpt::audio_span_interleaved<int8>(buffer.int8, channels, framesChunk), mpt::audio_span_interleaved<const MixSampleFloat>(normalizeBuffer, channels, framesChunk), ditherInstance, channels, framesChunk);
						break;
					case SampleFormat::Int16:
						ConvertBufferMixInternalToBuffer<false>(mpt::audio_span_interleaved<int16>(buffer.int16, channels, framesChunk), mpt::audio_span_interleaved<const MixSampleFloat>(normalizeBuffer, channels, framesChunk), ditherInstance, channels, framesChunk);
						break;
					case SampleFormat::Int24:
						ConvertBufferMixInternalToBuffer<false>(mpt::audio_span_interleaved<int24>(buffer.int24, channels, framesChunk), mpt::audio_span_interleaved<const MixSampleFloat>(normalizeBuffer, channels, framesChunk), ditherInstance, channels, framesChunk);
						break;
					case SampleFormat::Int32:
						ConvertBufferMixInternalToBuffer<false>(mpt::audio_span_interleaved<int32>(buffer.int32, channels, framesChunk), mpt::audio_span_interleaved<const MixSampleFloat>(normalizeBuffer, channels, framesChunk), ditherInstance, channels, framesChunk);
						break;
					case SampleFormat::Float32:
						ConvertBufferMixInternalToBuffer<false>(mpt::audio_span_interleaved<float>(buffer.float32, channels, framesChunk), mpt::audio_span_interleaved<const MixSampleFloat>(normalizeBuffer, channels, framesChunk), ditherInstance, channels, framesChunk);
						break;
					case SampleFormat::Float64:
						ConvertBufferMixInternalToBuffer<false>(mpt::audio_span_interleaved<double>(buffer.float64, channels, framesChunk), mpt::audio_span_interleaved<const MixSampleFloat>(normalizeBuffer, channels, framesChunk), ditherInstance, channels, framesChunk);
						break;
					default: MPT_ASSERT_NOTREACHED(); break;
					}
				},
				dithers.Variant()
			);
			switch(fileEnc->GetSampleFormat())
			{
			case SampleFormat::Float64:
				fileEnc->WriteInterleaved(framesChunk, buffer.float64);
				break;
			case SampleFormat::Float32:
				fileEnc->WriteInterleaved(framesChunk, buffer.float32);
				break;
			case SampleFormat::Int32:
				fileEnc->WriteInterleaved(framesChunk, buffer.int32);
				break;
			case SampleFormat::Int24:
				fileEnc->WriteInterleaved(framesChunk, buffer.int24);
				break;
			case SampleFormat::Int16:
				fileEnc->WriteInterleaved(framesChunk, buffer.int16);
				break;
			case SampleFormat::Int8:
				fileEnc->WriteInterleaved(framesChunk, buffer.int8);
				break;
			case SampleFormat::Unsigned8:
				fileEnc->WriteInterleaved(framesChunk, buffer.uint8);
				break;
			}
			const std::streampos newPos = fileStream.tellp();
			bytesWritten += static_cast<std::size_t>(newPos - oldPos);

			auto currentTime = timeGetTime();
			if((currentTime - prevTime) >= 16)
			{
				prevTime = currentTime;
				
				int percent = static_cast<int>(100 * framesProcessed / framesTotal);
				if(percent != lastPercent)
				{
					SetText(MPT_CFORMAT("Normalizing... ({}%)")(percent));
					SetProgress(framesProcessed);
					lastPercent = percent;
				}
				ProcessMessages();
			}

			framesProcessed += framesChunk;
			framesToProcess -= framesChunk;
		}

		mpt::IO::Flush(*normalizeFile);
		normalizeFile.reset();
		for(int retry=0; retry<10; retry++)
		{
			// stupid virus scanners
			if(DeleteFile(normalizeFileName.AsNative().c_str()) != EACCES)
			{
				break;
			}
			Sleep(10);
		}

	}

	if(!patternCuePoints.empty())
	{
		if(encSettings.Cues)
		{
			std::vector<uint64> cues;
			cues.reserve(patternCuePoints.size());
			for(const auto &cue : patternCuePoints)
			{
				cues.push_back(cue.offset);
			}
			fileEnc->WriteCues(cues);
		}
	}
	m_SndFile.m_PatternCuePoints = nullptr;

	fileEnc->WriteFinalize();

	fileEnc = nullptr;

	CMainFrame::UpdateAudioParameters(m_SndFile, TRUE);
	EndDialog(ok);
}


OPENMPT_NAMESPACE_END
