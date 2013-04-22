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
#include "../sounddev/SoundDevice.h"
#include "../sounddev/SoundDevices.h"
#include "../common/version.h"
#include "UpdateCheck.h"
#include "Mpdlgs.h"
#include "AutoSaver.h"
#include "../common/StringFixer.h"
#include "TrackerSettings.h"
#include "../common/misc_util.h"
#include "PatternClipboard.h"
#include "../sounddev/SoundDevice.h"


#define OLD_SNDDEV_MINBUFFERLEN			1    // 1ms
#define OLD_SNDDEV_MAXBUFFERLEN			1000 // 1sec

#define OLD_SOUNDSETUP_REVERSESTEREO         0x20
#define OLD_SOUNDSETUP_SECONDARY             0x40
#define OLD_SOUNDSETUP_NOBOOSTTHREADPRIORITY 0x80


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
	VuMeterUpdateInterval = 15;

	gcsPreviousVersion = 0;
	gcsInstallGUID = "";
	// Audio Setup
	gnAutoChordWaitTime = 60;
	gnMsgBoxVisiblityFlags = uint32_max;

	// Audio device
	gbLoopSong = TRUE;
	m_nWaveDevice = SNDDEV_BUILD_ID(0, SNDDEV_WAVEOUT);	// Default value will be overridden
	m_LatencyMS = SNDDEV_DEFAULT_LATENCY_MS;
	m_UpdateIntervalMS = SNDDEV_DEFAULT_UPDATEINTERVAL_MS;
	m_SoundDeviceExclusiveMode = false;
	m_SoundDeviceBoostThreadPriority = true;

#ifndef NO_EQ
	// Default EQ settings
	MemCopy(m_EqSettings, CEQSetupDlg::gEQPresets[0]);
#endif

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

	// Sample Editor
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

	DefaultPlugVolumeHandling = PLUGIN_VOLUMEHANDLING_IGNORE;

	gnPlugWindowX = 243;
	gnPlugWindowY = 273;
	gnPlugWindowWidth = 370;
	gnPlugWindowHeight = 332;
	gnPlugWindowLast = 0;

}


DWORD TrackerSettings::GetSoundDeviceFlags() const
//------------------------------------------------
{
	return (m_SoundDeviceExclusiveMode ? SNDDEV_OPTIONS_EXCLUSIVE : 0) | (m_SoundDeviceBoostThreadPriority ? SNDDEV_OPTIONS_BOOSTTHREADPRIORITY : 0);
}


void TrackerSettings::SetSoundDeviceFlags(DWORD flags)
//----------------------------------------------------
{
	m_SoundDeviceExclusiveMode = (flags & SNDDEV_OPTIONS_EXCLUSIVE) ? true : false;
	m_SoundDeviceBoostThreadPriority = (flags & SNDDEV_OPTIONS_BOOSTTHREADPRIORITY) ? true : false;
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
	theApp.GetDefaultMidiMacro(macros);
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
	theApp.SetDefaultMidiMacro(macros);

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
	m_ShowSplashScreen = CMainFrame::GetPrivateProfileBool("Display", "ShowSplashScreen", m_ShowSplashScreen, iniFile);
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
	VuMeterUpdateInterval = CMainFrame::GetPrivateProfileDWord("Display", "VuMeterUpdateInterval", VuMeterUpdateInterval, iniFile);

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
	if(vIniVersion < MAKE_VERSION_NUMERIC(1, 22, 01, 03)) m_MixerSettings.MixerFlags |= OLD_SOUNDSETUP_SECONDARY;
	m_MixerSettings.MixerFlags = CMainFrame::GetPrivateProfileDWord("Sound Settings", "SoundSetup", m_MixerSettings.MixerFlags, iniFile);
	m_SoundDeviceExclusiveMode = CMainFrame::GetPrivateProfileBool("Sound Settings", "ExclusiveMode", m_SoundDeviceExclusiveMode, iniFile);
	m_SoundDeviceBoostThreadPriority = CMainFrame::GetPrivateProfileBool("Sound Settings", "BoostThreadPriority", m_SoundDeviceBoostThreadPriority, iniFile);
	if(vIniVersion < MAKE_VERSION_NUMERIC(1, 21, 01, 26)) m_MixerSettings.MixerFlags &= ~OLD_SOUNDSETUP_REVERSESTEREO;
	if(vIniVersion < MAKE_VERSION_NUMERIC(1, 22, 01, 03))
	{
		m_SoundDeviceExclusiveMode = ((m_MixerSettings.MixerFlags & OLD_SOUNDSETUP_SECONDARY) == 0);
		m_SoundDeviceBoostThreadPriority = ((m_MixerSettings.MixerFlags & OLD_SOUNDSETUP_NOBOOSTTHREADPRIORITY) == 0);
		m_MixerSettings.MixerFlags &= ~OLD_SOUNDSETUP_SECONDARY;
		m_MixerSettings.MixerFlags &= ~OLD_SOUNDSETUP_NOBOOSTTHREADPRIORITY;
	}
	m_MixerSettings.DSPMask = CMainFrame::GetPrivateProfileDWord("Sound Settings", "Quality", m_MixerSettings.DSPMask, iniFile);
	m_ResamplerSettings.SrcMode = (ResamplingMode)CMainFrame::GetPrivateProfileDWord("Sound Settings", "SrcMode", m_ResamplerSettings.SrcMode, iniFile);
	m_MixerSettings.gdwMixingFreq = CMainFrame::GetPrivateProfileDWord("Sound Settings", "Mixing_Rate", 0, iniFile);
	m_MixerSettings.m_SampleFormat = (SampleFormat)CMainFrame::GetPrivateProfileDWord("Sound Settings", "BitsPerSample", (DWORD)m_MixerSettings.m_SampleFormat, iniFile);
	m_MixerSettings.gnChannels = CMainFrame::GetPrivateProfileDWord("Sound Settings", "ChannelMode", m_MixerSettings.gnChannels, iniFile);
	DWORD LatencyMS = CMainFrame::GetPrivateProfileDWord("Sound Settings", "Latency", 0, iniFile);
	DWORD UpdateIntervalMS = CMainFrame::GetPrivateProfileDWord("Sound Settings", "UpdateInterval", 0, iniFile);
	if(LatencyMS == 0 || UpdateIntervalMS == 0)
	{
		// old versions have set BufferLength which meant different things than the current ISoundDevice interface wants to know
		DWORD BufferLengthMS = CMainFrame::GetPrivateProfileDWord("Sound Settings", "BufferLength", 0, iniFile);
		if(BufferLengthMS != 0)
		{
			if(BufferLengthMS < OLD_SNDDEV_MINBUFFERLEN) BufferLengthMS = OLD_SNDDEV_MINBUFFERLEN;
			if(BufferLengthMS > OLD_SNDDEV_MAXBUFFERLEN) BufferLengthMS = OLD_SNDDEV_MAXBUFFERLEN;
			if(SNDDEV_GET_TYPE(m_nWaveDevice) == SNDDEV_ASIO)
			{
				m_LatencyMS = BufferLengthMS;
				m_UpdateIntervalMS = BufferLengthMS / 8;
			} else
			{
				m_LatencyMS = BufferLengthMS * 3;
				m_UpdateIntervalMS = BufferLengthMS / 8;
			}
		} else
		{
			// use defaults
			m_LatencyMS = SNDDEV_DEFAULT_LATENCY_MS;
			m_UpdateIntervalMS = SNDDEV_DEFAULT_UPDATEINTERVAL_MS;
		}
	} else
	{
		m_LatencyMS = LatencyMS;
		m_UpdateIntervalMS = UpdateIntervalMS;
	}
	if(m_MixerSettings.gdwMixingFreq == 0)
	{
		m_MixerSettings.gdwMixingFreq = 44100;
#ifndef NO_ASIO
		// If no mixing rate is specified and we're using ASIO, get a mixing rate supported by the device.
		if(SNDDEV_GET_TYPE(m_nWaveDevice) == SNDDEV_ASIO)
		{
			ISoundDevice *dummy = CreateSoundDevice(SNDDEV_ASIO);
			if(dummy)
			{
				m_MixerSettings.gdwMixingFreq = dummy->GetCurrentSampleRate(SNDDEV_GET_NUMBER(m_nWaveDevice));
				delete dummy;
			}
		}
#endif // NO_ASIO
	}

	m_MixerSettings.m_nPreAmp = CMainFrame::GetPrivateProfileDWord("Sound Settings", "PreAmp", m_MixerSettings.m_nPreAmp, iniFile);
	m_MixerSettings.m_nStereoSeparation = CMainFrame::GetPrivateProfileLong("Sound Settings", "StereoSeparation", m_MixerSettings.m_nStereoSeparation, iniFile);
	m_MixerSettings.m_nMaxMixChannels = CMainFrame::GetPrivateProfileLong("Sound Settings", "MixChannels", m_MixerSettings.m_nMaxMixChannels, iniFile);
	m_ResamplerSettings.gbWFIRType = static_cast<BYTE>(CMainFrame::GetPrivateProfileDWord("Sound Settings", "XMMSModplugResamplerWFIRType", m_ResamplerSettings.gbWFIRType, iniFile));
	//gdWFIRCutoff = static_cast<double>(CMainFrame::GetPrivateProfileLong("Sound Settings", "ResamplerWFIRCutoff", gdWFIRCutoff * 100.0, iniFile)) / 100.0;
	m_ResamplerSettings.gdWFIRCutoff = static_cast<double>(CMainFrame::GetPrivateProfileLong("Sound Settings", "ResamplerWFIRCutoff", Util::Round<long>(m_ResamplerSettings.gdWFIRCutoff * 100.0), iniFile)) / 100.0;
	
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
	DefaultPlugVolumeHandling = static_cast<uint8>(GetPrivateProfileInt("Misc", "DefaultPlugVolumeHandling", DefaultPlugVolumeHandling, iniFile));
	if(DefaultPlugVolumeHandling >= PLUGIN_VOLUMEHANDLING_MAX) DefaultPlugVolumeHandling = PLUGIN_VOLUMEHANDLING_IGNORE;

	m_nSampleUndoMaxBuffer = CMainFrame::GetPrivateProfileLong("Sample Editor" , "UndoBufferSize", m_nSampleUndoMaxBuffer >> 20, iniFile);
	m_nSampleUndoMaxBuffer = MAX(1, m_nSampleUndoMaxBuffer) << 20;

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
#ifndef NO_DSP
	m_DSPSettings.m_nXBassDepth = CMainFrame::GetPrivateProfileLong("Effects", "XBassDepth", m_DSPSettings.m_nXBassDepth, iniFile);
	m_DSPSettings.m_nXBassRange = CMainFrame::GetPrivateProfileLong("Effects", "XBassRange", m_DSPSettings.m_nXBassRange, iniFile);
#endif
#ifndef NO_REVERB
	m_ReverbSettings.m_nReverbDepth = CMainFrame::GetPrivateProfileLong("Effects", "ReverbDepth", m_ReverbSettings.m_nReverbDepth, iniFile);
	m_ReverbSettings.m_nReverbType = CMainFrame::GetPrivateProfileLong("Effects", "ReverbType", m_ReverbSettings.m_nReverbType, iniFile);
#endif
#ifndef NO_DSP
	m_DSPSettings.m_nProLogicDepth = CMainFrame::GetPrivateProfileLong("Effects", "ProLogicDepth", m_DSPSettings.m_nProLogicDepth, iniFile);
	m_DSPSettings.m_nProLogicDelay = CMainFrame::GetPrivateProfileLong("Effects", "ProLogicDelay", m_DSPSettings.m_nProLogicDelay, iniFile);
#endif


#ifndef NO_EQ
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
#endif


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
	defaultModType = CModSpecifications::ExtensionToType(mpt::String(CMainFrame::GetPrivateProfileCString("Misc", "DefaultModType", CSoundFile::GetModSpecifications(defaultModType).fileExtension, iniFile)));
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
		RegQueryValueEx(key, "SoundSetup", NULL, &dwREG_DWORD, (LPBYTE)&m_MixerSettings.MixerFlags, &dwDWORDSize);
		m_MixerSettings.MixerFlags &= ~OLD_SOUNDSETUP_REVERSESTEREO;
		m_SoundDeviceExclusiveMode = ((m_MixerSettings.MixerFlags & OLD_SOUNDSETUP_SECONDARY) == 0);
		m_MixerSettings.MixerFlags &= ~OLD_SOUNDSETUP_SECONDARY;
		RegQueryValueEx(key, "Quality", NULL, &dwREG_DWORD, (LPBYTE)&m_MixerSettings.DSPMask, &dwDWORDSize);
		DWORD dummysrcmode = m_ResamplerSettings.SrcMode;
		RegQueryValueEx(key, "SrcMode", NULL, &dwREG_DWORD, (LPBYTE)&dummysrcmode, &dwDWORDSize);
		m_ResamplerSettings.SrcMode = (ResamplingMode)dummysrcmode;
		RegQueryValueEx(key, "Mixing_Rate", NULL, &dwREG_DWORD, (LPBYTE)&m_MixerSettings.gdwMixingFreq, &dwDWORDSize);
		DWORD BufferLengthMS = 0;
		RegQueryValueEx(key, "BufferLength", NULL, &dwREG_DWORD, (LPBYTE)&BufferLengthMS, &dwDWORDSize);
		if(BufferLengthMS != 0)
		{
			if((BufferLengthMS < OLD_SNDDEV_MINBUFFERLEN) || (BufferLengthMS > OLD_SNDDEV_MAXBUFFERLEN)) BufferLengthMS = 100;
			if(SNDDEV_GET_TYPE(m_nWaveDevice) == SNDDEV_ASIO)
			{
				m_LatencyMS = BufferLengthMS;
				m_UpdateIntervalMS = BufferLengthMS / 8;
			} else
			{
				m_LatencyMS = BufferLengthMS * 3;
				m_UpdateIntervalMS = BufferLengthMS / 8;
			}
		}
		RegQueryValueEx(key, "PreAmp", NULL, &dwREG_DWORD, (LPBYTE)&m_MixerSettings.m_nPreAmp, &dwDWORDSize);

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

#ifndef NO_DSP
		RegQueryValueEx(key, "XBassDepth", NULL, &dwREG_DWORD, (LPBYTE)&m_DSPSettings.m_nXBassDepth, &dwDWORDSize);
		RegQueryValueEx(key, "XBassRange", NULL, &dwREG_DWORD, (LPBYTE)&m_DSPSettings.m_nXBassRange, &dwDWORDSize);
#endif
#ifndef NO_REVERB
		RegQueryValueEx(key, "ReverbDepth", NULL, &dwREG_DWORD, (LPBYTE)&m_ReverbSettings.m_nReverbDepth, &dwDWORDSize);
		RegQueryValueEx(key, "ReverbType", NULL, &dwREG_DWORD, (LPBYTE)&m_ReverbSettings.m_nReverbType, &dwDWORDSize);
#endif NO_REVERB
#ifndef NO_DSP
		RegQueryValueEx(key, "ProLogicDepth", NULL, &dwREG_DWORD, (LPBYTE)&m_DSPSettings.m_nProLogicDepth, &dwDWORDSize);
		RegQueryValueEx(key, "ProLogicDelay", NULL, &dwREG_DWORD, (LPBYTE)&m_DSPSettings.m_nProLogicDelay, &dwDWORDSize);
#endif
		RegQueryValueEx(key, "StereoSeparation", NULL, &dwREG_DWORD, (LPBYTE)&m_MixerSettings.m_nStereoSeparation, &dwDWORDSize);
		RegQueryValueEx(key, "MixChannels", NULL, &dwREG_DWORD, (LPBYTE)&m_MixerSettings.m_nMaxMixChannels, &dwDWORDSize);
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
		DWORD dummy_sampleformat = (DWORD)m_MixerSettings.m_SampleFormat;
		RegQueryValueEx(key, "BitsPerSample", NULL, &dwREG_DWORD, (LPBYTE)&dummy_sampleformat, &dwDWORDSize);
		m_MixerSettings.m_SampleFormat = (SampleFormat)dummy_sampleformat;
		RegQueryValueEx(key, "ChannelMode", NULL, &dwREG_DWORD, (LPBYTE)&m_MixerSettings.gnChannels, &dwDWORDSize);
		RegQueryValueEx(key, "MidiImportSpeed", NULL, &dwREG_DWORD, (LPBYTE)&midiImportSpeed, &dwDWORDSize);
		RegQueryValueEx(key, "MidiImportPatLen", NULL, &dwREG_DWORD, (LPBYTE)&midiImportPatternLen, &dwDWORDSize);
#ifndef NO_EQ
		// EQ
		LoadRegistryEQ(key, "EQ_Settings", &m_EqSettings);
		LoadRegistryEQ(key, "EQ_User1", &CEQSetupDlg::gUserPresets[0]);
		LoadRegistryEQ(key, "EQ_User2", &CEQSetupDlg::gUserPresets[1]);
		LoadRegistryEQ(key, "EQ_User3", &CEQSetupDlg::gUserPresets[2]);
		LoadRegistryEQ(key, "EQ_User4", &CEQSetupDlg::gUserPresets[3]);
#endif

		//rewbs.resamplerConf
		dwDWORDSize = sizeof(m_ResamplerSettings.gbWFIRType);
		RegQueryValueEx(key, "XMMSModplugResamplerWFIRType", NULL, &dwREG_DWORD, (LPBYTE)&m_ResamplerSettings.gbWFIRType, &dwDWORDSize);
		dwDWORDSize = sizeof(m_ResamplerSettings.gdWFIRCutoff);
		RegQueryValueEx(key, "ResamplerWFIRCutoff", NULL, &dwREG_DWORD, (LPBYTE)&m_ResamplerSettings.gdWFIRCutoff, &dwDWORDSize);
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
	CMainFrame::WritePrivateProfileDWord("Display", "VuMeterUpdateInterval", VuMeterUpdateInterval, iniFile);
	
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
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "SoundSetup", m_MixerSettings.MixerFlags, iniFile);
	CMainFrame::WritePrivateProfileBool("Sound Settings", "ExclusiveMode", m_SoundDeviceExclusiveMode, iniFile);
	CMainFrame::WritePrivateProfileBool("Sound Settings", "BoostThreadPriority", m_SoundDeviceBoostThreadPriority, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "Quality", m_MixerSettings.DSPMask, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "SrcMode", m_ResamplerSettings.SrcMode, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "Mixing_Rate", m_MixerSettings.gdwMixingFreq, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "BitsPerSample", (DWORD)m_MixerSettings.m_SampleFormat, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "ChannelMode", m_MixerSettings.gnChannels, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "Latency", m_LatencyMS, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "UpdateInterval", m_UpdateIntervalMS, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "PreAmp", m_MixerSettings.m_nPreAmp, iniFile);
	CMainFrame::WritePrivateProfileLong("Sound Settings", "StereoSeparation", m_MixerSettings.m_nStereoSeparation, iniFile);
	CMainFrame::WritePrivateProfileLong("Sound Settings", "MixChannels", m_MixerSettings.m_nMaxMixChannels, iniFile);
	CMainFrame::WritePrivateProfileDWord("Sound Settings", "XMMSModplugResamplerWFIRType", m_ResamplerSettings.gbWFIRType, iniFile);
	CMainFrame::WritePrivateProfileLong("Sound Settings", "ResamplerWFIRCutoff", static_cast<int>(m_ResamplerSettings.gdWFIRCutoff*100+0.5), iniFile);
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

#ifndef NO_DSP
	CMainFrame::WritePrivateProfileLong("Effects", "XBassDepth", m_DSPSettings.m_nXBassDepth, iniFile);
	CMainFrame::WritePrivateProfileLong("Effects", "XBassRange", m_DSPSettings.m_nXBassRange, iniFile);
#endif
#ifndef NO_REVERB
	CMainFrame::WritePrivateProfileLong("Effects", "ReverbDepth", m_ReverbSettings.m_nReverbDepth, iniFile);
	CMainFrame::WritePrivateProfileLong("Effects", "ReverbType", m_ReverbSettings.m_nReverbType, iniFile);
#endif
#ifndef NO_DSP
	CMainFrame::WritePrivateProfileLong("Effects", "ProLogicDepth", m_DSPSettings.m_nProLogicDepth, iniFile);
	CMainFrame::WritePrivateProfileLong("Effects", "ProLogicDelay", m_DSPSettings.m_nProLogicDelay, iniFile);
#endif

#ifndef NO_EQ
	WritePrivateProfileStruct("Effects", "EQ_Settings", &m_EqSettings, sizeof(EQPreset), iniFile);
	WritePrivateProfileStruct("Effects", "EQ_User1", &CEQSetupDlg::gUserPresets[0], sizeof(EQPreset), iniFile);
	WritePrivateProfileStruct("Effects", "EQ_User2", &CEQSetupDlg::gUserPresets[1], sizeof(EQPreset), iniFile);
	WritePrivateProfileStruct("Effects", "EQ_User3", &CEQSetupDlg::gUserPresets[2], sizeof(EQPreset), iniFile);
	WritePrivateProfileStruct("Effects", "EQ_User4", &CEQSetupDlg::gUserPresets[3], sizeof(EQPreset), iniFile);
#endif

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
	theApp.GetDefaultMidiMacro(macros);
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