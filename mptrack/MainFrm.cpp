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
#include "AutoSaver.h"
#include "performancecounter.h"
#include ".\mainfrm.h"
// -> CODE#0015
// -> DESC="channels management dlg"
#include "globals.h"
#include "ChannelManagerDlg.h"
#include "MIDIMappingDialog.h"
// -! NEW_FEATURE#0015
#include <direct.h>
#include "version.h"
#include "ctrl_pat.h"
#include "UpdateCheck.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAINFRAME_REGKEY_BASE		"Software\\Olivier Lapicque\\"
#define MAINFRAME_REGKEY_DEFAULT	"ModPlug Tracker"
#define MAINFRAME_REGEXT_WINDOW		"\\Window"
#define MAINFRAME_REGEXT_SETTINGS	"\\Settings"

#define MPTTIMER_PERIOD		200

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

// -> CODE#0002
// -> DESC="list box to choose VST plugin presets (programs)"
	ON_COMMAND(ID_PLUGIN_SETUP,				OnPluginManager)
// -! NEW_FEATURE#0002

// -> CODE#0015
// -> DESC="channels management dlg"
	ON_COMMAND(ID_CHANNEL_MANAGER,			OnChannelManager)
// -! NEW_FEATURE#0015
	ON_COMMAND(ID_VIEW_MIDIMAPPING,			OnViewMIDIMapping)
	//ON_COMMAND(ID_HELP,					CMDIFrameWnd::OnHelp)
	ON_COMMAND(ID_VIEW_SONGPROPERTIES,		OnSongProperties)
	ON_COMMAND(ID_HELP_FINDER,				CMDIFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_REPORT_BUG,				OnReportBug)	//rewbs.reportBug
	ON_COMMAND(ID_CONTEXT_HELP,				CMDIFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP,				CMDIFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_NEXTOCTAVE,				OnNextOctave)
	ON_COMMAND(ID_PREVOCTAVE,				OnPrevOctave)
	ON_COMMAND(ID_ADD_SOUNDBANK,			OnAddDlsBank)
	ON_COMMAND(ID_IMPORT_MIDILIB,			OnImportMidiLib)
	ON_COMMAND(ID_MIDI_RECORD,				OnMidiRecord)
	ON_COMMAND(ID_PANIC,					OnPanic)
	ON_COMMAND(ID_PLAYER_PAUSE,				OnPlayerPause)
	ON_COMMAND_EX(IDD_TREEVIEW,				OnBarCheck)
	ON_COMMAND_EX(ID_NETLINK_MODPLUG,		OnInternetLink)
	ON_COMMAND_EX(ID_NETLINK_TOP_PICKS,		OnInternetLink)
	ON_CBN_SELCHANGE(IDC_COMBO_BASEOCTAVE,	OnOctaveChanged)
	ON_UPDATE_COMMAND_UI(ID_MIDI_RECORD,	OnUpdateMidiRecord)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TIME,	OnUpdateTime)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_USER,	OnUpdateUser)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INFO,	OnUpdateInfo)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_XINFO,OnUpdateXInfo) //rewbs.xinfo
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CPU,  OnUpdateCPU)
	ON_UPDATE_COMMAND_UI(IDD_TREEVIEW,		OnUpdateControlBarMenu)
	ON_MESSAGE(WM_MOD_UPDATEPOSITION,		OnUpdatePosition)
	ON_MESSAGE(WM_MOD_INVALIDATEPATTERNS,	OnInvalidatePatterns)
	ON_MESSAGE(WM_MOD_SPECIALKEY,			OnSpecialKey)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,	OnCustomKeyMsg) //rewbs.customKeys
	ON_COMMAND(ID_INTERNETUPDATE,			OnInternetUpdate)
	//}}AFX_MSG_MAP
	ON_WM_INITMENU()
	ON_WM_KILLFOCUS() //rewbs.fix3116
	ON_WM_MOUSEWHEEL()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

// Static
static int gdwLastLowLatencyTime = 0;
static int gdwLastMixActiveTime = 0;
static DWORD gsdwTotalSamples = 0;
static DWORD gdwPlayLatency = 0;

// Globals
UINT CMainFrame::gnPatternSpacing = 0;
BOOL CMainFrame::gbPatternRecord = TRUE;
BOOL CMainFrame::gbPatternVUMeters = FALSE;
BOOL CMainFrame::gbPatternPluginNames = TRUE;
DWORD CMainFrame::gdwNotificationType = MPTNOTIFY_DEFAULT;
UINT CMainFrame::m_nLastOptionsPage = 0;
BOOL CMainFrame::gbMdiMaximize = FALSE;
bool CMainFrame::gbShowHackControls = false;
//rewbs.varWindowSize
LONG CMainFrame::glGeneralWindowHeight = 178;
LONG CMainFrame::glPatternWindowHeight = 152;
LONG CMainFrame::glSampleWindowHeight = 188;
LONG CMainFrame::glInstrumentWindowHeight = 300;
LONG CMainFrame::glCommentsWindowHeight = 288;
LONG CMainFrame::glGraphWindowHeight = 288; //rewbs.graph
//end rewbs.varWindowSize
LONG CMainFrame::glTreeWindowWidth = 160;
LONG CMainFrame::glTreeSplitRatio = 128;
HHOOK CMainFrame::ghKbdHook = NULL;
CString CMainFrame::gcsPreviousVersion = "";
CString CMainFrame::gcsInstallGUID = "";

DWORD CMainFrame::gnHotKeyMask = 0;
// Audio Setup
//rewbs.resamplerConf
long CMainFrame::glVolumeRampSamples = 42;
double CMainFrame::gdWFIRCutoff = 0.97;
BYTE  CMainFrame::gbWFIRType = 7; //WFIR_KAISER4T;
//end rewbs.resamplerConf
UINT CMainFrame::gnAutoChordWaitTime = 60;

int CMainFrame::gnPlugWindowX = 243;
int CMainFrame::gnPlugWindowY = 273;
int CMainFrame::gnPlugWindowWidth = 370;
int CMainFrame::gnPlugWindowHeight = 332;
DWORD CMainFrame::gnPlugWindowLast = 0;

uint32 CMainFrame::gnMsgBoxVisiblityFlags = uint32_max;

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
#ifndef NO_DSOUND
LONG CMainFrame::m_nWaveDevice = SNDDEV_BUILD_ID(0, SNDDEV_DSOUND);
#else
LONG CMainFrame::m_nWaveDevice = SNDDEV_BUILD_ID(0, SNDDEV_WAVEOUT);
#endif // NO_DSOUND
LONG CMainFrame::m_nMidiDevice = 0;
DWORD CMainFrame::m_nBufferLength = 75;
LONG CMainFrame::gnLVuMeter = 0;
LONG CMainFrame::gnRVuMeter = 0;
EQPRESET CMainFrame::m_EqSettings = { "", {16,16,16,16,16,16}, { 125, 300, 600, 1250, 4000, 8000 } };
// Midi Setup
DWORD CMainFrame::m_dwMidiSetup = MIDISETUP_RECORDVELOCITY|MIDISETUP_RECORDNOTEOFF;
// Pattern Setup
DWORD CMainFrame::m_dwPatternSetup = PATTERN_PLAYNEWNOTE | PATTERN_EFFECTHILIGHT
								   | PATTERN_SMALLFONT | PATTERN_CENTERROW
								   | PATTERN_DRAGNDROPEDIT | PATTERN_FLATBUTTONS | PATTERN_NOEXTRALOUD
								   | PATTERN_2NDHIGHLIGHT | PATTERN_STDHIGHLIGHT /*| PATTERN_HILITETIMESIGS*/
								   | PATTERN_SHOWPREVIOUS | PATTERN_CONTSCROLL | PATTERN_SYNCMUTE | PATTERN_AUTODELAY | PATTERN_NOTEFADE;
DWORD CMainFrame::m_nRowSpacing = 16;	// primary highlight (measures)
DWORD CMainFrame::m_nRowSpacing2 = 4;	// secondary highlight (beats)
UINT CMainFrame::m_nSampleUndoMaxBuffer = 0;	// Real sample buffer undo size will be set later.

// GDI
HICON CMainFrame::m_hIcon = NULL;
HFONT CMainFrame::m_hGUIFont = NULL;
HFONT CMainFrame::m_hFixedFont = NULL;
HFONT CMainFrame::m_hLargeFixedFont = NULL;
HPEN CMainFrame::penDarkGray = NULL;
HPEN CMainFrame::penScratch = NULL; //rewbs.fxVis
HPEN CMainFrame::penGray00 = NULL;
HPEN CMainFrame::penGray33 = NULL;
HPEN CMainFrame::penGray40 = NULL;
HPEN CMainFrame::penGray55 = NULL;
HPEN CMainFrame::penGray80 = NULL;
HPEN CMainFrame::penGray99 = NULL;
HPEN CMainFrame::penGraycc = NULL;
HPEN CMainFrame::penGrayff = NULL; //end rewbs.fxVis
HPEN CMainFrame::penLightGray = NULL;
HPEN CMainFrame::penBlack = NULL;
HPEN CMainFrame::penWhite = NULL;
HPEN CMainFrame::penHalfDarkGray = NULL;
HPEN CMainFrame::penSample = NULL;
HPEN CMainFrame::penEnvelope = NULL;
HPEN CMainFrame::penEnvelopeHighlight = NULL;
HPEN CMainFrame::penSeparator = NULL;
HBRUSH CMainFrame::brushGray = NULL;
HBRUSH CMainFrame::brushBlack = NULL;
HBRUSH CMainFrame::brushWhite = NULL;
HBRUSH CMainFrame::brushText = NULL;
//CBrush *CMainFrame::pbrushBlack = NULL;//rewbs.envRowGrid
//CBrush *CMainFrame::pbrushWhite = NULL;//rewbs.envRowGrid

HBRUSH CMainFrame::brushHighLight = NULL;
HBRUSH CMainFrame::brushHighLightRed = NULL;
HBRUSH CMainFrame::brushWindow = NULL;
HBRUSH CMainFrame::brushYellow = NULL;
HCURSOR CMainFrame::curDragging = NULL;
HCURSOR CMainFrame::curArrow = NULL;
HCURSOR CMainFrame::curNoDrop = NULL;
HCURSOR CMainFrame::curNoDrop2 = NULL;
HCURSOR CMainFrame::curVSplit = NULL;
LPMODPLUGDIB CMainFrame::bmpPatterns = NULL;
LPMODPLUGDIB CMainFrame::bmpNotes = NULL;
LPMODPLUGDIB CMainFrame::bmpVUMeters = NULL;
LPMODPLUGDIB CMainFrame::bmpVisNode = NULL;
LPMODPLUGDIB CMainFrame::bmpVisPcNode = NULL;
HPEN CMainFrame::gpenVuMeter[NUM_VUMETER_PENS*2];
COLORREF CMainFrame::rgbCustomColors[MAX_MODCOLORS] = 
	{
		RGB(0xFF, 0xFF, 0xFF), RGB(0x00, 0x00, 0x00), RGB(0xC0, 0xC0, 0xC0), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0xFF, 0xFF, 0xFF), 0x0000FF,
		RGB(0xFF, 0xFF, 0x80), RGB(0x00, 0x00, 0x00), RGB(0xE0, 0xE8, 0xE0),
		// Effect Colors
		RGB(0x00, 0x00, 0x80), RGB(0x00, 0x80, 0x80), RGB(0x00, 0x80, 0x00), RGB(0x00, 0x80, 0x80), RGB(0x80, 0x80, 0x00), RGB(0x80, 0x00, 0x00), RGB(0x00, 0x00, 0xFF),
		// VU-Meters
		RGB(0x00, 0xC8, 0x00), RGB(0xFF, 0xC8, 0x00), RGB(0xE1, 0x00, 0x00),
		// Channel separators
		GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_BTNFACE), GetSysColor(COLOR_BTNHIGHLIGHT),
		// Blend colour
		GetSysColor(COLOR_BTNFACE),
		// Dodgy commands
		RGB(0xC0, 0x00, 0x00),
	};

// Directory Arrays (Default + Last)
TCHAR CMainFrame::m_szDefaultDirectory[NUM_DIRS][_MAX_PATH] = {0};
TCHAR CMainFrame::m_szWorkingDirectory[NUM_DIRS][_MAX_PATH] = {0};
TCHAR CMainFrame::m_szKbdFile[_MAX_PATH] = "";			//rewbs.customKeys
// Directory to INI setting translation
const TCHAR CMainFrame::m_szDirectoryToSettingsName[NUM_DIRS][32] =
{
	_T("Songs_Directory"), _T("Samples_Directory"), _T("Instruments_Directory"), _T("Plugins_Directory"), _T("Plugin_Presets_Directory"), _T("Export_Directory"), _T("")
};


CInputHandler *CMainFrame::m_InputHandler = nullptr; //rewbs.customKeys
CAutoSaver *CMainFrame::m_pAutoSaver = nullptr; //rewbs.autosave
CPerformanceCounter *CMainFrame::m_pPerfCounter = nullptr;

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_XINFO,		//rewbs.xinfo
	ID_INDICATOR_INFO,
	ID_INDICATOR_USER,
	ID_INDICATOR_TIME,
	ID_INDICATOR_CPU
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction
//#include <direct.h>
CMainFrame::CMainFrame()
//----------------------
{
	m_bModTreeHasFocus = false;	//rewbs.customKeys
	m_pNoteMapHasFocus = nullptr;	//rewbs.customKeys
	m_pOrderlistHasFocus = nullptr;
	m_bOptionsLocked = false;	//rewbs.customKeys

	m_pJustModifiedDoc = nullptr;
	m_pModPlaying = nullptr;
	m_hFollowSong = NULL;
	m_hWndMidi = NULL;
	m_pSndFile = nullptr;
	m_dwStatus = 0;
	m_dwElapsedTime = 0;
	m_dwTimeSec = 0;
	m_dwNotifyType = 0;
	m_nTimer = 0;
	m_nAvgMixChn = m_nMixChn = 0;
	m_szUserText[0] = 0;
	m_szInfoText[0] = 0;
	m_szXInfoText[0]= 0;	//rewbs.xinfo

	for(UINT i = 0; i < NUM_DIRS; i++)
	{
		if (i == DIR_TUNING) // Hack: Tuning folder is set already so don't reset it.
			continue;
		MemsetZero(m_szDefaultDirectory[i]);
		MemsetZero(m_szWorkingDirectory[i]);
	}

	m_dTotalCPU=0;
	MemsetZero(gpenVuMeter);
	
	// Default chords
	MemsetZero(Chords);
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

	// Create Audio Critical Section
	MemsetZero(m_csAudio);
	InitializeCriticalSection(&m_csAudio);

	m_csRegKey.Format("%s%s", MAINFRAME_REGKEY_BASE, MAINFRAME_REGKEY_DEFAULT);
	m_csRegSettings.Format("%s%s", m_csRegKey, MAINFRAME_REGEXT_SETTINGS);
	m_csRegWindow.Format("%s%s", m_csRegKey, MAINFRAME_REGEXT_WINDOW);

	CString storedVersion = GetPrivateProfileCString("Version", "Version", "", theApp.GetConfigFileName());
	// If version number stored in INI is 1.17.02.40 or later, always load setting from INI file.
	// If it isn't, try loading from Registry first, then from the INI file.
	if (storedVersion >= "1.17.02.40" || !LoadRegistrySettings())
	{
		LoadIniSettings();
	}

	m_InputHandler = new CInputHandler(this); 	//rewbs.customKeys
	m_pPerfCounter= new CPerformanceCounter();

	//Loading static tunings here - probably not the best place to do that but anyway.
	CSoundFile::LoadStaticTunings();
}

void CMainFrame::LoadIniSettings()
//--------------------------------
{
	CString iniFile = theApp.GetConfigFileName();
	//CHAR collectedString[INIBUFFERSIZE];
	MptVersion::VersionNum vIniVersion;

	gcsPreviousVersion = GetPrivateProfileCString("Version", "Version", "", iniFile);
	if(gcsPreviousVersion == "")
		vIniVersion = MptVersion::num;
	else
		vIniVersion = MptVersion::ToNum(gcsPreviousVersion);

	gcsInstallGUID = GetPrivateProfileCString("Version", "InstallGUID", "", iniFile);
	if(gcsInstallGUID == "")
	{
		// No GUID found in INI file - generate one.
		GUID guid;
		CoCreateGuid(&guid);
		BYTE* Str;
		UuidToString((UUID*)&guid, &Str);
		gcsInstallGUID.Format("%s", (LPTSTR)Str);
		RpcStringFree(&Str);
	}

	gbMdiMaximize = GetPrivateProfileLong("Display", "MDIMaximize", true, iniFile);
	glTreeWindowWidth = GetPrivateProfileLong("Display", "MDITreeWidth", 160, iniFile);
	glTreeSplitRatio = GetPrivateProfileLong("Display", "MDITreeRatio", 128, iniFile);
	glGeneralWindowHeight = GetPrivateProfileLong("Display", "MDIGeneralHeight", 178, iniFile);
	glPatternWindowHeight = GetPrivateProfileLong("Display", "MDIPatternHeight", 152, iniFile);
	glSampleWindowHeight = GetPrivateProfileLong("Display", "MDISampleHeight", 188, iniFile);
	glInstrumentWindowHeight = GetPrivateProfileLong("Display", "MDIInstrumentHeight", 300, iniFile);
	glCommentsWindowHeight = GetPrivateProfileLong("Display", "MDICommentsHeight", 288, iniFile);
	glGraphWindowHeight = GetPrivateProfileLong("Display", "MDIGraphHeight", 288, iniFile); //rewbs.graph
	gnPlugWindowX = GetPrivateProfileInt("Display", "PlugSelectWindowX", 243, iniFile);
	gnPlugWindowY = GetPrivateProfileInt("Display", "PlugSelectWindowY", 273, iniFile);
	gnPlugWindowWidth = GetPrivateProfileInt("Display", "PlugSelectWindowWidth", 370, iniFile);
	gnPlugWindowHeight = GetPrivateProfileInt("Display", "PlugSelectWindowHeight", 332, iniFile);
	gnPlugWindowLast = GetPrivateProfileDWord("Display", "PlugSelectWindowLast", 0, iniFile);
	gnMsgBoxVisiblityFlags = GetPrivateProfileDWord("Display", "MsgBoxVisibilityFlags", uint32_max, iniFile);

	// Internet Update
	{
		tm lastUpdate;
		MemsetZero(lastUpdate);
		CString s = GetPrivateProfileCString("Update", "LastUpdateCheck", "1970-01-01 00:00", iniFile);
		if(sscanf(s, "%04d-%02d-%02d %02d:%02d", &lastUpdate.tm_year, &lastUpdate.tm_mon, &lastUpdate.tm_mday, &lastUpdate.tm_hour, &lastUpdate.tm_min) == 5)
		{
			lastUpdate.tm_year -= 1900;
			lastUpdate.tm_mon--;
		}

		time_t outTime = Util::sdTime::MakeGmTime(lastUpdate);
		
		if(outTime < 0) outTime = 0;

		CUpdateCheck::SetUpdateSettings
		(
			outTime,
			GetPrivateProfileInt("Update", "UpdateCheckPeriod", CUpdateCheck::GetUpdateCheckPeriod(), iniFile),
			GetPrivateProfileCString("Update", "UpdateURL", CUpdateCheck::GetUpdateURL(), iniFile),
			GetPrivateProfileInt("Update", "SendGUID", CUpdateCheck::GetSendGUID() ? 1 : 0, iniFile) ? true : false,
			GetPrivateProfileInt("Update", "ShowUpdateHint", CUpdateCheck::GetShowUpdateHint() ? 1 : 0, iniFile) ? true : false
		);
	}

	CHAR s[16];
	for (int ncol = 0; ncol < MAX_MODCOLORS; ncol++)
	{
		wsprintf(s, "Color%02d", ncol);
		rgbCustomColors[ncol] = GetPrivateProfileDWord("Display", s, rgbCustomColors[ncol], iniFile);
	}

#ifndef NO_DSOUND
	DWORD defaultDevice = SNDDEV_BUILD_ID(0, SNDDEV_DSOUND); // first DirectSound device
#else
	DWORD defaultDevice = SNDDEV_BUILD_ID(0, SNDDEV_WAVEOUT); // first WaveOut device
#endif // NO_DSOUND
#ifndef NO_ASIO
	// If there's an ASIO device available, prefer it over DirectSound
	if(EnumerateSoundDevices(SNDDEV_ASIO, 0, nullptr, 0))
	{
		defaultDevice = SNDDEV_BUILD_ID(0, SNDDEV_ASIO);
	}
#endif // NO_ASIO
	m_nWaveDevice = GetPrivateProfileLong("Sound Settings", "WaveDevice", defaultDevice, iniFile);
	m_dwSoundSetup = GetPrivateProfileDWord("Sound Settings", "SoundSetup", SOUNDSETUP_SECONDARY, iniFile);
	m_dwQuality = GetPrivateProfileDWord("Sound Settings", "Quality", 0, iniFile);
	m_nSrcMode = GetPrivateProfileDWord("Sound Settings", "SrcMode", SRCMODE_POLYPHASE, iniFile);
	m_dwRate = GetPrivateProfileDWord("Sound Settings", "Mixing_Rate", 0, iniFile);
	m_nBitsPerSample = GetPrivateProfileDWord("Sound Settings", "BitsPerSample", 16, iniFile);
	m_nChannels = GetPrivateProfileDWord("Sound Settings", "ChannelMode", 2, iniFile);
	m_nBufferLength = GetPrivateProfileDWord("Sound Settings", "BufferLength", 50, iniFile);
		if(m_nBufferLength < SNDDEV_MINBUFFERLEN) m_nBufferLength = SNDDEV_MINBUFFERLEN;
		if(m_nBufferLength > SNDDEV_MAXBUFFERLEN) m_nBufferLength = SNDDEV_MAXBUFFERLEN;
	if(m_dwRate == 0)
	{
		m_dwRate = 44100;
#ifndef NO_ASIO
		// If no mixing rate is specified and we're using ASIO, get a mixing rate supported by the device.
		if(SNDDEV_GET_TYPE(m_nWaveDevice) == SNDDEV_ASIO)
		{
			ISoundDevice *dummy;
			if(CreateSoundDevice(SNDDEV_ASIO, &dummy))
			{
				m_dwRate = dummy->GetCurrentSampleRate(SNDDEV_GET_NUMBER(m_nWaveDevice));
				delete dummy;
			}
		}
#endif // NO_ASIO
	}

	m_nPreAmp = GetPrivateProfileDWord("Sound Settings", "PreAmp", 128, iniFile);
	CSoundFile::m_nStereoSeparation = GetPrivateProfileLong("Sound Settings", "StereoSeparation", 128, iniFile);
	CSoundFile::m_nMaxMixChannels = GetPrivateProfileLong("Sound Settings", "MixChannels", MAX_CHANNELS, iniFile);
	gbWFIRType = static_cast<BYTE>(GetPrivateProfileDWord("Sound Settings", "XMMSModplugResamplerWFIRType", 7, iniFile));
	gdWFIRCutoff = static_cast<double>(GetPrivateProfileLong("Sound Settings", "ResamplerWFIRCutoff", 97, iniFile))/100.0;
	glVolumeRampSamples = GetPrivateProfileLong("Sound Settings", "VolumeRampSamples", 42, iniFile);

	m_dwMidiSetup = GetPrivateProfileDWord("MIDI Settings", "MidiSetup", m_dwMidiSetup, iniFile);
	m_nMidiDevice = GetPrivateProfileDWord("MIDI Settings", "MidiDevice", m_nMidiDevice, iniFile);
	gnMidiImportSpeed = GetPrivateProfileLong("MIDI Settings", "MidiImportSpeed", gnMidiImportSpeed, iniFile);
	gnMidiPatternLen = GetPrivateProfileLong("MIDI Settings", "MidiImportPatLen", gnMidiPatternLen, iniFile);

	m_dwPatternSetup = GetPrivateProfileDWord("Pattern Editor", "PatternSetup", m_dwPatternSetup, iniFile);
	if(vIniVersion < MAKE_VERSION_NUMERIC(1,17,02,50))
		m_dwPatternSetup |= PATTERN_NOTEFADE;
	if(vIniVersion < MAKE_VERSION_NUMERIC(1,17,03,01))
		m_dwPatternSetup |= PATTERN_RESETCHANNELS;
	if(vIniVersion < MAKE_VERSION_NUMERIC(1,19,00,07))
		m_dwPatternSetup &= ~0x800;					// this was previously deprecated and is now used for something else
	if(vIniVersion < MptVersion::num) 
		m_dwPatternSetup &= ~(0x200000|0x400000|0x10000000);	// various deprecated old options

	m_nRowSpacing = GetPrivateProfileDWord("Pattern Editor", "RowSpacing", 16, iniFile);
	m_nRowSpacing2 = GetPrivateProfileDWord("Pattern Editor", "RowSpacing2", 4, iniFile);
	gbLoopSong = GetPrivateProfileDWord("Pattern Editor", "LoopSong", true, iniFile);
	gnPatternSpacing = GetPrivateProfileDWord("Pattern Editor", "Spacing", 1, iniFile);
	gbPatternVUMeters = GetPrivateProfileDWord("Pattern Editor", "VU-Meters", false, iniFile);
	gbPatternPluginNames = GetPrivateProfileDWord("Pattern Editor", "Plugin-Names", true, iniFile);	
	gbPatternRecord = GetPrivateProfileDWord("Pattern Editor", "Record", true, iniFile);	
	gnAutoChordWaitTime = GetPrivateProfileDWord("Pattern Editor", "AutoChordWaitTime", 60, iniFile);	
	COrderList::s_nDefaultMargins = static_cast<BYTE>(GetPrivateProfileInt("Pattern Editor", "DefaultSequenceMargins", 2, iniFile));
	gbShowHackControls = (0 != GetPrivateProfileDWord("Misc", "ShowHackControls", 0, iniFile));
	CSoundFile::s_DefaultPlugVolumeHandling = static_cast<uint8>(GetPrivateProfileInt("Misc", "DefaultPlugVolumeHandling", PLUGIN_VOLUMEHANDLING_IGNORE, iniFile));
	if(CSoundFile::s_DefaultPlugVolumeHandling > 2) CSoundFile::s_DefaultPlugVolumeHandling = PLUGIN_VOLUMEHANDLING_IGNORE;

	m_nSampleUndoMaxBuffer = GetPrivateProfileLong("Sample Editor" , "UndoBufferSize", m_nSampleUndoMaxBuffer >> 20, iniFile);
	m_nSampleUndoMaxBuffer = max(1, m_nSampleUndoMaxBuffer) << 20;

	TCHAR szPath[_MAX_PATH] = "";
	for(size_t i = 0; i < NUM_DIRS; i++)
	{
		if(m_szDirectoryToSettingsName[i][0] == 0)
			continue;

		GetPrivateProfileString("Paths", m_szDirectoryToSettingsName[i], GetDefaultDirectory(static_cast<Directory>(i)), szPath, CountOf(szPath), iniFile);
		RelativePathToAbsolute(szPath);
		SetDefaultDirectory(szPath, static_cast<Directory>(i), false);

	}
	GetPrivateProfileString("Paths", "Key_Config_File", m_szKbdFile, m_szKbdFile, INIBUFFERSIZE, iniFile);
	RelativePathToAbsolute(m_szKbdFile);

	CSoundFile::m_nXBassDepth = GetPrivateProfileLong("Effects", "XBassDepth", 0, iniFile);
	CSoundFile::m_nXBassRange = GetPrivateProfileLong("Effects", "XBassRange", 0, iniFile);
	CSoundFile::m_nReverbDepth = GetPrivateProfileLong("Effects", "ReverbDepth", 0, iniFile);
	CSoundFile::gnReverbType = GetPrivateProfileLong("Effects", "ReverbType", 0, iniFile);
	CSoundFile::m_nProLogicDepth = GetPrivateProfileLong("Effects", "ProLogicDepth", 0, iniFile);
	CSoundFile::m_nProLogicDelay = GetPrivateProfileLong("Effects", "ProLogicDelay", 0, iniFile);

	GetPrivateProfileStruct("Effects", "EQ_Settings", &m_EqSettings, sizeof(EQPRESET), iniFile);
	GetPrivateProfileStruct("Effects", "EQ_User1", &CEQSetupDlg::gUserPresets[0], sizeof(EQPRESET), iniFile);
	GetPrivateProfileStruct("Effects", "EQ_User2", &CEQSetupDlg::gUserPresets[1], sizeof(EQPRESET), iniFile);
	GetPrivateProfileStruct("Effects", "EQ_User3", &CEQSetupDlg::gUserPresets[2], sizeof(EQPRESET), iniFile);
	GetPrivateProfileStruct("Effects", "EQ_User4", &CEQSetupDlg::gUserPresets[3], sizeof(EQPRESET), iniFile);


	m_pAutoSaver = new CAutoSaver();
	GetPrivateProfileLong("AutoSave", "Enabled", true, iniFile)?m_pAutoSaver->Enable():m_pAutoSaver->Disable();
	m_pAutoSaver->SetSaveInterval(GetPrivateProfileLong("AutoSave", "IntervalMinutes", 10, iniFile));
	m_pAutoSaver->SetHistoryDepth(GetPrivateProfileLong("AutoSave", "BackupHistory", 3, iniFile));
	m_pAutoSaver->SetUseOriginalPath(GetPrivateProfileLong("AutoSave", "UseOriginalPath", true, iniFile) != 0);
	GetPrivateProfileString("AutoSave", "Path", "", szPath, INIBUFFERSIZE, iniFile);
	RelativePathToAbsolute(szPath);
	m_pAutoSaver->SetPath(szPath);
	m_pAutoSaver->SetFilenameTemplate(GetPrivateProfileCString("AutoSave", "FileNameTemplate", "", iniFile));
}

bool CMainFrame::LoadRegistrySettings()
//-------------------------------------
{

	HKEY key;
	DWORD dwREG_DWORD = REG_DWORD;
	DWORD dwREG_SZ = REG_SZ;
	DWORD dwDWORDSize = sizeof(UINT);
	DWORD dwCRSIZE = sizeof(COLORREF);


	bool asEnabled=true;
	int asInterval=10;
	int asBackupHistory=3;
	bool asUseOriginalPath=true;
	CString asPath ="";
	CString asFileNameTemplate="";

	if (RegOpenKeyEx(HKEY_CURRENT_USER,	m_csRegWindow, 0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		DWORD d = 0;
		RegQueryValueEx(key, "Maximized", NULL, &dwREG_DWORD, (LPBYTE)&d, &dwDWORDSize);
		if (d) theApp.m_nCmdShow = SW_SHOWMAXIMIZED;
		RegQueryValueEx(key, "MDIMaximize", NULL, &dwREG_DWORD, (LPBYTE)&gbMdiMaximize, &dwDWORDSize);
		RegQueryValueEx(key, "MDITreeWidth", NULL, &dwREG_DWORD, (LPBYTE)&glTreeWindowWidth, &dwDWORDSize);
		RegQueryValueEx(key, "MDIGeneralHeight", NULL, &dwREG_DWORD, (LPBYTE)&glGeneralWindowHeight, &dwDWORDSize);
		RegQueryValueEx(key, "MDIPatternHeight", NULL, &dwREG_DWORD, (LPBYTE)&glPatternWindowHeight, &dwDWORDSize);
		RegQueryValueEx(key, "MDISampleHeight", NULL, &dwREG_DWORD,  (LPBYTE)&glSampleWindowHeight, &dwDWORDSize);
		RegQueryValueEx(key, "MDIInstrumentHeight", NULL, &dwREG_DWORD,  (LPBYTE)&glInstrumentWindowHeight, &dwDWORDSize);
		RegQueryValueEx(key, "MDICommentsHeight", NULL, &dwREG_DWORD,  (LPBYTE)&glCommentsWindowHeight, &dwDWORDSize);
		RegQueryValueEx(key, "MDIGraphHeight", NULL, &dwREG_DWORD,  (LPBYTE)&glGraphWindowHeight, &dwDWORDSize); //rewbs.graph
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

	if (RegOpenKeyEx(HKEY_CURRENT_USER,	m_csRegKey, 0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		RegQueryValueEx(key, "SoundSetup", NULL, &dwREG_DWORD, (LPBYTE)&m_dwSoundSetup, &dwDWORDSize);
		RegQueryValueEx(key, "Quality", NULL, &dwREG_DWORD, (LPBYTE)&m_dwQuality, &dwDWORDSize);
		RegQueryValueEx(key, "SrcMode", NULL, &dwREG_DWORD, (LPBYTE)&m_nSrcMode, &dwDWORDSize);
		RegQueryValueEx(key, "Mixing_Rate", NULL, &dwREG_DWORD, (LPBYTE)&m_dwRate, &dwDWORDSize);
		RegQueryValueEx(key, "BufferLength", NULL, &dwREG_DWORD, (LPBYTE)&m_nBufferLength, &dwDWORDSize);
		if ((m_nBufferLength < 10) || (m_nBufferLength > 200)) m_nBufferLength = 100;
		RegQueryValueEx(key, "PreAmp", NULL, &dwREG_DWORD, (LPBYTE)&m_nPreAmp, &dwDWORDSize);

		CHAR sPath[_MAX_PATH] = "";
		DWORD dwSZSIZE = sizeof(sPath);
		RegQueryValueEx(key, "Songs_Directory", NULL, &dwREG_SZ, (LPBYTE)sPath, &dwSZSIZE);
		SetDefaultDirectory(sPath, DIR_MODS);
		dwSZSIZE = sizeof(sPath);
		RegQueryValueEx(key, "Samples_Directory", NULL, &dwREG_SZ, (LPBYTE)sPath, &dwSZSIZE);
		SetDefaultDirectory(sPath, DIR_SAMPLES);
		dwSZSIZE = sizeof(sPath);
		RegQueryValueEx(key, "Instruments_Directory", NULL, &dwREG_SZ, (LPBYTE)sPath, &dwSZSIZE);
		SetDefaultDirectory(sPath, DIR_INSTRUMENTS);
		dwSZSIZE = sizeof(sPath);
		RegQueryValueEx(key, "Plugins_Directory", NULL, &dwREG_SZ, (LPBYTE)sPath, &dwSZSIZE);
		SetDefaultDirectory(sPath, DIR_PLUGINS);
		dwSZSIZE = sizeof(m_szKbdFile);
		RegQueryValueEx(key, "Key_Config_File", NULL, &dwREG_SZ, (LPBYTE)m_szKbdFile, &dwSZSIZE);

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
			m_dwPatternSetup &= ~(0x800|0x200000|0x400000);	// various deprecated old options
			m_dwPatternSetup |= PATTERN_NOTEFADE; // Set flag to maintain old behaviour (was changed in 1.17.02.50).
			m_dwPatternSetup |= PATTERN_RESETCHANNELS; // Set flag to reset channels on loop was changed in 1.17.03.01).
		RegQueryValueEx(key, "RowSpacing", NULL, &dwREG_DWORD, (LPBYTE)&m_nRowSpacing, &dwDWORDSize);
		RegQueryValueEx(key, "RowSpacing2", NULL, &dwREG_DWORD, (LPBYTE)&m_nRowSpacing2, &dwDWORDSize);
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

		//rewbs.resamplerConf
		dwDWORDSize = sizeof(gbWFIRType);
		RegQueryValueEx(key, "XMMSModplugResamplerWFIRType", NULL, &dwREG_DWORD, (LPBYTE)&gbWFIRType, &dwDWORDSize);
		dwDWORDSize = sizeof(gdWFIRCutoff);
		RegQueryValueEx(key, "ResamplerWFIRCutoff", NULL, &dwREG_DWORD, (LPBYTE)&gdWFIRCutoff, &dwDWORDSize);
		dwDWORDSize = sizeof(glVolumeRampSamples);
		RegQueryValueEx(key, "VolumeRampSamples", NULL, &dwREG_DWORD, (LPBYTE)&glVolumeRampSamples, &dwDWORDSize);
		
		//end rewbs.resamplerConf
		//rewbs.autochord
		dwDWORDSize = sizeof(gnAutoChordWaitTime);
		RegQueryValueEx(key, "AutoChordWaitTime", NULL, &dwREG_DWORD, (LPBYTE)&gnAutoChordWaitTime, &dwDWORDSize);
		//end rewbs.autochord

		dwDWORDSize = sizeof(gnPlugWindowX);
		RegQueryValueEx(key, "PlugSelectWindowX", NULL, &dwREG_DWORD, (LPBYTE)&gnPlugWindowX, &dwDWORDSize);
		dwDWORDSize = sizeof(gnPlugWindowY);
		RegQueryValueEx(key, "PlugSelectWindowY", NULL, &dwREG_DWORD, (LPBYTE)&gnPlugWindowY, &dwDWORDSize);
		dwDWORDSize = sizeof(gnPlugWindowWidth);
		RegQueryValueEx(key, "PlugSelectWindowWidth", NULL, &dwREG_DWORD, (LPBYTE)&gnPlugWindowWidth, &dwDWORDSize);
		dwDWORDSize = sizeof(gnPlugWindowHeight);
		RegQueryValueEx(key, "PlugSelectWindowHeight", NULL, &dwREG_DWORD, (LPBYTE)&gnPlugWindowHeight, &dwDWORDSize);
		dwDWORDSize = sizeof(gnPlugWindowLast);
		RegQueryValueEx(key, "PlugSelectWindowLast", NULL, &dwREG_DWORD, (LPBYTE)&gnPlugWindowLast, &dwDWORDSize);


		//rewbs.autoSave
		dwDWORDSize = sizeof(asEnabled);
		RegQueryValueEx(key, "AutoSave_Enabled", NULL, &dwREG_DWORD, (LPBYTE)&asEnabled, &dwDWORDSize);
		dwDWORDSize = sizeof(asInterval);
		RegQueryValueEx(key, "AutoSave_IntervalMinutes", NULL, &dwREG_DWORD, (LPBYTE)&asInterval, &dwDWORDSize);
		dwDWORDSize = sizeof(asBackupHistory);
		RegQueryValueEx(key, "AutoSave_BackupHistory", NULL, &dwREG_DWORD, (LPBYTE)&asBackupHistory, &dwDWORDSize);		
		dwDWORDSize = sizeof(asUseOriginalPath);
		RegQueryValueEx(key, "AutoSave_UseOriginalPath", NULL, &dwREG_DWORD, (LPBYTE)&asUseOriginalPath, &dwDWORDSize);		

		dwDWORDSize = MAX_PATH;
		RegQueryValueEx(key, "AutoSave_Path", NULL, &dwREG_DWORD, (LPBYTE)asPath.GetBuffer(dwDWORDSize/sizeof(TCHAR)), &dwDWORDSize);
		asPath.ReleaseBuffer();

		dwDWORDSize = MAX_PATH;
		RegQueryValueEx(key, "AutoSave_FileNameTemplate", NULL, &dwREG_DWORD, (LPBYTE)asFileNameTemplate.GetBuffer(dwDWORDSize/sizeof(TCHAR)), &dwDWORDSize);
		asFileNameTemplate.ReleaseBuffer();

		//end rewbs.autoSave

		RegCloseKey(key);
	} else
	{
		return false;
	}

	if (RegOpenKeyEx(HKEY_CURRENT_USER,	m_csRegSettings, 0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		// Version
		dwDWORDSize = sizeof(DWORD);
		DWORD dwPreviousVersion;
		RegQueryValueEx(key, "Version", NULL, &dwREG_DWORD, (LPBYTE)&dwPreviousVersion, &dwDWORDSize);
		gcsPreviousVersion = MptVersion::ToStr(dwPreviousVersion);
		RegCloseKey(key);
	}
	m_pAutoSaver = new CAutoSaver(asEnabled, asInterval, asBackupHistory, asUseOriginalPath, asPath, asFileNameTemplate);

	gnPatternSpacing = theApp.GetProfileInt("Pattern Editor", "Spacing", 0);
	gbPatternVUMeters = theApp.GetProfileInt("Pattern Editor", "VU-Meters", 0);
	gbPatternPluginNames = theApp.GetProfileInt("Pattern Editor", "Plugin-Names", 1);

	return true;
}


VOID CMainFrame::Initialize()
//---------------------------
{
	//Adding version number to the frame title
	CString title = GetTitle();
	title += CString(" ") + MptVersion::str;
	#ifdef DEBUG
		title += CString(" DEBUG");
	#endif
	#ifdef NO_VST
		title += " NO_VST";
	#endif
	#ifdef NO_ASIO
		title += " NO_ASIO";
	#endif
	#ifdef NO_DSOUND
		title += " NO_DSOUND";
	#endif
	SetTitle(title);
	OnUpdateFrameTitle(false);

	// Load Chords
	theApp.LoadChords(Chords);
	// Check for valid sound device
	if (!EnumerateSoundDevices(SNDDEV_GET_TYPE(m_nWaveDevice), SNDDEV_GET_NUMBER(m_nWaveDevice), nullptr, 0))
	{
		m_nWaveDevice = SNDDEV_BUILD_ID(0, SNDDEV_DSOUND);
		if (!EnumerateSoundDevices(SNDDEV_GET_TYPE(m_nWaveDevice), SNDDEV_GET_NUMBER(m_nWaveDevice), nullptr, 0))
		{
			m_nWaveDevice = SNDDEV_BUILD_ID(0, SNDDEV_WAVEOUT);
		}
	}
	// Default directory location
	for(UINT i = 0; i < NUM_DIRS; i++)
	{
		_tcscpy(m_szWorkingDirectory[i], m_szDefaultDirectory[i]);
	}
	if (m_szDefaultDirectory[DIR_MODS][0]) SetCurrentDirectory(m_szDefaultDirectory[DIR_MODS]);

	// Create Audio Thread
	m_hAudioWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hNotifyWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hPlayThread = CreateThread(NULL, 0, AudioThread, NULL, 0, &m_dwPlayThreadId);
	m_hNotifyThread = CreateThread(NULL, 0, NotifyThread, NULL, 0, &m_dwNotifyThreadId);
	// Setup timer
	OnUpdateUser(NULL);
	m_nTimer = SetTimer(1, MPTTIMER_PERIOD, NULL);
	
//rewbs: reduce to normal priority during debug for easier hang debugging
	//SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	// Setup Keyboard Hook
	ghKbdHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, AfxGetInstanceHandle(), GetCurrentThreadId());
	// Initialize Audio Mixer
	UpdateAudioParameters(TRUE);
	// Update the tree
	m_wndTree.Init();
}


CMainFrame::~CMainFrame()
//-----------------------
{
	DeleteCriticalSection(&m_csAudio);
	delete m_InputHandler; 	//rewbs.customKeys
	delete m_pAutoSaver; //rewbs.autosaver
	delete m_pPerfCounter;

	CChannelManagerDlg::DestroySharedInstance();
	CSoundFile::DeleteStaticdata();
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
	brushText = ::CreateSolidBrush(GetSysColor(COLOR_BTNTEXT));
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
	bmpVisNode = LoadDib(MAKEINTRESOURCE(IDB_VISNODE));
	bmpVisPcNode = LoadDib(MAKEINTRESOURCE(IDB_VISPCNODE));
	UpdateColors();
	// Toolbars
	EnableDocking(CBRS_ALIGN_ANY);
	if (!m_wndToolBar.Create(this)) return -1;
	if (!m_wndStatusBar.Create(this)) return -1;
	if (!m_wndTree.Create(this, IDD_TREEVIEW, CBRS_LEFT|CBRS_BORDER_RIGHT, IDD_TREEVIEW)) return -1;
	m_wndStatusBar.SetIndicators(indicators, CountOf(indicators));
	m_wndToolBar.Init(this);
	m_wndTree.RecalcLayout();

	RemoveControlBar(&m_wndStatusBar); //Removing statusbar because corrupted statusbar inifiledata can crash the program.
	m_wndTree.EnableDocking(0); //To prevent a crash when there's "Docking=1" for treebar in ini-settings.
	// Restore toobar positions
	LoadBarState("Toolbars");

	AddControlBar(&m_wndStatusBar); //Restore statusbar to mainframe.

	if(m_dwPatternSetup & PATTERN_MIDIRECORD) OnMidiRecord();

	return 0;
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
//------------------------------------------------
{
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
		if(TerminateThread(m_hPlayThread, 0)) m_hPlayThread = NULL;
	}
	if (m_hNotifyThread != NULL)
	{
		if(TerminateThread(m_hNotifyThread, 0)) m_hNotifyThread = NULL;
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
	if (bmpVisNode)
	{
		delete bmpVisNode;
		bmpVisNode = NULL;
	}
	if (bmpVisPcNode)
	{
		delete bmpVisPcNode;
		bmpVisPcNode = NULL;
	}

	// Kill GDI Objects
	DeleteGDIObject(brushGray);
	DeleteGDIObject(penLightGray);
	DeleteGDIObject(penDarkGray);
	DeleteGDIObject(penSample);
	DeleteGDIObject(penEnvelope);
	DeleteGDIObject(penEnvelopeHighlight);
	DeleteGDIObject(m_hFixedFont);
	DeleteGDIObject(m_hLargeFixedFont);
	DeleteGDIObject(penScratch);
	DeleteGDIObject(penGray00);
	DeleteGDIObject(penGray33);
	DeleteGDIObject(penGray40);
	DeleteGDIObject(penGray55);
	DeleteGDIObject(penGray80);
	DeleteGDIObject(penGray99);
	DeleteGDIObject(penGraycc);
	DeleteGDIObject(penGrayff);

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
	CChildFrame *pMDIActive = (CChildFrame *)MDIGetActive();

	BeginWaitCursor();
	if (m_dwStatus & MODSTATUS_PLAYING) PauseMod();
	if (pMDIActive) pMDIActive->SavePosition(TRUE);
	if (gpSoundDevice)
	{
		BEGIN_CRITICAL();
		//gpSoundDevice->Reset();
		//audioCloseDevice();
		gpSoundDevice->Release();
		gpSoundDevice = NULL;
		END_CRITICAL();
	}
	// Save Settings
	SaveIniSettings();
	if(m_InputHandler && m_InputHandler->activeCommandSet)
	{
		m_InputHandler->activeCommandSet->SaveFile(m_szKbdFile, false);
	}

	EndWaitCursor();
	CMDIFrameWnd::OnClose();
}

void CMainFrame::SaveIniSettings()
//--------------------------------
{
	CString iniFile = theApp.GetConfigFileName();

	CString version = MptVersion::str;
	WritePrivateProfileString("Version", "Version", version, iniFile);
	WritePrivateProfileString("Version", "InstallGUID", gcsInstallGUID, iniFile);
	
    WINDOWPLACEMENT wpl;
	wpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(&wpl);
	WritePrivateProfileStruct("Display", "WindowPlacement", &wpl, sizeof(WINDOWPLACEMENT), iniFile);

	WritePrivateProfileLong("Display", "MDIMaximize", gbMdiMaximize, iniFile);
	WritePrivateProfileLong("Display", "MDITreeWidth", glTreeWindowWidth, iniFile);
	WritePrivateProfileLong("Display", "MDITreeRatio", glTreeSplitRatio, iniFile);
	WritePrivateProfileLong("Display", "MDIGeneralHeight", glGeneralWindowHeight, iniFile);
	WritePrivateProfileLong("Display", "MDIPatternHeight", glPatternWindowHeight, iniFile);
	WritePrivateProfileLong("Display", "MDISampleHeight", glSampleWindowHeight, iniFile);
	WritePrivateProfileLong("Display", "MDIInstrumentHeight", glInstrumentWindowHeight, iniFile);
	WritePrivateProfileLong("Display", "MDICommentsHeight", glCommentsWindowHeight, iniFile);
	WritePrivateProfileLong("Display", "MDIGraphHeight", glGraphWindowHeight, iniFile); //rewbs.graph
	WritePrivateProfileLong("Display", "PlugSelectWindowX", gnPlugWindowX, iniFile);
	WritePrivateProfileLong("Display", "PlugSelectWindowY", gnPlugWindowY, iniFile);
	WritePrivateProfileLong("Display", "PlugSelectWindowWidth", gnPlugWindowWidth, iniFile);
	WritePrivateProfileLong("Display", "PlugSelectWindowHeight", gnPlugWindowHeight, iniFile);
	WritePrivateProfileLong("Display", "PlugSelectWindowLast", gnPlugWindowLast, iniFile);
	WritePrivateProfileDWord("Display", "MsgBoxVisibilityFlags", gnMsgBoxVisiblityFlags, iniFile);

	// Internet Update
	{
		CString outDate;
		const time_t t = CUpdateCheck::GetLastUpdateCheck();
		const tm* const lastUpdate = gmtime(&t);
		if(lastUpdate != nullptr)
		{
			outDate.Format("%04d-%02d-%02d %02d:%02d", lastUpdate->tm_year + 1900, lastUpdate->tm_mon + 1, lastUpdate->tm_mday, lastUpdate->tm_hour, lastUpdate->tm_min);
		}
		WritePrivateProfileString("Update", "LastUpdateCheck", outDate, iniFile);
		WritePrivateProfileLong("Update", "UpdateCheckPeriod", CUpdateCheck::GetUpdateCheckPeriod(), iniFile);
		WritePrivateProfileString("Update", "UpdateURL", CUpdateCheck::GetUpdateURL(), iniFile);
		WritePrivateProfileLong("Update", "SendGUID", CUpdateCheck::GetSendGUID() ? 1 : 0, iniFile);
		WritePrivateProfileLong("Update", "ShowUpdateHint", CUpdateCheck::GetShowUpdateHint() ? 1 : 0, iniFile);
	}

	CHAR s[16];
	for (int ncol = 0; ncol < MAX_MODCOLORS; ncol++)
	{
		wsprintf(s, "Color%02d", ncol);
		WritePrivateProfileDWord("Display", s, rgbCustomColors[ncol], iniFile);
	}
	
	WritePrivateProfileLong("Sound Settings", "WaveDevice", m_nWaveDevice, iniFile);
	WritePrivateProfileDWord("Sound Settings", "SoundSetup", m_dwSoundSetup, iniFile);
	WritePrivateProfileDWord("Sound Settings", "Quality", m_dwQuality, iniFile);
	WritePrivateProfileDWord("Sound Settings", "SrcMode", m_nSrcMode, iniFile);
	WritePrivateProfileDWord("Sound Settings", "Mixing_Rate", m_dwRate, iniFile);
	WritePrivateProfileDWord("Sound Settings", "BitsPerSample", m_nBitsPerSample, iniFile);
	WritePrivateProfileDWord("Sound Settings", "ChannelMode", m_nChannels, iniFile);
	WritePrivateProfileDWord("Sound Settings", "BufferLength", m_nBufferLength, iniFile);
	WritePrivateProfileDWord("Sound Settings", "PreAmp", m_nPreAmp, iniFile);
	WritePrivateProfileLong("Sound Settings", "StereoSeparation", CSoundFile::m_nStereoSeparation, iniFile);
	WritePrivateProfileLong("Sound Settings", "MixChannels", CSoundFile::m_nMaxMixChannels, iniFile);
	WritePrivateProfileDWord("Sound Settings", "XMMSModplugResamplerWFIRType", gbWFIRType, iniFile);
	WritePrivateProfileLong("Sound Settings", "ResamplerWFIRCutoff", static_cast<int>(gdWFIRCutoff*100+0.5), iniFile);
	WritePrivateProfileLong("Sound Settings", "VolumeRampSamples", glVolumeRampSamples, iniFile);

	WritePrivateProfileDWord("MIDI Settings", "MidiSetup", m_dwMidiSetup, iniFile);
	WritePrivateProfileDWord("MIDI Settings", "MidiDevice", m_nMidiDevice, iniFile);
	WritePrivateProfileLong("MIDI Settings", "MidiImportSpeed", gnMidiImportSpeed, iniFile);
	WritePrivateProfileLong("MIDI Settings", "MidiImportPatLen", gnMidiPatternLen, iniFile);

	WritePrivateProfileDWord("Pattern Editor", "PatternSetup", m_dwPatternSetup, iniFile);
	WritePrivateProfileDWord("Pattern Editor", "RowSpacing", m_nRowSpacing, iniFile);
	WritePrivateProfileDWord("Pattern Editor", "RowSpacing2", m_nRowSpacing2, iniFile);
	WritePrivateProfileDWord("Pattern Editor", "LoopSong", gbLoopSong, iniFile);
	WritePrivateProfileDWord("Pattern Editor", "Spacing", gnPatternSpacing, iniFile);
	WritePrivateProfileDWord("Pattern Editor", "VU-Meters", gbPatternVUMeters, iniFile);
	WritePrivateProfileDWord("Pattern Editor", "Plugin-Names", gbPatternPluginNames, iniFile);	
	WritePrivateProfileDWord("Pattern Editor", "Record", gbPatternRecord, iniFile);	
	WritePrivateProfileDWord("Pattern Editor", "AutoChordWaitTime", gnAutoChordWaitTime, iniFile);	

	// Write default paths
	const bool bConvertPaths = theApp.IsPortableMode();
	TCHAR szPath[_MAX_PATH] = "";
	for(size_t i = 0; i < NUM_DIRS; i++)
	{
		if(m_szDirectoryToSettingsName[i][0] == 0)
			continue;

		_tcscpy(szPath, GetDefaultDirectory(static_cast<Directory>(i)));
		if(bConvertPaths)
		{
			AbsolutePathToRelative(szPath);
		}
		WritePrivateProfileString("Paths", m_szDirectoryToSettingsName[i], szPath, iniFile);

	}
	// Obsolete, since we always write to Keybindings.mkb now. Older versions of OpenMPT 1.18+ will look for this file if this entry is missing, so this is kind of backwards compatible.
	WritePrivateProfileString("Paths", "Key_Config_File", NULL, iniFile);

	WritePrivateProfileLong("Effects", "XBassDepth", CSoundFile::m_nXBassDepth, iniFile);
	WritePrivateProfileLong("Effects", "XBassRange", CSoundFile::m_nXBassRange, iniFile);
	WritePrivateProfileLong("Effects", "ReverbDepth", CSoundFile::m_nReverbDepth, iniFile);
	WritePrivateProfileLong("Effects", "ReverbType", CSoundFile::gnReverbType, iniFile);
	WritePrivateProfileLong("Effects", "ProLogicDepth", CSoundFile::m_nProLogicDepth, iniFile);
	WritePrivateProfileLong("Effects", "ProLogicDelay", CSoundFile::m_nProLogicDelay, iniFile);

	WritePrivateProfileStruct("Effects", "EQ_Settings", &m_EqSettings, sizeof(EQPRESET), iniFile);
	WritePrivateProfileStruct("Effects", "EQ_User1", &CEQSetupDlg::gUserPresets[0], sizeof(EQPRESET), iniFile);
	WritePrivateProfileStruct("Effects", "EQ_User2", &CEQSetupDlg::gUserPresets[1], sizeof(EQPRESET), iniFile);
	WritePrivateProfileStruct("Effects", "EQ_User3", &CEQSetupDlg::gUserPresets[2], sizeof(EQPRESET), iniFile);
	WritePrivateProfileStruct("Effects", "EQ_User4", &CEQSetupDlg::gUserPresets[3], sizeof(EQPRESET), iniFile);

	WritePrivateProfileLong("AutoSave", "Enabled", m_pAutoSaver->IsEnabled(), iniFile);
	WritePrivateProfileLong("AutoSave", "IntervalMinutes", m_pAutoSaver->GetSaveInterval(), iniFile);
	WritePrivateProfileLong("AutoSave", "BackupHistory", m_pAutoSaver->GetHistoryDepth(), iniFile);
	WritePrivateProfileLong("AutoSave", "UseOriginalPath", m_pAutoSaver->GetUseOriginalPath(), iniFile);
	_tcscpy(szPath, m_pAutoSaver->GetPath()); 
	if(bConvertPaths)
	{
		AbsolutePathToRelative(szPath);
	}
	WritePrivateProfileString("AutoSave", "Path", szPath, iniFile);
	WritePrivateProfileString("AutoSave", "FileNameTemplate", m_pAutoSaver->GetFilenameTemplate(), iniFile);

	theApp.SaveChords(Chords);

	RemoveControlBar(&m_wndStatusBar); //Remove statusbar so that its state won't get saved.
	SaveBarState("Toolbars");
	AddControlBar(&m_wndStatusBar); //Restore statusbar to mainframe.
}

bool CMainFrame::WritePrivateProfileLong(const CString section, const CString key, const long value, const CString iniFile)
{
	CHAR valueBuffer[INIBUFFERSIZE];
	wsprintf(valueBuffer, "%li", value);
	return (WritePrivateProfileString(section, key, valueBuffer, iniFile) != 0);
}


long CMainFrame::GetPrivateProfileLong(const CString section, const CString key, const long defaultValue, const CString iniFile)
{
	CHAR defaultValueBuffer[INIBUFFERSIZE];
	wsprintf(defaultValueBuffer, "%li", defaultValue);

	CHAR valueBuffer[INIBUFFERSIZE];
	GetPrivateProfileString(section, key, defaultValueBuffer, valueBuffer, INIBUFFERSIZE, iniFile);

	return atol(valueBuffer);
}


bool CMainFrame::WritePrivateProfileDWord(const CString section, const CString key, const DWORD value, const CString iniFile)
{
	CHAR valueBuffer[INIBUFFERSIZE];
	wsprintf(valueBuffer, "%lu", value);
	return (WritePrivateProfileString(section, key, valueBuffer, iniFile) != 0);
}

DWORD CMainFrame::GetPrivateProfileDWord(const CString section, const CString key, const DWORD defaultValue, const CString iniFile)
{
	CHAR defaultValueBuffer[INIBUFFERSIZE];
	wsprintf(defaultValueBuffer, "%lu", defaultValue);

	CHAR valueBuffer[INIBUFFERSIZE];
	GetPrivateProfileString(section, key, defaultValueBuffer, valueBuffer, INIBUFFERSIZE, iniFile);
	return static_cast<DWORD>(atol(valueBuffer));
}

bool CMainFrame::WritePrivateProfileCString(const CString section, const CString key, const CString value, const CString iniFile) 
{
	return (WritePrivateProfileString(section, key, value, iniFile) != 0);
}

CString CMainFrame::GetPrivateProfileCString(const CString section, const CString key, const CString defaultValue, const CString iniFile)
{
	CHAR defaultValueBuffer[INIBUFFERSIZE];
	strcpy(defaultValueBuffer, defaultValue);
	CHAR valueBuffer[INIBUFFERSIZE];
	GetPrivateProfileString(section, key, defaultValueBuffer, valueBuffer, INIBUFFERSIZE, iniFile);
	return valueBuffer;
}



LRESULT CALLBACK CMainFrame::KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
//-------------------------------------------------------------------------------
{
	if (code>=0)
	{
		//Check if textbox has focus
		bool textboxHasFocus = false;
		bool handledByTextBox = false;

		HWND hWnd = ::GetFocus();
		if (hWnd != NULL) {
			TCHAR activeWindowClassName[512];
			GetClassName(hWnd, activeWindowClassName, 6);
			textboxHasFocus = _tcsicmp(activeWindowClassName, _T("Edit")) == 0;
			if (textboxHasFocus) {
				handledByTextBox = m_InputHandler->isKeyPressHandledByTextBox(wParam);
			}
		}

		if (!handledByTextBox && m_InputHandler->GeneralKeyEvent(kCtxAllContexts, code, wParam, lParam) != kcNull)
		{
			if (wParam != VK_ESCAPE)
				return -1;	// We've handled the keypress. No need to take it further.
							// Unless it was esc, in which case we need it to close Windows
							// (there might be other special cases, we'll see.. )
		}
	}

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
	if ((pMainFrm) && (pMainFrm->IsPlaying()) && (CMainFrame::gpSoundDevice)) {
		bOk = CMainFrame::gpSoundDevice->FillAudioBuffer(&gMPTSoundSource, gdwPlayLatency, dwUser);
	}/* else {
		CMainFrame::gpSoundDevice->SilenceAudioBuffer(&gMPTSoundSource, gdwPlayLatency, dwUser);
	}*/
	if (!bOk)
	{
		gbStopSent = TRUE;
		pMainFrm->PostMessage(WM_COMMAND, ID_PLAYER_STOP);
	}
	END_CRITICAL();
	return bOk;
}


void Terminate_AudioThread()
//----------------------------------------------
{	
	//TODO: Why does this not get called.
	AfxMessageBox("Audio thread terminated unexpectedly. Attempting to shut down audio device");
	CMainFrame* pMainFrame = CMainFrame::GetMainFrame();
	if (pMainFrame->gpSoundDevice) pMainFrame->gpSoundDevice->Reset();
	pMainFrame->audioCloseDevice();
	exit(-1);
}


// Audio thread
DWORD WINAPI CMainFrame::AudioThread(LPVOID)
//------------------------------------------
{
	CMainFrame *pMainFrm;
	BOOL bWait;
	UINT nSleep;

	set_terminate(Terminate_AudioThread);

// -> CODE#0021
// -> DESC="use multimedia timer instead of Sleep() in audio thread"
	HANDLE sleepEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
// -! BEHAVIOUR_CHANGE#0021

	bWait = TRUE;
	nSleep = 50;
// -> CODE#0021
// -> DESC="use multimedia timer instead of Sleep() in audio thread"
//rewbs: reduce to normal priority during debug for easier hang debugging
#ifdef NDEBUG
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
#endif 
#ifdef _DEBUG
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
#endif 
//	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
// -! BEHAVIOUR_CHANGE#0021
	for (;;)
	{
		if (bWait)
		{
			WaitForSingleObject(CMainFrame::m_hAudioWakeUp, 250);
		} else
		{
// -> CODE#0021
// -> DESC="use multimedia timer instead of Sleep() in audio thread"
//			Sleep(nSleep);
			timeSetEvent(nSleep,1,(LPTIMECALLBACK)sleepEvent,NULL,TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
			WaitForSingleObject(sleepEvent,nSleep);
			ResetEvent(sleepEvent);
// -! BEHAVIOUR_CHANGE#0021
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
					//CMainFrame::gpSoundDevice->SilenceAudioBuffer(&gMPTSoundSource, gdwPlayLatency);
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

// -> CODE#0021
// -> DESC="use multimedia timer instead of Sleep() in audio thread"
	// Commented as this caused "warning C4702: unreachable code"
	//CloseHandle(sleepEvent);
// -! BEHAVIOUR_CHANGE#0021

	// Commented the two lines below as those caused "warning C4702: unreachable code"
	//ExitThread(0);
	//return 0;
}


void Terminate_NotifyThread()
//----------------------------------------------
{	
	//TODO: Why does this not get called.
	AfxMessageBox("Notify thread terminated unexpectedly. Attempting to shut down audio device");
	CMainFrame* pMainFrame = CMainFrame::GetMainFrame();
	if (pMainFrame->gpSoundDevice) pMainFrame->gpSoundDevice->Reset();
	pMainFrame->audioCloseDevice();
	exit(-1);
}

// Notify thread
DWORD WINAPI CMainFrame::NotifyThread(LPVOID)
//-------------------------------------------
{
	CMainFrame *pMainFrm;

	set_terminate(Terminate_NotifyThread);

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
	// Commented the two lines below as those caused "warning C4702: unreachable code"
	//ExitThread(0);
	//return 0;
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
		//m_dTotalCPU = m_pPerfCounter->StartStop()/(static_cast<double>(dwSamplesRead)/m_dwRate);
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
	UINT nDevType = SNDDEV_GET_TYPE(m_nWaveDevice);
	UINT nDevNo = SNDDEV_GET_NUMBER(m_nWaveDevice);
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
			p->nPattern = m_pSndFile->m_nPattern;
			if (m_dwNotifyType & MPTNOTIFY_SAMPLE)
			{
				UINT nSmp = m_dwNotifyType & 0xFFFF;
				for (UINT k=0; k<MAX_CHANNELS; k++)
				{
					MODCHANNEL *pChn = &m_pSndFile->Chn[k];
					p->dwPos[k] = 0;
					if ((nSmp) && (nSmp <= m_pSndFile->m_nSamples) && (pChn->nLength)
					 && (pChn->pSample) && (pChn->pSample == m_pSndFile->Samples[nSmp].pSample)
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
					 && (pChn->pModInstrument) && (pChn->pModInstrument == m_pSndFile->Instruments[nIns])
					 && ((!(pChn->dwFlags & CHN_NOTEFADE)) || (pChn->nFadeOutVol)))
					{
						if (m_dwNotifyType & MPTNOTIFY_PITCHENV)
						{
							if (pChn->dwFlags & CHN_PITCHENV) p->dwPos[k] = MPTNOTIFY_POSVALID | (DWORD)(pChn->PitchEnv.nEnvPosition);
						} else
						if (m_dwNotifyType & MPTNOTIFY_PANENV)
						{
							if (pChn->dwFlags & CHN_PANENV) p->dwPos[k] = MPTNOTIFY_POSVALID | (DWORD)(pChn->PanEnv.nEnvPosition);
						} else
						{
							if (pChn->dwFlags & CHN_VOLENV) p->dwPos[k] = MPTNOTIFY_POSVALID | (DWORD)(pChn->VolEnv.nEnvPosition);
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
	if (penEnvelopeHighlight) DeleteObject(penEnvelopeHighlight);
	penEnvelopeHighlight = ::CreatePen(PS_SOLID, 0, RGB(0xFF, 0xFF, 0x00));

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
		if (brushHighLightRed) DeleteObject(brushHighLightRed);
		brushHighLightRed = CreateSolidBrush(RGB(0xFF,0x00,0x00));
		if (brushYellow) DeleteObject(brushYellow);
		brushYellow = CreateSolidBrush(RGB(0xFF,0xFF,0x00));

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


/////////////////////////////////////////////////////////////////////////////
// CMainFrame operations

UINT CMainFrame::GetBaseOctave()
//------------------------------
{
	return m_wndToolBar.GetBaseOctave();
}


void CMainFrame::SetPreAmp(UINT n)
//--------------------------------
{
	m_nPreAmp = n;
	if (m_pSndFile) m_pSndFile->SetMasterVolume(m_nPreAmp, true);
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
	if (!pModDoc) return FALSE;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if ((!pSndFile) || (!pSndFile->GetType())) return FALSE;
	const bool bPaused = pSndFile->IsPaused();
	const bool bPatLoop = (pSndFile->m_dwSongFlags & SONG_PATTERNLOOP) ? true : false;
	pSndFile->ResetChannels();
	// Select correct bidi loop mode when playing a module.
	pSndFile->SetupITBidiMode();

	if ((m_pSndFile) || (m_dwStatus & MODSTATUS_PLAYING)) PauseMod();
	if (((m_pSndFile) && (pSndFile != m_pSndFile)) || (!m_dwElapsedTime)) CSoundFile::ResetAGC();
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
			//rewbs.fix3185: removed this check so play position stays on last pattern if song ends and loop is off.
			//Otherwise play from cursor screws up.
			//if (pSndFile->GetCurrentPos() + 2 >= pSndFile->GetMaxPosition()) pSndFile->SetCurrentPos(0);

			// Tentative fix for http://bugs.openmpt.org/view.php?id=11 - Moved following line out of any condition checks
			//pSndFile->SetRepeatCount((gbLoopSong) ? -1 : 0);
		}
	}
	pSndFile->SetRepeatCount((gbLoopSong) ? -1 : 0);

	m_pSndFile->SetMasterVolume(m_nPreAmp, true);
	m_pSndFile->InitPlayer(TRUE);
	MemsetZero(NotifyBuffer);
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

		BEGIN_CRITICAL();
		m_pSndFile->SuspendPlugins(); 	//rewbs.VSTCompliance
		END_CRITICAL();

		m_nMixChn = m_nAvgMixChn = 0;
		Sleep(1);
		if (m_hFollowSong)
		{
			MPTNOTIFICATION mn;
			MemsetZero(mn);
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
		//m_pSndFile->LoopPattern(-1);
		//Commented above line - why loop should be disabled when pausing?

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
	m_pSndFile->SetMasterVolume(m_nPreAmp, true);
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
		m_WaveFile.SetRepeatCount(-1);
	}
	EndWaitCursor();
	return TRUE;
}


BOOL CMainFrame::PlaySoundFile(LPCSTR lpszFileName, UINT nNote)
//-------------------------------------------------------------
{
	CMappedFile f;
	bool bOk = false;

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

	//Avoid global volume ramping when trying samples in the treeview.
	m_WaveFile.m_pConfig->setGlobalVolumeAppliesToMaster(false);
	m_WaveFile.m_nDefaultGlobalVolume=64;

	m_WaveFile.m_nDefaultTempo = 125;
	m_WaveFile.m_nGlobalVolume=64;
	m_WaveFile.m_nDefaultSpeed = 4;
	m_WaveFile.SetRepeatCount(0);
	m_WaveFile.m_nType = MOD_TYPE_IT;
	m_WaveFile.m_nChannels = 4;
	m_WaveFile.m_nInstruments = 1;
	m_WaveFile.m_nSamples = 1;
	m_WaveFile.Order.resize(3);
	m_WaveFile.Order[0] = 0;
	m_WaveFile.Order[1] = 1;
	m_WaveFile.Order[2] = m_WaveFile.Order.GetInvalidPatIndex();
	m_WaveFile.Patterns.Insert(0,64);
	m_WaveFile.Patterns.Insert(1,64);
	if (m_WaveFile.Patterns[0])
	{
		if (!nNote) nNote = NOTE_MIDDLEC;
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
				if ((m_WaveFile.m_nSamples > 1) || (m_WaveFile.Samples[1].uFlags & CHN_LOOP))
				{
					MODCOMMAND *m = m_WaveFile.Patterns[0];
					m[32*4].note = NOTE_KEYOFF;
					m[32*4+1].note = NOTE_KEYOFF;
					m[63*4].note = NOTE_NOTECUT;
					m[63*4+1].note = NOTE_NOTECUT;
				} else
				{
					MODCOMMAND *m = m_WaveFile.Patterns[1];
					if (m)
					{
						m[63*4].command = CMD_POSITIONJUMP;
						m[63*4].param = 1;
					}
				}
				bOk = PlaySoundFile(&m_WaveFile) ? true : false;
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
	m_WaveFile.SetRepeatCount(0);
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
	m_WaveFile.Order.resize(3);
	m_WaveFile.Order[0] = 0;
	m_WaveFile.Order[1] = 1;
	m_WaveFile.Order[2] = m_WaveFile.Order.GetInvalidPatIndex();
	m_WaveFile.Patterns.Insert(0, 64);
	m_WaveFile.Patterns.Insert(1, 64);
	if (m_WaveFile.Patterns[0])
	{
		if (!nNote) nNote = 5*12+1;
		MODCOMMAND *m = m_WaveFile.Patterns[0];
		m[0].note = (BYTE)nNote;
		m[0].instr = 1;
		m[1].note = (BYTE)nNote;
		m[1].instr = 1;
		m = m_WaveFile.Patterns[1];
		m[32*4].note = NOTE_FADE;
		m[32*4+1].note = NOTE_FADE;
		m[63*4].note = NOTE_KEYOFF;
		m[63*4+1].note = NOTE_KEYOFF;
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


BOOL CMainFrame::SetupDirectories(LPCTSTR szModDir, LPCTSTR szSampleDir, LPCTSTR szInstrDir, LPCTSTR szVstDir, LPCTSTR szPresetDir)
//---------------------------------------------------------------------------------------------------------------------------------
{
	// will also set working directory
	SetDefaultDirectory(szModDir, DIR_MODS);
	SetDefaultDirectory(szSampleDir, DIR_SAMPLES);
	SetDefaultDirectory(szInstrDir, DIR_INSTRUMENTS);
	SetDefaultDirectory(szVstDir, DIR_PLUGINS);
	SetDefaultDirectory(szPresetDir, DIR_PLUGINPRESETS);
	return TRUE;
}

BOOL CMainFrame::SetupMiscOptions()
//---------------------------------
{
	if (CMainFrame::m_dwPatternSetup & PATTERN_MUTECHNMODE)
		CSoundFile::gdwSoundSetup |= SNDMIX_MUTECHNMODE;
	else
		CSoundFile::gdwSoundSetup &= ~SNDMIX_MUTECHNMODE;

	m_wndToolBar.EnableFlatButtons(m_dwPatternSetup & PATTERN_FLATBUTTONS);

	UpdateTree(NULL, HINT_MPTOPTIONS);
	UpdateAllViews(HINT_MPTOPTIONS, NULL);
	return true;
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

//rewbs.xinfo
VOID CMainFrame::SetXInfoText(LPCSTR lpszText)
//-------------------------------------------
{
	if (lpszText[0] | m_szXInfoText[0])
	{
		strcpy(m_szXInfoText, lpszText);
		OnUpdateInfo(NULL);
	}
}
//end rewbs.xinfo

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

	// Make sure that OnTimer() won't try to set the closed document modified anymore.
	if (pModDoc == m_pJustModifiedDoc) m_pJustModifiedDoc = 0; 

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
	if (m_bOptionsLocked)	//rewbs.customKeys
		return;
		
	CPropertySheet dlg("OpenMPT Setup", this, m_nLastOptionsPage);
	COptionsGeneral general;
	COptionsSoundcard sounddlg(m_dwRate, m_dwSoundSetup, m_nBitsPerSample, m_nChannels, m_nBufferLength, m_nWaveDevice);
	COptionsKeyboard keyboard;
	COptionsColors colors;
	COptionsPlayer playerdlg;
	CMidiSetupDlg mididlg(m_dwMidiSetup, m_nMidiDevice);
	CEQSetupDlg eqdlg(&m_EqSettings);
	CAutoSaverGUI autosavedlg(m_pAutoSaver); //rewbs.AutoSaver
	CUpdateSetupDlg updatedlg;
	dlg.AddPage(&general);
	dlg.AddPage(&sounddlg);
	dlg.AddPage(&playerdlg);
	dlg.AddPage(&eqdlg);
	dlg.AddPage(&keyboard);
	dlg.AddPage(&colors);
	dlg.AddPage(&mididlg);
	dlg.AddPage(&autosavedlg);
	dlg.AddPage(&updatedlg);
	m_bOptionsLocked=true;	//rewbs.customKeys
	dlg.DoModal();
	m_bOptionsLocked=false;	//rewbs.customKeys
	m_wndTree.OnOptionsChanged();
}


void CMainFrame::OnSongProperties()
//---------------------------------
{
	CModDoc* pModDoc = GetActiveDoc();
	if(pModDoc) pModDoc->SongProperties();
}


// -> CODE#0002
// -> DESC="list box to choose VST plugin presets (programs)"
void CMainFrame::OnPluginManager()
//--------------------------------
{
#ifndef NO_VST
	int nPlugslot=-1;
	CModDoc* pModDoc = GetActiveDoc();

	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		//Find empty plugin slot
		for (int nPlug=0; nPlug<MAX_MIXPLUGINS; nPlug++)
		{
			PSNDMIXPLUGIN pCandidatePlugin = &pSndFile->m_MixPlugins[nPlug];
			if (pCandidatePlugin->pMixPlugin == NULL)
			{
				nPlugslot=nPlug;
				break;
			}
		}
	}
	CSelectPluginDlg dlg(GetActiveDoc(), nPlugslot, this);
	if(dlg.DoModal() == IDOK && pModDoc)
	{
		pModDoc->SetModified();
        //Refresh views
		pModDoc->UpdateAllViews(NULL, HINT_MIXPLUGINS|HINT_MODTYPE);
		//Refresh Controls
		CChildFrame *pActiveChild = (CChildFrame *)MDIGetActive();
		pActiveChild->ForceRefresh();
	}
#endif // NO_VST
}
// -! NEW_FEATURE#0002


// -> CODE#0015
// -> DESC="channels management dlg"
void CMainFrame::OnChannelManager()
//---------------------------------
{
	if(GetActiveDoc() && CChannelManagerDlg::sharedInstance())
	{
		if(CChannelManagerDlg::sharedInstance()->IsDisplayed())
			CChannelManagerDlg::sharedInstance()->Hide();
		else
		{
			CChannelManagerDlg::sharedInstance()->SetDocument(NULL);
			CChannelManagerDlg::sharedInstance()->Show();
		}
	}
}
// -! NEW_FEATURE#0015
void CMainFrame::OnAddDlsBank()
//-----------------------------
{
	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(true, "dls", "",
		"All Sound Banks|*.dls;*.sbk;*.sf2;*.mss|"
		"Downloadable Sounds Banks (*.dls)|*.dls;*.mss|"
		"SoundFont 2.0 Banks (*.sf2)|*.sbk;*.sf2|"
		"All Files (*.*)|*.*||",
		"",
		true);
	if(files.abort) return;

	BeginWaitCursor();
	for(size_t counter = 0; counter < files.filenames.size(); counter++)
	{
		CTrackApp::AddDLSBank(files.filenames[counter].c_str());
	}
	m_wndTree.RefreshDlsBanks();
	EndWaitCursor();
}


void CMainFrame::OnImportMidiLib()
//--------------------------------
{
	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(true, "", "",
		"Text and INI files (*.txt,*.ini)|*.txt;*.ini;*.dls;*.sf2;*.sbk|"
		"Downloadable Sound Banks (*.dls)|*.dls;*.mss|"
		"SoundFont 2.0 banks (*.sf2)|*.sbk;*.sf2|"
		"Gravis UltraSound (ultrasnd.ini)|ultrasnd.ini|"
		"All Files (*.*)|*.*||");
	if(files.abort) return;

	BeginWaitCursor();
	CTrackApp::ImportMidiConfig(files.first_file.c_str());
	m_wndTree.RefreshMidiLibrary();
	EndWaitCursor();
}


void CMainFrame::SetLastMixActiveTime()		//rewbs.LiveVSTi
//-------------------------------------
{
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
	DWORD curTime = timeGetTime();
	if (IsPlaying())
	{
		gdwIdleTime = 0;
		if (curTime - gdwLastLowLatencyTime > 15000)
		{
			gdwPlayLatency = 0;
		}
		if ((m_pSndFile) && (m_pSndFile->IsPaused()) && (!m_pSndFile->m_nMixChannels))
		{
			//Log("%d (%d)\n", dwTime - gdwLastMixActiveTime, gdwLastMixActiveTime);
			if (curTime - gdwLastMixActiveTime > 5000)
			{
				//rewbs.instroVSTi: testing without shutting down audio device after 5s of idle time.
				//PauseMod();
			}
		} else
		{
			gdwLastMixActiveTime = curTime;
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

	if (m_pAutoSaver && m_pAutoSaver->IsEnabled())
	{
		bool success = m_pAutoSaver->DoSave(curTime);
		if (!success)		// autosave failure; bring up options.
		{
			CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_AUTOSAVE;
			OnViewOptions();
		}
	}
	
	// Ensure the modified flag gets set in the WinMain thread, even if modification
	// originated from Audio Thread (access to CWnd is not thread safe).
	// Flaw: if 2 docs are modified in between Timer ticks (very rare), one mod will be lost.
	/*CModDoc* pModDoc = GetActiveDoc();
	if (pModDoc && pModDoc->m_bModifiedChanged) {
		pModDoc->SetModifiedFlag(pModDoc->m_bDocModified);
		pModDoc->m_bModifiedChanged=false;
	}*/
	if (m_pJustModifiedDoc)
	{
		m_pJustModifiedDoc->SetModified(true);
		m_pJustModifiedDoc = NULL;
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

//rewbs.customKeys
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
//end rewbs.customKeys

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
			if(nPat < m_pSndFile->Patterns.Size())
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


void CMainFrame::OnUpdateCPU(CCmdUI *)
//-------------------------------------
{
/*	CString s;
	double totalCPUPercent = m_dTotalCPU*100;
	UINT intPart = static_cast<int>(totalCPUPercent);
	UINT decPart = static_cast<int>(totalCPUPercent-intPart)*100;
	s.Format("%d.%d%%", intPart, decPart);
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_CPU), s, TRUE);*/
}

//rewbs.xinfo
void CMainFrame::OnUpdateXInfo(CCmdUI *)
//-------------------------------------
{
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_XINFO), m_szXInfoText, TRUE);
}
//end rewbs.xinfo

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


void CMainFrame::OnPanic()
//------------------------
{
	// "Panic button." At the moment, it just resets all VSTi and sample notes.
	if(m_pModPlaying)
		m_pModPlaying->OnPanic();
}


void CMainFrame::OnPrevOctave()
//-----------------------------
{
	UINT n = GetBaseOctave();
	if (n > MIN_BASEOCTAVE) m_wndToolBar.SetBaseOctave(n-1);
// -> CODE#0009
// -> DESC="instrument editor note play & octave change"
//	SwitchToActiveView();
// -! BEHAVIOUR_CHANGE#0009
}


void CMainFrame::OnNextOctave()
//-----------------------------
{
	UINT n = GetBaseOctave();
	if (n < MAX_BASEOCTAVE) m_wndToolBar.SetBaseOctave(n+1);
// -> CODE#0009
// -> DESC="instrument editor note play & octave change"
//	SwitchToActiveView();
// -! BEHAVIOUR_CHANGE#0009
}


void CMainFrame::OnOctaveChanged()
//--------------------------------
{
	SwitchToActiveView();
}

//rewbs.reportBug
void CMainFrame::OnReportBug()
//----------------------------
{
	CTrackApp::OpenURL("http://bugs.openmpt.org/");
	return;
}
//end rewbs.reportBug

BOOL CMainFrame::OnInternetLink(UINT nID)
//---------------------------------------
{
	LPCSTR pszURL = NULL;

	switch(nID)
	{
	case ID_NETLINK_MODPLUG:	pszURL = "http://openmpt.org/"; break;
	case ID_NETLINK_TOP_PICKS:	pszURL = "http://openmpt.org/top_picks"; break;
	/*
	case ID_NETLINK_OPENMPTWIKI:pszURL = "http://wiki.openmpt.org/"; break;
//	case ID_NETLINK_UT:			pszURL = "http://www.united-trackers.org"; break;
//	case ID_NETLINK_OSMUSIC:	pszURL = "http://www.osmusic.net/"; break;
//	case ID_NETLINK_HANDBOOK:	pszURL = "http://www.modplug.com/mods/handbook/handbook.htm"; break;
	case ID_NETLINK_MPTFR:		pszURL = "http://mpt.new.fr/"; break;
	case ID_NETLINK_FORUMS:		pszURL = "http://forum.openmpt.org/"; break;
	case ID_NETLINK_PLUGINS:	pszURL = "http://www.kvraudio.com/"; break;
	case ID_NETLINK_MODARCHIVE: pszURL = "http://modarchive.org/"; break;
	case ID_NETLINK_OPENMPTWIKI_GERMAN: pszURL = "http://wikide.openmpt.org/Hauptseite"; break;
	*/
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


LRESULT CMainFrame::OnSpecialKey(WPARAM /*vKey*/, LPARAM)
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

//rewbs.customKeys
LRESULT CMainFrame::OnCustomKeyMsg(WPARAM wParam, LPARAM lParam)
//---------------------------------------------------------------
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
		case kcPauseSong:	OnPlayerPause(); break;
		case kcPrevOctave:	OnPrevOctave(); break;
		case kcNextOctave:	OnNextOctave(); break;
		case kcFileNew:		theApp.OnFileNew(); break;
		case kcFileOpen:	theApp.OnFileOpen(); break;
		case kcMidiRecord:	OnMidiRecord(); break;
		case kcHelp: 		CMDIFrameWnd::OnHelp(); break;
		case kcViewAddPlugin: OnPluginManager(); break;
		case kcViewChannelManager: OnChannelManager(); break;
		case kcViewMIDImapping: OnViewMIDIMapping(); break;
		case kcViewEditHistory:	OnViewEditHistory(); break;
		case kcNextDocument:	MDINext(); break;
		case kcPrevDocument:	MDIPrev(); break;


		//D'oh!! moddoc isn't a CWnd so we have to handle its messages and pass them on.

		case kcFileSaveAs:
		case kcFileSaveAsWave:
		case kcFileSaveMidi:
		case kcFileExportCompat:
		case kcFileClose:
		case kcFileSave:
		case kcViewGeneral:
		case kcViewPattern:
		case kcViewSamples:
		case kcViewInstruments:
		case kcViewComments: 
		case kcViewGraph: //rewbs.graph
		case kcViewSongProperties:
		case kcPlayPatternFromCursor:
		case kcPlayPatternFromStart: 
		case kcPlaySongFromCursor: 
		case kcPlaySongFromStart: 
		case kcPlayPauseSong:
		case kcStopSong:
		case kcEstimateSongLength:
		case kcApproxRealBPM:
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
			if (m_pOrderlistHasFocus)
				return m_pOrderlistHasFocus->PostMessage(WM_MOD_KEYCOMMAND, wParam, lParam);

			//Else send it to the active view
			CView* pView = GetActiveView();
			if (pView)
				return pView->PostMessage(WM_MOD_KEYCOMMAND, wParam, lParam);
	}

	return wParam;
}
//end rewbs.customKeys

void CMainFrame::OnInitMenu(CMenu* pMenu)
//---------------------------------------
{
	m_InputHandler->SetModifierMask(0);
	if (m_InputHandler->noAltMenu())
		return;

	CMDIFrameWnd::OnInitMenu(pMenu);
	
}

//end rewbs.VSTTimeInfo
long CMainFrame::GetSampleRate()
//------------------------------
{
	return CSoundFile::GetSampleRate();
}

long CMainFrame::GetTotalSampleCount()
//------------------------------------
{
	if (GetModPlaying())
		return GetModPlaying()->GetSoundFile()->m_lTotalSampleCount;
	return 0;
}

double CMainFrame::GetApproxBPM()
//-------------------------------
{
	CSoundFile *pSndFile = NULL;

	pSndFile = GetActiveDoc()->GetSoundFile();
	if (pSndFile) {
		return pSndFile->GetCurrentBPM();
	}
	return 0;
}

BOOL CMainFrame::InitRenderer(CSoundFile* pSndFile)
//-------------------------------------------------
{
	BEGIN_CRITICAL();
	pSndFile->m_bIsRendering=true;
	pSndFile->SuspendPlugins();
	pSndFile->ResumePlugins();
	END_CRITICAL();
	m_dwStatus |= MODSTATUS_RENDERING;
	m_pModPlaying = GetActiveDoc();
	return true;
}

BOOL CMainFrame::StopRenderer(CSoundFile* pSndFile)
//-------------------------------------------------
{
	m_dwStatus &= ~MODSTATUS_RENDERING;
	m_pModPlaying = NULL;
	BEGIN_CRITICAL();
	pSndFile->SuspendPlugins();
	pSndFile->m_bIsRendering=false;
	END_CRITICAL();
	return true;
}
//end rewbs.VSTTimeInfo

//rewbs.customKeys
// We have swicthed focus to a new module - might need to update effect keys to reflect module type
bool CMainFrame::UpdateEffectKeys(void)
//-------------------------------------
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
//end rewbs.customKeys


//rewbs.fix3116
void CMainFrame::OnKillFocus(CWnd* pNewWnd)
//-----------------------------------------
{
	CMDIFrameWnd::OnKillFocus(pNewWnd);
	
	//rewbs: ensure modifiers are reset when we leave the window (e.g. alt-tab)
	CMainFrame::GetMainFrame()->GetInputHandler()->SetModifierMask(0);
	//end rewbs
}
//end rewbs.fix3116

void CMainFrame::OnShowWindow(BOOL bShow, UINT /*nStatus*/)
//---------------------------------------------------------
{
    static bool firstShow = true;
    if (bShow && !IsWindowVisible() && firstShow)
	{
        firstShow = false;
		WINDOWPLACEMENT wpl;
		if (GetPrivateProfileStruct("Display", "WindowPlacement", &wpl, sizeof(WINDOWPLACEMENT), theApp.GetConfigFileName()))
		{
			SetWindowPlacement(&wpl);
		}
    }
}


void CMainFrame::OnViewMIDIMapping()
//----------------------------------
{
	CModDoc* pModDoc = GetActiveDoc();
	CSoundFile* pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : nullptr;
	if(!pSndFile) return;

	const HWND oldMIDIRecondWnd = GetMidiRecordWnd();
	CMIDIMappingDialog dlg(this, *pSndFile);
	dlg.DoModal();
	SetMidiRecordWnd(oldMIDIRecondWnd);
}


void CMainFrame::OnViewEditHistory()
//----------------------------------
{
	CModDoc* pModDoc = GetActiveDoc();
	if(pModDoc != nullptr)
	{
		pModDoc->OnViewEditHistory();
	}
}


void CMainFrame::OnInternetUpdate()
//---------------------------------
{
	CUpdateCheck *updateCheck = CUpdateCheck::Create(false);
	updateCheck->DoUpdateCheck();
}


/////////////////////////////////////////////
//Misc helper functions
/////////////////////////////////////////////

void AddPluginNamesToCombobox(CComboBox& CBox, SNDMIXPLUGIN* plugarray, const bool librarynames)
//----------------------------------------------------------------------------------------------
{
#ifndef NO_VST
	for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
	{
		PSNDMIXPLUGIN p = &plugarray[iPlug];
		CString str;
		str.Preallocate(80);
		str.Format("FX%d: ", iPlug+1);
		const int size0 = str.GetLength();
		str += (librarynames) ? p->GetLibraryName() : p->GetName();
		if(str.GetLength() <= size0) str += "undefined";

		CBox.SetItemData(CBox.AddString(str), iPlug + 1);
	}
#endif // NO_VST
}

void AddPluginParameternamesToCombobox(CComboBox& CBox, SNDMIXPLUGIN& plug)
//-------------------------------------------------------------------------
{
	if(plug.pMixPlugin)
		AddPluginParameternamesToCombobox(CBox, *(CVstPlugin *)plug.pMixPlugin);
}

void AddPluginParameternamesToCombobox(CComboBox& CBox, CVstPlugin& plug)
//-----------------------------------------------------------------------
{
	char s[72], sname[64];
	const PlugParamIndex nParams = plug.GetNumParameters();
	for (PlugParamIndex i = 0; i < nParams; i++)
	{
		plug.GetParamName(i, sname, sizeof(sname));
		wsprintf(s, "%02d: %s", i, sname);
		CBox.SetItemData(CBox.AddString(s), i);
	}
}


// retrieve / set default directory from given string and store it our setup variables
void CMainFrame::SetDirectory(const LPCTSTR szFilenameFrom, Directory dir, TCHAR (&directories)[NUM_DIRS][_MAX_PATH], bool bStripFilename)
//----------------------------------------------------------------------------------------------------------------------------------------
{
	TCHAR szPath[_MAX_PATH], szDir[_MAX_DIR];

	if(bStripFilename)
	{
		_tsplitpath(szFilenameFrom, szPath, szDir, 0, 0);
		_tcscat(szPath, szDir);
	}
	else
	{
		_tcscpy(szPath, szFilenameFrom);
	}

	TCHAR szOldDir[sizeof(directories[dir])]; // for comparison
	_tcscpy(szOldDir, directories[dir]);

	_tcscpy(directories[dir], szPath);

	// When updating default directory, also update the working directory.
	if(szPath[0] && directories == m_szDefaultDirectory)
	{
		if(_tcscmp(szOldDir, szPath) != 0) // update only if default directory has changed
			SetWorkingDirectory(szPath, dir);
	}
}

void CMainFrame::SetDefaultDirectory(const LPCTSTR szFilenameFrom, Directory dir, bool bStripFilename)
//----------------------------------------------------------------------------------------------------
{
	SetDirectory(szFilenameFrom, dir, m_szDefaultDirectory, bStripFilename);
}


void CMainFrame::SetWorkingDirectory(const LPCTSTR szFilenameFrom, Directory dir, bool bStripFilename)
//----------------------------------------------------------------------------------------------------
{
	SetDirectory(szFilenameFrom, dir, m_szWorkingDirectory, bStripFilename);
}


LPCTSTR CMainFrame::GetDefaultDirectory(Directory dir)
//----------------------------------------------------
{
	return m_szDefaultDirectory[dir];
}


LPCTSTR CMainFrame::GetWorkingDirectory(Directory dir)
//----------------------------------------------------
{
	return m_szWorkingDirectory[dir];
}


// Convert an absolute path to a path that's relative to OpenMPT's directory.
// Paths are relative to the executable path.
// nLength specifies the maximum number of character that can be written into szPath,
// including the trailing null char.
template <size_t nLength>
void CMainFrame::AbsolutePathToRelative(TCHAR (&szPath)[nLength])
//---------------------------------------------------------------
{
	STATIC_ASSERT(nLength >= 3);

	if(_tcslen(szPath) == 0)
		return;

	const size_t nStrLength = nLength - 1;	// "usable" length, i.e. not including the null char.
	TCHAR szExePath[nLength], szTempPath[nLength];
	_tcsncpy(szExePath, theApp.GetAppDirPath(), nStrLength);
	SetNullTerminator(szExePath);

	if(!_tcsncicmp(szExePath, szPath, _tcslen(szExePath)))
	{
		// Path is OpenMPT's directory or a sub directory ("C:\OpenMPT\Somepath" => ".\Somepath")
		_tcscpy(szTempPath, _T(".\\"));	// ".\"
		_tcsncat(szTempPath, &szPath[_tcslen(szExePath)], nStrLength - 2);	// "Somepath"
		_tcscpy(szPath, szTempPath);
	} else if(!_tcsncicmp(szExePath, szPath, 2))
	{
		// Path is on the same drive as OpenMPT ("C:\Somepath" => "\Somepath")
		_tcsncpy(szTempPath, &szPath[2], nStrLength);	// "\Somepath"
		_tcscpy(szPath, szTempPath);
	}
	SetNullTerminator(szPath);
}


// Convert a relative path to an absolute path.
// Paths are relative to the executable path.
// nLength specifies the maximum number of character that can be written into szPath,
// including the trailing null char.
template <size_t nLength>
void CMainFrame::RelativePathToAbsolute(TCHAR (&szPath)[nLength])
//---------------------------------------------------------------
{
	STATIC_ASSERT(nLength >= 3);

	if(_tcslen(szPath) == 0)
		return;

	const size_t nStrLength = nLength - 1;	// "usable" length, i.e. not including the null char.
	TCHAR szExePath[nLength], szTempPath[nLength] = _T("");
	_tcsncpy(szExePath, theApp.GetAppDirPath(), nStrLength);
	SetNullTerminator(szExePath);

	if(!_tcsncicmp(szPath, _T("\\"), 1) && _tcsncicmp(szPath, _T("\\\\"), 2))
	{
		// Path is on the same drive as OpenMPT ("\Somepath\" => "C:\Somepath\"), but ignore network paths starting with "\\"
		_tcsncat(szTempPath, szExePath, 2);	// "C:"
		_tcsncat(szTempPath, szPath, nStrLength - 2);	// "\Somepath\"
		_tcscpy(szPath, szTempPath);
	} else if(!_tcsncicmp(szPath, _T(".\\"), 2))
	{
		// Path is OpenMPT's directory or a sub directory (".\Somepath\" => "C:\OpenMPT\Somepath\")
		_tcsncpy(szTempPath, szExePath, nStrLength);	// "C:\OpenMPT\"
		if(_tcslen(szTempPath) < nStrLength)
		{
			_tcsncat(szTempPath, &szPath[2], nStrLength - _tcslen(szTempPath));	//	"Somepath"
		}
		_tcscpy(szPath, szTempPath);
	}
	SetNullTerminator(szPath);
}
