/*
 * mod2wave.cpp
 * ------------
 * Purpose: Module to WAV conversion (dialog + conversion code).
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "Sndfile.h"
#include "Dlsbank.h"
#include "mainfrm.h"
#include "mpdlgs.h"
#include "mod2wave.h"
#include "WAVTools.h"
#include "../common/mptString.h"
#include "../common/version.h"
#include "../soundlib/MixerLoops.h"
#include "../soundlib/Dither.h"
#include "../soundlib/AudioReadTarget.h"
#include "../soundlib/plugins/PlugInterface.h"

#include "../common/mptFileIO.h"


OPENMPT_NAMESPACE_BEGIN


extern const TCHAR *gszChnCfgNames[3];

static CSoundFile::samplecount_t ReadInterleaved(CSoundFile &sndFile, void *outputBuffer, CSoundFile::samplecount_t count, SampleFormat sampleFormat, Dither &dither)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	sndFile.ResetMixStat();
	AudioReadTargetBufferInterleavedDynamic target(sampleFormat, false, dither, outputBuffer);
	return sndFile.Read(count, target);
}


static mpt::ustring GetDefaultYear()
//----------------------------------
{
	return mpt::ToUnicode(CTime::GetCurrentTime().Format("%Y"));
}


StoredTags::StoredTags(SettingsContainer &conf)
//---------------------------------------------
	: artist(conf, "Export", "TagArtist", TrackerSettings::Instance().defaultArtist)
	, album(conf, "Export", "TagAlbum", MPT_USTRING(""))
	, trackno(conf, "Export", "TagTrackNo", MPT_USTRING(""))
	, year(conf, "Export", "TagYear", GetDefaultYear())
	, url(conf, "Export", "TagURL", MPT_USTRING(""))
	, genre(conf, "Export", "TagGenre", MPT_USTRING(""))
{
	return;
}


///////////////////////////////////////////////////
// CWaveConvert - setup for converting a wave file

BEGIN_MESSAGE_MAP(CWaveConvert, CDialog)
	ON_COMMAND(IDC_CHECK1,			OnCheckSizeLimit)
	ON_COMMAND(IDC_CHECK2,			OnCheckTimeLimit)
	ON_COMMAND(IDC_CHECK4,			OnCheckChannelMode)
	ON_COMMAND(IDC_CHECK6,			OnCheckInstrMode)
	ON_COMMAND(IDC_RADIO1,			UpdateDialog)
	ON_COMMAND(IDC_RADIO2,			UpdateDialog)
	ON_COMMAND(IDC_RADIO3,			OnExportModeChanged)
	ON_COMMAND(IDC_RADIO4,			OnExportModeChanged)
	ON_COMMAND(IDC_PLAYEROPTIONS,	OnPlayerOptions)
	ON_COMMAND(IDC_BUTTON1,			OnShowEncoderInfo)
	ON_CBN_SELCHANGE(IDC_COMBO5,	OnFileTypeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnSamplerateChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,	OnChannelsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO6,	OnDitherChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnFormatChanged)
	ON_CBN_SELCHANGE(IDC_COMBO9,	OnSampleSlotChanged)
END_MESSAGE_MAP()


CWaveConvert::CWaveConvert(CWnd *parent, ORDERINDEX minOrder, ORDERINDEX maxOrder, ORDERINDEX numOrders, CSoundFile &sndFile, const std::vector<EncoderFactoryBase*> &encFactories)
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	: CDialog(IDD_WAVECONVERT, parent)
	, m_SndFile(sndFile)
	, m_Settings(theApp.GetSettings(), encFactories)
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
	m_nNumOrders = numOrders;

	m_dwFileLimit = 0;
	m_dwSongLimit = 0;
}


void CWaveConvert::DoDataExchange(CDataExchange *pDX)
//---------------------------------------------------
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
//-------------------------------
{
	CDialog::OnInitDialog();

	CheckDlgButton(IDC_CHECK5, BST_UNCHECKED);	// Normalize
	CheckDlgButton(IDC_CHECK3, BST_CHECKED);	// Cue points

	CheckDlgButton(IDC_CHECK4, BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK6, BST_UNCHECKED);

	const bool selection = (m_Settings.minOrder != ORDERINDEX_INVALID && m_Settings.maxOrder != ORDERINDEX_INVALID);
	CheckRadioButton(IDC_RADIO1, IDC_RADIO2, selection ? IDC_RADIO2 : IDC_RADIO1);
	if(selection)
	{
		SetDlgItemInt(IDC_EDIT3, m_Settings.minOrder);
		SetDlgItemInt(IDC_EDIT4, m_Settings.maxOrder);
	}
	m_SpinMinOrder.SetRange(0, m_nNumOrders);
	m_SpinMaxOrder.SetRange(0, m_nNumOrders);

	SetDlgItemInt(IDC_EDIT5, m_Settings.repeatCount, FALSE);
	m_SpinLoopCount.SetRange(1, int16_max);

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
	for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
	{
		if(m_SndFile.m_MixPlugins[i].pMixPlugin != nullptr)
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
		s.Format(_T("%02u: %s%s"), smp, m_SndFile.GetSample(smp).HasSampleData() ? _T("*") : _T(""), m_SndFile.GetSampleName(smp));
		m_CbnSampleSlot.SetItemData(m_CbnSampleSlot.AddString(s), smp);
	}
	if(m_Settings.sampleSlot > m_SndFile.GetNumSamples()) m_Settings.sampleSlot = 0;
	m_CbnSampleSlot.SetCurSel(m_Settings.sampleSlot);

	CheckRadioButton(IDC_RADIO3, IDC_RADIO4, m_Settings.outputToSample ? IDC_RADIO4 : IDC_RADIO3);

	UpdateDialog();
	return TRUE;
}


void CWaveConvert::LoadTags()
//---------------------------
{
	m_Settings.Tags.title = mpt::ToUnicode(mpt::CharsetLocale, m_SndFile.GetTitle());
	m_Settings.Tags.comments = mpt::ToUnicode(mpt::CharsetLocale, m_SndFile.m_songMessage.GetFormatted(SongMessage::leLF));
	m_Settings.Tags.artist = m_SndFile.m_songArtist;
	m_Settings.Tags.album = m_Settings.storedTags.album;
	m_Settings.Tags.trackno = m_Settings.storedTags.trackno;
	m_Settings.Tags.year = m_Settings.storedTags.year;
	m_Settings.Tags.url = m_Settings.storedTags.url;
	m_Settings.Tags.genre = m_Settings.storedTags.genre;
}


void CWaveConvert::SaveTags()
//---------------------------
{
	m_Settings.storedTags.artist = m_Settings.Tags.artist;
	m_Settings.storedTags.album = m_Settings.Tags.album;
	m_Settings.storedTags.trackno = m_Settings.Tags.trackno;
	m_Settings.storedTags.year = m_Settings.Tags.year;
	m_Settings.storedTags.url = m_Settings.Tags.url;
	m_Settings.storedTags.genre = m_Settings.Tags.genre;
}


void CWaveConvert::FillTags()
//---------------------------
{
	Encoder::Settings &encSettings = m_Settings.GetEncoderSettings();

	const bool canTags = encTraits->canTags;

	DWORD_PTR dwFormat = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());
	Encoder::Mode mode = (Encoder::Mode)((dwFormat >> 24) & 0xff);

	CheckDlgButton(IDC_CHECK3, encTraits->canCues?encSettings.Cues?TRUE:FALSE:FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK3), encTraits->canCues?TRUE:FALSE);

	CheckDlgButton(IDC_CHECK7, canTags?encSettings.Tags?TRUE:FALSE:FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK7), canTags?TRUE:FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_COMBO3), canTags?TRUE:FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT11), canTags?TRUE:FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT6), canTags?TRUE:FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT7), canTags?TRUE:FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT8), canTags?TRUE:FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT9), canTags?TRUE:FALSE);

	if((encTraits->modesWithFixedGenres & mode) && !encTraits->genres.empty())
	{
		m_EditGenre.ShowWindow(SW_HIDE);
		m_CbnGenre.ShowWindow(SW_SHOW);
		m_EditGenre.Clear();
		m_CbnGenre.ResetContent();
		m_CbnGenre.AddString(_T(""));
		for(std::vector<mpt::ustring>::const_iterator genre = encTraits->genres.begin(); genre != encTraits->genres.end(); ++genre)
		{
			m_CbnGenre.AddString(mpt::ToCString(*genre));
		}
	} else
	{
		m_CbnGenre.ShowWindow(SW_HIDE);
		m_EditGenre.ShowWindow(SW_SHOW);
		m_CbnGenre.ResetContent();
		m_EditGenre.Clear();
	}

}


void CWaveConvert::OnShowEncoderInfo()
//------------------------------------
{
	mpt::ustring info;
	info += MPT_USTRING("Format: ");
	info += encTraits->fileDescription;
	info += MPT_USTRING("\r\n");
	info += MPT_USTRING("Encoder: ");
	info += encTraits->encoderName;
	info += MPT_USTRING("\r\n");
	info += mpt::String::Replace(encTraits->description, MPT_USTRING("\n"), MPT_USTRING("\r\n"));
	Reporting::Information(info, "Encoder Information");
}


void CWaveConvert::FillFileTypes()
//--------------------------------
{
	m_CbnFileType.ResetContent();
	int sel = 0;
	for(std::size_t i = 0; i < m_Settings.EncoderFactories.size(); ++i)
	{
		const Encoder::Traits &encTraits = m_Settings.EncoderFactories[i]->GetTraits();
		int ndx = m_CbnFileType.AddString(mpt::ToCString(encTraits.fileShortDescription));
		m_CbnFileType.SetItemData(ndx, i);
		if(m_Settings.EncoderIndex == i)
		{
			sel = ndx;
		}
	}
	m_CbnFileType.SetCurSel(sel);
}


void CWaveConvert::FillSamplerates()
//----------------------------------
{
	Encoder::Settings &encSettings = m_Settings.GetEncoderSettings();
	m_CbnSampleRate.CComboBox::ResetContent();
	int sel = -1;
	if(TrackerSettings::Instance().ExportDefaultToSoundcardSamplerate)
	{
		for(std::vector<uint32>::const_iterator it = encTraits->samplerates.begin(); it != encTraits->samplerates.end(); ++it)
		{
			uint32 samplerate = *it;
			if(samplerate == TrackerSettings::Instance().MixerSamplerate)
			{
				encSettings.Samplerate = samplerate;
			}
		}
	}
	for(std::vector<uint32>::const_iterator it = encTraits->samplerates.begin(); it != encTraits->samplerates.end(); ++it)
	{
		uint32 samplerate = *it;
		int ndx = m_CbnSampleRate.AddString(mpt::ToCString(mpt::String::Print(MPT_USTRING("%1 Hz"), samplerate)));
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
//-------------------------------
{
	Encoder::Settings &encSettings = m_Settings.GetEncoderSettings();
	m_CbnChannels.CComboBox::ResetContent();
	int sel = 0;
	for(int channels = 4; channels >= 1; channels /= 2)
	{
		if(channels > encTraits->maxChannels)
		{
			continue;
		}
		if(IsDlgButtonChecked(IDC_RADIO4) != BST_UNCHECKED)
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
//------------------------------
{
	Encoder::Settings &encSettings = m_Settings.GetEncoderSettings();
	m_CbnSampleFormat.CComboBox::ResetContent();
	int sel = -1;
	DWORD_PTR dwSamplerate = m_CbnSampleRate.GetItemData(m_CbnSampleRate.GetCurSel());
	LONG_PTR nChannels = m_CbnChannels.GetItemData(m_CbnChannels.GetCurSel());
	if(encTraits->modes & Encoder::ModeQuality)
	{
		for(int quality = 100; quality >= 0; quality -= 10)
		{
			int ndx = m_CbnSampleFormat.AddString(mpt::ToCString(m_Settings.GetEncoderFactory()->DescribeQuality(quality * 0.01f)));
			m_CbnSampleFormat.SetItemData(ndx, (Encoder::ModeQuality<<24) | (quality<<0));
			if(encSettings.Mode == Encoder::ModeQuality && Util::Round<int>(encSettings.Quality*100.0f) == quality)
			{
				sel = ndx;
			}
		}
	}
	if(encTraits->modes & Encoder::ModeVBR)
	{
		for(int bitrate = static_cast<int>(encTraits->bitrates.size()-1); bitrate >= 0; --bitrate)
		{
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
			int ndx = m_CbnSampleFormat.AddString(mpt::ToCString(m_Settings.GetEncoderFactory()->DescribeBitrateCBR(encTraits->bitrates[bitrate])));
			m_CbnSampleFormat.SetItemData(ndx, (Encoder::ModeCBR<<24) | (encTraits->bitrates[bitrate]<<0));
			if(encSettings.Mode == Encoder::ModeCBR && static_cast<int>(encSettings.Bitrate) == encTraits->bitrates[bitrate])
			{
				sel = ndx;
			}
		}
	}
	if(encTraits->modes & Encoder::ModeEnumerated)
	{
		for(std::size_t i = 0; i < encTraits->formats.size(); ++i)
		{
			const Encoder::Format &format = encTraits->formats[i];
			if(format.Samplerate != (int)dwSamplerate || format.Channels != nChannels)
			{
				continue;
			}
			if(i > 0xffff)
			{
				// too may formats
				break;
			}
			int ndx = m_CbnSampleFormat.AddString(mpt::ToCString(format.Description));
			m_CbnSampleFormat.SetItemData(ndx, i & 0xffff);
			if(encSettings.Mode & Encoder::ModeEnumerated && (int)i == encSettings.Format)
			{
				sel = ndx;
			}
		}
		if(sel == -1 && encSettings.Mode & Encoder::ModeEnumerated && encTraits->defaultBitrate != 0)
		{
			// select enumerated format based on bitrate
			for(int ndx = 0; ndx < m_CbnSampleFormat.GetCount(); ++ndx)
			{
				int i = (int)((m_CbnSampleFormat.GetItemData(ndx) >> 0) & 0xffff);
				const Encoder::Format &format = encTraits->formats[i];
				if(format.Bitrate != 0 && encSettings.Bitrate == format.Bitrate)
				{
					sel = ndx;
				}
			}
			if(sel == -1)
			{
				// select enumerated format based on default bitrate
				for(int ndx = 0; ndx < m_CbnSampleFormat.GetCount(); ++ndx)
				{
					int i = (int)((m_CbnSampleFormat.GetItemData(ndx) >> 0) & 0xffff);
					const Encoder::Format &format = encTraits->formats[i];
					if(format.Bitrate == encTraits->defaultBitrate)
					{
						sel = ndx;
					}
				}
			}
		}
		if(sel == -1 && encSettings.Mode & Encoder::ModeEnumerated && encTraits->defaultBitrate == 0)
		{
			// select enumerated format based on sampleformat
			for(int ndx = 0; ndx < m_CbnSampleFormat.GetCount(); ++ndx)
			{
				int i = (int)((m_CbnSampleFormat.GetItemData(ndx) >> 0) & 0xffff);
				const Encoder::Format &format = encTraits->formats[i];
				int32 currentFormat = encSettings.Format;
				if(encSettings.Format < 0 || (std::size_t)currentFormat >= encTraits->formats.size())
				{ // out of bounds
					continue;
				}
				if(format.Sampleformat != SampleFormatInvalid && encTraits->formats[currentFormat].Sampleformat == format.Sampleformat)
				{
					sel = ndx;
				}
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
//-----------------------------
{
	Encoder::Settings &encSettings = m_Settings.GetEncoderSettings();
	m_CbnDither.CComboBox::ResetContent();
	int format = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel()) & 0xffff;
	if((encTraits->modes & Encoder::ModeEnumerated) && encTraits->formats[format].Sampleformat != SampleFormatInvalid && encTraits->formats[format].Sampleformat != SampleFormatFloat32)
	{
		m_CbnDither.EnableWindow(TRUE);
		for(int dither = 0; dither < NumDitherModes; ++dither)
		{
			int ndx = m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName((DitherMode)dither) + MPT_USTRING(" dither")));
			m_CbnDither.SetItemData(ndx, dither);
		}
	} else
	{
		m_CbnDither.EnableWindow(FALSE);
		for(int dither = 0; dither < NumDitherModes; ++dither)
		{
			int ndx = m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName(DitherNone) + MPT_USTRING(" dither")));
			m_CbnDither.SetItemData(ndx, dither);
		}
	}
	m_CbnDither.SetCurSel(encSettings.Dither);
}


void CWaveConvert::OnFileTypeChanged()
//------------------------------------
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
//--------------------------------------
{
	SaveEncoderSettings();
	FillFormats();
	FillDither();
}


void CWaveConvert::OnChannelsChanged()
//------------------------------------
{
	SaveEncoderSettings();
	FillFormats();
	FillDither();
}


void CWaveConvert::OnDitherChanged()
//----------------------------------
{
	SaveEncoderSettings();
}


void CWaveConvert::OnFormatChanged()
//----------------------------------
{
	SaveEncoderSettings();
	FillDither();
	FillTags();
}


void CWaveConvert::UpdateDialog()
//-------------------------------
{
	CheckDlgButton(IDC_CHECK1, (m_dwFileLimit) ? BST_CHECKED : 0);
	CheckDlgButton(IDC_CHECK2, (m_dwSongLimit) ? BST_CHECKED : 0);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT1), (m_dwFileLimit) ? TRUE : FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT2), (m_dwSongLimit) ? TRUE : FALSE);

	// Repeat / selection play
	BOOL bSel = (IsDlgButtonChecked(IDC_RADIO2) != BST_UNCHECKED) ? TRUE : FALSE;
	GetDlgItem(IDC_EDIT3)->EnableWindow(bSel);
	GetDlgItem(IDC_EDIT4)->EnableWindow(bSel);
	GetDlgItem(IDC_EDIT5)->EnableWindow(!bSel);
	m_SpinLoopCount.EnableWindow(!bSel);

	// No free slots => Cannot do instrument- or channel-based export to sample
	BOOL canDoMultiExport = (bSel /* normal export */ || m_CbnSampleSlot.GetItemData(0) == 0 /* "free slot" is in list */) ? TRUE : FALSE;
	GetDlgItem(IDC_CHECK4)->EnableWindow(canDoMultiExport);
	GetDlgItem(IDC_CHECK6)->EnableWindow(canDoMultiExport);
}


void CWaveConvert::OnExportModeChanged()
//--------------------------------------
{
	SaveEncoderSettings();
	bool sampleExport = (IsDlgButtonChecked(IDC_RADIO4) != BST_UNCHECKED);
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
//--------------------------------------
{
	CheckRadioButton(IDC_RADIO3, IDC_RADIO4, IDC_RADIO4);
	// When choosing a specific sample slot, we cannot use per-channel or per-instrument export
	int sel = m_CbnSampleSlot.GetCurSel();
	if(sel >= 0 && m_CbnSampleSlot.GetItemData(sel) > 0)
	{
		CheckDlgButton(IDC_CHECK4, BST_UNCHECKED);
		CheckDlgButton(IDC_CHECK6, BST_UNCHECKED);
	}
	UpdateDialog();
}


void CWaveConvert::OnCheckSizeLimit()
//-----------------------------------
{
	if (IsDlgButtonChecked(IDC_CHECK1))
	{
		m_dwFileLimit = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
		if (!m_dwFileLimit)
		{
			m_dwFileLimit = 1000;
			SetDlgItemText(IDC_EDIT1, _T("1000"));
		}
	} else m_dwFileLimit = 0;
	UpdateDialog();
}


void CWaveConvert::OnPlayerOptions()
//----------------------------------
{
	CPropertySheet dlg(_T("Mixer Settings"), this);
	COptionsMixer mixerpage;
	dlg.AddPage(&mixerpage);
#if !defined(NO_REVERB) || !defined(NO_DSP) || !defined(NO_EQ) || !defined(NO_AGC) || !defined(NO_EQ)
	COptionsPlayer dsppage;
	dlg.AddPage(&dsppage);
#endif
	dlg.DoModal();
}


void CWaveConvert::OnCheckTimeLimit()
//-----------------------------------
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
//-------------------------------------
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
//-----------------------------------
{
	if(IsDlgButtonChecked(IDC_CHECK6) != BST_UNCHECKED)
	{
		CheckDlgButton(IDC_CHECK4, BST_UNCHECKED);
		m_CbnSampleSlot.SetCurSel(0);
	}
	UpdateDialog();
}


void CWaveConvert::OnOK()
//-----------------------
{
	if (m_dwFileLimit) m_dwFileLimit = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
	if (m_dwSongLimit) m_dwSongLimit = GetDlgItemInt(IDC_EDIT2, NULL, FALSE);

	const bool selection = IsDlgButtonChecked(IDC_RADIO2) != BST_UNCHECKED;
	if(selection)
	{
		// Play selection
		m_Settings.minOrder = static_cast<ORDERINDEX>(GetDlgItemInt(IDC_EDIT3, NULL, FALSE));
		m_Settings.maxOrder = static_cast<ORDERINDEX>(GetDlgItemInt(IDC_EDIT4, NULL, FALSE));
	}
	if(!selection || m_Settings.maxOrder < m_Settings.minOrder)
	{
		m_Settings.minOrder = m_Settings.maxOrder = ORDERINDEX_INVALID;
	}

	m_Settings.repeatCount = static_cast<uint16>(GetDlgItemInt(IDC_EDIT5, NULL, FALSE));
	m_Settings.normalize = IsDlgButtonChecked(IDC_CHECK5) != BST_UNCHECKED;
	m_Settings.silencePlugBuffers = IsDlgButtonChecked(IDC_RENDERSILENCE) != BST_UNCHECKED;
	m_Settings.outputToSample = IsDlgButtonChecked(IDC_RADIO4) != BST_UNCHECKED;
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

	Encoder::Settings &encSettings = m_Settings.GetEncoderSettings();

	m_Settings.Tags = FileTags();

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
		if(m_Settings.Tags.year == MPT_USTRING("0"))
		{
			m_Settings.Tags.year = mpt::ustring();
		}

		if(!m_SndFile.m_songMessage.empty())
		{
			m_Settings.Tags.comments = mpt::ToUnicode(mpt::CharsetLocale, m_SndFile.m_songMessage.GetFormatted(SongMessage::leLF));
		}

		m_Settings.Tags.bpm = mpt::ToUString(m_SndFile.GetCurrentBPM());

		SaveTags();

	}

	CDialog::OnOK();

}


void CWaveConvert::SaveEncoderSettings()
//--------------------------------------
{

	Encoder::Settings &encSettings = m_Settings.GetEncoderSettings();

	encSettings.Samplerate = static_cast<uint32>(m_CbnSampleRate.GetItemData(m_CbnSampleRate.GetCurSel()));
	encSettings.Channels = static_cast<uint16>(m_CbnChannels.GetItemData(m_CbnChannels.GetCurSel()));
	DWORD_PTR dwFormat = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());

	if(encTraits->modes & Encoder::ModeEnumerated)
	{
		int format = (int)((dwFormat >> 0) & 0xffff);
		if(encTraits->formats[format].Sampleformat == SampleFormatInvalid)
		{
			m_Settings.FinalSampleFormat = SampleFormatFloat32;
		} else
		{
			m_Settings.FinalSampleFormat = encTraits->formats[format].Sampleformat;
		}
		encSettings.Dither = static_cast<int>(m_CbnDither.GetItemData(m_CbnDither.GetCurSel()));
		encSettings.Format = format;
		encSettings.Mode = Encoder::ModeEnumerated;
		encSettings.Bitrate = encTraits->formats[format].Bitrate != 0 ? encTraits->formats[format].Bitrate : encTraits->defaultBitrate;
		encSettings.Quality = encTraits->defaultQuality;
	} else
	{
		m_Settings.FinalSampleFormat = SampleFormatFloat32;
		encSettings.Dither = static_cast<int>(m_CbnDither.GetItemData(m_CbnDither.GetCurSel()));
		Encoder::Mode mode = (Encoder::Mode)((dwFormat >> 24) & 0xff);
		int quality = (int)((dwFormat >> 0) & 0xff);
		int bitrate = (int)((dwFormat >> 0) & 0xffff);
		encSettings.Mode = mode;
		encSettings.Bitrate = bitrate;
		encSettings.Quality = static_cast<float>(quality) * 0.01f;
		encSettings.Format = -1;
	}
	
	encSettings.Cues = IsDlgButtonChecked(IDC_CHECK3) ? true : false;

	encSettings.Tags = IsDlgButtonChecked(IDC_CHECK7) ? true : false;

}


std::size_t CWaveConvertSettings::FindEncoder(const mpt::ustring &name) const
//---------------------------------------------------------------------------
{
	for(std::size_t i = 0; i < EncoderFactories.size(); ++i)
	{
		if(EncoderFactories[i]->GetTraits().encoderSettingsName == name)
		{
			return i;
		}
	}
	return EncoderFactories.size() > 2 ? 2 : 0;
}


void CWaveConvertSettings::SelectEncoder(std::size_t index)
//---------------------------------------------------------
{
	ASSERT(!EncoderFactories.empty());
	ASSERT(index < EncoderFactories.size());
	EncoderIndex = index;
	EncoderName = EncoderFactories[EncoderIndex]->GetTraits().encoderSettingsName;
}


EncoderFactoryBase *CWaveConvertSettings::GetEncoderFactory() const
//-----------------------------------------------------------------
{
	ASSERT(!EncoderFactories.empty());
	return EncoderFactories[EncoderIndex];
}


const Encoder::Traits *CWaveConvertSettings::GetTraits() const
//------------------------------------------------------------
{
	ASSERT(!EncoderFactories.empty());
	return &EncoderFactories[EncoderIndex]->GetTraits();
}


Encoder::Settings &CWaveConvertSettings::GetEncoderSettings() const
//-----------------------------------------------------------------
{
	ASSERT(!EncoderSettings.empty());
	return *(EncoderSettings[EncoderIndex]);
}


CWaveConvertSettings::CWaveConvertSettings(SettingsContainer &conf, const std::vector<EncoderFactoryBase*> &encFactories)
//-----------------------------------------------------------------------------------------------------------------------
	: EncoderFactories(encFactories)
	, EncoderName(conf, "Export", encFactories.size() > 2 ? "LossyEncoder" : "LosslessEncoder", MPT_USTRING(""))
	, EncoderIndex(FindEncoder(EncoderName))
	, FinalSampleFormat(SampleFormatInt16)
	, storedTags(conf)
	, repeatCount(0)
	, minOrder(ORDERINDEX_INVALID), maxOrder(ORDERINDEX_INVALID)
	, sampleSlot(0)
	, normalize(false)
	, silencePlugBuffers(false)
	, outputToSample(false)
{
	for(std::size_t i = 0; i < EncoderFactories.size(); ++i)
	{
		const Encoder::Traits &encTraits = EncoderFactories[i]->GetTraits();
		EncoderSettings.push_back(
			MPT_SHARED_PTR<Encoder::Settings>(
				new Encoder::Settings(
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
			)
		);
	}
	SelectEncoder(EncoderIndex);
}


/////////////////////////////////////////////////////////////////////////////////////////
// CDoWaveConvert: save a mod as a wave file

void CDoWaveConvert::Run()
//------------------------
{
	static char buffer[MIXBUFFERSIZE * 4 * 4]; // channels * sizeof(biggestsample)
	static float floatbuffer[MIXBUFFERSIZE * 4]; // channels
	static int mixbuffer[MIXBUFFERSIZE * 4]; // channels

	TCHAR s[80];
	UINT ok = IDOK, pos = 0;
	uint64 ullSamples = 0, ullMaxSamples;

	if(m_lpszFileName.empty())
	{
		EndDialog(IDCANCEL);
		return;
	}

	float normalizePeak = 0.0f;
	const mpt::PathString normalizeFileName = mpt::CreateTempFileName(MPT_PATHSTRING("OpenMPT"));
	mpt::fstream normalizeFile;
	if(m_Settings.normalize)
	{
		// Ensure this temporary file is marked as temporary in the file system, to increase the chance it will never be written to disk
		::CloseHandle(::CreateFileW(normalizeFileName.AsNative().c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL));

		normalizeFile.open(normalizeFileName, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
	}

	mpt::ofstream fileStream(m_lpszFileName, std::ios::binary | std::ios::trunc);

	if(!fileStream)
	{
		Reporting::Error("Could not open file for writing. Is it open in another application?");
		EndDialog(IDCANCEL);
		return;
	}

	Encoder::Settings &encSettings = m_Settings.GetEncoderSettings();
	const uint32 samplerate = encSettings.Samplerate;
	const uint16 channels = encSettings.Channels;

	ASSERT(m_Settings.GetEncoderFactory() && m_Settings.GetEncoderFactory()->IsAvailable());
	IAudioStreamEncoder *fileEnc = m_Settings.GetEncoderFactory()->ConstructStreamEncoder(fileStream);

	// Silence mix buffer of plugins, for plugins that don't clear their reverb buffers and similar stuff when they are reset
#ifndef NO_PLUGINS
	if(m_Settings.silencePlugBuffers)
	{
		SetText(_T("Clearing plugin buffers"));
		for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
		{
			if(m_SndFile.m_MixPlugins[i].pMixPlugin != nullptr)
			{
				// Render up to 20 seconds per plugin
				for(int j = 0; j < 20; j++)
				{
					const float maxVal = m_SndFile.m_MixPlugins[i].pMixPlugin->RenderSilence(samplerate);
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

	Dither dither;
	dither.SetMode((DitherMode)encSettings.Dither.Get());

	m_SndFile.ResetChannels();
	m_SndFile.SetMixerSettings(mixersettings);
	m_SndFile.SetResamplerSettings(TrackerSettings::Instance().GetResamplerSettings());
	m_SndFile.InitPlayer(true);
	if(!m_dwFileLimit) m_dwFileLimit = Util::MaxValueOfType(m_dwFileLimit) >> 10;
	m_dwFileLimit <<= 10;

	fileEnc->SetFormat(encSettings);
	if(encSettings.Tags)
	{
		// Tags must be written before audio data,
		// so that the encoder class could write them before audio data if mandated by the format,
		// otherwise they should just be cached by the encoder.
		fileEnc->WriteMetatags(m_Settings.Tags);
	}

	ullMaxSamples = m_dwFileLimit / (channels * ((m_Settings.FinalSampleFormat.GetBitsPerSample()+7) / 8));
	if (m_dwSongLimit)
	{
		uint64 l = (uint64)m_dwSongLimit * samplerate;
		if (l < ullMaxSamples) ullMaxSamples = l;
	}

	// Calculate maximum samples
	uint64 max = ullMaxSamples;
	uint64 l = static_cast<uint64>(m_SndFile.GetSongTime() + 0.5) * samplerate * std::max<uint64>(1, 1 + m_SndFile.GetRepeatCount());

	// Reset song position tracking
	m_SndFile.ResetPlayPos();
	m_SndFile.m_SongFlags.reset(SONG_PATTERNLOOP);
	ORDERINDEX startOrder = 0;
	if(m_Settings.minOrder != ORDERINDEX_INVALID && m_Settings.maxOrder != ORDERINDEX_INVALID)
	{
		startOrder = m_Settings.minOrder;
		m_SndFile.m_nMaxOrderPosition = m_Settings.maxOrder + 1;
		m_SndFile.SetRepeatCount(0);

		// Weird calculations ahead...
		ORDERINDEX dwOrds = m_SndFile.Order.GetLengthFirstEmpty();
		PATTERNINDEX maxPatterns = m_Settings.maxOrder - m_Settings.minOrder + 1;
		if((maxPatterns < dwOrds) && (dwOrds > 0)) l = (l * maxPatterns) / dwOrds;
	} else
	{
		m_SndFile.SetRepeatCount(std::max(0, m_Settings.repeatCount - 1));
	}
	m_SndFile.SetCurrentOrder(startOrder);
	m_SndFile.GetLength(eAdjust, GetLengthTarget(startOrder, 0));	// adjust playback variables / visited rows vector
	m_SndFile.m_PlayState.m_nCurrentOrder = startOrder;

	if (l < max) max = l;

	SetRange(0, static_cast<uint32>(max >> 14));

	// No pattern cue points yet
	m_SndFile.m_PatternCuePoints.clear();
	m_SndFile.m_PatternCuePoints.reserve(m_SndFile.Order.GetLength());

	CString progressStr;
	if(m_Settings.normalize)
		progressStr =  _T("Rendering ") + caption + _T("... (%umn%02us, %umn%02us remaining)");
	else
		progressStr =  _T("Writing ") + caption + _T("... (%lluKB, %umn%02us, %umn%02us remaining)");

	// Process the conversion

	// For calculating the remaining time
	DWORD dwStartTime = timeGetTime();

	uint64 bytesWritten = 0;

	CMainFrame::GetMainFrame()->PauseMod();
	m_SndFile.m_SongFlags.reset(SONG_STEP | SONG_PATTERNLOOP);
	CMainFrame::GetMainFrame()->InitRenderer(&m_SndFile);

	for (UINT n = 0; ; n++)
	{
		UINT lRead = 0;
		if(m_Settings.normalize || m_Settings.FinalSampleFormat == SampleFormatFloat32)
		{
			lRead = ReadInterleaved(m_SndFile, floatbuffer, MIXBUFFERSIZE, SampleFormatFloat32, dither);
		} else
		{
			lRead = ReadInterleaved(m_SndFile, buffer, MIXBUFFERSIZE, m_Settings.FinalSampleFormat, dither);
		}

		// Process cue points (add base offset), if there are any to process.
		std::vector<PatternCuePoint>::reverse_iterator iter;
		for(iter = m_SndFile.m_PatternCuePoints.rbegin(); iter != m_SndFile.m_PatternCuePoints.rend(); ++iter)
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
			const float *src = floatbuffer;
			while(countSamples--)
			{
				const float val = *src;
				if(val > normalizePeak) normalizePeak = val;
				else if(0.0f - val >= normalizePeak) normalizePeak = 0.0f - val;
				src++;
			}

			normalizeFile.write(reinterpret_cast<const char*>(floatbuffer), lRead * m_SndFile.m_MixerSettings.gnChannels * sizeof(float));
			if(!normalizeFile)
				break;

		} else
		{

			const std::streampos oldPos = fileStream.tellp();
			if(m_Settings.FinalSampleFormat == SampleFormatFloat32)
			{
				fileEnc->WriteInterleaved(lRead, floatbuffer);
			} else
			{
				fileEnc->WriteInterleavedConverted(lRead, buffer);
			}
			const std::streampos newPos = fileStream.tellp();
			bytesWritten += static_cast<uint64>(newPos - oldPos);

			if(bytesWritten >= m_dwFileLimit)
			{
				break;
			} else if(!fileStream)
			{
				break;
			}

		}
		if (ullSamples >= ullMaxSamples)
			break;
		if (!(n % 10))
		{
			DWORD l = (DWORD)(ullSamples / m_SndFile.m_MixerSettings.gdwMixingFreq);

			const DWORD dwCurrentTime = timeGetTime();
			uint32 timeRemaining = 0; // estimated remainig time
			if((ullSamples > 0) && (ullSamples < max))
			{
				timeRemaining = static_cast<uint32>(((dwCurrentTime - dwStartTime) * (max - ullSamples) / ullSamples) / 1000);
			}

			if(m_Settings.normalize)
				_stprintf(s, progressStr, l / 60, l % 60, timeRemaining / 60, timeRemaining % 60u);
			else
				_stprintf(s, progressStr, bytesWritten >> 10, l / 60, l % 60u, timeRemaining / 60, timeRemaining % 60u);
			SetText(s);
		}
		if (static_cast<uint32>(ullSamples >> 14) != pos)
		{
			pos = static_cast<uint32>(ullSamples >> 14);
			SetProgress(pos);
		}
		ProcessMessages();

		if (m_abort)
		{
			ok = IDCANCEL;
			break;
		}
	}

	m_SndFile.m_nMaxOrderPosition = 0;

	CMainFrame::GetMainFrame()->StopRenderer(&m_SndFile);

	if(m_Settings.normalize)
	{
		SetRange(0, 100);

		const float normalizeFactor = (normalizePeak != 0.0f) ? (1.0f / normalizePeak) : 1.0f;

		const uint64 framesTotal = ullSamples;
		int lastPercent = -1;

		normalizeFile.seekp(0);

		uint64 framesProcessed = 0;
		uint64 framesToProcess = framesTotal;
		while(framesToProcess)
		{
			const std::size_t framesChunk = std::min<std::size_t>(mpt::saturate_cast<std::size_t>(framesToProcess), MIXBUFFERSIZE);
			const uint32 samplesChunk = static_cast<uint32>(framesChunk * channels);
			
			normalizeFile.read(reinterpret_cast<char*>(floatbuffer), samplesChunk * sizeof(float));
			if(normalizeFile.gcount() != static_cast<std::streamsize>(samplesChunk * sizeof(float)))
				break;

			for(std::size_t i = 0; i < samplesChunk; ++i)
			{
				floatbuffer[i] *= normalizeFactor;
			}

			const std::streampos oldPos = fileStream.tellp();
			if(m_Settings.FinalSampleFormat == SampleFormatFloat32)
			{
				fileEnc->WriteInterleaved(framesChunk, floatbuffer);
			} else
			{
				// Convert float buffer to mixbuffer format so we can apply dither.
				// This can probably be changed in the future when dither supports floating point input directly.
				FloatToMonoMix(floatbuffer, mixbuffer, samplesChunk, MIXING_SCALEF);
				dither.Process(mixbuffer, framesChunk, channels, m_Settings.FinalSampleFormat.GetBitsPerSample());
				switch(m_Settings.FinalSampleFormat.value)
				{
					case SampleFormatUnsigned8: ConvertInterleavedFixedPointToInterleaved<MIXING_FRACTIONAL_BITS,false>(reinterpret_cast<uint8*>(buffer), mixbuffer, channels, framesChunk); break;
					case SampleFormatInt16:     ConvertInterleavedFixedPointToInterleaved<MIXING_FRACTIONAL_BITS,false>(reinterpret_cast<int16*>(buffer), mixbuffer, channels, framesChunk); break;
					case SampleFormatInt24:     ConvertInterleavedFixedPointToInterleaved<MIXING_FRACTIONAL_BITS,false>(reinterpret_cast<int24*>(buffer), mixbuffer, channels, framesChunk); break;
					case SampleFormatInt32:     ConvertInterleavedFixedPointToInterleaved<MIXING_FRACTIONAL_BITS,false>(reinterpret_cast<int32*>(buffer), mixbuffer, channels, framesChunk); break;
					default: ASSERT(false); break;
				}
				fileEnc->WriteInterleavedConverted(framesChunk, buffer);
			}
			const std::streampos newPos = fileStream.tellp();
			bytesWritten += static_cast<std::size_t>(newPos - oldPos);

			{
				int percent = static_cast<int>(100 * framesProcessed / framesTotal);
				if(percent != lastPercent)
				{
					_stprintf(s, _T("Normalizing... (%d%%)"), percent);
					SetText(s);
					SetProgress(percent);
					lastPercent = percent;
				}
			}

			if(!(framesToProcess % 10))
			{
				ProcessMessages();
			}

			framesProcessed += framesChunk;
			framesToProcess -= framesChunk;
		}

		normalizeFile.flush();
		normalizeFile.close();
		for(int retry=0; retry<10; retry++)
		{
			// stupid virus scanners
			if(DeleteFileW(normalizeFileName.AsNative().c_str()) != EACCES)
			{
				break;
			}
			Sleep(10);
		}

	}

	if(m_SndFile.m_PatternCuePoints.size() > 0)
	{
		if(encSettings.Cues)
		{
			std::vector<PatternCuePoint>::const_iterator iter;
			std::vector<uint64> cues;
			for(iter = m_SndFile.m_PatternCuePoints.begin(); iter != m_SndFile.m_PatternCuePoints.end(); iter++)
			{
				cues.push_back(static_cast<uint32>(iter->offset));
			}
			fileEnc->WriteCues(cues);
		}
		m_SndFile.m_PatternCuePoints.clear();
	}

	fileEnc->Finalize();
	delete fileEnc;
	fileEnc = nullptr;

	fileStream.flush();
	fileStream.close();

	CMainFrame::UpdateAudioParameters(m_SndFile, TRUE);
	EndDialog(ok);
}


OPENMPT_NAMESPACE_END
