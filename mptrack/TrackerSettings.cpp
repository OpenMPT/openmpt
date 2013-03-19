/*
 * TrackerSettings.cpp
 * -------------------
 * Purpose: Code for managing, loading and saving all applcation settings.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Moddoc.h"
#include "Mainfrm.h"
#include "snddev.h"
#include "SNDDEVX.H"
#include "../common/version.h"
#include "UpdateCheck.h"
#include "Mpdlgs.h"
#include "AutoSaver.h"
#include "../common/StringFixer.h"
#include "TrackerSettings.h"
#include "../common/misc_util.h"
#include "PatternClipboard.h"


TrackerSettings TrackerSettings::settings;
const TCHAR *TrackerSettings::m_szDirectoryToSettingsName[NUM_DIRS] = { _T("Songs_Directory"), _T("Samples_Directory"), _T("Instruments_Directory"), _T("Plugins_Directory"), _T("Plugin_Presets_Directory"), _T("Export_Directory"), _T(""), _T("") };


TrackerSettings::TrackerSettings()
//--------------------------------
{
	m_ShowSplashScreen = true;
	gnPatternSpacing = 0;
	gbPatternRecord = TRUE;
	gbPatternVUMeters = FALSE;
	gbPatternPluginNames = TRUE;
	gbMdiMaximize = TRUE;
	gbShowHackControls = false;
	//rewbs.varWindowSize
	glGeneralWindowHeight = 178;
	glPatternWindowHeight = 152;
	glSampleWindowHeight = 188;
	glInstrumentWindowHeight = 300;
	glCommentsWindowHeight = 288;
	glGraphWindowHeight = 288; //rewbs.graph
	//end rewbs.varWindowSize
	glTreeWindowWidth = 160;
	glTreeSplitRatio = 128;

	gcsPreviousVersion = 0;
	gcsInstallGUID = "";
	// Audio Setup
	gnAutoChordWaitTime = 60;
	gnMsgBoxVisiblityFlags = uint32_max;

	// Audio device
	m_dwRate = 44100;	// Default value will be overridden
	m_dwSoundSetup = SOUNDSETUP_SECONDARY;
	m_nChannels = 2;
	m_dwQuality = 0;
	m_nSrcMode = SRCMODE_FIRFILTER;
	m_nBitsPerSample = 16;
	m_nPreAmp = 128;
	gbLoopSong = TRUE;
	m_nWaveDevice = SNDDEV_BUILD_ID(0, SNDDEV_WAVEOUT);	// Default value will be overridden
	m_nBufferLength = 50;

	// Default EQ settings
	MemCopy(m_EqSettings, CEQSetupDlg::gEQPresets[0]);

	// MIDI Setup
	m_nMidiDevice = 0;
	m_dwMidiSetup = MIDISETUP_RECORDVELOCITY | MIDISETUP_RECORDNOTEOFF | MIDISETUP_TRANSPOSEKEYBOARD | MIDISETUP_MIDITOPLUG;
	aftertouchBehaviour = atDoNotRecord;
	midiVelocityAmp = 100;
	
	// MIDI Import
	midiImportPatternLen = 128;
	midiImportSpeed = 3;

	// Pattern Setup
	m_dwPatternSetup = PATTERN_PLAYNEWNOTE | PATTERN_EFFECTHILIGHT
		| PATTERN_SMALLFONT | PATTERN_CENTERROW | PATTERN_DRAGNDROPEDIT
		| PATTERN_FLATBUTTONS | PATTERN_NOEXTRALOUD | PATTERN_2NDHIGHLIGHT
		| PATTERN_STDHIGHLIGHT | PATTERN_SHOWPREVIOUS | PATTERN_CONTSCROLL
		| PATTERN_SYNCMUTE | PATTERN_AUTODELAY | PATTERN_NOTEFADE
		| PATTERN_LARGECOMMENTS | PATTERN_SHOWDEFAULTVOLUME;
	m_nRowHighlightMeasures = 16;
	m_nRowHighlightBeats = 4;
	recordQuantizeRows = 0;
	rowDisplayOffset = 0;

	m_nSampleUndoMaxBuffer = 0;	// Real sample buffer undo size will be set later.

	GetDefaultColourScheme(rgbCustomColors);

	// Directory Arrays (Default + Last)
	for(size_t i = 0; i < NUM_DIRS; i++)
	{
		if(i == DIR_TUNING) // Hack: Tuning folder is already set so don't reset it.
			continue;
		m_szDefaultDirectory[i][0] = '\0';
		m_szWorkingDirectory[i][0] = '\0';
	}
	m_szKbdFile[0] = '\0';

	// Default chords
	MemsetZero(Chords);
	for(UINT ichord = 0; ichord < 3 * 12; ichord++)
	{
		Chords[ichord].key = (uint8)ichord;
		Chords[ichord].notes[0] = 0;
		Chords[ichord].notes[1] = 0;
		Chords[ichord].notes[2] = 0;

		if(ichord < 12)
		{
			// Major Chords
			Chords[ichord].notes[0] = (uint8)(ichord+5);
			Chords[ichord].notes[1] = (uint8)(ichord+8);
			Chords[ichord].notes[2] = (uint8)(ichord+11);
		} else if(ichord < 24)
		{
			// Minor Chords
			Chords[ichord].notes[0] = (uint8)(ichord-8);
			Chords[ichord].notes[1] = (uint8)(ichord-4);
			Chords[ichord].notes[2] = (uint8)(ichord-1);
		}
	}

	defaultModType = MOD_TYPE_IT;

	gnPlugWindowX = 243;
	gnPlugWindowY = 273;
	gnPlugWindowWidth = 370;
	gnPlugWindowHeight = 332;
	gnPlugWindowLast = 0;

}


void TrackerSettings::GetDefaultColourScheme(COLORREF (&colours)[MAX_MODCOLORS])
//------------------------------------------------------------------------------
{
	colours[MODCOLOR_BACKNORMAL] = RGB(0xFF, 0xFF, 0xFF);
	colours[MODCOLOR_TEXTNORMAL] = RGB(0x00, 0x00, 0x00);
	colours[MODCOLOR_BACKCURROW] = RGB(0xC0, 0xC0, 0xC0);
	colours[MODCOLOR_TEXTCURROW] = RGB(0x00, 0x00, 0x00);
	colours[MODCOLOR_BACKSELECTED] = RGB(0x00, 0x00, 0x00);
	colours[MODCOLOR_TEXTSELECTED] = RGB(0xFF, 0xFF, 0xFF);
	colours[MODCOLOR_SAMPLE] = RGB(0xFF, 0x00, 0x00);
	colours[MODCOLOR_BACKPLAYCURSOR] = RGB(0xFF, 0xFF, 0x80);
	colours[MODCOLOR_TEXTPLAYCURSOR] = RGB(0x00, 0x00, 0x00);
	colours[MODCOLOR_BACKHILIGHT] = RGB(0xE0, 0xE8, 0xE0);
	// Effect Colors
	colours[MODCOLOR_NOTE] = RGB(0x00, 0x00, 0x80);
	colours[MODCOLOR_INSTRUMENT] = RGB(0x00, 0x80, 0x80);
	colours[MODCOLOR_VOLUME] = RGB(0x00, 0x80, 0x00);
	colours[MODCOLOR_PANNING] = RGB(0x00, 0x80, 0x80);
	colours[MODCOLOR_PITCH] = RGB(0x80, 0x80, 0x00);
	colours[MODCOLOR_GLOBALS] = RGB(0x80, 0x00, 0x00);
	colours[MODCOLOR_ENVELOPES] = RGB(0x00, 0x00, 0xFF);
	// VU-Meters
	colours[MODCOLOR_VUMETER_LO] = RGB(0x00, 0xC8, 0x00);
	colours[MODCOLOR_VUMETER_MED] = RGB(0xFF, 0xC8, 0x00);
	colours[MODCOLOR_VUMETER_HI] = RGB(0xE1, 0x00, 0x00);
	// Channel separators
	colours[MODCOLOR_SEPSHADOW] = GetSysColor(COLOR_BTNSHADOW);
	colours[MODCOLOR_SEPFACE] = GetSysColor(COLOR_BTNFACE);
	colours[MODCOLOR_SEPHILITE] = GetSysColor(COLOR_BTNHIGHLIGHT);
	// Pattern blend colour
	colours[MODCOLOR_BLENDCOLOR] = GetSysColor(COLOR_BTNFACE);
	// Dodgy commands
	colours[MODCOLOR_DODGY_COMMANDS] = RGB(0xC0, 0x00, 0x00);
}


void TrackerSettings::LoadSettings()
//----------------------------------
{
	const CString iniFile = theApp.GetConfigFileName();

	CString storedVersion = CMainFrame::GetPrivateProfileCString("Version", "Version", "", iniFile);
	// If version number stored in INI is 1.17.02.40 or later, always load setting from INI file.
	// If it isn't, try loading from Registry first, then from the INI file.
	if (storedVersion >= "1.17.02.40" || !LoadRegistrySettings())
	{
		LoadINISettings(iniFile);
	}

	// The following stuff was also stored in mptrack.ini while the registry was still being used...

	// Load Chords
	LoadChords(Chords);

	// Load default macro configuration
	MIDIMacroConfig macros;
	theApp.GetDefaultMidiMacro(&macros);
	for(int isfx = 0; isfx < 16; isfx++)
	{
		CHAR snam[8];
		wsprintf(snam, "SF%X", isfx);
		GetPrivateProfileString("Zxx Macros", snam, macros.szMidiSFXExt[isfx], macros.szMidiSFXExt[isfx], CountOf(macros.szMidiSFXExt[isfx]), iniFile);
		StringFixer::SetNullTerminator(macros.szMidiSFXExt[isfx]);
	}
	for(int izxx = 0; izxx < 128; izxx++)
	{
		CHAR snam[8];
		wsprintf(snam, "Z%02X", izxx | 0x80);
		GetPrivateProfileString("Zxx Macros", snam, macros.szMidiZXXExt[izxx], macros.szMidiZXXExt[izxx], CountOf(macros.szMidiZXXExt[izxx]), iniFile);
		StringFixer::SetNullTerminator(macros.szMidiZXXExt[izxx]);
	}
	// Fix old nasty broken (non-standard) MIDI configs in INI file.
	if(storedVersion >= "1.17" && storedVersion < "1.20")
	{
		macros.UpgradeMacros();
	}
	theApp.SetDefaultMidiMacro(&macros);

	// Default directory location
	for(UINT i = 0; i < NUM_DIRS; i++)
	{
		_tcscpy(m_szWorkingDirectory[i], m_szDefaultDirectory[i]);
	}
	if (m_szDefaultDirectory[DIR_MODS][0]) SetCurrentDirectory(m_szDefaultDirectory[DIR_MODS]);
}


void TrackerSettings::LoadINISettings(const CString &iniFile)
//----------------------------------------------------------
{
	MptVersion::VersionNum vIniVersion;

	vIniVersion = gcsPreviousVersion = MptVersion::ToNum(CMainFrame::GetPrivateProfileCString("Version", "Version", "", iniFile));
	if(vIniVersion == 0)
		vIniVersion = MptVersion::num;

	gcsInstallGUID = CMainFrame::GetPrivateProfileCString("Version", "InstallGUID", "", iniFile);
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

	// GUI Stuff
	m_ShowSplashScreen = CMainFrame::GetPrivateProfileLong("Display", "ShowSplashScreen", m_ShowSplashScreen, iniFile);
	gbMdiMaximize = CMainFrame::GetPrivateProfileLong("Display", "MDIMaximize", gbMdiMaximize, iniFile);
	glTreeWindowWidth = CMainFrame::GetPrivateProfileLong("Display", "MDITreeWidth", glTreeWindowWidth, iniFile);
	glTreeSplitRatio = CMainFrame::GetPrivateProfileLong("Display", "MDITreeRatio", glTreeSplitRatio, iniFile);
	glGeneralWindowHeight = CMainFrame::GetPrivateProfileLong("Display", "MDIGeneralHeight", glGeneralWindowHeight, iniFile);
	glPatternWindowHeight = CMainFrame::GetPrivateProfileLong("Display", "MDIPatternHeight", glPatternWindowHeight, iniFile);
	glSampleWindowHeight = CMainFrame::GetPrivateProfileLong("Display", "MDISampleHeight", glSampleWindowHeight, iniFile);
	glInstrumentWindowHeight = CMainFrame::GetPrivateProfileLong("Display", "MDIInstrumentHeight", glInstrumentWindowHeight, iniFile);
	glCommentsWindowHeight = CMainFrame::GetPrivateProfileLong("Display", "MDICommentsHeight", glCommentsWindowHeight, iniFile);
	glGraphWindowHeight = CMainFrame::GetPrivateProfileLong("Display", "MDIGraphHeight", glGraphWindowHeight, iniFile); //rewbs.graph
	gnPlugWindowX = GetPrivateProfileInt("Display", "PlugSelectWindowX", gnPlugWindowX, iniFile);
	gnPlugWindowY = GetPrivateProfileInt("Display", "PlugSelectWindowY", gnPlugWindowY, iniFile);
	gnPlugWindowWidth = GetPrivateProfileInt("Display", "PlugSelectWindowWidth", gnPlugWindowWidth, iniFile);
	gnPlugWindowHeight = GetPrivateProfileInt("Display", "PlugSelectWindowHeight", gnPlugWindowHeight, iniFile);
	gnPlugWindowLast = CMainFrame::GetPrivateProfileDWord("Display", "PlugSelectWindowLast", gnPlugWindowLast, iniFile);
	gnMsgBoxVisiblityFlags = CMainFrame::GetPrivateProfileDWord("Display", "MsgBoxVisibilityFlags", gnMsgBoxVisiblityFlags, iniFile);

	// Internet Update
	{
		tm lastUpdate;
		MemsetZero(lastUpdate);
		CString s = CMainFrame::GetPrivateProfileCString("Update", "LastUpdateCheck", "1970-01-01 00:00", iniFile);
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
			CMainFrame::GetPrivateProfileCString("Update", "UpdateURL", CUpdateCheck::GetUpdateURL(), iniFile),
			GetPrivateProfileInt("Update", "SendGUID", CUpdateCheck::GetSendGUID() ? 1 : 0, iniFile) != 0,
			GetPrivateProfileInt("Update", "ShowUpdateHint", CUpdateCheck::GetShowUpdateHint() ? 1 : 0, iniFile) != 0
			);
	}

	CHAR s[16];
	for (int ncol = 0; ncol < MAX_MODCOLORS; ncol++)
	{
		wsprintf(s, "Color%02d", ncol);
		rgbCustomColors[ncol] = CMainFrame::GetPrivateProfileDWord("Display", s, rgbCustomColors[ncol], iniFile);
	}

	DWORD defaultDevice = SNDDEV_BUILD_ID(0, SNDDEV_WAVEOUT); // first WaveOut device
#ifndef NO_ASIO
	// If there's an ASIO device available, prefer it over DirectSound
	if(EnumerateSoundDevices(SNDDEV_ASIO, 0, nullptr, 0))
	{
		defaultDevice = SNDDEV_BUILD_ID(0, SNDDEV_ASIO);
	}
	CASIODevice::baseChannel = GetPrivateProfileInt("Sound Settings", "ASIOBaseChannel", CASIODevice::baseChannel, iniFile);
#endif // NO_ASIO
	m_nWaveDevice = CMainFrame::GetPrivateProfileLong("Sound Settings", "WaveDevice", defaultDevice, iniFile);
	m_dwSoundSetup = CMainFrame::GetPrivateProfileDWord("Sound Settings", "SoundSetup", m_dwSoundSetup, iniFile);
	m_dwQuality = CMainFrame::GetPrivateProfileDWord("Sound Settings", "Quality", m_dwQuality, iniFile);
	m_nSrcMode = CMainFrame::GetPrivateProfileDWord("Sound Settings", "SrcMode", m_nSrcMode, iniFile);
	m_dwRate = CMainFrame::GetPrivateProfileDWord("Sound Settings", "Mixing_Rate", 0, iniFile);
	m_nBitsPerSample = CMainFrame::GetPrivateProfileDWord("Sound Settings", "BitsPerSample", m_nBitsPerSample, iniFile);
	m_nChannels = CMainFrame::GetPrivateProfileDWord("Sound Settings", "ChannelMode", m_nChannels, iniFile);
	m_nBufferLength = CMainFrame::GetPrivateProfileDWord("Sound Settings", "BufferLength", m_nBufferLength, iniFile);
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

	m_nPreAmp = CMainFrame::GetPrivateProfileDWord("Sound Settings", "PreAmp", m_nPreAmp, iniFile);
	CSoundFile::m_nStereoSeparation = CMainFrame::GetPrivateProfileLong("Sound Settings", "StereoSeparation", CSoundFile::m_nStereoSeparation, iniFile);
	CSoundFile::m_nMaxMixChannels = CMainFrame::GetPrivateProfileLong("Sound Settings", "MixChannels", CSoundFile::m_nMaxMixChannels, iniFile);
	m_MixerSettings.gbWFIRType = static_cast<BYTE>(CMainFrame::GetPrivateProfileDWord("Sound Settings", "XMMSModplugResamplerWFIRType", m_MixerSettings.gbWFIRType, iniFile));
	//gdWFIRCutoff = static_cast<double>(CMainFrame::GetPrivateProfileLong("Sound Settings", "ResamplerWFIRCutoff", gdWFIRCutoff * 100.0, iniFile)) / 100.0;
	m_MixerSettings.gdWFIRCutoff = static_cast<double>(CMainFrame::GetPrivateProfileLong("Sound Settings", "ResamplerWFIRCutoff", Util::Round<long>(m_MixerSettings.gdWFIRCutoff * 100.0), iniFile)) / 100.0;
	
	// Ramping... first try to read the old setting, then the new ones
	const long volRamp = CMainFrame::GetPrivateProfileLong("Sound Settings", "VolumeRampSamples", -1, iniFile);
	if(volRamp != -1)
	{
		m_MixerSettings.glVolumeRampUpSamples = m_MixerSettings.glVolumeRampDownSamples = volRamp;
	}
	m_MixerSettings.glVolumeRampUpSamples = CMainFrame::GetPrivateProfileLong("Sound Settings", "VolumeRampUpSamples", m_MixerSettings.glVolumeRampUpSamples, iniFile);
	m_MixerSettings.glVolumeRampDownSamples = CMainFrame::GetPrivateProfileLong("Sound Settings", "VolumeRampDownSamples", m_MixerSettings.glVolumeRampDownSamples, iniFile);

	m_dwMidiSetup = CMainFrame::GetPrivateProfileDWord("MIDI Settings", "MidiSetup", m_dwMidiSetup, iniFile);
	m_nMidiDevice = CMainFrame::GetPrivateProfileDWord("MIDI Settings", "MidiDevice", m_nMidiDevice, iniFile);
	aftertouchBehaviour = static_cast<RecordAftertouchOptions>(CMainFrame::GetPrivateProfileDWord("MIDI Settings", "AftertouchBehaviour", aftertouchBehaviour, iniFile));
	midiVelocityAmp = static_cast<uint16>(CMainFrame::GetPrivateProfileLong("MIDI Settings", "MidiVelocityAmp", midiVelocityAmp, iniFile));
	midiImportSpeed = CMainFrame::GetPrivateProfileLong("MIDI Settings", "MidiImportSpeed", midiImportSpeed, iniFile);
	midiImportPatternLen = CMainFrame::GetPrivateProfileLong("MIDI Settings", "MidiImportPatLen", midiImportPatternLen, iniFile);
	if((m_dwMidiSetup & 0x40) != 0 && vIniVersion < MAKE_VERSION_NUMERIC(1, 20, 00, 86))
	{
		// This flag used to be "amplify MIDI Note Velocity" - with a fixed amplification factor of 2.
		midiVelocityAmp = 200;
		m_dwMidiSetup &= ~0x40;
	}

	m_dwPatternSetup = CMainFrame::GetPrivateProfileDWord("Pattern Editor", "PatternSetup", m_dwPatternSetup, iniFile);
	if(vIniVersion < MAKE_VERSION_NUMERIC(1, 17, 02, 50))
		m_dwPatternSetup |= PATTERN_NOTEFADE;
	if(vIniVersion < MAKE_VERSION_NUMERIC(1, 17, 03, 01))
		m_dwPatternSetup |= PATTERN_RESETCHANNELS;
	if(vIniVersion < MAKE_VERSION_NUMERIC(1, 19, 00, 07))
		m_dwPatternSetup &= ~0x800;					// this was previously deprecated and is now used for something else
	if(vIniVersion < MAKE_VERSION_NUMERIC(1, 20, 00, 04))
		m_dwPatternSetup &= ~0x200000;				// dito
	if(vIniVersion < MAKE_VERSION_NUMERIC(1, 20, 00, 07))
		m_dwPatternSetup &= ~0x400000;				// dito
	if(vIniVersion < MAKE_VERSION_NUMERIC(1, 20, 00, 39))
		m_dwPatternSetup &= ~0x10000000;			// dito

	m_nRowHighlightMeasures = CMainFrame::GetPrivateProfileDWord("Pattern Editor", "RowSpacing", m_nRowHighlightMeasures, iniFile);
	m_nRowHighlightBeats = CMainFrame::GetPrivateProfileDWord("Pattern Editor", "RowSpacing2", m_nRowHighlightBeats, iniFile);
	gbLoopSong = CMainFrame::GetPrivateProfileDWord("Pattern Editor", "LoopSong", gbLoopSong, iniFile);
	gnPatternSpacing = CMainFrame::GetPrivateProfileDWord("Pattern Editor", "Spacing", gnPatternSpacing, iniFile);
	gbPatternVUMeters = CMainFrame::GetPrivateProfileDWord("Pattern Editor", "VU-Meters", gbPatternVUMeters, iniFile);
	gbPatternPluginNames = CMainFrame::GetPrivateProfileDWord("Pattern Editor", "Plugin-Names", gbPatternPluginNames, iniFile);
	gbPatternRecord = CMainFrame::GetPrivateProfileDWord("Pattern Editor", "Record", gbPatternRecord, iniFile);
	gnAutoChordWaitTime = CMainFrame::GetPrivateProfileDWord("Pattern Editor", "AutoChordWaitTime", gnAutoChordWaitTime, iniFile);
	orderlistMargins = GetPrivateProfileInt("Pattern Editor", "DefaultSequenceMargins", orderlistMargins, iniFile);
	rowDisplayOffset = GetPrivateProfileInt("Pattern Editor", "RowDisplayOffset", rowDisplayOffset, iniFile);
	recordQuantizeRows = CMainFrame::GetPrivateProfileDWord("Pattern Editor", "RecordQuantize", recordQuantizeRows, iniFile);
	gbShowHackControls = (0 != CMainFrame::GetPrivateProfileDWord("Misc", "ShowHackControls", gbShowHackControls ? 1 : 0, iniFile));
	CSoundFile::s_DefaultPlugVolumeHandling = static_cast<uint8>(GetPrivateProfileInt("Misc", "DefaultPlugVolumeHandling", CSoundFile::s_DefaultPlugVolumeHandling, iniFile));
	if(CSoundFile::s_DefaultPlugVolumeHandling >= PLUGIN_VOLUMEHANDLING_MAX) CSoundFile::s_DefaultPlugVolumeHandling = PLUGIN_VOLUMEHANDLING_IGNORE;

	m_nSampleUndoMaxBuffer = CMainFrame::GetPrivateProfileLong("Sample Editor" , "UndoBufferSize", m_nSampleUndoMaxBuffer >> 20, iniFile);
	m_nSampleUndoMaxBuffer = max(1, m_nSampleUndoMaxBuffer) << 20;

	PatternClipboard::SetClipboardSize(GetPrivateProfileInt("Pattern Editor", "NumClipboards", PatternClipboard::GetClipboardSize(), iniFile));
	
	// Default Paths
	TCHAR szPath[_MAX_PATH] = "";
	for(size_t i = 0; i < NUM_DIRS; i++)
	{
		if(m_szDirectoryToSettingsName[i][0] == '\0')
			continue;

		GetPrivateProfileString("Paths", m_szDirectoryToSettingsName[i], GetDefaultDirectory(static_cast<Directory>(i)), szPath, CountOf(szPath), iniFile);
		theApp.RelativePathToAbsolute(szPath);
		SetDefaultDirectory(szPath, static_cast<Directory>(i), false);

	}
	GetPrivateProfileString("Paths", "Key_Config_File", m_szKbdFile, m_szKbdFile, INIBUFFERSIZE, iniFile);
	theApp.RelativePathToAbsolute(m_szKbdFile);


	// Effects Settings
	CSoundFile::m_nXBassDepth = CMainFrame::GetPrivateProfileLong("Effects", "XBassDepth", CSoundFile::m_nXBassDepth, iniFile);
	CSoundFile::m_nXBassRange = CMainFrame::GetPrivateProfileLong("Effects", "XBassRange", CSoundFile::m_nXBassRange, iniFile);
	CSoundFile::m_nReverbDepth = CMainFrame::GetPrivateProfileLong("Effects", "ReverbDepth", CSoundFile::m_nReverbDepth, iniFile);
	CSoundFile::gnReverbType = CMainFrame::GetPrivateProfileLong("Effects", "ReverbType", CSoundFile::gnReverbType, iniFile);
	CSoundFile::m_nProLogicDepth = CMainFrame::GetPrivateProfileLong("Effects", "ProLogicDepth", CSoundFile::m_nProLogicDepth, iniFile);
	CSoundFile::m_nProLogicDelay = CMainFrame::GetPrivateProfileLong("Effects", "ProLogicDelay", CSoundFile::m_nProLogicDelay, iniFile);


	// EQ Settings
	GetPrivateProfileStruct("Effects", "EQ_Settings", &m_EqSettings, sizeof(EQPreset), iniFile);
	GetPrivateProfileStruct("Effects", "EQ_User1", &CEQSetupDlg::gUserPresets[0], sizeof(EQPreset), iniFile);
	GetPrivateProfileStruct("Effects", "EQ_User2", &CEQSetupDlg::gUserPresets[1], sizeof(EQPreset), iniFile);
	GetPrivateProfileStruct("Effects", "EQ_User3", &CEQSetupDlg::gUserPresets[2], sizeof(EQPreset), iniFile);
	GetPrivateProfileStruct("Effects", "EQ_User4", &CEQSetupDlg::gUserPresets[3], sizeof(EQPreset), iniFile);
	StringFixer::SetNullTerminator(m_EqSettings.szName);
	StringFixer::SetNullTerminator(CEQSetupDlg::gUserPresets[0].szName);
	StringFixer::SetNullTerminator(CEQSetupDlg::gUserPresets[1].szName);
	StringFixer::SetNullTerminator(CEQSetupDlg::gUserPresets[2].szName);
	StringFixer::SetNullTerminator(CEQSetupDlg::gUserPresets[3].szName);


	// Auto saver settings
	CMainFrame::m_pAutoSaver = new CAutoSaver();
	if(CMainFrame::GetPrivateProfileLong("AutoSave", "Enabled", true, iniFile))
	{
		CMainFrame::m_pAutoSaver->Enable();
	} else
	{
		CMainFrame::m_pAutoSaver->Disable();
	}
	CMainFrame::m_pAutoSaver->SetSaveInterval(CMainFrame::GetPrivateProfileLong("AutoSave", "IntervalMinutes", 10, iniFile));
	CMainFrame::m_pAutoSaver->SetHistoryDepth(CMainFrame::GetPrivateProfileLong("AutoSave", "BackupHistory", 3, iniFile));
	CMainFrame::m_pAutoSaver->SetUseOriginalPath(CMainFrame::GetPrivateProfileLong("AutoSave", "UseOriginalPath", true, iniFile) != 0);
	GetPrivateProfileString("AutoSave", "Path", "", szPath, INIBUFFERSIZE, iniFile);
	theApp.RelativePathToAbsolute(szPath);
	CMainFrame::m_pAutoSaver->SetPath(szPath);
	CMainFrame::m_pAutoSaver->SetFilenameTemplate(CMainFrame::GetPrivateProfileCString("AutoSave", "FileNameTemplate", "", iniFile));


	// Default mod type when using the "New" button
	const MODTYPE oldDefault = defaultModType;
	defaultModType = CModSpecifications::ExtensionToType(CMainFrame::GetPrivateProfileCString("Misc", "DefaultModType", CSoundFile::GetModSpecifications(defaultModType).fileExtension, iniFile));
	if(defaultModType == MOD_TYPE_NONE)
	{
		defaultModType = oldDefault;
	}
}


#define SETTINGS_REGKEY_BASE		"Software\\Olivier Lapicque\\"
#define SETTINGS_REGKEY_DEFAULT		"ModPlug Tracker"
#define SETTINGS_REGEXT_WINDOW		"\\Window"
#define SETTINGS_REGEXT_SETTINGS	"\\Settings"

void TrackerSettings::LoadRegistryEQ(HKEY key, LPCSTR pszName, EQPreset *pEqSettings)
//-----------------------------------------------------------------------------------
{
	DWORD dwType = REG_BINARY;
	DWORD dwSize = sizeof(EQPreset);
	RegQueryValueEx(key, pszName, NULL, &dwType, (LPBYTE)pEqSettings, &dwSize);
	for (UINT i=0; i<MAX_EQ_BANDS; i++)
	{
		if (pEqSettings->Gains[i] > 32) pEqSettings->Gains[i] = 16;
		if ((pEqSettings->Freqs[i] < 100) || (pEqSettings->Freqs[i] > 10000)) pEqSettings->Freqs[i] = CEQSetupDlg::gEQPresets[0].Freqs[i];
	}
	StringFixer::SetNullTerminator(pEqSettings->szName);
}


bool TrackerSettings::LoadRegistrySettings()
//------------------------------------------
{
	CString m_csRegKey;
	CString m_csRegExt;
	CString m_csRegSettings;
	CString m_csRegWindow;
	m_csRegKey.Format("%s%s", SETTINGS_REGKEY_BASE, SETTINGS_REGKEY_DEFAULT);
	m_csRegSettings.Format("%s%s", m_csRegKey, SETTINGS_REGEXT_SETTINGS);
	m_csRegWindow.Format("%s%s", m_csRegKey, SETTINGS_REGEXT_WINDOW);


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
		for (int ncol = 0; ncol < MAX_MODCOLORS; ncol++)
		{
			CHAR s[16];
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
		if ((m_nBufferLength < SNDDEV_MINBUFFERLEN) || (m_nBufferLength > SNDDEV_MAXBUFFERLEN)) m_nBufferLength = 100;
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
		if((m_dwMidiSetup & 0x40) != 0)
		{
			// This flag used to be "amplify MIDI Note Velocity" - with a fixed amplification factor of 2.
			midiVelocityAmp = 200;
			m_dwMidiSetup &= ~0x40;
		}
		RegQueryValueEx(key, "MidiDevice", NULL, &dwREG_DWORD, (LPBYTE)&m_nMidiDevice, &dwDWORDSize);
		RegQueryValueEx(key, "PatternSetup", NULL, &dwREG_DWORD, (LPBYTE)&m_dwPatternSetup, &dwDWORDSize);
		m_dwPatternSetup &= ~(0x800|0x200000|0x400000);	// various deprecated old options
		m_dwPatternSetup |= PATTERN_NOTEFADE; // Set flag to maintain old behaviour (was changed in 1.17.02.50).
		m_dwPatternSetup |= PATTERN_RESETCHANNELS; // Set flag to reset channels on loop was changed in 1.17.03.01).
		RegQueryValueEx(key, "RowSpacing", NULL, &dwREG_DWORD, (LPBYTE)&m_nRowHighlightMeasures, &dwDWORDSize);
		RegQueryValueEx(key, "RowSpacing2", NULL, &dwREG_DWORD, (LPBYTE)&m_nRowHighlightBeats, &dwDWORDSize);
		RegQueryValueEx(key, "LoopSong", NULL, &dwREG_DWORD, (LPBYTE)&gbLoopSong, &dwDWORDSize);
		RegQueryValueEx(key, "BitsPerSample", NULL, &dwREG_DWORD, (LPBYTE)&m_nBitsPerSample, &dwDWORDSize);
		RegQueryValueEx(key, "ChannelMode", NULL, &dwREG_DWORD, (LPBYTE)&m_nChannels, &dwDWORDSize);
		RegQueryValueEx(key, "MidiImportSpeed", NULL, &dwREG_DWORD, (LPBYTE)&midiImportSpeed, &dwDWORDSize);
		RegQueryValueEx(key, "MidiImportPatLen", NULL, &dwREG_DWORD, (LPBYTE)&midiImportPatternLen, &dwDWORDSize);
		// EQ
		LoadRegistryEQ(key, "EQ_Settings", &m_EqSettings);
		LoadRegistryEQ(key, "EQ_User1", &CEQSetupDlg::gUserPresets[0]);
		LoadRegistryEQ(key, "EQ_User2", &CEQSetupDlg::gUserPresets[1]);
		LoadRegistryEQ(key, "EQ_User3", &CEQSetupDlg::gUserPresets[2]);
		LoadRegistryEQ(key, "EQ_User4", &CEQSetupDlg::gUserPresets[3]);

		//rewbs.resamplerConf
		dwDWORDSize = sizeof(m_MixerSettings.gbWFIRType);
		RegQueryValueEx(key, "XMMSModplugResamplerWFIRType", NULL, &dwREG_DWORD, (LPBYTE)&m_MixerSettings.gbWFIRType, &dwDWORDSize);
		dwDWORDSize = sizeof(m_MixerSettings.gdWFIRCutoff);
		RegQueryValueEx(key, "ResamplerWFIRCutoff", NULL, &dwREG_DWORD, (LPBYTE)&m_MixerSettings.gdWFIRCutoff, &dwDWORDSize);
		dwDWORDSize = sizeof(m_MixerSettings.glVolumeRampUpSamples);
		RegQueryValueEx(key, "VolumeRampSamples", NULL, &dwREG_DWORD, (LPBYTE)&m_MixerSettings.glVolumeRampUpSamples, &dwDWORDSize);
		m_MixerSettings.glVolumeRampDownSamples = m_MixerSettings.glVolumeRampUpSamples;

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
		gcsPreviousVersion = dwPreviousVersion;
		RegCloseKey(key);
	}
	CMainFrame::m_pAutoSaver = new CAutoSaver(asEnabled, asInterval, asBackupHistory, asUseOriginalPath, asPath, asFileNameTemplate);

	gnPatternSpacing = theApp.GetProfileInt("Pattern Editor", "Spacing", 0);
	gbPatternVUMeters = theApp.GetProfileInt("Pattern Editor", "VU-Meters", 0);
	gbPatternPluginNames = theApp.GetProfileInt("Pattern Editor", "Plugin-Names", 1);

	return true;
}


void TrackerSettings::SaveSettings()
//----------------------------------
{
	CString iniFile = theApp.GetConfigFileName();

	CString version = MptVersion::str;
	WritePrivateProfileString("Version", "Version", version, iniFile);
	WritePrivateProfileString("Version", "InstallGUID", gcsInstallGUID, iniFile);

	WINDOWPLACEMENT wpl;
	wpl.length = sizeof(WINDOWPLACEMENT);
	CMainFrame::GetMainFrame()->GetWindowPlacement(&wpl);
	WritePrivateProfileStruct("Display", "WindowPlacement", &wpl, sizeof(WINDOWPLACEMENT), iniFile);

	CMainFrame::WritePrivateProfileLong("Display", "MDIMaximize", gbMdiMaximize, iniFile);
	CMainFrame::WritePrivateProfileLong("Display", "MDITreeWidth", glTreeWindowWidth, iniFile);
	CMainFrame::WritePrivateProfileLong("Display", "MDITreeRatio", glTreeSplitRatio, iniFile);
	CMainFrame::WritePrivateProfileLong("Display", "MDIGeneralHeight", glGeneralWindowHeight, iniFile);
	CMainFrame::WritePrivateProfileLong("Display", "MDIPatternHeight", glPatternWindowHeight, iniFile);
	CMainFrame::WritePrivateProfileLong("Display", "MDISampleHeight", glSampleWindowHeight, iniFile);
	CMainFrame::WritePrivateProfileLong("Display", "MDIInstrumentHeight", glInstrumentWindowHeight, iniFile);
	CMainFrame::WritePrivateProfileLong("Display", "MDICommentsHeight", glCommentsWindowHeight, iniFile);
	CMainFrame::WritePrivateProfileLong("Display", "MDIGraphHeight", glGraphWindowHeight, iniFile); //rewbs.graph
	CMainFrame::WritePrivateProfileLong("Display", "PlugSelectWindowX", gnPlugWindowX, iniFile);
	CMainFrame::WritePrivateProfileLong("Display", "PlugSelectWindowY", gnPlugWindowY, iniFile);
	CMainFrame::WritePrivateProfileLong("Display", "PlugSelectWindowWidth", gnPlugWindowWidth, iniFile);
	CMainFrame::WritePrivateProfileLong("Display", "PlugSelectWindowHeight", gnPlugWindowHeight, iniFile);
	CMainFrame::WritePrivateProfileLong("Display", "PlugSelectWindowLast", gnPlugWindowLast, iniFile);
	CMainFrame::WritePrivateProfileDWord("Display", "MsgBoxVisibilityFlags", gnMsgBoxVisiblityFlags, iniFile);

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
		CMainFrame::WritePrivateProfileLong("Update", "UpdateCheckPeriod", CUpdateCheck::GetUpdateCheckPeriod(), iniFile);
		WritePrivateProfileString("Update", "UpdateURL", CUpdateCheck::GetUpdateURL(), iniFile);
		CMainFrame::WritePrivateProfileLong("Update", "SendGUID", CUpdateCheck::GetSendGUID() ? 1 : 0, iniFile);
		CMainFrame::WritePrivateProfileLong("Update", "ShowUpdateHint", CUpdateCheck::GetShowUpdateHint() ? 1 : 0, iniFile);
	}

	CHAR s[16];
	for (int ncol = 0; ncol < MAX_MODCOLORS; ncol++)
	{
		wsprintf(s, "Color%02d", ncol);
		CMainFrame::WritePrivateProfileDWord("Display", s, rgbCustomColors[ncol], iniFile);
	}

	CMainFrame::WritePrivateProfileLong("Sound Settings", "WaveDevice", m_nWaveDevice, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "SoundSetup", m_dwSoundSetup, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "Quality", m_dwQuality, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "SrcMode", m_nSrcMode, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "Mixing_Rate", m_dwRate, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "BitsPerSample", m_nBitsPerSample, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "ChannelMode", m_nChannels, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "BufferLength", m_nBufferLength, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "PreAmp", m_nPreAmp, iniFile);
	CMainFrame::WritePrivateProfileLong("Sound Settings", "StereoSeparation", CSoundFile::m_nStereoSeparation, iniFile);
	CMainFrame::WritePrivateProfileLong("Sound Settings", "MixChannels", CSoundFile::m_nMaxMixChannels, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "XMMSModplugResamplerWFIRType", m_MixerSettings.gbWFIRType, iniFile);
	CMainFrame::WritePrivateProfileLong("Sound Settings", "ResamplerWFIRCutoff", static_cast<int>(m_MixerSettings.gdWFIRCutoff*100+0.5), iniFile);
	WritePrivateProfileString("Sound Settings", "VolumeRampSamples", NULL, iniFile);	// deprecated
	CMainFrame::WritePrivateProfileLong("Sound Settings", "VolumeRampUpSamples", m_MixerSettings.glVolumeRampUpSamples, iniFile);
	CMainFrame::WritePrivateProfileLong("Sound Settings", "VolumeRampDownSamples", m_MixerSettings.glVolumeRampDownSamples, iniFile);

	CMainFrame::WritePrivateProfileDWord("MIDI Settings", "MidiSetup", m_dwMidiSetup, iniFile);
	CMainFrame::WritePrivateProfileDWord("MIDI Settings", "MidiDevice", m_nMidiDevice, iniFile);
	CMainFrame::WritePrivateProfileDWord("MIDI Settings", "AftertouchBehaviour", aftertouchBehaviour, iniFile);
	CMainFrame::WritePrivateProfileLong("MIDI Settings", "MidiVelocityAmp", midiVelocityAmp, iniFile);
	CMainFrame::WritePrivateProfileLong("MIDI Settings", "MidiImportSpeed", midiImportSpeed, iniFile);
	CMainFrame::WritePrivateProfileLong("MIDI Settings", "MidiImportPatLen", midiImportPatternLen, iniFile);

	CMainFrame::WritePrivateProfileDWord("Pattern Editor", "PatternSetup", m_dwPatternSetup, iniFile);
	CMainFrame::WritePrivateProfileDWord("Pattern Editor", "RowSpacing", m_nRowHighlightMeasures, iniFile);
	CMainFrame::WritePrivateProfileDWord("Pattern Editor", "RowSpacing2", m_nRowHighlightBeats, iniFile);
	CMainFrame::WritePrivateProfileDWord("Pattern Editor", "LoopSong", gbLoopSong, iniFile);
	CMainFrame::WritePrivateProfileDWord("Pattern Editor", "Spacing", gnPatternSpacing, iniFile);
	CMainFrame::WritePrivateProfileDWord("Pattern Editor", "VU-Meters", gbPatternVUMeters, iniFile);
	CMainFrame::WritePrivateProfileDWord("Pattern Editor", "Plugin-Names", gbPatternPluginNames, iniFile);
	CMainFrame::WritePrivateProfileDWord("Pattern Editor", "Record", gbPatternRecord, iniFile);
	CMainFrame::WritePrivateProfileDWord("Pattern Editor", "AutoChordWaitTime", gnAutoChordWaitTime, iniFile);
	CMainFrame::WritePrivateProfileDWord("Pattern Editor", "RecordQuantize", recordQuantizeRows, iniFile);

	CMainFrame::WritePrivateProfileDWord("Pattern Editor", "NumClipboards", PatternClipboard::GetClipboardSize(), iniFile);

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
			theApp.AbsolutePathToRelative(szPath);
		}
		WritePrivateProfileString("Paths", m_szDirectoryToSettingsName[i], szPath, iniFile);

	}
	// Obsolete, since we always write to Keybindings.mkb now.
	// Older versions of OpenMPT 1.18+ will look for this file if this entry is missing, so removing this entry after having read it is kind of backwards compatible.
	WritePrivateProfileString("Paths", "Key_Config_File", nullptr, iniFile);

	CMainFrame::WritePrivateProfileLong("Effects", "XBassDepth", CSoundFile::m_nXBassDepth, iniFile);
	CMainFrame::WritePrivateProfileLong("Effects", "XBassRange", CSoundFile::m_nXBassRange, iniFile);
	CMainFrame::WritePrivateProfileLong("Effects", "ReverbDepth", CSoundFile::m_nReverbDepth, iniFile);
	CMainFrame::WritePrivateProfileLong("Effects", "ReverbType", CSoundFile::gnReverbType, iniFile);
	CMainFrame::WritePrivateProfileLong("Effects", "ProLogicDepth", CSoundFile::m_nProLogicDepth, iniFile);
	CMainFrame::WritePrivateProfileLong("Effects", "ProLogicDelay", CSoundFile::m_nProLogicDelay, iniFile);

	WritePrivateProfileStruct("Effects", "EQ_Settings", &m_EqSettings, sizeof(EQPreset), iniFile);
	WritePrivateProfileStruct("Effects", "EQ_User1", &CEQSetupDlg::gUserPresets[0], sizeof(EQPreset), iniFile);
	WritePrivateProfileStruct("Effects", "EQ_User2", &CEQSetupDlg::gUserPresets[1], sizeof(EQPreset), iniFile);
	WritePrivateProfileStruct("Effects", "EQ_User3", &CEQSetupDlg::gUserPresets[2], sizeof(EQPreset), iniFile);
	WritePrivateProfileStruct("Effects", "EQ_User4", &CEQSetupDlg::gUserPresets[3], sizeof(EQPreset), iniFile);

	if(CMainFrame::m_pAutoSaver != nullptr)
	{
		CMainFrame::WritePrivateProfileLong("AutoSave", "Enabled", CMainFrame::m_pAutoSaver->IsEnabled(), iniFile);
		CMainFrame::WritePrivateProfileLong("AutoSave", "IntervalMinutes", CMainFrame::m_pAutoSaver->GetSaveInterval(), iniFile);
		CMainFrame::WritePrivateProfileLong("AutoSave", "BackupHistory", CMainFrame::m_pAutoSaver->GetHistoryDepth(), iniFile);
		CMainFrame::WritePrivateProfileLong("AutoSave", "UseOriginalPath", CMainFrame::m_pAutoSaver->GetUseOriginalPath(), iniFile);
		_tcscpy(szPath, CMainFrame::m_pAutoSaver->GetPath());
		if(bConvertPaths)
		{
			theApp.AbsolutePathToRelative(szPath);
		}
		WritePrivateProfileString("AutoSave", "Path", szPath, iniFile);
		WritePrivateProfileString("AutoSave", "FileNameTemplate", CMainFrame::m_pAutoSaver->GetFilenameTemplate(), iniFile);
	}

	SaveChords(Chords);

	// Save default macro configuration
	MIDIMacroConfig macros;
	theApp.GetDefaultMidiMacro(&macros);
	for(int isfx = 0; isfx < 16; isfx++)
	{
		CHAR snam[8];
		wsprintf(snam, "SF%X", isfx);
		WritePrivateProfileString("Zxx Macros", snam, macros.szMidiSFXExt[isfx], iniFile);
	}
	for(int izxx = 0; izxx < 128; izxx++)
	{
		CHAR snam[8];
		wsprintf(snam, "Z%02X", izxx | 0x80);
		if (!WritePrivateProfileString("Zxx Macros", snam, macros.szMidiZXXExt[izxx], iniFile)) break;
	}

	WritePrivateProfileString("Misc", "DefaultModType", CSoundFile::GetModSpecifications(defaultModType).fileExtension, iniFile);

	CMainFrame::GetMainFrame()->SaveBarState("Toolbars");
}


////////////////////////////////////////////////////////////////////////////////
// Chords

void TrackerSettings::LoadChords(MPTChords &chords)
//-------------------------------------------------
{	
	for(size_t i = 0; i < CountOf(chords); i++)
	{
		uint32 chord;
		if((chord = GetPrivateProfileInt("Chords", szDefaultNoteNames[i], -1, theApp.GetConfigFileName())) >= 0)
		{
			if((chord & 0xFFFFFFC0) || (!chords[i].notes[0]))
			{
				chords[i].key = (uint8)(chord & 0x3F);
				chords[i].notes[0] = (uint8)((chord >> 6) & 0x3F);
				chords[i].notes[1] = (uint8)((chord >> 12) & 0x3F);
				chords[i].notes[2] = (uint8)((chord >> 18) & 0x3F);
			}
		}
	}
}


void TrackerSettings::SaveChords(MPTChords &chords)
//-------------------------------------------------
{
	CHAR s[64];
	for(size_t i = 0; i < CountOf(chords); i++)
	{
		wsprintf(s, "%d", (chords[i].key) | (chords[i].notes[0] << 6) | (chords[i].notes[1] << 12) | (chords[i].notes[2] << 18));
		if (!WritePrivateProfileString("Chords", szDefaultNoteNames[i], s, theApp.GetConfigFileName())) break;
	}
}


// retrieve / set default directory from given string and store it our setup variables
void TrackerSettings::SetDirectory(const LPCTSTR szFilenameFrom, Directory dir, TCHAR (&directories)[NUM_DIRS][_MAX_PATH], bool bStripFilename)
//---------------------------------------------------------------------------------------------------------------------------------------------
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

	TCHAR szOldDir[CountOf(directories[dir])]; // for comparison
	_tcscpy(szOldDir, directories[dir]);

	_tcscpy(directories[dir], szPath);

	// When updating default directory, also update the working directory.
	if(szPath[0] && directories == m_szDefaultDirectory)
	{
		if(_tcscmp(szOldDir, szPath) != 0) // update only if default directory has changed
			SetWorkingDirectory(szPath, dir);
	}
}

void TrackerSettings::SetDefaultDirectory(const LPCTSTR szFilenameFrom, Directory dir, bool bStripFilename)
//---------------------------------------------------------------------------------------------------------
{
	SetDirectory(szFilenameFrom, dir, m_szDefaultDirectory, bStripFilename);
}


void TrackerSettings::SetWorkingDirectory(const LPCTSTR szFilenameFrom, Directory dir, bool bStripFilename)
//---------------------------------------------------------------------------------------------------------
{
	SetDirectory(szFilenameFrom, dir, m_szWorkingDirectory, bStripFilename);
}


LPCTSTR TrackerSettings::GetDefaultDirectory(Directory dir) const
//---------------------------------------------------------------
{
	return m_szDefaultDirectory[dir];
}


LPCTSTR TrackerSettings::GetWorkingDirectory(Directory dir) const
//---------------------------------------------------------------
{
	return m_szWorkingDirectory[dir];
}