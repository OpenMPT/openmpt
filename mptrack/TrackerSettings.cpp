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
#include "../sounddev/SoundDeviceManager.h"
#include "../common/version.h"
#include "UpdateCheck.h"
#include "Mpdlgs.h"
#include "../common/StringFixer.h"
#include "TrackerSettings.h"
#include "../common/misc_util.h"
#include "PatternClipboard.h"
#include "../common/ComponentManager.h"
#include "ExceptionHandler.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/Tables.h"


#include <algorithm>


OPENMPT_NAMESPACE_BEGIN


#define OLD_SOUNDSETUP_REVERSESTEREO         0x20
#define OLD_SOUNDSETUP_SECONDARY             0x40
#define OLD_SOUNDSETUP_NOBOOSTTHREADPRIORITY 0x80


TrackerSettings &TrackerSettings::Instance()
//------------------------------------------
{
	return theApp.GetTrackerSettings();
}


static MptVersion::VersionNum GetPreviousSettingsVersion(const std::string &iniVersion)
//-------------------------------------------------------------------------------------
{
	MptVersion::VersionNum result = 0;
	if(!iniVersion.empty())
	{
		result = MptVersion::ToNum(iniVersion);
	} else
	{
		// No version stored.
		// This is the first run, thus set the previous version to our current
		// version which will avoid running all settings upgrade code.
		result = MptVersion::num;
	}
	return result;
}


std::string SettingsModTypeToString(MODTYPE modtype)
//--------------------------------------------------
{
	return CSoundFile::GetModSpecifications(modtype).fileExtension;
}

MODTYPE SettingsStringToModType(const std::string &str)
//-----------------------------------------------------
{
	return CModSpecifications::ExtensionToType(str);
}


static ResamplingMode GetDefaultResamplerMode()
//---------------------------------------------
{
	ResamplingMode result = CResamplerSettings().SrcMode;
#ifdef ENABLE_ASM
	// rough heuristic to select less cpu consuming defaults for old CPUs
	if(GetProcSupport() & PROCSUPPORT_SSE)
	{
		result = SRCMODE_POLYPHASE;
	} else if(GetProcSupport() & PROCSUPPORT_MMX)
	{
		result = SRCMODE_SPLINE;
	} else
	{
		result = SRCMODE_LINEAR;
	}
#else
	// just use a sane default
	result = CResamplerSettings().SrcMode;
#endif
	return result;
}


static uint32 GetDefaultPatternSetup()
//------------------------------------
{
	return PATTERN_PLAYNEWNOTE | PATTERN_EFFECTHILIGHT
		| PATTERN_CENTERROW | PATTERN_DRAGNDROPEDIT
		| PATTERN_FLATBUTTONS | PATTERN_NOEXTRALOUD | PATTERN_2NDHIGHLIGHT
		| PATTERN_STDHIGHLIGHT | PATTERN_SHOWPREVIOUS | PATTERN_CONTSCROLL
		| PATTERN_SYNCMUTE | PATTERN_AUTODELAY | PATTERN_NOTEFADE
		| PATTERN_SHOWDEFAULTVOLUME;
}


void SampleUndoBufferSize::CalculateSize()
//----------------------------------------
{
	if(sizePercent < 0)
		sizePercent = 0;
	// Don't use GlobalMemoryStatusEx here since we want a percentage of the memory that's actually *available* to OpenMPT, which is a max of 4GB in 32-bit mode.
	MEMORYSTATUS memStatus;
	memStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&memStatus);
	sizeByte = mpt::saturate_cast<size_t>(memStatus.dwTotalPhys * uint64(sizePercent) / 100);

	// Pretend there's at least one MiB of memory (haha)
	if(sizePercent != 0 && sizeByte < 1 * 1024 * 1024)
	{
		sizeByte = 1 * 1024 * 1024;
	}
}


DebugSettings::DebugSettings(SettingsContainer &conf)
//---------------------------------------------------
	: conf(conf)
	// Debug
#if !defined(NO_LOGGING) && !defined(MPT_LOG_IS_DISABLED)
	, DebugLogLevel(conf, "Debug", "LogLevel", static_cast<int>(mpt::log::GlobalLogLevel))
	, DebugLogFacilitySolo(conf, "Debug", "LogFacilitySolo", std::string())
	, DebugLogFacilityBlocked(conf, "Debug", "LogFacilityBlocked", std::string())
	, DebugLogFileEnable(conf, "Debug", "LogFileEnable", mpt::log::FileEnabled)
	, DebugLogDebuggerEnable(conf, "Debug", "LogDebuggerEnable", mpt::log::DebuggerEnabled)
	, DebugLogConsoleEnable(conf, "Debug", "LogConsoleEnable", mpt::log::ConsoleEnabled)
#endif
	, DebugTraceEnable(conf, "Debug", "TraceEnable", false)
	, DebugTraceSize(conf, "Debug", "TraceSize", 1000000)
	, DebugTraceAlwaysDump(conf, "Debug", "TraceAlwaysDump", false)
	, DebugStopSoundDeviceOnCrash(conf, "Debug", "StopSoundDeviceOnCrash", true)
	, DebugStopSoundDeviceBeforeDump(conf, "Debug", "StopSoundDeviceBeforeDump", false)
{

	// Duplicate state for debug stuff in order to avoid calling into settings framework from crash context.
	ExceptionHandler::stopSoundDeviceOnCrash = DebugStopSoundDeviceOnCrash;
	ExceptionHandler::stopSoundDeviceBeforeDump = DebugStopSoundDeviceBeforeDump;

		// enable debug features (as early as possible after reading the settings)
	#if !defined(NO_LOGGING) && !defined(MPT_LOG_IS_DISABLED)
		#if !defined(MPT_LOG_GLOBAL_LEVEL_STATIC)
			mpt::log::GlobalLogLevel = DebugLogLevel;
		#endif
		mpt::log::SetFacilities(DebugLogFacilitySolo, DebugLogFacilityBlocked);
		mpt::log::FileEnabled = DebugLogFileEnable;
		mpt::log::DebuggerEnabled = DebugLogDebuggerEnable;
		mpt::log::ConsoleEnabled = DebugLogConsoleEnable;
	#endif
	if(DebugTraceEnable)
	{
		mpt::log::Trace::Enable(DebugTraceSize);
	}

}


DebugSettings::~DebugSettings()
//-----------------------------
{
	if(DebugTraceAlwaysDump)
	{
		DebugTraceDump();
	}
}


TrackerSettings::TrackerSettings(SettingsContainer &conf)
//-------------------------------------------------------
	: conf(conf)
	// Version
	, IniVersion(conf, "Version", "Version", std::string())
	, FirstRun(IniVersion.Get() == std::string())
	, PreviousSettingsVersion(GetPreviousSettingsVersion(IniVersion))
	, gcsInstallGUID(conf, "Version", "InstallGUID", mpt::ustring())
	// Display
	, m_ShowSplashScreen(conf, "Display", "ShowSplashScreen", true)
	, gbMdiMaximize(conf, "Display", "MDIMaximize", true)
	, highResUI(conf, "Display", "HighResUI", false)
	, glTreeSplitRatio(conf, "Display", "MDITreeRatio", 128)
	, glTreeWindowWidth(conf, "Display", "MDITreeWidth", 160)
	, glGeneralWindowHeight(conf, "Display", "MDIGeneralHeight", 222)
	, glPatternWindowHeight(conf, "Display", "MDIPatternHeight", 152)
	, glSampleWindowHeight(conf, "Display", "MDISampleHeight", 190)
	, glInstrumentWindowHeight(conf, "Display", "MDIInstrumentHeight", 300)
	, glCommentsWindowHeight(conf, "Display", "MDICommentsHeight", 288)
	, glGraphWindowHeight(conf, "Display", "MDIGraphHeight", 288)
	, gnPlugWindowX(conf, "Display", "PlugSelectWindowX", 243)
	, gnPlugWindowY(conf, "Display", "PlugSelectWindowY", 273)
	, gnPlugWindowWidth(conf, "Display", "PlugSelectWindowWidth", 450)
	, gnPlugWindowHeight(conf, "Display", "PlugSelectWindowHeight", 540)
	, gnPlugWindowLast(conf, "Display", "PlugSelectWindowLast", 0)
	, gnMsgBoxVisiblityFlags(conf, "Display", "MsgBoxVisibilityFlags", uint32_max)
	, GUIUpdateInterval(conf, "Display", "GUIUpdateInterval", 0)
	, VuMeterUpdateInterval(conf, "Display", "VuMeterUpdateInterval", 15)
	, VuMeterDecaySpeedDecibelPerSecond(conf, "Display", "VuMeterDecaySpeedDecibelPerSecond", 88.0f)
	, accidentalFlats(conf, "Display", "AccidentalFlats", false)
	, rememberSongWindows(conf, "Display", "RememberSongWindows", true)
	, showDirsInSampleBrowser(conf, "Display", "ShowDirsInSampleBrowser", false)
	, commentsFont(conf, "Display", "Comments Font", FontSetting("Courier New", 120))
	// Misc
	, ShowSettingsOnNewVersion(conf, "Misc", "ShowSettingsOnNewVersion", true)
	, defaultModType(conf, "Misc", "DefaultModType", MOD_TYPE_IT)
	, defaultNewFileAction(conf, "Misc", "DefaultNewFileAction", nfDefaultFormat)
	, DefaultPlugVolumeHandling(conf, "Misc", "DefaultPlugVolumeHandling", PLUGIN_VOLUMEHANDLING_IGNORE)
	, autoApplySmoothFT2Ramping(conf, "Misc", "SmoothFT2Ramping", false)
	, MiscITCompressionStereo(conf, "Misc", "ITCompressionStereo", 4)
	, MiscITCompressionMono(conf, "Misc", "ITCompressionMono", 4)
	, MiscAllowMultipleCommandsPerKey(conf, "Misc", "AllowMultipleCommandsPerKey", false)
	, MiscDistinguishModifiers(conf, "Misc", "DistinguishModifiers", false)
	// Sound Settings
	, m_SoundShowDeprecatedDevices(conf, "Sound Settings", "ShowDeprecatedDevices", true)
	, m_SoundShowNotRecommendedDeviceWarning(conf, "Sound Settings", "ShowNotRecommendedDeviceWarning", true)
	, m_SoundSampleRates(conf, "Sound Settings", "SampleRates", GetDefaultSampleRates())
	, m_MorePortaudio(conf, "Sound Settings", "MorePortaudio", false)
	, m_SoundSettingsOpenDeviceAtStartup(conf, "Sound Settings", "OpenDeviceAtStartup", false)
	, m_SoundSettingsStopMode(conf, "Sound Settings", "StopMode", SoundDeviceStopModeClosed)
	, m_SoundDeviceSettingsUseOldDefaults(false)
	, m_SoundDeviceID_DEPRECATED(SoundDevice::Legacy::ID())
	, m_SoundDeviceDirectSoundOldDefaultIdentifier(false)
	, m_SoundDeviceIdentifier(conf, "Sound Settings", "Device", SoundDevice::Identifier())
	, m_SoundDevicePreferSameTypeIfDeviceUnavailable(conf, "Sound Settings", "PreferSameTypeIfDeviceUnavailable", false)
	, MixerMaxChannels(conf, "Sound Settings", "MixChannels", MixerSettings().m_nMaxMixChannels)
	, MixerDSPMask(conf, "Sound Settings", "Quality", MixerSettings().DSPMask)
	, MixerFlags(conf, "Sound Settings", "SoundSetup", MixerSettings().MixerFlags)
	, MixerSamplerate(conf, "Sound Settings", "Mixing_Rate", MixerSettings().gdwMixingFreq)
	, MixerOutputChannels(conf, "Sound Settings", "ChannelMode", MixerSettings().gnChannels)
	, MixerPreAmp(conf, "Sound Settings", "PreAmp", MixerSettings().m_nPreAmp)
	, MixerStereoSeparation(conf, "Sound Settings", "StereoSeparation", MixerSettings().m_nStereoSeparation)
	, MixerVolumeRampUpMicroseconds(conf, "Sound Settings", "VolumeRampUpMicroseconds", MixerSettings().GetVolumeRampUpMicroseconds())
	, MixerVolumeRampDownMicroseconds(conf, "Sound Settings", "VolumeRampDownMicroseconds", MixerSettings().GetVolumeRampDownMicroseconds())
	, ResamplerMode(conf, "Sound Settings", "SrcMode", GetDefaultResamplerMode())
	, ResamplerSubMode(conf, "Sound Settings", "XMMSModplugResamplerWFIRType", CResamplerSettings().gbWFIRType)
	, ResamplerCutoffPercent(conf, "Sound Settings", "ResamplerWFIRCutoff", Util::Round<int32>(CResamplerSettings().gdWFIRCutoff * 100.0))
	// MIDI Settings
	, m_nMidiDevice(conf, "MIDI Settings", "MidiDevice", 0)
	, midiDeviceName(conf, "MIDI Settings", "MidiDeviceName", "")
	, m_dwMidiSetup(conf, "MIDI Settings", "MidiSetup", MIDISETUP_RECORDVELOCITY | MIDISETUP_RECORDNOTEOFF | MIDISETUP_TRANSPOSEKEYBOARD | MIDISETUP_MIDITOPLUG)
	, aftertouchBehaviour(conf, "MIDI Settings", "AftertouchBehaviour", atDoNotRecord)
	, midiVelocityAmp(conf, "MIDI Settings", "MidiVelocityAmp", 100)
	, midiIgnoreCCs(conf, "MIDI Settings", "IgnoredCCs", std::bitset<128>())
	, midiImportSpeed(conf, "MIDI Settings", "MidiImportSpeed", 3)
	, midiImportPatternLen(conf, "MIDI Settings", "MidiImportPatLen", 128)
	// Pattern Editor
	, gbLoopSong(conf, "Pattern Editor", "LoopSong", true)
	, gnPatternSpacing(conf, "Pattern Editor", "Spacing", 0)
	, gbPatternVUMeters(conf, "Pattern Editor", "VU-Meters", false)
	, gbPatternPluginNames(conf, "Pattern Editor", "Plugin-Names", true)
	, gbPatternRecord(conf, "Pattern Editor", "Record", true)
	, patternNoEditPopup(conf, "Pattern Editor", "NoEditPopup", false)
	, patternStepCommands(conf, "Pattern Editor", "EditStepAppliesToCommands", false)
	, m_dwPatternSetup(conf, "Pattern Editor", "PatternSetup", GetDefaultPatternSetup())
	, m_nRowHighlightMeasures(conf, "Pattern Editor", "RowSpacing", 16)
	, m_nRowHighlightBeats(conf, "Pattern Editor", "RowSpacing2", 4)
	, recordQuantizeRows(conf, "Pattern Editor", "RecordQuantize", 0)
	, gnAutoChordWaitTime(conf, "Pattern Editor", "AutoChordWaitTime", 60)
	, orderlistMargins(conf, "Pattern Editor", "DefaultSequenceMargins", 0)
	, rowDisplayOffset(conf, "Pattern Editor", "RowDisplayOffset", 0)
	, patternFont(conf, "Pattern Editor", "Font", FontSetting(PATTERNFONT_SMALL, 0))
	, effectVisWidth(conf, "Pattern Editor", "EffectVisWidth", -1)
	, effectVisHeight(conf, "Pattern Editor", "EffectVisHeight", -1)
	// Sample Editor
	, m_SampleUndoBufferSize(conf, "Sample Editor", "UndoBufferSize", SampleUndoBufferSize())
	, sampleEditorKeyBehaviour(conf, "Sample Editor", "KeyBehaviour", seNoteOffOnNewKey)
	, m_defaultSampleFormat(conf, "Sample Editor", "DefaultFormat", dfFLAC)
	, m_nFinetuneStep(conf, "Sample Editor", "FinetuneStep", 10)
	, m_FLACCompressionLevel(conf, "Sample Editor", "FLACCompressionLevel", 5)
	, compressITI(conf, "Sample Editor", "CompressITI", true)
	, m_MayNormalizeSamplesOnLoad(conf, "Sample Editor", "MayNormalizeSamplesOnLoad", true)
	, previewInFileDialogs(conf, "Sample Editor", "PreviewInFileDialogs", false)
	// Export
	, ExportDefaultToSoundcardSamplerate(conf, "Export", "DefaultToSoundcardSamplerate", true)
	, ExportStreamEncoderSettings(conf, MPT_USTRING("Export"))
	// Components
	, ComponentsLoadOnStartup(conf, "Components", "LoadOnStartup", ComponentManagerSettingsDefault().LoadOnStartup())
	, ComponentsKeepLoaded(conf, "Components", "KeepLoaded", ComponentManagerSettingsDefault().KeepLoaded())
	// AutoSave
	, AutosaveEnabled(conf, "AutoSave", "Enabled", true)
	, AutosaveIntervalMinutes(conf, "AutoSave", "IntervalMinutes", 10)
	, AutosaveHistoryDepth(conf, "AutoSave", "BackupHistory", 3)
	, AutosaveUseOriginalPath(conf, "AutoSave", "UseOriginalPath", true)
	, AutosavePath(conf, "AutoSave", "Path", mpt::PathString())
	// Paths
	, PathSongs(conf, "Paths", "Songs_Directory", mpt::PathString())
	, PathSamples(conf, "Paths", "Samples_Directory", mpt::PathString())
	, PathInstruments(conf, "Paths", "Instruments_Directory", mpt::PathString())
	, PathPlugins(conf, "Paths", "Plugins_Directory", mpt::PathString())
	, PathPluginPresets(conf, "Paths", "Plugin_Presets_Directory", mpt::PathString())
	, PathExport(conf, "Paths", "Export_Directory", mpt::PathString())
	, PathTunings(theApp.GetConfigPath() + MPT_PATHSTRING("tunings\\"))
	, PathUserTemplates(theApp.GetConfigPath() + MPT_PATHSTRING("TemplateModules\\"))
	// Default template
	, defaultTemplateFile(conf, "Paths", "DefaultTemplate", mpt::PathString())
	, defaultArtist(conf, "Misc", "DefaultArtist", mpt::ToUnicode(mpt::CharsetLocale, mpt::getenv("USERNAME")))
	// MRU List
	, mruListLength(conf, "Misc", "MRUListLength", 10)
	// Plugins
	, bridgeAllPlugins(conf, "VST Plugins", "BridgeAllPlugins", false)
	, enableAutoSuspend(conf, "VST Plugins", "EnableAutoSuspend", false)
	, pluginProjectPath(conf, "VST Plugins", "ProjectPath", std::wstring())
	, vstHostProductString(conf, "VST Plugins", "HostProductString", "OpenMPT")
	, vstHostVendorString(conf, "VST Plugins", "HostVendorString", "OpenMPT project")
	, vstHostVendorVersion(conf, "VST Plugins", "HostVendorVersion", MptVersion::num)
	// Update
	, UpdateLastUpdateCheck(conf, "Update", "LastUpdateCheck", mpt::Date::Unix(time_t()))
	, UpdateUpdateCheckPeriod(conf, "Update", "UpdateCheckPeriod", 7)
	, UpdateUpdateURL(conf, "Update", "UpdateURL", CUpdateCheck::defaultUpdateURL)
	, UpdateSendGUID(conf, "Update", "SendGUID", true)
	, UpdateShowUpdateHint(conf, "Update", "ShowUpdateHint", true)
	, UpdateSuggestDifferentBuildVariant(conf, "Update", "SuggestDifferentBuildVariant", true)
	, UpdateIgnoreVersion(conf, "Update", "IgnoreVersion", _T(""))
{

	// Effects
#ifndef NO_DSP
	m_MegaBassSettings.m_nXBassDepth = conf.Read<int32>("Effects", "XBassDepth", m_MegaBassSettings.m_nXBassDepth);
	m_MegaBassSettings.m_nXBassRange = conf.Read<int32>("Effects", "XBassRange", m_MegaBassSettings.m_nXBassRange);
#endif
#ifndef NO_REVERB
	m_ReverbSettings.m_nReverbDepth = conf.Read<int32>("Effects", "ReverbDepth", m_ReverbSettings.m_nReverbDepth);
	m_ReverbSettings.m_nReverbType = conf.Read<int32>("Effects", "ReverbType", m_ReverbSettings.m_nReverbType);
#endif
#ifndef NO_DSP
	m_SurroundSettings.m_nProLogicDepth = conf.Read<int32>("Effects", "ProLogicDepth", m_SurroundSettings.m_nProLogicDepth);
	m_SurroundSettings.m_nProLogicDelay = conf.Read<int32>("Effects", "ProLogicDelay", m_SurroundSettings.m_nProLogicDelay);
#endif
#ifndef NO_EQ
	m_EqSettings = conf.Read<EQPreset>("Effects", "EQ_Settings", FlatEQPreset);
	const EQPreset userPresets[] =
	{
		FlatEQPreset,																// User1
		{ "User 1",	{16,16,16,16,16,16}, { 150, 350, 700, 1500, 4500, 8000 } },		// User2
		{ "User 2",	{16,16,16,16,16,16}, { 200, 400, 800, 1750, 5000, 9000 } },		// User3
		{ "User 3",	{16,16,16,16,16,16}, { 250, 450, 900, 2000, 5000, 10000 } }		// User4
	};

	m_EqUserPresets[0] = conf.Read<EQPreset>("Effects", "EQ_User1", userPresets[0]);
	m_EqUserPresets[1] = conf.Read<EQPreset>("Effects", "EQ_User2", userPresets[1]);
	m_EqUserPresets[2] = conf.Read<EQPreset>("Effects", "EQ_User3", userPresets[2]);
	m_EqUserPresets[3] = conf.Read<EQPreset>("Effects", "EQ_User4", userPresets[3]);
#endif
	// Display (Colors)
	GetDefaultColourScheme(rgbCustomColors);
	for(int ncol = 0; ncol < MAX_MODCOLORS; ncol++)
	{
		const std::string colorName = mpt::String::Print("Color%1", mpt::fmt::dec0<2>(ncol));
		rgbCustomColors[ncol] = conf.Read<uint32>("Display", colorName, rgbCustomColors[ncol]);
	}
	// Paths
	m_szKbdFile = conf.Read<mpt::PathString>("Paths", "Key_Config_File", mpt::PathString());
	conf.Forget("Paths", "Key_Config_File");

	// init old and messy stuff:

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


	// load old and messy stuff:

	PatternClipboard::SetClipboardSize(conf.Read<int32>("Pattern Editor", "NumClipboards", mpt::saturate_cast<int32>(PatternClipboard::GetClipboardSize())));

	// Chords
	LoadChords(Chords);

	// Zxx Macros
	MIDIMacroConfig macros;
	theApp.GetDefaultMidiMacro(macros);
	for(int isfx = 0; isfx < 16; isfx++)
	{
		mpt::String::Copy(macros.szMidiSFXExt[isfx], conf.Read<std::string>("Zxx Macros", mpt::String::Print("SF%1", mpt::fmt::HEX(isfx)), macros.szMidiSFXExt[isfx]));
	}
	for(int izxx = 0; izxx < 128; izxx++)
	{
		mpt::String::Copy(macros.szMidiZXXExt[izxx], conf.Read<std::string>("Zxx Macros", mpt::String::Print("Z%1", mpt::fmt::HEX0<2>(izxx | 0x80)), macros.szMidiZXXExt[izxx]));
	}


	// MRU list
	Limit(mruListLength, 0u, 32u);
	mruFiles.reserve(mruListLength);
	for(uint32 i = 0; i < mruListLength; i++)
	{
		char key[16];
		sprintf(key, "File%u", i);

		mpt::PathString path = theApp.RelativePathToAbsolute(conf.Read<mpt::PathString>("Recent File List", key, mpt::PathString()));
		if(!path.empty())
		{
			mruFiles.push_back(path);
		}
	}

	// Fixups:
	// -------

	const MptVersion::VersionNum storedVersion = PreviousSettingsVersion;

	// Version
	if(gcsInstallGUID.Get().empty())
	{
		// No UUID found - generate one.
		gcsInstallGUID = Util::UUIDToString(Util::CreateUUID());
	}

	// Plugins
	if(storedVersion < MAKE_VERSION_NUMERIC(1,19,03,01) && vstHostProductString.Get() == "OpenMPT")
	{
		vstHostVendorVersion = MptVersion::num;
	}

	// Sound Settings
	if(storedVersion < MAKE_VERSION_NUMERIC(1,22,07,30))
	{
		if(conf.Read<bool>("Sound Settings", "KeepDeviceOpen", false))
		{
			m_SoundSettingsStopMode = SoundDeviceStopModePlaying;
		} else
		{
			m_SoundSettingsStopMode = SoundDeviceStopModeStopped;
		}
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,22,07,04))
	{
		std::vector<uint32> sampleRates = m_SoundSampleRates;
		if(std::count(sampleRates.begin(), sampleRates.end(), MixerSamplerate) == 0)
		{
			sampleRates.push_back(MixerSamplerate);
			std::sort(sampleRates.begin(), sampleRates.end());
			std::reverse(sampleRates.begin(), sampleRates.end());
			m_SoundSampleRates = sampleRates;
		}
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,22,07,04))
	{
		m_SoundDeviceID_DEPRECATED = conf.Read<SoundDevice::Legacy::ID>("Sound Settings", "WaveDevice", SoundDevice::Legacy::ID());
		Setting<uint32> m_BufferLength_DEPRECATED(conf, "Sound Settings", "BufferLength", 50);
		Setting<uint32> m_LatencyMS(conf, "Sound Settings", "Latency", Util::Round<int32>(SoundDevice::Settings().Latency * 1000.0));
		Setting<uint32> m_UpdateIntervalMS(conf, "Sound Settings", "UpdateInterval", Util::Round<int32>(SoundDevice::Settings().UpdateInterval * 1000.0));
		Setting<SampleFormat> m_SampleFormat(conf, "Sound Settings", "BitsPerSample", SoundDevice::Settings().sampleFormat);
		Setting<bool> m_SoundDeviceExclusiveMode(conf, "Sound Settings", "ExclusiveMode", SoundDevice::Settings().ExclusiveMode);
		Setting<bool> m_SoundDeviceBoostThreadPriority(conf, "Sound Settings", "BoostThreadPriority", SoundDevice::Settings().BoostThreadPriority);
		Setting<bool> m_SoundDeviceUseHardwareTiming(conf, "Sound Settings", "UseHardwareTiming", SoundDevice::Settings().UseHardwareTiming);
		Setting<SoundDevice::ChannelMapping> m_SoundDeviceChannelMapping(conf, "Sound Settings", "ChannelMapping", SoundDevice::Settings().Channels);
		if(storedVersion < MAKE_VERSION_NUMERIC(1,21,01,26))
		{
			if(m_BufferLength_DEPRECATED != 0)
			{
				if(m_BufferLength_DEPRECATED < 1) m_BufferLength_DEPRECATED = 1; // 1ms
				if(m_BufferLength_DEPRECATED > 1000) m_BufferLength_DEPRECATED = 1000; // 1sec
				if((m_SoundDeviceID_DEPRECATED & SoundDevice::Legacy::MaskType) == SoundDevice::Legacy::TypeASIO)
				{
					m_LatencyMS = m_BufferLength_DEPRECATED * 1;
					m_UpdateIntervalMS = m_BufferLength_DEPRECATED / 8;
				} else
				{
					m_LatencyMS = m_BufferLength_DEPRECATED * 3;
					m_UpdateIntervalMS = m_BufferLength_DEPRECATED / 8;
				}
				if(!m_UpdateIntervalMS) m_UpdateIntervalMS = static_cast<uint32>(SoundDevice::Settings().UpdateInterval * 1000.0);
			}
			conf.Remove(m_BufferLength_DEPRECATED.GetPath());
		}
		if(storedVersion < MAKE_VERSION_NUMERIC(1,22,01,03))
		{
			m_SoundDeviceExclusiveMode = ((MixerFlags & OLD_SOUNDSETUP_SECONDARY) == 0);
		}
		if(storedVersion < MAKE_VERSION_NUMERIC(1,22,01,03))
		{
			m_SoundDeviceBoostThreadPriority = ((MixerFlags & OLD_SOUNDSETUP_NOBOOSTTHREADPRIORITY) == 0);
		}
		if(storedVersion < MAKE_VERSION_NUMERIC(1,22,07,03))
		{
			m_SoundDeviceChannelMapping = SoundDevice::ChannelMapping::BaseChannel(MixerOutputChannels, conf.Read<int>("Sound Settings", "ASIOBaseChannel", 0));
		}
		m_SoundDeviceSettingsDefaults.Latency = m_LatencyMS / 1000.0;
		m_SoundDeviceSettingsDefaults.UpdateInterval = m_UpdateIntervalMS / 1000.0;
		m_SoundDeviceSettingsDefaults.Samplerate = MixerSamplerate;
		if(m_SoundDeviceSettingsDefaults.Channels.GetNumHostChannels() != MixerOutputChannels)
		{
			// reset invalid channel mapping to default
			m_SoundDeviceSettingsDefaults.Channels = SoundDevice::ChannelMapping(MixerOutputChannels);
		}
		m_SoundDeviceSettingsDefaults.sampleFormat = m_SampleFormat;
		m_SoundDeviceSettingsDefaults.ExclusiveMode = m_SoundDeviceExclusiveMode;
		m_SoundDeviceSettingsDefaults.BoostThreadPriority = m_SoundDeviceBoostThreadPriority;
		m_SoundDeviceSettingsDefaults.UseHardwareTiming = m_SoundDeviceUseHardwareTiming;
		m_SoundDeviceSettingsUseOldDefaults = true;
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,25,00,04))
	{
		m_SoundDeviceDirectSoundOldDefaultIdentifier = true;
	}
	if(MixerSamplerate == 0)
	{
		MixerSamplerate = MixerSettings().gdwMixingFreq;
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,21,01,26))
	{
		MixerFlags &= ~OLD_SOUNDSETUP_REVERSESTEREO;
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,22,01,03))
	{
		MixerFlags &= ~OLD_SOUNDSETUP_SECONDARY;
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,22,01,03))
	{
		MixerFlags &= ~OLD_SOUNDSETUP_NOBOOSTTHREADPRIORITY;
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,20,00,22))
	{
		MixerSettings settings = GetMixerSettings();
		settings.SetVolumeRampUpSamples(conf.Read<int32>("Sound Settings", "VolumeRampSamples", 42));
		settings.SetVolumeRampDownSamples(conf.Read<int32>("Sound Settings", "VolumeRampSamples", 42));
		SetMixerSettings(settings);
		conf.Remove("Sound Settings", "VolumeRampSamples");
	} else if(storedVersion < MAKE_VERSION_NUMERIC(1,22,07,18))
	{
		MixerSettings settings = GetMixerSettings();
		settings.SetVolumeRampUpSamples(conf.Read<int32>("Sound Settings", "VolumeRampUpSamples", MixerSettings().GetVolumeRampUpSamples()));
		settings.SetVolumeRampDownSamples(conf.Read<int32>("Sound Settings", "VolumeRampDownSamples", MixerSettings().GetVolumeRampDownSamples()));
		SetMixerSettings(settings);
	}
	Limit(ResamplerCutoffPercent, 0, 100);

	// Misc
	if(defaultModType == MOD_TYPE_NONE)
	{
		defaultModType = MOD_TYPE_IT;
	}

	// MIDI Settings
	if((m_dwMidiSetup & 0x40) != 0 && storedVersion < MAKE_VERSION_NUMERIC(1,20,00,86))
	{
		// This flag used to be "amplify MIDI Note Velocity" - with a fixed amplification factor of 2.
		midiVelocityAmp = 200;
		m_dwMidiSetup &= ~0x40;
	}

	// Pattern Editor
	if(storedVersion < MAKE_VERSION_NUMERIC(1,17,02,50))
	{
		m_dwPatternSetup |= PATTERN_NOTEFADE;
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,17,03,01))
	{
		m_dwPatternSetup |= PATTERN_RESETCHANNELS;
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,19,00,07))
	{
		m_dwPatternSetup &= ~0x800;					// this was previously deprecated and is now used for something else
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,20,00,04))
	{
		m_dwPatternSetup &= ~0x200000;				// ditto
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,20,00,07))
	{
		m_dwPatternSetup &= ~0x400000;				// ditto
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,20,00,39))
	{
		m_dwPatternSetup &= ~0x10000000;			// ditto
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,24,01,04))
	{
		commentsFont = FontSetting("Courier New", (m_dwPatternSetup & 0x02) ? 120 : 90);
		patternFont = FontSetting((m_dwPatternSetup & 0x08) ? PATTERNFONT_SMALL : PATTERNFONT_LARGE, 0);
		m_dwPatternSetup &= ~(0x08 | 0x02);
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,25,00,08) && glGeneralWindowHeight < 222)
	{
		glGeneralWindowHeight += 44;
	}
	if(storedVersion < MAKE_VERSION_NUMERIC(1,25,00,16) && (m_dwPatternSetup & 0x100000))
	{
		// Move MIDI recording to MIDI setup
		m_dwPatternSetup &= ~0x100000;
		m_dwMidiSetup |= MIDISETUP_ENABLE_RECORD_DEFAULT;
	}

	// Effects
#ifndef NO_EQ
	FixupEQ(&m_EqSettings);
	FixupEQ(&m_EqUserPresets[0]);
	FixupEQ(&m_EqUserPresets[1]);
	FixupEQ(&m_EqUserPresets[2]);
	FixupEQ(&m_EqUserPresets[3]);
#endif // !NO_EQ

	// Zxx Macros
	if((MAKE_VERSION_NUMERIC(1,17,00,00) <= storedVersion) && (storedVersion < MAKE_VERSION_NUMERIC(1,20,00,00)))
	{
		// Fix old nasty broken (non-standard) MIDI configs in INI file.
		macros.UpgradeMacros();
	}
	theApp.SetDefaultMidiMacro(macros);

	// Paths
	m_szKbdFile = theApp.RelativePathToAbsolute(m_szKbdFile);

	// Sample undo buffer size (used to be a hidden, absolute setting in MiB)
	int64 oldUndoSize = m_SampleUndoBufferSize.Get().GetSizeInPercent();
	if(storedVersion < MAKE_VERSION_NUMERIC(1,22,07,25) && oldUndoSize != SampleUndoBufferSize::defaultSize && oldUndoSize != 0)
	{
		m_SampleUndoBufferSize = SampleUndoBufferSize(static_cast<int32>(100 * (oldUndoSize << 20) / SampleUndoBufferSize(100).GetSizeInBytes()));
	}

	// More controls in the plugin selection dialog
	if(storedVersion < MAKE_VERSION_NUMERIC(1,26,00,26))
	{
		gnPlugWindowHeight += 40;
	}

	// Last fixup: update config version
	IniVersion = MptVersion::str;

	// Write updated settings
	conf.Flush();

}


void TrackerSettings::MigrateOldSoundDeviceSettings(SoundDevice::Manager &manager)
//--------------------------------------------------------------------------------
{
	if(m_SoundDeviceSettingsUseOldDefaults)
	{
		// get the old default device
		SetSoundDeviceIdentifier(SoundDevice::Legacy::FindDeviceInfo(manager, m_SoundDeviceID_DEPRECATED).GetIdentifier());
		// apply old global sound device settings to each found device
		for(std::vector<SoundDevice::Info>::const_iterator it = manager.begin(); it != manager.end(); ++it)
		{
			SetSoundDeviceSettings(it->GetIdentifier(), GetSoundDeviceSettingsDefaults());
		}
	}
#ifdef MPT_WITH_DSOUND
	if(m_SoundDeviceDirectSoundOldDefaultIdentifier)
	{
		mpt::ustring oldIdentifier = SoundDevice::Legacy::GetDirectSoundDefaultDeviceIdentifierPre_1_25_00_04();
		mpt::ustring newIdentifier = SoundDevice::Legacy::GetDirectSoundDefaultDeviceIdentifier_1_25_00_04();
		if(!oldIdentifier.empty())
		{
			SoundDevice::Info info = manager.FindDeviceInfo(newIdentifier);
			if(info.IsValid())
			{
				SoundDevice::Settings defaults =
					m_SoundDeviceSettingsUseOldDefaults ?
						GetSoundDeviceSettingsDefaults()
					:
						manager.GetDeviceCaps(newIdentifier).DefaultSettings
					;
				conf.Write(L"Sound Settings", mpt::ToWide(newIdentifier) + L"_" + L"Latency",
					conf.Read(L"Sound Settings", mpt::ToWide(oldIdentifier) + L"_" + L"Latency", Util::Round<int32>(defaults.Latency * 1000000.0)));
				conf.Write(L"Sound Settings", mpt::ToWide(newIdentifier) + L"_" + L"UpdateInterval",
					conf.Read(L"Sound Settings", mpt::ToWide(oldIdentifier) + L"_" + L"UpdateInterval", Util::Round<int32>(defaults.UpdateInterval * 1000000.0)));
				conf.Write(L"Sound Settings", mpt::ToWide(newIdentifier) + L"_" + L"SampleRate",
					conf.Read(L"Sound Settings", mpt::ToWide(oldIdentifier) + L"_" + L"SampleRate", defaults.Samplerate));
				conf.Write(L"Sound Settings", mpt::ToWide(newIdentifier) + L"_" + L"Channels",
					conf.Read(L"Sound Settings", mpt::ToWide(oldIdentifier) + L"_" + L"Channels", defaults.Channels.GetNumHostChannels()));
				conf.Write(L"Sound Settings", mpt::ToWide(newIdentifier) + L"_" + L"SampleFormat",
					conf.Read(L"Sound Settings", mpt::ToWide(oldIdentifier) + L"_" + L"SampleFormat", defaults.sampleFormat));
				conf.Write(L"Sound Settings", mpt::ToWide(newIdentifier) + L"_" + L"ExclusiveMode",
					conf.Read(L"Sound Settings", mpt::ToWide(oldIdentifier) + L"_" + L"ExclusiveMode", defaults.ExclusiveMode));
				conf.Write(L"Sound Settings", mpt::ToWide(newIdentifier) + L"_" + L"BoostThreadPriority",
					conf.Read(L"Sound Settings", mpt::ToWide(oldIdentifier) + L"_" + L"BoostThreadPriority", defaults.BoostThreadPriority));
				conf.Write(L"Sound Settings", mpt::ToWide(newIdentifier) + L"_" + L"KeepDeviceRunning",
					conf.Read(L"Sound Settings", mpt::ToWide(oldIdentifier) + L"_" + L"KeepDeviceRunning", defaults.KeepDeviceRunning));
				conf.Write(L"Sound Settings", mpt::ToWide(newIdentifier) + L"_" + L"UseHardwareTiming",
					conf.Read(L"Sound Settings", mpt::ToWide(oldIdentifier) + L"_" + L"UseHardwareTiming", defaults.UseHardwareTiming));
				conf.Write(L"Sound Settings", mpt::ToWide(newIdentifier) + L"_" + L"DitherType",
					conf.Read(L"Sound Settings", mpt::ToWide(oldIdentifier) + L"_" + L"DitherType", defaults.DitherType));
				conf.Write(L"Sound Settings", mpt::ToWide(newIdentifier) + L"_" + L"ChannelMapping",
					conf.Read(L"Sound Settings", mpt::ToWide(oldIdentifier) + L"_" + L"ChannelMapping", defaults.Channels));
			}
		}
	}
#endif // MPT_WITH_DSOUND
}


struct StoredSoundDeviceSettings
{

private:

	SettingsContainer &conf;
	const SoundDevice::Info deviceInfo;
	
private:

	Setting<uint32> LatencyUS;
	Setting<uint32> UpdateIntervalUS;
	Setting<uint32> Samplerate;
	Setting<uint8> ChannelsOld; // compatibility with older versions
	Setting<SoundDevice::ChannelMapping> ChannelMapping;
	Setting<SampleFormat> sampleFormat;
	Setting<bool> ExclusiveMode;
	Setting<bool> BoostThreadPriority;
	Setting<bool> KeepDeviceRunning;
	Setting<bool> UseHardwareTiming;
	Setting<int> DitherType;

public:

	StoredSoundDeviceSettings(SettingsContainer &conf, const SoundDevice::Info & deviceInfo, const SoundDevice::Settings & defaults)
		: conf(conf)
		, deviceInfo(deviceInfo)
		, LatencyUS(conf, L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"Latency", Util::Round<int32>(defaults.Latency * 1000000.0))
		, UpdateIntervalUS(conf, L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"UpdateInterval", Util::Round<int32>(defaults.UpdateInterval * 1000000.0))
		, Samplerate(conf, L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"SampleRate", defaults.Samplerate)
		, ChannelsOld(conf, L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"Channels", mpt::saturate_cast<uint8>((int)defaults.Channels))
		, ChannelMapping(conf, L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"ChannelMapping", defaults.Channels)
		, sampleFormat(conf, L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"SampleFormat", defaults.sampleFormat)
		, ExclusiveMode(conf, L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"ExclusiveMode", defaults.ExclusiveMode)
		, BoostThreadPriority(conf, L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"BoostThreadPriority", defaults.BoostThreadPriority)
		, KeepDeviceRunning(conf, L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"KeepDeviceRunning", defaults.KeepDeviceRunning)
		, UseHardwareTiming(conf, L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"UseHardwareTiming", defaults.UseHardwareTiming)
		, DitherType(conf, L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"DitherType", defaults.DitherType)
	{
		if(ChannelMapping.Get().GetNumHostChannels() != ChannelsOld)
		{
			// If the stored channel count and the count of channels used in the channel mapping do not match,
			// construct a default mapping from the channel count.
			ChannelMapping = SoundDevice::ChannelMapping(ChannelsOld);
		}
		// store informational data (not read back, just to allow the user to mock with the raw ini file)
		conf.Write(L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"Type", deviceInfo.type);
		conf.Write(L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"InternalID", deviceInfo.internalID);
		conf.Write(L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"API", deviceInfo.apiName);
		conf.Write(L"Sound Settings", mpt::ToWide(deviceInfo.GetIdentifier()) + L"_" + L"Name", deviceInfo.name);
	}

	StoredSoundDeviceSettings & operator = (const SoundDevice::Settings &settings)
	{
		LatencyUS = Util::Round<int32>(settings.Latency * 1000000.0);
		UpdateIntervalUS = Util::Round<int32>(settings.UpdateInterval * 1000000.0);
		Samplerate = settings.Samplerate;
		ChannelsOld = mpt::saturate_cast<uint8>((int)settings.Channels);
		ChannelMapping = settings.Channels;
		sampleFormat = settings.sampleFormat;
		ExclusiveMode = settings.ExclusiveMode;
		BoostThreadPriority = settings.BoostThreadPriority;
		KeepDeviceRunning = settings.KeepDeviceRunning;
		UseHardwareTiming = settings.UseHardwareTiming;
		DitherType = settings.DitherType;
		return *this;
	}

	operator SoundDevice::Settings () const
	{
		SoundDevice::Settings settings;
		settings.Latency = LatencyUS / 1000000.0;
		settings.UpdateInterval = UpdateIntervalUS / 1000000.0;
		settings.Samplerate = Samplerate;
		settings.Channels = ChannelMapping;
		settings.sampleFormat = sampleFormat;
		settings.ExclusiveMode = ExclusiveMode;
		settings.BoostThreadPriority = BoostThreadPriority;
		settings.KeepDeviceRunning = KeepDeviceRunning;
		settings.UseHardwareTiming = UseHardwareTiming;
		settings.DitherType = DitherType;
		return settings;
	}

};

SoundDevice::Settings TrackerSettings::GetSoundDeviceSettingsDefaults() const
//---------------------------------------------------------------------------
{
	return m_SoundDeviceSettingsDefaults;
}

SoundDevice::Identifier TrackerSettings::GetSoundDeviceIdentifier() const
//-----------------------------------------------------------------------
{
	return m_SoundDeviceIdentifier;
}

void TrackerSettings::SetSoundDeviceIdentifier(const SoundDevice::Identifier &identifier)
//---------------------------------------------------------------------------------------
{
	m_SoundDeviceIdentifier = identifier;
}

SoundDevice::Settings TrackerSettings::GetSoundDeviceSettings(const SoundDevice::Identifier &device) const
//--------------------------------------------------------------------------------------------------------
{
	const SoundDevice::Info deviceInfo = theApp.GetSoundDevicesManager()->FindDeviceInfo(device);
	if(!deviceInfo.IsValid())
	{
		return SoundDevice::Settings();
	}
	const SoundDevice::Caps deviceCaps = theApp.GetSoundDevicesManager()->GetDeviceCaps(device, CMainFrame::GetMainFrame()->gpSoundDevice);
	SoundDevice::Settings settings = StoredSoundDeviceSettings(conf, deviceInfo, deviceCaps.DefaultSettings);
	return settings;
}

void TrackerSettings::SetSoundDeviceSettings(const SoundDevice::Identifier &device, const SoundDevice::Settings &settings)
//------------------------------------------------------------------------------------------------------------------------
{
	const SoundDevice::Info deviceInfo = theApp.GetSoundDevicesManager()->FindDeviceInfo(device);
	if(!deviceInfo.IsValid())
	{
		return;
	}
	const SoundDevice::Caps deviceCaps = theApp.GetSoundDevicesManager()->GetDeviceCaps(device, CMainFrame::GetMainFrame()->gpSoundDevice);
	StoredSoundDeviceSettings(conf, deviceInfo, deviceCaps.DefaultSettings) = settings;
}


MixerSettings TrackerSettings::GetMixerSettings() const
//-----------------------------------------------------
{
	MixerSettings settings;
	settings.m_nMaxMixChannels = MixerMaxChannels;
	settings.DSPMask = MixerDSPMask;
	settings.MixerFlags = MixerFlags;
	settings.gdwMixingFreq = MixerSamplerate;
	settings.gnChannels = MixerOutputChannels;
	settings.m_nPreAmp = MixerPreAmp;
	settings.m_nStereoSeparation = MixerStereoSeparation;
	settings.VolumeRampUpMicroseconds = MixerVolumeRampUpMicroseconds;
	settings.VolumeRampDownMicroseconds = MixerVolumeRampDownMicroseconds;
	return settings;
}

void TrackerSettings::SetMixerSettings(const MixerSettings &settings)
//-------------------------------------------------------------------
{
	MixerMaxChannels = settings.m_nMaxMixChannels;
	MixerDSPMask = settings.DSPMask;
	MixerFlags = settings.MixerFlags;
	MixerSamplerate = settings.gdwMixingFreq;
	MixerOutputChannels = settings.gnChannels;
	MixerPreAmp = settings.m_nPreAmp;
	MixerStereoSeparation = settings.m_nStereoSeparation;
	MixerVolumeRampUpMicroseconds = settings.VolumeRampUpMicroseconds;
	MixerVolumeRampDownMicroseconds = settings.VolumeRampDownMicroseconds;
}


CResamplerSettings TrackerSettings::GetResamplerSettings() const
//--------------------------------------------------------------
{
	CResamplerSettings settings;
	settings.SrcMode = ResamplerMode;
	settings.gbWFIRType = ResamplerSubMode;
	settings.gdWFIRCutoff = ResamplerCutoffPercent * 0.01;
	return settings;
}

void TrackerSettings::SetResamplerSettings(const CResamplerSettings &settings)
//----------------------------------------------------------------------------
{
	ResamplerMode = settings.SrcMode;
	ResamplerSubMode = settings.gbWFIRType;
	ResamplerCutoffPercent = Util::Round<int32>(settings.gdWFIRCutoff * 100.0);
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
	// Sample / instrument editor
	colours[MODCOLOR_BACKSAMPLE] = RGB(0x00, 0x00, 0x00);
	colours[MODCOLOR_SAMPLESELECTED] = RGB(0xFF, 0xFF, 0xFF);
	colours[MODCOLOR_BACKENV] = RGB(0x00, 0x00, 0x00);
}


void TrackerSettings::FixupEQ(EQPreset *pEqSettings)
//--------------------------------------------------
{
	for(UINT i=0; i<MAX_EQ_BANDS; i++)
	{
		if(pEqSettings->Gains[i] > 32)
			pEqSettings->Gains[i] = 16;
		if((pEqSettings->Freqs[i] < 100) || (pEqSettings->Freqs[i] > 10000))
			pEqSettings->Freqs[i] = FlatEQPreset.Freqs[i];
	}
	mpt::String::SetNullTerminator(pEqSettings->szName);
}


void TrackerSettings::SaveSettings()
//----------------------------------
{

	WINDOWPLACEMENT wpl;
	wpl.length = sizeof(WINDOWPLACEMENT);
	CMainFrame::GetMainFrame()->GetWindowPlacement(&wpl);
	conf.Write<WINDOWPLACEMENT>("Display", "WindowPlacement", wpl);

	conf.Write<int32>("Pattern Editor", "NumClipboards", mpt::saturate_cast<int32>(PatternClipboard::GetClipboardSize()));

	// Effects
#ifndef NO_DSP
	conf.Write<int32>("Effects", "XBassDepth", m_MegaBassSettings.m_nXBassDepth);
	conf.Write<int32>("Effects", "XBassRange", m_MegaBassSettings.m_nXBassRange);
#endif
#ifndef NO_REVERB
	conf.Write<int32>("Effects", "ReverbDepth", m_ReverbSettings.m_nReverbDepth);
	conf.Write<int32>("Effects", "ReverbType", m_ReverbSettings.m_nReverbType);
#endif
#ifndef NO_DSP
	conf.Write<int32>("Effects", "ProLogicDepth", m_SurroundSettings.m_nProLogicDepth);
	conf.Write<int32>("Effects", "ProLogicDelay", m_SurroundSettings.m_nProLogicDelay);
#endif
#ifndef NO_EQ
	conf.Write<EQPreset>("Effects", "EQ_Settings", m_EqSettings);
	conf.Write<EQPreset>("Effects", "EQ_User1", m_EqUserPresets[0]);
	conf.Write<EQPreset>("Effects", "EQ_User2", m_EqUserPresets[1]);
	conf.Write<EQPreset>("Effects", "EQ_User3", m_EqUserPresets[2]);
	conf.Write<EQPreset>("Effects", "EQ_User4", m_EqUserPresets[3]);
#endif

	// Display (Colors)
	for(int ncol = 0; ncol < MAX_MODCOLORS; ncol++)
	{
		conf.Write<uint32>("Display", mpt::String::Print("Color%1", mpt::fmt::dec0<2>(ncol)), rgbCustomColors[ncol]);
	}

	// Paths
	// Obsolete, since we always write to Keybindings.mkb now.
	// Older versions of OpenMPT 1.18+ will look for this file if this entry is missing, so removing this entry after having read it is kind of backwards compatible.
	conf.Remove("Paths", "Key_Config_File");

	// Chords
	SaveChords(Chords);

	// Save default macro configuration
	MIDIMacroConfig macros;
	theApp.GetDefaultMidiMacro(macros);
	for(int isfx = 0; isfx < 16; isfx++)
	{
		conf.Write<std::string>("Zxx Macros", mpt::String::Print("SF%1", mpt::fmt::HEX(isfx)), macros.szMidiSFXExt[isfx]);
	}
	for(int izxx = 0; izxx < 128; izxx++)
	{
		conf.Write<std::string>("Zxx Macros", mpt::String::Print("Z%1", mpt::fmt::HEX0<2>(izxx | 0x80)), macros.szMidiZXXExt[izxx]);
	}

	// MRU list
	for(uint32 i = 0; i < (ID_MRU_LIST_LAST - ID_MRU_LIST_FIRST + 1); i++)
	{
		char key[16];
		sprintf(key, "File%u", i);

		if(i < mruFiles.size())
		{
			mpt::PathString path = mruFiles[i];
			if(theApp.IsPortableMode())
			{
				path = theApp.AbsolutePathToRelative(path);
			}
			conf.Write<mpt::PathString>("Recent File List", key, path);
		} else
		{
			conf.Remove("Recent File List", key);
		}
	}
}


bool TrackerSettings::IsComponentBlocked(const std::string &key)
//---------------------------------------------------------------
{
	return Setting<bool>(conf, "Components", std::string("Block") + key, ComponentManagerSettingsDefault().IsBlocked(key));
}


std::vector<uint32> TrackerSettings::GetSampleRates() const
//---------------------------------------------------------
{
	return m_SoundSampleRates;
}


std::vector<uint32> TrackerSettings::GetDefaultSampleRates()
//----------------------------------------------------------
{
	static const uint32 samplerates [] = {
		192000,
		176400,
		96000,
		88200,
		48000,
		44100,
		32000,
		24000,
		22050,
		16000,
		11025,
		8000
	};
	return std::vector<uint32>(samplerates, samplerates + CountOf(samplerates));
}


////////////////////////////////////////////////////////////////////////////////
// Chords

void TrackerSettings::LoadChords(MPTChords &chords)
//-------------------------------------------------
{	
	for(size_t i = 0; i < CountOf(chords); i++)
	{
		uint32 chord;
		char note[4];
		sprintf(note, "%s%u", NoteNamesSharp[i % 12], i / 12);
		if((chord = conf.Read<int32>("Chords", note, -1)) != uint32(-1))
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
	for(size_t i = 0; i < CountOf(chords); i++)
	{
		int32 s = (chords[i].key) | (chords[i].notes[0] << 6) | (chords[i].notes[1] << 12) | (chords[i].notes[2] << 18);
		char note[4];
		sprintf(note, "%s%u", NoteNamesSharp[i % 12], i / 12);
		conf.Write<int32>("Chords", note, s);
	}
}


void TrackerSettings::SetMIDIDevice(UINT id)
//------------------------------------------
{
	m_nMidiDevice = id;
	MIDIINCAPS mic;
	mic.szPname[0] = 0;
	if(midiInGetDevCaps(id, &mic, sizeof(mic)) == MMSYSERR_NOERROR)
	{
		midiDeviceName = mic.szPname;
	}
}


UINT TrackerSettings::GetCurrentMIDIDevice()
//------------------------------------------
{
	if(midiDeviceName.Get().IsEmpty())
		return m_nMidiDevice;
	
	CString deviceName = midiDeviceName;
	deviceName.TrimRight();

	MIDIINCAPS mic;
	UINT candidate = m_nMidiDevice, numDevs = midiInGetNumDevs();
	for(UINT i = 0; i < numDevs; i++)
	{
		mic.szPname[0] = 0;
		if(midiInGetDevCaps(i, &mic, sizeof(mic)) != MMSYSERR_NOERROR)
			continue;

		// Some device names have trailing spaces (e.g. "USB MIDI Interface "), but those may get lost in our settings framework.
		mpt::String::SetNullTerminator(mic.szPname);
		size_t strLen = _tcslen(mic.szPname);
		while(strLen-- > 0)
		{
			if(mic.szPname[strLen] == _T(' '))
				mic.szPname[strLen] = 0;
			else
				break;
		}
		if(mic.szPname == deviceName)
		{
			candidate = i;
			numDevs = m_nMidiDevice + 1;
			// If the same device name exists twice, try to match both device number and name
			if(candidate == m_nMidiDevice)
				return candidate;
		}
	}
	// If the device changed its ID, update it now.
	m_nMidiDevice = candidate;
	return candidate;
}


std::string IgnoredCCsToString(const std::bitset<128> &midiIgnoreCCs)
//-------------------------------------------------------------------
{
	std::string cc;
	bool first = true;
	for(int i = 0; i < 128; i++)
	{
		if(midiIgnoreCCs[i])
		{
			if(!first)
			{
				cc += ",";
			}
			cc += mpt::ToString(i);
			first = false;
		}
	}
	return cc;
}


std::bitset<128> StringToIgnoredCCs(const std::string &in)
//--------------------------------------------------------
{
	CString cc = in.c_str();
	std::bitset<128> midiIgnoreCCs;
	midiIgnoreCCs.reset();
	int curPos = 0;
	CString ccToken = cc.Tokenize(_T(", "), curPos);
	while(ccToken != _T(""))
	{
		int ccNumber = ConvertStrTo<int>(ccToken);
		if(ccNumber >= 0 && ccNumber <= 127)
			midiIgnoreCCs.set(ccNumber);
		ccToken = cc.Tokenize(_T(", "), curPos);
	}
	return midiIgnoreCCs;
}


DefaultAndWorkingDirectory::DefaultAndWorkingDirectory()
//------------------------------------------------------
{
	return;
}

DefaultAndWorkingDirectory::DefaultAndWorkingDirectory(const mpt::PathString &def)
//--------------------------------------------------------------------------------
	: m_Default(def)
	, m_Working(def)
{
	return;
}

DefaultAndWorkingDirectory::~DefaultAndWorkingDirectory()
//-------------------------------------------------------
{
	return;
}

void DefaultAndWorkingDirectory::SetDefaultDir(const mpt::PathString &filenameFrom, bool stripFilename)
//-----------------------------------------------------------------------------------------------------
{
	if(InternalSet(m_Default, filenameFrom, stripFilename) && !m_Default.empty())
	{
		// When updating default directory, also update the working directory.
		InternalSet(m_Working, filenameFrom, stripFilename);
	}
}

void DefaultAndWorkingDirectory::SetWorkingDir(const mpt::PathString &filenameFrom, bool stripFilename)
//-----------------------------------------------------------------------------------------------------
{
	InternalSet(m_Working, filenameFrom, stripFilename);
}

mpt::PathString DefaultAndWorkingDirectory::GetDefaultDir() const
//---------------------------------------------------------------
{
	return m_Default;
}

mpt::PathString DefaultAndWorkingDirectory::GetWorkingDir() const
//---------------------------------------------------------------
{
	return m_Working;
}

// Retrieve / set default directory from given string and store it our setup variables
// If stripFilename is true, the filenameFrom parameter is assumed to be a full path including a filename.
// Return true if the value changed.
bool DefaultAndWorkingDirectory::InternalSet(mpt::PathString &dest, const mpt::PathString &filenameFrom, bool stripFilename)
//--------------------------------------------------------------------------------------------------------------------------
{
	mpt::PathString newPath = (stripFilename ? filenameFrom.GetPath() : filenameFrom);
	newPath.EnsureTrailingSlash();
	mpt::PathString oldPath = dest;
	dest = newPath;
	return newPath != oldPath;
}

ConfigurableDirectory::ConfigurableDirectory(SettingsContainer &conf, const AnyStringLocale &section, const AnyStringLocale &key, const mpt::PathString &def)
//-----------------------------------------------------------------------------------------------------------------------------------------------------------
	: conf(conf)
	, m_Setting(conf, section, key, def)
{
	SetDefaultDir(theApp.RelativePathToAbsolute(m_Setting), false);
}

ConfigurableDirectory::~ConfigurableDirectory()
//---------------------------------------------
{
	m_Setting = theApp.IsPortableMode() ? theApp.AbsolutePathToRelative(m_Default) : m_Default;
}



OPENMPT_NAMESPACE_END
