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

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/uuid/uuid.hpp"

#include "../common/Logging.h"
#include "../common/version.h"
#include "openmpt/soundbase/SampleFormat.hpp"
#include "../soundlib/MixerSettings.h"
#include "../soundlib/Resampler.h"
#include "../sounddsp/EQ.h"
#include "../sounddsp/DSP.h"
#include "../sounddsp/Reverb.h"
#include "mpt/format/join.hpp"
#include "mpt/parse/parse.hpp"
#include "mpt/parse/split.hpp"
#include "openmpt/sounddevice/SoundDevice.hpp"
#include "StreamEncoderSettings.h"
#include "Settings.h"

#include <array>
#include <bitset>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {
class Manager;
} // namespace SoundDevice


namespace Tuning {
class CTuningCollection;
} // namespace Tuning
using CTuningCollection = Tuning::CTuningCollection;


// User-defined colors
enum ModColor : uint8
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
	MODCOLOR_ENVELOPE_RELEASE,
	MODCOLOR_SAMPLE_LOOPMARKER,
	MODCOLOR_SAMPLE_SUSTAINMARKER,
	MODCOLOR_SAMPLE_CUEPOINT,
	MODCOLOR_BACKRECORDROW,
	MAX_MODCOLORS,
	// Internal color codes (not saved to color preset files)
	MODCOLOR_2NDHIGHLIGHT,
	MODCOLOR_DEFAULTVOLUME,
	MODCOLOR_DUMMYCOMMAND,
	MAX_MODPALETTECOLORS
};


// Pattern Setup (contains also non-pattern related settings)
// Feel free to replace the deprecated flags by new flags, but be sure to
// update TrackerSettings::TrackerSettings() as well.
enum class PatternSetup : uint32
{
	PlayNewNotesWhileRecording =       0x01,  // play new notes while recording
	SmoothScrolling            =       0x02,  // scroll tick by tick, not row by row
	HighlightMeasures          =       0x04,  // enable primary highlight (measures)
	DisableFollowOnClick       =       0x08,  // disable song follow when clicking into pattern
	CenterActiveRow            =       0x10,  // always center active row
	CursorWrap                 =       0x20,  // wrap around cursor in editor
	EffectHighlight            =       0x40,  // effect syntax highlighting
	RowAndOrderNumbersHex      =       0x80,  // display row number and order in hex
	FlatToolbarButtons         =      0x100,  // flat toolbar buttons
	PlayRowOnNavigate          =      0x200,  // play whole row when navigating
	SingleClickToExpand        =      0x400,  // single click to expand tree
	PlayRowOnNoteEntry         =      0x800,  // play all notes on the current row while entering notes
	NoLoudSamplePreview        =     0x1000,  // no loud samples in sample editor
	DragNDropEdit              =     0x2000,  // enable drag and drop editing
	HighlightBeats             =     0x4000,  // activate secondary highlight (beats)
	IgnoreMutedChannels        =     0x8000,  // ignore muted channels
	ShowPrevNextPattern        =    0x10000,  // show prev/next patterns
	ContinuousScrolling        =    0x20000,  // continous pattern scrolling
	RecordNoteOff              =    0x40000,  // Record note-off events
	FollowSongOffByDefault     =    0x80000,  // follow song off by default
	PreviewNoteTransposition   =   0x100000,  // Preview note transposition
	NoCustomCloseDialog        =   0x200000,  // Don't use OpenMPT's custom close dialog with a list of saved files when closing the main window
	DblClickSelectsChannel     =   0x400000,  // Double-clicking pattern selects whole channel
	HideUnavailableMenuEntries =   0x800000,  // Hide pattern context menu entries instead of greying them out.
	SyncMute                   =  0x1000000,  // maintain sample sync on mute
	AutoDelayCommands          =  0x2000000,  // automatically insert delay commands in pattern when entering notes
	NoteFadeOnKeyUp            =  0x4000000,  // alt. note fade behaviour when entering notes
	OverflowPaste              =  0x8000000,  // continue paste in the next pattern instead of cutting off
	ShowDefaultVolume          = 0x10000000,  // if there is no volume command next to note+instr, display the sample's default volume.
	ResetChannelsOnLoop        = 0x20000000,  // reset channels when looping
	LiveUpdateTreeView         = 0x40000000,  // update active sample / instr icons in treeview
	SampleSyncOnSeek           = 0x80000000,  // sync sample positions when seeking

	Default = PlayNewNotesWhileRecording | EffectHighlight
			| CenterActiveRow | DragNDropEdit
			| FlatToolbarButtons | NoLoudSamplePreview | HighlightBeats
			| HighlightMeasures | ShowPrevNextPattern | ContinuousScrolling
			| SyncMute | AutoDelayCommands | NoteFadeOnKeyUp
			| ShowDefaultVolume | LiveUpdateTreeView | SampleSyncOnSeek
};
DECLARE_FLAGSET(PatternSetup)

#define PATTERNFONT_SMALL UL_("@1")
#define PATTERNFONT_LARGE UL_("@2")

// MIDI Setup
enum class MidiSetup : int32
{
	RecordVelocity               =  0x01,  // Record MIDI velocity
	TransposeKeyboard            =  0x02,  // Apply transpose value to MIDI Notes
	SendMidiToPlugins            =  0x04,  // Pass MIDI messages to plugins
	ApplyChannelVolumeToVelocity =  0x08,  // Combine MIDI volume to note velocity
	RecordNoteOff                =  0x10,  // Record MIDI Note Off to pattern
	RespondToPlayControl         =  0x20,  // Respond to Restart/Continue/Stop MIDI commands
	RecordCCsAsMacros            =  0x80,  // Record MIDI controller changes a MIDI macro changes in pattern
	PlayPatternOnMidiNote        = 0x100,  // Play pattern if MIDI Note is received and playback is paused
	EnableMidiInOnStartup        = 0x200,  // Enable MIDI recording by default
	PlayPatternFromStart         = 0x800,  // When continuing playback due to received MIDI events, restart pattern instead of continuing from current row

	Default = RecordVelocity | RecordNoteOff | TransposeKeyboard | SendMidiToPlugins
};
DECLARE_FLAGSET(MidiSetup)


#ifndef NO_EQ

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


template<> inline SettingValue ToSettingValue(const EQPreset &val)
{
	EQPresetPacked valpacked;
	std::memcpy(valpacked.szName, val.szName, std::size(valpacked.szName));
	std::copy(val.Gains, val.Gains + MAX_EQ_BANDS, valpacked.Gains);
	std::copy(val.Freqs, val.Freqs + MAX_EQ_BANDS, valpacked.Freqs);
	return SettingValue(EncodeBinarySetting<EQPresetPacked>(valpacked), "EQPreset");
}
template<> inline EQPreset FromSettingValue(const SettingValue &val)
{
	ASSERT(val.GetTypeTag() == "EQPreset");
	EQPresetPacked valpacked = DecodeBinarySetting<EQPresetPacked>(val.as<std::vector<std::byte>>());
	EQPreset valresult;
	std::memcpy(valresult.szName, valpacked.szName, std::size(valresult.szName));
	std::copy(valpacked.Gains, valpacked.Gains + MAX_EQ_BANDS, valresult.Gains);
	std::copy(valpacked.Freqs, valpacked.Freqs + MAX_EQ_BANDS, valresult.Freqs);
	return valresult;
}

#endif // !NO_EQ


template<> inline SettingValue ToSettingValue(const mpt::UUID &val) { return SettingValue(val.ToUString()); }
template<> inline mpt::UUID FromSettingValue(const SettingValue &val) { return mpt::UUID::FromString(val.as<mpt::ustring>()); }



// Chords
struct MPTChord
{
	enum
	{
		notesPerChord = 4,
		relativeMode = 0x3F,
		noNote = int8_min,
	};
	using NoteType = int8;

	uint8 key;  // Base note
	std::array<NoteType, notesPerChord - 1> notes;  // Additional chord notes
};

using MPTChords = std::array<MPTChord, 60>;  // Size == kcCommandSetNumNotes + 1

// MIDI recording
enum class RecordAftertouch
{
	DoNotRecord = 0,
	RecordAsVolume,
	RecordAsMacro,
};

enum class RecordPitchBend
{
	DoNotRecord = 0,
	RecordAsMacro,
	RecordAsFinetuneOrMacro,
	RecordAsFinetune,
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
	dfS3I,
	dfIFF,
};

enum class FollowSamplePlayCursor
{
	DoNotFollow = 0,
	Follow,
	FollowCentered,
	MaxOptions
};

enum class TimelineFormat
{
	Seconds = 0,
	Samples,
	SamplesPow2,
};

enum class DefaultChannelColors
{
	NoColors = 0,
	Rainbow,
	Random,
};

enum class DPIAwarenessMode
{
	NoDPIAwareness = 0,
	NoDPIAwarenessGDIUpscaled,
	SystemDPIAware,
	PerMonitorDPIAware,
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
		defaultSize = 10,  // In percent
	};

	SampleUndoBufferSize(int32 percent = defaultSize) : sizePercent(percent) { CalculateSize(); }
	void Set(int32 percent) { sizePercent = percent; CalculateSize(); }

	int32 GetSizeInPercent() const { return sizePercent; }
	size_t GetSizeInBytes() const { return sizeByte; }
};

template<> inline SettingValue ToSettingValue(const SampleUndoBufferSize &val) { return SettingValue(val.GetSizeInPercent()); }
template<> inline SampleUndoBufferSize FromSettingValue(const SettingValue &val) { return SampleUndoBufferSize(val.as<int32>()); }


struct LastPluginID
{
	uint32 pluginID1, pluginID2, shellPluginID;
};

template<> inline SettingValue ToSettingValue(const LastPluginID &val) { return EncodeBinarySetting(val); }
template<> inline LastPluginID FromSettingValue(const SettingValue &val) { return DecodeBinarySetting<LastPluginID>(val.as<std::vector<std::byte>>()); }


mpt::ustring IgnoredCCsToString(const std::bitset<128> &midiIgnoreCCs);
std::bitset<128> StringToIgnoredCCs(const mpt::ustring &in);

mpt::ustring SettingsModTypeToString(MODTYPE modtype);
MODTYPE SettingsStringToModType(const mpt::ustring &str);


template<> inline SettingValue ToSettingValue(const RecordAftertouch &val) { return SettingValue(int32(val)); }
template<> inline RecordAftertouch FromSettingValue(const SettingValue &val) { return RecordAftertouch(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const RecordPitchBend &val) { return SettingValue(int32(val)); }
template<> inline RecordPitchBend FromSettingValue(const SettingValue& val) { return RecordPitchBend(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const SampleEditorKeyBehaviour &val) { return SettingValue(int32(val)); }
template<> inline SampleEditorKeyBehaviour FromSettingValue(const SettingValue &val) { return SampleEditorKeyBehaviour(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const FollowSamplePlayCursor &val) { return SettingValue(int32(val)); }
template<> inline FollowSamplePlayCursor FromSettingValue(const SettingValue& val) { return FollowSamplePlayCursor(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const TimelineFormat &val) { return SettingValue(int32(val)); }
template<> inline TimelineFormat FromSettingValue(const SettingValue &val) { return TimelineFormat(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const DefaultChannelColors & val) { return SettingValue(int32(val)); }
template<> inline DefaultChannelColors FromSettingValue(const SettingValue& val) { return DefaultChannelColors(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const DPIAwarenessMode &val) { return SettingValue(int32(val)); }
template<> inline DPIAwarenessMode FromSettingValue(const SettingValue &val) { return DPIAwarenessMode(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const FlagSet<PatternSetup>& val) { return SettingValue(int32(val.GetRaw())); }
template<> inline FlagSet<PatternSetup> FromSettingValue(const SettingValue& val) { return PatternSetup(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const FlagSet<MidiSetup> &val) { return SettingValue(int32(val.GetRaw())); }
template<> inline FlagSet<MidiSetup> FromSettingValue(const SettingValue &val) { return MidiSetup(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const MODTYPE &val) { return SettingValue(SettingsModTypeToString(val), "MODTYPE"); }
template<> inline MODTYPE FromSettingValue(const SettingValue &val) { ASSERT(val.GetTypeTag() == "MODTYPE"); return SettingsStringToModType(val.as<mpt::ustring>()); }

template<> inline SettingValue ToSettingValue(const PlugVolumeHandling &val)
{
	return SettingValue(int32(val), "PlugVolumeHandling");
}
template<> inline PlugVolumeHandling FromSettingValue(const SettingValue &val)
{
	ASSERT(val.GetTypeTag() == "PlugVolumeHandling");
	if((uint32)val.as<int32>() > PLUGIN_VOLUMEHANDLING_MAX)
	{
		return PLUGIN_VOLUMEHANDLING_IGNORE;
	}
	return static_cast<PlugVolumeHandling>(val.as<int32>());
}

template<> inline SettingValue ToSettingValue(const std::vector<uint32> &val) { return mpt::join_format(val, U_(",")); }
template<> inline std::vector<uint32> FromSettingValue(const SettingValue &val) { return mpt::split_parse<uint32>(val.as<mpt::ustring>(), U_(",")); }

template<> inline SettingValue ToSettingValue(const std::vector<mpt::ustring> &val) { return mpt::join_format(val, U_(";")); }
template<> inline std::vector<mpt::ustring> FromSettingValue(const SettingValue &val) { return mpt::split(val.as<mpt::ustring>(), U_(";")); }

template<> inline SettingValue ToSettingValue(const SampleFormat &val) { return SettingValue(SampleFormat::ToInt(val)); }
template<> inline SampleFormat FromSettingValue(const SettingValue &val) { return SampleFormat::FromInt(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const SoundDevice::ChannelMapping &val) { return SettingValue(val.ToUString(), "ChannelMapping"); }
template<> inline SoundDevice::ChannelMapping FromSettingValue(const SettingValue &val) { ASSERT(val.GetTypeTag() == "ChannelMapping"); return SoundDevice::ChannelMapping::FromString(val.as<mpt::ustring>()); }

template<> inline SettingValue ToSettingValue(const ResamplingMode &val) { return SettingValue(int32(val)); }
template<> inline ResamplingMode FromSettingValue(const SettingValue &val) { return ResamplingMode(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const Resampling::AmigaFilter &val) { return SettingValue(int32(val)); }
template<> inline Resampling::AmigaFilter FromSettingValue(const SettingValue &val) { return static_cast<Resampling::AmigaFilter>(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const NewFileAction &val) { return SettingValue(int32(val)); }
template<> inline NewFileAction FromSettingValue(const SettingValue &val) { return NewFileAction(val.as<int32>()); }

enum class MainToolBarItem : uint8
{
	Octave = 0x01,
	Tempo = 0x02,
	Speed = 0x04,
	RowsPerBeat = 0x08,
	GlobalVolume = 0x10,
	VUMeter = 0x20,

	Default = Octave | Tempo | Speed | RowsPerBeat | GlobalVolume | VUMeter
};
DECLARE_FLAGSET(MainToolBarItem)

template<> inline SettingValue ToSettingValue(const MainToolBarItem &val) { return SettingValue(static_cast<int32>(val)); }
template<> inline MainToolBarItem FromSettingValue(const SettingValue &val) { return static_cast<MainToolBarItem>(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const std::bitset<128> &val)
{
	return SettingValue(IgnoredCCsToString(val), "IgnoredCCs");
}
template<> inline std::bitset<128> FromSettingValue(const SettingValue &val)
{
	ASSERT(val.GetTypeTag() == "IgnoredCCs");
	return StringToIgnoredCCs(val.as<mpt::ustring>());
}

template<> inline SettingValue ToSettingValue(const SampleEditorDefaultFormat &val)
{
	mpt::ustring format;
	switch(val)
	{
	case dfWAV:
		format = U_("wav");
		break;
	case dfFLAC:
	default:
		format = U_("flac");
		break;
	case dfRAW:
		format = U_("raw");
		break;
	case dfS3I:
		format = U_("s3i");
		break;
	case dfIFF:
		format = U_("iff");
		break;
	}
	return SettingValue(format);
}
template<> inline SampleEditorDefaultFormat FromSettingValue(const SettingValue &val)
{
	mpt::ustring format = mpt::ToLowerCase(val.as<mpt::ustring>());
	if(format == U_("wav"))
		return dfWAV;
	if(format == U_("raw"))
		return dfRAW;
	if(format == U_("s3i"))
		return dfS3I;
	if(format == U_("iff"))
		return dfIFF;
	else  // if(format == U_("flac"))
		return dfFLAC;
}

enum SoundDeviceStopMode : int
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
		s = U_("idle");
		break;
	case ProcessPriorityClassBELOW:
		s = U_("below");
		break;
	case ProcessPriorityClassNORMAL:
		s = U_("normal");
		break;
	case ProcessPriorityClassABOVE:
		s = U_("above");
		break;
	case ProcessPriorityClassHIGH:
		s = U_("high");
		break;
	case ProcessPriorityClassREALTIME:
		s = U_("realtime");
		break;
	default:
		s = U_("normal");
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
	} else if(s == U_("idle"))
	{
		result = ProcessPriorityClassIDLE;
	} else if(s == U_("below"))
	{
		result = ProcessPriorityClassBELOW;
	} else if(s == U_("normal"))
	{
		result = ProcessPriorityClassNORMAL;
	} else if(s == U_("above"))
	{
		result = ProcessPriorityClassABOVE;
	} else if(s == U_("high"))
	{
		result = ProcessPriorityClassHIGH;
	} else if(s == U_("realtime"))
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
	return SettingValue(mpt::ufmt::val(mpt::Date::UnixAsSeconds(val)), "UnixTime");
}
template<> inline mpt::Date::Unix FromSettingValue(const SettingValue &val)
{
	MPT_ASSERT(val.GetTypeTag() == "UnixTime");
	return mpt::Date::UnixFromSeconds(mpt::parse<int64>(val.as<mpt::ustring>()));
}

struct FontSetting
{
	enum FontFlags
	{
		None = 0,
		Bold = 1,
		Italic = 2,
	};

	mpt::ustring name;
	int32 size;
	FlagSet<FontFlags> flags;

	FontSetting(const mpt::ustring &name = U_(""), int32 size = 120, FontFlags flags = None) : name(name), size(size), flags(flags) { }

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
	return SettingValue(mpt::ToUnicode(val.name) + U_(",") + mpt::ufmt::val(val.size) + U_("|") + mpt::ufmt::val(val.flags.GetRaw()));
}
template<> inline FontSetting FromSettingValue(const SettingValue &val)
{
	FontSetting setting(val.as<mpt::ustring>());
	std::size_t sizeStart = setting.name.rfind(UC_(','));
	if(sizeStart != std::string::npos)
	{
		const std::vector<mpt::ustring> fields = mpt::split(setting.name.substr(sizeStart + 1), U_("|"));
		if(fields.size() >= 1)
		{
			setting.size = mpt::parse<int32>(fields[0]);
		}
		if(fields.size() >= 2)
		{
			setting.flags = static_cast<FontSetting::FontFlags>(mpt::parse<int32>(fields[1]));
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


class DebugSettings
{

private:

	SettingsContainer &conf;

private:

	// Debug

#if !defined(MPT_LOG_IS_DISABLED)
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

	Setting<bool> DebugDelegateToWindowsHandler;

public:

	DebugSettings(SettingsContainer &conf_);

	~DebugSettings();

};


namespace SoundDevice
{
namespace Legacy
{
using ID = uint16;
inline constexpr SoundDevice::Legacy::ID MaskType = 0xff00;
inline constexpr SoundDevice::Legacy::ID MaskIndex = 0x00ff;
inline constexpr int ShiftType = 8;
inline constexpr int ShiftIndex = 0;
inline constexpr SoundDevice::Legacy::ID TypeWAVEOUT          = 0;
inline constexpr SoundDevice::Legacy::ID TypeDSOUND           = 1;
inline constexpr SoundDevice::Legacy::ID TypeASIO             = 2;
inline constexpr SoundDevice::Legacy::ID TypePORTAUDIO_WASAPI = 3;
inline constexpr SoundDevice::Legacy::ID TypePORTAUDIO_WDMKS  = 4;
inline constexpr SoundDevice::Legacy::ID TypePORTAUDIO_WMME   = 5;
inline constexpr SoundDevice::Legacy::ID TypePORTAUDIO_DS     = 6;
} // namespace Legacy
} // namespace SoundDevice


class TrackerSettings
{

private:
	SettingsContainer &conf;

public:

	// Version

	Setting<mpt::ustring> IniVersion;
	const bool FirstRun;
	const Version PreviousSettingsVersion;
	Setting<mpt::UUID> VersionInstallGUID;

	// Display

	Setting<bool> m_ShowSplashScreen;
	Setting<bool> gbMdiMaximize;
	Setting<DPIAwarenessMode> dpiAwareness;
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
	Setting<LastPluginID> lastSelectedPlugin;

	Setting<uint32> gnMsgBoxVisiblityFlags;
	Setting<uint32> GUIUpdateInterval;
	CachedSetting<uint32> FSUpdateInterval;
	CachedSetting<uint32> VuMeterUpdateInterval;
	CachedSetting<float> VuMeterDecaySpeedDecibelPerSecond;

	CachedSetting<bool> accidentalFlats;
	Setting<bool> rememberSongWindows;
	Setting<bool> showDirsInSampleBrowser;
	Setting<bool> useOldStyleFolderBrowser;
	Setting<DefaultChannelColors> defaultRainbowChannelColors;

	Setting<FontSetting> commentsFont;

	CachedSetting<MainToolBarItem> mainToolBarVisibleItems;
	Setting<bool> treeViewOnLeft;

	// Misc

	Setting<MODTYPE> defaultModType;
	Setting<NewFileAction> defaultNewFileAction;
	Setting<PlugVolumeHandling> DefaultPlugVolumeHandling;
	Setting<bool> autoApplySmoothFT2Ramping;
	CachedSetting<uint32> MiscITCompressionStereo; // Mask: bit0: IT, bit1: Compat IT, bit2: MPTM
	CachedSetting<uint32> MiscITCompressionMono;   // Mask: bit0: IT, bit1: Compat IT, bit2: MPTM
	CachedSetting<bool> MiscSaveChannelMuteStatus;
	CachedSetting<bool> MiscAllowMultipleCommandsPerKey;
	CachedSetting<bool> MiscDistinguishModifiers;
	Setting<ProcessPriorityClass> MiscProcessPriorityClass;
	CachedSetting<bool> MiscFlushFileBuffersOnSave;
	CachedSetting<bool> MiscCacheCompleteFileBeforeLoading;
	Setting<bool> MiscUseSingleInstance;

	// Sound Settings

	bool m_SoundShowRecordingSettings;
	Setting<bool> m_SoundShowDeprecatedDevices;
	Setting<bool> m_SoundDeprecatedDeviceWarningShown;
	Setting<std::vector<uint32> > m_SoundSampleRates;
	Setting<bool> m_SoundSettingsOpenDeviceAtStartup;
	Setting<SoundDeviceStopMode> m_SoundSettingsStopMode;

	bool m_SoundDeviceSettingsUseOldDefaults;
	SoundDevice::Legacy::ID m_SoundDeviceID_DEPRECATED;
	SoundDevice::Settings m_SoundDeviceSettingsDefaults;
	SoundDevice::Settings GetSoundDeviceSettingsDefaults() const;
#if defined(MPT_WITH_DIRECTSOUND)
	bool m_SoundDeviceDirectSoundOldDefaultIdentifier;
#endif // MPT_WITH_DIRECTSOUND

	Setting<SoundDevice::Identifier> m_SoundDeviceIdentifier;
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
	Setting<uint32> MixerNumInputChannels;
	MixerSettings GetMixerSettings() const;
	void SetMixerSettings(const MixerSettings &settings);

	Setting<ResamplingMode> ResamplerMode;
	Setting<uint8> ResamplerSubMode;
	Setting<int32> ResamplerCutoffPercent;
	Setting<Resampling::AmigaFilter> ResamplerEmulateAmiga;
	CResamplerSettings GetResamplerSettings() const;
	void SetResamplerSettings(const CResamplerSettings &settings);

	Setting<int> SoundBoostedThreadPriority;
	Setting<mpt::ustring> SoundBoostedThreadMMCSSClass;
	Setting<bool> SoundBoostedThreadRealtimePosix;
	Setting<int> SoundBoostedThreadNicenessPosix;
	Setting<int> SoundBoostedThreadRtprioPosix;
	Setting<bool> SoundMaskDriverCrashes;
	Setting<bool> SoundAllowDeferredProcessing;

	// MIDI Settings

	Setting<UINT> m_nMidiDevice;
	Setting<CString> midiDeviceName;
	// FIXME: MIDI recording is currently done in its own callback/thread and
	// accesses settings framework from in there. Work-around the ASSERTs for
	// now by using cached settings.
	CachedSetting<FlagSet<MidiSetup>> midiSetup;
	CachedSetting<RecordAftertouch> aftertouchBehaviour;
	CachedSetting<RecordPitchBend> pitchBendBehaviour;
	CachedSetting<uint16> midiVelocityAmp;
	CachedSetting<std::bitset<128> > midiIgnoreCCs;

	Setting<uint32> midiImportPatternLen;
	Setting<uint32> midiImportQuantize;
	Setting<uint8> midiImportTicks;

	// Pattern Editor

	CachedSetting<bool> gbLoopSong;
	CachedSetting<UINT> gnPatternSpacing;
	CachedSetting<bool> gbPatternVUMeters;
	CachedSetting<bool> gbPatternPluginNames;
	CachedSetting<bool> gbPatternRecord;
	CachedSetting<bool> patternNoEditPopup;
	CachedSetting<bool> patternStepCommands;
	CachedSetting<bool> patternVolColHex;
	CachedSetting<FlagSet<PatternSetup>> patternSetup;
	CachedSetting<uint32> m_nRowHighlightMeasures; // primary (measures) and secondary (beats) highlight
	CachedSetting<uint32> m_nRowHighlightBeats;	// primary (measures) and secondary (beats) highlight
	CachedSetting<bool> patternIgnoreSongTimeSignature;
	CachedSetting<ROWINDEX> recordQuantizeRows;
	CachedSetting<UINT> gnAutoChordWaitTime;
	CachedSetting<int32> orderlistMargins;
	CachedSetting<int32> rowDisplayOffset;
	Setting<FontSetting> patternFont;
	Setting<mpt::ustring> patternFontDot;
	Setting<int32> effectVisWidth;
	Setting<int32> effectVisHeight;
	Setting<int32> effectVisX;
	Setting<int32> effectVisY;
	Setting<CString> patternAccessibilityFormat;
	CachedSetting<bool> patternAlwaysDrawWholePatternOnScrollSlow;
	CachedSetting<bool> orderListOldDropBehaviour;
	Setting<bool> autoHideVolumeColumnForMOD;
	Setting<bool> metronomeEnabled;
	Setting<float> metronomeVolume;
	Setting<mpt::PathString> metronomeSampleMeasure;
	Setting<mpt::PathString> metronomeSampleBeat;

	// Sample Editor

	Setting<SampleUndoBufferSize> m_SampleUndoBufferSize;
	Setting<SampleEditorKeyBehaviour> sampleEditorKeyBehaviour;
	Setting<SampleEditorDefaultFormat> m_defaultSampleFormat;
	CachedSetting<FollowSamplePlayCursor> m_followSamplePlayCursor;
	Setting<TimelineFormat> sampleEditorTimelineFormat;
	Setting<ResamplingMode> sampleEditorDefaultResampler;
	Setting<int32> m_nFinetuneStep;	// Increment finetune by x cents when using spin control.
	Setting<int32> m_FLACCompressionLevel;	// FLAC compression level for saving (0...8)
	Setting<bool> m_FLACMultithreading;
	Setting<bool> compressITI;
	Setting<bool> m_MayNormalizeSamplesOnLoad;
	Setting<bool> previewInFileDialogs;
	CachedSetting<bool> cursorPositionInHex;

	// Export

	Setting<bool> ExportDefaultToSoundcardSamplerate;
	StreamEncoderSettingsConf ExportStreamEncoderSettings;
	Setting<bool> ExportNormalize;
	Setting<bool> ExportClearPluginBuffers;

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
#ifndef NO_DSP
	BitCrushSettings m_BitCrushSettings;
#endif

	// Display (Colors)

	std::array<COLORREF, MAX_MODCOLORS> rgbCustomColors;

	// AutoSave
	CachedSetting<bool> CreateBackupFiles;
	CachedSetting<bool> AutosaveEnabled;
	CachedSetting<uint32> AutosaveIntervalMinutes;
	CachedSetting<uint32> AutosaveHistoryDepth;
	CachedSetting<uint32> AutosaveRetentionTimeDays;
	CachedSetting<bool> AutosaveUseOriginalPath;
	CachedSetting<bool> AutosaveDeletePermanently;
	ConfigurableDirectory AutosavePath;
	
	// Paths

	ConfigurableDirectory PathSongs;
	ConfigurableDirectory PathSamples;
	ConfigurableDirectory PathInstruments;
	ConfigurableDirectory PathPlugins;
	ConfigurableDirectory PathPluginPresets;
	ConfigurableDirectory PathExport;
	DefaultAndWorkingDirectory PathTunings;
	mpt::PathString m_szKbdFile;

	// Default template

	Setting<mpt::PathString> defaultTemplateFile;
	Setting<mpt::ustring> defaultArtist;

	Setting<uint32> mruListLength;
	std::vector<mpt::PathString> mruFiles;

	// Chords

	MPTChords Chords;

	// Tunings

	std::unique_ptr<CTuningCollection> oldLocalTunings;

	// Plugins

	Setting<bool> bridgeAllPlugins;
	Setting<bool> enableAutoSuspend;
	CachedSetting<bool> midiMappingInPluginEditor;
	Setting<mpt::ustring> pluginProjectPath;
	CachedSetting<mpt::lstring> vstHostProductString;
	CachedSetting<mpt::lstring> vstHostVendorString;
	CachedSetting<int32> vstHostVendorVersion;

	// Broken Plugins Workarounds

	Setting<bool> BrokenPluginsWorkaroundVSTMaskAllCrashes;
	Setting<bool> BrokenPluginsWorkaroundVSTNeverUnloadAnyPlugin;

#if defined(MPT_ENABLE_UPDATE)

	// Update

	Setting<bool> UpdateEnabled;
	Setting<bool> UpdateInstallAutomatically;
	Setting<mpt::Date::Unix> UpdateLastUpdateCheck;
	Setting<int32> UpdateUpdateCheckPeriod_DEPRECATED;
	Setting<int32> UpdateIntervalDays;
	Setting<uint32> UpdateChannel;
	Setting<mpt::ustring> UpdateUpdateURL_DEPRECATED;
	Setting<mpt::ustring> UpdateAPIURL;
	Setting<bool> UpdateStatisticsConsentAsked;
	Setting<bool> UpdateStatistics;
	Setting<bool> UpdateSendGUID_DEPRECATED;
	Setting<bool> UpdateShowUpdateHint;
	Setting<CString> UpdateIgnoreVersion;
	Setting<bool> UpdateSkipSignatureVerificationUNSECURE;
	Setting<std::vector<mpt::ustring>> UpdateSigningKeysRootAnchors;

#endif // MPT_ENABLE_UPDATE

	// Wine support

	Setting<bool> WineSupportEnabled;
	Setting<bool> WineSupportAlwaysRecompile;
	Setting<bool> WineSupportAskCompile;
	Setting<int32> WineSupportCompileVerbosity;
	Setting<bool> WineSupportForeignOpenMPT;
	Setting<bool> WineSupportAllowUnknownHost;
	Setting<int32> WineSupportEnablePulseAudio; // 0==off 1==auto 2==on
	Setting<int32> WineSupportEnablePortAudio; // 0==off 1==auto 2==on
	Setting<int32> WineSupportEnableRtAudio; // 0==off 1==auto 2==on

public:

	TrackerSettings(SettingsContainer &conf);

	~TrackerSettings();

	void MigrateOldSoundDeviceSettings(SoundDevice::Manager &manager);

private:
	void MigrateTunings(const Version storedVersion);
	std::unique_ptr<CTuningCollection> LoadLocalTunings();
public:

	void SaveSettings();

	static void GetDefaultColourScheme(std::array<COLORREF, MAX_MODCOLORS> &colours);

	std::vector<uint32> GetSampleRates() const;

	static MPTChords &GetChords() { return Instance().Chords; }

	// Get settings object singleton
	static TrackerSettings &Instance();

	void SetMIDIDevice(UINT id);
	UINT GetCurrentMIDIDevice();

	static mpt::PathString GetDefaultAutosavePath();

	static mpt::PathString GetDefaultMetronomeSample() { return P_("*sine"); }

protected:

	static std::vector<uint32> GetDefaultSampleRates();

#ifndef NO_EQ

	void FixupEQ(EQPreset &eqSettings);

#endif // !NO_EQ

	void LoadChords(MPTChords &chords);
	void SaveChords(MPTChords &chords);

};



OPENMPT_NAMESPACE_END
