#include "stdafx.h"
#include "mptrack.h"
#include "sndfile.h"
#include "dlsbank.h"
#include "mainfrm.h"
#include "mpdlgs.h"
#include "mod2wave.h"

#define NUM_GENRES		128

static LPCSTR gpszGenreNames[NUM_GENRES] =
{
	"Blues",			"Classic Rock",			"Country",			"Dance",
	"Disco",			"Funk",					"Grunge",			"Hip Hop",
	"Jazz",				"Metal",				"New_Age",			"Oldies",
	"Other",			"Pop",					"Rhythm n Blues",	"Rap",
	"Reggae",			"Rock",					"Techno",			"Industrial",
	"Alternative",		"Ska",					"Death Metal",		"Pranks",
	"Soundtrack",		"Euro Techno",			"Ambient",			"Trip_Hop",
	"Vocal",			"Jazz Funk",			"Fusion",			"Trance",
	"Classical",		"Instrumental",			"Acid",				"House",
	"Game",				"Sound Clip",			"Gospel",			"Noise",
	"Alternative Rock",	"Bass",					"Soul",				"Punk",
	"Space",			"Meditative",			"Instrumental Pop",	"Instrumental Rock",
	"Ethnic",			"Gothic",				"Darkwave",			"Techno Industrial",
	"Electronic",		"Pop Folk",				"Eurodance",		"Dream",
	"Southern Rock",	"Comedy",				"Cult",				"Gangsta",
	"Top 40",			"Christian Rap",		"Pop Funk",			"Jungle",
	"Native_American",	"Cabaret",				"New_Wave",			"Psychadelic",
	"Rave",				"ShowTunes",			"Trailer",			"Lo Fi",
	"Tribal",			"Acid Punk",			"Acid Jazz",		"Polka",
	"Retro",			"Musical",				"Rock n Roll",		"Hard_Rock",
	"Folk",				"Folk Rock",			"National Folk",	"Swing",
	"Fast Fusion",		"Bebob",				"Latin",			"Revival",
	"Celtic",			"Bluegrass",			"Avantgarde",		"Gothic Rock",
	"Progressive Rock",	"Psychedelic Rock",		"Symphonic Rock",	"Slow Rock",
	"Big Band",			"Chorus",				"Easy Listening",	"Acoustic",
	"Humour",			"Speech",				"Chanson",			"Opera",
	"Chamber Music",	"Sonata",				"Symphony",			"Booty_Bass",
	"Primus",			"Porn Groove",			"Satire",			"Slow Jam",
	"Club",				"Tango",				"Samba",			"Folklore",
	"Ballad",			"Power Ballad",			"Rhytmic Soul",		"Freestyle",
	"Duet",				"Punk Rock",			"Drum Solo",		"Acapella",
	"Euro House",		"Dance Hall",			"Goa",				"Drum n Bass",
};


extern UINT nMixingRates[NUMMIXRATE];
extern LPCSTR gszChnCfgNames[3];

static void __cdecl M2W_32ToFloat(void *pBuffer, long nCount)
{
	//const float _ki2f = 1.0f / (FLOAT)(ULONG)0x80000000;  
	const float _ki2f = 1.0f / (FLOAT)(ULONG)(0x7fffffff); //ericus' 32bit fix
	_asm {
	mov esi, pBuffer
	mov ecx, nCount
	fld _ki2f
	test ecx, 1
	jz evencount
	fild dword ptr [esi]
	fmul st(0), st(1)
	fstp dword ptr [esi]
	add esi, 4
evencount:
	shr ecx, 1
	or ecx, ecx
	jz loopdone
cvtloop:
	fild dword ptr [esi]
	fild dword ptr [esi+4]
	fmul st(0), st(2)
	fstp dword ptr [esi+4]
	fmul st(0), st(1)
	fstp dword ptr [esi]
	add esi, 8
	dec ecx
	jnz cvtloop
loopdone:
	fstp st(0)
	}
}

///////////////////////////////////////////////////
// CWaveConvert - setup for converting a wave file

BEGIN_MESSAGE_MAP(CWaveConvert, CDialog)
	ON_COMMAND(IDC_CHECK1,			OnCheck1)
	ON_COMMAND(IDC_CHECK2,			OnCheck2)
	ON_COMMAND(IDC_RADIO1,			UpdateDialog)
	ON_COMMAND(IDC_RADIO2,			UpdateDialog)
	ON_COMMAND(IDC_PLAYEROPTIONS,   OnPlayerOptions) //rewbs.resamplerConf
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnFormatChanged)
END_MESSAGE_MAP()


CWaveConvert::CWaveConvert(CWnd *parent):CDialog(IDD_WAVECONVERT, parent)
//-----------------------------------------------------------------------
{
	m_bNormalize = FALSE;
	m_bHighQuality = FALSE;
	m_bSelectPlay = FALSE;
	m_nMinOrder = m_nMaxOrder = 0;
	m_dwFileLimit = 0;
	m_dwSongLimit = 0;
	memset(&WaveFormat, 0, sizeof(WaveFormat));
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
	DDX_Control(pDX, IDC_EDIT3,		m_EditMinOrder);
	DDX_Control(pDX, IDC_EDIT4,		m_EditMaxOrder);
}


BOOL CWaveConvert::OnInitDialog()
//-------------------------------
{
	CHAR s[128];

	CDialog::OnInitDialog();
	if (m_bSelectPlay)
		CheckDlgButton(IDC_RADIO2, MF_CHECKED);
	else
		CheckDlgButton(IDC_RADIO1, MF_CHECKED);
	CheckDlgButton(IDC_CHECK3, MF_CHECKED); // HQ resampling
	CheckDlgButton(IDC_CHECK5, MF_UNCHECKED);	// rewbs.NoNormalize

// -> CODE#0024
// -> DESC="wav export update"
	CheckDlgButton(IDC_CHECK4, MF_UNCHECKED);
// -! NEW_FEATURE#0024

	SetDlgItemInt(IDC_EDIT3, m_nMinOrder);
	SetDlgItemInt(IDC_EDIT4, m_nMaxOrder);
	for (UINT i=0; i<NUMMIXRATE; i++)
	{
		UINT n = nMixingRates[i];
		wsprintf(s, "%d Hz", n);
		m_CbnSampleRate.SetItemData(m_CbnSampleRate.AddString(s), n);
		if (n == WaveFormat.Format.nSamplesPerSec) m_CbnSampleRate.SetCurSel(i);
	}
// -> CODE#0024
// -> DESC="wav export update"
//	for (UINT j=0; j<3*3; j++)
	for (UINT j=0; j<3*4; j++)
// -! NEW_FEATURE#0024
	{
// -> CODE#0024
// -> DESC="wav export update"
//		UINT n = 3*3-1-j;
//		UINT nBits = 8 << (n % 3);
//		UINT nChannels = 1 << (n/3);
		UINT n = 3*4-1-j;
		UINT nBits = 8 * (1 + n % 4);
		UINT nChannels = 1 << (n/4);
// -! NEW_FEATURE#0024
		if ((nBits >= 16) || (nChannels <= 2))
		{
// -> CODE#0024
// -> DESC="wav export update"
//			wsprintf(s, "%s, %d Bit", gszChnCfgNames[j/3], nBits);
			wsprintf(s, "%s, %d-Bit", gszChnCfgNames[n/4], nBits);
// -! NEW_FEATURE#0024
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
	UINT nBits = dwFormat & 0xFF;
// -> CODE#0024
// -> DESC="wav export update"
//	::EnableWindow( ::GetDlgItem(m_hWnd, IDC_CHECK5), (nBits <= 16) ? TRUE : FALSE );
	::EnableWindow( ::GetDlgItem(m_hWnd, IDC_CHECK5), (nBits <= 24) ? TRUE : FALSE );
// -! NEW_FEATURE#0024
}


void CWaveConvert::UpdateDialog()
//-------------------------------
{
	CheckDlgButton(IDC_CHECK1, (m_dwFileLimit) ? MF_CHECKED : 0);
	CheckDlgButton(IDC_CHECK2, (m_dwSongLimit) ? MF_CHECKED : 0);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT1), (m_dwFileLimit) ? TRUE : FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT2), (m_dwSongLimit) ? TRUE : FALSE);
	BOOL bSel = IsDlgButtonChecked(IDC_RADIO2) ? TRUE : FALSE;
	m_EditMinOrder.EnableWindow(bSel);
	m_EditMaxOrder.EnableWindow(bSel);
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

//rewbs.resamplerConf
void CWaveConvert::OnPlayerOptions()
//----------------------------------
{
	CMainFrame::m_nLastOptionsPage = 2;
	CMainFrame::GetMainFrame()->OnViewOptions();
}
//end rewbs.resamplerConf

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


void CWaveConvert::OnOK()
//-----------------------
{
	if (m_dwFileLimit) m_dwFileLimit = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
	if (m_dwSongLimit) m_dwSongLimit = GetDlgItemInt(IDC_EDIT2, NULL, FALSE);
	m_bSelectPlay = IsDlgButtonChecked(IDC_RADIO2) ? TRUE : FALSE;
	m_nMinOrder = GetDlgItemInt(IDC_EDIT3, NULL, FALSE);
	m_nMaxOrder = GetDlgItemInt(IDC_EDIT4, NULL, FALSE);
	if (m_nMaxOrder < m_nMinOrder) m_bSelectPlay = FALSE;
	//m_bHighQuality = IsDlgButtonChecked(IDC_CHECK3) ? TRUE : FALSE; //rewbs.resamplerConf - we don't want this anymore.
	m_bNormalize = IsDlgButtonChecked(IDC_CHECK5) ? TRUE : FALSE;

// -> CODE#0024
// -> DESC="wav export update"
	m_bChannelMode = IsDlgButtonChecked(IDC_CHECK4) ? TRUE : FALSE;
// -! NEW_FEATURE#0024

	// WaveFormatEx
	DWORD dwFormat = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());
	WaveFormat.Format.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.Format.nSamplesPerSec = m_CbnSampleRate.GetItemData(m_CbnSampleRate.GetCurSel());
	if (WaveFormat.Format.nSamplesPerSec < 11025) WaveFormat.Format.nSamplesPerSec = 11025;
	if (WaveFormat.Format.nSamplesPerSec > 96000) WaveFormat.Format.nSamplesPerSec = 96000; //ericus' fix: 44100 -> 96000
	WaveFormat.Format.nChannels = (WORD)(dwFormat >> 8);
	if ((WaveFormat.Format.nChannels != 1) && (WaveFormat.Format.nChannels != 4)) WaveFormat.Format.nChannels = 2;
	WaveFormat.Format.wBitsPerSample = (WORD)(dwFormat & 0xFF);

// -> CODE#0024
// -> DESC="wav export update"
//	if ((WaveFormat.Format.wBitsPerSample != 8) && (WaveFormat.Format.wBitsPerSample != 32)) WaveFormat.Format.wBitsPerSample = 16;
	if ((WaveFormat.Format.wBitsPerSample != 8) && (WaveFormat.Format.wBitsPerSample != 24) && (WaveFormat.Format.wBitsPerSample != 32)) WaveFormat.Format.wBitsPerSample = 16;
// -! NEW_FEATURE#0024

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
		const GUID guid_MEDIASUBTYPE_PCM = {0x00000001, 0x0000, 0x0010, 0x80, 0x00, 0x0, 0xAA, 0x0, 0x38, 0x9B, 0x71};
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
	DDX_Control(pDX, IDC_EDIT4,			m_EditCopyright);
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
	memset(&afd, 0, sizeof(afd));
	memset(pwfx, 0, sizeof(wfx));
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
	theApp.AcmFormatEnum(NULL, &afd, AcmFormatEnumCB, (DWORD)this, ACM_FORMATENUMF_CONVERT);
	m_bDriversEnumerated = TRUE;
	m_CbnDriver.SetCurSel(m_nDriverIndex);
	if (m_bSaveInfoField) CheckDlgButton(IDC_CHECK3, MF_CHECKED);
	for (UINT iGnr=0; iGnr<NUM_GENRES; iGnr++)
	{
		m_CbnGenre.SetItemData(m_CbnGenre.AddString(gpszGenreNames[iGnr]), iGnr);
	}
	UINT nSel = m_CbnGenre.AddString("Unspecified");
	m_CbnGenre.SetItemData(nSel, 0xff);
	m_CbnGenre.SetCurSel(nSel);
	m_EditYear.SetWindowText("2000");
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
	memset(&afd, 0, sizeof(afd));
	memset(wfx, 0, sizeof(wfx));
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
	theApp.AcmFormatEnum(NULL, &afd, AcmFormatEnumCB, (DWORD)this, ACM_FORMATENUMF_CONVERT);
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
		memset(&add, 0, sizeof(add));
		add.cbStruct = sizeof(add);
		if (theApp.AcmDriverDetails(hdid, &add, 0L) == MMSYSERR_NOERROR)
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
// -> CODE#0024
// -> DESC="wav export update"
//	 && (pafd->pwfx->nSamplesPerSec <= 48000)
// -! NEW_FEATURE#0024
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
	CHAR s[40];

	if (m_dwFileLimit) m_dwFileLimit = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
	if (m_dwSongLimit) m_dwSongLimit = GetDlgItemInt(IDC_EDIT2, NULL, FALSE);
	m_nFormatIndex = m_CbnFormat.GetItemData(m_CbnFormat.GetCurSel());
	m_nDriverIndex = m_CbnDriver.GetItemData(m_CbnDriver.GetCurSel());
	m_bSaveInfoField = IsDlgButtonChecked(IDC_CHECK3);
	memset(&m_id3tag, 0, sizeof(m_id3tag));
	m_id3tag.tag[0] = 'T'; m_id3tag.tag[1] = 'A'; m_id3tag.tag[2] = 'G';
	memset(s, 0, sizeof(s)); m_pSndFile->GetTitle(s); s[30] = 0;
	memcpy(m_id3tag.title, s, 30);
	memset(s, 0, sizeof(s)); m_EditAuthor.GetWindowText(s, 31);
	memcpy(m_id3tag.artist, s, 30);
	memset(s, 0, sizeof(s)); m_EditCopyright.GetWindowText(s, 31);
	memcpy(m_id3tag.comments, s, 30);
	memset(s, 0, sizeof(s)); m_EditAlbum.GetWindowText(s, 31);
	memcpy(m_id3tag.album, s, 30);
	strcpy(s, "2000");
	m_EditYear.GetWindowText(s, 5);
	if (!s[0]) strcpy(s, "2000");
	memcpy(m_id3tag.year, s, 4);
	m_id3tag.genre = (BYTE)m_CbnGenre.GetItemData(m_CbnGenre.GetCurSel());
	for (UINT i=0; i<30; i++)
	{
		if (m_id3tag.title[i] == 0) m_id3tag.title[i] = ' ';
		if (m_id3tag.artist[i] == 0) m_id3tag.artist[i] = ' ';
		if (m_id3tag.album[i] == 0) m_id3tag.album[i] = ' ';
		if (m_id3tag.comments[i] == 0) m_id3tag.comments[i] = ' ';
	}
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

// -> CODE#0024
// -> DESC="wav export update"
//#define WAVECONVERTBUFSIZE	2048
#define WAVECONVERTBUFSIZE	MIXBUFFERSIZE //Going over MIXBUFFERSIZE can kill VSTPlugs 
// -! NEW_FEATURE#0024


void CDoWaveConvert::OnButton1()
//------------------------------
{
	FILE *f;
	WAVEFILEHEADER header;
	WAVEDATAHEADER datahdr, fmthdr;
	MSG msg;
	BYTE buffer[WAVECONVERTBUFSIZE];
	CHAR s[64];
	HWND progress = ::GetDlgItem(m_hWnd, IDC_PROGRESS1);
	UINT ok = IDOK, pos;
	ULONGLONG ullSamples, ullMaxSamples;
	DWORD dwDataOffset;
	LONG lMax = 256;

	if ((!m_pSndFile) || (!m_lpszFileName) || ((f = fopen(m_lpszFileName, "w+b")) == NULL))
	{
		::AfxMessageBox("Could not open file for writing. Is it open in another application?");
		EndDialog(IDCANCEL);
		return;
	}
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	int oldVol = m_pSndFile->GetMasterVolume();
	int nOldRepeat = m_pSndFile->GetRepeatCount();
	CSoundFile::gdwSoundSetup |= SNDMIX_DIRECTTODISK;
	if ((!m_dwFileLimit) && (!m_dwSongLimit)) CSoundFile::gdwSoundSetup |= SNDMIX_NOBACKWARDJUMPS;
	CSoundFile::gdwMixingFreq = m_pWaveFormat->nSamplesPerSec;
	CSoundFile::gnBitsPerSample = m_pWaveFormat->wBitsPerSample;
	CSoundFile::gnChannels = m_pWaveFormat->nChannels;
// -> CODE#0024
// -> DESC="wav export update"
//	if ((m_bNormalize) && (m_pWaveFormat->wBitsPerSample <= 16))
	if ((m_bNormalize) && (m_pWaveFormat->wBitsPerSample <= 24))
// -! NEW_FEATURE#0024
	{
		m_pSndFile->gnBitsPerSample = 24;
		m_pSndFile->SetAGC(FALSE);
		if (oldVol > 128) m_pSndFile->SetMasterVolume(128);
	} else
	{
		m_bNormalize = FALSE;
	}
	m_pSndFile->ResetChannels();
	CSoundFile::InitPlayer(TRUE);
	m_pSndFile->SetRepeatCount(0);
	if ((!m_dwFileLimit) || (m_dwFileLimit > 2047*1024)) m_dwFileLimit = 2047*1024; // 2GB
	m_dwFileLimit <<= 10;
	// File Header
	header.id_RIFF = IFFID_RIFF;
	header.filesize = sizeof(WAVEFILEHEADER) - 8;
	header.id_WAVE = IFFID_WAVE;
	// Wave Format Header
	fmthdr.id_data = IFFID_fmt;
	fmthdr.length = 16;
	if (m_pWaveFormat->cbSize) fmthdr.length += 2 + m_pWaveFormat->cbSize;
	header.filesize += sizeof(fmthdr) + fmthdr.length;
	// Data header
	datahdr.id_data = IFFID_data;
	datahdr.length = 0;
	header.filesize += sizeof(datahdr);
	// Writing Headers
	fwrite(&header, 1, sizeof(header), f);
	fwrite(&fmthdr, 1, sizeof(fmthdr), f);
	fwrite(m_pWaveFormat, 1, fmthdr.length, f);
	fwrite(&datahdr, 1, sizeof(datahdr), f);
	dwDataOffset = ftell(f);
	pos = 0;
	ullSamples = 0;
	ullMaxSamples = m_dwFileLimit / (m_pWaveFormat->nChannels * m_pWaveFormat->wBitsPerSample / 8);
	if (m_dwSongLimit)
	{
		ULONGLONG l = (ULONGLONG)m_dwSongLimit * m_pWaveFormat->nSamplesPerSec;
		if (l < ullMaxSamples) ullMaxSamples = l;
	}
	if (progress != NULL)
	{
		ULONGLONG max = ullMaxSamples;
		ULONGLONG l = ((ULONGLONG)m_pSndFile->GetSongTime()) * m_pWaveFormat->nSamplesPerSec;
		if (m_nMaxPatterns > 0)
		{
			DWORD dwOrds = m_pSndFile->GetNumPatterns();
			if ((m_nMaxPatterns < dwOrds) && (dwOrds > 0)) l = (l*m_nMaxPatterns) / dwOrds;
		}
		if (l < max) max = l;
		::SendMessage(progress, PBM_SETRANGE, 0, MAKELPARAM(0, (DWORD)(max >> 14)));
	}
	// Process the conversion
	UINT nBytesPerSample = (CSoundFile::gnBitsPerSample * CSoundFile::gnChannels) / 8;
	CMainFrame::GetMainFrame()->InitRenderer(m_pSndFile);	//rewbs.VSTTimeInfo
	for (UINT n=0; ; n++)
	{
		UINT lRead = m_pSndFile->Read(buffer, sizeof(buffer));
		if (!lRead) 
			break;
		ullSamples += lRead;
		if (m_bNormalize)
		{
			UINT imax = lRead*3*CSoundFile::gnChannels;
			for (UINT i=0; i<imax; i+=3)
			{
				LONG l = ((((buffer[i+2] << 8) + buffer[i+1]) << 8) + buffer[i]) << 8;
				l /= 256;
				if (l > lMax) lMax = l;
				if (-l > lMax) lMax = -l;
			}
		} else
		if (m_pWaveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
		{
			M2W_32ToFloat(buffer, lRead*(nBytesPerSample>>2));
		}
		UINT lWrite = fwrite(buffer, 1, lRead*nBytesPerSample, f);
		if (!lWrite) 
			break;
		datahdr.length += lWrite;
		if (m_bNormalize)
		{
			ULONGLONG d = ((ULONGLONG)datahdr.length * m_pWaveFormat->wBitsPerSample) / 24;
			if (d >= m_dwFileLimit) 
				break;
		} else
		{
			if (datahdr.length >= m_dwFileLimit) 
				break;
		}
		if (ullSamples >= ullMaxSamples) 
			break;
		if (!(n % 10))
		{
			DWORD l = (DWORD)(ullSamples / CSoundFile::gdwMixingFreq);
			wsprintf(s, "Writing file... (%uKB, %umn%02us)", datahdr.length >> 10, l / 60, l % 60);
			SetDlgItemText(IDC_TEXT1, s);
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
		DWORD dwLength = datahdr.length;
		DWORD percent = 0xFF, dwPos, dwSize, dwCount;
		DWORD dwBitSize, dwOutPos;

		dwPos = dwOutPos = dwDataOffset;
		dwBitSize = m_pWaveFormat->wBitsPerSample / 8;
		datahdr.length = 0;
		::SendMessage(progress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		dwCount = dwLength;
		while (dwCount >= 3)
		{
			dwSize = (sizeof(buffer) / 3) * 3;
			if (dwSize > dwCount) dwSize = dwCount;
			fseek(f, dwPos, SEEK_SET);
			if (fread(buffer, 1, dwSize, f) != dwSize) break;
			CSoundFile::Normalize24BitBuffer(buffer, dwSize, lMax, dwBitSize);
			fseek(f, dwOutPos, SEEK_SET);
			datahdr.length += (dwSize/3)*dwBitSize;
			fwrite(buffer, 1, (dwSize/3)*dwBitSize, f);
			dwCount -= dwSize;
			dwPos += dwSize;
			dwOutPos += (dwSize * m_pWaveFormat->wBitsPerSample) / 24;
			UINT k = (UINT)( ((ULONGLONG)(dwLength - dwCount) * 100) / dwLength );
			if (k != percent)
			{
				percent = k;
				wsprintf(s, "Normalizing... (%d%%)", percent);
				SetDlgItemText(IDC_TEXT1, s);
				::SendMessage(progress, PBM_SETPOS, percent, 0);
			}
		}
	}
	header.filesize = (sizeof(WAVEFILEHEADER)-8) + (8+fmthdr.length) + (8+datahdr.length);
	fseek(f, 0, SEEK_SET);
	fwrite(&header, sizeof(header), 1, f);
	fseek(f, dwDataOffset-sizeof(datahdr), SEEK_SET);
	fwrite(&datahdr, sizeof(datahdr), 1, f);
	fclose(f);
	CSoundFile::gdwSoundSetup &= ~(SNDMIX_DIRECTTODISK|SNDMIX_NOBACKWARDJUMPS);
	m_pSndFile->SetRepeatCount(nOldRepeat);
	m_pSndFile->m_nMaxOrderPosition = 0;
	if (m_bNormalize)
	{
		m_pSndFile->SetMasterVolume(oldVol);
		CFile fw;
		if (fw.Open(m_lpszFileName, CFile::modeReadWrite | CFile::modeNoTruncate))
		{
			fw.SetLength(header.filesize+8);
			fw.Close();
		}
	}
	CMainFrame::UpdateAudioParameters(TRUE);
//rewbs: reduce to normal priority during debug for easier hang debugging
#ifdef NDEBUG
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif 
#ifdef _DEBUG
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
#endif 
// -> CODE#0024	UPDATE#03
// -> DESC="wav export update"
//	EndDialog(IDOK);
	EndDialog(ok);
// -! NEW_FEATURE#0024
}


/////////////////////////////////////////////////////////////////////////////////////////
// CDoAcmConvert: save a mod as a compressed file

BEGIN_MESSAGE_MAP(CDoAcmConvert, CDialog)
	ON_COMMAND(IDC_BUTTON1,	OnButton1)
END_MESSAGE_MAP()


CDoAcmConvert::CDoAcmConvert(CSoundFile *sndfile, LPCSTR fname, PWAVEFORMATEX pwfx, HACMDRIVERID hadid, PTAGID3INFO pTag, CWnd *parent):
	CDialog(IDD_PROGRESS, parent)
//--------------------------------------------------------------------------------------------------------------------------------------
{
	m_pSndFile = sndfile;
	m_lpszFileName = fname;
	m_bAbort = FALSE;
	m_dwFileLimit = m_dwSongLimit = 0;
	m_pwfx = pwfx;
	m_hadid = hadid;
	m_bSaveInfoField = FALSE;
	memset(&m_id3tag, 0, sizeof(m_id3tag));
	if (pTag)
	{
		m_id3tag = *pTag;
		m_bSaveInfoField = TRUE;
	}
}


BOOL CDoAcmConvert::OnInitDialog()
//---------------------------------
{
	CDialog::OnInitDialog();
	SetWindowText("Encoding File...");
	PostMessage(WM_COMMAND, IDC_BUTTON1);
	return TRUE;
}


void CDoAcmConvert::OnButton1()
//-----------------------------
{
	BOOL bSaveWave;
	CHAR s[64], fext[_MAX_EXT];
	WAVEFILEHEADER wfh;
	WAVEDATAHEADER wdh, chunk;
	ACMSTREAMHEADER ash;
	WAVEFORMATEX wfxSrc;
	HACMDRIVER had;
	HACMSTREAM has;
	HWND progress;
	MSG msg;
	LPBYTE pcmBuffer, dstBuffer;
	UINT retval = IDCANCEL, pos, n;
	DWORD oldsndcfg, dwDstBufSize, dwMaxSamples, dwSamples, pcmBufSize, data_ofs;
	int oldrepeat;
	BOOL bPrepared, bFinished;
	FILE *f;

	had = NULL;
	has = NULL;
	dstBuffer = NULL;
	pcmBuffer = NULL;
	bPrepared = FALSE;
	progress = ::GetDlgItem(m_hWnd, IDC_PROGRESS1);
	if ((!m_pSndFile) || (!m_lpszFileName) || (!m_pwfx) || (!m_hadid)) goto OnError;
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	bSaveWave = FALSE;
	_splitpath(m_lpszFileName, NULL, NULL, NULL, fext);
	if (((m_bSaveInfoField) && (m_pwfx->wFormatTag != WAVE_FORMAT_MPEGLAYER3))
	 || (!lstrcmpi(fext, ".wav"))) bSaveWave = TRUE;
	memset(&wfxSrc, 0, sizeof(wfxSrc));
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
	if (theApp.AcmDriverOpen(&had, m_hadid, 0L) != MMSYSERR_NOERROR) goto OnError;
	if (theApp.AcmStreamOpen(&has, had, &wfxSrc, m_pwfx, NULL, 0L, 0L, ACM_STREAMOPENF_NONREALTIME) != MMSYSERR_NOERROR) goto OnError;
	if (theApp.AcmStreamSize(has, WAVECONVERTBUFSIZE, &dwDstBufSize, ACM_STREAMSIZEF_SOURCE) != MMSYSERR_NOERROR) goto OnError;
	if (dwDstBufSize > 0x10000) dwDstBufSize = 0x10000;
	pcmBuffer = (LPBYTE)GlobalAllocPtr(GHND, WAVECONVERTBUFSIZE);
	dstBuffer = (LPBYTE)GlobalAllocPtr(GHND, dwDstBufSize);
	if ((!dstBuffer) || (!pcmBuffer)) goto OnError;
	memset(&ash, 0, sizeof(ash));
	ash.cbStruct = sizeof(ash);
	ash.pbSrc = pcmBuffer;
	ash.cbSrcLength = WAVECONVERTBUFSIZE;
	ash.pbDst = dstBuffer;
	ash.cbDstLength = dwDstBufSize;
	if (theApp.AcmStreamPrepareHeader(has, &ash, 0L) != MMSYSERR_NOERROR) goto OnError;
	bPrepared = TRUE;
	// Creating the output file
	if ((f = fopen(m_lpszFileName, "wb")) == NULL) {
		::AfxMessageBox("Could not open file for writing. Is it open in another application?");
		goto OnError;
	}
	wfh.id_RIFF = IFFID_RIFF;
	wfh.id_WAVE = IFFID_WAVE;
	wfh.filesize = 0;
	wdh.id_data = IFFID_data;
	wdh.length = 0;
	data_ofs = 0;
	if (bSaveWave)
	{
		fwrite(&wfh, 1, sizeof(wfh), f);
		chunk.id_data = IFFID_fmt;
		chunk.length = sizeof(WAVEFORMATEX) + m_pwfx->cbSize;
		fwrite(&chunk, 1, sizeof(chunk), f);
		fwrite(m_pwfx, 1, chunk.length, f);
		wfh.filesize += sizeof(chunk) + chunk.length + 4;
		data_ofs = ftell(f);
		fwrite(&wdh, 1, sizeof(wdh), f);
		wfh.filesize += sizeof(wdh);
	}
	oldsndcfg = CSoundFile::gdwSoundSetup;
	oldrepeat = m_pSndFile->GetRepeatCount();
	CSoundFile::gdwMixingFreq = wfxSrc.nSamplesPerSec;
	CSoundFile::gnBitsPerSample = 16;
//	CSoundFile::SetResamplingMode(SRCMODE_POLYPHASE); //rewbs.resamplerConf - we don't want this anymore.
	CSoundFile::gnChannels = wfxSrc.nChannels;
	m_pSndFile->SetRepeatCount(0);
	m_pSndFile->ResetChannels();
	CSoundFile::InitPlayer(TRUE);
	CSoundFile::gdwSoundSetup |= SNDMIX_DIRECTTODISK;
	if ((!m_dwFileLimit) && (!m_dwSongLimit)) CSoundFile::gdwSoundSetup |= SNDMIX_NOBACKWARDJUMPS;
	// Setting up file limits and progress range
	if ((!m_dwFileLimit) || (m_dwFileLimit > 512000)) m_dwFileLimit = 512000;
	m_dwFileLimit <<= 10;
	dwMaxSamples = 60*60*wfxSrc.nSamplesPerSec; // 1 hour
	if (m_dwSongLimit)
	{
		DWORD l = m_dwSongLimit * wfxSrc.nSamplesPerSec;
		if (l < dwMaxSamples) dwMaxSamples = l;
	}
	if (progress != NULL)
	{
		DWORD max = dwMaxSamples;
		DWORD l = m_pSndFile->GetSongTime() * wfxSrc.nSamplesPerSec;
		if (l < max) max = l;
		::SendMessage(progress, PBM_SETRANGE, 0, MAKELPARAM(0, max >> 14));
	}
	dwSamples = 0;
	pos = 0;
	pcmBufSize = WAVECONVERTBUFSIZE;
	bFinished = FALSE;
	// Writing File
	CMainFrame::GetMainFrame()->InitRenderer(m_pSndFile);	//rewbs.VSTTimeInfo
	for (n=0; ; n++)
	{
		UINT lRead = 0;
		if (!bFinished)
		{
			lRead = m_pSndFile->Read(pcmBuffer + WAVECONVERTBUFSIZE - pcmBufSize, pcmBufSize);
			if (!lRead) bFinished = TRUE;
		}
		dwSamples += lRead;
		ash.cbSrcLength = lRead * wfxSrc.nBlockAlign + WAVECONVERTBUFSIZE - pcmBufSize;
		ash.cbDstLengthUsed = 0;
		if (theApp.AcmStreamConvert(has, &ash, (lRead) ? ACM_STREAMCONVERTF_BLOCKALIGN : ACM_STREAMCONVERTF_END) != MMSYSERR_NOERROR) break;
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
			UINT lWrite = fwrite(dstBuffer, 1, ash.cbDstLengthUsed, f);
			if (!lWrite) break;
			wdh.length += lWrite;
		}
		if ((!lRead) || (dwSamples >= dwMaxSamples) || (wdh.length >= m_dwFileLimit)) break;
		if (!(n % 10))
		{
			DWORD l = dwSamples / wfxSrc.nSamplesPerSec;
			wsprintf(s, "Writing file... (%uKB, %umn%02us)", wdh.length >> 10, l / 60, l % 60);
			SetDlgItemText(IDC_TEXT1, s);
		}
		if ((progress != NULL) && ((dwSamples >> 14) != pos))
		{
			pos = dwSamples >> 14;
			::SendMessage(progress, PBM_SETPOS, pos, 0);
		}
		if (m_bAbort) break;
	}
	CMainFrame::GetMainFrame()->StopRenderer(m_pSndFile);	//rewbs.VSTTimeInfo
	// Done
	CSoundFile::gdwSoundSetup = oldsndcfg;
	CSoundFile::gdwSoundSetup &= ~(SNDMIX_DIRECTTODISK|SNDMIX_NOBACKWARDJUMPS);
	m_pSndFile->SetRepeatCount(oldrepeat);
	m_pSndFile->m_nMaxOrderPosition = 0;
	CMainFrame::UpdateAudioParameters(TRUE);
	// Success
	if (bSaveWave)
	{
		if (m_bSaveInfoField)
		{
			WAVEFILEHEADER list;
			DWORD info_ofs, end_ofs;
			DWORD zero = 0;
			
			info_ofs = ftell(f);
			if (info_ofs & 1)
			{
				wdh.length++;
				fwrite(&zero, 1, 1, f);
				info_ofs++;
			}
			list.id_RIFF = IFFID_LIST;
			list.id_WAVE = IFFID_INFO;
			list.filesize = 4;
			fwrite(&list, 1, sizeof(list), f);
			// ICMT
			if (m_pSndFile->m_lpszSongComments)
			{
				CHAR *pszComments = NULL;
				UINT szlen = m_pSndFile->GetSongComments(NULL, 8192, 80);
				if (szlen > 1) pszComments = new CHAR[szlen+1];
				if (pszComments)
				{
					szlen = m_pSndFile->GetSongComments(pszComments, szlen, 80);
					pszComments[szlen] = 0;
					chunk.id_data = IFFID_ICMT;
					chunk.length = strlen(pszComments)+1;
					fwrite(&chunk, 1, sizeof(chunk), f);
					fwrite(pszComments, 1, chunk.length, f);
					list.filesize += chunk.length + sizeof(chunk);
					if (chunk.length & 1)
					{
						fwrite(&zero, 1, 1, f);
						list.filesize++;
					}
					delete pszComments;
				}
			}
			for (UINT iCmt=0; iCmt<=6; iCmt++)
			{
				s[0] = 0;
				switch(iCmt)
				{
				// INAM
				case 0: memcpy(s, m_id3tag.title, 30); s[30] = 0; chunk.id_data = IFFID_INAM; break;
				// IART
				case 1: memcpy(s, m_id3tag.artist, 30); s[30] = 0; chunk.id_data = IFFID_IART; break;
				// IPRD
				case 2: memcpy(s, m_id3tag.album, 30); s[30] = 0; chunk.id_data = IFFID_IPRD; break;
				// ICOP
				case 3: memcpy(s, m_id3tag.comments, 30); s[30] = 0; chunk.id_data = IFFID_ICOP; break;
				// IGNR
				case 4: if (m_id3tag.genre < NUM_GENRES) strcpy(s, gpszGenreNames[m_id3tag.genre]); chunk.id_data = IFFID_IGNR; break;
				// ISFT
				case 5: strcpy(s, "Modplug Tracker"); chunk.id_data = IFFID_ISFT; break;
				// ICRD
				case 6: memcpy(s, m_id3tag.year, 4); s[4] = 0; strcat(s, "-01-01"); if (s[0] <= '0') s[0] = 0; chunk.id_data = IFFID_ICRD; break;
				}
				int l = strlen(s);
				while ((l > 0) && (s[l-1] == ' ')) s[--l] = 0;
				if (s[0])
				{
					chunk.length = strlen(s)+1;
					fwrite(&chunk, 1, sizeof(chunk), f);
					fwrite(s, 1, chunk.length, f);
					list.filesize += chunk.length + sizeof(chunk);
					if (chunk.length & 1)
					{
						fwrite(&zero, 1, 1, f);
						list.filesize++;
					}
				}
			}
			// Update INFO size
			end_ofs = ftell(f);
			fseek(f, info_ofs, SEEK_SET);
			fwrite(&list, 1, sizeof(list), f);
			fseek(f, end_ofs, SEEK_SET);
			wfh.filesize += list.filesize + 8;
		}
		wfh.filesize += wdh.length;
		fseek(f, 0, SEEK_SET);
		fwrite(&wfh, 1, sizeof(wfh), f);
		if (data_ofs > 0)
		{
			fseek(f, data_ofs, SEEK_SET);
			fwrite(&wdh, 1, sizeof(wdh), f);
		}
	} else
	if (m_bSaveInfoField)
	{
		fwrite(&m_id3tag, 1, sizeof(m_id3tag), f);
	}
	fclose(f);
	if (!m_bAbort) retval = IDOK;
OnError:
	if (bPrepared) theApp.AcmStreamUnprepareHeader(has, &ash, 0L);
	if (has != NULL) theApp.AcmStreamClose(has, 0L);
	if (had != NULL) theApp.AcmDriverClose(had, 0L);
	if (pcmBuffer) GlobalFreePtr(pcmBuffer);
	if (dstBuffer) GlobalFreePtr(dstBuffer);
//rewbs: reduce to normal priority during debug for easier hang debugging
#ifdef NDEBUG
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif 
#ifdef _DEBUG
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
#endif 
	EndDialog(retval);
}


/*

TEXT("IARL"), TEXT("Archival Location"),  TEXT("Indicates where the subject of the file is archived."),
TEXT("IART"), TEXT("Artist"),             TEXT("Lists the artist of the original subject of the file. For example, \"Michaelangelo.\""),
TEXT("ICMS"), TEXT("Commissioned"),       TEXT("Lists the name of the person or organization that commissioned the subject of the file."
											" For example, \"Pope Julian II.\""),
TEXT("ICMT"), TEXT("Comments"),           TEXT("Provides general comments about the file or the subject of the file. If the comment is "
											"several sentences long, end each sentence with a period. Do not include newline characters."),
TEXT("ICOP"), TEXT("Copyright"),          TEXT("Records the copyright information for the file. For example, \"Copyright Encyclopedia "
											"International 1991.\" If there are multiple copyrights, separate them by a semicolon followed "
											"by a space."),
TEXT("ICRD"), TEXT("Creation date"),      TEXT("Specifies the date the subject of the file was created. List dates in year-month-day "
											"format, padding one-digit months and days with a zero on the left. For example, "
											"'1553-05-03' for May 3, 1553."),
TEXT("IENG"), TEXT("Engineer"),           TEXT("Stores the name of the engineer who worked on the file. If there are multiple engineers, "
											"separate the names by a semicolon and a blank. For example, \"Smith, John; Adams, Joe.\""),
TEXT("IGNR"), TEXT("Genre"),              TEXT("Describes the original work, such as, \"landscape,\" \"portrait,\" \"still life,\" etc."),
TEXT("IKEY"), TEXT("Keywords"),           TEXT("Provides a list of keywords that refer to the file or subject of the file. Separate "
											"multiple keywords with a semicolon and a blank. For example, \"Seattle; aerial view; scenery.\""),
TEXT("IMED"), TEXT("Medium"),             TEXT("Describes the original subject of the file, such as, \"computer image,\" \"drawing,\""
											"\"lithograph,\" and so forth."),
TEXT("INAM"), TEXT("Name"),               TEXT("Stores the title of the subject of the file, such as, \"Seattle From Above.\""),
TEXT("IPRD"), TEXT("Product"),            TEXT("Specifies the name of the title the file was originally intended for, such as \"Encyclopedia"
											" of Pacific Northwest Geography.\""),
TEXT("ISBJ"), TEXT("Subject"),            TEXT("Describes the contents of the file, such as \"Aerial view of Seattle.\""),
TEXT("ISFT"), TEXT("Software"),           TEXT("Identifies the name of the software package used to create the file, such as "
											"\"Microsoft WaveEdit.\""),
TEXT("ISRC"), TEXT("Source"),             TEXT("Identifies the name of the person or organization who supplied the original subject of the file."
											" For example, \"Trey Research.\""),
TEXT("ITCH"), TEXT("Technician"),         TEXT("Identifies the technician who digitized the subject file. For example, \"Smith, John.\""),

*/
