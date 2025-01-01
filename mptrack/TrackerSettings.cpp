/*
 * TrackerSettings.cpp
 * -------------------
 * Purpose: Code for managing, loading and saving all application settings.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "TrackerSettings.h"
#include "ExceptionHandler.h"
#include "HighDPISupport.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Mpdlgs.h"
#include "Mptrack.h"
#include "PatternClipboard.h"
#include "resource.h"
#include "TuningDialog.h"
#include "UpdateCheck.h"
#include "../common/ComponentManager.h"
#include "../common/misc_util.h"
#include "../common/mptFileIO.h"
#include "../common/mptStringBuffer.h"
#include "../common/version.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/Tables.h"
#include "../soundlib/tuningcollection.h"
#include "mpt/environment/environment.hpp"
#include "mpt/fs/common_directories.hpp"
#include "mpt/fs/fs.hpp"
#include "mpt/io_file/outputfile.hpp"
#include "mpt/parse/parse.hpp"
#include "mpt/uuid/uuid.hpp"
#include "openmpt/sounddevice/SoundDevice.hpp"
#include "openmpt/sounddevice/SoundDeviceManager.hpp"

#include <algorithm>


OPENMPT_NAMESPACE_BEGIN


#define OLD_SOUNDSETUP_REVERSESTEREO         0x20
#define OLD_SOUNDSETUP_SECONDARY             0x40
#define OLD_SOUNDSETUP_NOBOOSTTHREADPRIORITY 0x80

#ifndef NO_EQ

constexpr EQPreset FlatEQPreset = {"Flat", {16, 16, 16, 16, 16, 16}, {125, 300, 600, 1250, 4000, 8000}};

#endif // !NO_EQ


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
	if(modtype == MOD_TYPE_MOD_PC)
		return UL_("mod-pc");
	else
		return CSoundFile::GetModSpecifications(modtype).GetFileExtension();
}

MODTYPE SettingsStringToModType(const mpt::ustring &str)
{
	if(str == UL_("mod-pc"))
		return MOD_TYPE_MOD_PC;
	else
		return CModSpecifications::ExtensionToType(str);
}


void SampleUndoBufferSize::CalculateSize()
{
	if(sizePercent < 0)
		sizePercent = 0;
	MEMORYSTATUSEX memStatus;
	memStatus.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memStatus);
	// The setting is a percentage of the memory that's actually *available* to OpenMPT, which is a max of 4GB in 32-bit mode.
	sizeByte = mpt::saturate_cast<size_t>(std::min(memStatus.ullTotalPhys, DWORDLONG(SIZE_T_MAX)) * sizePercent / 100);

	// Pretend there's at least one MiB of memory (haha)
	if(sizePercent != 0 && sizeByte < 1 * 1024 * 1024)
	{
		sizeByte = 1 * 1024 * 1024;
	}
}


DebugSettings::DebugSettings(SettingsContainer &conf_)
	: conf(conf_)
	// Debug
#if !defined(MPT_LOG_IS_DISABLED)
	, DebugLogLevel(conf, UL_("Debug"), UL_("LogLevel"), static_cast<int>(mpt::log::GlobalLogLevel))
	, DebugLogFacilitySolo(conf, UL_("Debug"), UL_("LogFacilitySolo"), std::string())
	, DebugLogFacilityBlocked(conf, UL_("Debug"), UL_("LogFacilityBlocked"), std::string())
	, DebugLogFileEnable(conf, UL_("Debug"), UL_("LogFileEnable"), mpt::log::FileEnabled)
	, DebugLogDebuggerEnable(conf, UL_("Debug"), UL_("LogDebuggerEnable"), mpt::log::DebuggerEnabled)
	, DebugLogConsoleEnable(conf, UL_("Debug"), UL_("LogConsoleEnable"), mpt::log::ConsoleEnabled)
#endif
	, DebugTraceEnable(conf, UL_("Debug"), UL_("TraceEnable"), false)
	, DebugTraceSize(conf, UL_("Debug"), UL_("TraceSize"), 1000000)
	, DebugTraceAlwaysDump(conf, UL_("Debug"), UL_("TraceAlwaysDump"), false)
	, DebugStopSoundDeviceOnCrash(conf, UL_("Debug"), UL_("StopSoundDeviceOnCrash"), true)
	, DebugStopSoundDeviceBeforeDump(conf, UL_("Debug"), UL_("StopSoundDeviceBeforeDump"), false)
	, DebugDelegateToWindowsHandler(conf, UL_("Debug"), UL_("DelegateToWindowsHandler"), false)
{

	// Duplicate state for debug stuff in order to avoid calling into settings framework from crash context.
	ExceptionHandler::stopSoundDeviceOnCrash = DebugStopSoundDeviceOnCrash;
	ExceptionHandler::stopSoundDeviceBeforeDump = DebugStopSoundDeviceBeforeDump;
	ExceptionHandler::delegateToWindowsHandler = DebugDelegateToWindowsHandler;

		// enable debug features (as early as possible after reading the settings)
	#if !defined(MPT_LOG_IS_DISABLED)
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
	, IniVersion(conf, UL_("Version"), UL_("Version"), mpt::ustring())
	, FirstRun(IniVersion.Get() == mpt::ustring())
	, PreviousSettingsVersion(GetPreviousSettingsVersion(IniVersion))
	, VersionInstallGUID(conf, UL_("Version"), UL_("InstallGUID"), mpt::UUID())
	// Display
	, m_ShowSplashScreen(conf, UL_("Display"), UL_("ShowSplashScreen"), true)
	, gbMdiMaximize(conf, UL_("Display"), UL_("MDIMaximize"), true)
	, dpiAwareness(conf, UL_("Display"), UL_("DPIAwareness"), DPIAwarenessMode::PerMonitorDPIAware)
	, glTreeSplitRatio(conf, UL_("Display"), UL_("MDITreeRatio"), 128)
	, glTreeWindowWidth(conf, UL_("Display"), UL_("MDITreeWidth"), 160)
	, glGeneralWindowHeight(conf, UL_("Display"), UL_("MDIGeneralHeight"), 222)
	, glPatternWindowHeight(conf, UL_("Display"), UL_("MDIPatternHeight"), 152)
	, glSampleWindowHeight(conf, UL_("Display"), UL_("MDISampleHeight"), 190)
	, glInstrumentWindowHeight(conf, UL_("Display"), UL_("MDIInstrumentHeight"), 300)
	, glCommentsWindowHeight(conf, UL_("Display"), UL_("MDICommentsHeight"), 288)
	, glGraphWindowHeight(conf, UL_("Display"), UL_("MDIGraphHeight"), 288)
	, gnPlugWindowX(conf, UL_("Display"), UL_("PlugSelectWindowX"), 243)
	, gnPlugWindowY(conf, UL_("Display"), UL_("PlugSelectWindowY"), 273)
	, gnPlugWindowWidth(conf, UL_("Display"), UL_("PlugSelectWindowWidth"), 450)
	, gnPlugWindowHeight(conf, UL_("Display"), UL_("PlugSelectWindowHeight"), 540)
	, lastSelectedPlugin(conf, UL_("Display"), UL_("PlugSelectWindowLast"), {})
	, gnMsgBoxVisiblityFlags(conf, UL_("Display"), UL_("MsgBoxVisibilityFlags"), uint32_max)
	, GUIUpdateInterval(conf, UL_("Display"), UL_("GUIUpdateInterval"), 0)
	, FSUpdateInterval(conf, UL_("Display"), UL_("FSUpdateInterval"), 500)
	, VuMeterUpdateInterval(conf, UL_("Display"), UL_("VuMeterUpdateInterval"), 15)
	, VuMeterDecaySpeedDecibelPerSecond(conf, UL_("Display"), UL_("VuMeterDecaySpeedDecibelPerSecond"), 88.0f)
	, accidentalFlats(conf, UL_("Display"), UL_("AccidentalFlats"), false)
	, rememberSongWindows(conf, UL_("Display"), UL_("RememberSongWindows"), true)
	, showDirsInSampleBrowser(conf, UL_("Display"), UL_("ShowDirsInSampleBrowser"), false)
	, useOldStyleFolderBrowser(conf, UL_("Display"), UL_("UseOldStyleFolderBrowser"), false)
	, defaultRainbowChannelColors(conf, UL_("Display"), UL_("DefaultChannelColors"), DefaultChannelColors::Random)
	, commentsFont(conf, UL_("Display"), UL_("Comments Font"), FontSetting(UL_("Courier New"), 120))
	, mainToolBarVisibleItems(conf, UL_("Display"), UL_("MainToolBarVisibleItems"), MainToolBarItem::Default)
	, treeViewOnLeft(conf, UL_("Display"), UL_("TreeViewOnLeft"), true)
	// Misc
	, defaultModType(conf, UL_("Misc"), UL_("DefaultModType"), MOD_TYPE_IT)
	, defaultNewFileAction(conf, UL_("Misc"), UL_("DefaultNewFileAction"), nfDefaultFormat)
	, DefaultPlugVolumeHandling(conf, UL_("Misc"), UL_("DefaultPlugVolumeHandling"), PLUGIN_VOLUMEHANDLING_IGNORE)
	, autoApplySmoothFT2Ramping(conf, UL_("Misc"), UL_("SmoothFT2Ramping"), false)
	, MiscITCompressionStereo(conf, UL_("Misc"), UL_("ITCompressionStereo"), 4)
	, MiscITCompressionMono(conf, UL_("Misc"), UL_("ITCompressionMono"), 7)
	, MiscSaveChannelMuteStatus(conf, UL_("Misc"), UL_("SaveChannelMuteStatus"), true)
	, MiscAllowMultipleCommandsPerKey(conf, UL_("Misc"), UL_("AllowMultipleCommandsPerKey"), false)
	, MiscDistinguishModifiers(conf, UL_("Misc"), UL_("DistinguishModifiers"), false)
	, MiscProcessPriorityClass(conf, UL_("Misc"), UL_("ProcessPriorityClass"), ProcessPriorityClassNORMAL)
	, MiscFlushFileBuffersOnSave(conf, UL_("Misc"), UL_("FlushFileBuffersOnSave"), true)
	, MiscCacheCompleteFileBeforeLoading(conf, UL_("Misc"), UL_("CacheCompleteFileBeforeLoading"), false)
	, MiscUseSingleInstance(conf, UL_("Misc"), UL_("UseSingleInstance"), false)
	// Sound Settings
	, m_SoundShowRecordingSettings(false)
	, m_SoundShowDeprecatedDevices(conf, UL_("Sound Settings"), UL_("ShowDeprecatedDevices"), false)
	, m_SoundDeprecatedDeviceWarningShown(conf, UL_("Sound Settings"), UL_("DeprecatedDeviceWarningShown"), false)
	, m_SoundSampleRates(conf, UL_("Sound Settings"), UL_("SampleRates"), GetDefaultSampleRates())
	, m_SoundSettingsOpenDeviceAtStartup(conf, UL_("Sound Settings"), UL_("OpenDeviceAtStartup"), false)
	, m_SoundSettingsStopMode(conf, UL_("Sound Settings"), UL_("StopMode"), SoundDeviceStopModeClosed)
	, m_SoundDeviceSettingsUseOldDefaults(false)
	, m_SoundDeviceID_DEPRECATED(SoundDevice::Legacy::ID())
	, m_SoundDeviceIdentifier(conf, UL_("Sound Settings"), UL_("Device"), SoundDevice::Identifier())
	, MixerMaxChannels(conf, UL_("Sound Settings"), UL_("MixChannels"), MixerSettings().m_nMaxMixChannels)
	, MixerDSPMask(conf, UL_("Sound Settings"), UL_("Quality"), MixerSettings().DSPMask)
	, MixerFlags(conf, UL_("Sound Settings"), UL_("SoundSetup"), MixerSettings().MixerFlags)
	, MixerSamplerate(conf, UL_("Sound Settings"), UL_("Mixing_Rate"), MixerSettings().gdwMixingFreq)
	, MixerOutputChannels(conf, UL_("Sound Settings"), UL_("ChannelMode"), MixerSettings().gnChannels)
	, MixerPreAmp(conf, UL_("Sound Settings"), UL_("PreAmp"), MixerSettings().m_nPreAmp)
	, MixerStereoSeparation(conf, UL_("Sound Settings"), UL_("StereoSeparation"), MixerSettings().m_nStereoSeparation)
	, MixerVolumeRampUpMicroseconds(conf, UL_("Sound Settings"), UL_("VolumeRampUpMicroseconds"), MixerSettings().GetVolumeRampUpMicroseconds())
	, MixerVolumeRampDownMicroseconds(conf, UL_("Sound Settings"), UL_("VolumeRampDownMicroseconds"), MixerSettings().GetVolumeRampDownMicroseconds())
	, MixerNumInputChannels(conf, UL_("Sound Settings"), UL_("NumInputChannels"), static_cast<uint32>(MixerSettings().NumInputChannels))
	, ResamplerMode(conf, UL_("Sound Settings"), UL_("SrcMode"), CResamplerSettings().SrcMode)
	, ResamplerSubMode(conf, UL_("Sound Settings"), UL_("XMMSModplugResamplerWFIRType"), CResamplerSettings().gbWFIRType)
	, ResamplerCutoffPercent(conf, UL_("Sound Settings"), UL_("ResamplerWFIRCutoff"), mpt::saturate_round<int32>(CResamplerSettings().gdWFIRCutoff * 100.0))
	, ResamplerEmulateAmiga(conf, UL_("Sound Settings"), UL_("ResamplerEmulateAmiga"), Resampling::AmigaFilter::A1200)
	, SoundBoostedThreadPriority(conf, UL_("Sound Settings"), UL_("BoostedThreadPriority"), SoundDevice::AppInfo().BoostedThreadPriorityXP)
	, SoundBoostedThreadMMCSSClass(conf, UL_("Sound Settings"), UL_("BoostedThreadMMCSSClass"), SoundDevice::AppInfo().BoostedThreadMMCSSClassVista)
	, SoundBoostedThreadRealtimePosix(conf, UL_("Sound Settings"), UL_("BoostedThreadRealtimeLinux"), SoundDevice::AppInfo().BoostedThreadRealtimePosix)
	, SoundBoostedThreadNicenessPosix(conf, UL_("Sound Settings"), UL_("BoostedThreadNicenessPosix"), SoundDevice::AppInfo().BoostedThreadNicenessPosix)
	, SoundBoostedThreadRtprioPosix(conf, UL_("Sound Settings"), UL_("BoostedThreadRtprioLinux"), SoundDevice::AppInfo().BoostedThreadRtprioPosix)
	, SoundMaskDriverCrashes(conf, UL_("Sound Settings"), UL_("MaskDriverCrashes"), SoundDevice::AppInfo().MaskDriverCrashes)
	, SoundAllowDeferredProcessing(conf, UL_("Sound Settings"), UL_("AllowDeferredProcessing"), SoundDevice::AppInfo().AllowDeferredProcessing)
	// MIDI Settings
	, m_nMidiDevice(conf, UL_("MIDI Settings"), UL_("MidiDevice"), 0)
	, midiDeviceName(conf, UL_("MIDI Settings"), UL_("MidiDeviceName"), _T(""))
	, midiSetup(conf, UL_("MIDI Settings"), UL_("MidiSetup"), MidiSetup::Default)
	, aftertouchBehaviour(conf, UL_("MIDI Settings"), UL_("AftertouchBehaviour"), atDoNotRecord)
	, midiVelocityAmp(conf, UL_("MIDI Settings"), UL_("MidiVelocityAmp"), 100)
	, midiIgnoreCCs(conf, UL_("MIDI Settings"), UL_("IgnoredCCs"), std::bitset<128>())
	, midiImportPatternLen(conf, UL_("MIDI Settings"), UL_("MidiImportPatLen"), 128)
	, midiImportQuantize(conf, UL_("MIDI Settings"), UL_("MidiImportQuantize"), 32)
	, midiImportTicks(conf, UL_("MIDI Settings"), UL_("MidiImportTicks"), 6)
	// Pattern Editor
	, gbLoopSong(conf, UL_("Pattern Editor"), UL_("LoopSong"), true)
	, gnPatternSpacing(conf, UL_("Pattern Editor"), UL_("Spacing"), 0)
	, gbPatternVUMeters(conf, UL_("Pattern Editor"), UL_("VU-Meters"), true)
	, gbPatternPluginNames(conf, UL_("Pattern Editor"), UL_("Plugin-Names"), true)
	, gbPatternRecord(conf, UL_("Pattern Editor"), UL_("Record"), true)
	, patternNoEditPopup(conf, UL_("Pattern Editor"), UL_("NoEditPopup"), false)
	, patternStepCommands(conf, UL_("Pattern Editor"), UL_("EditStepAppliesToCommands"), false)
	, patternVolColHex(conf, UL_("Pattern Editor"), UL_("VolumeColumnInHex"), false)
	, patternSetup(conf, UL_("Pattern Editor"), UL_("PatternSetup"), PatternSetup::Default)
	, m_nRowHighlightMeasures(conf, UL_("Pattern Editor"), UL_("RowSpacing"), 16)
	, m_nRowHighlightBeats(conf, UL_("Pattern Editor"), UL_("RowSpacing2"), 4)
	, patternIgnoreSongTimeSignature(conf, UL_("Pattern Editor"), UL_("IgnoreSongTimeSignature"), false)
	, recordQuantizeRows(conf, UL_("Pattern Editor"), UL_("RecordQuantize"), 0)
	, gnAutoChordWaitTime(conf, UL_("Pattern Editor"), UL_("AutoChordWaitTime"), 60)
	, orderlistMargins(conf, UL_("Pattern Editor"), UL_("DefaultSequenceMargins"), 0)
	, rowDisplayOffset(conf, UL_("Pattern Editor"), UL_("RowDisplayOffset"), 0)
	, patternFont(conf, UL_("Pattern Editor"), UL_("Font"), FontSetting(PATTERNFONT_SMALL, 0))
	, patternFontDot(conf, UL_("Pattern Editor"), UL_("FontDot"), UL_("."))
	, effectVisWidth(conf, UL_("Pattern Editor"), UL_("EffectVisWidth"), -1)
	, effectVisHeight(conf, UL_("Pattern Editor"), UL_("EffectVisHeight"), -1)
	, effectVisX(conf, UL_("Pattern Editor"), UL_("EffectVisX"), int32_min)
	, effectVisY(conf, UL_("Pattern Editor"), UL_("EffectVisY"), int32_min)
	, patternAccessibilityFormat(conf, UL_("Pattern Editor"), UL_("AccessibilityFormat"), _T("Row %row%, Channel %channel%, %column_type%: %column_description%"))
	, patternAlwaysDrawWholePatternOnScrollSlow(conf, UL_("Pattern Editor"), UL_("AlwaysDrawWholePatternOnScrollSlow"), false)
	, orderListOldDropBehaviour(conf, UL_("Pattern Editor"), UL_("OrderListOldDropBehaviour"), false)
	, autoHideVolumeColumnForMOD(conf, UL_("Pattern Editor"), UL_("AutoHideVolumeColumnForMOD"), false)
	, metronomeEnabled(conf, UL_("Pattern Editor"), UL_("MetronomeEnabled"), false)
	, metronomeVolume(conf, UL_("Pattern Editor"), UL_("MetronomeVolume"), -3.0f)
	, metronomeSampleMeasure(conf, UL_("Pattern Editor"), UL_("MetronomeSampleMeasure"), GetDefaultMetronomeSample())
	, metronomeSampleBeat(conf, UL_("Pattern Editor"), UL_("MetronomeSampleBeat"), GetDefaultMetronomeSample())
	// Sample Editor
	, m_SampleUndoBufferSize(conf, UL_("Sample Editor"), UL_("UndoBufferSize"), SampleUndoBufferSize())
	, sampleEditorKeyBehaviour(conf, UL_("Sample Editor"), UL_("KeyBehaviour"), seNoteOffOnNewKey)
	, m_defaultSampleFormat(conf, UL_("Sample Editor"), UL_("DefaultFormat"), dfFLAC)
	, m_followSamplePlayCursor(conf, UL_("Sample Editor"), UL_("FollowSamplePlayCursor"), FollowSamplePlayCursor::DoNotFollow)
	, sampleEditorTimelineFormat(conf, UL_("Sample Editor"), UL_("TimelineFormat"), TimelineFormat::Seconds)
	, sampleEditorDefaultResampler(conf, UL_("Sample Editor"), UL_("DefaultResampler"), SRCMODE_DEFAULT)
	, m_nFinetuneStep(conf, UL_("Sample Editor"), UL_("FinetuneStep"), 10)
	, m_FLACCompressionLevel(conf, UL_("Sample Editor"), UL_("FLACCompressionLevel"), 5)
	, compressITI(conf, UL_("Sample Editor"), UL_("CompressITI"), true)
	, m_MayNormalizeSamplesOnLoad(conf, UL_("Sample Editor"), UL_("MayNormalizeSamplesOnLoad"), true)
	, previewInFileDialogs(conf, UL_("Sample Editor"), UL_("PreviewInFileDialogs"), false)
	, cursorPositionInHex(conf, UL_("Sample Editor"), UL_("CursorPositionInHex"), false)
	// Export
	, ExportDefaultToSoundcardSamplerate(conf, UL_("Export"), UL_("DefaultToSoundcardSamplerate"), true)
	, ExportStreamEncoderSettings(conf, UL_("Export"))
	, ExportNormalize(conf, UL_("Export"), UL_("Normalize"), false)
	, ExportClearPluginBuffers(conf, UL_("Export"), UL_("ClearPluginBuffers"), true)
	// Components
	, ComponentsLoadOnStartup(conf, UL_("Components"), UL_("LoadOnStartup"), ComponentManagerSettingsDefault().LoadOnStartup())
	, ComponentsKeepLoaded(conf, UL_("Components"), UL_("KeepLoaded"), ComponentManagerSettingsDefault().KeepLoaded())
	// AutoSave
	, CreateBackupFiles(conf, UL_("AutoSave"), UL_("CreateBackupFiles"), true)
	, AutosaveEnabled(conf, UL_("AutoSave"), UL_("Enabled"), true)
	, AutosaveIntervalMinutes(conf, UL_("AutoSave"), UL_("IntervalMinutes"), 10)
	, AutosaveHistoryDepth(conf, UL_("AutoSave"), UL_("BackupHistory"), 3)
	, AutosaveRetentionTimeDays(conf, UL_("AutoSave"), UL_("RetentionTimeDays"), 30)
	, AutosaveUseOriginalPath(conf, UL_("AutoSave"), UL_("UseOriginalPath"), false)
	, AutosaveDeletePermanently(conf, UL_("AutoSave"), UL_("DeletePermanently"), false)
	, AutosavePath(conf, UL_("AutoSave"), UL_("Path"), GetDefaultAutosavePath())
	// Paths
	, PathSongs(conf, UL_("Paths"), UL_("Songs_Directory"), mpt::PathString())
	, PathSamples(conf, UL_("Paths"), UL_("Samples_Directory"), mpt::PathString())
	, PathInstruments(conf, UL_("Paths"), UL_("Instruments_Directory"), mpt::PathString())
	, PathPlugins(conf, UL_("Paths"), UL_("Plugins_Directory"), mpt::PathString())
	, PathPluginPresets(conf, UL_("Paths"), UL_("Plugin_Presets_Directory"), mpt::PathString())
	, PathExport(conf, UL_("Paths"), UL_("Export_Directory"), mpt::PathString())
	, PathTunings(theApp.GetConfigPath() + P_("tunings\\"))
	// Default template
	, defaultTemplateFile(conf, UL_("Paths"), UL_("DefaultTemplate"), mpt::PathString())
	, defaultArtist(conf, UL_("Misc"), UL_("DefaultArtist"), mpt::getenv(UL_("USERNAME")).value_or(UL_("")))
	// MRU List
	, mruListLength(conf, UL_("Misc"), UL_("MRUListLength"), 10)
	// Plugins
	, bridgeAllPlugins(conf, UL_("VST Plugins"), UL_("BridgeAllPlugins"), false)
	, enableAutoSuspend(conf, UL_("VST Plugins"), UL_("EnableAutoSuspend"), false)
	, midiMappingInPluginEditor(conf, UL_("VST Plugins"), UL_("EnableMidiMappingInEditor"), true)
	, pluginProjectPath(conf, UL_("VST Plugins"), UL_("ProjectPath"), mpt::ustring())
	, vstHostProductString(conf, UL_("VST Plugins"), UL_("HostProductString"), "OpenMPT")
	, vstHostVendorString(conf, UL_("VST Plugins"), UL_("HostVendorString"), "OpenMPT project")
	, vstHostVendorVersion(conf, UL_("VST Plugins"), UL_("HostVendorVersion"), Version::Current().GetRawVersion())
	// Broken Plugins Workarounds
	, BrokenPluginsWorkaroundVSTMaskAllCrashes(conf, UL_("Broken Plugins Workarounds"), UL_("VSTMaskAllCrashes"), true)  // TODO: really should be false
	, BrokenPluginsWorkaroundVSTNeverUnloadAnyPlugin(conf, UL_("Broken Plugins Workarounds"), UL_("VSTNeverUnloadAnyPlugin"), false)
#if defined(MPT_ENABLE_UPDATE)
	// Update
	, UpdateEnabled(conf, UL_("Update"), UL_("Enabled"), true)
	, UpdateInstallAutomatically(conf, UL_("Update"), UL_("InstallAutomatically"), false)
	, UpdateLastUpdateCheck(conf, UL_("Update"), UL_("LastUpdateCheck"), mpt::Date::Unix{})
	, UpdateUpdateCheckPeriod_DEPRECATED(conf, UL_("Update"), UL_("UpdateCheckPeriod"), 7)
	, UpdateIntervalDays(conf, UL_("Update"), UL_("UpdateCheckIntervalDays"), 7)
	, UpdateChannel(conf, UL_("Update"), UL_("Channel"), UpdateChannelRelease)
	, UpdateUpdateURL_DEPRECATED(conf, UL_("Update"), UL_("UpdateURL"), UL_("https://update.openmpt.org/check/$VERSION/$GUID"))
	, UpdateAPIURL(conf, UL_("Update"), UL_("APIURL"), CUpdateCheck::GetDefaultAPIURL())
	, UpdateStatisticsConsentAsked(conf, UL_("Update"), UL_("StatistisConsentAsked"), false)
	, UpdateStatistics(conf, UL_("Update"), UL_("Statistis"), false)
	, UpdateSendGUID_DEPRECATED(conf, UL_("Update"), UL_("SendGUID"), false)
	, UpdateShowUpdateHint(conf, UL_("Update"), UL_("ShowUpdateHint"), true)
	, UpdateIgnoreVersion(conf, UL_("Update"), UL_("IgnoreVersion"), _T(""))
	, UpdateSkipSignatureVerificationUNSECURE(conf, UL_("Update"), UL_("SkipSignatureVerification"), false)
	, UpdateSigningKeysRootAnchors(conf, UL_("Update"), UL_("SigningKeysRootAnchors"), CUpdateCheck::GetDefaultUpdateSigningKeysRootAnchors())
#endif // MPT_ENABLE_UPDATE
	// Wine suppport
	, WineSupportEnabled(conf, UL_("WineSupport"), UL_("Enabled"), false)
	, WineSupportAlwaysRecompile(conf, UL_("WineSupport"), UL_("AlwaysRecompile"), false)
	, WineSupportAskCompile(conf, UL_("WineSupport"), UL_("AskCompile"), false)
	, WineSupportCompileVerbosity(conf, UL_("WineSupport"), UL_("CompileVerbosity"), 2) // 0=silent 1=silentmake 2=progresswindow 3=standard 4=verbosemake 5=veryverbosemake 6=msgboxes
	, WineSupportForeignOpenMPT(conf, UL_("WineSupport"), UL_("ForeignOpenMPT"), false)
	, WineSupportAllowUnknownHost(conf, UL_("WineSupport"), UL_("AllowUnknownHost"), false)
	, WineSupportEnablePulseAudio(conf, UL_("WineSupport"), UL_("EnablePulseAudio"), 1)
	, WineSupportEnablePortAudio(conf, UL_("WineSupport"), UL_("EnablePortAudio"), 1)
	, WineSupportEnableRtAudio(conf, UL_("WineSupport"), UL_("EnableRtAudio"), 1)
{

	// Effects
#ifndef NO_DSP
	m_MegaBassSettings.m_nXBassDepth = conf.Read<int32>(UL_("Effects"), UL_("XBassDepth"), m_MegaBassSettings.m_nXBassDepth);
	m_MegaBassSettings.m_nXBassRange = conf.Read<int32>(UL_("Effects"), UL_("XBassRange"), m_MegaBassSettings.m_nXBassRange);
#endif
#ifndef NO_REVERB
	m_ReverbSettings.m_nReverbDepth = conf.Read<int32>(UL_("Effects"), UL_("ReverbDepth"), m_ReverbSettings.m_nReverbDepth);
	m_ReverbSettings.m_nReverbType = conf.Read<int32>(UL_("Effects"), UL_("ReverbType"), m_ReverbSettings.m_nReverbType);
#endif
#ifndef NO_DSP
	m_SurroundSettings.m_nProLogicDepth = conf.Read<int32>(UL_("Effects"), UL_("ProLogicDepth"), m_SurroundSettings.m_nProLogicDepth);
	m_SurroundSettings.m_nProLogicDelay = conf.Read<int32>(UL_("Effects"), UL_("ProLogicDelay"), m_SurroundSettings.m_nProLogicDelay);
#endif
#ifndef NO_EQ
	m_EqSettings = conf.Read<EQPreset>(UL_("Effects"), UL_("EQ_Settings"), FlatEQPreset);
	const EQPreset userPresets[] =
	{
		FlatEQPreset,
		{ "User 1", {16,16,16,16,16,16}, { 150, 350, 700, 1500, 4500, 8000 } },
		{ "User 2", {16,16,16,16,16,16}, { 200, 400, 800, 1750, 5000, 9000 } },
		{ "User 3", {16,16,16,16,16,16}, { 250, 450, 900, 2000, 5000, 10000 } }
	};

	m_EqUserPresets[0] = conf.Read<EQPreset>(UL_("Effects"), UL_("EQ_User1"), userPresets[0]);
	m_EqUserPresets[1] = conf.Read<EQPreset>(UL_("Effects"), UL_("EQ_User2"), userPresets[1]);
	m_EqUserPresets[2] = conf.Read<EQPreset>(UL_("Effects"), UL_("EQ_User3"), userPresets[2]);
	m_EqUserPresets[3] = conf.Read<EQPreset>(UL_("Effects"), UL_("EQ_User4"), userPresets[3]);
#endif
#ifndef NO_DSP
	m_BitCrushSettings.m_Bits = conf.Read<int32>(UL_("Effects"), UL_("BitCrushBits"), m_BitCrushSettings.m_Bits);
#endif
	// Display (Colors)
	GetDefaultColourScheme(rgbCustomColors);
	for(int ncol = 0; ncol < MAX_MODCOLORS; ncol++)
	{
		const mpt::ustring colorName = MPT_UFORMAT("Color{}")(mpt::ufmt::dec0<2>(ncol));
		rgbCustomColors[ncol] = conf.Read<uint32>(UL_("Display"), colorName, rgbCustomColors[ncol]);
		// For old color schemes that don't have this color yet
		if(ncol == MODCOLOR_BACKCURROW)
			rgbCustomColors[MODCOLOR_BACKRECORDROW] = rgbCustomColors[MODCOLOR_BACKCURROW];
	}
	// Paths
	m_szKbdFile = conf.Read<mpt::PathString>(UL_("Paths"), UL_("Key_Config_File"), mpt::PathString());
	conf.Forget(UL_("Paths"), UL_("Key_Config_File"));

	// init old and messy stuff:

	// Default chords
	for(size_t i = 0; i < Chords.size(); i++)
	{
		mpt::reset(Chords[i]);
		Chords[i].key = static_cast<uint8>(i);
		Chords[i].notes[0] = MPTChord::noNote;
		Chords[i].notes[1] = MPTChord::noNote;
		Chords[i].notes[2] = MPTChord::noNote;

		if(i < 12)
		{
			// Major Chords
			Chords[i].notes[0] = static_cast<int8>(i + 4);
			Chords[i].notes[1] = static_cast<int8>(i + 7);
			Chords[i].notes[2] = static_cast<int8>(i + 10);
		} else if(i < 24)
		{
			// Minor Chords
			Chords[i].notes[0] = static_cast<int8>(i - 9);
			Chords[i].notes[1] = static_cast<int8>(i - 5);
			Chords[i].notes[2] = static_cast<int8>(i - 2);
		}
	}


	// load old and messy stuff:

	PatternClipboard::SetClipboardSize(conf.Read<int32>(UL_("Pattern Editor"), UL_("NumClipboards"), mpt::saturate_cast<int32>(PatternClipboard::GetClipboardSize())));

	// Chords
	LoadChords(Chords);

	// Zxx Macros
	MIDIMacroConfig macros;
	theApp.GetDefaultMidiMacro(macros);
	for(int i = 0; i < kSFxMacros; i++)
	{
		macros.SFx[i] = conf.Read<std::string>(UL_("Zxx Macros"), MPT_UFORMAT("SF{}")(mpt::ufmt::HEX(i)), macros.SFx[i]);
	}
	for(int i = 0; i < kZxxMacros; i++)
	{
		macros.Zxx[i] = conf.Read<std::string>(UL_("Zxx Macros"), MPT_UFORMAT("Z{}")(mpt::ufmt::HEX0<2>(i | 0x80)), macros.Zxx[i]);
	}


	// MRU list
	Limit(mruListLength, 0u, 32u);
	mruFiles.reserve(mruListLength);
	for(uint32 i = 0; i < mruListLength; i++)
	{
		mpt::ustring key = MPT_UFORMAT("File{}")(i);

		mpt::PathString path = theApp.PathInstallRelativeToAbsolute(conf.Read<mpt::PathString>(UL_("Recent File List"), key, mpt::PathString()));
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
		VersionInstallGUID = mpt::UUID::Generate(mpt::global_prng());
	}

	// Plugins
	if(storedVersion < MPT_V("1.19.03.01") && vstHostProductString.Get() == "OpenMPT")
	{
		vstHostVendorVersion = Version::Current().GetRawVersion();
	}
	if(storedVersion < MPT_V("1.30.00.24"))
	{
		BrokenPluginsWorkaroundVSTNeverUnloadAnyPlugin = !conf.Read<bool>(UL_("VST Plugins"), UL_("FullyUnloadPlugins"), true);
		conf.Remove(UL_("VST Plugins"), UL_("FullyUnloadPlugins"));
	}

	// Sound Settings
	if(storedVersion < MPT_V("1.22.07.30"))
	{
		if(conf.Read<bool>(UL_("Sound Settings"), UL_("KeepDeviceOpen"), false))
		{
			m_SoundSettingsStopMode = SoundDeviceStopModePlaying;
		} else
		{
			m_SoundSettingsStopMode = SoundDeviceStopModeStopped;
		}
	}
	if(storedVersion < MPT_V("1.22.07.04"))
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
	if(storedVersion < MPT_V("1.22.07.04"))
	{
		m_SoundDeviceID_DEPRECATED = conf.Read<SoundDevice::Legacy::ID>(UL_("Sound Settings"), UL_("WaveDevice"), SoundDevice::Legacy::ID());
		Setting<uint32> m_BufferLength_DEPRECATED(conf, UL_("Sound Settings"), UL_("BufferLength"), 50);
		Setting<uint32> m_LatencyMS(conf, UL_("Sound Settings"), UL_("Latency"), mpt::saturate_round<int32>(SoundDevice::Settings().Latency * 1000.0));
		Setting<uint32> m_UpdateIntervalMS(conf, UL_("Sound Settings"), UL_("UpdateInterval"), mpt::saturate_round<int32>(SoundDevice::Settings().UpdateInterval * 1000.0));
		Setting<SampleFormat> m_SampleFormat(conf, UL_("Sound Settings"), UL_("BitsPerSample"), SoundDevice::Settings().sampleFormat);
		Setting<bool> m_SoundDeviceExclusiveMode(conf, UL_("Sound Settings"), UL_("ExclusiveMode"), SoundDevice::Settings().ExclusiveMode);
		Setting<bool> m_SoundDeviceBoostThreadPriority(conf, UL_("Sound Settings"), UL_("BoostThreadPriority"), SoundDevice::Settings().BoostThreadPriority);
		Setting<bool> m_SoundDeviceUseHardwareTiming(conf, UL_("Sound Settings"), UL_("UseHardwareTiming"), SoundDevice::Settings().UseHardwareTiming);
		Setting<SoundDevice::ChannelMapping> m_SoundDeviceChannelMapping(conf, UL_("Sound Settings"), UL_("ChannelMapping"), SoundDevice::Settings().Channels);
		if(storedVersion < MPT_V("1.21.01.26"))
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
		if(storedVersion < MPT_V("1.22.01.03"))
		{
			m_SoundDeviceExclusiveMode = ((MixerFlags & OLD_SOUNDSETUP_SECONDARY) == 0);
		}
		if(storedVersion < MPT_V("1.22.01.03"))
		{
			m_SoundDeviceBoostThreadPriority = ((MixerFlags & OLD_SOUNDSETUP_NOBOOSTTHREADPRIORITY) == 0);
		}
		if(storedVersion < MPT_V("1.22.07.03"))
		{
			m_SoundDeviceChannelMapping = SoundDevice::ChannelMapping::BaseChannel(MixerOutputChannels, conf.Read<int>(UL_("Sound Settings"), UL_("ASIOBaseChannel"), 0));
		}
		m_SoundDeviceSettingsDefaults.Latency = m_LatencyMS / 1000.0;
		m_SoundDeviceSettingsDefaults.UpdateInterval = m_UpdateIntervalMS / 1000.0;
		m_SoundDeviceSettingsDefaults.Samplerate = MixerSamplerate;
		if(m_SoundDeviceSettingsDefaults.Channels.GetNumHostChannels() != MixerOutputChannels)
		{
			// reset invalid channel mapping to default
			m_SoundDeviceSettingsDefaults.Channels = SoundDevice::ChannelMapping(MixerOutputChannels);
		}
		m_SoundDeviceSettingsDefaults.InputChannels = 0;
		m_SoundDeviceSettingsDefaults.sampleFormat = m_SampleFormat;
		m_SoundDeviceSettingsDefaults.ExclusiveMode = m_SoundDeviceExclusiveMode;
		m_SoundDeviceSettingsDefaults.BoostThreadPriority = m_SoundDeviceBoostThreadPriority;
		m_SoundDeviceSettingsDefaults.UseHardwareTiming = m_SoundDeviceUseHardwareTiming;
		m_SoundDeviceSettingsDefaults.InputSourceID = 0;
		m_SoundDeviceSettingsUseOldDefaults = true;
	}
	if(storedVersion < MPT_V("1.28.00.41"))
	{
		// reset this setting to the default when updating,
		// because we do not provide a GUI any more,
		// and in general, it should not get changed anyway
		ResamplerCutoffPercent = mpt::saturate_round<int32>(CResamplerSettings().gdWFIRCutoff * 100.0);
	}
	if(MixerSamplerate == 0)
	{
		MixerSamplerate = MixerSettings().gdwMixingFreq;
	}
	if(storedVersion < MPT_V("1.21.01.26"))
	{
		MixerFlags &= ~OLD_SOUNDSETUP_REVERSESTEREO;
	}
	if(storedVersion < MPT_V("1.22.01.03"))
	{
		MixerFlags &= ~OLD_SOUNDSETUP_SECONDARY;
	}
	if(storedVersion < MPT_V("1.22.01.03"))
	{
		MixerFlags &= ~OLD_SOUNDSETUP_NOBOOSTTHREADPRIORITY;
	}
	if(storedVersion < MPT_V("1.20.00.22"))
	{
		MixerSettings settings = GetMixerSettings();
		settings.SetVolumeRampUpSamples(conf.Read<int32>(UL_("Sound Settings"), UL_("VolumeRampSamples"), 42));
		settings.SetVolumeRampDownSamples(conf.Read<int32>(UL_("Sound Settings"), UL_("VolumeRampSamples"), 42));
		SetMixerSettings(settings);
		conf.Remove(UL_("Sound Settings"), UL_("VolumeRampSamples"));
	} else if(storedVersion < MPT_V("1.22.07.18"))
	{
		MixerSettings settings = GetMixerSettings();
		settings.SetVolumeRampUpSamples(conf.Read<int32>(UL_("Sound Settings"), UL_("VolumeRampUpSamples"), MixerSettings().GetVolumeRampUpSamples()));
		settings.SetVolumeRampDownSamples(conf.Read<int32>(UL_("Sound Settings"), UL_("VolumeRampDownSamples"), MixerSettings().GetVolumeRampDownSamples()));
		SetMixerSettings(settings);
	}
	Limit(ResamplerCutoffPercent, 0, 100);
	if(storedVersion < MPT_V("1.29.00.11"))
	{
		MixerMaxChannels = MixerSettings().m_nMaxMixChannels;  // reset to default on update because we removed the setting in the GUI
	}
	if(storedVersion < MPT_V("1.29.00.20"))
	{
		MixerDSPMask = MixerDSPMask & ~SNDDSP_BITCRUSH;
	}

	// Misc
	if(defaultModType == MOD_TYPE_NONE)
	{
		defaultModType = MOD_TYPE_IT;
	}

	// MIDI Settings
	static constexpr MidiSetup LegacyFlagAmplify2x = static_cast<MidiSetup>(0x40);
	if(midiSetup.Get()[LegacyFlagAmplify2x] && storedVersion < MPT_V("1.20.00.86"))
	{
		// This flag used to be "amplify MIDI Note Velocity" - with a fixed amplification factor of 2.
		midiVelocityAmp = 200;
		midiSetup &= ~LegacyFlagAmplify2x;
	}

	// Pattern Editor
	if(storedVersion < MPT_V("1.17.02.50"))
	{
		patternSetup |= PatternSetup::NoteFadeOnKeyUp;
	}
	if(storedVersion < MPT_V("1.17.03.01"))
	{
		patternSetup |= PatternSetup::ResetChannelsOnLoop;
	}
	if(storedVersion < MPT_V("1.19.00.07"))
	{
		patternSetup &= ~PatternSetup(0x800);  // this was previously deprecated and is now used for something else
	}
	if(storedVersion < MPT_V("1.20.00.04"))
	{
		patternSetup &= ~PatternSetup(0x200000);  // ditto
	}
	if(storedVersion < MPT_V("1.20.00.07"))
	{
		patternSetup &= ~PatternSetup(0x400000);  // ditto
	}
	if(storedVersion < MPT_V("1.20.00.39"))
	{
		patternSetup &= ~PatternSetup(0x10000000);  // ditto
	}
	if(storedVersion < MPT_V("1.24.01.04"))
	{
		commentsFont = FontSetting(UL_("Courier New"), (patternSetup & PatternSetup(0x02)) ? 120 : 90);
		patternFont = FontSetting((patternSetup & PatternSetup(0x08)) ? PATTERNFONT_SMALL : PATTERNFONT_LARGE, 0);
		patternSetup &= ~PatternSetup(0x08 | 0x02);
	}
	if(storedVersion < MPT_V("1.25.00.08") && glGeneralWindowHeight < 222)
	{
		glGeneralWindowHeight += 44;
	}
	if(storedVersion < MPT_V("1.25.00.16") && (patternSetup & PatternSetup(0x100000)))
	{
		// Move MIDI recording to MIDI setup
		patternSetup &= ~PatternSetup(0x100000);
		midiSetup |= MidiSetup::EnableMidiInOnStartup;
	}
	if(storedVersion < MPT_V("1.27.00.51"))
	{
		// Moving option out of pattern config
		CreateBackupFiles = (patternSetup & PatternSetup(0x200));
		patternSetup &= ~PatternSetup(0x200);
	}

	// Export
	if(storedVersion < MPT_V("1.30.00.38"))
	{
		{
			conf.Write<Encoder::Mode>(UL_("Export"), UL_("FLAC_Mode"), Encoder::ModeLossless);
			const int oldformat = conf.Read<int>(UL_("Export"), UL_("FLAC_Format"), 1);
			Encoder::Format newformat = { Encoder::Format::Encoding::Integer, 24, mpt::get_endian() };
			if (oldformat >= 0)
			{
				switch (oldformat % 3)
				{
				case 0:
					newformat = { Encoder::Format::Encoding::Integer, 24, mpt::get_endian() };
					break;
				case 1:
					newformat = { Encoder::Format::Encoding::Integer, 16, mpt::get_endian() };
					break;
				case 2:
					newformat = { Encoder::Format::Encoding::Integer, 8, mpt::get_endian() };
					break;
				}
			}
			conf.Write<Encoder::Format>(UL_("Export"), UL_("FLAC_Format2"), newformat);
			conf.Forget(UL_("Export"), UL_("FLAC_Format"));
		}
		{
			conf.Write<Encoder::Mode>(UL_("Export"), UL_("Wave_Mode"), Encoder::ModeLossless);
			const int oldformat = conf.Read<int>(UL_("Export"), UL_("Wave_Format"), 1);
			Encoder::Format newformat = { Encoder::Format::Encoding::Float, 32, mpt::endian::little };
			if (oldformat >= 0)
			{
				switch (oldformat % 6)
				{
				case 0:
					newformat = { Encoder::Format::Encoding::Float, 64, mpt::endian::little };
					break;
				case 1:
					newformat = { Encoder::Format::Encoding::Float, 32, mpt::endian::little };
					break;
				case 2:
					newformat = { Encoder::Format::Encoding::Integer, 32, mpt::endian::little };
					break;
				case 3:
					newformat = { Encoder::Format::Encoding::Integer, 24, mpt::endian::little };
					break;
				case 4:
					newformat = { Encoder::Format::Encoding::Integer, 16, mpt::endian::little };
					break;
				case 5:
					newformat = { Encoder::Format::Encoding::Unsigned, 8, mpt::endian::little };
					break;
				}
			}
			conf.Write<Encoder::Format>(UL_("Export"), UL_("Wave_Format2"), newformat);
			conf.Forget(UL_("Export"), UL_("Wave_Format"));
		}
		{
			conf.Write<Encoder::Mode>(UL_("Export"), UL_("AU_Mode"), Encoder::ModeLossless);
			const int oldformat = conf.Read<int>(UL_("Export"), UL_("AU_Format"), 1);
			Encoder::Format newformat = { Encoder::Format::Encoding::Float, 32, mpt::endian::big };
			if(oldformat >= 0)
			{
				switch(oldformat % 6)
				{
				case 0:
					newformat = { Encoder::Format::Encoding::Float, 64, mpt::endian::big };
					break;
				case 1:
					newformat = { Encoder::Format::Encoding::Float, 32, mpt::endian::big };
					break;
				case 2:
					newformat = { Encoder::Format::Encoding::Integer, 32, mpt::endian::big };
					break;
				case 3:
					newformat = { Encoder::Format::Encoding::Integer, 24, mpt::endian::big };
					break;
				case 4:
					newformat = { Encoder::Format::Encoding::Integer, 16, mpt::endian::big };
					break;
				case 5:
					newformat = { Encoder::Format::Encoding::Integer, 8, mpt::endian::big };
					break;
				}
			}
			conf.Write<Encoder::Format>(UL_("Export"), UL_("AU_Format2"), newformat);
			conf.Forget(UL_("Export"), UL_("AU_Format"));
		}
		{
			conf.Write<Encoder::Mode>(UL_("Export"), UL_("RAW_Mode"), Encoder::ModeLossless);
			const int oldformat = conf.Read<int>(UL_("Export"), UL_("RAW_Format"), 1);
			Encoder::Format newformat = { Encoder::Format::Encoding::Float, 32, mpt::get_endian() };
			if(oldformat >= 0)
			{
				switch(oldformat % 7)
				{
				case 0:
					newformat = { Encoder::Format::Encoding::Float, 64, mpt::get_endian() };
					break;
				case 1:
					newformat = { Encoder::Format::Encoding::Float, 32, mpt::get_endian() };
					break;
				case 2:
					newformat = { Encoder::Format::Encoding::Integer, 32, mpt::get_endian() };
					break;
				case 3:
					newformat = { Encoder::Format::Encoding::Integer, 24, mpt::get_endian() };
					break;
				case 4:
					newformat = { Encoder::Format::Encoding::Integer, 16, mpt::get_endian() };
					break;
				case 5:
					newformat = { Encoder::Format::Encoding::Integer, 8, mpt::get_endian() };
					break;
				case 6:
					newformat = { Encoder::Format::Encoding::Unsigned, 8, mpt::get_endian() };
					break;
				}
			}
			conf.Write<Encoder::Format>(UL_("Export"), UL_("RAW_Format2"), newformat);
			conf.Forget(UL_("Export"), UL_("RAW_Format"));
		}
	}

#if defined(MPT_ENABLE_UPDATE)
	// Update
	if(storedVersion < MPT_V("1.28.00.39"))
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
		const auto url = UpdateUpdateURL_DEPRECATED.Get();
		if(url.empty() ||
		   url == UL_("http://update.openmpt.org/check/$VERSION/$GUID") ||
		   url == UL_("https://update.openmpt.org/check/$VERSION/$GUID"))
		{
			UpdateChannel = UpdateChannelRelease;
		} else if(url == UL_("http://update.openmpt.org/check/testing/$VERSION/$GUID") ||
		          url == UL_("https://update.openmpt.org/check/testing/$VERSION/$GUID"))
		{
			UpdateChannel = UpdateChannelDevelopment;
		} else
		{
			UpdateChannel = UpdateChannelDevelopment;
		}
		UpdateStatistics = UpdateSendGUID_DEPRECATED.Get();
		conf.Forget(UpdateUpdateCheckPeriod_DEPRECATED.GetPath());
		conf.Forget(UpdateUpdateURL_DEPRECATED.GetPath());
		conf.Forget(UpdateSendGUID_DEPRECATED.GetPath());
	}
	if(storedVersion < MPT_V("1.31.00.12"))
	{
		UpdateLastUpdateCheck = mpt::Date::Unix{};
	}
#endif // MPT_ENABLE_UPDATE

	if(storedVersion < MPT_V("1.29.00.39"))
	{
		// ASIO device IDs are now normalized to upper-case in the device enumeration code.
		// Previous device IDs could be mixed-case (as retrieved from the registry), which would make direct ID comparison fail now.
		auto device = m_SoundDeviceIdentifier.Get();
		if(device.substr(0, 5) == UL_("ASIO_"))
		{
			device = mpt::ToUpperCase(device);
			m_SoundDeviceIdentifier = device;
		}
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
	if((MPT_V("1.17.00.00") <= storedVersion) && (storedVersion < MPT_V("1.20.00.00")))
	{
		// Fix old nasty broken (non-standard) MIDI configs in INI file.
		macros.UpgradeMacros();
	}
	theApp.SetDefaultMidiMacro(macros);

	// Paths
	m_szKbdFile = theApp.PathInstallRelativeToAbsolute(m_szKbdFile);

	// Sample undo buffer size (used to be a hidden, absolute setting in MiB)
	int64 oldUndoSize = m_SampleUndoBufferSize.Get().GetSizeInPercent();
	if(storedVersion < MPT_V("1.22.07.25") && oldUndoSize != SampleUndoBufferSize::defaultSize && oldUndoSize != 0)
	{
		m_SampleUndoBufferSize = SampleUndoBufferSize(static_cast<int32>(100 * (oldUndoSize << 20) / SampleUndoBufferSize(100).GetSizeInBytes()));
	}

	// More controls in the plugin selection dialog
	if(storedVersion < MPT_V("1.26.00.26"))
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
	if(midiImportPatternLen < ModSpecs::mptm.patternRowsMin || midiImportPatternLen > ModSpecs::mptm.patternRowsMax)
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


namespace SoundDevice
{
namespace Legacy
{
SoundDevice::Info FindDeviceInfo(SoundDevice::Manager &manager, SoundDevice::Legacy::ID id)
{
	if(manager.GetDeviceInfos().empty())
	{
		return SoundDevice::Info();
	}
	SoundDevice::Type type = SoundDevice::Type();
	switch((id & SoundDevice::Legacy::MaskType) >> SoundDevice::Legacy::ShiftType)
	{
		case SoundDevice::Legacy::TypeWAVEOUT:
			type = SoundDevice::TypeWAVEOUT;
			break;
		case SoundDevice::Legacy::TypeDSOUND:
			type = SoundDevice::TypeDSOUND;
			break;
		case SoundDevice::Legacy::TypeASIO:
			type = SoundDevice::TypeASIO;
			break;
		case SoundDevice::Legacy::TypePORTAUDIO_WASAPI:
			type = SoundDevice::TypePORTAUDIO_WASAPI;
			break;
		case SoundDevice::Legacy::TypePORTAUDIO_WDMKS:
			type = SoundDevice::TypePORTAUDIO_WDMKS;
			break;
		case SoundDevice::Legacy::TypePORTAUDIO_WMME:
			type = SoundDevice::TypePORTAUDIO_WMME;
			break;
		case SoundDevice::Legacy::TypePORTAUDIO_DS:
			type = SoundDevice::TypePORTAUDIO_DS;
			break;
	}
	if(type.empty())
	{	// fallback to first device
		return *manager.begin();
	}
	std::size_t index = static_cast<uint8>((id & SoundDevice::Legacy::MaskIndex) >> SoundDevice::Legacy::ShiftIndex);
	std::size_t seenDevicesOfDesiredType = 0;
	for(const auto &info : manager)
	{
		if(info.type == type)
		{
			if(seenDevicesOfDesiredType == index)
			{
				if(!info.IsValid())
				{	// fallback to first device
					return *manager.begin();
				}
				return info;
			}
			seenDevicesOfDesiredType++;
		}
	}
	// default to first device
	return *manager.begin();
}
} // namespace Legacy
} // namespace SoundDevice


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
}


void TrackerSettings::MigrateTunings(const Version storedVersion)
{
	if(!mpt::native_fs{}.is_directory(PathTunings.GetDefaultDir()))
	{
		CreateDirectory(PathTunings.GetDefaultDir().AsNative().c_str(), 0);
	}
	if(!mpt::native_fs{}.is_directory(PathTunings.GetDefaultDir() + P_("Built-in\\")))
	{
		CreateDirectory((PathTunings.GetDefaultDir() + P_("Built-in\\")).AsNative().c_str(), 0);
	}
	if(!mpt::native_fs{}.is_directory(PathTunings.GetDefaultDir() + P_("Locale\\")))
	{
		CreateDirectory((PathTunings.GetDefaultDir() + P_("Local\\")).AsNative().c_str(), 0);
	}
	{
		mpt::PathString fn = PathTunings.GetDefaultDir() + P_("Built-in\\12TET.tun");
		if(!mpt::native_fs{}.exists(fn))
		{
			std::unique_ptr<CTuning> pT = CSoundFile::CreateTuning12TET(UL_("12TET"));
			mpt::IO::SafeOutputFile sf(fn, std::ios::binary, mpt::IO::FlushMode::Full);
			pT->Serialize(sf);
		}
	}
	{
		mpt::PathString fn = PathTunings.GetDefaultDir() + P_("Built-in\\12TET [[fs15 1.17.02.49]].tun");
		if(!mpt::native_fs{}.exists(fn))
		{
			std::unique_ptr<CTuning> pT = CSoundFile::CreateTuning12TET(UL_("12TET [[fs15 1.17.02.49]]"));
			mpt::IO::SafeOutputFile sf(fn, std::ios::binary, mpt::IO::FlushMode::Full);
			pT->Serialize(sf);
		}
	}
	oldLocalTunings = LoadLocalTunings();
	if(storedVersion < MPT_V("1.27.00.56"))
	{
		UnpackTuningCollection(*oldLocalTunings, PathTunings.GetDefaultDir() + P_("Local\\"));
	}
}


std::unique_ptr<CTuningCollection> TrackerSettings::LoadLocalTunings()
{
	std::unique_ptr<CTuningCollection> s_pTuningsSharedLocal = std::make_unique<CTuningCollection>();
	mpt::ifstream f(
			PathTunings.GetDefaultDir()
			+ P_("local_tunings")
			+ mpt::PathString::FromUTF8(CTuningCollection::s_FileExtension)
		, std::ios::binary);
	if(f.good())
	{
		mpt::ustring dummyName;
		s_pTuningsSharedLocal->Deserialize(f, dummyName, TuningCharsetFallback);
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
	Setting<uint8> InputChannels;
	Setting<SampleFormat> sampleFormat;
	Setting<bool> ExclusiveMode;
	Setting<bool> BoostThreadPriority;
	Setting<bool> KeepDeviceRunning;
	Setting<bool> UseHardwareTiming;
	Setting<int32> DitherType;
	Setting<uint32> InputSourceID;

public:

	StoredSoundDeviceSettings(SettingsContainer &conf_, const SoundDevice::Info & deviceInfo, const SoundDevice::Settings & defaults)
		: conf(conf_)
		, deviceInfo(deviceInfo)
		, LatencyUS(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("Latency"), mpt::saturate_round<int32>(defaults.Latency * 1000000.0))
		, UpdateIntervalUS(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("UpdateInterval"), mpt::saturate_round<int32>(defaults.UpdateInterval * 1000000.0))
		, Samplerate(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("SampleRate"), defaults.Samplerate)
		, ChannelsOld(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("Channels"), mpt::saturate_cast<uint8>((int)defaults.Channels))
		, ChannelMapping(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("ChannelMapping"), defaults.Channels)
		, InputChannels(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("InputChannels"), defaults.InputChannels)
		, sampleFormat(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("SampleFormat"), defaults.sampleFormat)
		, ExclusiveMode(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("ExclusiveMode"), defaults.ExclusiveMode)
		, BoostThreadPriority(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("BoostThreadPriority"), defaults.BoostThreadPriority)
		, KeepDeviceRunning(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("KeepDeviceRunning"), defaults.KeepDeviceRunning)
		, UseHardwareTiming(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("UseHardwareTiming"), defaults.UseHardwareTiming)
		, DitherType(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("DitherType"), static_cast<int32>(defaults.DitherType))
		, InputSourceID(conf, UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("InputSourceID"), defaults.InputSourceID)
	{
		if(ChannelMapping.Get().GetNumHostChannels() != ChannelsOld)
		{
			// If the stored channel count and the count of channels used in the channel mapping do not match,
			// construct a default mapping from the channel count.
			ChannelMapping = SoundDevice::ChannelMapping(ChannelsOld);
		}
		// store informational data (not read back, just to allow the user to mock with the raw ini file)
		conf.Write(UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("Type"), deviceInfo.type);
		conf.Write(UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("InternalID"), deviceInfo.internalID);
		conf.Write(UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("API"), deviceInfo.apiName);
		conf.Write(UL_("Sound Settings"), deviceInfo.GetIdentifier() + UL_("_") + UL_("Name"), deviceInfo.name);
	}

	StoredSoundDeviceSettings & operator = (const SoundDevice::Settings &settings)
	{
		LatencyUS = mpt::saturate_round<int32>(settings.Latency * 1000000.0);
		UpdateIntervalUS = mpt::saturate_round<int32>(settings.UpdateInterval * 1000000.0);
		Samplerate = settings.Samplerate;
		ChannelsOld = mpt::saturate_cast<uint8>((int)settings.Channels);
		ChannelMapping = settings.Channels;
		InputChannels = settings.InputChannels;
		sampleFormat = settings.sampleFormat;
		ExclusiveMode = settings.ExclusiveMode;
		BoostThreadPriority = settings.BoostThreadPriority;
		KeepDeviceRunning = settings.KeepDeviceRunning;
		UseHardwareTiming = settings.UseHardwareTiming;
		DitherType = static_cast<int32>(settings.DitherType);
		InputSourceID = settings.InputSourceID;
		return *this;
	}

	operator SoundDevice::Settings () const
	{
		SoundDevice::Settings settings;
		settings.Latency = LatencyUS / 1000000.0;
		settings.UpdateInterval = UpdateIntervalUS / 1000000.0;
		settings.Samplerate = Samplerate;
		settings.Channels = ChannelMapping;
		settings.InputChannels = InputChannels;
		settings.sampleFormat = sampleFormat;
		settings.ExclusiveMode = ExclusiveMode;
		settings.BoostThreadPriority = BoostThreadPriority;
		settings.KeepDeviceRunning = KeepDeviceRunning;
		settings.UseHardwareTiming = UseHardwareTiming;
		settings.DitherType = DitherType;
		settings.InputSourceID = InputSourceID;
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
	settings.NumInputChannels = MixerNumInputChannels;
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
	MixerNumInputChannels = static_cast<uint32>(settings.NumInputChannels);
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
	colours[MODCOLOR_BACKRECORDROW] = RGB(0xC0, 0xC0, 0xC0);
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
	colours[MODCOLOR_SAMPLE_LOOPMARKER] = RGB(0x30, 0xCC, 0x30);
	colours[MODCOLOR_SAMPLE_SUSTAINMARKER] = RGB(50, 0xCC, 0xCC);
	colours[MODCOLOR_SAMPLE_CUEPOINT] = RGB(0xFF, 0xCC, 0x30);
}


#ifndef NO_EQ

void TrackerSettings::FixupEQ(EQPreset &eqSettings)
{
	for(UINT i = 0; i < MAX_EQ_BANDS; i++)
	{
		if(eqSettings.Gains[i] > 32)
			eqSettings.Gains[i] = 16;
		if((eqSettings.Freqs[i] < 100) || (eqSettings.Freqs[i] > 10000))
			eqSettings.Freqs[i] = FlatEQPreset.Freqs[i];
	}
	mpt::String::SetNullTerminator(eqSettings.szName);
}

#endif // !NO_EQ


void TrackerSettings::SaveSettings()
{
	WINDOWPLACEMENT wpl;
	HighDPISupport::GetWindowPlacement(*CMainFrame::GetMainFrame(), wpl);
	conf.Write<WINDOWPLACEMENT>(UL_("Display"), UL_("WindowPlacement"), wpl);

	conf.Write<int32>(UL_("Pattern Editor"), UL_("NumClipboards"), mpt::saturate_cast<int32>(PatternClipboard::GetClipboardSize()));

	// Effects
#ifndef NO_DSP
	conf.Write<int32>(UL_("Effects"), UL_("XBassDepth"), m_MegaBassSettings.m_nXBassDepth);
	conf.Write<int32>(UL_("Effects"), UL_("XBassRange"), m_MegaBassSettings.m_nXBassRange);
#endif
#ifndef NO_REVERB
	conf.Write<int32>(UL_("Effects"), UL_("ReverbDepth"), m_ReverbSettings.m_nReverbDepth);
	conf.Write<int32>(UL_("Effects"), UL_("ReverbType"), m_ReverbSettings.m_nReverbType);
#endif
#ifndef NO_DSP
	conf.Write<int32>(UL_("Effects"), UL_("ProLogicDepth"), m_SurroundSettings.m_nProLogicDepth);
	conf.Write<int32>(UL_("Effects"), UL_("ProLogicDelay"), m_SurroundSettings.m_nProLogicDelay);
#endif
#ifndef NO_EQ
	conf.Write<EQPreset>(UL_("Effects"), UL_("EQ_Settings"), m_EqSettings);
	conf.Write<EQPreset>(UL_("Effects"), UL_("EQ_User1"), m_EqUserPresets[0]);
	conf.Write<EQPreset>(UL_("Effects"), UL_("EQ_User2"), m_EqUserPresets[1]);
	conf.Write<EQPreset>(UL_("Effects"), UL_("EQ_User3"), m_EqUserPresets[2]);
	conf.Write<EQPreset>(UL_("Effects"), UL_("EQ_User4"), m_EqUserPresets[3]);
#endif
#ifndef NO_DSP
	conf.Write<int32>(UL_("Effects"), UL_("BitCrushBits"), m_BitCrushSettings.m_Bits);
#endif

	// Display (Colors)
	for(int ncol = 0; ncol < MAX_MODCOLORS; ncol++)
	{
		conf.Write<uint32>(UL_("Display"), MPT_UFORMAT("Color{}")(mpt::ufmt::dec0<2>(ncol)), rgbCustomColors[ncol]);
	}

	// Paths
	// Obsolete, since we always write to Keybindings.mkb now.
	// Older versions of OpenMPT 1.18+ will look for this file if this entry is missing, so removing this entry after having read it is kind of backwards compatible.
	conf.Remove(UL_("Paths"), UL_("Key_Config_File"));

	// Chords
	SaveChords(Chords);

	// Save default macro configuration
	MIDIMacroConfig macros;
	theApp.GetDefaultMidiMacro(macros);
	for(int isfx = 0; isfx < kSFxMacros; isfx++)
	{
		conf.Write<std::string>(UL_("Zxx Macros"), MPT_UFORMAT("SF{}")(mpt::ufmt::HEX(isfx)), macros.SFx[isfx]);
	}
	for(int izxx = 0; izxx < kZxxMacros; izxx++)
	{
		conf.Write<std::string>(UL_("Zxx Macros"), MPT_UFORMAT("Z{}")(mpt::ufmt::HEX0<2>(izxx | 0x80)), macros.Zxx[izxx]);
	}

	// MRU list
	for(uint32 i = 0; i < (ID_MRU_LIST_LAST - ID_MRU_LIST_FIRST + 1); i++)
	{
		mpt::ustring key = MPT_UFORMAT("File{}")(i);

		if(i < mruFiles.size())
		{
			mpt::PathString path = mruFiles[i];
			if(theApp.IsPortableMode())
			{
				path = theApp.PathAbsoluteToInstallRelative(path);
			}
			conf.Write<mpt::PathString>(UL_("Recent File List"), key, path);
		} else
		{
			conf.Remove(UL_("Recent File List"), key);
		}
	}
}


bool TrackerSettings::IsComponentBlocked(const std::string &key)
{
	return Setting<bool>(conf, UL_("Components"), UL_("Block") + mpt::ToUnicode(mpt::Charset::ASCII, key), ComponentManagerSettingsDefault().IsBlocked(key));
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


mpt::PathString TrackerSettings::GetDefaultAutosavePath()
{
	mpt::PathString path;
	if(theApp.IsPortableMode())
	{
		path = theApp.GetInstallPath().WithTrailingSlash();
	} else
	{
		// Try to find non-roaming (local) app data first, fall back to other directory
		TCHAR dir[MAX_PATH] = {0};
		if((SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, dir) == S_OK)
		   || (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, dir) == S_OK)
		   || (SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, dir) == S_OK))
		{
			path = mpt::PathString::FromNative(dir);
		} else
		{
			path = mpt::common_directories::get_temp_directory();
		}
		path = path.WithTrailingSlash() + P_("OpenMPT\\");
	}
	return path + P_("Autosave");
}


////////////////////////////////////////////////////////////////////////////////
// Chords

void TrackerSettings::LoadChords(MPTChords &chords)
{
	for(std::size_t i = 0; i < chords.size(); i++)
	{
		uint32 chord;
		mpt::ustring noteName = MPT_UFORMAT("{}{}")(mpt::ustring(NoteNamesSharp[i % 12]), i / 12);
		if((chord = conf.Read<int32>(UL_("Chords"), noteName, -1)) != uint32(-1))
		{
			if((chord & 0xFFFFFFC0) || chords[i].notes[0] == MPTChord::noNote)
			{
				chords[i].key = (uint8)(chord & 0x3F);
				int shift = 6;
				for(auto &note : chords[i].notes)
				{
					// Extract 6 bits and sign-extend to 8 bits
					const int signBit = ((chord >> (shift + 5)) & 1);
					note = static_cast<MPTChord::NoteType>(((chord >> shift) & 0x3F) | (0xC0 * signBit));
					shift += 6;
					if(note == 0)
						note = MPTChord::noNote;
					else if(note > 0)
						note--;
				}
			}
		}
	}
}


void TrackerSettings::SaveChords(MPTChords &chords)
{
	for(std::size_t i = 0; i < chords.size(); i++)
	{
		auto notes = chords[i].notes;
		for(auto &note : notes)
		{
			if(note == MPTChord::noNote)
				note = 0;
			else if(note >= 0)
				note++;
			note &= 0x3F;
		}
		int32 s = (chords[i].key) | (notes[0] << 6) | (notes[1] << 12) | (notes[2] << 18);
		mpt::ustring noteName = MPT_UFORMAT("{}{}")(mpt::ustring(NoteNamesSharp[i % 12]), i / 12);
		conf.Write<int32>(UL_("Chords"), noteName, s);
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
		if(CString(mic.szPname) == deviceName)
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
				cc += UL_(",");
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
		int ccNumber = mpt::parse<int>(ccToken);
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
	mpt::PathString newPath = (stripFilename ? filenameFrom.GetDirectoryWithDrive() : filenameFrom);
	newPath = newPath.WithTrailingSlash();
	mpt::PathString oldPath = dest;
	dest = newPath;
	return newPath != oldPath;
}

ConfigurableDirectory::ConfigurableDirectory(SettingsContainer &conf, const AnyStringLocale &section, const AnyStringLocale &key, const mpt::PathString &def)
	: conf(conf)
	, m_Setting(conf, section, key, def)
{
	SetDefaultDir(theApp.PathInstallRelativeToAbsolute(m_Setting), false);
}

ConfigurableDirectory::~ConfigurableDirectory()
{
	m_Setting = theApp.IsPortableMode() ? theApp.PathAbsoluteToInstallRelative(m_Default) : m_Default;
}



OPENMPT_NAMESPACE_END
