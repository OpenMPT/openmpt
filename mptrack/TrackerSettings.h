/*
 * TrackerSettings.h
 * -----------------
 * Purpose: Header file for application setting handling.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/Logging.h"
#include "../common/version.h"
#include "../soundbase/SampleFormat.h"
#include "../soundlib/MixerSettings.h"
#include "../soundlib/Resampler.h"
#include "../sounddsp/EQ.h"
#include "../sounddsp/DSP.h"
#include "../sounddsp/Reverb.h"
#include "../sounddev/SoundDevice.h"
#include "StreamEncoder.h"
#include "Settings.h"

#include <bitset>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {
class Manager;
} // namespace SoundDevice


// User-defined colors
enum
{
	MODCOLOR_BACKNORMAL = 0,
	MODCOLOR_TEXTNORMAL,
	MODCOLOR_BACKCURROW,
	MODCOLOR_TEXTCURROW,
	MODCOLOR_BACKSELECTED,
	MODCOLOR_TEXTSELECTED,
	MODCOLOR_SAMPLE,
	MODCOLOR_BACKPLAYCURSOR,
	MODCOLOR_TEXTPLAYCURSOR,
	MODCOLOR_BACKHILIGHT,
	MODCOLOR_NOTE,
	MODCOLOR_INSTRUMENT,
	MODCOLOR_VOLUME,
	MODCOLOR_PANNING,
	MODCOLOR_PITCH,
	MODCOLOR_GLOBALS,
	MODCOLOR_ENVELOPES,
	MODCOLOR_VUMETER_LO,
	MODCOLOR_VUMETER_MED,
	MODCOLOR_VUMETER_HI,
	MODCOLOR_SEPSHADOW,
	MODCOLOR_SEPFACE,
	MODCOLOR_SEPHILITE,
	MODCOLOR_BLENDCOLOR,
	MODCOLOR_DODGY_COMMANDS,
	MODCOLOR_BACKSAMPLE,
	MODCOLOR_SAMPLESELECTED,
	MODCOLOR_BACKENV,
	MODCOLOR_VUMETER_LO_VST,
	MODCOLOR_VUMETER_MED_VST,
	MODCOLOR_VUMETER_HI_VST,
	MAX_MODCOLORS,
	// Internal color codes (not saved to color preset files)
	MODCOLOR_2NDHIGHLIGHT,
	MODCOLOR_DEFAULTVOLUME,
	MAX_MODPALETTECOLORS
};


// Pattern Setup (contains also non-pattern related settings)
// Feel free to replace the deprecated flags by new flags, but be sure to
// update TrackerSettings::TrackerSettings() as well.
#define PATTERN_PLAYNEWNOTE			0x01		// play new notes while recording
#define PATTERN_SMOOTHSCROLL		0x02		// scroll tick by tick, not row by row
#define PATTERN_STDHIGHLIGHT		0x04		// enable primary highlight (measures)
//#define PATTERN_SMALLFONT			0x08		// use small font in pattern editor
#define PATTERN_CENTERROW			0x10		// always center active row
#define PATTERN_WRAP				0x20		// wrap around cursor in editor
#define PATTERN_EFFECTHILIGHT		0x40		// effect syntax highlighting
#define PATTERN_HEXDISPLAY			0x80		// display row number in hex
#define PATTERN_FLATBUTTONS			0x100		// flat toolbar buttons
#define PATTERN_CREATEBACKUP		0x200		// create .bak files when saving
#define PATTERN_SINGLEEXPAND		0x400		// single click to expand tree
#define PATTERN_PLAYEDITROW			0x800		// play all notes on the current row while entering notes
#define PATTERN_NOEXTRALOUD			0x1000		// no loud samples in sample editor
#define PATTERN_DRAGNDROPEDIT		0x2000		// enable drag and drop editing
#define PATTERN_2NDHIGHLIGHT		0x4000		// activate secondary highlight (beats)
#define PATTERN_MUTECHNMODE			0x8000		// ignore muted channels
#define PATTERN_SHOWPREVIOUS		0x10000		// show prev/next patterns
#define PATTERN_CONTSCROLL			0x20000		// continous pattern scrolling
#define PATTERN_KBDNOTEOFF			0x40000		// Record note-off events
#define PATTERN_FOLLOWSONGOFF		0x80000		// follow song off by default
#define PATTERN_PLAYTRANSPOSE		0x100000	// Preview note transposition
#define PATTERN_NOCLOSEDIALOG		0x200000	// Don't use OpenMPT's custom close dialog with a list of saved files when closing the main window
#define PATTERN_DBLCLICKSELECT		0x400000	// Double-clicking pattern selects whole channel
#define PATTERN_OLDCTXMENUSTYLE		0x800000	// Hide pattern context menu entries instead of greying them out.
#define PATTERN_SYNCMUTE			0x1000000	// maintain sample sync on mute
#define PATTERN_AUTODELAY			0x2000000	// automatically insert delay commands in pattern when entering notes
#define PATTERN_NOTEFADE			0x4000000	// alt. note fade behaviour when entering notes
#define PATTERN_OVERFLOWPASTE		0x8000000	// continue paste in the next pattern instead of cutting off
#define PATTERN_SHOWDEFAULTVOLUME	0x10000000	// if there is no volume command next to note+instr, display the sample's default volume.
#define PATTERN_RESETCHANNELS		0x20000000	// reset channels when looping
#define PATTERN_LIVEUPDATETREE		0x40000000	// update active sample / instr icons in treeview
#define PATTERN_SYNCSAMPLEPOS		0x80000000	// sync sample positions when seeking

#define PATTERNFONT_SMALL "@1"
#define PATTERNFONT_LARGE "@2"

// MIDI Setup
#define MIDISETUP_RECORDVELOCITY			0x01	// Record MIDI velocity
#define MIDISETUP_TRANSPOSEKEYBOARD			0x02	// Apply transpose value to MIDI Notes
#define MIDISETUP_MIDITOPLUG				0x04	// Pass MIDI messages to plugins
#define MIDISETUP_MIDIVOL_TO_NOTEVOL		0x08	// Combine MIDI volume to note velocity
#define MIDISETUP_RECORDNOTEOFF				0x10	// Record MIDI Note Off to pattern
#define MIDISETUP_RESPONDTOPLAYCONTROLMSGS	0x20	// Respond to Restart/Continue/Stop MIDI commands
#define MIDISETUP_MIDIMACROCONTROL			0x80	// Record MIDI controller changes a MIDI macro changes in pattern
#define MIDISETUP_PLAYPATTERNONMIDIIN		0x100	// Play pattern if MIDI Note is received and playback is paused
#define MIDISETUP_ENABLE_RECORD_DEFAULT		0x200	// Enable MIDI recording by default
#define MIDISETUP_MIDIMACROPITCHBEND		0x400	// Record MIDI pitch bend messages a MIDI macro changes in pattern


// EQ

struct EQPresetPacked
{
	char     szName[12]; // locale encoding
	uint32le Gains[MAX_EQ_BANDS];
	uint32le Freqs[MAX_EQ_BANDS];
};
MPT_BINARY_STRUCT(EQPresetPacked, 60)

struct EQPreset
{
	char   szName[12]; // locale encoding
	uint32 Gains[MAX_EQ_BANDS];
	uint32 Freqs[MAX_EQ_BANDS];
};

static const EQPreset FlatEQPreset = { "Flat", {16,16,16,16,16,16}, { 125, 300, 600, 1250, 4000, 8000 } };

template<> inline SettingValue ToSettingValue(const EQPreset &val)
{
	EQPresetPacked valpacked;
	std::memcpy(valpacked.szName, val.szName, CountOf(valpacked.szName));
	std::copy(val.Gains, val.Gains + MAX_EQ_BANDS, valpacked.Gains);
	std::copy(val.Freqs, val.Freqs + MAX_EQ_BANDS, valpacked.Freqs);
	return SettingValue(EncodeBinarySetting<EQPresetPacked>(valpacked), "EQPreset");
}
template<> inline EQPreset FromSettingValue(const SettingValue &val)
{
	ASSERT(val.GetTypeTag() == "EQPreset");
	EQPresetPacked valpacked = DecodeBinarySetting<EQPresetPacked>(val.as<std::vector<mpt::byte> >());
	EQPreset valresult;
	std::memcpy(valresult.szName, valpacked.szName, CountOf(valresult.szName));
	std::copy(valpacked.Gains, valpacked.Gains + MAX_EQ_BANDS, valresult.Gains);
	std::copy(valpacked.Freqs, valpacked.Freqs + MAX_EQ_BANDS, valresult.Freqs);
	return valresult;
}



// Chords
struct MPTChord
{
	enum
	{
		notesPerChord = 4,
		relativeMode = 0x3F,
	};

	uint8 key;			// Base note
	uint8 notes[3];		// Additional chord notes
};

typedef MPTChord MPTChords[3 * 12];	// 3 octaves

// MIDI recording
enum RecordAftertouchOptions
{
	atDoNotRecord = 0,
	atRecordAsVolume,
	atRecordAsMacro,
};

// New file action
enum NewFileAction
{
	nfDefaultFormat = 0,
	nfSameAsCurrent,
	nfDefaultTemplate
};

// Sample editor preview behaviour
enum SampleEditorKeyBehaviour
{
	seNoteOffOnNewKey = 0,
	seNoteOffOnKeyUp,
	seNoteOffOnKeyRestrike,
};

enum SampleEditorDefaultFormat
{
	dfFLAC,
	dfWAV,
	dfRAW,
};


class SampleUndoBufferSize
{
protected:
	size_t sizeByte;
	int32 sizePercent;

	void CalculateSize();

public:
	enum
	{
		defaultSize = 10,	// In percent
	};

	SampleUndoBufferSize(int32 percent = defaultSize) : sizePercent(percent) { CalculateSize(); }
	void Set(int32 percent) { sizePercent = percent; CalculateSize(); }

	int32 GetSizeInPercent() const { return sizePercent; }
	size_t GetSizeInBytes() const { return sizeByte; }
};

template<> inline SettingValue ToSettingValue(const SampleUndoBufferSize &val) { return SettingValue(val.GetSizeInPercent()); }
template<> inline SampleUndoBufferSize FromSettingValue(const SettingValue &val) { return SampleUndoBufferSize(val.as<int32>()); }


std::string IgnoredCCsToString(const std::bitset<128> &midiIgnoreCCs);
std::bitset<128> StringToIgnoredCCs(const std::string &in);

std::string SettingsModTypeToString(MODTYPE modtype);
MODTYPE SettingsStringToModType(const std::string &str);


template<> inline SettingValue ToSettingValue(const RecordAftertouchOptions &val) { return SettingValue(int32(val)); }
template<> inline RecordAftertouchOptions FromSettingValue(const SettingValue &val) { return RecordAftertouchOptions(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const SampleEditorKeyBehaviour &val) { return SettingValue(int32(val)); }
template<> inline SampleEditorKeyBehaviour FromSettingValue(const SettingValue &val) { return SampleEditorKeyBehaviour(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const MODTYPE &val) { return SettingValue(SettingsModTypeToString(val), "MODTYPE"); }
template<> inline MODTYPE FromSettingValue(const SettingValue &val) { ASSERT(val.GetTypeTag() == "MODTYPE"); return SettingsStringToModType(val.as<std::string>()); }

template<> inline SettingValue ToSettingValue(const PLUGVOLUMEHANDLING &val)
{
	return SettingValue(int32(val), "PLUGVOLUMEHANDLING");
}
template<> inline PLUGVOLUMEHANDLING FromSettingValue(const SettingValue &val)
{
	ASSERT(val.GetTypeTag() == "PLUGVOLUMEHANDLING");
	if((uint32)val.as<int32>() > PLUGIN_VOLUMEHANDLING_MAX)
	{
		return PLUGIN_VOLUMEHANDLING_IGNORE;
	}
	return static_cast<PLUGVOLUMEHANDLING>(val.as<int32>());
}

template<> inline SettingValue ToSettingValue(const std::vector<uint32> &val) { return mpt::String::Combine(val, MPT_USTRING(",")); }
template<> inline std::vector<uint32> FromSettingValue(const SettingValue &val) { return mpt::String::Split<uint32>(val, MPT_USTRING(",")); }

template<> inline SettingValue ToSettingValue(const SampleFormat &val) { return SettingValue(int32(val.value)); }
template<> inline SampleFormat FromSettingValue(const SettingValue &val) { return SampleFormatEnum(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const SoundDevice::ChannelMapping &val) { return SettingValue(val.ToString(), "ChannelMapping"); }
template<> inline SoundDevice::ChannelMapping FromSettingValue(const SettingValue &val) { ASSERT(val.GetTypeTag() == "ChannelMapping"); return SoundDevice::ChannelMapping::FromString(val.as<mpt::ustring>()); }

template<> inline SettingValue ToSettingValue(const ResamplingMode &val) { return SettingValue(int32(val)); }
template<> inline ResamplingMode FromSettingValue(const SettingValue &val) { return ResamplingMode(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const NewFileAction &val) { return SettingValue(int32(val)); }
template<> inline NewFileAction FromSettingValue(const SettingValue &val) { return NewFileAction(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const std::bitset<128> &val)
{
	return SettingValue(IgnoredCCsToString(val), "IgnoredCCs");
}
template<> inline std::bitset<128> FromSettingValue(const SettingValue &val)
{
	ASSERT(val.GetTypeTag() == "IgnoredCCs");
	return StringToIgnoredCCs(val.as<std::string>());
}

template<> inline SettingValue ToSettingValue(const SampleEditorDefaultFormat &val)
{
	const char *format;
	switch(val)
	{
	case dfWAV:
		format = "wav";
		break;
	case dfFLAC:
	default:
		format = "flac";
		break;
	case dfRAW:
		format = "raw";
	}
	return SettingValue(format);
}
template<> inline SampleEditorDefaultFormat FromSettingValue(const SettingValue &val)
{
	std::string format = mpt::ToLowerCaseAscii(val.as<std::string>());
	if(format == "wav")
		return dfWAV;
	if(format == "raw")
		return dfRAW;
	else // if(format == "flac")
		return dfFLAC;
}

enum SoundDeviceStopMode
{
	SoundDeviceStopModeClosed  = 0,
	SoundDeviceStopModeStopped = 1,
	SoundDeviceStopModePlaying = 2,
};

template<> inline SettingValue ToSettingValue(const SoundDeviceStopMode &val)
{
	return SettingValue(static_cast<int32>(val));
}
template<> inline SoundDeviceStopMode FromSettingValue(const SettingValue &val)
{
	return static_cast<SoundDeviceStopMode>(static_cast<int32>(val));
}


enum ProcessPriorityClass
{
	ProcessPriorityClassIDLE     = IDLE_PRIORITY_CLASS,
	ProcessPriorityClassBELOW    = BELOW_NORMAL_PRIORITY_CLASS,
	ProcessPriorityClassNORMAL   = NORMAL_PRIORITY_CLASS,
	ProcessPriorityClassABOVE    = ABOVE_NORMAL_PRIORITY_CLASS,
	ProcessPriorityClassHIGH     = HIGH_PRIORITY_CLASS,
	ProcessPriorityClassREALTIME = REALTIME_PRIORITY_CLASS
};
template<> inline SettingValue ToSettingValue(const ProcessPriorityClass &val)
{
	mpt::ustring s;
	switch(val)
	{
	case ProcessPriorityClassIDLE:
		s = MPT_USTRING("idle");
		break;
	case ProcessPriorityClassBELOW:
		s = MPT_USTRING("below");
		break;
	case ProcessPriorityClassNORMAL:
		s = MPT_USTRING("normal");
		break;
	case ProcessPriorityClassABOVE:
		s = MPT_USTRING("above");
		break;
	case ProcessPriorityClassHIGH:
		s = MPT_USTRING("high");
		break;
	case ProcessPriorityClassREALTIME:
		s = MPT_USTRING("realtime");
		break;
	default:
		s = MPT_USTRING("normal");
		break;
	}
	return SettingValue(s);
}
template<> inline ProcessPriorityClass FromSettingValue(const SettingValue &val)
{
	ProcessPriorityClass result = ProcessPriorityClassNORMAL;
	mpt::ustring s = val.as<mpt::ustring>();
	if(s.empty())
	{
		result = ProcessPriorityClassNORMAL;
	} else if(s == MPT_USTRING("idle"))
	{
		result = ProcessPriorityClassIDLE;
	} else if(s == MPT_USTRING("below"))
	{
		result = ProcessPriorityClassBELOW;
	} else if(s == MPT_USTRING("normal"))
	{
		result = ProcessPriorityClassNORMAL;
	} else if(s == MPT_USTRING("above"))
	{
		result = ProcessPriorityClassABOVE;
	} else if(s == MPT_USTRING("high"))
	{
		result = ProcessPriorityClassHIGH;
	} else if(s == MPT_USTRING("realtime"))
	{
		result = ProcessPriorityClassREALTIME;
	} else
	{
		result = ProcessPriorityClassNORMAL;
	}
	return result;
}


template<> inline SettingValue ToSettingValue(const mpt::Date::Unix &val)
{
	time_t t = val;
	const tm* lastUpdate = gmtime(&t);
	CString outDate;
	if(lastUpdate)
	{
		outDate.Format(_T("%04d-%02d-%02d %02d:%02d"), lastUpdate->tm_year + 1900, lastUpdate->tm_mon + 1, lastUpdate->tm_mday, lastUpdate->tm_hour, lastUpdate->tm_min);
	}
	return SettingValue(outDate, "UTC");
}
template<> inline mpt::Date::Unix FromSettingValue(const SettingValue &val)
{
	MPT_ASSERT(val.GetTypeTag() == "UTC");
	std::string s = val.as<std::string>();
	tm lastUpdate;
	MemsetZero(lastUpdate);
	if(sscanf(s.c_str(), "%04d-%02d-%02d %02d:%02d", &lastUpdate.tm_year, &lastUpdate.tm_mon, &lastUpdate.tm_mday, &lastUpdate.tm_hour, &lastUpdate.tm_min) == 5)
	{
		lastUpdate.tm_year -= 1900;
		lastUpdate.tm_mon--;
	}
	time_t outTime = mpt::Date::Unix::FromUTC(lastUpdate);
	if(outTime < 0)
	{
		outTime = 0;
	}
	return mpt::Date::Unix(outTime);
}

struct FontSetting
{
	enum FontFlags
	{
		None = 0,
		Bold = 1,
		Italic = 2,
	};

	std::string name;
	int32_t size;
	FlagSet<FontFlags> flags;

	FontSetting(const FontSetting &other) : name(other.name), size(other.size), flags(other.flags) { }
	FontSetting(const std::string &name = "", int32_t size = 120, FontFlags flags = None) : name(name), size(size), flags(flags) { }

	bool operator== (const FontSetting &other) const
	{
		return name == other.name && size == other.size && flags == other.flags;
	}

	bool operator!= (const FontSetting &other) const
	{
		return !(*this == other);
	}
};

MPT_DECLARE_ENUM(FontSetting::FontFlags)

template<> inline SettingValue ToSettingValue(const FontSetting &val)
{
	return SettingValue(val.name + "," + mpt::ToString(val.size) + "|" + mpt::ToString(val.flags.GetRaw()));
}
template<> inline FontSetting FromSettingValue(const SettingValue &val)
{
	FontSetting setting(val.as<std::string>());
	size_t sizeStart = setting.name.rfind(',');
	if(sizeStart != std::string::npos)
	{
		setting.size = atoi(&setting.name[sizeStart + 1]);
		size_t flagsStart = setting.name.find('|', sizeStart + 1);
		if(flagsStart != std::string::npos)
		{
			setting.flags = static_cast<FontSetting::FontFlags>(atoi(&setting.name[flagsStart + 1]));
		}
		setting.name.resize(sizeStart);
	}

	return setting;
}


class DefaultAndWorkingDirectory
{
protected:
	mpt::PathString m_Default;
	mpt::PathString m_Working;
public:
	DefaultAndWorkingDirectory();
	DefaultAndWorkingDirectory(const mpt::PathString &def);
	~DefaultAndWorkingDirectory();
public:
	void SetDefaultDir(const mpt::PathString &filenameFrom, bool stripFilename = false);
	void SetWorkingDir(const mpt::PathString &filenameFrom, bool stripFilename = false);
	mpt::PathString GetDefaultDir() const;
	mpt::PathString GetWorkingDir() const;
private:
	bool InternalSet(mpt::PathString &dest, const mpt::PathString &filenameFrom, bool stripFilename);
};

class ConfigurableDirectory
	: public DefaultAndWorkingDirectory
{
protected:
	SettingsContainer &conf;
	Setting<mpt::PathString> m_Setting;
public:
	ConfigurableDirectory(SettingsContainer &conf, const AnyStringLocale &section, const AnyStringLocale &key, const mpt::PathString &def);
	~ConfigurableDirectory();
};


//=================
class DebugSettings
//=================
{

private:

	SettingsContainer &conf;

private:

	// Debug

#if !defined(NO_LOGGING) && !defined(MPT_LOG_IS_DISABLED)
	Setting<int> DebugLogLevel;
	Setting<std::string> DebugLogFacilitySolo;
	Setting<std::string> DebugLogFacilityBlocked;
	Setting<bool> DebugLogFileEnable;
	Setting<bool> DebugLogDebuggerEnable;
	Setting<bool> DebugLogConsoleEnable;
#endif

	Setting<bool> DebugTraceEnable;
	Setting<uint32> DebugTraceSize;
	Setting<bool> DebugTraceAlwaysDump;

	Setting<bool> DebugStopSoundDeviceOnCrash;
	Setting<bool> DebugStopSoundDeviceBeforeDump;

public:

	DebugSettings(SettingsContainer &conf);

	~DebugSettings();

};


//===================
class TrackerSettings
//===================
{

private:
	SettingsContainer &conf;

public:

	// Version

	Setting<std::string> IniVersion;
	const bool FirstRun;
	const MptVersion::VersionNum PreviousSettingsVersion;
	Setting<mpt::ustring> gcsInstallGUID;

	// Display

	Setting<bool> m_ShowSplashScreen;
	Setting<bool> gbMdiMaximize;
	Setting<bool> highResUI;
	Setting<LONG> glTreeSplitRatio;
	Setting<LONG> glTreeWindowWidth;
	Setting<LONG> glGeneralWindowHeight;
	Setting<LONG> glPatternWindowHeight;
	Setting<LONG> glSampleWindowHeight;
	Setting<LONG> glInstrumentWindowHeight;
	Setting<LONG> glCommentsWindowHeight;
	Setting<LONG> glGraphWindowHeight;

	Setting<int32> gnPlugWindowX;
	Setting<int32> gnPlugWindowY;
	Setting<int32> gnPlugWindowWidth;
	Setting<int32> gnPlugWindowHeight;
	Setting<int32> gnPlugWindowLast;	// Last selected plugin ID

	Setting<uint32> gnMsgBoxVisiblityFlags;
	Setting<uint32> GUIUpdateInterval;
	CachedSetting<uint32> FSUpdateInterval;
	CachedSetting<uint32> VuMeterUpdateInterval;
	CachedSetting<float> VuMeterDecaySpeedDecibelPerSecond;

	CachedSetting<bool> accidentalFlats;
	Setting<bool> rememberSongWindows;
	Setting<bool> showDirsInSampleBrowser;

	Setting<FontSetting> commentsFont;

	// Misc

	Setting<bool> ShowSettingsOnNewVersion;
	Setting<MODTYPE> defaultModType;
	Setting<NewFileAction> defaultNewFileAction;
	Setting<PLUGVOLUMEHANDLING> DefaultPlugVolumeHandling;
	Setting<bool> autoApplySmoothFT2Ramping;
	CachedSetting<uint32> MiscITCompressionStereo; // Mask: bit0: IT, bit1: Compat IT, bit2: MPTM
	CachedSetting<uint32> MiscITCompressionMono;   // Mask: bit0: IT, bit1: Compat IT, bit2: MPTM
	CachedSetting<bool> MiscSaveChannelMuteStatus;
	CachedSetting<bool> MiscAllowMultipleCommandsPerKey;
	CachedSetting<bool> MiscDistinguishModifiers;
	Setting<ProcessPriorityClass> MiscProcessPriorityClass;

	// Sound Settings

	Setting<bool> m_SoundShowDeprecatedDevices;
	Setting<bool> m_SoundShowNotRecommendedDeviceWarning;
	Setting<std::vector<uint32> > m_SoundSampleRates;
	Setting<bool> m_MorePortaudio;
	Setting<bool> m_SoundSettingsOpenDeviceAtStartup;
	Setting<SoundDeviceStopMode> m_SoundSettingsStopMode;

	bool m_SoundDeviceSettingsUseOldDefaults;
	SoundDevice::Legacy::ID m_SoundDeviceID_DEPRECATED;
	SoundDevice::Settings m_SoundDeviceSettingsDefaults;
	SoundDevice::Settings GetSoundDeviceSettingsDefaults() const;
	bool m_SoundDeviceDirectSoundOldDefaultIdentifier;

	Setting<SoundDevice::Identifier> m_SoundDeviceIdentifier;
	Setting<bool> m_SoundDevicePreferSameTypeIfDeviceUnavailable;
	SoundDevice::Identifier GetSoundDeviceIdentifier() const;
	void SetSoundDeviceIdentifier(const SoundDevice::Identifier &identifier);
	SoundDevice::Settings GetSoundDeviceSettings(const SoundDevice::Identifier &device) const;
	void SetSoundDeviceSettings(const SoundDevice::Identifier &device, const SoundDevice::Settings &settings);

	Setting<uint32> MixerMaxChannels;
	Setting<uint32> MixerDSPMask;
	Setting<uint32> MixerFlags;
	Setting<uint32> MixerSamplerate;
	Setting<uint32> MixerOutputChannels;
	Setting<uint32> MixerPreAmp;
	Setting<uint32> MixerStereoSeparation;
	Setting<uint32> MixerVolumeRampUpMicroseconds;
	Setting<uint32> MixerVolumeRampDownMicroseconds;
	MixerSettings GetMixerSettings() const;
	void SetMixerSettings(const MixerSettings &settings);

	Setting<ResamplingMode> ResamplerMode;
	Setting<uint8> ResamplerSubMode;
	Setting<int32> ResamplerCutoffPercent;
	CResamplerSettings GetResamplerSettings() const;
	void SetResamplerSettings(const CResamplerSettings &settings);

	Setting<int> SoundBoostedThreadPriority;
	Setting<mpt::ustring> SoundBoostedThreadMMCSSClass;
	Setting<bool> SoundBoostedThreadRealtimePosix;
	Setting<int> SoundBoostedThreadNicenessPosix;
	Setting<int> SoundBoostedThreadRtprioPosix;

	// MIDI Settings

	Setting<UINT> m_nMidiDevice;
	Setting<CString> midiDeviceName;
	// FIXME: MIDI recording is currently done in its own callback/thread and
	// accesses settings framework from in there. Work-around the ASSERTs for
	// now by using cached settings.
	CachedSetting<uint32> m_dwMidiSetup;
	CachedSetting<RecordAftertouchOptions> aftertouchBehaviour;
	CachedSetting<uint16> midiVelocityAmp;
	CachedSetting<std::bitset<128> > midiIgnoreCCs;

	Setting<uint32> midiImportPatternLen;
	Setting<uint32> midiImportQuantize;
	Setting<uint8> midiImportTicks;

	// Pattern Editor

	Setting<bool> gbLoopSong;
	CachedSetting<UINT> gnPatternSpacing;
	CachedSetting<bool> gbPatternVUMeters;
	CachedSetting<bool> gbPatternPluginNames;
	CachedSetting<bool> gbPatternRecord;
	CachedSetting<bool> patternNoEditPopup;
	CachedSetting<bool> patternStepCommands;
	CachedSetting<uint32> m_dwPatternSetup;
	CachedSetting<uint32> m_nRowHighlightMeasures; // primary (measures) and secondary (beats) highlight
	CachedSetting<uint32> m_nRowHighlightBeats;	// primary (measures) and secondary (beats) highlight
	CachedSetting<ROWINDEX> recordQuantizeRows;
	CachedSetting<UINT> gnAutoChordWaitTime;
	CachedSetting<int32> orderlistMargins;
	CachedSetting<int32> rowDisplayOffset;
	Setting<FontSetting> patternFont;
	Setting<int32> effectVisWidth;
	Setting<int32> effectVisHeight;

	// Sample Editor

	Setting<SampleUndoBufferSize> m_SampleUndoBufferSize;
	Setting<SampleEditorKeyBehaviour> sampleEditorKeyBehaviour;
	Setting<SampleEditorDefaultFormat> m_defaultSampleFormat;
	Setting<ResamplingMode> sampleEditorDefaultResampler;
	Setting<int32> m_nFinetuneStep;	// Increment finetune by x cents when using spin control.
	Setting<int32> m_FLACCompressionLevel;	// FLAC compression level for saving (0...8)
	Setting<bool> compressITI;
	Setting<bool> m_MayNormalizeSamplesOnLoad;
	Setting<bool> previewInFileDialogs;

	// Export

	Setting<bool> ExportDefaultToSoundcardSamplerate;
	StreamEncoderSettings ExportStreamEncoderSettings;

	// Components

	Setting<bool> ComponentsLoadOnStartup;
	Setting<bool> ComponentsKeepLoaded;
	bool IsComponentBlocked(const std::string &name);

	// Effects

#ifndef NO_REVERB
	CReverbSettings m_ReverbSettings;
#endif
#ifndef NO_DSP
	CSurroundSettings m_SurroundSettings;
#endif
#ifndef NO_DSP
	CMegaBassSettings m_MegaBassSettings;
#endif
#ifndef NO_EQ
	EQPreset m_EqSettings;
	EQPreset m_EqUserPresets[4];
#endif

	// Display (Colors)

	COLORREF rgbCustomColors[MAX_MODCOLORS];

	// AutoSave

	CachedSetting<bool> AutosaveEnabled;
	CachedSetting<uint32> AutosaveIntervalMinutes;
	CachedSetting<uint32> AutosaveHistoryDepth;
	CachedSetting<bool> AutosaveUseOriginalPath;
	ConfigurableDirectory AutosavePath;
	
	// Paths

	ConfigurableDirectory PathSongs;
	ConfigurableDirectory PathSamples;
	ConfigurableDirectory PathInstruments;
	ConfigurableDirectory PathPlugins;
	ConfigurableDirectory PathPluginPresets;
	ConfigurableDirectory PathExport;
	DefaultAndWorkingDirectory PathTunings;
	DefaultAndWorkingDirectory PathUserTemplates;
	mpt::PathString m_szKbdFile;

	// Default template

	Setting<mpt::PathString> defaultTemplateFile;
	Setting<mpt::ustring> defaultArtist;

	Setting<uint32> mruListLength;
	std::vector<mpt::PathString> mruFiles;

	// Chords

	MPTChords Chords;

	// Plugins

	Setting<bool> bridgeAllPlugins;
	Setting<bool> enableAutoSuspend;
	CachedSetting<bool> midiMappingInPluginEditor;
	Setting<std::wstring> pluginProjectPath;
	CachedSetting<std::string> vstHostProductString;
	CachedSetting<std::string> vstHostVendorString;
	CachedSetting<int32> vstHostVendorVersion;

	// Update

	Setting<mpt::Date::Unix> UpdateLastUpdateCheck;
	Setting<int32> UpdateUpdateCheckPeriod;
	Setting<CString> UpdateUpdateURL;
	Setting<bool> UpdateSendGUID;
	Setting<bool> UpdateShowUpdateHint;
	Setting<bool> UpdateSuggestDifferentBuildVariant;
	Setting<CString> UpdateIgnoreVersion;

	// Wine support

	Setting<bool> WineSupportEnabled;
	Setting<bool> WineSupportInitialQuestionAsked;
	Setting<bool> WineSupportAlwaysRecompile;
	Setting<bool> WineSupportAskCompile;
	Setting<int32> WineSupportCompileVerbosity;
	Setting<bool> WineSupportForeignOpenMPT;
	Setting<bool> WineSupportAllowNonLinux;
	Setting<int32> WineSupportEnablePulseAudio; // 0==off 1==auto 2==on
	Setting<int32> WineSupportEnablePortAudio; // 0==off 1==auto 2==on

public:

	TrackerSettings(SettingsContainer &conf);

	void MigrateOldSoundDeviceSettings(SoundDevice::Manager &manager);

	void SaveSettings();

	static void GetDefaultColourScheme(COLORREF (&colours)[MAX_MODCOLORS]);

	std::vector<uint32> GetSampleRates() const;

	static MPTChords &GetChords() { return Instance().Chords; }

	// Get settings object singleton
	static TrackerSettings &Instance();

	void SetMIDIDevice(UINT id);
	UINT GetCurrentMIDIDevice();

protected:

	static std::vector<uint32> GetDefaultSampleRates();

	void FixupEQ(EQPreset &eqSettings);

	void LoadChords(MPTChords &chords);
	void SaveChords(MPTChords &chords);

};



OPENMPT_NAMESPACE_END
