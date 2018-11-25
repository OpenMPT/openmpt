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
#include "../common/mptStringBuffer.h"
#include "TrackerSettings.h"
#include "../common/misc_util.h"
#include "PatternClipboard.h"
#include "../common/ComponentManager.h"
#include "ExceptionHandler.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/Tables.h"
#include "../common/mptUUID.h"
#include "../common/mptFileIO.h"
#include "../soundlib/tuningcollection.h"


#include <algorithm>


OPENMPT_NAMESPACE_BEGIN


#define OLD_SOUNDSETUP_REVERSESTEREO         0x20
#define OLD_SOUNDSETUP_SECONDARY             0x40
#define OLD_SOUNDSETUP_NOBOOSTTHREADPRIORITY 0x80


TrackerSettings &TrackerSettings::Instance()
{
	return theApp.GetTrackerSettings();
}


static Version GetPreviousSettingsVersion(const mpt::ustring &iniVersion)
{
	if(!iniVersion.empty())
	{
		return Version::Parse(iniVersion);
	} else
	{
		// No version stored.
		// This is the first run, thus set the previous version to our current
		// version which will avoid running all settings upgrade code.
		return Version::Current();
	}
}


mpt::ustring SettingsModTypeToString(MODTYPE modtype)
{
	return mpt::ToUnicode(mpt::CharsetUTF8, CSoundFile::GetModSpecifications(modtype).fileExtension);
}

MODTYPE SettingsStringToModType(const mpt::ustring &str)
{
	return CModSpecifications::ExtensionToType(mpt::ToCharset(mpt::CharsetUTF8, str));
}


static uint32 GetDefaultPatternSetup()
{
	return PATTERN_PLAYNEWNOTE | PATTERN_EFFECTHILIGHT
		| PATTERN_CENTERROW | PATTERN_DRAGNDROPEDIT
		| PATTERN_FLATBUTTONS | PATTERN_NOEXTRALOUD | PATTERN_2NDHIGHLIGHT
		| PATTERN_STDHIGHLIGHT | PATTERN_SHOWPREVIOUS | PATTERN_CONTSCROLL
		| PATTERN_SYNCMUTE | PATTERN_AUTODELAY | PATTERN_NOTEFADE
		| PATTERN_SHOWDEFAULTVOLUME | PATTERN_LIVEUPDATETREE | PATTERN_SYNCSAMPLEPOS;
}


void SampleUndoBufferSize::CalculateSize()
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
	: conf(conf)
	// Debug
#if !defined(NO_LOGGING) && !defined(MPT_LOG_IS_DISABLED)
	, DebugLogLevel(conf, MPT_USTRING("Debug"), MPT_USTRING("LogLevel"), static_cast<int>(mpt::log::GlobalLogLevel))
	, DebugLogFacilitySolo(conf, MPT_USTRING("Debug"), MPT_USTRING("LogFacilitySolo"), std::string())
	, DebugLogFacilityBlocked(conf, MPT_USTRING("Debug"), MPT_USTRING("LogFacilityBlocked"), std::string())
	, DebugLogFileEnable(conf, MPT_USTRING("Debug"), MPT_USTRING("LogFileEnable"), mpt::log::FileEnabled)
	, DebugLogDebuggerEnable(conf, MPT_USTRING("Debug"), MPT_USTRING("LogDebuggerEnable"), mpt::log::DebuggerEnabled)
	, DebugLogConsoleEnable(conf, MPT_USTRING("Debug"), MPT_USTRING("LogConsoleEnable"), mpt::log::ConsoleEnabled)
#endif
	, DebugTraceEnable(conf, MPT_USTRING("Debug"), MPT_USTRING("TraceEnable"), false)
	, DebugTraceSize(conf, MPT_USTRING("Debug"), MPT_USTRING("TraceSize"), 1000000)
	, DebugTraceAlwaysDump(conf, MPT_USTRING("Debug"), MPT_USTRING("TraceAlwaysDump"), false)
	, DebugStopSoundDeviceOnCrash(conf, MPT_USTRING("Debug"), MPT_USTRING("StopSoundDeviceOnCrash"), true)
	, DebugStopSoundDeviceBeforeDump(conf, MPT_USTRING("Debug"), MPT_USTRING("StopSoundDeviceBeforeDump"), false)
	, DebugDelegateToWindowsHandler(conf, MPT_USTRING("Debug"), MPT_USTRING("DelegateToWindowsHandler"), false)
{

	// Duplicate state for debug stuff in order to avoid calling into settings framework from crash context.
	ExceptionHandler::stopSoundDeviceOnCrash = DebugStopSoundDeviceOnCrash;
	ExceptionHandler::stopSoundDeviceBeforeDump = DebugStopSoundDeviceBeforeDump;
	ExceptionHandler::delegateToWindowsHandler = DebugDelegateToWindowsHandler;

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
{
	if(DebugTraceAlwaysDump)
	{
		DebugTraceDump();
	}
}


TrackerSettings::TrackerSettings(SettingsContainer &conf)
	: conf(conf)
	// Version
	, IniVersion(conf, MPT_USTRING("Version"), MPT_USTRING("Version"), mpt::ustring())
	, FirstRun(IniVersion.Get() == mpt::ustring())
	, PreviousSettingsVersion(GetPreviousSettingsVersion(IniVersion))
	, VersionInstallGUID(conf, MPT_USTRING("Version"), MPT_USTRING("InstallGUID"), mpt::UUID())
	// Display
	, m_ShowSplashScreen(conf, MPT_USTRING("Display"), MPT_USTRING("ShowSplashScreen"), true)
	, gbMdiMaximize(conf, MPT_USTRING("Display"), MPT_USTRING("MDIMaximize"), true)
	, highResUI(conf, MPT_USTRING("Display"), MPT_USTRING("HighResUI"), false)
	, glTreeSplitRatio(conf, MPT_USTRING("Display"), MPT_USTRING("MDITreeRatio"), 128)
	, glTreeWindowWidth(conf, MPT_USTRING("Display"), MPT_USTRING("MDITreeWidth"), 160)
	, glGeneralWindowHeight(conf, MPT_USTRING("Display"), MPT_USTRING("MDIGeneralHeight"), 222)
	, glPatternWindowHeight(conf, MPT_USTRING("Display"), MPT_USTRING("MDIPatternHeight"), 152)
	, glSampleWindowHeight(conf, MPT_USTRING("Display"), MPT_USTRING("MDISampleHeight"), 190)
	, glInstrumentWindowHeight(conf, MPT_USTRING("Display"), MPT_USTRING("MDIInstrumentHeight"), 300)
	, glCommentsWindowHeight(conf, MPT_USTRING("Display"), MPT_USTRING("MDICommentsHeight"), 288)
	, glGraphWindowHeight(conf, MPT_USTRING("Display"), MPT_USTRING("MDIGraphHeight"), 288)
	, gnPlugWindowX(conf, MPT_USTRING("Display"), MPT_USTRING("PlugSelectWindowX"), 243)
	, gnPlugWindowY(conf, MPT_USTRING("Display"), MPT_USTRING("PlugSelectWindowY"), 273)
	, gnPlugWindowWidth(conf, MPT_USTRING("Display"), MPT_USTRING("PlugSelectWindowWidth"), 450)
	, gnPlugWindowHeight(conf, MPT_USTRING("Display"), MPT_USTRING("PlugSelectWindowHeight"), 540)
	, gnPlugWindowLast(conf, MPT_USTRING("Display"), MPT_USTRING("PlugSelectWindowLast"), 0)
	, gnMsgBoxVisiblityFlags(conf, MPT_USTRING("Display"), MPT_USTRING("MsgBoxVisibilityFlags"), uint32_max)
	, GUIUpdateInterval(conf, MPT_USTRING("Display"), MPT_USTRING("GUIUpdateInterval"), 0)
	, FSUpdateInterval(conf, MPT_USTRING("Display"), MPT_USTRING("FSUpdateInterval"), 500)
	, VuMeterUpdateInterval(conf, MPT_USTRING("Display"), MPT_USTRING("VuMeterUpdateInterval"), 15)
	, VuMeterDecaySpeedDecibelPerSecond(conf, MPT_USTRING("Display"), MPT_USTRING("VuMeterDecaySpeedDecibelPerSecond"), 88.0f)
	, accidentalFlats(conf, MPT_USTRING("Display"), MPT_USTRING("AccidentalFlats"), false)
	, rememberSongWindows(conf, MPT_USTRING("Display"), MPT_USTRING("RememberSongWindows"), true)
	, showDirsInSampleBrowser(conf, MPT_USTRING("Display"), MPT_USTRING("ShowDirsInSampleBrowser"), false)
	, commentsFont(conf, MPT_USTRING("Display"), MPT_USTRING("Comments Font"), FontSetting(MPT_USTRING("Courier New"), 120))
	// Misc
	, ShowSettingsOnNewVersion(conf, MPT_USTRING("Misc"), MPT_USTRING("ShowSettingsOnNewVersion"), true)
	, defaultModType(conf, MPT_USTRING("Misc"), MPT_USTRING("DefaultModType"), MOD_TYPE_IT)
	, defaultNewFileAction(conf, MPT_USTRING("Misc"), MPT_USTRING("DefaultNewFileAction"), nfDefaultFormat)
	, DefaultPlugVolumeHandling(conf, MPT_USTRING("Misc"), MPT_USTRING("DefaultPlugVolumeHandling"), PLUGIN_VOLUMEHANDLING_IGNORE)
	, autoApplySmoothFT2Ramping(conf, MPT_USTRING("Misc"), MPT_USTRING("SmoothFT2Ramping"), false)
	, MiscITCompressionStereo(conf, MPT_USTRING("Misc"), MPT_USTRING("ITCompressionStereo"), 4)
	, MiscITCompressionMono(conf, MPT_USTRING("Misc"), MPT_USTRING("ITCompressionMono"), 4)
	, MiscSaveChannelMuteStatus(conf, MPT_USTRING("Misc"), MPT_USTRING("SaveChannelMuteStatus"), true)
	, MiscAllowMultipleCommandsPerKey(conf, MPT_USTRING("Misc"), MPT_USTRING("AllowMultipleCommandsPerKey"), false)
	, MiscDistinguishModifiers(conf, MPT_USTRING("Misc"), MPT_USTRING("DistinguishModifiers"), false)
	, MiscProcessPriorityClass(conf, MPT_USTRING("Misc"), MPT_USTRING("ProcessPriorityClass"), ProcessPriorityClassNORMAL)
	, MiscFlushFileBuffersOnSave(conf, MPT_USTRING("Misc"), MPT_USTRING("FlushFileBuffersOnSave"), true)
	// Sound Settings
	, m_SoundShowDeprecatedDevices(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("ShowDeprecatedDevices"), true)
	, m_SoundShowNotRecommendedDeviceWarning(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("ShowNotRecommendedDeviceWarning"), true)
	, m_SoundSampleRates(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("SampleRates"), GetDefaultSampleRates())
	, m_MorePortaudio(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("MorePortaudio"), false)
	, m_MoreRtaudio(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("MoreRtaudio"), false)
	, m_SoundSettingsOpenDeviceAtStartup(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("OpenDeviceAtStartup"), false)
	, m_SoundSettingsStopMode(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("StopMode"), SoundDeviceStopModeClosed)
	, m_SoundDeviceSettingsUseOldDefaults(false)
	, m_SoundDeviceID_DEPRECATED(SoundDevice::Legacy::ID())
	, m_SoundDeviceDirectSoundOldDefaultIdentifier(false)
	, m_SoundDeviceIdentifier(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("Device"), SoundDevice::Identifier())
	, m_SoundDevicePreferSameTypeIfDeviceUnavailable(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("PreferSameTypeIfDeviceUnavailable"), false)
	, MixerMaxChannels(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("MixChannels"), MixerSettings().m_nMaxMixChannels)
	, MixerDSPMask(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("Quality"), MixerSettings().DSPMask)
	, MixerFlags(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("SoundSetup"), MixerSettings().MixerFlags)
	, MixerSamplerate(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("Mixing_Rate"), MixerSettings().gdwMixingFreq)
	, MixerOutputChannels(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("ChannelMode"), MixerSettings().gnChannels)
	, MixerPreAmp(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("PreAmp"), MixerSettings().m_nPreAmp)
	, MixerStereoSeparation(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("StereoSeparation"), MixerSettings().m_nStereoSeparation)
	, MixerVolumeRampUpMicroseconds(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("VolumeRampUpMicroseconds"), MixerSettings().GetVolumeRampUpMicroseconds())
	, MixerVolumeRampDownMicroseconds(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("VolumeRampDownMicroseconds"), MixerSettings().GetVolumeRampDownMicroseconds())
	, ResamplerMode(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("SrcMode"), CResamplerSettings().SrcMode)
	, ResamplerSubMode(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("XMMSModplugResamplerWFIRType"), CResamplerSettings().gbWFIRType)
	, ResamplerCutoffPercent(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("ResamplerWFIRCutoff"), mpt::saturate_round<int32>(CResamplerSettings().gdWFIRCutoff * 100.0))
	, ResamplerEmulateAmiga(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("ResamplerEmulateAmiga"), true)
	, SoundBoostedThreadPriority(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("BoostedThreadPriority"), SoundDevice::AppInfo().BoostedThreadPriorityXP)
	, SoundBoostedThreadMMCSSClass(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("BoostedThreadMMCSSClass"), SoundDevice::AppInfo().BoostedThreadMMCSSClassVista)
	, SoundBoostedThreadRealtimePosix(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("BoostedThreadRealtimeLinux"), SoundDevice::AppInfo().BoostedThreadRealtimePosix)
	, SoundBoostedThreadNicenessPosix(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("BoostedThreadNicenessPosix"), SoundDevice::AppInfo().BoostedThreadNicenessPosix)
	, SoundBoostedThreadRtprioPosix(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("BoostedThreadRtprioLinux"), SoundDevice::AppInfo().BoostedThreadRtprioPosix)
	// MIDI Settings
	, m_nMidiDevice(conf, MPT_USTRING("MIDI Settings"), MPT_USTRING("MidiDevice"), 0)
	, midiDeviceName(conf, MPT_USTRING("MIDI Settings"), MPT_USTRING("MidiDeviceName"), "")
	, m_dwMidiSetup(conf, MPT_USTRING("MIDI Settings"), MPT_USTRING("MidiSetup"), MIDISETUP_RECORDVELOCITY | MIDISETUP_RECORDNOTEOFF | MIDISETUP_TRANSPOSEKEYBOARD | MIDISETUP_MIDITOPLUG)
	, aftertouchBehaviour(conf, MPT_USTRING("MIDI Settings"), MPT_USTRING("AftertouchBehaviour"), atDoNotRecord)
	, midiVelocityAmp(conf, MPT_USTRING("MIDI Settings"), MPT_USTRING("MidiVelocityAmp"), 100)
	, midiIgnoreCCs(conf, MPT_USTRING("MIDI Settings"), MPT_USTRING("IgnoredCCs"), std::bitset<128>())
	, midiImportPatternLen(conf, MPT_USTRING("MIDI Settings"), MPT_USTRING("MidiImportPatLen"), 128)
	, midiImportQuantize(conf, MPT_USTRING("MIDI Settings"), MPT_USTRING("MidiImportQuantize"), 32)
	, midiImportTicks(conf, MPT_USTRING("MIDI Settings"), MPT_USTRING("MidiImportTicks"), 6)
	// Pattern Editor
	, gbLoopSong(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("LoopSong"), true)
	, gnPatternSpacing(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("Spacing"), 0)
	, gbPatternVUMeters(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("VU-Meters"), true)
	, gbPatternPluginNames(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("Plugin-Names"), true)
	, gbPatternRecord(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("Record"), true)
	, patternNoEditPopup(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("NoEditPopup"), false)
	, patternStepCommands(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("EditStepAppliesToCommands"), false)
	, m_dwPatternSetup(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("PatternSetup"), GetDefaultPatternSetup())
	, m_nRowHighlightMeasures(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("RowSpacing"), 16)
	, m_nRowHighlightBeats(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("RowSpacing2"), 4)
	, recordQuantizeRows(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("RecordQuantize"), 0)
	, gnAutoChordWaitTime(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("AutoChordWaitTime"), 60)
	, orderlistMargins(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("DefaultSequenceMargins"), 0)
	, rowDisplayOffset(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("RowDisplayOffset"), 0)
	, patternFont(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("Font"), FontSetting(PATTERNFONT_SMALL, 0))
	, patternFontDot(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("FontDot"), MPT_USTRING("."))
	, effectVisWidth(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("EffectVisWidth"), -1)
	, effectVisHeight(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("EffectVisHeight"), -1)
	, patternAccessibilityFormat(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("AccessibilityFormat"), _T("Row %row%, Channel %channel%, %column_type%: %column_description%"))
	, patternAlwaysDrawWholePatternOnScrollSlow(conf, MPT_USTRING("Pattern Editor"), MPT_USTRING("AlwaysDrawWholePatternOnScrollSlow"), false)
	// Sample Editor
	, m_SampleUndoBufferSize(conf, MPT_USTRING("Sample Editor"), MPT_USTRING("UndoBufferSize"), SampleUndoBufferSize())
	, sampleEditorKeyBehaviour(conf, MPT_USTRING("Sample Editor"), MPT_USTRING("KeyBehaviour"), seNoteOffOnNewKey)
	, m_defaultSampleFormat(conf, MPT_USTRING("Sample Editor"), MPT_USTRING("DefaultFormat"), dfFLAC)
	, sampleEditorDefaultResampler(conf, MPT_USTRING("Sample Editor"), MPT_USTRING("DefaultResampler"), SRCMODE_DEFAULT)
	, m_nFinetuneStep(conf, MPT_USTRING("Sample Editor"), MPT_USTRING("FinetuneStep"), 10)
	, m_FLACCompressionLevel(conf, MPT_USTRING("Sample Editor"), MPT_USTRING("FLACCompressionLevel"), 5)
	, compressITI(conf, MPT_USTRING("Sample Editor"), MPT_USTRING("CompressITI"), true)
	, m_MayNormalizeSamplesOnLoad(conf, MPT_USTRING("Sample Editor"), MPT_USTRING("MayNormalizeSamplesOnLoad"), true)
	, previewInFileDialogs(conf, MPT_USTRING("Sample Editor"), MPT_USTRING("PreviewInFileDialogs"), false)
	, cursorPositionInHex(conf, MPT_USTRING("Sample Editor"), MPT_USTRING("CursorPositionInHex"), false)
	// Export
	, ExportDefaultToSoundcardSamplerate(conf, MPT_USTRING("Export"), MPT_USTRING("DefaultToSoundcardSamplerate"), true)
	, ExportStreamEncoderSettings(conf, MPT_USTRING("Export"))
	// Components
	, ComponentsLoadOnStartup(conf, MPT_USTRING("Components"), MPT_USTRING("LoadOnStartup"), ComponentManagerSettingsDefault().LoadOnStartup())
	, ComponentsKeepLoaded(conf, MPT_USTRING("Components"), MPT_USTRING("KeepLoaded"), ComponentManagerSettingsDefault().KeepLoaded())
	// AutoSave
	, CreateBackupFiles(conf, MPT_USTRING("AutoSave"), MPT_USTRING("CreateBackupFiles"), true)
	, AutosaveEnabled(conf, MPT_USTRING("AutoSave"), MPT_USTRING("Enabled"), true)
	, AutosaveIntervalMinutes(conf, MPT_USTRING("AutoSave"), MPT_USTRING("IntervalMinutes"), 10)
	, AutosaveHistoryDepth(conf, MPT_USTRING("AutoSave"), MPT_USTRING("BackupHistory"), 3)
	, AutosaveUseOriginalPath(conf, MPT_USTRING("AutoSave"), MPT_USTRING("UseOriginalPath"), true)
	, AutosavePath(conf, MPT_USTRING("AutoSave"), MPT_USTRING("Path"), mpt::GetTempDirectory())
	// Paths
	, PathSongs(conf, MPT_USTRING("Paths"), MPT_USTRING("Songs_Directory"), mpt::PathString())
	, PathSamples(conf, MPT_USTRING("Paths"), MPT_USTRING("Samples_Directory"), mpt::PathString())
	, PathInstruments(conf, MPT_USTRING("Paths"), MPT_USTRING("Instruments_Directory"), mpt::PathString())
	, PathPlugins(conf, MPT_USTRING("Paths"), MPT_USTRING("Plugins_Directory"), mpt::PathString())
	, PathPluginPresets(conf, MPT_USTRING("Paths"), MPT_USTRING("Plugin_Presets_Directory"), mpt::PathString())
	, PathExport(conf, MPT_USTRING("Paths"), MPT_USTRING("Export_Directory"), mpt::PathString())
	, PathTunings(theApp.GetConfigPath() + MPT_PATHSTRING("tunings\\"))
	, PathUserTemplates(theApp.GetConfigPath() + MPT_PATHSTRING("TemplateModules\\"))
	// Default template
	, defaultTemplateFile(conf, MPT_USTRING("Paths"), MPT_USTRING("DefaultTemplate"), mpt::PathString())
	, defaultArtist(conf, MPT_USTRING("Misc"), MPT_USTRING("DefaultArtist"), mpt::ToUnicode(mpt::CharsetLocale, mpt::getenv("USERNAME")))
	// MRU List
	, mruListLength(conf, MPT_USTRING("Misc"), MPT_USTRING("MRUListLength"), 10)
	// Plugins
	, bridgeAllPlugins(conf, MPT_USTRING("VST Plugins"), MPT_USTRING("BridgeAllPlugins"), false)
	, enableAutoSuspend(conf, MPT_USTRING("VST Plugins"), MPT_USTRING("EnableAutoSuspend"), false)
	, midiMappingInPluginEditor(conf, MPT_USTRING("VST Plugins"), MPT_USTRING("EnableMidiMappingInEditor"), true)
	, pluginProjectPath(conf, MPT_USTRING("VST Plugins"), MPT_USTRING("ProjectPath"), mpt::ustring())
	, vstHostProductString(conf, MPT_USTRING("VST Plugins"), MPT_USTRING("HostProductString"), "OpenMPT")
	, vstHostVendorString(conf, MPT_USTRING("VST Plugins"), MPT_USTRING("HostVendorString"), "OpenMPT project")
	, vstHostVendorVersion(conf, MPT_USTRING("VST Plugins"), MPT_USTRING("HostVendorVersion"), Version::Current().GetRawVersion())
	// Update
	, UpdateEnabled(conf, MPT_USTRING("Update"), MPT_USTRING("Enabled"), true)
	, UpdateLastUpdateCheck(conf, MPT_USTRING("Update"), MPT_USTRING("LastUpdateCheck"), mpt::Date::Unix(time_t()))
	, UpdateUpdateCheckPeriod_DEPRECATED(conf, MPT_USTRING("Update"), MPT_USTRING("UpdateCheckPeriod"), 7)
	, UpdateIntervalDays(conf, MPT_USTRING("Update"), MPT_USTRING("UpdateCheckIntervalDays"), 7)
	, UpdateChannel(conf, MPT_USTRING("Update"), MPT_USTRING("Channel"), UpdateChannelRelease)
	, UpdateUpdateURL_DEPRECATED(conf, MPT_USTRING("Update"), MPT_USTRING("UpdateURL"), CUpdateCheck::GetDefaultChannelReleaseURL())
	, UpdateChannelReleaseURL(conf, MPT_USTRING("Update"), MPT_USTRING("ChannelReleaseURL"), CUpdateCheck::GetDefaultChannelReleaseURL())
	, UpdateChannelNextURL(conf, MPT_USTRING("Update"), MPT_USTRING("ChannelStableURL"), CUpdateCheck::GetDefaultChannelNextURL())
	, UpdateChannelDevelopmentURL(conf, MPT_USTRING("Update"), MPT_USTRING("ChannelDevelopmentURL"), CUpdateCheck::GetDefaultChannelDevelopmentURL())
	, UpdateAPIURL(conf, MPT_USTRING("Update"), MPT_USTRING("APIURL"), CUpdateCheck::GetDefaultAPIURL())
	, UpdateStatisticsConsentAsked(conf, MPT_USTRING("Update"), MPT_USTRING("StatistisConsentAsked"), false)
	, UpdateStatistics(conf, MPT_USTRING("Update"), MPT_USTRING("Statistis"), false)
	, UpdateSendGUID_DEPRECATED(conf, MPT_USTRING("Update"), MPT_USTRING("SendGUID"), false)
	, UpdateShowUpdateHint(conf, MPT_USTRING("Update"), MPT_USTRING("ShowUpdateHint"), true)
	, UpdateSuggestDifferentBuildVariant(conf, MPT_USTRING("Update"), MPT_USTRING("SuggestDifferentBuildVariant"), true)
	, UpdateIgnoreVersion(conf, MPT_USTRING("Update"), MPT_USTRING("IgnoreVersion"), _T(""))
	// Wine suppport
	, WineSupportEnabled(conf, MPT_USTRING("WineSupport"), MPT_USTRING("Enabled"), false)
	, WineSupportAlwaysRecompile(conf, MPT_USTRING("WineSupport"), MPT_USTRING("AlwaysRecompile"), false)
	, WineSupportAskCompile(conf, MPT_USTRING("WineSupport"), MPT_USTRING("AskCompile"), false)
	, WineSupportCompileVerbosity(conf, MPT_USTRING("WineSupport"), MPT_USTRING("CompileVerbosity"), 2) // 0=silent 1=silentmake 2=progresswindow 3=standard 4=verbosemake 5=veryverbosemake 6=msgboxes
	, WineSupportForeignOpenMPT(conf, MPT_USTRING("WineSupport"), MPT_USTRING("ForeignOpenMPT"), false)
	, WineSupportAllowUnknownHost(conf, MPT_USTRING("WineSupport"), MPT_USTRING("AllowUnknownHost"), false)
	, WineSupportEnablePulseAudio(conf, MPT_USTRING("WineSupport"), MPT_USTRING("EnablePulseAudio"), 1)
	, WineSupportEnablePortAudio(conf, MPT_USTRING("WineSupport"), MPT_USTRING("EnablePortAudio"), 1)
	, WineSupportEnableRtAudio(conf, MPT_USTRING("WineSupport"), MPT_USTRING("EnableRtAudio"), 1)
{

	// Effects
#ifndef NO_DSP
	m_MegaBassSettings.m_nXBassDepth = conf.Read<int32>(MPT_USTRING("Effects"), MPT_USTRING("XBassDepth"), m_MegaBassSettings.m_nXBassDepth);
	m_MegaBassSettings.m_nXBassRange = conf.Read<int32>(MPT_USTRING("Effects"), MPT_USTRING("XBassRange"), m_MegaBassSettings.m_nXBassRange);
#endif
#ifndef NO_REVERB
	m_ReverbSettings.m_nReverbDepth = conf.Read<int32>(MPT_USTRING("Effects"), MPT_USTRING("ReverbDepth"), m_ReverbSettings.m_nReverbDepth);
	m_ReverbSettings.m_nReverbType = conf.Read<int32>(MPT_USTRING("Effects"), MPT_USTRING("ReverbType"), m_ReverbSettings.m_nReverbType);
#endif
#ifndef NO_DSP
	m_SurroundSettings.m_nProLogicDepth = conf.Read<int32>(MPT_USTRING("Effects"), MPT_USTRING("ProLogicDepth"), m_SurroundSettings.m_nProLogicDepth);
	m_SurroundSettings.m_nProLogicDelay = conf.Read<int32>(MPT_USTRING("Effects"), MPT_USTRING("ProLogicDelay"), m_SurroundSettings.m_nProLogicDelay);
#endif
#ifndef NO_EQ
	m_EqSettings = conf.Read<EQPreset>(MPT_USTRING("Effects"), MPT_USTRING("EQ_Settings"), FlatEQPreset);
	const EQPreset userPresets[] =
	{
		FlatEQPreset,																// User1
		{ "User 1",	{16,16,16,16,16,16}, { 150, 350, 700, 1500, 4500, 8000 } },		// User2
		{ "User 2",	{16,16,16,16,16,16}, { 200, 400, 800, 1750, 5000, 9000 } },		// User3
		{ "User 3",	{16,16,16,16,16,16}, { 250, 450, 900, 2000, 5000, 10000 } }		// User4
	};

	m_EqUserPresets[0] = conf.Read<EQPreset>(MPT_USTRING("Effects"), MPT_USTRING("EQ_User1"), userPresets[0]);
	m_EqUserPresets[1] = conf.Read<EQPreset>(MPT_USTRING("Effects"), MPT_USTRING("EQ_User2"), userPresets[1]);
	m_EqUserPresets[2] = conf.Read<EQPreset>(MPT_USTRING("Effects"), MPT_USTRING("EQ_User3"), userPresets[2]);
	m_EqUserPresets[3] = conf.Read<EQPreset>(MPT_USTRING("Effects"), MPT_USTRING("EQ_User4"), userPresets[3]);
#endif
	// Display (Colors)
	GetDefaultColourScheme(rgbCustomColors);
	for(int ncol = 0; ncol < MAX_MODCOLORS; ncol++)
	{
		const mpt::ustring colorName = mpt::format(MPT_USTRING("Color%1"))(mpt::ufmt::dec0<2>(ncol));
		rgbCustomColors[ncol] = conf.Read<uint32>(MPT_USTRING("Display"), colorName, rgbCustomColors[ncol]);
	}
	// Paths
	m_szKbdFile = conf.Read<mpt::PathString>(MPT_USTRING("Paths"), MPT_USTRING("Key_Config_File"), mpt::PathString());
	conf.Forget(MPT_USTRING("Paths"), MPT_USTRING("Key_Config_File"));

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

	PatternClipboard::SetClipboardSize(conf.Read<int32>(MPT_USTRING("Pattern Editor"), MPT_USTRING("NumClipboards"), mpt::saturate_cast<int32>(PatternClipboard::GetClipboardSize())));

	// Chords
	LoadChords(Chords);

	// Zxx Macros
	MIDIMacroConfig macros;
	theApp.GetDefaultMidiMacro(macros);
	for(int isfx = 0; isfx < 16; isfx++)
	{
		mpt::String::Copy(macros.szMidiSFXExt[isfx], conf.Read<std::string>(MPT_USTRING("Zxx Macros"), mpt::format(MPT_USTRING("SF%1"))(mpt::ufmt::HEX(isfx)), macros.szMidiSFXExt[isfx]));
	}
	for(int izxx = 0; izxx < 128; izxx++)
	{
		mpt::String::Copy(macros.szMidiZXXExt[izxx], conf.Read<std::string>(MPT_USTRING("Zxx Macros"), mpt::format(MPT_USTRING("Z%1"))(mpt::ufmt::HEX0<2>(izxx | 0x80)), macros.szMidiZXXExt[izxx]));
	}


	// MRU list
	Limit(mruListLength, 0u, 32u);
	mruFiles.reserve(mruListLength);
	for(uint32 i = 0; i < mruListLength; i++)
	{
		mpt::ustring key = mpt::format(MPT_USTRING("File%1"))(i);

		mpt::PathString path = theApp.RelativePathToAbsolute(conf.Read<mpt::PathString>(MPT_USTRING("Recent File List"), key, mpt::PathString()));
		if(!path.empty())
		{
			mruFiles.push_back(path);
		}
	}

	// Fixups:
	// -------

	const Version storedVersion = PreviousSettingsVersion;

	// Version
	if(!VersionInstallGUID.Get().IsValid())
	{
		// No UUID found - generate one.
		VersionInstallGUID = mpt::UUID::Generate();
	}

	// Plugins
	if(storedVersion < MAKE_VERSION_NUMERIC(1,19,03,01) && vstHostProductString.Get() == "OpenMPT")
	{
		vstHostVendorVersion = Version::Current().GetRawVersion();
	}

	// Sound Settings
	if(storedVersion < MAKE_VERSION_NUMERIC(1,22,07,30))
	{
		if(conf.Read<bool>(MPT_USTRING("Sound Settings"), MPT_USTRING("KeepDeviceOpen"), false))
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
		m_SoundDeviceID_DEPRECATED = conf.Read<SoundDevice::Legacy::ID>(MPT_USTRING("Sound Settings"), MPT_USTRING("WaveDevice"), SoundDevice::Legacy::ID());
		Setting<uint32> m_BufferLength_DEPRECATED(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("BufferLength"), 50);
		Setting<uint32> m_LatencyMS(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("Latency"), mpt::saturate_round<int32>(SoundDevice::Settings().Latency * 1000.0));
		Setting<uint32> m_UpdateIntervalMS(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("UpdateInterval"), mpt::saturate_round<int32>(SoundDevice::Settings().UpdateInterval * 1000.0));
		Setting<SampleFormat> m_SampleFormat(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("BitsPerSample"), SoundDevice::Settings().sampleFormat);
		Setting<bool> m_SoundDeviceExclusiveMode(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("ExclusiveMode"), SoundDevice::Settings().ExclusiveMode);
		Setting<bool> m_SoundDeviceBoostThreadPriority(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("BoostThreadPriority"), SoundDevice::Settings().BoostThreadPriority);
		Setting<bool> m_SoundDeviceUseHardwareTiming(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("UseHardwareTiming"), SoundDevice::Settings().UseHardwareTiming);
		Setting<SoundDevice::ChannelMapping> m_SoundDeviceChannelMapping(conf, MPT_USTRING("Sound Settings"), MPT_USTRING("ChannelMapping"), SoundDevice::Settings().Channels);
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
			m_SoundDeviceChannelMapping = SoundDevice::ChannelMapping::BaseChannel(MixerOutputChannels, conf.Read<int>(MPT_USTRING("Sound Settings"), MPT_USTRING("ASIOBaseChannel"), 0));
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
	if(storedVersion < MAKE_VERSION_NUMERIC(1,28,00,41))
	{
		// reset this setting to the default when updating,
		// because we do not provide a GUI any more,
		// and in general, it should not get changed anyway
		ResamplerCutoffPercent = mpt::saturate_round<int32>(CResamplerSettings().gdWFIRCutoff * 100.0);
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
		settings.SetVolumeRampUpSamples(conf.Read<int32>(MPT_USTRING("Sound Settings"), MPT_USTRING("VolumeRampSamples"), 42));
		settings.SetVolumeRampDownSamples(conf.Read<int32>(MPT_USTRING("Sound Settings"), MPT_USTRING("VolumeRampSamples"), 42));
		SetMixerSettings(settings);
		conf.Remove(MPT_USTRING("Sound Settings"), MPT_USTRING("VolumeRampSamples"));
	} else if(storedVersion < MAKE_VERSION_NUMERIC(1,22,07,18))
	{
		MixerSettings settings = GetMixerSettings();
		settings.SetVolumeRampUpSamples(conf.Read<int32>(MPT_USTRING("Sound Settings"), MPT_USTRING("VolumeRampUpSamples"), MixerSettings().GetVolumeRampUpSamples()));
		settings.SetVolumeRampDownSamples(conf.Read<int32>(MPT_USTRING("Sound Settings"), MPT_USTRING("VolumeRampDownSamples"), MixerSettings().GetVolumeRampDownSamples()));
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
		commentsFont = FontSetting(MPT_USTRING("Courier New"), (m_dwPatternSetup & 0x02) ? 120 : 90);
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
	if(storedVersion < MAKE_VERSION_NUMERIC(1, 27, 00, 51))
	{
		// Moving option out of pattern config
		CreateBackupFiles = (m_dwPatternSetup & 0x200) != 0;
		m_dwPatternSetup &= ~0x200;
	}

	// Update
	if(storedVersion < MAKE_VERSION_NUMERIC(1,28,00,39))
	{
		if(UpdateUpdateCheckPeriod_DEPRECATED <= 0)
		{
			UpdateEnabled = true;
			UpdateIntervalDays = -1;
		} else
		{
			UpdateEnabled = true;
			UpdateIntervalDays = UpdateUpdateCheckPeriod_DEPRECATED.Get();
		}
		if(UpdateUpdateURL_DEPRECATED.Get() == MPT_USTRING(""))
		{
			UpdateChannel = UpdateChannelRelease;
		} else if(UpdateUpdateURL_DEPRECATED.Get() == MPT_USTRING("http://update.openmpt.org/check/$VERSION/$GUID"))
		{
			UpdateChannel = UpdateChannelRelease;
		} else if(UpdateUpdateURL_DEPRECATED.Get() == MPT_USTRING("https://update.openmpt.org/check/$VERSION/$GUID"))
		{
			UpdateChannel = UpdateChannelRelease;
		} else if(UpdateUpdateURL_DEPRECATED.Get() == MPT_USTRING("http://update.openmpt.org/check/testing/$VERSION/$GUID"))
		{
			UpdateChannel = UpdateChannelDevelopment;
		} else if(UpdateUpdateURL_DEPRECATED.Get() == MPT_USTRING("https://update.openmpt.org/check/testing/$VERSION/$GUID"))
		{
			UpdateChannel = UpdateChannelDevelopment;
		} else
		{
			UpdateChannel = UpdateChannelDevelopment;
			UpdateChannelDevelopmentURL = UpdateUpdateURL_DEPRECATED.Get();
		}
		UpdateStatistics = UpdateSendGUID_DEPRECATED.Get();
		conf.Forget(UpdateUpdateCheckPeriod_DEPRECATED.GetPath());
		conf.Forget(UpdateUpdateURL_DEPRECATED.GetPath());
		conf.Forget(UpdateSendGUID_DEPRECATED.GetPath());
	}

	// Effects
#ifndef NO_EQ
	FixupEQ(m_EqSettings);
	FixupEQ(m_EqUserPresets[0]);
	FixupEQ(m_EqUserPresets[1]);
	FixupEQ(m_EqUserPresets[2]);
	FixupEQ(m_EqUserPresets[3]);
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

	// Sanitize resampling mode for sample editor
	if(!Resampling::IsKnownMode(sampleEditorDefaultResampler) && sampleEditorDefaultResampler != SRCMODE_DEFAULT)
	{
		sampleEditorDefaultResampler = SRCMODE_DEFAULT;
	}

	// Migrate Tuning data
	MigrateTunings(storedVersion);

	// Sanitize MIDI import data
	if(midiImportPatternLen < 1 || midiImportPatternLen > MAX_PATTERN_ROWS)
		midiImportPatternLen = 128;
	if(midiImportQuantize < 4 || midiImportQuantize > 256)
		midiImportQuantize = 32;
	if(midiImportTicks < 2 || midiImportTicks > 16)
		midiImportTicks = 16;

	// Last fixup: update config version
	IniVersion = mpt::ufmt::val(Version::Current());

	// Write updated settings
	conf.Flush();

}


TrackerSettings::~TrackerSettings()
{
	return;
}


void TrackerSettings::MigrateOldSoundDeviceSettings(SoundDevice::Manager &manager)
{
	if(m_SoundDeviceSettingsUseOldDefaults)
	{
		// get the old default device
		SetSoundDeviceIdentifier(SoundDevice::Legacy::FindDeviceInfo(manager, m_SoundDeviceID_DEPRECATED).GetIdentifier());
		// apply old global sound device settings to each found device
		for(const auto &it : manager)
		{
			SetSoundDeviceSettings(it.GetIdentifier(), GetSoundDeviceSettingsDefaults());
		}
	}
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
				const mpt::ustring newIdentifierW = newIdentifier + MPT_USTRING("_");
				const mpt::ustring oldIdentifierW = oldIdentifier + MPT_USTRING("_");
				conf.Write(MPT_USTRING("Sound Settings"), newIdentifierW + MPT_USTRING("Latency"),
					conf.Read(MPT_USTRING("Sound Settings"), oldIdentifierW + MPT_USTRING("Latency"), mpt::saturate_round<int32>(defaults.Latency * 1000000.0)));
				conf.Write(MPT_USTRING("Sound Settings"), newIdentifierW + MPT_USTRING("UpdateInterval"),
					conf.Read(MPT_USTRING("Sound Settings"), oldIdentifierW + MPT_USTRING("UpdateInterval"), mpt::saturate_round<int32>(defaults.UpdateInterval * 1000000.0)));
				conf.Write(MPT_USTRING("Sound Settings"), newIdentifierW + MPT_USTRING("SampleRate"),
					conf.Read(MPT_USTRING("Sound Settings"), oldIdentifierW + MPT_USTRING("SampleRate"), defaults.Samplerate));
				conf.Write(MPT_USTRING("Sound Settings"), newIdentifierW + MPT_USTRING("Channels"),
					conf.Read(MPT_USTRING("Sound Settings"), oldIdentifierW + MPT_USTRING("Channels"), defaults.Channels.GetNumHostChannels()));
				conf.Write(MPT_USTRING("Sound Settings"), newIdentifierW + MPT_USTRING("SampleFormat"),
					conf.Read(MPT_USTRING("Sound Settings"), oldIdentifierW + MPT_USTRING("SampleFormat"), defaults.sampleFormat));
				conf.Write(MPT_USTRING("Sound Settings"), newIdentifierW + MPT_USTRING("ExclusiveMode"),
					conf.Read(MPT_USTRING("Sound Settings"), oldIdentifierW + MPT_USTRING("ExclusiveMode"), defaults.ExclusiveMode));
				conf.Write(MPT_USTRING("Sound Settings"), newIdentifierW + MPT_USTRING("BoostThreadPriority"),
					conf.Read(MPT_USTRING("Sound Settings"), oldIdentifierW + MPT_USTRING("BoostThreadPriority"), defaults.BoostThreadPriority));
				conf.Write(MPT_USTRING("Sound Settings"), newIdentifierW + MPT_USTRING("KeepDeviceRunning"),
					conf.Read(MPT_USTRING("Sound Settings"), oldIdentifierW + MPT_USTRING("KeepDeviceRunning"), defaults.KeepDeviceRunning));
				conf.Write(MPT_USTRING("Sound Settings"), newIdentifierW + MPT_USTRING("UseHardwareTiming"),
					conf.Read(MPT_USTRING("Sound Settings"), oldIdentifierW + MPT_USTRING("UseHardwareTiming"), defaults.UseHardwareTiming));
				conf.Write(MPT_USTRING("Sound Settings"), newIdentifierW + MPT_USTRING("DitherType"),
					conf.Read(MPT_USTRING("Sound Settings"), oldIdentifierW + MPT_USTRING("DitherType"), defaults.DitherType));
				conf.Write(MPT_USTRING("Sound Settings"), newIdentifierW + MPT_USTRING("ChannelMapping"),
					conf.Read(MPT_USTRING("Sound Settings"), oldIdentifierW + MPT_USTRING("ChannelMapping"), defaults.Channels));
			}
		}
	}
}


void TrackerSettings::MigrateTunings(const Version storedVersion)
{
	
	if(!PathTunings.GetDefaultDir().IsDirectory())
	{
		CreateDirectory(PathTunings.GetDefaultDir().AsNative().c_str(), 0);
	}
	if(!(PathTunings.GetDefaultDir() + MPT_PATHSTRING("Built-in\\")).IsDirectory())
	{
		CreateDirectory((PathTunings.GetDefaultDir() + MPT_PATHSTRING("Built-in\\")).AsNative().c_str(), 0);
	}
	if(!(PathTunings.GetDefaultDir() + MPT_PATHSTRING("Locale\\")).IsDirectory())
	{
		CreateDirectory((PathTunings.GetDefaultDir() + MPT_PATHSTRING("Local\\")).AsNative().c_str(), 0);
	}
	{
		mpt::PathString fn = PathTunings.GetDefaultDir() + MPT_PATHSTRING("Built-in\\12TET.tun");
		if(!fn.FileOrDirectoryExists())
		{
			CTuning * pT = CSoundFile::CreateTuning12TET("12TET");
			mpt::SafeOutputFile f(fn, std::ios::binary, mpt::FlushMode::Full);
			pT->Serialize(f);
			f.close();
			delete pT;
			pT = nullptr;
		}
	}
	{
		mpt::PathString fn = PathTunings.GetDefaultDir() + MPT_PATHSTRING("Built-in\\12TET [[fs15 1.17.02.49]].tun");
		if(!fn.FileOrDirectoryExists())
		{
			CTuning * pT = CSoundFile::CreateTuning12TET("12TET [[fs15 1.17.02.49]]");
			mpt::SafeOutputFile f(fn, std::ios::binary, mpt::FlushMode::Full);
			pT->Serialize(f);
			f.close();
			delete pT;
			pT = nullptr;
		}
	}
	oldLocalTunings = LoadLocalTunings();
	if(storedVersion < MAKE_VERSION_NUMERIC(1,27,00,56))
	{
		UnpackTuningCollection(*oldLocalTunings, PathTunings.GetDefaultDir() + MPT_PATHSTRING("Local\\"));
	}
}


std::unique_ptr<CTuningCollection> TrackerSettings::LoadLocalTunings()
{
	std::unique_ptr<CTuningCollection> s_pTuningsSharedLocal = mpt::make_unique<CTuningCollection>();
	mpt::ifstream f(
			PathTunings.GetDefaultDir()
			+ MPT_PATHSTRING("local_tunings")
			+ mpt::PathString::FromUTF8(CTuningCollection::s_FileExtension)
		, std::ios::binary);
	if(f.good())
	{
		std::string dummyName;
		s_pTuningsSharedLocal->Deserialize(f, dummyName);
	}
	return s_pTuningsSharedLocal;
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
		, LatencyUS(conf, MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("Latency"), mpt::saturate_round<int32>(defaults.Latency * 1000000.0))
		, UpdateIntervalUS(conf, MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("UpdateInterval"), mpt::saturate_round<int32>(defaults.UpdateInterval * 1000000.0))
		, Samplerate(conf, MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("SampleRate"), defaults.Samplerate)
		, ChannelsOld(conf, MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("Channels"), mpt::saturate_cast<uint8>((int)defaults.Channels))
		, ChannelMapping(conf, MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("ChannelMapping"), defaults.Channels)
		, sampleFormat(conf, MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("SampleFormat"), defaults.sampleFormat)
		, ExclusiveMode(conf, MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("ExclusiveMode"), defaults.ExclusiveMode)
		, BoostThreadPriority(conf, MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("BoostThreadPriority"), defaults.BoostThreadPriority)
		, KeepDeviceRunning(conf, MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("KeepDeviceRunning"), defaults.KeepDeviceRunning)
		, UseHardwareTiming(conf, MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("UseHardwareTiming"), defaults.UseHardwareTiming)
		, DitherType(conf, MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("DitherType"), defaults.DitherType)
	{
		if(ChannelMapping.Get().GetNumHostChannels() != ChannelsOld)
		{
			// If the stored channel count and the count of channels used in the channel mapping do not match,
			// construct a default mapping from the channel count.
			ChannelMapping = SoundDevice::ChannelMapping(ChannelsOld);
		}
		// store informational data (not read back, just to allow the user to mock with the raw ini file)
		conf.Write(MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("Type"), deviceInfo.type);
		conf.Write(MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("InternalID"), deviceInfo.internalID);
		conf.Write(MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("API"), deviceInfo.apiName);
		conf.Write(MPT_USTRING("Sound Settings"), deviceInfo.GetIdentifier() + MPT_USTRING("_") + MPT_USTRING("Name"), deviceInfo.name);
	}

	StoredSoundDeviceSettings & operator = (const SoundDevice::Settings &settings)
	{
		LatencyUS = mpt::saturate_round<int32>(settings.Latency * 1000000.0);
		UpdateIntervalUS = mpt::saturate_round<int32>(settings.UpdateInterval * 1000000.0);
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
{
	return m_SoundDeviceSettingsDefaults;
}

SoundDevice::Identifier TrackerSettings::GetSoundDeviceIdentifier() const
{
	return m_SoundDeviceIdentifier;
}

void TrackerSettings::SetSoundDeviceIdentifier(const SoundDevice::Identifier &identifier)
{
	m_SoundDeviceIdentifier = identifier;
}

SoundDevice::Settings TrackerSettings::GetSoundDeviceSettings(const SoundDevice::Identifier &device) const
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
{
	CResamplerSettings settings;
	settings.SrcMode = ResamplerMode;
	settings.gbWFIRType = ResamplerSubMode;
	settings.gdWFIRCutoff = ResamplerCutoffPercent * 0.01;
	settings.emulateAmiga = ResamplerEmulateAmiga;
	return settings;
}

void TrackerSettings::SetResamplerSettings(const CResamplerSettings &settings)
{
	ResamplerMode = settings.SrcMode;
	ResamplerSubMode = settings.gbWFIRType;
	ResamplerCutoffPercent = mpt::saturate_round<int32>(settings.gdWFIRCutoff * 100.0);
	ResamplerEmulateAmiga = settings.emulateAmiga;
}


void TrackerSettings::GetDefaultColourScheme(std::array<COLORREF, MAX_MODCOLORS> &colours)
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
	// VU-Meters
	colours[MODCOLOR_VUMETER_LO] = RGB(0x00, 0xC8, 0x00);
	colours[MODCOLOR_VUMETER_MED] = RGB(0xFF, 0xC8, 0x00);
	colours[MODCOLOR_VUMETER_HI] = RGB(0xE1, 0x00, 0x00);
	colours[MODCOLOR_VUMETER_LO_VST] = RGB(0x18, 0x96, 0xE1);
	colours[MODCOLOR_VUMETER_MED_VST] = RGB(0xFF, 0xC8, 0x00);
	colours[MODCOLOR_VUMETER_HI_VST] = RGB(0xE1, 0x00, 0x00);
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
	colours[MODCOLOR_ENVELOPES] = RGB(0x00, 0x00, 0xFF);
	colours[MODCOLOR_ENVELOPE_RELEASE] = RGB(0xFF, 0xFF, 0x00);
}


void TrackerSettings::FixupEQ(EQPreset &eqSettings)
{
	for(UINT i=0; i<MAX_EQ_BANDS; i++)
	{
		if(eqSettings.Gains[i] > 32)
			eqSettings.Gains[i] = 16;
		if((eqSettings.Freqs[i] < 100) || (eqSettings.Freqs[i] > 10000))
			eqSettings.Freqs[i] = FlatEQPreset.Freqs[i];
	}
	mpt::String::SetNullTerminator(eqSettings.szName);
}


void TrackerSettings::SaveSettings()
{

	WINDOWPLACEMENT wpl;
	wpl.length = sizeof(WINDOWPLACEMENT);
	CMainFrame::GetMainFrame()->GetWindowPlacement(&wpl);
	conf.Write<WINDOWPLACEMENT>(MPT_USTRING("Display"), MPT_USTRING("WindowPlacement"), wpl);

	conf.Write<int32>(MPT_USTRING("Pattern Editor"), MPT_USTRING("NumClipboards"), mpt::saturate_cast<int32>(PatternClipboard::GetClipboardSize()));

	// Effects
#ifndef NO_DSP
	conf.Write<int32>(MPT_USTRING("Effects"), MPT_USTRING("XBassDepth"), m_MegaBassSettings.m_nXBassDepth);
	conf.Write<int32>(MPT_USTRING("Effects"), MPT_USTRING("XBassRange"), m_MegaBassSettings.m_nXBassRange);
#endif
#ifndef NO_REVERB
	conf.Write<int32>(MPT_USTRING("Effects"), MPT_USTRING("ReverbDepth"), m_ReverbSettings.m_nReverbDepth);
	conf.Write<int32>(MPT_USTRING("Effects"), MPT_USTRING("ReverbType"), m_ReverbSettings.m_nReverbType);
#endif
#ifndef NO_DSP
	conf.Write<int32>(MPT_USTRING("Effects"), MPT_USTRING("ProLogicDepth"), m_SurroundSettings.m_nProLogicDepth);
	conf.Write<int32>(MPT_USTRING("Effects"), MPT_USTRING("ProLogicDelay"), m_SurroundSettings.m_nProLogicDelay);
#endif
#ifndef NO_EQ
	conf.Write<EQPreset>(MPT_USTRING("Effects"), MPT_USTRING("EQ_Settings"), m_EqSettings);
	conf.Write<EQPreset>(MPT_USTRING("Effects"), MPT_USTRING("EQ_User1"), m_EqUserPresets[0]);
	conf.Write<EQPreset>(MPT_USTRING("Effects"), MPT_USTRING("EQ_User2"), m_EqUserPresets[1]);
	conf.Write<EQPreset>(MPT_USTRING("Effects"), MPT_USTRING("EQ_User3"), m_EqUserPresets[2]);
	conf.Write<EQPreset>(MPT_USTRING("Effects"), MPT_USTRING("EQ_User4"), m_EqUserPresets[3]);
#endif

	// Display (Colors)
	for(int ncol = 0; ncol < MAX_MODCOLORS; ncol++)
	{
		conf.Write<uint32>(MPT_USTRING("Display"), mpt::format(MPT_USTRING("Color%1"))(mpt::ufmt::dec0<2>(ncol)), rgbCustomColors[ncol]);
	}

	// Paths
	// Obsolete, since we always write to Keybindings.mkb now.
	// Older versions of OpenMPT 1.18+ will look for this file if this entry is missing, so removing this entry after having read it is kind of backwards compatible.
	conf.Remove(MPT_USTRING("Paths"), MPT_USTRING("Key_Config_File"));

	// Chords
	SaveChords(Chords);

	// Save default macro configuration
	MIDIMacroConfig macros;
	theApp.GetDefaultMidiMacro(macros);
	for(int isfx = 0; isfx < 16; isfx++)
	{
		conf.Write<std::string>(MPT_USTRING("Zxx Macros"), mpt::format(MPT_USTRING("SF%1"))(mpt::ufmt::HEX(isfx)), macros.szMidiSFXExt[isfx]);
	}
	for(int izxx = 0; izxx < 128; izxx++)
	{
		conf.Write<std::string>(MPT_USTRING("Zxx Macros"), mpt::format(MPT_USTRING("Z%1"))(mpt::ufmt::HEX0<2>(izxx | 0x80)), macros.szMidiZXXExt[izxx]);
	}

	// MRU list
	for(uint32 i = 0; i < (ID_MRU_LIST_LAST - ID_MRU_LIST_FIRST + 1); i++)
	{
		mpt::ustring key = mpt::format(MPT_USTRING("File%1"))(i);

		if(i < mruFiles.size())
		{
			mpt::PathString path = mruFiles[i];
			if(theApp.IsPortableMode())
			{
				path = theApp.AbsolutePathToRelative(path);
			}
			conf.Write<mpt::PathString>(MPT_USTRING("Recent File List"), key, path);
		} else
		{
			conf.Remove(MPT_USTRING("Recent File List"), key);
		}
	}
}


bool TrackerSettings::IsComponentBlocked(const std::string &key)
{
	return Setting<bool>(conf, MPT_USTRING("Components"), MPT_USTRING("Block") + mpt::ToUnicode(mpt::CharsetASCII, key), ComponentManagerSettingsDefault().IsBlocked(key));
}


std::vector<uint32> TrackerSettings::GetSampleRates() const
{
	return m_SoundSampleRates;
}


std::vector<uint32> TrackerSettings::GetDefaultSampleRates()
{
	return std::vector<uint32>{
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
}


////////////////////////////////////////////////////////////////////////////////
// Chords

void TrackerSettings::LoadChords(MPTChords &chords)
{	
	for(std::size_t i = 0; i < mpt::size(chords); i++)
	{
		uint32 chord;
		mpt::ustring note = mpt::format(MPT_USTRING("%1%2"))(mpt::ustring(NoteNamesSharp[i % 12]), i / 12);
		if((chord = conf.Read<int32>(MPT_USTRING("Chords"), note, -1)) != uint32(-1))
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
{
	for(std::size_t i = 0; i < mpt::size(chords); i++)
	{
		int32 s = (chords[i].key) | (chords[i].notes[0] << 6) | (chords[i].notes[1] << 12) | (chords[i].notes[2] << 18);
		mpt::ustring note = mpt::format(MPT_USTRING("%1%2"))(mpt::ustring(NoteNamesSharp[i % 12]), i / 12);
		conf.Write<int32>(MPT_USTRING("Chords"), note, s);
	}
}


void TrackerSettings::SetMIDIDevice(UINT id)
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


mpt::ustring IgnoredCCsToString(const std::bitset<128> &midiIgnoreCCs)
{
	mpt::ustring cc;
	bool first = true;
	for(int i = 0; i < 128; i++)
	{
		if(midiIgnoreCCs[i])
		{
			if(!first)
			{
				cc += MPT_USTRING(",");
			}
			cc += mpt::ufmt::val(i);
			first = false;
		}
	}
	return cc;
}


std::bitset<128> StringToIgnoredCCs(const mpt::ustring &in)
{
	CString cc = mpt::ToCString(in);
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
{
	return;
}

DefaultAndWorkingDirectory::DefaultAndWorkingDirectory(const mpt::PathString &def)
	: m_Default(def)
	, m_Working(def)
{
	return;
}

DefaultAndWorkingDirectory::~DefaultAndWorkingDirectory()
{
	return;
}

void DefaultAndWorkingDirectory::SetDefaultDir(const mpt::PathString &filenameFrom, bool stripFilename)
{
	if(InternalSet(m_Default, filenameFrom, stripFilename) && !m_Default.empty())
	{
		// When updating default directory, also update the working directory.
		InternalSet(m_Working, filenameFrom, stripFilename);
	}
}

void DefaultAndWorkingDirectory::SetWorkingDir(const mpt::PathString &filenameFrom, bool stripFilename)
{
	InternalSet(m_Working, filenameFrom, stripFilename);
}

mpt::PathString DefaultAndWorkingDirectory::GetDefaultDir() const
{
	return m_Default;
}

mpt::PathString DefaultAndWorkingDirectory::GetWorkingDir() const
{
	return m_Working;
}

// Retrieve / set default directory from given string and store it our setup variables
// If stripFilename is true, the filenameFrom parameter is assumed to be a full path including a filename.
// Return true if the value changed.
bool DefaultAndWorkingDirectory::InternalSet(mpt::PathString &dest, const mpt::PathString &filenameFrom, bool stripFilename)
{
	mpt::PathString newPath = (stripFilename ? filenameFrom.GetPath() : filenameFrom);
	newPath.EnsureTrailingSlash();
	mpt::PathString oldPath = dest;
	dest = newPath;
	return newPath != oldPath;
}

ConfigurableDirectory::ConfigurableDirectory(SettingsContainer &conf, const AnyStringLocale &section, const AnyStringLocale &key, const mpt::PathString &def)
	: conf(conf)
	, m_Setting(conf, section, key, def)
{
	SetDefaultDir(theApp.RelativePathToAbsolute(m_Setting), false);
}

ConfigurableDirectory::~ConfigurableDirectory()
{
	m_Setting = theApp.IsPortableMode() ? theApp.AbsolutePathToRelative(m_Default) : m_Default;
}



OPENMPT_NAMESPACE_END
