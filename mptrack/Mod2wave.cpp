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
#include "../common/version.h"
#include "ACMConvert.h"
#include "../soundlib/Dither.h"

#include <fstream>

extern UINT nMixingRates[NUMMIXRATE];
extern LPCSTR gszChnCfgNames[3];

static const GUID guid_MEDIASUBTYPE_PCM        = {0x00000001, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
static const GUID guid_MEDIASUBTYPE_IEEE_FLOAT = {0x00000003, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};

///////////////////////////////////////////////////////////////////////////////////////////////////
// ID3v1 Genres

static const char *id3v1GenreNames[] =
{
	"Blues",                "Classic Rock",     "Country",          "Dance",
	"Disco",                "Funk",             "Grunge",           "Hip-Hop",
	"Jazz",                 "Metal",            "New Age",          "Oldies",
	"Other",                "Pop",              "R&B",              "Rap",
	"Reggae",               "Rock",             "Techno",           "Industrial",
	"Alternative",          "Ska",              "Death Metal",      "Pranks",
	"Soundtrack",           "Euro-Techno",      "Ambient",          "Trip-Hop",
	"Vocal",                "Jazz+Funk",        "Fusion",           "Trance",
	"Classical",            "Instrumental",     "Acid",             "House",
	"Game",                 "Sound Clip",       "Gospel",           "Noise",
	"Alt. Rock",            "Bass",             "Soul",             "Punk",
	"Space",                "Meditative",       "Instrumental Pop", "Instrumental Rock",
	"Ethnic",               "Gothic",           "Darkwave",         "Techno-Industrial",
	"Electronic",           "Pop-Folk",         "Eurodance",        "Dream",
	"Southern Rock",        "Comedy",           "Cult",             "Gangsta",
	"Top 40",               "Christian Rap",    "Pop/Funk",         "Jungle",
	"Native American",      "Cabaret",          "New Wave",         "Psychadelic",
	"Rave",                 "Showtunes",        "Trailer",          "Lo-Fi",
	"Tribal",               "Acid Punk",        "Acid Jazz",        "Polka",
	"Retro",                "Musical",          "Rock & Roll",      "Hard Rock",
	"Folk",                 "Folk-Rock",        "National Folk",    "Swing",
	"Fast Fusion",          "Bebob",            "Latin",            "Revival",
	"Celtic",               "Bluegrass",        "Avantgarde",       "Gothic Rock",
	"Progressive Rock",     "Psychedelic Rock", "Symphonic Rock",   "Slow Rock",
	"Big Band",             "Chorus",           "Easy Listening",   "Acoustic",
	"Humour",               "Speech",           "Chanson",          "Opera",
	"Chamber Music",        "Sonata",           "Symphony",         "Booty Bass",
	"Primus",               "Porn Groove",      "Satire",           "Slow Jam",
	"Club",                 "Tango",            "Samba",            "Folklore",
	"Ballad",               "Power Ballad",     "Rhythmic Soul",    "Freestyle",
	"Duet",                 "Punk Rock",        "Drum Solo",        "A Capella",
	"Euro-House",           "Dance Hall",       "Goa",              "Drum & Bass",
	"Club House",           "Hardcore",         "Terror",           "Indie",
	"BritPop",              "Negerpunk",        "Polsk Punk",       "Beat",
	"Christian Gangsta Rap","Heavy Metal",      "Black Metal",      "Crossover",
	"Contemporary Christian", "Christian Rock", "Merengue",         "Salsa",
	"Thrash Metal",         "Anime",            "JPop",             "Synthpop"
};

///////////////////////////////////////////////////
// CWaveConvert - setup for converting a wave file

BEGIN_MESSAGE_MAP(CWaveConvert, CDialog)
	ON_COMMAND(IDC_CHECK1,			OnCheck1)
	ON_COMMAND(IDC_CHECK2,			OnCheck2)
	ON_COMMAND(IDC_CHECK4,			OnCheckChannelMode)
	ON_COMMAND(IDC_CHECK6,			OnCheckInstrMode)
	ON_COMMAND(IDC_RADIO1,			UpdateDialog)
	ON_COMMAND(IDC_RADIO2,			UpdateDialog)
	ON_COMMAND(IDC_PLAYEROPTIONS,   OnPlayerOptions) //rewbs.resamplerConf
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnFormatChanged)
END_MESSAGE_MAP()


CWaveConvert::CWaveConvert(CWnd *parent, ORDERINDEX minOrder, ORDERINDEX maxOrder, ORDERINDEX numOrders) : CDialog(IDD_WAVECONVERT, parent)
//-----------------------------------------------------------------------------------------------------------------------------------------
{
	m_bGivePlugsIdleTime = false;
	m_bNormalize = false;
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
	MemsetZero(WaveFormat);
	WaveFormat.Format.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.Format.nChannels = 2;
	WaveFormat.Format.nSamplesPerSec = 44100;
	WaveFormat.Format.wBitsPerSample = 16;
	WaveFormat.Format.nBlockAlign = (WaveFormat.Format.wBitsPerSample * WaveFormat.Format.nChannels) / 8;
	WaveFormat.Format.nAvgBytesPerSec = WaveFormat.Format.nSamplesPerSec * WaveFormat.Format.nBlockAlign;
}


void CWaveConvert::DoDataExchange(CDataExchange *pDX)
//---------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1,	m_CbnSampleRate);
	DDX_Control(pDX, IDC_COMBO2,	m_CbnSampleFormat);
	DDX_Control(pDX, IDC_SPIN3,		m_SpinMinOrder);
	DDX_Control(pDX, IDC_SPIN4,		m_SpinMaxOrder);
	DDX_Control(pDX, IDC_SPIN5,		m_SpinLoopCount);
}


BOOL CWaveConvert::OnInitDialog()
//-------------------------------
{
	CHAR s[128];

	CDialog::OnInitDialog();
	CheckRadioButton(IDC_RADIO1, IDC_RADIO2, m_bSelectPlay ? IDC_RADIO2 : IDC_RADIO1);

	CheckDlgButton(IDC_CHECK3, MF_CHECKED);		// HQ resampling
	CheckDlgButton(IDC_CHECK5, MF_UNCHECKED);	// rewbs.NoNormalize

	CheckDlgButton(IDC_CHECK4, MF_UNCHECKED);
	CheckDlgButton(IDC_CHECK6, MF_UNCHECKED);

	SetDlgItemInt(IDC_EDIT3, m_nMinOrder);
	m_SpinMinOrder.SetRange(0, m_nNumOrders);
	SetDlgItemInt(IDC_EDIT4, m_nMaxOrder);
	m_SpinMaxOrder.SetRange(0, m_nNumOrders);

	SetDlgItemInt(IDC_EDIT5, loopCount, FALSE);
	m_SpinLoopCount.SetRange(1, int16_max);


	for (size_t i = 0; i < CountOf(nMixingRates); i++)
	{
		UINT n = nMixingRates[i];
		wsprintf(s, "%d Hz", n);
		m_CbnSampleRate.SetItemData(m_CbnSampleRate.AddString(s), n);
		if (n == WaveFormat.Format.nSamplesPerSec) m_CbnSampleRate.SetCurSel(i);
	}
	for (UINT j=0; j<3*4; j++)
	{
		UINT n = 3*4-1-j;
		UINT nBits = 8 * (1 + n % 4);
		UINT nChannels = 1 << (n/4);
		if ((nBits >= 16) || (nChannels <= 2))
		{
			wsprintf(s, "%s, %d-Bit", gszChnCfgNames[n/4], nBits);
			UINT ndx = m_CbnSampleFormat.AddString(s);
			m_CbnSampleFormat.SetItemData(ndx, (nChannels<<8)|nBits);
			if ((nBits == WaveFormat.Format.wBitsPerSample) && (nChannels == WaveFormat.Format.nChannels))
			{
				m_CbnSampleFormat.SetCurSel(ndx);
			}
		}
	}

	UpdateDialog();
	return TRUE;
}


void CWaveConvert::OnFormatChanged()
//----------------------------------
{
	DWORD dwFormat = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());
	(void)dwFormat;
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
	//m_bHighQuality = IsDlgButtonChecked(IDC_CHECK3) != BST_UNCHECKED; //rewbs.resamplerConf - we don't want this anymore.
	m_bNormalize = IsDlgButtonChecked(IDC_CHECK5) != BST_UNCHECKED;
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

	// WaveFormatEx
	DWORD dwFormat = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());
	WaveFormat.Format.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.Format.nSamplesPerSec = m_CbnSampleRate.GetItemData(m_CbnSampleRate.GetCurSel());
	if (WaveFormat.Format.nSamplesPerSec < 11025) WaveFormat.Format.nSamplesPerSec = 11025;
	if (WaveFormat.Format.nSamplesPerSec > 192000) WaveFormat.Format.nSamplesPerSec = 192000;
	WaveFormat.Format.nChannels = (WORD)(dwFormat >> 8);
	if ((WaveFormat.Format.nChannels != 1) && (WaveFormat.Format.nChannels != 4)) WaveFormat.Format.nChannels = 2;
	WaveFormat.Format.wBitsPerSample = (WORD)(dwFormat & 0xFF);

	if ((WaveFormat.Format.wBitsPerSample != 8) && (WaveFormat.Format.wBitsPerSample != 24) && (WaveFormat.Format.wBitsPerSample != 32)) WaveFormat.Format.wBitsPerSample = 16;

	WaveFormat.Format.nBlockAlign = (WaveFormat.Format.wBitsPerSample * WaveFormat.Format.nChannels) / 8;
	WaveFormat.Format.nAvgBytesPerSec = WaveFormat.Format.nSamplesPerSec * WaveFormat.Format.nBlockAlign;
	WaveFormat.Format.cbSize = 0;
	// Mono/Stereo 32-bit floating-point
	if ((WaveFormat.Format.wBitsPerSample == 32) && (WaveFormat.Format.nChannels <= 2))
	{
		WaveFormat.Format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	} else
	// MultiChannel configuration
	if ((WaveFormat.Format.wBitsPerSample == 32) || (WaveFormat.Format.nChannels > 2))
	{
		WaveFormat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		WaveFormat.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
		WaveFormat.Samples.wValidBitsPerSample = WaveFormat.Format.wBitsPerSample;
		switch(WaveFormat.Format.nChannels)
		{
		case 1:		WaveFormat.dwChannelMask = 0x0004; break; // FRONT_CENTER
		case 2:		WaveFormat.dwChannelMask = 0x0003; break; // FRONT_LEFT | FRONT_RIGHT
		case 3:		WaveFormat.dwChannelMask = 0x0103; break; // FRONT_LEFT|FRONT_RIGHT|BACK_CENTER
		case 4:		WaveFormat.dwChannelMask = 0x0033; break; // FRONT_LEFT|FRONT_RIGHT|BACK_LEFT|BACK_RIGHT
		default:	WaveFormat.dwChannelMask = 0; break;
		}
		WaveFormat.SubFormat = guid_MEDIASUBTYPE_PCM;
	}
	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////////////////
// CLayer3Convert: Converting to MPEG Layer 3

BEGIN_MESSAGE_MAP(CLayer3Convert, CDialog)
	ON_COMMAND(IDC_CHECK1,			OnCheck1)
	ON_COMMAND(IDC_CHECK2,			OnCheck2)
	ON_CBN_SELCHANGE(IDC_COMBO2,	UpdateDialog)
END_MESSAGE_MAP()


void CLayer3Convert::DoDataExchange(CDataExchange* pDX)
//-----------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsPlayer)
	DDX_Control(pDX, IDC_COMBO1,		m_CbnFormat);
	DDX_Control(pDX, IDC_COMBO2,		m_CbnDriver);
	DDX_Control(pDX, IDC_COMBO3,		m_CbnGenre);
	DDX_Control(pDX, IDC_EDIT3,			m_EditAuthor);
	DDX_Control(pDX, IDC_EDIT4,			m_EditURL);
	DDX_Control(pDX, IDC_EDIT5,			m_EditAlbum);
	DDX_Control(pDX, IDC_EDIT6,			m_EditYear);
	//}}AFX_DATA_MAP
}


BOOL CLayer3Convert::OnInitDialog()
//---------------------------------
{
	ACMFORMATDETAILS afd;
	BYTE wfx[256];
	WAVEFORMATEX *pwfx = (WAVEFORMATEX *)&wfx;
	
	CDialog::OnInitDialog();
	Drivers[0] = 0;
	m_bInitialFound = FALSE;
	m_bDriversEnumerated = FALSE;
	m_nNumFormats = 0;
	m_nNumDrivers = 0;
	MemsetZero(afd);
	MemsetZero(*pwfx);
	afd.cbStruct = sizeof(ACMFORMATDETAILS);
	afd.dwFormatTag = WAVE_FORMAT_PCM;
	afd.pwfx = pwfx;
	afd.cbwfx = sizeof(wfx);
	pwfx->wFormatTag = WAVE_FORMAT_PCM;
	pwfx->nChannels = 2;
	pwfx->nSamplesPerSec = 44100;
	pwfx->wBitsPerSample = 16;
	pwfx->nBlockAlign = (pwfx->nChannels * pwfx->wBitsPerSample) / 8;
	pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
	acmConvert.AcmFormatEnum(NULL, &afd, AcmFormatEnumCB, (DWORD)this, ACM_FORMATENUMF_CONVERT);
	m_bDriversEnumerated = TRUE;
	m_CbnDriver.SetCurSel(m_nDriverIndex);
	if (m_bSaveInfoField) CheckDlgButton(IDC_CHECK3, MF_CHECKED);
	for(size_t iGnr = 0; iGnr < CountOf(id3v1GenreNames); iGnr++)
	{
		m_CbnGenre.SetItemData(m_CbnGenre.AddString(id3v1GenreNames[iGnr]), iGnr);
	}

	m_EditYear.SetLimitText(4);
	CTime tTime = CTime::GetCurrentTime();
	m_EditYear.SetWindowText(tTime.Format("%Y"));
	UpdateDialog();
	return TRUE;
}


VOID CLayer3Convert::UpdateDialog()
//---------------------------------
{
	ACMFORMATDETAILS afd;
	BYTE wfx[256];
	WAVEFORMATEX *pwfx = (WAVEFORMATEX *)&wfx;
	
	m_CbnFormat.ResetContent();
	m_nNumFormats = 0;
	m_nFormatIndex = 0;
	m_nDriverIndex = m_CbnDriver.GetItemData(m_CbnDriver.GetCurSel());
	m_bInitialFound = FALSE;
	if (m_nDriverIndex >= m_nNumDrivers) m_nDriverIndex = 0;
	MemsetZero(afd);
	MemsetZero(wfx);
	afd.cbStruct = sizeof(ACMFORMATDETAILS);
	afd.dwFormatTag = WAVE_FORMAT_PCM;
	afd.pwfx = pwfx;
	afd.cbwfx = sizeof(wfx);
	pwfx->wFormatTag = WAVE_FORMAT_PCM;
	pwfx->nChannels = 2;
	pwfx->nSamplesPerSec = 44100;
	pwfx->wBitsPerSample = 16;
	pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
	pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
	acmConvert.AcmFormatEnum(NULL, &afd, AcmFormatEnumCB, (DWORD)this, ACM_FORMATENUMF_CONVERT);
	m_CbnFormat.SetCurSel(m_nFormatIndex);
}


void CLayer3Convert::GetFormat(PMPEGLAYER3WAVEFORMAT pwfx, HACMDRIVERID *phadid)
//------------------------------------------------------------------------------
{
	*pwfx = Formats[m_nFormatIndex];
	*phadid = Drivers[m_nDriverIndex];
}


BOOL CLayer3Convert::AcmFormatEnumCB(HACMDRIVERID hdid, LPACMFORMATDETAILS pafd, DWORD dwInstance, DWORD fdwSupport)
//------------------------------------------------------------------------------------------------------------------
{
	if (dwInstance)
	{
		CLayer3Convert *that = ((CLayer3Convert *)dwInstance);
		if (that->m_bDriversEnumerated)
			return ((CLayer3Convert *)dwInstance)->FormatEnumCB(hdid, pafd, fdwSupport);
		else
			return ((CLayer3Convert *)dwInstance)->DriverEnumCB(hdid, pafd, fdwSupport);
	}
	return FALSE;
}


BOOL CLayer3Convert::DriverEnumCB(HACMDRIVERID hdid, LPACMFORMATDETAILS pafd, DWORD fdwSupport)
//---------------------------------------------------------------------------------------------
{
	if ((pafd) && (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC)
	 && (pafd->dwFormatTag == WAVE_FORMAT_MPEGLAYER3) && (m_nNumDrivers < MAX_DRIVERS))
	{
		ACMDRIVERDETAILS add;
		for (UINT i=0; i<m_nNumDrivers; i++)
		{
			if (Drivers[i] == hdid) return TRUE;
		}
		MemsetZero(add);
		add.cbStruct = sizeof(add);
		if (acmConvert.AcmDriverDetails(hdid, &add, 0L) == MMSYSERR_NOERROR)
		{
			Drivers[m_nNumDrivers] = hdid;
			CHAR *pszName = ((add.szLongName[0]) && (strlen(add.szLongName) < 40)) ? add.szLongName : add.szShortName;
			m_CbnDriver.SetItemData( m_CbnDriver.AddString(pszName), m_nNumDrivers);
			m_nNumDrivers++;
		}
	}
	return TRUE;
}


BOOL CLayer3Convert::FormatEnumCB(HACMDRIVERID hdid, LPACMFORMATDETAILS pafd, DWORD fdwSupport)
//---------------------------------------------------------------------------------------------
{
	if ((pafd) && (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC)
	 && (hdid == Drivers[m_nDriverIndex])
	 && (pafd->dwFormatTag == WAVE_FORMAT_MPEGLAYER3) && (pafd->pwfx)
	 && (pafd->cbwfx >= sizeof(MPEGLAYER3WAVEFORMAT))
	 && (pafd->pwfx->wFormatTag == WAVE_FORMAT_MPEGLAYER3)
	 && (pafd->pwfx->cbSize == sizeof(MPEGLAYER3WAVEFORMAT)-sizeof(WAVEFORMATEX))
	 && (pafd->pwfx->nSamplesPerSec >= 11025)
	 && (pafd->pwfx->nChannels >= 1)
	 && (pafd->pwfx->nChannels <= 2)
	 && (m_nNumFormats < MAX_FORMATS))
	{
		PMPEGLAYER3WAVEFORMAT pmwf = (PMPEGLAYER3WAVEFORMAT)pafd->pwfx;
		Formats[m_nNumFormats] = *pmwf;
		if ((!m_bInitialFound) && (pmwf->wfx.nSamplesPerSec == 44100) && (pmwf->wfx.nChannels == 2))
		{
			m_nFormatIndex = m_nNumFormats;
			m_bInitialFound = TRUE;
		}
		m_CbnFormat.SetItemData( m_CbnFormat.AddString(pafd->szFormat), m_nNumFormats);
		m_nNumFormats++;
	}
	return (m_nNumFormats < MAX_FORMATS) ? TRUE : FALSE;
}


void CLayer3Convert::OnCheck1()
//-----------------------------
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
}


void CLayer3Convert::OnCheck2()
//-----------------------------
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
}


void CLayer3Convert::OnOK()
//-------------------------
{
	CHAR sText[256] = "";

	if (m_dwFileLimit) m_dwFileLimit = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
	if (m_dwSongLimit) m_dwSongLimit = GetDlgItemInt(IDC_EDIT2, NULL, FALSE);
	m_nFormatIndex = m_CbnFormat.GetItemData(m_CbnFormat.GetCurSel());
	m_nDriverIndex = m_CbnDriver.GetItemData(m_CbnDriver.GetCurSel());
	m_bSaveInfoField = IsDlgButtonChecked(IDC_CHECK3);

	m_FileTags.title = m_pSndFile->GetTitle();

	m_EditAuthor.GetWindowText(sText, sizeof(sText));
	m_FileTags.artist = sText;

	m_EditURL.GetWindowText(sText, sizeof(sText));
	m_FileTags.url = sText;

	m_EditAlbum.GetWindowText(sText, sizeof(sText));
	m_FileTags.album = sText;

	m_EditYear.GetWindowText(sText, MIN(5, sizeof(sText)));
	m_FileTags.year = sText;
	if(m_FileTags.year == "0")
		m_FileTags.year = "";

	m_CbnGenre.GetWindowText(sText, sizeof(sText));
	m_FileTags.genre = sText;

	if(!m_pSndFile->songMessage.empty())
	{
		m_FileTags.comments = m_pSndFile->songMessage.GetFormatted(SongMessage::leLF);
	}
	else
	{
		m_FileTags.comments = "";
	}

	wsprintf(sText, "%d", (int)m_pSndFile->GetCurrentBPM());
	m_FileTags.bpm = sText;

	CDialog::OnOK();
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


#define WAVECONVERTBUFSIZE	MIXBUFFERSIZE //Going over MIXBUFFERSIZE can kill VSTPlugs 


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
	std::string normalizeFileName = _tempnam("", "OpenMPT_mod2wave");
	std::fstream normalizeFile;
	if(m_bNormalize)
	{
		normalizeFile.open(normalizeFileName.c_str(), std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
	}

	WAVWriter file(m_lpszFileName);
	while(!file.IsValid())
	{
		if(Reporting::RetryCancel("Could not open file for writing. Is it open in another application?") == rtyCancel)
		{
			EndDialog(IDCANCEL);
			return;
		}
		file.Open(m_lpszFileName);
	}

	Dither dither;

	SampleFormat sampleFormat = (m_pWaveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) ? SampleFormatFloat32 : (SampleFormat)m_pWaveFormat->wBitsPerSample;
	MixerSettings oldmixersettings = m_pSndFile->m_MixerSettings;
	MixerSettings mixersettings = TrackerSettings::Instance().m_MixerSettings;
	mixersettings.gdwMixingFreq = m_pWaveFormat->nSamplesPerSec;
	mixersettings.gnChannels = m_pWaveFormat->nChannels;
	m_pSndFile->m_SongFlags.reset(SONG_PAUSED | SONG_STEP);
	if(m_bNormalize)
	{
		sampleFormat = SampleFormatFloat32;
#ifndef NO_AGC
		mixersettings.DSPMask &= ~SNDDSP_AGC;
#endif
		if(mixersettings.m_nPreAmp > 128) mixersettings.m_nPreAmp = 128;
	}

	m_pSndFile->ResetChannels();
	m_pSndFile->SetMixerSettings(mixersettings);
	m_pSndFile->SetResamplerSettings(TrackerSettings::Instance().m_ResamplerSettings);
	m_pSndFile->InitPlayer(TRUE);
	if ((!m_dwFileLimit) || (m_dwFileLimit > 2047*1024)) m_dwFileLimit = 2047*1024; // 2GB
	m_dwFileLimit <<= 10;

	file.WriteFormat(m_pWaveFormat->nSamplesPerSec, m_pWaveFormat->wBitsPerSample, m_pWaveFormat->nChannels, m_pWaveFormat->wBitsPerSample == 32 ? WAVFormatChunk::fmtFloat : WAVFormatChunk::fmtPCM);

	file.StartChunk(RIFFChunk::iddata);

	ullMaxSamples = m_dwFileLimit / (m_pWaveFormat->nChannels * m_pWaveFormat->wBitsPerSample / 8);
	if (m_dwSongLimit)
	{
		uint64 l = (uint64)m_dwSongLimit * m_pWaveFormat->nSamplesPerSec;
		if (l < ullMaxSamples) ullMaxSamples = l;
	}

	// calculate maximum samples
	uint64 max = ullMaxSamples;
	uint64 l = static_cast<uint64>(m_pSndFile->GetSongTime() + 0.5) * m_pWaveFormat->nSamplesPerSec * std::max<uint64>(1, 1 + m_pSndFile->GetRepeatCount());
	if (m_nMaxPatterns > 0)
	{
		DWORD dwOrds = m_pSndFile->Order.GetLengthFirstEmpty();
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
		if(m_bNormalize)
		{
			lRead = m_pSndFile->ReadInterleaved(floatbuffer, MIXBUFFERSIZE, sampleFormat, dither);
		} else
		{
			lRead = m_pSndFile->ReadInterleaved(buffer, MIXBUFFERSIZE, sampleFormat, dither);
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

		if (m_bNormalize)
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

			UINT lWrite = fwrite(buffer, 1, lRead * m_pSndFile->m_MixerSettings.gnChannels * (sampleFormat.GetBitsPerSample()/8), file.GetFile());
			if (!lWrite) 
				break;
			bytesWritten += lWrite;
			if (bytesWritten >= m_dwFileLimit) 
				break;

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

			if(m_bNormalize)
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

	CMainFrame::GetMainFrame()->StopRenderer(m_pSndFile);	//rewbs.VSTTimeInfo

	if (m_bNormalize)
	{
		::SendMessage(progress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

		const float normalizeFactor = (normalizePeak != 0.0f) ? (1.0f / normalizePeak) : 1.0f;

		const DWORD dwBitSize = m_pWaveFormat->wBitsPerSample / 8;
		const uint64 framesTotal = ullSamples;
		int lastPercent = -1;

		normalizeFile.seekp(0);

		uint64 framesProcessed = 0;
		uint64 framesToProcess = framesTotal;
		while(framesToProcess)
		{
			const std::size_t framesChunk = std::min<std::size_t>(mpt::saturate_cast<std::size_t>(framesToProcess), MIXBUFFERSIZE);
			const std::size_t samplesChunk = framesChunk * m_pWaveFormat->nChannels;
			
			normalizeFile.read(reinterpret_cast<char*>(floatbuffer), samplesChunk * sizeof(float));
			if(normalizeFile.gcount() != samplesChunk * sizeof(float))
				break;

			for(std::size_t i = 0; i < samplesChunk; ++i)
			{
				floatbuffer[i] *= normalizeFactor;
			}
			if(
				m_pWaveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT
				|| (m_pWaveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE && reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(m_pWaveFormat)->SubFormat == guid_MEDIASUBTYPE_IEEE_FLOAT)
				)
			{
				// Do not dither for 32bit float output.
				ASSERT(sizeof(float) == dwBitSize);
				std::memcpy(buffer, floatbuffer, samplesChunk * sizeof(float));
			} else
			{
				// Convert float buffer to mixbuffer format so we can apply dither.
				// This can probably be changed in the future when dither supports floating point input directly.
				FloatToMonoMix(floatbuffer, mixbuffer, samplesChunk, MIXING_SCALEF);
				dither.Process(mixbuffer, framesChunk, m_pWaveFormat->nChannels, m_pWaveFormat->wBitsPerSample);
				switch(dwBitSize)
				{
					case 1: Convert32ToInterleaved(reinterpret_cast<uint8*>(buffer), mixbuffer, samplesChunk); break;
					case 2: Convert32ToInterleaved(reinterpret_cast<int16*>(buffer), mixbuffer, samplesChunk); break;
					case 3: Convert32ToInterleaved(reinterpret_cast<int24*>(buffer), mixbuffer, samplesChunk); break;
					case 4: Convert32ToInterleaved(reinterpret_cast<int32*>(buffer), mixbuffer, samplesChunk); break;
					default: ASSERT(false); break;
				}
			}

			fwrite(buffer, 1, samplesChunk * dwBitSize, file.GetFile());
			bytesWritten += samplesChunk * dwBitSize;

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
	}

	file.Skip(bytesWritten);
	file.Truncate();

	// Write cue points
	if(m_pSndFile->m_PatternCuePoints.size() > 0)
	{
		// Cue point header
		file.StartChunk(RIFFChunk::idcue_);
		uint32 numPoints = m_pSndFile->m_PatternCuePoints.size();
		SwapBytesLE(numPoints);
		file.Write(numPoints);

		// Write all cue points
		std::vector<PatternCuePoint>::const_iterator iter;
		uint32 index = 0;
		for(iter = m_pSndFile->m_PatternCuePoints.begin(); iter != m_pSndFile->m_PatternCuePoints.end(); iter++)
		{
			WAVCuePoint cuePoint;
			cuePoint.id = index++;
			cuePoint.position = static_cast<uint32>(iter->offset);
			cuePoint.riffChunkID = static_cast<uint32>(RIFFChunk::iddata);
			cuePoint.chunkStart = 0;	// we use no Wave List Chunk (wavl) as we have only one data block, so this should be 0.
			cuePoint.blockStart = 0;	// dito
			cuePoint.offset = cuePoint.position;
			cuePoint.ConvertEndianness();
			file.Write(cuePoint);
		}
		m_pSndFile->m_PatternCuePoints.clear();
	}

	WAVWriter::Metatags tags;
	tags.push_back(WAVWriter::Metatag(RIFFChunk::idINAM, m_pSndFile->GetTitle()));
	tags.push_back(WAVWriter::Metatag(RIFFChunk::idISFT, MptVersion::GetOpenMPTVersionStr()));
	file.WriteMetatags(tags);

	size_t fileSize = file.Finalize();

	m_pSndFile->m_nMaxOrderPosition = 0;
	if (m_bNormalize)
	{
		m_pSndFile->SetMixerSettings(oldmixersettings);
		CFile fw;
		if (fw.Open(m_lpszFileName, CFile::modeReadWrite | CFile::modeNoTruncate))
		{
			fw.SetLength(fileSize);
			fw.Close();
		}
	}

	if(m_bNormalize)
	{
		normalizeFile.close();
		for(int retry=0; retry<10; retry++)
			if(remove(normalizeFileName.c_str()) != EACCES)
				break;
	}

	CMainFrame::UpdateAudioParameters(*m_pSndFile, TRUE);
	EndDialog(ok);
}


/////////////////////////////////////////////////////////////////////////////////////////
// CDoAcmConvert: save a mod as a compressed file

BEGIN_MESSAGE_MAP(CDoAcmConvert, CDialog)
	ON_COMMAND(IDC_BUTTON1,	OnButton1)
END_MESSAGE_MAP()


CDoAcmConvert::CDoAcmConvert(CSoundFile *sndfile, LPCSTR fname, PWAVEFORMATEX pwfx, HACMDRIVERID hadid, CFileTagging *pTag, ACMConvert &acmConvert_, CWnd *parent):
	CDialog(IDD_PROGRESS, parent),
	acmConvert(acmConvert_)
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	m_pSndFile = sndfile;
	m_lpszFileName = fname;
	m_bAbort = FALSE;
	m_dwFileLimit = m_dwSongLimit = 0;
	m_pwfx = pwfx;
	m_hadid = hadid;
	m_bSaveInfoField = FALSE;
	if (pTag)
	{
		m_FileTags = *pTag;
		m_bSaveInfoField = TRUE;
	}
}


BOOL CDoAcmConvert::OnInitDialog()
//--------------------------------
{
	CDialog::OnInitDialog();
	SetWindowText("Encoding File...");
	PostMessage(WM_COMMAND, IDC_BUTTON1);
	return TRUE;
}


void CDoAcmConvert::OnButton1()
//-----------------------------
{
	CHAR s[80], fext[_MAX_EXT];
	WAVEFILEHEADER wfh;
	WAVEDATAHEADER wdh;
	ACMSTREAMHEADER ash;
	WAVEFORMATEX wfxSrc;
	HACMDRIVER hADriver = nullptr;
	HACMSTREAM hAStream = nullptr;
	HWND progress;
	MSG msg;
	LPBYTE pcmBuffer = nullptr, dstBuffer = nullptr;	// Render and conversion buffers
	UINT retval = IDCANCEL, pos, n;
	DWORD dwDstBufSize, pcmBufSize, data_ofs;
	uint64 ullMaxSamples, ullSamples;
	int oldrepeat;
	bool bPrepared = false, bFinished = false;
	FILE *f;

	MixerSettings mixersettings = TrackerSettings::Instance().m_MixerSettings;
	SampleFormat sampleFormat = SampleFormatInvalid;
	Dither dither;

	progress = ::GetDlgItem(m_hWnd, IDC_PROGRESS1);
	if ((!m_pSndFile) || (!m_lpszFileName) || (!m_pwfx) || (!m_hadid)) goto OnError;
	_splitpath(m_lpszFileName, NULL, NULL, NULL, fext);
	MemsetZero(wfxSrc);
	wfxSrc.wFormatTag = WAVE_FORMAT_PCM;
	wfxSrc.nSamplesPerSec = m_pwfx->nSamplesPerSec;
	if (wfxSrc.nSamplesPerSec < 11025) wfxSrc.nSamplesPerSec = 11025;
	wfxSrc.wBitsPerSample = 16;
	wfxSrc.nChannels = m_pwfx->nChannels;
	wfxSrc.nBlockAlign = (wfxSrc.nChannels * wfxSrc.wBitsPerSample) / 8;
	wfxSrc.nAvgBytesPerSec = wfxSrc.nSamplesPerSec * wfxSrc.nBlockAlign;
	wfxSrc.cbSize = 0;
	dwDstBufSize = WAVECONVERTBUFSIZE;
	// Open the ACM Driver
	if (acmConvert.AcmDriverOpen(&hADriver, m_hadid, 0L) != MMSYSERR_NOERROR) goto OnError;
	if (acmConvert.AcmStreamOpen(&hAStream, hADriver, &wfxSrc, m_pwfx, NULL, 0L, 0L, ACM_STREAMOPENF_NONREALTIME) != MMSYSERR_NOERROR) goto OnError;
	// This call is useless for BLADEenc/LAMEenc, but required for ACM codecs!
	if (acmConvert.AcmStreamSize(hAStream, WAVECONVERTBUFSIZE, &dwDstBufSize, ACM_STREAMSIZEF_SOURCE) != MMSYSERR_NOERROR) goto OnError;
	// This call is useless for ACM, but required for BLADEenc/LAMEenc codecs!
	if (acmConvert.AcmStreamSize(hAStream, WAVECONVERTBUFSIZE, &dwDstBufSize, ACM_STREAMSIZEF_DESTINATION) != MMSYSERR_NOERROR) goto OnError;
	//if (dwDstBufSize > 0x10000) dwDstBufSize = 0x10000;
	pcmBuffer = new BYTE[WAVECONVERTBUFSIZE];
	dstBuffer = new BYTE[dwDstBufSize];
	if ((!dstBuffer) || (!pcmBuffer)) goto OnError;
	memset(pcmBuffer, 0, WAVECONVERTBUFSIZE);
	memset(dstBuffer, 0, dwDstBufSize);
	MemsetZero(ash);
	ash.cbStruct = sizeof(ash);
	ash.pbSrc = pcmBuffer;
	ash.cbSrcLength = WAVECONVERTBUFSIZE;
	ash.pbDst = dstBuffer;
	ash.cbDstLength = dwDstBufSize;
	if (acmConvert.AcmStreamPrepareHeader(hAStream, &ash, 0L) != MMSYSERR_NOERROR) goto OnError;
	bPrepared = true;
	// Creating the output file
	while ((f = fopen(m_lpszFileName, "wb")) == NULL)
	{
		if(Reporting::RetryCancel("Could not open file for writing. Is it open in another application?") == rtyCancel)
		{
			goto OnError;
		}
	}
	wfh.id_RIFF = IFFID_RIFF;
	wfh.id_WAVE = IFFID_WAVE;
	wfh.filesize = 0;
	wdh.id_data = IFFID_data;
	wdh.length = 0;
	data_ofs = 0;
	if(m_bSaveInfoField)
	{
		// Write ID3v2.4 Tags
		m_FileTags.WriteID3v2Tags(f);
	}
	sampleFormat = SampleFormatInt16;
	DWORD oldsndcfg = m_pSndFile->m_MixerSettings.MixerFlags;
	oldrepeat = m_pSndFile->GetRepeatCount();
	const uint64 dwSongTime = static_cast<uint64>(m_pSndFile->GetSongTime() + 0.5);
	mixersettings.gdwMixingFreq = wfxSrc.nSamplesPerSec;
	mixersettings.gnChannels = wfxSrc.nChannels;
	m_pSndFile->SetRepeatCount(0);
	m_pSndFile->ResetChannels();
	m_pSndFile->SetMixerSettings(mixersettings);
	m_pSndFile->SetResamplerSettings(TrackerSettings::Instance().m_ResamplerSettings);
	m_pSndFile->InitPlayer(TRUE);
	m_pSndFile->m_SongFlags.reset(SONG_PAUSED | SONG_STEP);

	m_pSndFile->InitializeVisitedRows();

	// Setting up file limits and progress range
	if ((!m_dwFileLimit) || (m_dwFileLimit > 512000)) m_dwFileLimit = 512000;
	m_dwFileLimit <<= 10;
	ullMaxSamples = uint64_max; //60*60*wfxSrc.nSamplesPerSec; // 1 hour
	if (m_dwSongLimit)
	{
		uint64 l = m_dwSongLimit * wfxSrc.nSamplesPerSec;
		if (l < ullMaxSamples) ullMaxSamples = l;
	}

	// calculate maximum samples
	uint64 max = ullMaxSamples;
	uint64 l = dwSongTime * wfxSrc.nSamplesPerSec;
	if (l < max) max = l;
	if (progress != NULL)
	{
		::SendMessage(progress, PBM_SETRANGE, 0, MAKELPARAM(0, max >> 14));
	}

	// For calculating the remaining time
	DWORD dwStartTime = timeGetTime();

	ullSamples = 0;
	pos = 0;
	pcmBufSize = WAVECONVERTBUFSIZE;

	// Writing File
	CMainFrame::GetMainFrame()->InitRenderer(m_pSndFile);	//rewbs.VSTTimeInfo
	for (n=0; ; n++)
	{
		UINT lRead = 0;
		if (!bFinished)
		{
			lRead = m_pSndFile->ReadInterleaved(pcmBuffer + WAVECONVERTBUFSIZE - pcmBufSize, pcmBufSize/(m_pSndFile->m_MixerSettings.gnChannels*sampleFormat.GetBitsPerSample()/8), sampleFormat, dither);
			if (!lRead) bFinished = true;
		}
		ullSamples += lRead;
		ash.cbSrcLength = lRead * wfxSrc.nBlockAlign + WAVECONVERTBUFSIZE - pcmBufSize;
		ash.cbDstLengthUsed = 0;
		if (acmConvert.AcmStreamConvert(hAStream, &ash, (lRead) ? ACM_STREAMCONVERTF_BLOCKALIGN : ACM_STREAMCONVERTF_END) != MMSYSERR_NOERROR) break;
		do
		{
			if (::PeekMessage(&msg, m_hWnd, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
			if (m_bAbort) break;
		} while (0 == (ACMSTREAMHEADER_STATUSF_DONE & (*((volatile DWORD *)&ash.fdwStatus))));
		// Prepare for next time
		pcmBufSize = WAVECONVERTBUFSIZE;
		if ((ash.cbSrcLengthUsed > 0) && (ash.cbSrcLengthUsed < WAVECONVERTBUFSIZE))
		{
			memmove(pcmBuffer, pcmBuffer + ash.cbSrcLengthUsed, WAVECONVERTBUFSIZE - ash.cbSrcLengthUsed);
			pcmBufSize = ash.cbSrcLengthUsed;
		}
		if (ash.cbDstLengthUsed > 0)
		{
			ASSERT(ash.cbDstLengthUsed <= dwDstBufSize);
			UINT lWrite = fwrite(dstBuffer, 1, ash.cbDstLengthUsed, f);
			if (!lWrite) break;
			wdh.length += lWrite;
		}
		if ((!lRead) || (ullSamples >= ullMaxSamples) || (wdh.length >= m_dwFileLimit)) break;
		if (!(n % 10))
		{
			DWORD l = (DWORD)(ullSamples / wfxSrc.nSamplesPerSec);

			DWORD timeRemaining = 0; // estimated remainig time
			if((ullSamples > 0) && (ullSamples < max))
			{
				timeRemaining = static_cast<DWORD>(((timeGetTime() - dwStartTime) * (max - ullSamples) / ullSamples) / 1000);
			}

			wsprintf(s, "Writing file... (%uKB, %umn%02us) %umn%02us remaining)", wdh.length >> 10, l / 60, l % 60, timeRemaining / 60, timeRemaining % 60);
			SetDlgItemText(IDC_TEXT1, s);
		}
		if ((progress != NULL) && ((ullSamples >> 14) != pos))
		{
			pos = (DWORD)(ullSamples >> 14);
			::SendMessage(progress, PBM_SETPOS, pos, 0);
		}
		if (m_bAbort) break;
	}
	CMainFrame::GetMainFrame()->StopRenderer(m_pSndFile);	//rewbs.VSTTimeInfo
	// Done
	m_pSndFile->m_MixerSettings.MixerFlags = oldsndcfg;
	m_pSndFile->SetRepeatCount(oldrepeat);
	m_pSndFile->m_nMaxOrderPosition = 0;
	m_pSndFile->InitializeVisitedRows();
	CMainFrame::UpdateAudioParameters(*m_pSndFile, TRUE);

	// Success

	fclose(f);
	if (!m_bAbort) retval = IDOK;
OnError:
	if (bPrepared) acmConvert.AcmStreamUnprepareHeader(hAStream, &ash, 0L);
	if (hAStream != NULL) acmConvert.AcmStreamClose(hAStream, 0L);
	if (hADriver != NULL) acmConvert.AcmDriverClose(hADriver, 0L);
	if (pcmBuffer) delete[] pcmBuffer;
	if (dstBuffer) delete[] dstBuffer;
	EndDialog(retval);
}
