// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "mptrack.h"
#include "MainFrm.h"
#include "snddev.h"
#include "moddoc.h"
#include "childfrm.h"
#include "dlsbank.h"
#include "mpdlgs.h"
#include "moptions.h"
#include "vstplug.h"
#include "KeyConfigDlg.h"
#include ".\mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAINFRAME_REGKEY		"Software\\Olivier Lapicque\\ModPlug Tracker"
#define MAINFRAME_REG_WINDOW	"Software\\Olivier Lapicque\\ModPlug Tracker\\Window"
#define MAINFRAME_REG_SETTINGS	"Software\\Olivier Lapicque\\ModPlug Tracker\\Settings"


#define MPTTIMER_PERIOD		200

extern void MPT_LoadPatternState(LPCSTR pszSection);
extern void MPT_SavePatternState(LPCSTR pszSection);
extern UINT gnMidiImportSpeed;
extern UINT gnMidiPatternLen;


//========================================
class CMPTSoundSource: public ISoundSource
//========================================
{
public:
	CMPTSoundSource() {}
	ULONG AudioRead(PVOID pData, ULONG cbSize);
	VOID AudioDone(ULONG dwSize, ULONG dwLatency);
};


CMPTSoundSource gMPTSoundSource;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_WM_CREATE()
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_VIEW_OPTIONS,				OnViewOptions)
	//ON_COMMAND(ID_HELP,						CMDIFrameWnd::OnHelp)
	ON_COMMAND(ID_HELP_FINDER,				CMDIFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_REPORT_BUG,				OnReportBug)
	ON_COMMAND(ID_CONTEXT_HELP,				CMDIFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP,				CMDIFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_NEXTOCTAVE,				OnNextOctave)
	ON_COMMAND(ID_PREVOCTAVE,				OnPrevOctave)
	ON_COMMAND(ID_ADD_SOUNDBANK,			OnAddDlsBank)
	ON_COMMAND(ID_IMPORT_MIDILIB,			OnImportMidiLib)
	ON_COMMAND(ID_MIDI_RECORD,				OnMidiRecord)
	ON_COMMAND(ID_PLAYER_PAUSE,				OnPlayerPause)
	ON_COMMAND_EX(IDD_TREEVIEW,				OnBarCheck)
	ON_COMMAND_EX(ID_NETLINK_MODPLUG,		OnInternetLink)
	ON_COMMAND_EX(ID_NETLINK_UT,			OnInternetLink)
	ON_COMMAND_EX(ID_NETLINK_OSMUSIC,	OnInternetLink)
	ON_COMMAND_EX(ID_NETLINK_HANDBOOK,		OnInternetLink)
	ON_COMMAND_EX(ID_NETLINK_FORUMS,		OnInternetLink)
	ON_COMMAND_EX(ID_NETLINK_PLUGINS,		OnInternetLink)
	ON_CBN_SELCHANGE(IDC_COMBO_BASEOCTAVE,	OnOctaveChanged)
	ON_UPDATE_COMMAND_UI(ID_MIDI_RECORD,	OnUpdateMidiRecord)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TIME,	OnUpdateTime)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_USER,	OnUpdateUser)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INFO,	OnUpdateInfo)
	ON_UPDATE_COMMAND_UI(IDD_TREEVIEW,		OnUpdateControlBarMenu)
	ON_MESSAGE(WM_MOD_UPDATEPOSITION,		OnUpdatePosition)
	ON_MESSAGE(WM_MOD_INVALIDATEPATTERNS,	OnInvalidatePatterns)
	ON_MESSAGE(WM_MOD_SPECIALKEY,			OnSpecialKey)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,	OnCustomKeyMsg)
	//}}AFX_MSG_MAP
	ON_WM_INITMENU()
END_MESSAGE_MAP()

// Static
static gdwLastLowLatencyTime = 0;
static gdwLastMixActiveTime = 0;
static DWORD gsdwTotalSamples = 0;
static DWORD gdwPlayLatency = 0;
// Globals
DWORD CMainFrame::gdwNotificationType = MPTNOTIFY_DEFAULT;
UINT CMainFrame::m_nFilterIndex = 0;
UINT CMainFrame::m_nLastOptionsPage = 0;
BOOL CMainFrame::gbMdiMaximize = FALSE;
//rewbs.varWindowSize
LONG CMainFrame::glCtrlWindowHeight = 188; //obsolete, for backwards compat only
LONG CMainFrame::glGeneralWindowHeight = 188;
LONG CMainFrame::glPatternWindowHeight = 159;
LONG CMainFrame::glSampleWindowHeight = 188;
LONG CMainFrame::glInstrumentWindowHeight = 188;
LONG CMainFrame::glCommentsWindowHeight = 288;
//end rewbs.varWindowSize
LONG CMainFrame::glTreeWindowWidth = 160;
LONG CMainFrame::glTreeSplitRatio = 128;
HHOOK CMainFrame::ghKbdHook = NULL;
DWORD CMainFrame::gdwPreviousVersion = 0x01000000;
DWORD CMainFrame::gnHotKeyMask = 0;
// Audio Setup
CRITICAL_SECTION CMainFrame::m_csAudio;
HANDLE CMainFrame::m_hPlayThread = NULL;
DWORD CMainFrame::m_dwPlayThreadId = 0;
HANDLE CMainFrame::m_hAudioWakeUp = NULL;
HANDLE CMainFrame::m_hNotifyThread = NULL;
DWORD CMainFrame::m_dwNotifyThreadId = 0;
HANDLE CMainFrame::m_hNotifyWakeUp = NULL;
ISoundDevice *CMainFrame::gpSoundDevice = NULL;
LONG CMainFrame::slSampleSize = 2;
LONG CMainFrame::sdwSamplesPerSec = 44100;
LONG CMainFrame::sdwAudioBufferSize = MAX_AUDIO_BUFFERSIZE;
UINT CMainFrame::gdwIdleTime = 0;
DWORD CMainFrame::m_dwRate = 44100;
DWORD CMainFrame::m_dwSoundSetup = SOUNDSETUP_SECONDARY;
DWORD CMainFrame::m_nChannels = 2;
DWORD CMainFrame::m_dwQuality = 0;
DWORD CMainFrame::m_nSrcMode = SRCMODE_LINEAR;
DWORD CMainFrame::m_nBitsPerSample = 16;
DWORD CMainFrame::m_nPreAmp = 128;
DWORD CMainFrame::gbLoopSong = TRUE;
LONG CMainFrame::m_nWaveDevice = (SNDDEV_DSOUND<<8);
LONG CMainFrame::m_nMidiDevice = 0;
DWORD CMainFrame::m_nBufferLength = 75;
LONG CMainFrame::gnLVuMeter = 0;
LONG CMainFrame::gnRVuMeter = 0;
EQPRESET CMainFrame::m_EqSettings = { "", {16,16,16,16,16,16}, { 125, 300, 600, 1250, 4000, 8000 } };
// Midi Setup
DWORD CMainFrame::m_dwMidiSetup = MIDISETUP_RECORDVELOCITY|MIDISETUP_RECORDNOTEOFF;
// Pattern Setup
DWORD CMainFrame::m_dwPatternSetup = PATTERN_PLAYNEWNOTE | PATTERN_EFFECTHILIGHT
								   | PATTERN_SMALLFONT | PATTERN_CENTERROW | PATTERN_AUTOSPACEBAR
								   | PATTERN_DRAGNDROPEDIT | PATTERN_FLATBUTTONS;
DWORD CMainFrame::m_nRowSpacing = 16;
DWORD CMainFrame::m_nRowSpacing2 = 4;
DWORD CMainFrame::m_nKeyboardCfg = KEYBOARD_MPT;
DWORD CMainFrame::CustomKeys[MAX_MPTHOTKEYS];
// US Keyboard Maps
DWORD CMainFrame::KeyboardMap[KEYBOARDMAP_LENGTH];
DWORD CMainFrame::KeyboardFT2[KEYBOARDMAP_LENGTH] =
{
	44,31,45,32,46,47,34,48,35,49,36,50,	// ZSXDCVGBHNJM
	16, 3,17, 4,18,19, 6,20, 7,21, 8,22,	// Q2W3ER5T6Y7U
	23,10,24,11,25, 0, 0, 0, 0, 0, 0, 0,	// I9O0P
	41,13	// `=
};
DWORD CMainFrame::KeyboardIT[KEYBOARDMAP_LENGTH] =
{
	44,31,45,32,46,47,34,48,35,49,36,50,	// ZSXDCVGBHNJM
	16, 3,17, 4,18,19, 6,20, 7,21, 8,22,	// Q2W3ER5T6Y7U
	23,10,24,11,25, 0, 0, 0, 0, 0, 0, 0,	// I9O0P
	2,41	// 1`
};
DWORD CMainFrame::KeyboardMPT[KEYBOARDMAP_LENGTH] =
{	16,17,18,19,20,21,22,23,24,25,26,27,	// QWERTYUIOP[]
	30,31,32,33,34,35,36,37,38,39,40,43,	// ASDFGHJKL;:/
	44,45,46,47,48,49,50,51,52,53,0,0,
	41,13	// `=
};
	 
// GDI
HICON CMainFrame::m_hIcon = NULL;
HFONT CMainFrame::m_hGUIFont = NULL;
HFONT CMainFrame::m_hFixedFont = NULL;
HFONT CMainFrame::m_hLargeFixedFont = NULL;
HPEN CMainFrame::penDarkGray = NULL;
HPEN CMainFrame::penScratch = NULL;
HPEN CMainFrame::penGray00 = NULL;
HPEN CMainFrame::penGray33 = NULL;
HPEN CMainFrame::penGray40 = NULL;
HPEN CMainFrame::penGray55 = NULL;
HPEN CMainFrame::penGray80 = NULL;
HPEN CMainFrame::penGray99 = NULL;
HPEN CMainFrame::penGraycc = NULL;
HPEN CMainFrame::penGrayff = NULL;
HPEN CMainFrame::penLightGray = NULL;
HPEN CMainFrame::penBlack = NULL;
HPEN CMainFrame::penWhite = NULL;
HPEN CMainFrame::penHalfDarkGray = NULL;
HPEN CMainFrame::penSample = NULL;
HPEN CMainFrame::penEnvelope = NULL;
HPEN CMainFrame::penSeparator = NULL;
HBRUSH CMainFrame::brushGray = NULL;
HBRUSH CMainFrame::brushBlack = NULL;
HBRUSH CMainFrame::brushWhite = NULL;
CBrush *CMainFrame::pbrushBlack = NULL;
CBrush *CMainFrame::pbrushWhite = NULL;

HBRUSH CMainFrame::brushHighLight = NULL;
HBRUSH CMainFrame::brushWindow = NULL;
HCURSOR CMainFrame::curDragging = NULL;
HCURSOR CMainFrame::curArrow = NULL;
HCURSOR CMainFrame::curNoDrop = NULL;
HCURSOR CMainFrame::curNoDrop2 = NULL;
HCURSOR CMainFrame::curVSplit = NULL;
LPMODPLUGDIB CMainFrame::bmpPatterns = NULL;
LPMODPLUGDIB CMainFrame::bmpNotes = NULL;
LPMODPLUGDIB CMainFrame::bmpVUMeters = NULL;
LPMODPLUGDIB CMainFrame::bmpVisNode = NULL;  //rewbs.fxVis
HPEN CMainFrame::gpenVuMeter[NUM_VUMETER_PENS*2];
COLORREF CMainFrame::rgbCustomColors[MAX_MODCOLORS] = 
	{
		0xFFFFFF, 0x000000, 0xC0C0C0, 0x000000, 0x000000, 0xFFFFFF, 0x0000FF,
		0x80FFFF, 0x000000, 0xE0E8E0,
		// Effect Colors
		0x800000, 0x808000,	0x008000, 0x808000, 0x008080, 0x000080, 0xFF0000,
		// VU-Meters
		0x00FF00, 0x00FFFF, 0x0000FF,
	};

// Arrays
CHAR CMainFrame::m_szModDir[_MAX_PATH] = "";
CHAR CMainFrame::m_szSmpDir[_MAX_PATH] = "";
CHAR CMainFrame::m_szInsDir[_MAX_PATH] = "";
CHAR CMainFrame::m_szKbdFile[_MAX_PATH] = "";		//rewbs.customKeys
CHAR CMainFrame::m_szCurModDir[_MAX_PATH] = "";
CHAR CMainFrame::m_szCurSmpDir[_MAX_PATH] = "";
CHAR CMainFrame::m_szCurInsDir[_MAX_PATH] = "";
//CHAR CMainFrame::m_szCurKbdFile[_MAX_PATH] = "";		//rewbs.customKeys

CInputHandler *CMainFrame::m_InputHandler = NULL;

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_INFO,
	ID_INDICATOR_USER,
	ID_INDICATOR_TIME
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
//----------------------
{
	DWORD dwREG_DWORD = REG_DWORD;
	DWORD dwREG_SZ = REG_SZ;
	DWORD dwDWORDSize = sizeof(UINT);
	DWORD dwSZSIZE = sizeof(m_szModDir);
	DWORD dwCRSIZE = sizeof(COLORREF);
	HKEY key;
	m_bModTreeHasFocus = false;
	m_pNoteMapHasFocus = NULL;
	m_bOptionsLocked = false;

	m_pModPlaying = NULL;
	m_hFollowSong = NULL;
	m_hWndMidi = NULL;
	m_pSndFile = NULL;
	m_dwStatus = 0;
	m_dwElapsedTime = 0;
	m_dwTimeSec = 0;
	m_dwLastPluginIdleCall=0;
	m_dwNotifyType = 0;
	m_nTimer = 0;
	m_nAvgMixChn = m_nMixChn = 0;
	m_szUserText[0] = 0;
	m_szInfoText[0] = 0;
	m_szPluginsDir[0] = 0;
	m_szExportDir[0] = 0;
	memset(gpenVuMeter, 0, sizeof(gpenVuMeter));
	memset(Chords, 0, sizeof(Chords));
	memcpy(KeyboardMap, KeyboardMPT, sizeof(KeyboardMap));
	// Create Audio Critical Section
	memset(&m_csAudio, 0, sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(&m_csAudio);
	// Default chords
	for (UINT ichord=0; ichord<3*12; ichord++)
	{
		Chords[ichord].key = (BYTE)ichord;
		Chords[ichord].notes[0] = 0;
		Chords[ichord].notes[1] = 0;
		Chords[ichord].notes[2] = 0;
		// Major Chords
		if (ichord < 12)
		{
			Chords[ichord].notes[0] = (BYTE)(ichord+5);
			Chords[ichord].notes[1] = (BYTE)(ichord+8);
			Chords[ichord].notes[2] = (BYTE)(ichord+11);
		} else
		// Minor Chords
		if (ichord < 24)
		{
			Chords[ichord].notes[0] = (BYTE)(ichord-8);
			Chords[ichord].notes[1] = (BYTE)(ichord-4);
			Chords[ichord].notes[2] = (BYTE)(ichord-1);
		}
	}
	// Read Display Registry Settings
	if (RegOpenKeyEx(HKEY_CURRENT_USER,	MAINFRAME_REG_WINDOW, 0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		DWORD d = 0;
		RegQueryValueEx(key, "Maximized", NULL, &dwREG_DWORD, (LPBYTE)&d, &dwDWORDSize);
		if (d) theApp.m_nCmdShow = SW_SHOWMAXIMIZED;
		RegQueryValueEx(key, "MDIMaximize", NULL, &dwREG_DWORD, (LPBYTE)&gbMdiMaximize, &dwDWORDSize);
		RegQueryValueEx(key, "MDITreeWidth", NULL, &dwREG_DWORD, (LPBYTE)&glTreeWindowWidth, &dwDWORDSize);
		//rewbs.varWindowSize
		RegQueryValueEx(key, "MDICtrlHeight", NULL, &dwREG_DWORD, (LPBYTE)&glCtrlWindowHeight, &dwDWORDSize); //obsolete, for backwards compat only
		RegQueryValueEx(key, "MDIGeneralHeight", NULL, &dwREG_DWORD, (LPBYTE)&glGeneralWindowHeight, &dwDWORDSize);
		RegQueryValueEx(key, "MDIPatternHeight", NULL, &dwREG_DWORD, (LPBYTE)&glPatternWindowHeight, &dwDWORDSize);
		RegQueryValueEx(key, "MDISampleHeight", NULL, &dwREG_DWORD,  (LPBYTE)&glSampleWindowHeight, &dwDWORDSize);
		RegQueryValueEx(key, "MDIInstrumentHeight", NULL, &dwREG_DWORD,  (LPBYTE)&glInstrumentWindowHeight, &dwDWORDSize);
		RegQueryValueEx(key, "MDICommentsHeight", NULL, &dwREG_DWORD,  (LPBYTE)&glCommentsWindowHeight, &dwDWORDSize);
		//end rewbs.varWindowSize
		RegQueryValueEx(key, "MDITreeRatio", NULL, &dwREG_DWORD, (LPBYTE)&glTreeSplitRatio, &dwDWORDSize);
		// Colors
		for (int ncol=0; ncol<MAX_MODCOLORS; ncol++)
		{
			CHAR s[64];
			wsprintf(s, "Color%02d", ncol);
			RegQueryValueEx(key, s, NULL, &dwREG_DWORD, (LPBYTE)&rgbCustomColors[ncol], &dwCRSIZE);
		}
		RegCloseKey(key);
	}
	CSoundFile::m_nMaxMixChannels = 32;
	if (CSoundFile::gdwSysInfo & SYSMIX_ENABLEMMX) CSoundFile::m_nMaxMixChannels = 64;
	if (CSoundFile::gdwSysInfo & SYSMIX_SSE) CSoundFile::m_nMaxMixChannels = MAX_CHANNELS;
	// Read registry settings
	if (RegOpenKeyEx(HKEY_CURRENT_USER,	MAINFRAME_REGKEY, 0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		RegQueryValueEx(key, "SoundSetup", NULL, &dwREG_DWORD, (LPBYTE)&m_dwSoundSetup, &dwDWORDSize);
		RegQueryValueEx(key, "Quality", NULL, &dwREG_DWORD, (LPBYTE)&m_dwQuality, &dwDWORDSize);
		RegQueryValueEx(key, "SrcMode", NULL, &dwREG_DWORD, (LPBYTE)&m_nSrcMode, &dwDWORDSize);
		RegQueryValueEx(key, "Mixing_Rate", NULL, &dwREG_DWORD, (LPBYTE)&m_dwRate, &dwDWORDSize);
		RegQueryValueEx(key, "BufferLength", NULL, &dwREG_DWORD, (LPBYTE)&m_nBufferLength, &dwDWORDSize);
		if ((m_nBufferLength < 10) || (m_nBufferLength > 200)) m_nBufferLength = 100;
		RegQueryValueEx(key, "PreAmp", NULL, &dwREG_DWORD, (LPBYTE)&m_nPreAmp, &dwDWORDSize);
		RegQueryValueEx(key, "Songs_Directory", NULL, &dwREG_SZ, (LPBYTE)m_szModDir, &dwSZSIZE);
		dwSZSIZE = sizeof(m_szSmpDir);
		RegQueryValueEx(key, "Samples_Directory", NULL, &dwREG_SZ, (LPBYTE)m_szSmpDir, &dwSZSIZE);
		dwSZSIZE = sizeof(m_szInsDir);
		RegQueryValueEx(key, "Instruments_Directory", NULL, &dwREG_SZ, (LPBYTE)m_szInsDir, &dwSZSIZE);
		dwSZSIZE = sizeof(m_szPluginsDir);
		RegQueryValueEx(key, "Plugins_Directory", NULL, &dwREG_SZ, (LPBYTE)m_szPluginsDir, &dwSZSIZE);
		dwSZSIZE = sizeof(m_szKbdFile);																	//rewbs.custKeys
		RegQueryValueEx(key, "Key_Config_File", NULL, &dwREG_SZ, (LPBYTE)m_szKbdFile, &dwSZSIZE);	//rewbs.custKeys
		RegQueryValueEx(key, "XBassDepth", NULL, &dwREG_DWORD, (LPBYTE)&CSoundFile::m_nXBassDepth, &dwDWORDSize);
		RegQueryValueEx(key, "XBassRange", NULL, &dwREG_DWORD, (LPBYTE)&CSoundFile::m_nXBassRange, &dwDWORDSize);
		RegQueryValueEx(key, "ReverbDepth", NULL, &dwREG_DWORD, (LPBYTE)&CSoundFile::m_nReverbDepth, &dwDWORDSize);
		RegQueryValueEx(key, "ReverbType", NULL, &dwREG_DWORD, (LPBYTE)&CSoundFile::gnReverbType, &dwDWORDSize);
		RegQueryValueEx(key, "ProLogicDepth", NULL, &dwREG_DWORD, (LPBYTE)&CSoundFile::m_nProLogicDepth, &dwDWORDSize);
		RegQueryValueEx(key, "ProLogicDelay", NULL, &dwREG_DWORD, (LPBYTE)&CSoundFile::m_nProLogicDelay, &dwDWORDSize);
		RegQueryValueEx(key, "StereoSeparation", NULL, &dwREG_DWORD, (LPBYTE)&CSoundFile::m_nStereoSeparation, &dwDWORDSize);
		RegQueryValueEx(key, "MixChannels", NULL, &dwREG_DWORD, (LPBYTE)&CSoundFile::m_nMaxMixChannels, &dwDWORDSize);
		RegQueryValueEx(key, "WaveDevice", NULL, &dwREG_DWORD, (LPBYTE)&m_nWaveDevice, &dwDWORDSize);
		RegQueryValueEx(key, "MidiSetup", NULL, &dwREG_DWORD, (LPBYTE)&m_dwMidiSetup, &dwDWORDSize);
		RegQueryValueEx(key, "MidiDevice", NULL, &dwREG_DWORD, (LPBYTE)&m_nMidiDevice, &dwDWORDSize);
		RegQueryValueEx(key, "PatternSetup", NULL, &dwREG_DWORD, (LPBYTE)&m_dwPatternSetup, &dwDWORDSize);
		RegQueryValueEx(key, "RowSpacing", NULL, &dwREG_DWORD, (LPBYTE)&m_nRowSpacing, &dwDWORDSize);
		RegQueryValueEx(key, "RowSpacing2", NULL, &dwREG_DWORD, (LPBYTE)&m_nRowSpacing2, &dwDWORDSize);
		//RegQueryValueEx(key, "KeyboardCfg", NULL, &dwREG_DWORD, (LPBYTE)&m_nKeyboardCfg, &dwDWORDSize);
		RegQueryValueEx(key, "LoopSong", NULL, &dwREG_DWORD, (LPBYTE)&gbLoopSong, &dwDWORDSize);
		RegQueryValueEx(key, "BitsPerSample", NULL, &dwREG_DWORD, (LPBYTE)&m_nBitsPerSample, &dwDWORDSize);
		RegQueryValueEx(key, "ChannelMode", NULL, &dwREG_DWORD, (LPBYTE)&m_nChannels, &dwDWORDSize);
		RegQueryValueEx(key, "MidiImportSpeed", NULL, &dwREG_DWORD, (LPBYTE)&gnMidiImportSpeed, &dwDWORDSize);
		RegQueryValueEx(key, "MidiImportPatLen", NULL, &dwREG_DWORD, (LPBYTE)&gnMidiPatternLen, &dwDWORDSize);
		// EQ
		CEQSetupDlg::LoadEQ(key, "EQ_Settings", &m_EqSettings);
		CEQSetupDlg::LoadEQ(key, "EQ_User1", &CEQSetupDlg::gUserPresets[0]);
		CEQSetupDlg::LoadEQ(key, "EQ_User2", &CEQSetupDlg::gUserPresets[1]);
		CEQSetupDlg::LoadEQ(key, "EQ_User3", &CEQSetupDlg::gUserPresets[2]);
		CEQSetupDlg::LoadEQ(key, "EQ_User4", &CEQSetupDlg::gUserPresets[3]);
		RegCloseKey(key);
	}
	// Read more registry settings
	if (RegOpenKeyEx(HKEY_CURRENT_USER,	MAINFRAME_REG_SETTINGS, 0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		// Version
		dwDWORDSize = sizeof(DWORD);
		RegQueryValueEx(key, "Version", NULL, &dwREG_DWORD, (LPBYTE)&gdwPreviousVersion, &dwDWORDSize);
		RegCloseKey(key);
	}
	if (gdwPreviousVersion < MPTRACK_FINALRELEASEVERSION)
	{
		m_dwPatternSetup |= (PATTERN_EFFECTHILIGHT | PATTERN_SMALLFONT | PATTERN_CENTERROW | PATTERN_DRAGNDROPEDIT);
	}
	if (gdwPreviousVersion < 0x0114009A)
	{
		m_dwPatternSetup |= PATTERN_SHOWPREVIOUS|PATTERN_CONTSCROLL;
	}

	m_InputHandler = new CInputHandler(this);

	CHAR s[1024];
	wsprintf(s, "WARNING!\nYou're insane enough to have a very experimental build.\nThis build is for testing purposes only.\n\nPlease read the Readme.txt.\n\n\n If you're reading this you know where to send bug reports! :)\n -Rewbs");
	//::MessageBox(NULL, s, "Warning - experimental build", MB_OK|MB_ICONEXCLAMATION);
}


VOID CMainFrame::Initialize()
//---------------------------
{
	CHAR s[256];
	


	// Load Chords
	theApp.LoadChords(Chords);
	// Check for valid sound device
	if (!EnumerateSoundDevices(m_nWaveDevice>>8, m_nWaveDevice&0xff, s, sizeof(s)))
	{
		m_nWaveDevice = (SNDDEV_DSOUND<<8);
		if (!EnumerateSoundDevices(m_nWaveDevice>>8, m_nWaveDevice&0xff, s, sizeof(s)))
		{
			m_nWaveDevice = (SNDDEV_WAVEOUT<<8);
		}
	}
	// Default directory location
	strcpy(m_szCurModDir, m_szModDir);
	strcpy(m_szCurSmpDir, m_szSmpDir);
	strcpy(m_szCurInsDir, m_szInsDir);
//	strcpy(m_szCurKbdFile, m_szKbdFile);		//rewbs.custKeys
	if (m_szModDir[0]) SetCurrentDirectory(m_szModDir);
	// Create Audio Thread
	m_hAudioWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hNotifyWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hPlayThread = CreateThread(NULL, 0, AudioThread, NULL, 0, &m_dwPlayThreadId);
	m_hNotifyThread = CreateThread(NULL, 0, NotifyThread, NULL, 0, &m_dwNotifyThreadId);
	// Setup timer
	OnUpdateUser(NULL);
	m_nTimer = SetTimer(1, MPTTIMER_PERIOD, NULL);
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	// Setup Keyboard Hook
	ghKbdHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, AfxGetInstanceHandle(), GetCurrentThreadId());
	// Initialize Audio Mixer
	UpdateAudioParameters(TRUE);
	// Load View State
	MPT_LoadPatternState("Pattern Editor");
	// Load Hot Keys
	for (UINT iKey=0; iKey<MAX_MPTHOTKEYS; iKey++)
	{
		wsprintf(s, "Key_%02d", iKey);
		DWORD vkCode;
		if (0xFFFF == (vkCode = theApp.GetProfileInt("Hotkeys", s, 0xFFFF))) vkCode = gDefaultHotKeys[iKey].nMPTHotKey;
		CustomKeys[iKey] = vkCode;
	}
	// Keyboard Mapping
	for (UINT iNote=0; iNote<KEYBOARDMAP_LENGTH; iNote++)
	{
		wsprintf(s, "Note_%02d", iNote);
		DWORD dwKey = 0xFFFF;
		if (0xFFFF != (dwKey = theApp.GetProfileInt("Hotkeys", s, 0xFFFF))) KeyboardMap[iNote] = dwKey;
	}
	// Update the tree
	m_wndTree.Init();
}


CMainFrame::~CMainFrame()
//-----------------------
{
	DeleteCriticalSection(&m_csAudio);
	delete m_InputHandler;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
//-----------------------------------------------------
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1) return -1;
	// Load resources
	m_hIcon = theApp.LoadIcon(IDR_MAINFRAME);
	m_ImageList.Create(IDB_IMAGELIST, 16, 0, RGB(0,128,128));
	m_hGUIFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	m_hFixedFont = ::CreateFont(12,5, 0,0, 300,
							FALSE, FALSE, FALSE,
							OEM_CHARSET, OUT_RASTER_PRECIS,
							CLIP_DEFAULT_PRECIS, DRAFT_QUALITY,
							FIXED_PITCH | FF_MODERN, "");
	m_hLargeFixedFont = ::CreateFont(18,8, 0,0, 400,
							FALSE, FALSE, FALSE,
							OEM_CHARSET, OUT_RASTER_PRECIS,
							CLIP_DEFAULT_PRECIS, DRAFT_QUALITY,
							FIXED_PITCH | FF_MODERN, "");
	if (m_hGUIFont == NULL) m_hGUIFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	brushBlack = (HBRUSH)::GetStockObject(BLACK_BRUSH);
	brushWhite = (HBRUSH)::GetStockObject(WHITE_BRUSH);
	pbrushBlack = new CBrush(RGB(0,0,0));
	pbrushWhite = new CBrush(RGB(0xff,0xff,0xff));
	brushGray = ::CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	penLightGray = ::CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNHIGHLIGHT));
	penDarkGray = ::CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));
	penScratch = ::CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));
	penGray00 = ::CreatePen(PS_SOLID,0, RGB(0x00, 0x00, 0x00));
	penGray33 = ::CreatePen(PS_SOLID,0, RGB(0x33, 0x33, 0x33));
	penGray40 = ::CreatePen(PS_SOLID,0, RGB(0x40, 0x40, 0x40));
	penGray55 = ::CreatePen(PS_SOLID,0, RGB(0x55, 0x55, 0x55));
	penGray80 = ::CreatePen(PS_SOLID,0, RGB(0x80, 0x80, 0x80));
	penGray99 = ::CreatePen(PS_SOLID,0, RGB(0x99, 0x99, 0x99));
	penGraycc = ::CreatePen(PS_SOLID,0, RGB(0xcc, 0xcc, 0xcc));
	penGrayff = ::CreatePen(PS_SOLID,0, RGB(0xff, 0xff, 0xff));

	penHalfDarkGray = ::CreatePen(PS_DOT, 0, GetSysColor(COLOR_BTNSHADOW));
	penBlack = (HPEN)::GetStockObject(BLACK_PEN);
	penWhite = (HPEN)::GetStockObject(WHITE_PEN);
	
	
	
	// Cursors
	curDragging = theApp.LoadCursor(IDC_DRAGGING);
	curArrow = theApp.LoadStandardCursor(IDC_ARROW);
	curNoDrop = theApp.LoadCursor(IDC_NODROP);
	curNoDrop2 = theApp.LoadCursor(IDC_NODRAG);
	curVSplit = theApp.LoadCursor(AFX_IDC_HSPLITBAR);
	// bitmaps
	bmpPatterns = LoadDib(MAKEINTRESOURCE(IDB_PATTERNS));
	bmpNotes = LoadDib(MAKEINTRESOURCE(IDB_PATTERNVIEW));
	bmpVUMeters = LoadDib(MAKEINTRESOURCE(IDB_VUMETERS));
	bmpVisNode = LoadDib(MAKEINTRESOURCE(IDB_VISNODE));		//rewbs.fxVis
	UpdateColors();
	// Toolbars
	EnableDocking(CBRS_ALIGN_ANY);
	if (!m_wndToolBar.Create(this)) return -1;
	if (!m_wndStatusBar.Create(this)) return -1;
	if (!m_wndTree.Create(this, IDD_TREEVIEW, CBRS_LEFT|CBRS_BORDER_RIGHT, IDD_TREEVIEW)) return -1;
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));
	m_wndToolBar.Init(this);
	m_wndTree.RecalcLayout();
	// Restore toobar positions
	if (gdwPreviousVersion == MPTRACK_VERSION)
	{
//		try { try {
			LoadBarState("Toolbars");
//		} 
//		} catch(EXCEPTION_EXECUTE_HANDLER) {}
	}
	return 0;
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
//------------------------------------------------
{
	CRect rect;
	HKEY key;

	// Read registry settings
	if (RegOpenKeyEx(HKEY_CURRENT_USER,	MAINFRAME_REG_WINDOW, 0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		DWORD dwREG_DWORD = REG_DWORD;
		DWORD dwDWORDSize = sizeof(UINT);

		rect.SetRect(cs.x, cs.y, cs.cx, cs.cy);
		RegQueryValueEx(key, "MainWindow_x", NULL, &dwREG_DWORD, (LPBYTE)&rect.left, &dwDWORDSize);
		RegQueryValueEx(key, "MainWindow_y", NULL, &dwREG_DWORD, (LPBYTE)&rect.top, &dwDWORDSize);
		RegQueryValueEx(key, "MainWindow_width", NULL, &dwREG_DWORD, (LPBYTE)&rect.right, &dwDWORDSize);
		RegQueryValueEx(key, "MainWindow_height", NULL, &dwREG_DWORD, (LPBYTE)&rect.bottom, &dwDWORDSize);
		RegCloseKey(key);
		rect.right += rect.left;
		rect.bottom += rect.top;
		if ((rect.Width() <	GetSystemMetrics(SM_CXSCREEN)) && (rect.Width() > 16)
		 && (rect.Height() < GetSystemMetrics(SM_CYSCREEN)) && (rect.Height() > 16)
		 && (rect.right > 16) && (rect.left < GetSystemMetrics(SM_CXSCREEN) - 16)
		 && (rect.bottom > 16) && (rect.top < GetSystemMetrics(SM_CYSCREEN) - 16))
		{
			cs.x = rect.left;
			cs.y = rect.top;
			cs.cx = rect.Width();
			cs.cy = rect.Height();
		}
	}
	return CMDIFrameWnd::PreCreateWindow(cs);
}


BOOL CMainFrame::DestroyWindow()
//------------------------------
{
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	// Uninstall Keyboard Hook
	if (ghKbdHook)
	{
		UnhookWindowsHookEx(ghKbdHook);
		ghKbdHook = NULL;
	}
	// Kill Timer
	if (m_nTimer)
	{
		KillTimer(m_nTimer);
		m_nTimer = 0;
	}
	if (shMidiIn) midiCloseDevice();
	if (m_hPlayThread != NULL)
	{
		TerminateThread(m_hPlayThread, 0);
		m_hPlayThread = NULL;
	}
	if (m_hNotifyThread != NULL)
	{
		TerminateThread(m_hNotifyThread, 0);
		m_hNotifyThread = NULL;
	}
	// Delete bitmaps
	if (bmpPatterns)
	{
		delete bmpPatterns;
		bmpPatterns = NULL;
	}
	if (bmpNotes)
	{
		delete bmpNotes;
		bmpNotes = NULL;
	}
	if (bmpVUMeters)
	{
		delete bmpVUMeters;
		bmpVUMeters = NULL;
	}
	//rewbs.fxvis
	if (bmpVisNode)
	{
		delete bmpVisNode;
		bmpVisNode = NULL;
	}
	//end rewbs.fxvis
	if	(pbrushBlack)
		pbrushBlack->DeleteObject();
	if	(pbrushWhite)
		pbrushWhite->DeleteObject();		


	// Kill GDI Objects
	DeleteGDIObject(brushGray);
	DeleteGDIObject(penLightGray);
	DeleteGDIObject(penDarkGray);
	DeleteGDIObject(penSample);
	DeleteGDIObject(penEnvelope);
	DeleteGDIObject(m_hFixedFont);
	DeleteGDIObject(penScratch);
	DeleteGDIObject(penGray00);
	DeleteGDIObject(penGray33);
	DeleteGDIObject(penGray40);
	DeleteGDIObject(penGray55);
	DeleteGDIObject(penGray80);
	DeleteGDIObject(penGray99);
	DeleteGDIObject(penGraycc);
	DeleteGDIObject(penGrayff);
	DeleteGDIObject(pbrushBlack);
	DeleteGDIObject(pbrushWhite);

	for (UINT i=0; i<NUM_VUMETER_PENS*2; i++)
	{
		if (gpenVuMeter[i])
		{
			DeleteObject(gpenVuMeter[i]);
			gpenVuMeter[i] = NULL;
		}
	}

	return CMDIFrameWnd::DestroyWindow();
}


void CMainFrame::OnClose()
//------------------------
{
	CHAR s[64];
	CChildFrame *pMDIActive = (CChildFrame *)MDIGetActive();
	CRect rect;
	HKEY key;

	BeginWaitCursor();
	if (m_dwStatus & MODSTATUS_PLAYING) PauseMod();
	if (pMDIActive) pMDIActive->SavePosition(TRUE);
	if (gpSoundDevice)
	{
		BEGIN_CRITICAL();
		gpSoundDevice->Release();
		gpSoundDevice = NULL;
		END_CRITICAL();
	}
	GetWindowRect(&rect);
	// Save Window Settings
	if (RegCreateKey(HKEY_CURRENT_USER,	MAINFRAME_REG_WINDOW, &key) == ERROR_SUCCESS)
	{
		DWORD d = IsZoomed();
		if ((!IsIconic()) && (!d))
		{
			rect.right -= rect.left;
			rect.bottom -= rect.top;
			RegSetValueEx(key, "MainWindow_x", NULL, REG_DWORD, (LPBYTE)&rect.left, sizeof(UINT));
			RegSetValueEx(key, "MainWindow_y", NULL, REG_DWORD, (LPBYTE)&rect.top, sizeof(UINT));
			RegSetValueEx(key, "MainWindow_width", NULL, REG_DWORD, (LPBYTE)&rect.right, sizeof(UINT));
			RegSetValueEx(key, "MainWindow_height", NULL, REG_DWORD, (LPBYTE)&rect.bottom, sizeof(UINT));
		}
		RegSetValueEx(key, "Maximized", NULL, REG_DWORD, (LPBYTE)&d, sizeof(DWORD));
		RegSetValueEx(key, "MDIMaximize", NULL, REG_DWORD, (LPBYTE)&gbMdiMaximize, sizeof(BOOL));
		RegSetValueEx(key, "MDITreeWidth", NULL, REG_DWORD, (LPBYTE)&glTreeWindowWidth, sizeof(DWORD));

		//rewbs.varWindowSize
		RegSetValueEx(key, "MDICtrlHeight", NULL, REG_DWORD, (LPBYTE)&glCtrlWindowHeight, sizeof(DWORD));	//obsolete, for backwards compat only
		RegSetValueEx(key, "MDIGeneralHeight", NULL, REG_DWORD, (LPBYTE)&glGeneralWindowHeight, sizeof(DWORD));
		RegSetValueEx(key, "MDIPatternHeight", NULL, REG_DWORD, (LPBYTE)&glPatternWindowHeight, sizeof(DWORD));
		RegSetValueEx(key, "MDISampleHeight", NULL, REG_DWORD,  (LPBYTE)&glSampleWindowHeight, sizeof(DWORD));
		RegSetValueEx(key, "MDIInstrumentHeight", NULL, REG_DWORD,  (LPBYTE)&glInstrumentWindowHeight, sizeof(DWORD));
		RegSetValueEx(key, "MDICommentsHeight", NULL, REG_DWORD,  (LPBYTE)&glCommentsWindowHeight, sizeof(DWORD));
		RegSetValueEx(key, "MDITreeRatio", NULL, REG_DWORD, (LPBYTE)&glTreeSplitRatio, sizeof(DWORD));
		// Colors
		for (int ncol=0; ncol<MAX_MODCOLORS; ncol++)
		{
			wsprintf(s, "Color%02d", ncol);
			RegSetValueEx(key, s, NULL, REG_DWORD, (LPBYTE)&rgbCustomColors[ncol], sizeof(COLORREF));
		}
		RegCloseKey(key);
	}
	if (RegCreateKey(HKEY_CURRENT_USER,	MAINFRAME_REGKEY, &key) == ERROR_SUCCESS)
	{
		// Player
		RegSetValueEx(key, "WaveDevice", NULL, REG_DWORD, (LPBYTE)&m_nWaveDevice, sizeof(DWORD));
		RegSetValueEx(key, "SoundSetup", NULL, REG_DWORD, (LPBYTE)&m_dwSoundSetup, sizeof(DWORD));
		RegSetValueEx(key, "Quality", NULL, REG_DWORD, (LPBYTE)&m_dwQuality, sizeof(DWORD));
		RegSetValueEx(key, "SrcMode", NULL, REG_DWORD, (LPBYTE)&m_nSrcMode, sizeof(DWORD));
		RegSetValueEx(key, "Mixing_Rate", NULL, REG_DWORD, (LPBYTE)&m_dwRate, sizeof(DWORD));
		RegSetValueEx(key, "BitsPerSample", NULL, REG_DWORD, (LPBYTE)&m_nBitsPerSample, sizeof(DWORD));
		RegSetValueEx(key, "ChannelMode", NULL, REG_DWORD, (LPBYTE)&m_nChannels, sizeof(DWORD));
		RegSetValueEx(key, "BufferLength", NULL, REG_DWORD, (LPBYTE)&m_nBufferLength, sizeof(DWORD));
		RegSetValueEx(key, "Songs_Directory", NULL, REG_SZ, (LPBYTE)m_szModDir, strlen(m_szModDir)+1);
		RegSetValueEx(key, "Samples_Directory", NULL, REG_SZ, (LPBYTE)m_szSmpDir, strlen(m_szSmpDir)+1);
		RegSetValueEx(key, "Instruments_Directory", NULL, REG_SZ, (LPBYTE)m_szInsDir, strlen(m_szInsDir)+1);
		RegSetValueEx(key, "Plugins_Directory", NULL, REG_SZ, (LPBYTE)m_szPluginsDir, strlen(m_szPluginsDir)+1);
		RegSetValueEx(key, "Key_Config_File", NULL, REG_SZ, (LPBYTE)m_szKbdFile, strlen(m_szKbdFile)+1);
		RegSetValueEx(key, "PreAmp", NULL, REG_DWORD, (LPBYTE)&m_nPreAmp, sizeof(DWORD));
		RegSetValueEx(key, "StereoSeparation", NULL, REG_DWORD, (LPBYTE)&CSoundFile::m_nStereoSeparation, sizeof(DWORD));
		RegSetValueEx(key, "MidiSetup", NULL, REG_DWORD, (LPBYTE)&m_dwMidiSetup, sizeof(DWORD));
		RegSetValueEx(key, "MidiDevice", NULL, REG_DWORD, (LPBYTE)&m_nMidiDevice, sizeof(DWORD));
		RegSetValueEx(key, "PatternSetup", NULL, REG_DWORD, (LPBYTE)&m_dwPatternSetup, sizeof(DWORD));
		RegSetValueEx(key, "RowSpacing", NULL, REG_DWORD, (LPBYTE)&m_nRowSpacing, sizeof(DWORD));
		RegSetValueEx(key, "RowSpacing2", NULL, REG_DWORD, (LPBYTE)&m_nRowSpacing2, sizeof(DWORD));
		RegSetValueEx(key, "LoopSong", NULL, REG_DWORD, (LPBYTE)&gbLoopSong, sizeof(BOOL));
		RegSetValueEx(key, "MidiImportSpeed", NULL, REG_DWORD, (LPBYTE)&gnMidiImportSpeed, sizeof(DWORD));
		RegSetValueEx(key, "MidiImportPatLen", NULL, REG_DWORD, (LPBYTE)&gnMidiPatternLen, sizeof(DWORD));
		// Effects
		RegSetValueEx(key, "MixChannels", NULL, REG_DWORD, (LPBYTE)&CSoundFile::m_nMaxMixChannels, sizeof(UINT));
		RegSetValueEx(key, "XBassDepth", NULL, REG_DWORD, (LPBYTE)&CSoundFile::m_nXBassDepth, sizeof(UINT));
		RegSetValueEx(key, "XBassRange", NULL, REG_DWORD, (LPBYTE)&CSoundFile::m_nXBassRange, sizeof(UINT));
		RegSetValueEx(key, "ReverbDepth", NULL, REG_DWORD, (LPBYTE)&CSoundFile::m_nReverbDepth, sizeof(UINT));
		RegSetValueEx(key, "ReverbType", NULL, REG_DWORD, (LPBYTE)&CSoundFile::gnReverbType, sizeof(UINT));
		RegSetValueEx(key, "ProLogicDepth", NULL, REG_DWORD, (LPBYTE)&CSoundFile::m_nProLogicDepth, sizeof(UINT));
		RegSetValueEx(key, "ProLogicDelay", NULL, REG_DWORD, (LPBYTE)&CSoundFile::m_nProLogicDelay, sizeof(UINT));
		// EQ
		CEQSetupDlg::SaveEQ(key, "EQ_Settings", &m_EqSettings);
		CEQSetupDlg::SaveEQ(key, "EQ_User1", &CEQSetupDlg::gUserPresets[0]);
		CEQSetupDlg::SaveEQ(key, "EQ_User2", &CEQSetupDlg::gUserPresets[1]);
		CEQSetupDlg::SaveEQ(key, "EQ_User3", &CEQSetupDlg::gUserPresets[2]);
		CEQSetupDlg::SaveEQ(key, "EQ_User4", &CEQSetupDlg::gUserPresets[3]);
		// Keyboard Config
		//RegSetValueEx(key, "KeyboardCfg", NULL, REG_DWORD, (LPBYTE)&m_nKeyboardCfg, sizeof(DWORD));
		RegCloseKey(key);
	}
	if (RegCreateKey(HKEY_CURRENT_USER,	MAINFRAME_REG_SETTINGS, &key) == ERROR_SUCCESS)
	{
		DWORD dwVersion = MPTRACK_VERSION;
		// Author information
		RegSetValueEx(key, "Version", NULL, REG_DWORD, (LPBYTE)&dwVersion, sizeof(DWORD));
		RegCloseKey(key);
	}
	// Save Custom HotKeys
	for (UINT iKey=0; iKey<MAX_MPTHOTKEYS; iKey++)
	{
		wsprintf(s, "Key_%02d", iKey);
		theApp.WriteProfileInt("Hotkeys", s, CustomKeys[iKey]);
	}
	// Save Keyboard Mapping
	for (UINT iNote=0; iNote<KEYBOARDMAP_LENGTH; iNote++)
	{
		wsprintf(s, "Note_%02d", iNote);
		theApp.WriteProfileInt("Hotkeys", s, KeyboardMap[iNote]);
	}
	// Save chords
	theApp.SaveChords(Chords);
	MPT_SavePatternState("Pattern Editor");
	SaveBarState("Toolbars");
	EndWaitCursor();
	CMDIFrameWnd::OnClose();
}


LRESULT CALLBACK CMainFrame::KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
//-------------------------------------------------------------------------------
{
	if (code>=0)
	{
		if (m_InputHandler->GeneralKeyEvent(kCtxAllContexts, code, wParam, lParam) != kcNull)
		{
			
			if ((wParam != VK_ESCAPE) &&
				!(wParam == 'C' && m_InputHandler->CtrlPressed()) && 
				!(wParam == 'X' && m_InputHandler->CtrlPressed()) && 
				!(wParam == 'V' && m_InputHandler->CtrlPressed()))
				return -1;	// We've handled the keypress. No need to take it further.
							// Unless it was esc, in which case we need it to close Windows
							// (there might be other special cases.. will have to see..)
		}
	}
	//TODO: get rid of this. As we wittle out hardcoded keys we won't need this.
/*	static BOOL sbPostKeyPending = FALSE;
	if (code == HC_ACTION)
	{
		DWORD scancode = lParam >> 16;
		BOOL bUp = ((scancode & 0xC000) == 0xC000);
		BOOL bDn = ((scancode & 0xC000) == 0x0000);

		if ((bUp) || (bDn))
		{
			UINT hkmask = 0;
			switch(wParam)
			{
			case VK_LCONTROL:
			case VK_RCONTROL:
			case VK_CONTROL:
				hkmask = HOTKEYF_CONTROL;
				break;
			case VK_LMENU:
			case VK_RMENU:
			case VK_MENU:
				hkmask = HOTKEYF_ALT;
				break;
			case VK_LSHIFT:
			case VK_RSHIFT:
			case VK_SHIFT:
				hkmask = HOTKEYF_SHIFT;
				break;
			}
			if (hkmask)
			{
				if (bUp) gnHotKeyMask &= ~hkmask;
				if (bDn) gnHotKeyMask |= hkmask;
			}
		}
	}
*/
	return CallNextHookEx(ghKbdHook, code, wParam, lParam);
}	


BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
//---------------------------------------------
{
	if ((pMsg->message == WM_RBUTTONDOWN) || (pMsg->message == WM_NCRBUTTONDOWN))
	{
		CWnd* pWnd = CWnd::FromHandlePermanent(pMsg->hwnd);
		CControlBar* pBar = NULL;
		HWND hwnd = (pWnd) ? pWnd->m_hWnd : NULL;
		
		if ((hwnd) && (pMsg->message == WM_RBUTTONDOWN)) pBar = DYNAMIC_DOWNCAST(CControlBar, pWnd);
		if ((pBar != NULL) || ((pMsg->message == WM_NCRBUTTONDOWN) && (pMsg->wParam == HTMENU)))
		{
			CMenu Menu;
			CPoint pt;

			GetCursorPos(&pt);
			if (Menu.LoadMenu(IDR_TOOLBARS))
			{
				CMenu* pSubMenu = Menu.GetSubMenu(0);
				if (pSubMenu!=NULL) pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,this);
			}
		}
	}
	return CMDIFrameWnd::PreTranslateMessage(pMsg);
}


void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
//---------------------------------------------------
{
	if ((GetStyle() & FWS_ADDTOTITLE) == 0)	return;     // leave it alone!

	CMDIChildWnd* pActiveChild = NULL;
	CDocument* pDocument = GetActiveDocument();
	if (bAddToTitle &&
	  (pActiveChild = MDIGetActive()) != NULL &&
	  (pActiveChild->GetStyle() & WS_MAXIMIZE) == 0 &&
	  (pDocument != NULL ||
	   (pDocument = pActiveChild->GetActiveDocument()) != NULL))
	{
		TCHAR szText[256+_MAX_PATH];
		lstrcpy(szText, pDocument->GetTitle());
		if (pDocument->IsModified()) lstrcat(szText, "*");
		UpdateFrameTitleForDocument(szText);
	} else
	{
		LPCTSTR lpstrTitle = NULL;
		CString strTitle;

		if (pActiveChild != NULL)
		{
			strTitle = pActiveChild->GetTitle();
			if (!strTitle.IsEmpty())
				lpstrTitle = strTitle;
		}
		UpdateFrameTitleForDocument(lpstrTitle);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame Sound Library

static BOOL gbStopSent = FALSE;

// Sound Device Callback
BOOL SoundDeviceCallback(DWORD dwUser)
//------------------------------------
{
	BOOL bOk = FALSE;
	CMainFrame *pMainFrm = (CMainFrame *)theApp.m_pMainWnd;
	if (gbStopSent) return FALSE;
	BEGIN_CRITICAL();
	if ((pMainFrm) && (pMainFrm->IsPlaying()) && (CMainFrame::gpSoundDevice))
	{
		bOk = CMainFrame::gpSoundDevice->FillAudioBuffer(&gMPTSoundSource, gdwPlayLatency, dwUser);
	}
	if (!bOk)
	{
		gbStopSent = TRUE;
		pMainFrm->PostMessage(WM_COMMAND, ID_PLAYER_STOP);
	}
	END_CRITICAL();
	return bOk;
}

// Audio thread
DWORD WINAPI CMainFrame::AudioThread(LPVOID)
//------------------------------------------
{
	CMainFrame *pMainFrm;
	BOOL bWait;
	UINT nSleep;

	bWait = TRUE;
	nSleep = 50;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	for (;;)
	{
		if (bWait)
		{
			WaitForSingleObject(CMainFrame::m_hAudioWakeUp, 250);
		} else
		{
			Sleep(nSleep);
		}
		bWait = TRUE;
		pMainFrm = (CMainFrame *)theApp.m_pMainWnd;
		BEGIN_CRITICAL();
		if ((!gbStopSent) && (pMainFrm) && (pMainFrm->IsPlaying()) && (CMainFrame::gpSoundDevice))
		{
			if (!CMainFrame::gpSoundDevice->Directcallback())
			{
				if (CMainFrame::gpSoundDevice->FillAudioBuffer(&gMPTSoundSource, gdwPlayLatency))
				{
					ULONG nMaxSleep = CMainFrame::gpSoundDevice->GetMaxFillInterval();
					bWait = FALSE;
					nSleep = CMainFrame::m_nBufferLength / 8;
					if (nSleep > nMaxSleep) nSleep = nMaxSleep;
					if (nSleep < 10) nSleep = 10;
					if (nSleep > 40) nSleep = 40;
				} else
				{
					gbStopSent = TRUE;
					pMainFrm->PostMessage(WM_COMMAND, ID_PLAYER_STOP);
				}
			} else
			{
				nSleep = 50;
			}
		}
		END_CRITICAL();
	}
	ExitThread(0);
	return 0;
}


// Audio thread
DWORD WINAPI CMainFrame::NotifyThread(LPVOID)
//-------------------------------------------
{
	CMainFrame *pMainFrm;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	for (;;)
	{
		WaitForSingleObject(CMainFrame::m_hNotifyWakeUp, 1000);
		pMainFrm = (CMainFrame *)theApp.m_pMainWnd;
		if ((pMainFrm) && (pMainFrm->IsPlaying()))
		{
			MPTNOTIFICATION *pnotify = NULL;
			DWORD dwLatency = 0;

			for (UINT i=0; i<MAX_UPDATE_HISTORY; i++)
			{
				MPTNOTIFICATION *p = &pMainFrm->NotifyBuffer[i];
				if ((p->dwType & MPTNOTIFY_PENDING)
				 && (!(pMainFrm->m_dwStatus & MODSTATUS_BUSY)))
				{
					if (p->dwLatency >= dwLatency)
					{
						if (pnotify) pnotify->dwType = 0;
						pnotify = p;
					} else
					{
						p->dwType = 0;
					}
				}
			}
			if (pnotify)
			{
				pMainFrm->m_dwStatus |= MODSTATUS_BUSY;
				pMainFrm->PostMessage(WM_MOD_UPDATEPOSITION, 0, (LPARAM)pnotify);
			}
		}
	}
	ExitThread(0);
	return 0;
}


ULONG CMPTSoundSource::AudioRead(PVOID pData, ULONG cbSize)
//---------------------------------------------------------
{
	CMainFrame *pMainFrm = (CMainFrame *)theApp.m_pMainWnd;
	if (pMainFrm) return pMainFrm->AudioRead(pData, cbSize);
	return 0;
}


VOID CMPTSoundSource::AudioDone(ULONG nBytesWritten, ULONG nLatency)
//------------------------------------------------------------------
{
	CMainFrame *pMainFrm = (CMainFrame *)theApp.m_pMainWnd;
	if (pMainFrm) pMainFrm->AudioDone(nBytesWritten, nLatency);
}



ULONG CMainFrame::AudioRead(PVOID pvData, ULONG ulSize)
//-----------------------------------------------------
{
	if ((IsPlaying()) && (m_pSndFile))
	{
		DWORD dwSamplesRead = m_pSndFile->Read(pvData, ulSize);
		return dwSamplesRead * slSampleSize;
	}
	return 0;
}


VOID CMainFrame::AudioDone(ULONG nBytesWritten, ULONG nLatency)
//-------------------------------------------------------------
{
	if (nBytesWritten > (DWORD)slSampleSize)
	{
		DoNotification(nBytesWritten/CMainFrame::slSampleSize, nLatency);
	}
}


LONG CMainFrame::audioTryOpeningDevice(UINT channels, UINT bits, UINT samplespersec)
//----------------------------------------------------------------------------------
{
	WAVEFORMATEXTENSIBLE WaveFormat;
	UINT buflen = m_nBufferLength;
	
	if (!m_pSndFile) return -1;
	slSampleSize = (bits/8) * channels;
	sdwAudioBufferSize = ((samplespersec * buflen) / 1000) * slSampleSize;
	sdwAudioBufferSize = (sdwAudioBufferSize + 0x0F) & ~0x0F;
	if (sdwAudioBufferSize < MIN_AUDIO_BUFFERSIZE) sdwAudioBufferSize = MIN_AUDIO_BUFFERSIZE;
	if (sdwAudioBufferSize > MAX_AUDIO_BUFFERSIZE) sdwAudioBufferSize = MAX_AUDIO_BUFFERSIZE;
	WaveFormat.Format.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.Format.nChannels = (unsigned short) channels;
	WaveFormat.Format.nSamplesPerSec = samplespersec;
	WaveFormat.Format.nAvgBytesPerSec = samplespersec * slSampleSize;
	WaveFormat.Format.nBlockAlign = (unsigned short) slSampleSize;
	WaveFormat.Format.wBitsPerSample = (unsigned short)bits;
	WaveFormat.Format.cbSize = 0;
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
	if (m_dwSoundSetup & SOUNDSETUP_STREVERSE) CSoundFile::gdwSoundSetup |= SNDMIX_REVERSESTEREO;
	else CSoundFile::gdwSoundSetup &= ~SNDMIX_REVERSESTEREO;
	m_pSndFile->SetWaveConfig(samplespersec, bits, channels, (m_dwSoundSetup & SOUNDSETUP_ENABLEMMX) ? TRUE : FALSE);
	// Maybe we failed because someone is playing sound already.
	// Shut any sound off, and try once more before giving up.
	UINT nDevType = m_nWaveDevice>>8;
	UINT nDevNo = m_nWaveDevice&0xff;
	UINT fulOptions = (m_dwSoundSetup & SOUNDSETUP_SECONDARY) ? SNDDEV_OPTIONS_SECONDARY : 0;
	if ((gpSoundDevice) && (gpSoundDevice->GetDeviceType() != nDevType))
	{
		gpSoundDevice->Release();
		gpSoundDevice = NULL;
	}
	if (!gpSoundDevice)
	{
		if (!CreateSoundDevice(nDevType, &gpSoundDevice)) return -1;
	}
	gpSoundDevice->Configure(m_hWnd, NUM_AUDIO_BUFFERS, m_nBufferLength, fulOptions);
	gbStopSent = FALSE;
	m_pSndFile->SetResamplingMode(m_nSrcMode);
	m_pSndFile->UPDATEDSPEFFECTS();
	m_pSndFile->SetAGC(m_dwQuality & QUALITY_AGC);
	if (!gpSoundDevice->Open(nDevNo, &WaveFormat.Format)) return -1;
	return 0;
}


BOOL CMainFrame::audioOpenDevice()
//--------------------------------
{
	UINT nFixedBitsPerSample;
	LONG err;

	if ((!m_pSndFile) || (!m_pSndFile->GetType())) return FALSE;
	if (m_dwStatus & MODSTATUS_PLAYING) return TRUE;
	if (!m_dwRate) m_dwRate = 22050;
	if ((m_nChannels != 1) && (m_nChannels != 2) && (m_nChannels != 4)) m_nChannels = 2;
	err = audioTryOpeningDevice(m_nChannels,
								m_nBitsPerSample,
								m_dwRate);
	nFixedBitsPerSample = (gpSoundDevice) ? gpSoundDevice->HasFixedBitsPerSample() : 0;
	if ((err) && ((m_dwRate > 44100) || (m_nChannels > 2) || (m_nBitsPerSample > 16)
			   || ((nFixedBitsPerSample) && (nFixedBitsPerSample != m_nBitsPerSample))))
	{
		DWORD oldrate = m_dwRate;

		m_dwRate = 44100;
		if (m_nChannels > 2) m_nChannels = 2;
		if (nFixedBitsPerSample) m_nBitsPerSample = nFixedBitsPerSample;
		else if (m_nBitsPerSample > 16) m_nBitsPerSample = 16;
		err = audioTryOpeningDevice(m_nChannels,
									m_nBitsPerSample,
									m_dwRate);
		if (err) m_dwRate = oldrate;
	}
	// Display error message box
	if (err != 0)
	{
		MessageBox("Unable to open sound device!", NULL, MB_OK|MB_ICONERROR);
		return FALSE;
	}
	// Device is ready
	gdwLastMixActiveTime = timeGetTime();
	return TRUE;
}


void CMainFrame::audioCloseDevice()
//---------------------------------
{
	BEGIN_CRITICAL();
	if (gpSoundDevice)
	{
		gpSoundDevice->Reset();
		gpSoundDevice->Close();
    }
	END_CRITICAL();
}


void CMainFrame::CalcStereoVuMeters(int *pMix, unsigned long nSamples, unsigned long nChannels)
//---------------------------------------------------------------------------------------------
{
	const int * const p = pMix;
	int lmax = gnLVuMeter, rmax = gnRVuMeter;
	if (nChannels > 1)
	{
		for (UINT i=0; i<nSamples; i+=nChannels)
		{
			int vl = p[i];
			int vr = p[i+1];
			if (vl < 0) vl = -vl;
			if (vr < 0) vr = -vr;
			if (vl > lmax) lmax = vl;
			if (vr > rmax) rmax = vr;
		}
	} else
	{
		for (UINT i=0; i<nSamples; i++)
		{
			int vl = p[i];
			if (vl < 0) vl = -vl;
			if (vl > lmax) lmax = vl;
		}
		rmax = lmax;
	}
	gnLVuMeter = lmax;
	gnRVuMeter = rmax;
}


BOOL CMainFrame::DoNotification(DWORD dwSamplesRead, DWORD dwLatency)
//-------------------------------------------------------------------
{
	m_dwElapsedTime += (dwSamplesRead * 1000) / m_dwRate;
	gsdwTotalSamples += dwSamplesRead;
	if (!m_pSndFile) return FALSE;
	if (m_nMixChn < m_pSndFile->m_nMixStat) m_nMixChn++;
	if (m_nMixChn > m_pSndFile->m_nMixStat) m_nMixChn--;
	if (!(m_dwNotifyType & MPTNOTIFY_TYPEMASK)) return FALSE;
	// Notify Client
	for (UINT i=0; i<MAX_UPDATE_HISTORY; i++)
	{
		MPTNOTIFICATION *p = &NotifyBuffer[i];
		if ((p->dwType & MPTNOTIFY_TYPEMASK)
		 && (!(p->dwType & MPTNOTIFY_PENDING))
		 && (gsdwTotalSamples >= p->dwLatency))
		{
			p->dwType |= MPTNOTIFY_PENDING;
			SetEvent(m_hNotifyWakeUp);
		}
	}
	if (!m_pSndFile) return FALSE;
	// Add an entry to the notification history
	for (UINT j=0; j<MAX_UPDATE_HISTORY; j++)
	{
		MPTNOTIFICATION *p = &NotifyBuffer[j];
		if (!(p->dwType & MPTNOTIFY_TYPEMASK))
		{
			p->dwType = m_dwNotifyType;
			DWORD d = dwLatency / slSampleSize;
			p->dwLatency = gsdwTotalSamples + d;
			p->nOrder = m_pSndFile->m_nCurrentPattern;
			p->nRow = m_pSndFile->m_nRow;
			if (m_dwNotifyType & MPTNOTIFY_SAMPLE)
			{
				UINT nSmp = m_dwNotifyType & 0xFFFF;
				for (UINT k=0; k<MAX_CHANNELS; k++)
				{
					MODCHANNEL *pChn = &m_pSndFile->Chn[k];
					p->dwPos[k] = 0;
					if ((nSmp) && (nSmp <= m_pSndFile->m_nSamples) && (pChn->nLength)
					 && (pChn->pSample) && (pChn->pSample == m_pSndFile->Ins[nSmp].pSample)
					 && ((!(pChn->dwFlags & CHN_NOTEFADE)) || (pChn->nFadeOutVol)))
					{
						p->dwPos[k] = MPTNOTIFY_POSVALID | (DWORD)(pChn->nPos);
					}
				}
			} else
			if (m_dwNotifyType & (MPTNOTIFY_VOLENV|MPTNOTIFY_PANENV|MPTNOTIFY_PITCHENV))
			{
				UINT nIns = m_dwNotifyType & 0xFFFF;
				for (UINT k=0; k<MAX_CHANNELS; k++)
				{
					MODCHANNEL *pChn = &m_pSndFile->Chn[k];
					p->dwPos[k] = 0;
					if ((nIns) && (nIns <= m_pSndFile->m_nInstruments) && (pChn->nLength)
					 && (pChn->pHeader) && (pChn->pHeader == m_pSndFile->Headers[nIns])
					 && ((!(pChn->dwFlags & CHN_NOTEFADE)) || (pChn->nFadeOutVol)))
					{
						if (m_dwNotifyType & MPTNOTIFY_PITCHENV)
						{
							if (pChn->dwFlags & CHN_PITCHENV) p->dwPos[k] = MPTNOTIFY_POSVALID | (DWORD)(pChn->nPitchEnvPosition);
						} else
						if (m_dwNotifyType & MPTNOTIFY_PANENV)
						{
							if (pChn->dwFlags & CHN_PANENV) p->dwPos[k] = MPTNOTIFY_POSVALID | (DWORD)(pChn->nPanEnvPosition);
						} else
						{
							if (pChn->dwFlags & CHN_VOLENV) p->dwPos[k] = MPTNOTIFY_POSVALID | (DWORD)(pChn->nVolEnvPosition);
						}
					}
				}
			} else
			if (m_dwNotifyType & (MPTNOTIFY_VUMETERS))
			{
				for (UINT k=0; k<MAX_CHANNELS; k++)
				{
					MODCHANNEL *pChn = &m_pSndFile->Chn[k];
					UINT vul = pChn->nLeftVU;
					UINT vur = pChn->nRightVU;
					p->dwPos[k] = (vul << 8) | (vur);
				}
			} else
			if (m_dwNotifyType & MPTNOTIFY_MASTERVU)
			{
				DWORD lVu = (gnLVuMeter >> 11);
				DWORD rVu = (gnRVuMeter >> 11);
				if (lVu > 0x10000) lVu = 0x10000;
				if (rVu > 0x10000) rVu = 0x10000;
				p->dwPos[0] = lVu;
				p->dwPos[1] = rVu;
				DWORD dwVuDecay = _muldiv(dwSamplesRead, 120000, m_dwRate) + 1;
				if (lVu >= dwVuDecay) gnLVuMeter = (lVu - dwVuDecay) << 11; else gnLVuMeter = 0;
				if (rVu >= dwVuDecay) gnRVuMeter = (rVu - dwVuDecay) << 11; else gnRVuMeter = 0;
			}
			return TRUE;
		}
	}
	return FALSE;
}


void CMainFrame::UpdateAudioParameters(BOOL bReset)
//-------------------------------------------------
{
	if ((m_nBitsPerSample != 8) && (m_nBitsPerSample != 32)) m_nBitsPerSample = 16;
	CSoundFile::SetWaveConfig(m_dwRate,
			m_nBitsPerSample,
			m_nChannels,
			(m_dwSoundSetup & SOUNDSETUP_ENABLEMMX) ? TRUE : FALSE);
	if (m_dwSoundSetup & SOUNDSETUP_STREVERSE)
		CSoundFile::gdwSoundSetup |= SNDMIX_REVERSESTEREO;
	else
		CSoundFile::gdwSoundSetup &= ~SNDMIX_REVERSESTEREO;
	
	// Soft panning
	if (m_dwSoundSetup & SOUNDSETUP_SOFTPANNING)
		CSoundFile::gdwSoundSetup |= SNDMIX_SOFTPANNING;
	else
		CSoundFile::gdwSoundSetup &= ~SNDMIX_SOFTPANNING;
	if (m_dwPatternSetup & PATTERN_MUTECHNMODE)
		CSoundFile::gdwSoundSetup |= SNDMIX_MUTECHNMODE;
	else
		CSoundFile::gdwSoundSetup &= ~SNDMIX_MUTECHNMODE;
	CSoundFile::SetResamplingMode(m_nSrcMode);
	CSoundFile::UPDATEDSPEFFECTS();
	CSoundFile::SetAGC(m_dwQuality & QUALITY_AGC);
	CSoundFile::SetEQGains(	m_EqSettings.Gains, MAX_EQ_BANDS, m_EqSettings.Freqs, bReset );
	if (bReset) CSoundFile::InitPlayer(TRUE);
}


void CMainFrame::EnableLowLatencyMode(BOOL bOn)
//---------------------------------------------
{
	gdwPlayLatency = (bOn) ? sdwAudioBufferSize : 0;
	gdwLastLowLatencyTime = timeGetTime();
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame static helpers

void CMainFrame::UpdateColors()
//-----------------------------
{
	if (bmpPatterns)
	{
		bmpPatterns->bmiColors[7] = rgb2quad(GetSysColor(COLOR_BTNFACE));
	}
	if (bmpVUMeters)
	{
		bmpVUMeters->bmiColors[7] = rgb2quad(GetSysColor(COLOR_BTNFACE));
		bmpVUMeters->bmiColors[8] = rgb2quad(GetSysColor(COLOR_BTNSHADOW));
		bmpVUMeters->bmiColors[15] = rgb2quad(GetSysColor(COLOR_BTNHIGHLIGHT));
		bmpVUMeters->bmiColors[10] = rgb2quad(rgbCustomColors[MODCOLOR_VUMETER_LO]);
		bmpVUMeters->bmiColors[11] = rgb2quad(rgbCustomColors[MODCOLOR_VUMETER_MED]);
		bmpVUMeters->bmiColors[9] = rgb2quad(rgbCustomColors[MODCOLOR_VUMETER_HI]);
		bmpVUMeters->bmiColors[2] = rgb2quad((rgbCustomColors[MODCOLOR_VUMETER_LO] >> 1) & 0x7F7F7F);
		bmpVUMeters->bmiColors[3] = rgb2quad((rgbCustomColors[MODCOLOR_VUMETER_MED] >> 1) & 0x7F7F7F);
		bmpVUMeters->bmiColors[1] = rgb2quad((rgbCustomColors[MODCOLOR_VUMETER_HI] >> 1) & 0x7F7F7F);
	}
	if (penSample) DeleteObject(penSample);
	penSample = ::CreatePen(PS_SOLID, 0, rgbCustomColors[MODCOLOR_SAMPLE]);
	if (penEnvelope) DeleteObject(penEnvelope);
	penEnvelope = ::CreatePen(PS_SOLID, 0, rgbCustomColors[MODCOLOR_ENVELOPES]);
	for (UINT i=0; i<NUM_VUMETER_PENS*2; i++)
	{
		int r0,g0,b0, r1,g1,b1;
		int r, g, b;
		int y;

		if (gpenVuMeter[i])
		{
			DeleteObject(gpenVuMeter[i]);
			gpenVuMeter[i] = NULL;
		}
		y = (i >= NUM_VUMETER_PENS) ? (i-NUM_VUMETER_PENS) : i;
		if (y < (NUM_VUMETER_PENS/2))
		{
			r0 = GetRValue(rgbCustomColors[MODCOLOR_VUMETER_LO]);
			g0 = GetGValue(rgbCustomColors[MODCOLOR_VUMETER_LO]);
			b0 = GetBValue(rgbCustomColors[MODCOLOR_VUMETER_LO]);
			r1 = GetRValue(rgbCustomColors[MODCOLOR_VUMETER_MED]);
			g1 = GetGValue(rgbCustomColors[MODCOLOR_VUMETER_MED]);
			b1 = GetBValue(rgbCustomColors[MODCOLOR_VUMETER_MED]);
		} else
		{
			y -= (NUM_VUMETER_PENS/2);
			r0 = GetRValue(rgbCustomColors[MODCOLOR_VUMETER_MED]);
			g0 = GetGValue(rgbCustomColors[MODCOLOR_VUMETER_MED]);
			b0 = GetBValue(rgbCustomColors[MODCOLOR_VUMETER_MED]);
			r1 = GetRValue(rgbCustomColors[MODCOLOR_VUMETER_HI]);
			g1 = GetGValue(rgbCustomColors[MODCOLOR_VUMETER_HI]);
			b1 = GetBValue(rgbCustomColors[MODCOLOR_VUMETER_HI]);
		}
		r = r0 + ((r1 - r0) * y) / (NUM_VUMETER_PENS/2);
		g = g0 + ((g1 - g0) * y) / (NUM_VUMETER_PENS/2);
		b = b0 + ((b1 - b0) * y) / (NUM_VUMETER_PENS/2);
		if (i >= NUM_VUMETER_PENS)
		{
			r = (r*2)/5;
			g = (g*2)/5;
			b = (b*2)/5;
		}
		gpenVuMeter[i] = CreatePen(PS_SOLID, 0, RGB(r, g, b));
	}
	// Sequence window
	{
		COLORREF crBkgnd = GetSysColor(COLOR_WINDOW);
		if (brushHighLight) DeleteObject(brushHighLight);
		brushHighLight = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
		if (brushWindow) DeleteObject(brushWindow);
		brushWindow = CreateSolidBrush(crBkgnd);
		if (penSeparator) DeleteObject(penSeparator);
		penSeparator = CreatePen(PS_SOLID, 0, RGB(GetRValue(crBkgnd)/2, GetGValue(crBkgnd)/2, GetBValue(crBkgnd)/2));
	}
}


VOID CMainFrame::GetKeyName(LONG lParam, LPSTR pszName, UINT cbSize)
//------------------------------------------------------------------
{
	pszName[0] = (char)cbSize;
	if ((cbSize > 0) && (lParam))
	{
		GetKeyNameText(lParam, pszName, cbSize);
	}
}


UINT CMainFrame::GetNoteFromKey(UINT nChar, DWORD dwFlags)
//--------------------------------------------------------
{
	const DWORD *lpKeyboardMap;
	UINT note = 0;

	if ((m_nKeyboardCfg & KEYBOARD_FT2KEYS) && (nChar == VK_CAPITAL)) return 0xFF;
	lpKeyboardMap = GetKeyboardMap();
	dwFlags &= 0x1FF; // Keep scan code only
	if (!dwFlags) return 0;
	for (UINT ich=0; ich<KEYBOARDMAP_LENGTH; ich++)
	{
		if (lpKeyboardMap[ich] == dwFlags)
		{
			if (ich == 3*12) return 0xFE;
			if (ich == 3*12+1) return 0xFF;
			note = ich+1;
			break;
		}
	}
	if (note > 0)
	{
		CMainFrame *pMainFrm = GetMainFrame();
		if (!pMainFrm) return 0;
		int octave = pMainFrm->GetBaseOctave();
		int n = octave * 12 + note;
		if (n < 1) n = 1;
		if (n > 120) n = 0;
		note = n;
	}
	return note;
}


const DWORD *CMainFrame::GetKeyboardMap()
//---------------------------------------
{
	switch(m_nKeyboardCfg & KEYBOARD_MASK)
	{
	case KEYBOARD_FT2:	return KeyboardFT2;
	case KEYBOARD_IT:	return KeyboardIT;
	case KEYBOARD_MPT:	return KeyboardMPT;
	default:			return KeyboardMap;
	}
}


UINT CMainFrame::IsHotKey(DWORD dwKey)
//------------------------------------
{
/*	for (UINT i=0; i<MAX_MPTHOTKEYS; i++)
	{
		UINT n = CustomKeys[i];
		switch(m_nKeyboardCfg & KEYBOARD_MASK)
		{
		case KEYBOARD_FT2:	n = gDefaultHotKeys[i].nFT2HotKey; break;
		case KEYBOARD_IT:	n = gDefaultHotKeys[i].nITHotKey; break;
		case KEYBOARD_MPT:	n = gDefaultHotKeys[i].nMPTHotKey; break;
		}
		if ((n) && (n == dwKey)) return gDefaultHotKeys[i].nID;
	}
*/
	return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame operations

UINT CMainFrame::GetBaseOctave()
//------------------------------
{
	return m_wndToolBar.GetBaseOctave();
}


UINT CMainFrame::GetCurrentInstrument()
//-------------------------------------
{
	return 0;
}


void CMainFrame::SetPreAmp(UINT n)
//--------------------------------
{
	m_nPreAmp = n;
	if (m_pSndFile) m_pSndFile->SetMasterVolume(m_nPreAmp, TRUE);
}


BOOL CMainFrame::ResetNotificationBuffer(HWND hwnd)
//-------------------------------------------------
{
	if ((!hwnd) || (m_hFollowSong == hwnd))
	{
		memset(NotifyBuffer, 0, sizeof(NotifyBuffer));
		gsdwTotalSamples = 0;
		return TRUE;
	}
	return FALSE;
}


BOOL CMainFrame::PlayMod(CModDoc *pModDoc, HWND hPat, DWORD dwNotifyType)
//-----------------------------------------------------------------------
{
	BOOL bPaused, bPatLoop, bResetAGC;
	if (!pModDoc) return FALSE;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if ((!pSndFile) || (!pSndFile->GetType())) return FALSE;
	bPaused = (pSndFile->m_dwSongFlags & SONG_PAUSED) ? TRUE : FALSE;
	bPatLoop = (pSndFile->m_dwSongFlags & SONG_PATTERNLOOP) ? TRUE : FALSE;
	pSndFile->ResetChannels();
	bResetAGC = FALSE;
	if (((m_pSndFile) && (pSndFile != m_pSndFile)) || (!m_dwElapsedTime)) bResetAGC = TRUE;
	if ((m_pSndFile) || (m_dwStatus & MODSTATUS_PLAYING)) PauseMod();
	if (bResetAGC) CSoundFile::ResetAGC();
	m_pSndFile = pSndFile;
	m_pModPlaying = pModDoc;
	m_hFollowSong = hPat;
	m_dwNotifyType = dwNotifyType;
	if (m_dwNotifyType & MPTNOTIFY_MASTERVU)
	{
		gnLVuMeter = gnRVuMeter = 0;
		CSoundFile::gpSndMixHook = CalcStereoVuMeters;
	} else
	{
		CSoundFile::gpSndMixHook = NULL;
	}
	if (!audioOpenDevice())
	{
		m_pSndFile = NULL;
		m_pModPlaying = NULL;
		m_hFollowSong = NULL;
		return FALSE;
	}
	m_nMixChn = m_nAvgMixChn = 0;
	gsdwTotalSamples = 0;
	if (!bPatLoop)
	{
		if (bPaused)
		{
			pSndFile->m_dwSongFlags |= SONG_PAUSED;
		} else
		{
			pModDoc->SetPause(FALSE);
			if (pSndFile->GetCurrentPos() + 2 >= pSndFile->GetMaxPosition()) pSndFile->SetCurrentPos(0);
			pSndFile->SetRepeatCount((gbLoopSong) ? -1 : 0);
		}
	}
	m_pSndFile->SetMasterVolume(m_nPreAmp, TRUE);
	m_pSndFile->InitPlayer(TRUE);
	memset(NotifyBuffer, 0, sizeof(NotifyBuffer));
	m_dwStatus |= MODSTATUS_PLAYING;
	m_wndToolBar.SetCurrentSong(m_pSndFile);
	if (gpSoundDevice) gpSoundDevice->Start();
	SetEvent(m_hAudioWakeUp);
	return TRUE;
}


BOOL CMainFrame::PauseMod(CModDoc *pModDoc)
//-----------------------------------------
{
	if ((pModDoc) && (pModDoc != m_pModPlaying)) return FALSE;
	if (m_dwStatus & MODSTATUS_PLAYING)
	{
		m_dwStatus &= ~MODSTATUS_PLAYING;
		if (gpSoundDevice) gpSoundDevice->Reset();
		audioCloseDevice();
		m_nMixChn = m_nAvgMixChn = 0;
		Sleep(1);
		if (m_hFollowSong)
		{
			MPTNOTIFICATION mn;
			memset(&mn, 0, sizeof(mn));
			mn.dwType = MPTNOTIFY_STOP;
			::SendMessage(m_hFollowSong, WM_MOD_UPDATEPOSITION, 0, (LPARAM)&mn);
		}
	}
	if (m_pModPlaying)
	{
		m_pModPlaying->SetPause(TRUE);
		m_wndTree.UpdatePlayPos(m_pModPlaying, NULL);
	}
	if (m_pSndFile)
	{
		m_pSndFile->SuspendPlugins();
		m_pSndFile->LoopPattern(-1);
		m_pSndFile->m_dwSongFlags &= ~SONG_PAUSED;
		if (m_pSndFile == &m_WaveFile)
		{
			m_pSndFile = NULL;
			m_WaveFile.Destroy();
		} else
		{
			for (UINT i=m_pSndFile->m_nChannels; i<MAX_CHANNELS; i++)
			{
				if (!(m_pSndFile->Chn[i].nMasterChn))
				{
					m_pSndFile->Chn[i].nPos = m_pSndFile->Chn[i].nPosLo = m_pSndFile->Chn[i].nLength = 0;
				}
			}
		}

	}
	m_pModPlaying = NULL;
	m_pSndFile = NULL;
	m_hFollowSong = NULL;
	m_wndToolBar.SetCurrentSong(NULL);
	return TRUE;
}


BOOL CMainFrame::StopMod(CModDoc *pModDoc)
//----------------------------------------
{
	if ((pModDoc) && (pModDoc != m_pModPlaying)) return FALSE;
	CModDoc *pPlay = m_pModPlaying;
	CSoundFile *pSndFile = m_pSndFile;
	PauseMod();
	if (pPlay) pPlay->SetPause(FALSE);
	if (pSndFile) pSndFile->SetCurrentPos(0);
	m_dwElapsedTime = 0;
	return TRUE;
}


BOOL CMainFrame::PlaySoundFile(CSoundFile *pSndFile)
//--------------------------------------------------
{
	if (m_pSndFile) PauseMod(NULL);
	if ((!pSndFile) || (!pSndFile->GetType())) return FALSE;
	m_pSndFile = pSndFile;
	if (!audioOpenDevice())
	{
		m_pSndFile = NULL;
		return FALSE;
	}
	gsdwTotalSamples = 0;
	m_pSndFile->SetMasterVolume(m_nPreAmp, TRUE);
	m_pSndFile->InitPlayer(TRUE);
	m_dwStatus |= MODSTATUS_PLAYING;
	if (gpSoundDevice) gpSoundDevice->Start();
	SetEvent(m_hAudioWakeUp);
	return TRUE;
}


BOOL CMainFrame::PlayDLSInstrument(UINT nDLSBank, UINT nIns, UINT nRgn)
//---------------------------------------------------------------------
{
	if ((nDLSBank >= MAX_DLS_BANKS) || (!CTrackApp::gpDLSBanks[nDLSBank])) return FALSE;
	BeginWaitCursor();
	PlaySoundFile((LPCSTR)NULL);
	m_WaveFile.m_nInstruments = 1;
	if (CTrackApp::gpDLSBanks[nDLSBank]->ExtractInstrument(&m_WaveFile, 1, nIns, nRgn))
	{
		PlaySoundFile(&m_WaveFile);
		m_WaveFile.SetRepeatCount(0);
	}
	EndWaitCursor();
	return TRUE;
}


BOOL CMainFrame::PlaySoundFile(LPCSTR lpszFileName, UINT nNote)
//-------------------------------------------------------------
{
	CMappedFile f;
	BOOL bOk = FALSE;

	if (lpszFileName)
	{
		BeginWaitCursor();
		if (!f.Open(lpszFileName))
		{
			EndWaitCursor();
			return FALSE;
		}
	}
	StopSoundFile(&m_WaveFile);
	m_WaveFile.Destroy();
	m_WaveFile.Create(NULL, 0);
	m_WaveFile.m_nDefaultTempo = 125;
	m_WaveFile.m_nDefaultSpeed = 4;
	m_WaveFile.m_nRepeatCount = 0;
	m_WaveFile.m_nType = MOD_TYPE_IT;
	m_WaveFile.m_nChannels = 4;
	m_WaveFile.m_nInstruments = 1;
	m_WaveFile.m_nSamples = 1;
	m_WaveFile.Order[0] = 0;
	m_WaveFile.Order[1] = 1;
	m_WaveFile.Order[2] = 0xFF;
	m_WaveFile.PatternSize[0] = 64;
	m_WaveFile.PatternSize[1] = 64;
	m_WaveFile.Patterns[0] = CSoundFile::AllocatePattern(64, 4);
	m_WaveFile.Patterns[1] = CSoundFile::AllocatePattern(64, 4);
	if (m_WaveFile.Patterns[0])
	{
		if (!nNote) nNote = 5*12+1;
		MODCOMMAND *m = m_WaveFile.Patterns[0];
		m[0].note = (BYTE)nNote;
		m[0].instr = 1;
		m[1].note = (BYTE)nNote;
		m[1].instr = 1;
	}
	if (lpszFileName)
	{
		DWORD dwLen = f.GetLength();
		if (dwLen)
		{
			LPBYTE p = f.Lock();
			if (p)
			{
				bOk = m_WaveFile.ReadInstrumentFromFile(1, p, dwLen);
				f.Unlock();
			}
			if (bOk)
			{
				if ((m_WaveFile.m_nSamples > 1) || (m_WaveFile.Ins[1].uFlags & CHN_LOOP))
				{
					MODCOMMAND *m = m_WaveFile.Patterns[0];
					m[32*4].note = 0xFF;
					m[32*4+1].note = 0xFF;
					m[63*4].note = 0xFE;
					m[63*4+1].note = 0xFE;
				} else
				{
					MODCOMMAND *m = m_WaveFile.Patterns[1];
					if (m)
					{
						m[63*4].command = CMD_POSITIONJUMP;
						m[63*4].param = 1;
					}
				}
				bOk = PlaySoundFile(&m_WaveFile);
			}
		}
		f.Close();
		EndWaitCursor();
	}
	return bOk;
}


BOOL CMainFrame::PlaySoundFile(CSoundFile *pSong, UINT nInstrument, UINT nSample, UINT nNote)
//-------------------------------------------------------------------------------------------
{
	StopSoundFile(&m_WaveFile);
	m_WaveFile.Destroy();
	m_WaveFile.Create(NULL, 0);
	m_WaveFile.m_nDefaultTempo = 125;
	m_WaveFile.m_nDefaultSpeed = 6;
	m_WaveFile.m_nRepeatCount = 0;
	m_WaveFile.m_nType = pSong->m_nType;
	m_WaveFile.m_nChannels = 4;
	if ((nInstrument) && (nInstrument <= pSong->m_nInstruments))
	{
		m_WaveFile.m_nInstruments = 1;
		m_WaveFile.m_nSamples = 32;
	} else
	{
		m_WaveFile.m_nInstruments = 0;
		m_WaveFile.m_nSamples = 1;
	}
	m_WaveFile.Order[0] = 0;
	m_WaveFile.Order[1] = 1;
	m_WaveFile.Order[2] = 0xFF;
	m_WaveFile.PatternSize[0] = 64;
	m_WaveFile.PatternSize[1] = 64;
	m_WaveFile.Patterns[0] = CSoundFile::AllocatePattern(64, 4);
	m_WaveFile.Patterns[1] = CSoundFile::AllocatePattern(64, 4);
	if (m_WaveFile.Patterns[0])
	{
		if (!nNote) nNote = 5*12+1;
		MODCOMMAND *m = m_WaveFile.Patterns[0];
		m[0].note = (BYTE)nNote;
		m[0].instr = 1;
		m[1].note = (BYTE)nNote;
		m[1].instr = 1;
		m = m_WaveFile.Patterns[1];
		m[32*4].note = 0xFF;
		m[32*4+1].note = 0xFF;
		m[63*4].note = 0xFE;
		m[63*4+1].note = 0xFE;
	}
	if ((nInstrument) && (nInstrument <= pSong->m_nInstruments))
	{
		m_WaveFile.ReadInstrumentFromSong(1, pSong, nInstrument);
	} else
	{
		m_WaveFile.ReadSampleFromSong(1, pSong, nSample);
	}
	return PlaySoundFile(&m_WaveFile);
}


BOOL CMainFrame::StopSoundFile(CSoundFile *pSndFile)
//--------------------------------------------------
{
	if ((pSndFile) && (pSndFile != m_pSndFile)) return FALSE;
	PauseMod(NULL);
	return TRUE;
}


BOOL CMainFrame::SetFollowSong(CModDoc *pDoc, HWND hwnd, BOOL bFollowSong, DWORD dwType)
//--------------------------------------------------------------------------------------
{
	if ((!pDoc) || (pDoc != m_pModPlaying)) return FALSE;
	if (bFollowSong)
	{
		m_hFollowSong = hwnd;
	} else
	{
		if (hwnd == m_hFollowSong) m_hFollowSong = NULL;
	}
	if (dwType) m_dwNotifyType = dwType;
	if (m_dwNotifyType & MPTNOTIFY_MASTERVU)
	{
		CSoundFile::gpSndMixHook = CalcStereoVuMeters;
	} else
	{
		gnLVuMeter = gnRVuMeter = 0;
		CSoundFile::gpSndMixHook = NULL;
	}
	return TRUE;
}


BOOL CMainFrame::SetupSoundCard(DWORD q, DWORD rate, UINT nBits, UINT nChns, UINT bufsize, LONG wd)
//-------------------------------------------------------------------------------------------------
{
	BOOL bPlaying = (m_dwStatus & MODSTATUS_PLAYING) ? TRUE : FALSE;
	if ((m_dwRate != rate) || ((m_dwSoundSetup & SOUNDSETUP_RESTARTMASK) != (q & SOUNDSETUP_RESTARTMASK))
	 || (m_nWaveDevice != wd) || (m_nBufferLength != bufsize) || (nBits != m_nBitsPerSample)
	 || (m_nChannels != nChns))
	{
		CModDoc *pActiveMod = NULL;
		HWND hFollow = m_hFollowSong;
		if (bPlaying)
		{
			if ((m_pSndFile) && (!m_pSndFile->IsPaused())) pActiveMod = m_pModPlaying;
			PauseMod();
		}
		m_nWaveDevice = wd;
		m_dwRate = rate;
		m_dwSoundSetup = q;
		m_nBufferLength = bufsize;
		m_nBitsPerSample = nBits;
		m_nChannels = nChns;
		BEGIN_CRITICAL();
		UpdateAudioParameters(FALSE);
		END_CRITICAL();
		if (pActiveMod) PlayMod(pActiveMod, hFollow, m_dwNotifyType);
		UpdateWindow();
	} else
	{
		// No need to restart playback
		m_dwSoundSetup = q;
		CSoundFile::EnableMMX((m_dwSoundSetup & SOUNDSETUP_ENABLEMMX) ? TRUE : FALSE);
		if (m_dwSoundSetup & SOUNDSETUP_STREVERSE)
			CSoundFile::gdwSoundSetup |= SNDMIX_REVERSESTEREO;
		else
			CSoundFile::gdwSoundSetup &= ~SNDMIX_REVERSESTEREO;
	}
	return TRUE;
}


BOOL CMainFrame::SetupPlayer(DWORD q, DWORD srcmode, BOOL bForceUpdate)
//---------------------------------------------------------------------
{
	if ((q != m_dwQuality) || (srcmode != m_nSrcMode) || (bForceUpdate))
	{
		m_nSrcMode = srcmode;
		m_dwQuality = q;
		BEGIN_CRITICAL();
		CSoundFile::SetResamplingMode(m_nSrcMode);
		CSoundFile::UPDATEDSPEFFECTS();
		CSoundFile::SetAGC(m_dwQuality & QUALITY_AGC);
		END_CRITICAL();
		PostMessage(WM_MOD_INVALIDATEPATTERNS, HINT_MPTSETUP);
	}
	return TRUE;
}


BOOL CMainFrame::SetupDirectories(LPCSTR songs, LPCSTR samples, LPCSTR instr)
//---------------------------------------------------------------------------
{
	strcpy(m_szModDir, songs);
	strcpy(m_szSmpDir, samples);
	strcpy(m_szInsDir, instr);
	if (songs[0]) strcpy(m_szCurModDir, songs);
	if (samples[0]) strcpy(m_szCurSmpDir, samples);
	if (instr[0]) strcpy(m_szCurInsDir, instr);
	// This shouldn't be here (misc options)
	m_wndToolBar.EnableFlatButtons(m_dwPatternSetup & PATTERN_FLATBUTTONS);
	UpdateAllViews(HINT_MPTOPTIONS, NULL);
	if (m_dwPatternSetup & PATTERN_MUTECHNMODE)
		CSoundFile::gdwSoundSetup |= SNDMIX_MUTECHNMODE;
	else
		CSoundFile::gdwSoundSetup &= ~SNDMIX_MUTECHNMODE;
	return TRUE;
}


BOOL CMainFrame::SetupMidi(DWORD d, LONG n)
//-----------------------------------------
{
	m_dwMidiSetup = d;
	m_nMidiDevice = n;
	return TRUE;
}


void CMainFrame::UpdateAllViews(DWORD dwHint, CObject *pHint)
//-----------------------------------------------------------
{
	CDocTemplate *pDocTmpl = theApp.GetModDocTemplate();
	if (pDocTmpl)
	{
		POSITION pos = pDocTmpl->GetFirstDocPosition();
		CDocument *pDoc;
		while ((pos != NULL) && ((pDoc = pDocTmpl->GetNextDoc(pos)) != NULL))
		{
			pDoc->UpdateAllViews(NULL, dwHint, pHint);
		}
	}
}


VOID CMainFrame::SetUserText(LPCSTR lpszText)
//-------------------------------------------
{
	if (lpszText[0] | m_szUserText[0])
	{
		strcpy(m_szUserText, lpszText);
		OnUpdateUser(NULL); 
	}
}


VOID CMainFrame::SetInfoText(LPCSTR lpszText)
//-------------------------------------------
{
	if (lpszText[0] | m_szInfoText[0])
	{
		strcpy(m_szInfoText, lpszText);
		OnUpdateInfo(NULL);
	}
}


VOID CMainFrame::SetHelpText(LPCSTR lpszText)
//-------------------------------------------
{
	m_wndStatusBar.SetPaneText(0, lpszText);
}


VOID CMainFrame::OnDocumentCreated(CModDoc *pModDoc)
//--------------------------------------------------
{
	m_wndTree.OnDocumentCreated(pModDoc);
}


VOID CMainFrame::OnDocumentClosed(CModDoc *pModDoc)
//-------------------------------------------------
{
	if (pModDoc == m_pModPlaying) PauseMod();
	m_wndTree.OnDocumentClosed(pModDoc);
}


VOID CMainFrame::UpdateTree(CModDoc *pModDoc, DWORD lHint, CObject *pHint)
//------------------------------------------------------------------------
{
	m_wndTree.OnUpdate(pModDoc, lHint, pHint);
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


void CMainFrame::OnViewOptions()
//------------------------------
{
	if (m_bOptionsLocked)
		return;
		
	CPropertySheet dlg("Modplug Tracker Setup", this, m_nLastOptionsPage);
	COptionsGeneral general;
	COptionsSoundcard sounddlg(m_dwRate, m_dwSoundSetup, m_nBitsPerSample, m_nChannels, m_nBufferLength, m_nWaveDevice);
	COptionsKeyboard keyboard;
	COptionsColors colors;
	COptionsPlayer playerdlg;
	CMidiSetupDlg mididlg(m_dwMidiSetup, m_nMidiDevice);
	CEQSetupDlg eqdlg(&m_EqSettings);
	dlg.AddPage(&general);
	dlg.AddPage(&sounddlg);
	dlg.AddPage(&playerdlg);
	dlg.AddPage(&eqdlg);
	dlg.AddPage(&keyboard);
	dlg.AddPage(&colors);
	dlg.AddPage(&mididlg);
	m_bOptionsLocked=true;
	dlg.DoModal();
	m_bOptionsLocked=false;
	m_wndTree.OnOptionsChanged();
}


void CMainFrame::OnAddDlsBank()
//-----------------------------
{
	CFileDialog dlg(TRUE,
					".dls",
					NULL,
					OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
					"All Sound Banks|*.dls;*.sbk;*.sf2;*.mss|"
					"Downloadable Sounds Banks (*.dls)|*.dls;*.mss|"
					"SoundFont 2.0 Banks (*.sf2)|*.sbk;*.sf2|"
					"All Files (*.*)|*.*||",
					this);
	if (dlg.DoModal() == IDOK)
	{
		BeginWaitCursor();
		CTrackApp::AddDLSBank(dlg.GetPathName());
		m_wndTree.RefreshDlsBanks();
		EndWaitCursor();
	}
}


void CMainFrame::OnImportMidiLib()
//--------------------------------
{
	CFileDialog dlg(TRUE,
					NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
					"Text and INI files (*.txt,*.ini)|*.txt;*.ini;*.dls;*.sf2;*.sbk|"
					"Downloadable Sound Banks (*.dls)|*.dls;*.mss|"
					"SoundFont 2.0 banks (*.sf2)|*.sbk;*.sf2|"
					"Gravis UltraSound (ultrasnd.ini)|ultrasnd.ini|"
					"All Files (*.*)|*.*||",
					this);
	if (dlg.DoModal() == IDOK)
	{
		BeginWaitCursor();
		CTrackApp::ImportMidiConfig(dlg.GetPathName());
		m_wndTree.RefreshMidiLibrary();
		EndWaitCursor();
	}
}


void CMainFrame::SetLastMixActiveTime()		//rewbs.LiveVSTi
//-------------------------------------
{
	Log("setting to %d\n", gdwLastMixActiveTime);
	gdwLastMixActiveTime = timeGetTime();
}

void CMainFrame::OnTimer(UINT)
//----------------------------
{
	// Display Time in status bar
	DWORD dwTime = m_dwElapsedTime / 1000;
	if (dwTime != m_dwTimeSec)
	{
		m_dwTimeSec = dwTime;
		m_nAvgMixChn = m_nMixChn;
		OnUpdateTime(NULL);
	}
	// Idle Time Check
	if (IsPlaying())
	{
		DWORD dwTime = timeGetTime();
		gdwIdleTime = 0;
		if (dwTime - gdwLastLowLatencyTime > 15000)
		{
			gdwPlayLatency = 0;
		}
		if ((m_pSndFile) && (m_pSndFile->IsPaused()) && (!m_pSndFile->m_nMixChannels))
		{
			Log("%d (%d)\n", dwTime - gdwLastMixActiveTime, gdwLastMixActiveTime);
			if (dwTime - gdwLastMixActiveTime > 5000)
			{
				PauseMod();
			}
		} else
		{
			gdwLastMixActiveTime = dwTime;
		}
	} else
	{
		gdwIdleTime += MPTTIMER_PERIOD;
		if (gdwIdleTime > 15000)
		{
			gdwIdleTime = 0;
			// After 15 seconds of inactivity, we reset the AGC
			CSoundFile::ResetAGC();
			gdwPlayLatency = 0;
		}
	}
	m_wndToolBar.SetCurrentSong(m_pSndFile);
	// Call plugins idle routine for open editor
	CVstPluginManager *pPluginManager = theApp.GetPluginManager();
	if (pPluginManager)
	{
		//call @ 10Hz
		DWORD curTime = timeGetTime();
		if (curTime - m_dwLastPluginIdleCall > 100)
		{
			pPluginManager->OnIdle();
			m_dwLastPluginIdleCall = curTime;
		}
	}
}


CModDoc *CMainFrame::GetActiveDoc()
//---------------------------------
{
	CMDIChildWnd *pMDIActive = MDIGetActive();
	if (pMDIActive)
	{
		CView *pView = pMDIActive->GetActiveView();
		if (pView) return (CModDoc *)pView->GetDocument();
	}
	return NULL;
}

CView *CMainFrame::GetActiveView()
//---------------------------------
{
	CMDIChildWnd *pMDIActive = MDIGetActive();
	if (pMDIActive)
	{
		return pMDIActive->GetActiveView();
	}

	return NULL;
}


void CMainFrame::SwitchToActiveView()
//-----------------------------------
{
	CMDIChildWnd *pMDIActive = MDIGetActive();
	if (pMDIActive)
	{
		CView *pView = pMDIActive->GetActiveView();
		if (pView)
		{
			pMDIActive->SetActiveView(pView);
			pView->SetFocus();
		}
	}
}


void CMainFrame::OnUpdateTime(CCmdUI *)
//-------------------------------------
{
	CHAR s[64];
	wsprintf(s, "%d:%02d:%02d",
		m_dwTimeSec / 3600, (m_dwTimeSec / 60) % 60, (m_dwTimeSec % 60));
	if ((m_pSndFile) && (!(m_pSndFile->IsPaused())))
	{
		if (m_pSndFile != &m_WaveFile)
		{
			UINT nPat = m_pSndFile->m_nPattern;
			if (nPat < MAX_PATTERNS)
			{
				if (nPat < 10) strcat(s, " ");
				if (nPat < 100) strcat(s, " ");
				wsprintf(s+strlen(s), " [%d]", nPat);
			}
		}
		wsprintf(s+strlen(s), " %dch", m_nAvgMixChn);
	}
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_TIME), s, TRUE);
}


void CMainFrame::OnUpdateUser(CCmdUI *)
//-------------------------------------
{
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_USER), m_szUserText, TRUE);
}


void CMainFrame::OnUpdateInfo(CCmdUI *)
//-------------------------------------
{
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_INFO), m_szInfoText, TRUE);
}


void CMainFrame::OnPlayerPause()
//------------------------------
{
	if (m_pModPlaying)
	{
		m_pModPlaying->OnPlayerPause();
	} else
	{
		PauseMod();
	}
}


LRESULT CMainFrame::OnInvalidatePatterns(WPARAM wParam, LPARAM)
//-------------------------------------------------------------
{
	UpdateAllViews(wParam, NULL);
	return TRUE;
}


LRESULT CMainFrame::OnUpdatePosition(WPARAM, LPARAM lParam)
//---------------------------------------------------------
{
	MPTNOTIFICATION *pnotify = (MPTNOTIFICATION *)lParam;
	if (pnotify)
	{
		//Log("OnUpdatePosition: row=%d time=%lu\n", pnotify->nRow, pnotify->dwLatency);
		if ((m_pModPlaying) && (m_pSndFile))
		{
			m_wndTree.UpdatePlayPos(m_pModPlaying, pnotify);
			if (m_hFollowSong) ::SendMessage(m_hFollowSong, WM_MOD_UPDATEPOSITION, 0, lParam);
		}
		pnotify->dwType = 0;
	}
	m_dwStatus &= ~MODSTATUS_BUSY;
	return 0;
}


void CMainFrame::OnPrevOctave()
//-----------------------------
{
	UINT n = GetBaseOctave();
	if (n > MIN_BASEOCTAVE) m_wndToolBar.SetBaseOctave(n-1);
	SwitchToActiveView();
}


void CMainFrame::OnNextOctave()
//-----------------------------
{
	UINT n = GetBaseOctave();
	if (n < MAX_BASEOCTAVE) m_wndToolBar.SetBaseOctave(n+1);
	SwitchToActiveView();
}


void CMainFrame::OnOctaveChanged()
//--------------------------------
{
	SwitchToActiveView();
}


void CMainFrame::OnReportBug()
{
	CTrackApp::OpenURL("http://www.modplug.com/forum/viewforum.php?f=22");
	return;
}

BOOL CMainFrame::OnInternetLink(UINT nID)
//---------------------------------------
{
	LPCSTR pszURL = NULL;

	switch(nID)
	{
	case ID_NETLINK_MODPLUG:	pszURL = "http://www.modplug.com"; break;
	case ID_NETLINK_UT:			pszURL = "http://www.united-trackers.org"; break;
	case ID_NETLINK_OSMUSIC:	pszURL = "http://www.osmusic.net/"; break;
	case ID_NETLINK_HANDBOOK:	pszURL = "http://www.modplug.com/mods/handbook/handbook.htm"; break;
	case ID_NETLINK_FORUMS:		pszURL = "http://www.modplug.com/forums"; break;
	case ID_NETLINK_PLUGINS:	pszURL = "http://www.kvr-vst.com"; break;
	}
	if (pszURL) return CTrackApp::OpenURL(pszURL);
	return FALSE;
}


void CMainFrame::OnRButtonDown(UINT, CPoint pt)
//---------------------------------------------
{
	CMenu Menu;

	ClientToScreen(&pt);
	if (Menu.LoadMenu(IDR_TOOLBARS))
	{
		CMenu* pSubMenu = Menu.GetSubMenu(0);
		if (pSubMenu!=NULL) pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,this);
	}
}


LRESULT CMainFrame::OnSpecialKey(WPARAM vKey, LPARAM)
//---------------------------------------------------
{
/*	CMDIChildWnd *pMDIActive = MDIGetActive();
	CView *pView = NULL;
	if (pMDIActive) pView = pMDIActive->GetActiveView();
	switch(vKey)
	{
	case VK_RMENU:
		if (pView) pView->PostMessage(WM_COMMAND, ID_PATTERN_RESTART, 0);
		break;
	case VK_RCONTROL:
		if (pView) pView->PostMessage(WM_COMMAND, ID_PLAYER_PLAY, 0);
		break;
	}
*/
	return 0;
}

LRESULT CMainFrame::OnCustomKeyMsg(WPARAM wParam, LPARAM lParam)
{
	if (wParam == kcNull)
		return NULL;

	switch(wParam)
	{
		case kcViewTree: OnBarCheck(IDD_TREEVIEW); break;
		case kcViewOptions: OnViewOptions(); break;
		case kcViewMain: OnBarCheck(59392); break;
	 	case kcFileImportMidiLib: OnImportMidiLib(); break;
		case kcFileAddSoundBank: OnAddDlsBank(); break;
		case kcPauseSong: OnPlayerPause(); break;
		case kcPrevOctave: OnPrevOctave(); break;
		case kcNextOctave: OnNextOctave(); break;
		case kcFileNew:	theApp.OnFileNew(); break;
		case kcFileOpen: theApp.OnFileOpen(); break;
		case kcMidiRecord:	OnMidiRecord(); break;
		case kcHelp: 		CMDIFrameWnd::OnHelp(); break;
		//D'oh!! moddoc isn't a CWnd so we have to handle its messages and pass them on.

		case kcFileSaveAs:
		case kcFileSaveAsWave:
		case kcFileSaveMidi:
		case kcFileClose:
		case kcFileSave:
		case kcViewGeneral:
		case kcViewPattern:
		case kcViewSamples:
		case kcViewInstruments:
		case kcViewComments: 
		case kcPlayPatternFromCursor:
		case kcPlayPatternFromStart: 
		case kcPlaySongFromCursor: 
		case kcPlaySongFromStart: 
		case kcPlayPauseSong:
		case kcStopSong:
		case kcEstimateSongLength:
			{	CModDoc* pModDoc = GetActiveDoc();
				if (pModDoc)
					return GetActiveDoc()->OnCustomKeyMsg(wParam, lParam);
				break;
			}

		//if handled neither by MainFrame nor by ModDoc...
		default:
			//If the treeview has focus, post command to the tree view
			if (m_bModTreeHasFocus)
				return m_wndTree.PostMessageToModTree(WM_MOD_KEYCOMMAND, wParam, lParam);
			if (m_pNoteMapHasFocus)
				return m_pNoteMapHasFocus->PostMessage(WM_MOD_KEYCOMMAND, wParam, lParam);

			//Else send it to the active view
			CView* pView = GetActiveView();
			if (pView)
				return pView->PostMessage(WM_MOD_KEYCOMMAND, wParam, lParam);
	}

	return wParam;
}
void CMainFrame::OnInitMenu(CMenu* pMenu)
{
	m_InputHandler->SetModifierMask(0);
	if (m_InputHandler->noAltMenu())
		return;

	CMDIFrameWnd::OnInitMenu(pMenu);
	
}

long CMainFrame::GetSampleRate()
{
	if (GetModPlaying())
		return GetModPlaying()->GetSoundFile()->GetSampleRate();
	return 0;
}

long CMainFrame::GetTotalSampleCount()
{
	if (GetModPlaying())
		return GetModPlaying()->GetSoundFile()->m_lTotalSampleCount;
	return 0;
}

double CMainFrame::GetApproxBPM()
{
	if (!GetModPlaying())
		return 0;
    
	CSoundFile *pSndFile = GetModPlaying()->GetSoundFile();

	// Assumes Highlight1 rows per beat.
	double ticksPerBeat = pSndFile->m_nMusicSpeed *m_nRowSpacing2;
	double msPerBeat = (5000.0*pSndFile->m_nTempoFactor)/(pSndFile->m_nMusicTempo*256.0) * ticksPerBeat;
	double bpm = 60000/msPerBeat;
	return bpm;
}
// We have swicthed focus to a new module - might need to update effect keys to reflect module type
bool CMainFrame::UpdateEffectKeys(void)
{
	CModDoc* pModDoc = GetActiveDoc();
	if (pModDoc)
	{
		CSoundFile* pSndFile = pModDoc->GetSoundFile();
		if (pSndFile)
		{
			if	(pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)) 
				return m_InputHandler->SetXMEffects();
			else
				return m_InputHandler->SetITEffects();
		}
	}
	
	return false;
}


