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
#include "vstplug.h"
#include "mod2wave.h"
#include "Wav.h"
#include "WAVTools.h"
#include "../common/mptString.h"
#include "../common/version.h"
#include "../soundlib/MixerLoops.h"
#include "../soundlib/Dither.h"
#include "../soundlib/AudioReadTarget.h"

#include "../common/mptFstream.h"

extern const char *gszChnCfgNames[3];

static CSoundFile::samplecount_t ReadInterleaved(CSoundFile &sndFile, void *outputBuffer, CSoundFile::samplecount_t count, SampleFormat sampleFormat, Dither &dither)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	switch(sampleFormat.value)
	{
	case SampleFormatUnsigned8:
		{
			typedef SampleFormatToType<SampleFormatUnsigned8>::type Tsample;
			AudioReadTargetBuffer<Tsample> target(dither, reinterpret_cast<Tsample*>(outputBuffer), nullptr);
			return sndFile.Read(count, target);
		}
		break;
	case SampleFormatInt16:
		{
			typedef SampleFormatToType<SampleFormatInt16>::type Tsample;
			AudioReadTargetBuffer<Tsample> target(dither, reinterpret_cast<Tsample*>(outputBuffer), nullptr);
			return sndFile.Read(count, target);
		}
		break;
	case SampleFormatInt24:
		{
			typedef SampleFormatToType<SampleFormatInt24>::type Tsample;
			AudioReadTargetBuffer<Tsample> target(dither, reinterpret_cast<Tsample*>(outputBuffer), nullptr);
			return sndFile.Read(count, target);
		}
		break;
	case SampleFormatInt32:
		{
			typedef SampleFormatToType<SampleFormatInt32>::type Tsample;
			AudioReadTargetBuffer<Tsample> target(dither, reinterpret_cast<Tsample*>(outputBuffer), nullptr);
			return sndFile.Read(count, target);
		}
		break;
	case SampleFormatFloat32:
		{
			typedef SampleFormatToType<SampleFormatFloat32>::type Tsample;
			AudioReadTargetBuffer<Tsample> target(dither, reinterpret_cast<Tsample*>(outputBuffer), nullptr);
			return sndFile.Read(count, target);
		}
		break;
	}
	return 0;
}

///////////////////////////////////////////////////
// CWaveConvert - setup for converting a wave file

BEGIN_MESSAGE_MAP(CWaveConvert, CDialog)
	ON_COMMAND(IDC_CHECK1,			OnCheck1)
	ON_COMMAND(IDC_CHECK2,			OnCheck2)
	ON_COMMAND(IDC_CHECK4,			OnCheckChannelMode)
	ON_COMMAND(IDC_CHECK6,			OnCheckInstrMode)
	ON_COMMAND(IDC_RADIO1,			UpdateDialog)
	ON_COMMAND(IDC_RADIO2,			UpdateDialog)
	ON_COMMAND(IDC_PLAYEROPTIONS,	OnPlayerOptions)
	ON_COMMAND(IDC_BUTTON1,			OnShowEncoderInfo)
	ON_CBN_SELCHANGE(IDC_COMBO5,	OnFileTypeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnSamplerateChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,	OnChannelsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnFormatChanged)
END_MESSAGE_MAP()


CWaveConvert::CWaveConvert(CWnd *parent, ORDERINDEX minOrder, ORDERINDEX maxOrder, ORDERINDEX numOrders, CSoundFile *sndfile, std::size_t defaultEncoder, const std::vector<EncoderFactoryBase*> &encFactories)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	: CDialog(IDD_WAVECONVERT, parent)
	, m_Settings(defaultEncoder, encFactories)
{
	ASSERT(!encFactories.empty());
	encTraits = m_Settings.GetTraits();
	m_pSndFile = sndfile;
	m_bGivePlugsIdleTime = false;
	m_bHighQuality = false;
	m_bSelectPlay = false;
	if(minOrder != ORDERINDEX_INVALID && maxOrder != ORDERINDEX_INVALID)
	{
		// render selection
		m_nMinOrder = minOrder;
		m_nMaxOrder = maxOrder;
		m_bSelectPlay = true;
	} else
	{
		m_nMinOrder = m_nMaxOrder = 0;
	}
	loopCount = 1;
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
	DDX_Control(pDX, IDC_COMBO2,	m_CbnSampleFormat);
	DDX_Control(pDX, IDC_SPIN3,		m_SpinMinOrder);
	DDX_Control(pDX, IDC_SPIN4,		m_SpinMaxOrder);
	DDX_Control(pDX, IDC_SPIN5,		m_SpinLoopCount);

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
	CheckRadioButton(IDC_RADIO1, IDC_RADIO2, m_bSelectPlay ? IDC_RADIO2 : IDC_RADIO1);

	CheckDlgButton(IDC_CHECK5, MF_UNCHECKED);	// Normalize
	CheckDlgButton(IDC_CHECK3, MF_CHECKED);	// Cue points

	CheckDlgButton(IDC_CHECK4, MF_UNCHECKED);
	CheckDlgButton(IDC_CHECK6, MF_UNCHECKED);

	SetDlgItemInt(IDC_EDIT3, m_nMinOrder);
	m_SpinMinOrder.SetRange(0, m_nNumOrders);
	SetDlgItemInt(IDC_EDIT4, m_nMaxOrder);
	m_SpinMaxOrder.SetRange(0, m_nNumOrders);

	SetDlgItemInt(IDC_EDIT5, loopCount, FALSE);
	m_SpinLoopCount.SetRange(1, int16_max);

	FillFileTypes();
	FillSamplerates();
	FillChannels();
	FillFormats();

	SetDlgItemText(IDC_EDIT11, m_pSndFile->GetTitle().c_str());
	m_EditYear.SetLimitText(4);
	CTime tTime = CTime::GetCurrentTime();
	m_EditYear.SetWindowText(tTime.Format("%Y"));

	FillTags();

	// Plugin quirk options are only available if there are any plugins loaded.
	GetDlgItem(IDC_GIVEPLUGSIDLETIME)->EnableWindow(FALSE);
	GetDlgItem(IDC_RENDERSILENCE)->EnableWindow(FALSE);
	for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
	{
		if(m_pSndFile->m_MixPlugins[i].pMixPlugin != nullptr)
		{
			GetDlgItem(IDC_GIVEPLUGSIDLETIME)->EnableWindow(TRUE);
			GetDlgItem(IDC_RENDERSILENCE)->EnableWindow(TRUE);
			break;
		}
	}

	UpdateDialog();
	return TRUE;
}


void CWaveConvert::FillTags()
//---------------------------
{

	const bool canTags = encTraits->canTags;

	DWORD dwFormat = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());
	Encoder::Mode mode = (Encoder::Mode)((dwFormat >> 24) & 0xff);

	CheckDlgButton(IDC_CHECK3, encTraits->canCues?m_Settings.EncoderSettings.Cues?TRUE:FALSE:FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK3), encTraits->canCues?TRUE:FALSE);

	CheckDlgButton(IDC_CHECK7, canTags?m_Settings.EncoderSettings.Tags?TRUE:FALSE:FALSE);
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
		m_CbnGenre.AddString("");
		for(std::vector<std::string>::const_iterator genre = encTraits->genres.begin(); genre != encTraits->genres.end(); ++genre)
		{
			m_CbnGenre.AddString((*genre).c_str());
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
	std::string info;
	info += "Format: ";
	info += encTraits->fileDescription;
	info += "\r\n";
	info += "Encoder: ";
	info += encTraits->encoderName;
	info += "\r\n";
	info += mpt::String::Replace(encTraits->description, "\n", "\r\n");
	Reporting::Information(info.c_str(), "Encoder Information");
}


void CWaveConvert::FillFileTypes()
//--------------------------------
{
	m_CbnFileType.ResetContent();
	int sel = 0;
	for(std::size_t i = 0; i < m_Settings.EncoderFactories.size(); ++i)
	{
		const Encoder::Traits &encTraits = m_Settings.EncoderFactories[i]->GetTraits();
		int ndx = m_CbnFileType.AddString(encTraits.fileShortDescription.c_str());
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
	m_CbnSampleRate.CComboBox::ResetContent();
	int sel = 0;
	for(std::vector<uint32>::const_iterator it = encTraits->samplerates.begin(); it != encTraits->samplerates.end(); ++it)
	{
		uint32 samplerate = *it;
		int ndx = m_CbnSampleRate.AddString(mpt::String::Format("%d Hz", samplerate).c_str());
		m_CbnSampleRate.SetItemData(ndx, samplerate);
		if(samplerate == m_Settings.EncoderSettings.Samplerate)
		{
			sel = ndx;
		}
	}
	m_CbnSampleRate.SetCurSel(sel);
}


void CWaveConvert::FillChannels()
//-------------------------------
{
	m_CbnChannels.CComboBox::ResetContent();
	int sel = 0;
	for(int channels = 4; channels >= 1; channels /= 2)
	{
		if(channels > encTraits->maxChannels)
		{
			continue;
		}
		int ndx = m_CbnChannels.AddString(gszChnCfgNames[(channels+2)/2-1]);
		m_CbnChannels.SetItemData(ndx, channels);
		if(channels == m_Settings.EncoderSettings.Channels)
		{
			sel = ndx;
		}
	}
	m_CbnChannels.SetCurSel(sel);
}


void CWaveConvert::FillFormats()
//------------------------------
{
	m_CbnSampleFormat.CComboBox::ResetContent();
	int sel = 0;
	DWORD dwSamplerate = m_CbnSampleRate.GetItemData(m_CbnSampleRate.GetCurSel());
	int nChannels = m_CbnChannels.GetItemData(m_CbnChannels.GetCurSel());
	if(encTraits->modes & Encoder::ModeQuality)
	{
		for(int quality = 100; quality >= 0; quality -= 10)
		{
			int ndx = m_CbnSampleFormat.AddString(m_Settings.GetEncoderFactory()->DescribeQuality(quality * 0.01f).c_str());
			m_CbnSampleFormat.SetItemData(ndx, (Encoder::ModeQuality<<24) | (quality<<0));
			if(m_Settings.EncoderSettings.Mode == Encoder::ModeQuality && Util::Round<int>(m_Settings.EncoderSettings.Quality*100.0f) == quality)
			{
				sel = ndx;
			}
		}
	}
	if(encTraits->modes & Encoder::ModeVBR)
	{
		for(int bitrate = encTraits->bitrates.size()-1; bitrate >= 0; --bitrate)
		{
			int ndx = m_CbnSampleFormat.AddString(m_Settings.GetEncoderFactory()->DescribeBitrateVBR(encTraits->bitrates[bitrate]).c_str());
			m_CbnSampleFormat.SetItemData(ndx, (Encoder::ModeVBR<<24) | (encTraits->bitrates[bitrate]<<0));
			if(m_Settings.EncoderSettings.Mode == Encoder::ModeVBR && static_cast<int>(m_Settings.EncoderSettings.Bitrate) == encTraits->bitrates[bitrate])
			{
				sel = ndx;
			}
		}
	}
	if(encTraits->modes & Encoder::ModeABR)
	{
		for(int bitrate = encTraits->bitrates.size()-1; bitrate >= 0; --bitrate)
		{
			int ndx = m_CbnSampleFormat.AddString(m_Settings.GetEncoderFactory()->DescribeBitrateABR(encTraits->bitrates[bitrate]).c_str());
			m_CbnSampleFormat.SetItemData(ndx, (Encoder::ModeABR<<24) | (encTraits->bitrates[bitrate]<<0));
			if(m_Settings.EncoderSettings.Mode == Encoder::ModeABR && static_cast<int>(m_Settings.EncoderSettings.Bitrate) == encTraits->bitrates[bitrate])
			{
				sel = ndx;
			}
		}
	}
	if(encTraits->modes & Encoder::ModeCBR)
	{
		for(int bitrate = encTraits->bitrates.size()-1; bitrate >= 0; --bitrate)
		{
			int ndx = m_CbnSampleFormat.AddString(m_Settings.GetEncoderFactory()->DescribeBitrateCBR(encTraits->bitrates[bitrate]).c_str());
			m_CbnSampleFormat.SetItemData(ndx, (Encoder::ModeCBR<<24) | (encTraits->bitrates[bitrate]<<0));
			if(m_Settings.EncoderSettings.Mode == Encoder::ModeCBR && static_cast<int>(m_Settings.EncoderSettings.Bitrate) == encTraits->bitrates[bitrate])
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
			int ndx = m_CbnSampleFormat.AddString(format.Description.c_str());
			m_CbnSampleFormat.SetItemData(ndx, i & 0xffff);
			if(m_Settings.EncoderSettings.Mode & Encoder::ModeEnumerated && (int)i == encTraits->defaultFormat)
			{
				sel = ndx;
			} else if(m_Settings.EncoderSettings.Mode & Encoder::ModeEnumerated && format.Bitrate > 0 && m_Settings.EncoderSettings.Bitrate == format.Bitrate)
			{
				sel = ndx;
			}
		}
	}
	m_CbnSampleFormat.SetCurSel(sel);
}


void CWaveConvert::OnFileTypeChanged()
//------------------------------------
{
	DWORD dwFileType = m_CbnFileType.GetItemData(m_CbnFileType.GetCurSel());
	m_Settings.SelectEncoder(dwFileType);
	encTraits = m_Settings.GetTraits();
	FillSamplerates();
	FillChannels();
	FillFormats();
	FillTags();
}


void CWaveConvert::OnSamplerateChanged()
//--------------------------------------
{
	//DWORD dwSamplerate = m_CbnSampleRate.GetItemData(m_CbnSampleRate.GetCurSel());
	FillFormats();
}


void CWaveConvert::OnChannelsChanged()
//------------------------------------
{
	//UINT nChannels = m_CbnChannels.GetItemData(m_CbnChannels.GetCurSel());
	FillFormats();
}


void CWaveConvert::OnFormatChanged()
//----------------------------------
{
	//DWORD dwFormat = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());
	FillTags();
}


void CWaveConvert::UpdateDialog()
//-------------------------------
{
	CheckDlgButton(IDC_CHECK1, (m_dwFileLimit) ? MF_CHECKED : 0);
	CheckDlgButton(IDC_CHECK2, (m_dwSongLimit) ? MF_CHECKED : 0);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT1), (m_dwFileLimit) ? TRUE : FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT2), (m_dwSongLimit) ? TRUE : FALSE);
	BOOL bSel = IsDlgButtonChecked(IDC_RADIO2) ? TRUE : FALSE;
	GetDlgItem(IDC_EDIT3)->EnableWindow(bSel);
	GetDlgItem(IDC_EDIT4)->EnableWindow(bSel);
	GetDlgItem(IDC_EDIT5)->EnableWindow(!bSel);
	m_SpinLoopCount.EnableWindow(!bSel);
}


void CWaveConvert::OnCheck1()
//---------------------------
{
	if (IsDlgButtonChecked(IDC_CHECK1))
	{
		m_dwFileLimit = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
		if (!m_dwFileLimit)
		{
			m_dwFileLimit = 1000;
			SetDlgItemText(IDC_EDIT1, "1000");
		}
	} else m_dwFileLimit = 0;
	UpdateDialog();
}


void CWaveConvert::OnPlayerOptions()
//----------------------------------
{
	CMainFrame::m_nLastOptionsPage = 2;
	CMainFrame::GetMainFrame()->OnViewOptions();
}


void CWaveConvert::OnCheck2()
//---------------------------
{
	if (IsDlgButtonChecked(IDC_CHECK2))
	{
		m_dwSongLimit = GetDlgItemInt(IDC_EDIT2, NULL, FALSE);
		if (!m_dwSongLimit)
		{
			m_dwSongLimit = 600;
			SetDlgItemText(IDC_EDIT2, "600");
		}
	} else m_dwSongLimit = 0;
	UpdateDialog();
}


// Channel render is mutually exclusive with instrument render
void CWaveConvert::OnCheckChannelMode()
//-------------------------------------
{
	CheckDlgButton(IDC_CHECK6, MF_UNCHECKED);
}


// Channel render is mutually exclusive with instrument render
void CWaveConvert::OnCheckInstrMode()
//-----------------------------------
{
	CheckDlgButton(IDC_CHECK4, MF_UNCHECKED);
}


void CWaveConvert::OnOK()
//-----------------------
{
	if (m_dwFileLimit) m_dwFileLimit = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
	if (m_dwSongLimit) m_dwSongLimit = GetDlgItemInt(IDC_EDIT2, NULL, FALSE);
	m_bSelectPlay = IsDlgButtonChecked(IDC_RADIO2) != BST_UNCHECKED;
	m_nMinOrder = static_cast<ORDERINDEX>(GetDlgItemInt(IDC_EDIT3, NULL, FALSE));
	m_nMaxOrder = static_cast<ORDERINDEX>(GetDlgItemInt(IDC_EDIT4, NULL, FALSE));
	loopCount = static_cast<uint16>(GetDlgItemInt(IDC_EDIT5, NULL, FALSE));
	if (m_nMaxOrder < m_nMinOrder) m_bSelectPlay = false;
	m_Settings.Normalize = IsDlgButtonChecked(IDC_CHECK5) != BST_UNCHECKED;
	m_Settings.SilencePlugBuffers = IsDlgButtonChecked(IDC_RENDERSILENCE) != BST_UNCHECKED;
	m_bGivePlugsIdleTime = IsDlgButtonChecked(IDC_GIVEPLUGSIDLETIME) != BST_UNCHECKED;
	if (m_bGivePlugsIdleTime)
	{
		if (Reporting::Confirm("You only need slow render if you are experiencing dropped notes with a Kontakt based sampler with Direct-From-Disk enabled.\nIt will make rendering *very* slow.\n\nAre you sure you want to enable slow render?",
			"Really enable slow render?") == cnfNo)
		{
			CheckDlgButton(IDC_GIVEPLUGSIDLETIME, BST_UNCHECKED);
			return;
		}
	}

	m_bChannelMode = IsDlgButtonChecked(IDC_CHECK4) != BST_UNCHECKED;
	m_bInstrumentMode= IsDlgButtonChecked(IDC_CHECK6) != BST_UNCHECKED;

	m_Settings.EncoderSettings.Samplerate = m_CbnSampleRate.GetItemData(m_CbnSampleRate.GetCurSel());
	m_Settings.EncoderSettings.Channels = (uint16)m_CbnChannels.GetItemData(m_CbnChannels.GetCurSel());
	DWORD dwFormat = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());

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
		m_Settings.EncoderSettings.Format = format;
		m_Settings.EncoderSettings.Mode = encTraits->defaultMode;
		m_Settings.EncoderSettings.Bitrate = encTraits->defaultBitrate;
		m_Settings.EncoderSettings.Quality = encTraits->defaultQuality;
	} else
	{
		m_Settings.FinalSampleFormat = SampleFormatFloat32;
		Encoder::Mode mode = (Encoder::Mode)((dwFormat >> 24) & 0xff);
		int quality = (int)((dwFormat >> 0) & 0xff);
		int bitrate = (int)((dwFormat >> 0) & 0xffff);
		m_Settings.EncoderSettings.Mode = mode;
		m_Settings.EncoderSettings.Bitrate = bitrate;
		m_Settings.EncoderSettings.Quality = static_cast<float>(quality) * 0.01f;
		m_Settings.EncoderSettings.Format = -1;
	}
	
	m_Settings.EncoderSettings.Cues = IsDlgButtonChecked(IDC_CHECK3) ? true : false;

	{

		m_Settings.EncoderSettings.Tags = IsDlgButtonChecked(IDC_CHECK7) ? true : false;

		m_Settings.Tags = FileTags();

		if(m_Settings.EncoderSettings.Tags)
		{
			CString tmp;

			m_EditTitle.GetWindowText(tmp);
			m_Settings.Tags.title = mpt::String::Decode(tmp.GetString(), mpt::CharsetLocale);

			m_EditAuthor.GetWindowText(tmp);
			m_Settings.Tags.artist = mpt::String::Decode(tmp.GetString(), mpt::CharsetLocale);

			m_EditAlbum.GetWindowText(tmp);
			m_Settings.Tags.album = mpt::String::Decode(tmp.GetString(), mpt::CharsetLocale);

			m_EditURL.GetWindowText(tmp);
			m_Settings.Tags.url = mpt::String::Decode(tmp.GetString(), mpt::CharsetLocale);

			if((encTraits->modesWithFixedGenres & m_Settings.EncoderSettings.Mode) && !encTraits->genres.empty())
			{
				m_CbnGenre.GetWindowText(tmp);
				m_Settings.Tags.genre = mpt::String::Decode(tmp.GetString(), mpt::CharsetLocale);
			} else
			{
				m_EditGenre.GetWindowText(tmp);
				m_Settings.Tags.genre = mpt::String::Decode(tmp.GetString(), mpt::CharsetLocale);
			}

			m_EditYear.GetWindowText(tmp);
			m_Settings.Tags.year = mpt::String::Decode(tmp.GetString(), mpt::CharsetLocale);
			if(m_Settings.Tags.year == L"0")
			{
				m_Settings.Tags.year = std::wstring();
			}

			if(!m_pSndFile->songMessage.empty())
			{
				m_Settings.Tags.comments = mpt::String::Decode(m_pSndFile->songMessage, mpt::CharsetLocale);
			}

			m_Settings.Tags.bpm = mpt::String::Decode(mpt::String::Format("%d", (int)m_pSndFile->GetCurrentBPM()), mpt::CharsetLocale);

		}

	}

	CDialog::OnOK();
}


void CWaveConvertSettings::SelectEncoder(std::size_t index)
//---------------------------------------------------------
{
	ASSERT(!EncoderFactories.empty());
	ASSERT(index < EncoderFactories.size());
	EncoderIndex = index;
	const Encoder::Traits *encTraits = GetTraits();
	EncoderSettings.Mode = encTraits->defaultMode;
	EncoderSettings.Bitrate = encTraits->defaultBitrate;
	EncoderSettings.Quality = encTraits->defaultQuality;
}


EncoderFactoryBase *CWaveConvertSettings::GetEncoderFactory() const
//--------------------------------------------------------------
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


CWaveConvertSettings::CWaveConvertSettings(std::size_t defaultEncoder, const std::vector<EncoderFactoryBase*> &encFactories)
//--------------------------------------------------------------------------------------------------------------------------
	: EncoderFactories(encFactories)
	, EncoderIndex(defaultEncoder)
	, FinalSampleFormat(SampleFormatInt16)
	, EncoderSettings(true, true, 44100, 2, Encoder::ModeCBR, 256, 0.8f, -1)
	, Normalize(false)
	, SilencePlugBuffers(false)
{
	SelectEncoder(EncoderIndex);
}


/////////////////////////////////////////////////////////////////////////////////////////
// CDoWaveConvert: save a mod as a wave file

BEGIN_MESSAGE_MAP(CDoWaveConvert, CDialog)
	ON_COMMAND(IDC_BUTTON1,	OnButton1)
END_MESSAGE_MAP()


BOOL CDoWaveConvert::OnInitDialog()
//---------------------------------
{
	CDialog::OnInitDialog();
	PostMessage(WM_COMMAND, IDC_BUTTON1);
	return TRUE;
}


void CDoWaveConvert::OnButton1()
//------------------------------
{
	static char buffer[MIXBUFFERSIZE * 4 * 4]; // channels * sizeof(biggestsample)
	static float floatbuffer[MIXBUFFERSIZE * 4]; // channels
	static int mixbuffer[MIXBUFFERSIZE * 4]; // channels

	MSG msg;
	CHAR s[80];
	HWND progress = ::GetDlgItem(m_hWnd, IDC_PROGRESS1);
	UINT ok = IDOK, pos = 0;
	uint64 ullSamples = 0, ullMaxSamples;

	if (!m_pSndFile || !m_lpszFileName)
	{
		EndDialog(IDCANCEL);
		return;
	}
	
	float normalizePeak = 0.0f;
	char *tempPath = _tempnam("", "OpenMPT_mod2wave");
	const std::string normalizeFileName = tempPath;
	free(tempPath);
	mpt::fstream normalizeFile;
	if(m_Settings.Normalize)
	{
		normalizeFile.open(normalizeFileName.c_str(), std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
	}

	mpt::ofstream fileStream(m_lpszFileName, std::ios::binary | std::ios::trunc);

	if(!fileStream)
	{
		Reporting::Error("Could not open file for writing. Is it open in another application?");
		EndDialog(IDCANCEL);
		return;
	}

	const uint32 samplerate = m_Settings.EncoderSettings.Samplerate;
	const uint16 channels = m_Settings.EncoderSettings.Channels;

	ASSERT(m_Settings.GetEncoderFactory() && m_Settings.GetEncoderFactory()->IsAvailable());
	IAudioStreamEncoder *fileEnc = m_Settings.GetEncoderFactory()->ConstructStreamEncoder(fileStream);

	// Silence mix buffer of plugins, for plugins that don't clear their reverb buffers and similar stuff when they are reset
	if(m_Settings.SilencePlugBuffers)
	{
		for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
		{
			if(m_pSndFile->m_MixPlugins[i].pMixPlugin != nullptr)
			{
				// Render up to 20 seconds per plugin
				for(int j = 0; j < 20; j++)
				{
					const float maxVal = m_pSndFile->m_MixPlugins[i].pMixPlugin->RenderSilence(samplerate);
					if(maxVal <= FLT_EPSILON)
					{
						break;
					}
				}
			}
		}
	}

	MixerSettings oldmixersettings = m_pSndFile->m_MixerSettings;
	MixerSettings mixersettings = TrackerSettings::Instance().m_MixerSettings;
	mixersettings.m_nMaxMixChannels = MAX_CHANNELS; // always use max mixing channels when rendering
	mixersettings.gdwMixingFreq = samplerate;
	mixersettings.gnChannels = channels;
	m_pSndFile->m_SongFlags.reset(SONG_PAUSED | SONG_STEP);
	if(m_Settings.Normalize)
	{
#ifndef NO_AGC
		mixersettings.DSPMask &= ~SNDDSP_AGC;
#endif
	}

	Dither dither;

	m_pSndFile->ResetChannels();
	m_pSndFile->SetMixerSettings(mixersettings);
	m_pSndFile->SetResamplerSettings(TrackerSettings::Instance().m_ResamplerSettings);
	m_pSndFile->InitPlayer(TRUE);
	if ((!m_dwFileLimit) || (m_dwFileLimit > 2047*1024)) m_dwFileLimit = 2047*1024; // 2GB
	m_dwFileLimit <<= 10;

	fileEnc->SetFormat(m_Settings.EncoderSettings);
	if(m_Settings.EncoderSettings.Tags)
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

	// calculate maximum samples
	uint64 max = ullMaxSamples;
	uint64 l = static_cast<uint64>(m_pSndFile->GetSongTime() + 0.5) * samplerate * std::max<uint64>(1, 1 + m_pSndFile->GetRepeatCount());
	if (m_nMaxPatterns > 0)
	{
		ORDERINDEX dwOrds = m_pSndFile->Order.GetLengthFirstEmpty();
		if ((m_nMaxPatterns < dwOrds) && (dwOrds > 0)) l = (l*m_nMaxPatterns) / dwOrds;
	}

	if (l < max) max = l;

	if (progress != NULL)
	{
		::SendMessage(progress, PBM_SETRANGE, 0, MAKELPARAM(0, (DWORD)(max >> 14)));
	}

	// No pattern cue points yet
	m_pSndFile->m_PatternCuePoints.clear();
	m_pSndFile->m_PatternCuePoints.reserve(m_pSndFile->Order.GetLength());

	// Process the conversion

	// For calculating the remaining time
	DWORD dwStartTime = timeGetTime();
	// For giving away some processing time every now and then
	DWORD dwSleepTime = dwStartTime;

	size_t bytesWritten = 0;

	CMainFrame::GetMainFrame()->PauseMod();
	m_pSndFile->m_SongFlags.reset(SONG_STEP | SONG_PATTERNLOOP);
	CMainFrame::GetMainFrame()->InitRenderer(m_pSndFile);	//rewbs.VSTTimeInfo
	for (UINT n = 0; ; n++)
	{
		UINT lRead = 0;
		if(m_Settings.Normalize || m_Settings.FinalSampleFormat == SampleFormatFloat32)
		{
			lRead = ReadInterleaved(*m_pSndFile, floatbuffer, MIXBUFFERSIZE, SampleFormatFloat32, dither);
		} else
		{
			lRead = ReadInterleaved(*m_pSndFile, buffer, MIXBUFFERSIZE, m_Settings.FinalSampleFormat, dither);
		}

		// Process cue points (add base offset), if there are any to process.
		std::vector<PatternCuePoint>::reverse_iterator iter;
		for(iter = m_pSndFile->m_PatternCuePoints.rbegin(); iter != m_pSndFile->m_PatternCuePoints.rend(); ++iter)
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

		if(m_Settings.Normalize)
		{

			std::size_t countSamples = lRead * m_pSndFile->m_MixerSettings.gnChannels;
			const float *src = floatbuffer;
			while(countSamples--)
			{
				const float val = *src;
				if(val > normalizePeak) normalizePeak = val;
				else if(0.0f - val >= normalizePeak) normalizePeak = 0.0f - val;
				src++;
			}

			normalizeFile.write(reinterpret_cast<const char*>(floatbuffer), lRead * m_pSndFile->m_MixerSettings.gnChannels * sizeof(float));
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
			bytesWritten += static_cast<std::size_t>(newPos - oldPos);

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
			DWORD l = (DWORD)(ullSamples / m_pSndFile->m_MixerSettings.gdwMixingFreq);

			const DWORD dwCurrentTime = timeGetTime();
			DWORD timeRemaining = 0; // estimated remainig time
			if((ullSamples > 0) && (ullSamples < max))
			{
				timeRemaining = static_cast<DWORD>(((dwCurrentTime - dwStartTime) * (max - ullSamples) / ullSamples) / 1000);
			}

			if(m_Settings.Normalize)
			{
				wsprintf(s, "Rendering file... (%umn%02us, %umn%02us remaining)", l / 60, l % 60, timeRemaining / 60, timeRemaining % 60);
			} else
			{
				wsprintf(s, "Writing file... (%uKB, %umn%02us, %umn%02us remaining)", bytesWritten >> 10, l / 60, l % 60, timeRemaining / 60, timeRemaining % 60);
			}
			SetDlgItemText(IDC_TEXT1, s);

			// Give windows some time to redraw the window, if necessary (else, the infamous "OpenMPT does not respond" will pop up)
			if ((!m_bGivePlugsIdleTime) && (dwCurrentTime > dwSleepTime + 1000))
			{
				Sleep(1);
				dwSleepTime = dwCurrentTime;
			}
		}
		if ((progress != NULL) && ((DWORD)(ullSamples >> 14) != pos))
		{
			pos = (DWORD)(ullSamples >> 14);
			::SendMessage(progress, PBM_SETPOS, pos, 0);
		}
		if (::PeekMessage(&msg, m_hWnd, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		if (m_bAbort)
		{
			ok = IDCANCEL;
			break;
		}
	}

	m_pSndFile->m_nMaxOrderPosition = 0;

	CMainFrame::GetMainFrame()->StopRenderer(m_pSndFile);	//rewbs.VSTTimeInfo

	if(m_Settings.Normalize)
	{

		::SendMessage(progress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

		const float normalizeFactor = (normalizePeak != 0.0f) ? (1.0f / normalizePeak) : 1.0f;

		const uint64 framesTotal = ullSamples;
		int lastPercent = -1;

		normalizeFile.seekp(0);

		uint64 framesProcessed = 0;
		uint64 framesToProcess = framesTotal;
		while(framesToProcess)
		{
			const std::size_t framesChunk = std::min<std::size_t>(mpt::saturate_cast<std::size_t>(framesToProcess), MIXBUFFERSIZE);
			const std::size_t samplesChunk = framesChunk * channels;
			
			normalizeFile.read(reinterpret_cast<char*>(floatbuffer), samplesChunk * sizeof(float));
			if(normalizeFile.gcount() != samplesChunk * sizeof(float))
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
					case SampleFormatUnsigned8: ConvertInterleavedFixedPointToInterleaved<MIXING_FRACTIONAL_BITS>(reinterpret_cast<uint8*>(buffer), mixbuffer, channels, framesChunk); break;
					case SampleFormatInt16:     ConvertInterleavedFixedPointToInterleaved<MIXING_FRACTIONAL_BITS>(reinterpret_cast<int16*>(buffer), mixbuffer, channels, framesChunk); break;
					case SampleFormatInt24:     ConvertInterleavedFixedPointToInterleaved<MIXING_FRACTIONAL_BITS>(reinterpret_cast<int24*>(buffer), mixbuffer, channels, framesChunk); break;
					case SampleFormatInt32:     ConvertInterleavedFixedPointToInterleaved<MIXING_FRACTIONAL_BITS>(reinterpret_cast<int32*>(buffer), mixbuffer, channels, framesChunk); break;
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
					wsprintf(s, "Normalizing... (%d%%)", percent);
					SetDlgItemText(IDC_TEXT1, s);
					::SendMessage(progress, PBM_SETPOS, percent, 0);
					lastPercent = percent;
				}
			}

			framesProcessed += framesChunk;
			framesToProcess -= framesChunk;
		}

		normalizeFile.flush();
		normalizeFile.close();
		for(int retry=0; retry<10; retry++)
		{
			// stupid virus scanners
			if(remove(normalizeFileName.c_str()) != EACCES)
			{
				break;
			}
			Sleep(10);
		}

	}

	if(m_pSndFile->m_PatternCuePoints.size() > 0)
	{
		if(m_Settings.EncoderSettings.Cues)
		{
			std::vector<PatternCuePoint>::const_iterator iter;
			std::vector<uint64> cues;
			for(iter = m_pSndFile->m_PatternCuePoints.begin(); iter != m_pSndFile->m_PatternCuePoints.end(); iter++)
			{
				cues.push_back(static_cast<uint32>(iter->offset));
			}
			fileEnc->WriteCues(cues);
		}
		m_pSndFile->m_PatternCuePoints.clear();
	}

	fileEnc->Finalize();
	delete fileEnc;
	fileEnc = nullptr;

	fileStream.flush();
	fileStream.close();

	CMainFrame::UpdateAudioParameters(*m_pSndFile, TRUE);
	EndDialog(ok);
}

