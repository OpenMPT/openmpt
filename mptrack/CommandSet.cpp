/*
 * CommandSet.cpp
 * --------------
 * Purpose: Implementation of custom key handling.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "CommandSet.h"
#include "DefaultKeyBindings.h"
#include "resource.h"
#include "Mptrack.h"	// For ErrorBox
#include "../soundlib/mod_specifications.h"
#include "../soundlib/Tables.h"
#include "../mptrack/Reporting.h"
#include "mpt/io_file/outputfile.hpp"
#include "../common/mptFileIO.h"
#include <sstream>
#include "TrackerSettings.h"
#include "mpt/parse/parse.hpp"
#include "mpt/string/utility.hpp"


OPENMPT_NAMESPACE_BEGIN

namespace
{

constexpr CommandID ModifierCommands[] =
{
	kcSelect, kcCopySelect, kcChordModifier, kcSetSpacing
};

constexpr std::pair<CommandID, CommandID> NoteRanges[] =
{
	{kcVPStartNotes, kcVPStartNoteStops},
	{kcSampStartNotes, kcSampStartNoteStops},
	{kcInstrumentStartNotes, kcInstrumentStartNoteStops},
	{kcTreeViewStartNotes, kcTreeViewStartNoteStops},
	{kcInsNoteMapStartNotes, kcInsNoteMapStartNoteStops},
	{kcVSTGUIStartNotes, kcVSTGUIStartNoteStops},
	{kcCommentsStartNotes, kcCommentsStartNoteStops},
};

std::vector<HKL> GetKeyboardLayouts()
{
	std::vector<HKL> layouts(GetKeyboardLayoutList(0, nullptr));
	GetKeyboardLayoutList(static_cast<int>(layouts.size()), layouts.data());
	// GetKeyboardLayoutList appears to return the layouts in no particular order. Always force the active layout to be evaluated first.
	layouts.insert(layouts.begin(), GetKeyboardLayout(0));
	return layouts;
}

};  // namespace

#ifdef MPT_ALL_LOGGING
#define MPT_COMMANDSET_LOGGING
#endif

#ifdef MPT_COMMANDSET_LOGGING
#define LOG_COMMANDSET(x) MPT_LOG_GLOBAL(LogDebug, "CommandSet", x)
#else
#define LOG_COMMANDSET(x) do { } while(0)
#endif


CCommandSet::CCommandSet()
{
	// Which key binding rules to enforce?
	m_enforceRule[krPreventDuplicate]             = true;
	m_enforceRule[krDeleteOldOnConflict]          = true;
	m_enforceRule[krAllowNavigationWithSelection] = true;
	m_enforceRule[krAllowSelectionWithNavigation] = true;
	m_enforceRule[krAllowSelectCopySelectCombos]  = true;
	m_enforceRule[krLockNotesToChords]            = true;
	m_enforceRule[krNoteOffOnKeyRelease]          = true;
	m_enforceRule[krPropagateNotes]               = true;
	m_enforceRule[krReassignDigitsToOctaves]      = false;
	m_enforceRule[krAutoSelectOff]                = true;
	m_enforceRule[krAutoSpacing]                  = true;
	m_enforceRule[krCheckModifiers]               = true;
	m_enforceRule[krPropagateSampleManipulation]  = true;
//	enforceRule[krCheckContextHierarchy]          = true;

	SetupCommands();
	SetupContextHierarchy();
}


// Setup

KeyCommand::KeyCommand(uint32 uid, const TCHAR *commandName, std::vector<KeyCombination> keys)
    : kcList{std::move(keys)}
    , name{commandName}
    , UID{uid}
{
}

static constexpr struct
{
	uint32 uid = 0;  // ID | Hidden | Dummy
	CommandID cmd = kcNull;
	const TCHAR *description = nullptr;
} CommandDefinitions[] =
// clang-format off
{
	{KeyCommand::Dummy, kcNull, _T("")},
	{1001, kcPatternRecord, _T("Enable Recording")},
	{1002, kcPatternPlayRow, _T("Play Row")},
	{1003, kcCursorCopy, _T("Quick Copy")},
	{1004, kcCursorPaste, _T("Quick Paste")},
	{1005, kcChannelMute, _T("Mute Current Channel")},
	{1006, kcChannelSolo, _T("Solo Current Channel")},
	{1007, kcTransposeUp, _T("Transpose +1")},
	{1008, kcTransposeDown, _T("Transpose -1")},
	{1009, kcTransposeOctUp, _T("Transpose +1 Octave")},
	{1010, kcTransposeOctDown, _T("Transpose -1 Octave")},
	{1011, kcSelectChannel, _T("Select Channel / Select All")},
	{1012, kcPatternAmplify, _T("Amplify Selection")},
	{1013, kcPatternSetInstrument, _T("Apply current instrument")},
	{1014, kcPatternInterpolateVol, _T("Interpolate Volume")},
	{1015, kcPatternInterpolateEffect, _T("Interpolate Effect")},
	{1016, kcPatternVisualizeEffect, _T("Open Effect Visualizer")},
	{1017, kcPatternJumpDownh1, _T("Jump down by measure")},
	{1018, kcPatternJumpUph1, _T("Jump up by measure")},
	{1019, kcPatternSnapDownh1, _T("Snap down to measure")},
	{1020, kcPatternSnapUph1, _T("Snap up to measure")},
	{1021, kcViewGeneral, _T("View General")},
	{1022, kcViewPattern, _T("View Pattern")},
	{1023, kcViewSamples, _T("View Samples")},
	{1024, kcViewInstruments, _T("View Instruments")},
	{1025, kcViewComments, _T("View Comments")},
	{1026, kcPlayPatternFromCursor, _T("Play Pattern from Cursor")},
	{1027, kcPlayPatternFromStart, _T("Play Pattern from Start")},
	{1028, kcPlaySongFromCursor, _T("Play Song from Cursor")},
	{1029, kcPlaySongFromStart, _T("Play Song from Start")},
	{1030, kcPlayPauseSong, _T("Play Song / Pause Song")},
	{1031, kcPauseSong, _T("Pause Song")},
	{1032, kcPrevInstrument, _T("Previous Instrument")},
	{1033, kcNextInstrument, _T("Next Instrument")},
	{1034, kcPrevOrder, _T("Previous Order")},
	{1035, kcNextOrder, _T("Next Order")},
	{1036, kcPrevOctave, _T("Previous Octave")},
	{1037, kcNextOctave, _T("Next Octave")},
	{1038, kcNavigateDown, _T("Navigate down by 1 row")},
	{1039, kcNavigateUp, _T("Navigate up by 1 row")},
	{1040, kcNavigateLeft, _T("Navigate left")},
	{1041, kcNavigateRight, _T("Navigate right")},
	{1042, kcNavigateNextChan, _T("Navigate to next channel")},
	{1043, kcNavigatePrevChan, _T("Navigate to previous channel")},
	{1044, kcHomeHorizontal, _T("Go to first channel")},
	{1045, kcHomeVertical, _T("Go to first row")},
	{1046, kcHomeAbsolute, _T("Go to first row of first channel")},
	{1047, kcEndHorizontal, _T("Go to last channel")},
	{1048, kcEndVertical, _T("Go to last row")},
	{1049, kcEndAbsolute, _T("Go to last row of last channel")},
	{1050, kcSelect, _T("Selection key")},
	{1051, kcCopySelect, _T("Copy select key")},
	{KeyCommand::Hidden, kcSelectOff, _T("Deselect")},
	{KeyCommand::Hidden, kcCopySelectOff, _T("Copy deselect key")},
	{1054, kcNextPattern, _T("Next Pattern")},
	{1055, kcPrevPattern, _T("Previous Pattern")},
	//{1056, kcClearSelection, _T("Wipe selection")},
	{1057, kcClearRow, _T("Clear Row")},
	{1058, kcClearField, _T("Clear Field")},
	{1059, kcClearRowStep, _T("Clear Row and Step")},
	{1060, kcClearFieldStep, _T("Clear Field and Step")},
	{1061, kcDeleteRow, _T("Delete Row(s)")},
	{1062, kcShowNoteProperties, _T("Show Note Properties")},
	{1063, kcShowEditMenu, _T("Show Context (Right-Click) Menu")},
	{1064, kcVPNoteC_0, _T("Base octave C")},
	{1065, kcVPNoteCS0, _T("Base octave C#")},
	{1066, kcVPNoteD_0, _T("Base octave D")},
	{1067, kcVPNoteDS0, _T("Base octave D#")},
	{1068, kcVPNoteE_0, _T("Base octave E")},
	{1069, kcVPNoteF_0, _T("Base octave F")},
	{1070, kcVPNoteFS0, _T("Base octave F#")},
	{1071, kcVPNoteG_0, _T("Base octave G")},
	{1072, kcVPNoteGS0, _T("Base octave G#")},
	{1073, kcVPNoteA_1, _T("Base octave A")},
	{1074, kcVPNoteAS1, _T("Base octave A#")},
	{1075, kcVPNoteB_1, _T("Base octave B")},
	{1076, kcVPNoteC_1, _T("Base octave +1 C")},
	{1077, kcVPNoteCS1, _T("Base octave +1 C#")},
	{1078, kcVPNoteD_1, _T("Base octave +1 D")},
	{1079, kcVPNoteDS1, _T("Base octave +1 D#")},
	{1080, kcVPNoteE_1, _T("Base octave +1 E")},
	{1081, kcVPNoteF_1, _T("Base octave +1 F")},
	{1082, kcVPNoteFS1, _T("Base octave +1 F#")},
	{1083, kcVPNoteG_1, _T("Base octave +1 G")},
	{1084, kcVPNoteGS1, _T("Base octave +1 G#")},
	{1085, kcVPNoteA_2, _T("Base octave +1 A")},
	{1086, kcVPNoteAS2, _T("Base octave +1 A#")},
	{1087, kcVPNoteB_2, _T("Base octave +1 B")},
	{1088, kcVPNoteC_2, _T("Base octave +2 C")},
	{1089, kcVPNoteCS2, _T("Base octave +2 C#")},
	{1090, kcVPNoteD_2, _T("Base octave +2 D")},
	{1091, kcVPNoteDS2, _T("Base octave +2 D#")},
	{1092, kcVPNoteE_2, _T("Base octave +2 E")},
	{1093, kcVPNoteF_2, _T("Base octave +2 F")},
	{1094, kcVPNoteFS2, _T("Base octave +2 F#")},
	{1095, kcVPNoteG_2, _T("Base octave +2 G")},
	{1096, kcVPNoteGS2, _T("Base octave +2 G#")},
	{1097, kcVPNoteA_3, _T("Base octave +2 A")},
	{2070, kcVPNoteAS3, _T("Base octave +2 A#")},
	{2071, kcVPNoteB_3, _T("Base octave +2 B")},
	{2072, kcVPNoteC_3, _T("Base octave +3 C")},
	{2073, kcVPNoteCS3, _T("Base octave +3 C#")},
	{2074, kcVPNoteD_3, _T("Base octave +3 D")},
	{2075, kcVPNoteDS3, _T("Base octave +3 D#")},
	{2076, kcVPNoteE_3, _T("Base octave +3 E")},
	{2077, kcVPNoteF_3, _T("Base octave +3 F")},
	{2078, kcVPNoteFS3, _T("Base octave +3 F#")},
	{2079, kcVPNoteG_3, _T("Base octave +3 G")},
	{2080, kcVPNoteGS3, _T("Base octave +3 G#")},
	{2081, kcVPNoteA_4, _T("Base octave +3 A")},
	{2082, kcVPNoteAS4, _T("Base octave +3 A#")},
	{2083, kcVPNoteB_4, _T("Base octave +3 B")},
	{2084, kcVPNoteC_4, _T("Base octave +4 C")},
	{2085, kcVPNoteCS4, _T("Base octave +4 C#")},
	{2086, kcVPNoteD_4, _T("Base octave +4 D")},
	{2087, kcVPNoteDS4, _T("Base octave +4 D#")},
	{2088, kcVPNoteE_4, _T("Base octave +4 E")},
	{2089, kcVPNoteF_4, _T("Base octave +4 F")},
	{2090, kcVPNoteFS4, _T("Base octave +4 F#")},
	{2091, kcVPNoteG_4, _T("Base octave +4 G")},
	{2092, kcVPNoteGS4, _T("Base octave +4 G#")},
	{2093, kcVPNoteA_5, _T("Base octave +4 A")},
	{2094, kcVPNoteAS5, _T("Base octave +4 A#")},
	{2095, kcVPNoteB_5, _T("Base octave +4 B")},
	{KeyCommand::Hidden, kcVPNoteStopC_0, _T("Stop base octave C")},
	{KeyCommand::Hidden, kcVPNoteStopCS0, _T("Stop base octave C#")},
	{KeyCommand::Hidden, kcVPNoteStopD_0, _T("Stop base octave D")},
	{KeyCommand::Hidden, kcVPNoteStopDS0, _T("Stop base octave D#")},
	{KeyCommand::Hidden, kcVPNoteStopE_0, _T("Stop base octave E")},
	{KeyCommand::Hidden, kcVPNoteStopF_0, _T("Stop base octave F")},
	{KeyCommand::Hidden, kcVPNoteStopFS0, _T("Stop base octave F#")},
	{KeyCommand::Hidden, kcVPNoteStopG_0, _T("Stop base octave G")},
	{KeyCommand::Hidden, kcVPNoteStopGS0, _T("Stop base octave G#")},
	{KeyCommand::Hidden, kcVPNoteStopA_1, _T("Stop base octave A")},
	{KeyCommand::Hidden, kcVPNoteStopAS1, _T("Stop base octave A#")},
	{KeyCommand::Hidden, kcVPNoteStopB_1, _T("Stop base octave B")},
	{KeyCommand::Hidden, kcVPNoteStopC_1, _T("Stop base octave +1 C")},
	{KeyCommand::Hidden, kcVPNoteStopCS1, _T("Stop base octave +1 C#")},
	{KeyCommand::Hidden, kcVPNoteStopD_1, _T("Stop base octave +1 D")},
	{KeyCommand::Hidden, kcVPNoteStopDS1, _T("Stop base octave +1 D#")},
	{KeyCommand::Hidden, kcVPNoteStopE_1, _T("Stop base octave +1 E")},
	{KeyCommand::Hidden, kcVPNoteStopF_1, _T("Stop base octave +1 F")},
	{KeyCommand::Hidden, kcVPNoteStopFS1, _T("Stop base octave +1 F#")},
	{KeyCommand::Hidden, kcVPNoteStopG_1, _T("Stop base octave +1 G")},
	{KeyCommand::Hidden, kcVPNoteStopGS1, _T("Stop base octave +1 G#")},
	{KeyCommand::Hidden, kcVPNoteStopA_2, _T("Stop base octave +1 A")},
	{KeyCommand::Hidden, kcVPNoteStopAS2, _T("Stop base octave +1 A#")},
	{KeyCommand::Hidden, kcVPNoteStopB_2, _T("Stop base octave +1 B")},
	{KeyCommand::Hidden, kcVPNoteStopC_2, _T("Stop base octave +2 C")},
	{KeyCommand::Hidden, kcVPNoteStopCS2, _T("Stop base octave +2 C#")},
	{KeyCommand::Hidden, kcVPNoteStopD_2, _T("Stop base octave +2 D")},
	{KeyCommand::Hidden, kcVPNoteStopDS2, _T("Stop base octave +2 D#")},
	{KeyCommand::Hidden, kcVPNoteStopE_2, _T("Stop base octave +2 E")},
	{KeyCommand::Hidden, kcVPNoteStopF_2, _T("Stop base octave +2 F")},
	{KeyCommand::Hidden, kcVPNoteStopFS2, _T("Stop base octave +2 F#")},
	{KeyCommand::Hidden, kcVPNoteStopG_2, _T("Stop base octave +2 G")},
	{KeyCommand::Hidden, kcVPNoteStopGS2, _T("Stop base octave +2 G#")},
	{KeyCommand::Hidden, kcVPNoteStopA_3, _T("Stop base octave +2 A")},
	{KeyCommand::Hidden, kcVPNoteStopAS3, _T("Stop base octave +2 A#")},
	{KeyCommand::Hidden, kcVPNoteStopB_3, _T("Stop base octave +2 B")},
	{KeyCommand::Hidden, kcVPNoteStopC_3, _T("Stop base octave +3 C")},
	{KeyCommand::Hidden, kcVPNoteStopCS3, _T("Stop base octave +3 C#")},
	{KeyCommand::Hidden, kcVPNoteStopD_3, _T("Stop base octave +3 D")},
	{KeyCommand::Hidden, kcVPNoteStopDS3, _T("Stop base octave +3 D#")},
	{KeyCommand::Hidden, kcVPNoteStopE_3, _T("Stop base octave +3 E")},
	{KeyCommand::Hidden, kcVPNoteStopF_3, _T("Stop base octave +3 F")},
	{KeyCommand::Hidden, kcVPNoteStopFS3, _T("Stop base octave +3 F#")},
	{KeyCommand::Hidden, kcVPNoteStopG_3, _T("Stop base octave +3 G")},
	{KeyCommand::Hidden, kcVPNoteStopGS3, _T("Stop base octave +3 G#")},
	{KeyCommand::Hidden, kcVPNoteStopA_4, _T("Stop base octave +3 A")},
	{KeyCommand::Hidden, kcVPNoteStopAS4, _T("Stop base octave +3 A#")},
	{KeyCommand::Hidden, kcVPNoteStopB_4, _T("Stop base octave +3 B")},
	{KeyCommand::Hidden, kcVPNoteStopC_4, _T("Stop base octave +4 C")},
	{KeyCommand::Hidden, kcVPNoteStopCS4, _T("Stop base octave +4 C#")},
	{KeyCommand::Hidden, kcVPNoteStopD_4, _T("Stop base octave +4 D")},
	{KeyCommand::Hidden, kcVPNoteStopDS4, _T("Stop base octave +4 D#")},
	{KeyCommand::Hidden, kcVPNoteStopE_4, _T("Stop base octave +4 E")},
	{KeyCommand::Hidden, kcVPNoteStopF_4, _T("Stop base octave +4 F")},
	{KeyCommand::Hidden, kcVPNoteStopFS4, _T("Stop base octave +4 F#")},
	{KeyCommand::Hidden, kcVPNoteStopG_4, _T("Stop base octave +4 G")},
	{KeyCommand::Hidden, kcVPNoteStopGS4, _T("Stop base octave +4 G#")},
	{KeyCommand::Hidden, kcVPNoteStopA_5, _T("Stop base octave +4 A")},
	{KeyCommand::Hidden, kcVPNoteStopAS5, _T("Stop base octave +4 A#")},
	{KeyCommand::Hidden, kcVPNoteStopB_5, _T("Stop base octave +4 B")},
	{KeyCommand::Hidden, kcVPChordC_0, _T("Base octave chord C")},
	{KeyCommand::Hidden, kcVPChordCS0, _T("Base octave chord C#")},
	{KeyCommand::Hidden, kcVPChordD_0, _T("Base octave chord D")},
	{KeyCommand::Hidden, kcVPChordDS0, _T("Base octave chord D#")},
	{KeyCommand::Hidden, kcVPChordE_0, _T("Base octave chord E")},
	{KeyCommand::Hidden, kcVPChordF_0, _T("Base octave chord F")},
	{KeyCommand::Hidden, kcVPChordFS0, _T("Base octave chord F#")},
	{KeyCommand::Hidden, kcVPChordG_0, _T("Base octave chord G")},
	{KeyCommand::Hidden, kcVPChordGS0, _T("Base octave chord G#")},
	{KeyCommand::Hidden, kcVPChordA_1, _T("Base octave chord A")},
	{KeyCommand::Hidden, kcVPChordAS1, _T("Base octave chord A#")},
	{KeyCommand::Hidden, kcVPChordB_1, _T("Base octave chord B")},
	{KeyCommand::Hidden, kcVPChordC_1, _T("Base octave +1 chord C")},
	{KeyCommand::Hidden, kcVPChordCS1, _T("Base octave +1 chord C#")},
	{KeyCommand::Hidden, kcVPChordD_1, _T("Base octave +1 chord D")},
	{KeyCommand::Hidden, kcVPChordDS1, _T("Base octave +1 chord D#")},
	{KeyCommand::Hidden, kcVPChordE_1, _T("Base octave +1 chord E")},
	{KeyCommand::Hidden, kcVPChordF_1, _T("Base octave +1 chord F")},
	{KeyCommand::Hidden, kcVPChordFS1, _T("Base octave +1 chord F#")},
	{KeyCommand::Hidden, kcVPChordG_1, _T("Base octave +1 chord G")},
	{KeyCommand::Hidden, kcVPChordGS1, _T("Base octave +1 chord G#")},
	{KeyCommand::Hidden, kcVPChordA_2, _T("Base octave +1 chord A")},
	{KeyCommand::Hidden, kcVPChordAS2, _T("Base octave +1 chord A#")},
	{KeyCommand::Hidden, kcVPChordB_2, _T("Base octave +1 chord B")},
	{KeyCommand::Hidden, kcVPChordC_2, _T("Base octave +2 chord C")},
	{KeyCommand::Hidden, kcVPChordCS2, _T("Base octave +2 chord C#")},
	{KeyCommand::Hidden, kcVPChordD_2, _T("Base octave +2 chord D")},
	{KeyCommand::Hidden, kcVPChordDS2, _T("Base octave +2 chord D#")},
	{KeyCommand::Hidden, kcVPChordE_2, _T("Base octave +2 chord E")},
	{KeyCommand::Hidden, kcVPChordF_2, _T("Base octave +2 chord F")},
	{KeyCommand::Hidden, kcVPChordFS2, _T("Base octave +2 chord F#")},
	{KeyCommand::Hidden, kcVPChordG_2, _T("Base octave +2 chord G")},
	{KeyCommand::Hidden, kcVPChordGS2, _T("Base octave +2 chord G#")},
	{KeyCommand::Hidden, kcVPChordA_3, _T("Base octave +2 chord A")},
	{KeyCommand::Hidden, kcVPChordAS3, _T("Base octave +2 chord A#")},
	{KeyCommand::Hidden, kcVPChordB_3, _T("Base octave +2 chord B")},
	{KeyCommand::Hidden, kcVPChordC_3, _T("Base octave +3 chord C")},
	{KeyCommand::Hidden, kcVPChordCS3, _T("Base octave +3 chord C#")},
	{KeyCommand::Hidden, kcVPChordD_3, _T("Base octave +3 chord D")},
	{KeyCommand::Hidden, kcVPChordDS3, _T("Base octave +3 chord D#")},
	{KeyCommand::Hidden, kcVPChordE_3, _T("Base octave +3 chord E")},
	{KeyCommand::Hidden, kcVPChordF_3, _T("Base octave +3 chord F")},
	{KeyCommand::Hidden, kcVPChordFS3, _T("Base octave +3 chord F#")},
	{KeyCommand::Hidden, kcVPChordG_3, _T("Base octave +3 chord G")},
	{KeyCommand::Hidden, kcVPChordGS3, _T("Base octave +3 chord G#")},
	{KeyCommand::Hidden, kcVPChordA_4, _T("Base octave +3 chord A")},
	{KeyCommand::Hidden, kcVPChordAS4, _T("Base octave +3 chord A#")},
	{KeyCommand::Hidden, kcVPChordB_4, _T("Base octave +3 chord B")},
	{KeyCommand::Hidden, kcVPChordC_4, _T("Base octave +4 chord C")},
	{KeyCommand::Hidden, kcVPChordCS4, _T("Base octave +4 chord C#")},
	{KeyCommand::Hidden, kcVPChordD_4, _T("Base octave +4 chord D")},
	{KeyCommand::Hidden, kcVPChordDS4, _T("Base octave +4 chord D#")},
	{KeyCommand::Hidden, kcVPChordE_4, _T("Base octave +4 chord E")},
	{KeyCommand::Hidden, kcVPChordF_4, _T("Base octave +4 chord F")},
	{KeyCommand::Hidden, kcVPChordFS4, _T("Base octave +4 chord F#")},
	{KeyCommand::Hidden, kcVPChordG_4, _T("Base octave +4 chord G")},
	{KeyCommand::Hidden, kcVPChordGS4, _T("Base octave +4 chord G#")},
	{KeyCommand::Hidden, kcVPChordA_5, _T("Base octave +4 chord A")},
	{KeyCommand::Hidden, kcVPChordAS5, _T("Base octave +4 chord A#")},
	{KeyCommand::Hidden, kcVPChordB_5, _T("Base octave +4 chord B")},
	{KeyCommand::Hidden, kcVPChordStopC_0, _T("Stop base octave chord C")},
	{KeyCommand::Hidden, kcVPChordStopCS0, _T("Stop base octave chord C#")},
	{KeyCommand::Hidden, kcVPChordStopD_0, _T("Stop base octave chord D")},
	{KeyCommand::Hidden, kcVPChordStopDS0, _T("Stop base octave chord D#")},
	{KeyCommand::Hidden, kcVPChordStopE_0, _T("Stop base octave chord E")},
	{KeyCommand::Hidden, kcVPChordStopF_0, _T("Stop base octave chord F")},
	{KeyCommand::Hidden, kcVPChordStopFS0, _T("Stop base octave chord F#")},
	{KeyCommand::Hidden, kcVPChordStopG_0, _T("Stop base octave chord G")},
	{KeyCommand::Hidden, kcVPChordStopGS0, _T("Stop base octave chord G#")},
	{KeyCommand::Hidden, kcVPChordStopA_1, _T("Stop base octave chord A")},
	{KeyCommand::Hidden, kcVPChordStopAS1, _T("Stop base octave chord A#")},
	{KeyCommand::Hidden, kcVPChordStopB_1, _T("Stop base octave chord B")},
	{KeyCommand::Hidden, kcVPChordStopC_1, _T("Stop base octave +1 chord C")},
	{KeyCommand::Hidden, kcVPChordStopCS1, _T("Stop base octave +1 chord C#")},
	{KeyCommand::Hidden, kcVPChordStopD_1, _T("Stop base octave +1 chord D")},
	{KeyCommand::Hidden, kcVPChordStopDS1, _T("Stop base octave +1 chord D#")},
	{KeyCommand::Hidden, kcVPChordStopE_1, _T("Stop base octave +1 chord E")},
	{KeyCommand::Hidden, kcVPChordStopF_1, _T("Stop base octave +1 chord F")},
	{KeyCommand::Hidden, kcVPChordStopFS1, _T("Stop base octave +1 chord F#")},
	{KeyCommand::Hidden, kcVPChordStopG_1, _T("Stop base octave +1 chord G")},
	{KeyCommand::Hidden, kcVPChordStopGS1, _T("Stop base octave +1 chord G#")},
	{KeyCommand::Hidden, kcVPChordStopA_2, _T("Stop base octave +1 chord A")},
	{KeyCommand::Hidden, kcVPChordStopAS2, _T("Stop base octave +1 chord A#")},
	{KeyCommand::Hidden, kcVPChordStopB_2, _T("Stop base octave +1 chord B")},
	{KeyCommand::Hidden, kcVPChordStopC_2, _T("Stop base octave +2 chord C")},
	{KeyCommand::Hidden, kcVPChordStopCS2, _T("Stop base octave +2 chord C#")},
	{KeyCommand::Hidden, kcVPChordStopD_2, _T("Stop base octave +2 chord D")},
	{KeyCommand::Hidden, kcVPChordStopDS2, _T("Stop base octave +2 chord D#")},
	{KeyCommand::Hidden, kcVPChordStopE_2, _T("Stop base octave +2 chord E")},
	{KeyCommand::Hidden, kcVPChordStopF_2, _T("Stop base octave +2 chord F")},
	{KeyCommand::Hidden, kcVPChordStopFS2, _T("Stop base octave +2 chord F#")},
	{KeyCommand::Hidden, kcVPChordStopG_2, _T("Stop base octave +2 chord G")},
	{KeyCommand::Hidden, kcVPChordStopGS2, _T("Stop base octave +2 chord G#")},
	{KeyCommand::Hidden, kcVPChordStopA_3, _T("Stop base octave +2 chord A")},
	{KeyCommand::Hidden, kcVPChordStopAS3, _T("Stop base octave +2 chord A#")},
	{KeyCommand::Hidden, kcVPChordStopB_3, _T("Stop base octave +2 chord B")},
	{KeyCommand::Hidden, kcVPChordStopC_3, _T("Stop base octave +3 chord C")},
	{KeyCommand::Hidden, kcVPChordStopCS3, _T("Stop base octave +3 chord C#")},
	{KeyCommand::Hidden, kcVPChordStopD_3, _T("Stop base octave +3 chord D")},
	{KeyCommand::Hidden, kcVPChordStopDS3, _T("Stop base octave +3 chord D#")},
	{KeyCommand::Hidden, kcVPChordStopE_3, _T("Stop base octave +3 chord E")},
	{KeyCommand::Hidden, kcVPChordStopF_3, _T("Stop base octave +3 chord F")},
	{KeyCommand::Hidden, kcVPChordStopFS3, _T("Stop base octave +3 chord F#")},
	{KeyCommand::Hidden, kcVPChordStopG_3, _T("Stop base octave +3 chord G")},
	{KeyCommand::Hidden, kcVPChordStopGS3, _T("Stop base octave +3 chord G#")},
	{KeyCommand::Hidden, kcVPChordStopA_4, _T("Stop base octave +3 chord A")},
	{KeyCommand::Hidden, kcVPChordStopAS4, _T("Stop base octave +3 chord A#")},
	{KeyCommand::Hidden, kcVPChordStopB_4, _T("Stop base octave +3 chord B")},
	{KeyCommand::Hidden, kcVPChordStopC_4, _T("Stop base octave +4 chord C")},
	{KeyCommand::Hidden, kcVPChordStopCS4, _T("Stop base octave +4 chord C#")},
	{KeyCommand::Hidden, kcVPChordStopD_4, _T("Stop base octave +4 chord D")},
	{KeyCommand::Hidden, kcVPChordStopDS4, _T("Stop base octave +4 chord D#")},
	{KeyCommand::Hidden, kcVPChordStopE_4, _T("Stop base octave +4 chord E")},
	{KeyCommand::Hidden, kcVPChordStopF_4, _T("Stop base octave +4 chord F")},
	{KeyCommand::Hidden, kcVPChordStopFS4, _T("Stop base octave +4 chord F#")},
	{KeyCommand::Hidden, kcVPChordStopG_4, _T("Stop base octave +4 chord G")},
	{KeyCommand::Hidden, kcVPChordStopGS4, _T("Stop base octave +4 chord G#")},
	{KeyCommand::Hidden, kcVPChordStopA_5, _T("Stop base octave +4 chord A")},
	{KeyCommand::Hidden, kcVPChordStopAS5, _T("Stop base octave +4 chord A#")},
	{KeyCommand::Hidden, kcVPChordStopB_5, _T("Stop base octave +4 chord B")},
	{1200, kcNoteCut, _T("Note Cut")},
	{1201, kcNoteOff, _T("Note Off")},
	{1202, kcSetIns0, _T("Set instrument digit 0")},
	{1203, kcSetIns1, _T("Set instrument digit 1")},
	{1204, kcSetIns2, _T("Set instrument digit 2")},
	{1205, kcSetIns3, _T("Set instrument digit 3")},
	{1206, kcSetIns4, _T("Set instrument digit 4")},
	{1207, kcSetIns5, _T("Set instrument digit 5")},
	{1208, kcSetIns6, _T("Set instrument digit 6")},
	{1209, kcSetIns7, _T("Set instrument digit 7")},
	{1210, kcSetIns8, _T("Set instrument digit 8")},
	{1211, kcSetIns9, _T("Set instrument digit 9")},
	{1212, kcSetOctave0, _T("Set octave 0")},
	{1213, kcSetOctave1, _T("Set octave 1")},
	{1214, kcSetOctave2, _T("Set octave 2")},
	{1215, kcSetOctave3, _T("Set octave 3")},
	{1216, kcSetOctave4, _T("Set octave 4")},
	{1217, kcSetOctave5, _T("Set octave 5")},
	{1218, kcSetOctave6, _T("Set octave 6")},
	{1219, kcSetOctave7, _T("Set octave 7")},
	{1220, kcSetOctave8, _T("Set octave 8")},
	{1221, kcSetOctave9, _T("Set octave 9")},
	{1222, kcSetVolume0, _T("Set volume digit 0")},
	{1223, kcSetVolume1, _T("Set volume digit 1")},
	{1224, kcSetVolume2, _T("Set volume digit 2")},
	{1225, kcSetVolume3, _T("Set volume digit 3")},
	{1226, kcSetVolume4, _T("Set volume digit 4")},
	{1227, kcSetVolume5, _T("Set volume digit 5")},
	{1228, kcSetVolume6, _T("Set volume digit 6")},
	{1229, kcSetVolume7, _T("Set volume digit 7")},
	{1230, kcSetVolume8, _T("Set volume digit 8")},
	{1231, kcSetVolume9, _T("Set volume digit 9")},
	{1232, kcSetVolumeVol, _T("Volume Command - Volume")},
	{1233, kcSetVolumePan, _T("Volume Command - Panning")},
	{1234, kcSetVolumeVolSlideUp, _T("Volume Command - Volume Slide Up")},
	{1235, kcSetVolumeVolSlideDown, _T("Volume Command - Volume Slide Down")},
	{1236, kcSetVolumeFineVolUp, _T("Volume Command - Fine Volume Slide Up")},
	{1237, kcSetVolumeFineVolDown, _T("Volume Command - Fine Volume Slide Down")},
	{1238, kcSetVolumeVibratoSpd, _T("Volume Command - Vibrato Speed")},
	{1239, kcSetVolumeVibrato, _T("Volume Command - Vibrato Depth")},
	{1240, kcSetVolumeXMPanLeft, _T("Volume Command - XM Pan Slide Left")},
	{1241, kcSetVolumeXMPanRight, _T("Volume Command - XM Pan Slide Right")},
	{1242, kcSetVolumePortamento, _T("Volume Command - Tone Portamento")},
	{1243, kcSetVolumeITPortaUp, _T("Volume Command - Portamento Up")},
	{1244, kcSetVolumeITPortaDown, _T("Volume Command - Portamento Down")},
	{KeyCommand::Hidden, kcSetVolumeITUnused, _T("Volume Command - Unused")},
	{1246, kcSetVolumeITOffset, _T("Volume Command - Offset")},
	{1247, kcSetFXParam0, _T("Effect Parameter Digit 0")},
	{1248, kcSetFXParam1, _T("Effect Parameter Digit 1")},
	{1249, kcSetFXParam2, _T("Effect Parameter Digit 2")},
	{1250, kcSetFXParam3, _T("Effect Parameter Digit 3")},
	{1251, kcSetFXParam4, _T("Effect Parameter Digit 4")},
	{1252, kcSetFXParam5, _T("Effect Parameter Digit 5")},
	{1253, kcSetFXParam6, _T("Effect Parameter Digit 6")},
	{1254, kcSetFXParam7, _T("Effect Parameter Digit 7")},
	{1255, kcSetFXParam8, _T("Effect Parameter Digit 8")},
	{1256, kcSetFXParam9, _T("Effect Parameter Digit 9")},
	{1257, kcSetFXParamA, _T("Effect Parameter Digit A")},
	{1258, kcSetFXParamB, _T("Effect Parameter Digit B")},
	{1259, kcSetFXParamC, _T("Effect Parameter Digit C")},
	{1260, kcSetFXParamD, _T("Effect Parameter Digit D")},
	{1261, kcSetFXParamE, _T("Effect Parameter Digit E")},
	{1262, kcSetFXParamF, _T("Effect Parameter Digit F")},
	{KeyCommand::Hidden, kcSetFXarp, _T("FX Arpeggio")},
	{KeyCommand::Hidden, kcSetFXportUp, _T("FX Portamento Up")},
	{KeyCommand::Hidden, kcSetFXportDown, _T("FX Portamento Down")},
	{KeyCommand::Hidden, kcSetFXport, _T("FX Tone Portamento")},
	{KeyCommand::Hidden, kcSetFXvibrato, _T("FX Vibrato")},
	{KeyCommand::Hidden, kcSetFXportSlide, _T("FX Portamento + Volume Slide")},
	{KeyCommand::Hidden, kcSetFXvibSlide, _T("FX Vibrato + Volume Slide")},
	{KeyCommand::Hidden, kcSetFXtremolo, _T("FX Tremolo")},
	{KeyCommand::Hidden, kcSetFXpan, _T("FX Pan")},
	{KeyCommand::Hidden, kcSetFXoffset, _T("FX Offset")},
	{KeyCommand::Hidden, kcSetFXvolSlide, _T("FX Volume Slide")},
	{KeyCommand::Hidden, kcSetFXgotoOrd, _T("FX Pattern Jump")},
	{KeyCommand::Hidden, kcSetFXsetVol, _T("FX Set Volume")},
	{KeyCommand::Hidden, kcSetFXgotoRow, _T("FX Break To Row")},
	{KeyCommand::Hidden, kcSetFXretrig, _T("FX Retrigger")},
	{KeyCommand::Hidden, kcSetFXspeed, _T("FX Set Speed")},
	{KeyCommand::Hidden, kcSetFXtempo, _T("FX Set Tempo")},
	{KeyCommand::Hidden, kcSetFXtremor, _T("FX Tremor")},
	{KeyCommand::Hidden, kcSetFXextendedMOD, _T("FX Extended MOD Commands")},
	{KeyCommand::Hidden, kcSetFXextendedS3M, _T("FX Extended S3M Commands")},
	{KeyCommand::Hidden, kcSetFXchannelVol, _T("FX Set Channel Volume")},
	{KeyCommand::Hidden, kcSetFXchannelVols, _T("FX Channel Volume Slide")},
	{KeyCommand::Hidden, kcSetFXglobalVol, _T("FX Set Global Volume")},
	{KeyCommand::Hidden, kcSetFXglobalVols, _T("FX Global Volume Slide")},
	{KeyCommand::Hidden, kcSetFXkeyoff, _T("FX Key Off (XM)")},
	{KeyCommand::Hidden, kcSetFXfineVib, _T("FX Fine Vibrato")},
	{KeyCommand::Hidden, kcSetFXpanbrello, _T("FX Panbrello")},
	{KeyCommand::Hidden, kcSetFXextendedXM, _T("FX Extended XM Commands")},
	{KeyCommand::Hidden, kcSetFXpanSlide, _T("FX Pan Slide")},
	{KeyCommand::Hidden, kcSetFXsetEnvPos, _T("FX Set Envelope Position (XM)")},
	{KeyCommand::Hidden, kcSetFXmacro, _T("FX MIDI Macro")},
	{KeyCommand::Hidden, kcSetFXDummy, _T("FX Dummy") },
	{1294, kcSetFXmacroSlide, _T("Smooth MIDI Macro Slide")},
	{1295, kcSetFXdelaycut, _T("Combined Note Delay and Note Cut")},
	{KeyCommand::Hidden, kcPatternJumpDownh1Select, _T("Jump down by measure select")},
	{KeyCommand::Hidden, kcPatternJumpUph1Select, _T("Jump up by measure select")},
	{KeyCommand::Hidden, kcPatternSnapDownh1Select, _T("Snap down to measure select")},
	{KeyCommand::Hidden, kcPatternSnapUph1Select, _T("Snap up to measure select")},
	{KeyCommand::Hidden, kcNavigateDownSelect, _T("Select to Down")},
	{KeyCommand::Hidden, kcNavigateUpSelect, _T("Select to Up")},
	{KeyCommand::Hidden, kcNavigateLeftSelect, _T("Select to Left")},
	{KeyCommand::Hidden, kcNavigateRightSelect, _T("Select to Right")},
	{KeyCommand::Hidden, kcNavigateNextChanSelect, _T("Select to Next Channel")},
	{KeyCommand::Hidden, kcNavigatePrevChanSelect, _T("Select to Previous Channel")},
	{KeyCommand::Hidden, kcHomeHorizontalSelect, _T("Select to First Channel")},
	{KeyCommand::Hidden, kcHomeVerticalSelect, _T("Select to First Row")},
	{KeyCommand::Hidden, kcHomeAbsoluteSelect, _T("Selecto to First Row / Channel")},
	{KeyCommand::Hidden, kcEndHorizontalSelect, _T("Select to Last Channel")},
	{KeyCommand::Hidden, kcEndVerticalSelect, _T("Select to Last Row")},
	{KeyCommand::Hidden, kcEndAbsoluteSelect, _T("Select to Last Row /channel")},
	{KeyCommand::Hidden, kcSelectWithNav, _T("kcSelectWithNav")},
	{KeyCommand::Hidden, kcSelectOffWithNav, _T("kcSelectOffWithNav")},
	{KeyCommand::Hidden, kcCopySelectWithNav, _T("kcCopySelectWithNav")},
	{KeyCommand::Hidden, kcCopySelectOffWithNav, _T("kcCopySelectOffWithNav")},
	{1316 | KeyCommand::Dummy, kcChordModifier, _T("Chord Modifier")},
	{1317 | KeyCommand::Dummy, kcSetSpacing, _T("Edit Step Modifier")},
	{KeyCommand::Hidden, kcSetSpacing0, _T("Set Edit Step to 0")},
	{KeyCommand::Hidden, kcSetSpacing1, _T("Set Edit Step to 1")},
	{KeyCommand::Hidden, kcSetSpacing2, _T("Set Edit Step to 2")},
	{KeyCommand::Hidden, kcSetSpacing3, _T("Set Edit Step to 3")},
	{KeyCommand::Hidden, kcSetSpacing4, _T("Set Edit Step to 4")},
	{KeyCommand::Hidden, kcSetSpacing5, _T("Set Edit Step to 5")},
	{KeyCommand::Hidden, kcSetSpacing6, _T("Set Edit Step to 6")},
	{KeyCommand::Hidden, kcSetSpacing7, _T("Set Edit Step to 7")},
	{KeyCommand::Hidden, kcSetSpacing8, _T("Set Edit Step to 8")},
	{KeyCommand::Hidden, kcSetSpacing9, _T("Set Edit Step to 9")},
	{KeyCommand::Hidden, kcCopySelectWithSelect, _T("kcCopySelectWithSelect")},
	{KeyCommand::Hidden, kcCopySelectOffWithSelect, _T("kcCopySelectOffWithSelect")},
	{KeyCommand::Hidden, kcSelectWithCopySelect, _T("kcSelectWithCopySelect")},
	{KeyCommand::Hidden, kcSelectOffWithCopySelect, _T("kcSelectOffWithCopySelect")},
	/*
	{1332, kcCopy, _T("Copy pattern data")},
	{1333, kcCut, _T("Cut pattern data")},
	{1334, kcPaste, _T("Paste pattern data")},
	{1335, kcMixPaste, _T("Mix-paste pattern data")},
	{1336, kcSelectAll, _T("Select all pattern data")},
	{CommandStruct::Hidden, kcSelectCol, _T("Select Channel / Select All")},
	*/
	{1338, kcPatternJumpDownh2, _T("Jump down by beat")},
	{1339, kcPatternJumpUph2, _T("Jump up by beat")},
	{1340, kcPatternSnapDownh2, _T("Snap down to beat")},
	{1341, kcPatternSnapUph2, _T("Snap up to beat")},
	{KeyCommand::Hidden, kcPatternJumpDownh2Select, _T("Jump down by beat select")},
	{KeyCommand::Hidden, kcPatternJumpUph2Select, _T("Jump up by beat select")},
	{KeyCommand::Hidden, kcPatternSnapDownh2Select, _T("Snap down to beat select")},
	{KeyCommand::Hidden, kcPatternSnapUph2Select, _T("Snap up to beat select")},
	{1346, kcFileOpen, _T("File/Open")},
	{1347, kcFileNew, _T("File/New")},
	{1348, kcFileClose, _T("File/Close")},
	{1349, kcFileSave, _T("File/Save")},
	{1350, kcFileSaveAs, _T("File/Save As")},
	{1351, kcFileSaveAsWave, _T("File/Stream Export")},
	{1352 | KeyCommand::Hidden, kcFileSaveAsMP3, _T("File/Stream Export")}, // Legacy
	{1353, kcFileSaveMidi, _T("File/Export as MIDI")},
	{1354, kcFileImportMidiLib, _T("File/Import MIDI Library")},
	{1355, kcFileAddSoundBank, _T("File/Add Sound Bank")},
	{1359, kcEditUndo, _T("Undo")},
	{1360, kcEditCut, _T("Cut")},
	{1361, kcEditCopy, _T("Copy")},
	{1362, kcEditPaste, _T("Paste")},
	{1363, kcEditMixPaste, _T("Mix Paste")},
	{1364, kcEditSelectAll, _T("Select All")},
	{1365, kcEditFind, _T("Find / Replace")},
	{1366, kcEditFindNext, _T("Find Next")},
	{1367, kcViewMain, _T("Toggle Main Toolbar")},
	{1368, kcViewTree, _T("Toggle Tree View")},
	{1369, kcViewOptions, _T("View Options")},
	{1370, kcHelp, _T("Help")},
	/*
	{1370, kcWindowNew, _T("New Window")},
	{1371, kcWindowCascade, _T("Cascade Windows")},
	{1372, kcWindowTileHorz, _T("Tile Windows Horizontally")},
	{1373, kcWindowTileVert, _T("Tile Windows Vertically")},
	*/
	{1374, kcEstimateSongLength, _T("Estimate Song Length")},
	{1375, kcStopSong, _T("Stop Song")},
	{1376, kcMidiRecord, _T("Toggle MIDI Record")},
	{1377, kcDeleteWholeRow, _T("Delete Row(s) (All Channels)")},
	{1378, kcInsertRow, _T("Insert Row(s)")},
	{1379, kcInsertWholeRow, _T("Insert Row(s) (All Channels)")},
	{1380, kcSampleTrim, _T("Trim sample around loop points")},
	{1381, kcSampleReverse, _T("Reverse Sample")},
	{1382, kcSampleDelete, _T("Delete Sample Selection")},
	{1383, kcSampleSilence, _T("Silence Sample Selection")},
	{1384, kcSampleNormalize, _T("Normalize Sample")},
	{1385, kcSampleAmplify, _T("Amplify Sample")},
	{1386, kcSampleZoomUp, _T("Zoom In")},
	{1387, kcSampleZoomDown, _T("Zoom Out")},
	{1660, kcPatternGrowSelection, _T("Grow selection")},
	{1661, kcPatternShrinkSelection, _T("Shrink selection")},
	{1662, kcTogglePluginEditor, _T("Toggle channel's plugin editor")},
	{1663, kcToggleFollowSong, _T("Toggle follow song")},
	{1664, kcClearFieldITStyle, _T("Clear Field (IT Style)")},
	{1665, kcClearFieldStepITStyle, _T("Clear Field and Step (IT Style)")},
	{1666, kcSetFXextension, _T("Parameter Extension Command")},
	{1667 | KeyCommand::Hidden, kcNoteCutOld, _T("Note Cut")},  // Legacy
	{1668 | KeyCommand::Hidden, kcNoteOffOld, _T("Note Off")},  // Legacy
	{1669, kcViewAddPlugin, _T("View Plugin Manager")},
	{1670, kcViewChannelManager, _T("View Channel Manager")},
	{1671, kcCopyAndLoseSelection, _T("Copy and lose selection")},
	{1672, kcNewPattern, _T("Insert New Pattern")},
	{1673, kcSampleLoad, _T("Load Sample")},
	{1674, kcSampleSave, _T("Save Sample")},
	{1675, kcSampleNew, _T("New Sample")},
	//{CommandStruct::Hidden, kcSampleCtrlLoad, _T("Load Sample")},
	//{CommandStruct::Hidden, kcSampleCtrlSave, _T("Save Sample")},
	//{CommandStruct::Hidden, kcSampleCtrlNew, _T("New Sample")},
	{KeyCommand::Hidden, kcInstrumentLoad, _T("Load Instrument")},
	{KeyCommand::Hidden, kcInstrumentSave, _T("Save Instrument")},
	{KeyCommand::Hidden, kcInstrumentNew, _T("New Instrument")},
	{KeyCommand::Hidden, kcInstrumentCtrlLoad, _T("Load Instrument")},
	{KeyCommand::Hidden, kcInstrumentCtrlSave, _T("Save Instrument")},
	{KeyCommand::Hidden, kcInstrumentCtrlNew, _T("New Instrument")},
	{1685, kcSwitchToOrderList, _T("Switch to Order List")},
	{1686, kcEditMixPasteITStyle, _T("Mix Paste (IT Style)")},
	{1687, kcApproxRealBPM, _T("Show approx. real BPM")},
	{KeyCommand::Hidden, kcNavigateDownBySpacingSelect, _T("Up-By-Spacing-Select")},
	{KeyCommand::Hidden, kcNavigateUpBySpacingSelect, _T("Down-By-Spacing-Select")},
	{1691, kcNavigateDownBySpacing, _T("Navigate down by spacing")},
	{1692, kcNavigateUpBySpacing, _T("Navigate up by spacing")},
	{1693, kcPrevDocument, _T("Previous Document")},
	{1694, kcNextDocument, _T("Next Document")},
	{1763, kcVSTGUIPrevPreset, _T("Previous Plugin Preset")},
	{1764, kcVSTGUINextPreset, _T("Next Plugin Preset")},
	{1765, kcVSTGUIRandParams, _T("Randomize Plugin Parameters")},
	{1766, kcPatternGoto, _T("Go to row/channel/...")},
	{KeyCommand::Hidden, kcPatternOpenRandomizer, _T("Pattern Randomizer")},  // while there's not randomizer yet, let's just disable it for now
	{1768, kcPatternInterpolateNote, _T("Interpolate Note")},
	{KeyCommand::Hidden, kcViewGraph, _T("View Graph")},  // while there's no graph yet, let's just disable it for now
	{1770, kcToggleChanMuteOnPatTransition, _T("(Un)mute channel on pattern transition")},
	{1771, kcChannelUnmuteAll, _T("Unmute all channels")},
	{1772, kcShowPatternProperties, _T("Show Pattern Properties")},
	{1773, kcShowMacroConfig, _T("View Zxx Macro Configuration")},
	{1775, kcViewSongProperties, _T("View Song Properties")},
	{1776, kcChangeLoopStatus, _T("Toggle Loop Pattern")},
	{1777, kcFileExportCompat, _T("File/Compatibility Export")},
	{1778, kcUnmuteAllChnOnPatTransition, _T("Unmute all channels on pattern transition")},
	{1779, kcSoloChnOnPatTransition, _T("Solo channel on pattern transition")},
	{1780, kcTimeAtRow, _T("Show playback time at current row")},
	{1781, kcViewMIDImapping, _T("View MIDI Mapping")},
	{1782, kcVSTGUIPrevPresetJump, _T("Plugin Preset -10")},
	{1783, kcVSTGUINextPresetJump, _T("Plugin Preset +10")},
	{1784, kcSampleInvert, _T("Invert Sample Phase")},
	{1785, kcSampleSignUnsign, _T("Signed / Unsigned Conversion")},
	{1786, kcChannelReset, _T("Reset Channel")},
	{1787, kcToggleOverflowPaste, _T("Toggle Overflow Paste")},
	{1788, kcNotePC, _T("Parameter Control")},
	{1789, kcNotePCS, _T("Parameter Control (smooth)")},
	{1790, kcSampleRemoveDCOffset, _T("Remove DC Offset")},
	{1791, kcNoteFade, _T("Note Fade")},
	{1792 | KeyCommand::Hidden, kcNoteFadeOld, _T("Note Fade")},  // Legacy
	{1793, kcEditPasteFlood, _T("Paste Flood")},
	{1794, kcOrderlistNavigateLeft, _T("Previous Order")},
	{1795, kcOrderlistNavigateRight, _T("Next Order")},
	{1796, kcOrderlistNavigateFirst, _T("First Order")},
	{1797, kcOrderlistNavigateLast, _T("Last Order")},
	{KeyCommand::Hidden, kcOrderlistNavigateLeftSelect, _T("Left-Select")},
	{KeyCommand::Hidden, kcOrderlistNavigateRightSelect, _T("Right-Select")},
	{KeyCommand::Hidden, kcOrderlistNavigateFirstSelect, _T("First-Select")},
	{KeyCommand::Hidden, kcOrderlistNavigateLastSelect, _T("Last-Select")},
	{1802, kcOrderlistEditDelete, _T("Delete Order")},
	{1803, kcOrderlistEditInsert, _T("Insert Order")},
	{1804, kcOrderlistEditPattern, _T("Edit Pattern")},
	{1805, kcOrderlistSwitchToPatternView, _T("Switch to pattern editor")},
	{1806, kcDuplicatePattern, _T("Duplicate Pattern")},
	{1807, kcOrderlistPat0, _T("Pattern index digit 0")},
	{1808, kcOrderlistPat1, _T("Pattern index digit 1")},
	{1809, kcOrderlistPat2, _T("Pattern index digit 2")},
	{1810, kcOrderlistPat3, _T("Pattern index digit 3")},
	{1811, kcOrderlistPat4, _T("Pattern index digit 4")},
	{1812, kcOrderlistPat5, _T("Pattern index digit 5")},
	{1813, kcOrderlistPat6, _T("Pattern index digit 6")},
	{1814, kcOrderlistPat7, _T("Pattern index digit 7")},
	{1815, kcOrderlistPat8, _T("Pattern index digit 8")},
	{1816, kcOrderlistPat9, _T("Pattern index digit 9")},
	{1817, kcOrderlistPatPlus, _T("Increase Pattern Index")},
	{1818, kcOrderlistPatMinus, _T("Decrease Pattern Index")},
	{1819, kcShowSplitKeyboardSettings, _T("Split Keyboard Settings dialog")},
	{1820, kcEditPushForwardPaste, _T("Push Forward Paste (Insert)")},
	{1821, kcInstrumentEnvelopePointMoveLeft, _T("Move Envelope Point Left")},
	{1822, kcInstrumentEnvelopePointMoveRight, _T("Move Envelope Point Right")},
	{1823, kcInstrumentEnvelopePointMoveUp, _T("Move envelope Point Up")},
	{1824, kcInstrumentEnvelopePointMoveDown, _T("Move Envelope Point Down")},
	{1825, kcInstrumentEnvelopePointPrev, _T("Select Previous Envelope Point")},
	{1826, kcInstrumentEnvelopePointNext, _T("Select Next Envelope Point")},
	{1827, kcInstrumentEnvelopePointInsert, _T("Insert Envelope Point")},
	{1828, kcInstrumentEnvelopePointRemove, _T("Remove Envelope Point")},
	{1829, kcInstrumentEnvelopeSetLoopStart, _T("Set Loop Start")},
	{1830, kcInstrumentEnvelopeSetLoopEnd, _T("Set Loop End")},
	{1831, kcInstrumentEnvelopeSetSustainLoopStart, _T("Set Sustain Loop Start")},
	{1832, kcInstrumentEnvelopeSetSustainLoopEnd, _T("Set Sustain Loop End")},
	{1833, kcInstrumentEnvelopeToggleReleaseNode, _T("Toggle Release Mode")},
	{1834, kcInstrumentEnvelopePointMoveUp8, _T("Move Envelope Point Up (Coarse)")},
	{1835, kcInstrumentEnvelopePointMoveDown8, _T("Move Envelope Point Down (Coarse)")},
	{1836, kcPatternEditPCNotePlugin, _T("Toggle PC Event/Instrument Plugin Editor")},
	{1837, kcInstrumentEnvelopeZoomIn, _T("Zoom In")},
	{1838, kcInstrumentEnvelopeZoomOut, _T("Zoom Out")},
	{1839, kcVSTGUIToggleRecordParams, _T("Toggle Parameter Recording")},
	{1840, kcVSTGUIToggleSendKeysToPlug, _T("Pass Key Presses to Plugin")},
	{1841, kcVSTGUIBypassPlug, _T("Bypass Plugin")},
	{1842, kcInsNoteMapTransposeDown, _T("Transpose -1 (Note Map)")},
	{1843, kcInsNoteMapTransposeUp, _T("Transpose +1 (Note Map)")},
	{1844, kcInsNoteMapTransposeOctDown, _T("Transpose -1 Octave (Note Map)")},
	{1845, kcInsNoteMapTransposeOctUp, _T("Transpose +1 Octave (Note Map)")},
	{1846, kcInsNoteMapCopyCurrentNote, _T("Map all notes to selected note")},
	{1847, kcInsNoteMapCopyCurrentSample, _T("Map all notes to selected sample")},
	{1848, kcInsNoteMapReset, _T("Reset Note Mapping")},
	{1849, kcInsNoteMapEditSample, _T("Edit Current Sample")},
	{1850, kcInsNoteMapEditSampleMap, _T("Edit Sample Map")},
	{1851, kcInstrumentCtrlDuplicate, _T("Duplicate Instrument")},
	{1852, kcPanic, _T("Panic")},
	{1853, kcOrderlistPatIgnore, _T("Separator (+++) Index")},
	{1854, kcOrderlistPatInvalid, _T("Stop (---) Index")},
	{1855, kcViewEditHistory, _T("View Edit History")},
	{1856, kcSampleQuickFade, _T("Quick Fade")},
	{1857, kcSampleXFade, _T("Crossfade Sample Loop")},
	{1858, kcSelectBeat, _T("Select Beat")},
	{1859, kcSelectMeasure, _T("Select Measure")},
	{1860, kcFileSaveTemplate, _T("File/Save As Template")},
	{1861, kcIncreaseSpacing, _T("Increase Edit Step")},
	{1862, kcDecreaseSpacing, _T("Decrease Edit Step")},
	{1863, kcSampleAutotune, _T("Tune Sample to given Note")},
	{1864, kcFileCloseAll, _T("File/Close All")},
	{KeyCommand::Hidden, kcSetOctaveStop0, _T("Set Octave 0")},
	{KeyCommand::Hidden, kcSetOctaveStop1, _T("Set Octave 1")},
	{KeyCommand::Hidden, kcSetOctaveStop2, _T("Set Octave 2")},
	{KeyCommand::Hidden, kcSetOctaveStop3, _T("Set Octave 3")},
	{KeyCommand::Hidden, kcSetOctaveStop4, _T("Set Octave 4")},
	{KeyCommand::Hidden, kcSetOctaveStop5, _T("Set Octave 5")},
	{KeyCommand::Hidden, kcSetOctaveStop6, _T("Set Octave 6")},
	{KeyCommand::Hidden, kcSetOctaveStop7, _T("Set Octave 7")},
	{KeyCommand::Hidden, kcSetOctaveStop8, _T("Set Octave 8")},
	{KeyCommand::Hidden, kcSetOctaveStop9, _T("Set Octave 9")},
	{1875, kcOrderlistLockPlayback, _T("Lock Playback to Selection")},
	{1876, kcOrderlistUnlockPlayback, _T("Unlock Playback")},
	{1877, kcChannelSettings, _T("Quick Channel Settings")},
	{1878, kcChnSettingsPrev, _T("Previous Channel")},
	{1879, kcChnSettingsNext, _T("Next Channel")},
	{1880, kcChnSettingsClose, _T("Switch to Pattern Editor")},
	{1881, kcTransposeCustom, _T("Transpose Custom")},
	{1882, kcSampleZoomSelection, _T("Zoom into Selection")},
	{1883, kcChannelRecordSelect, _T("Channel Record Select")},
	{1884, kcChannelSplitRecordSelect, _T("Channel Split Record Select")},
	{1885, kcDataEntryUp, _T("Data Entry +1")},
	{1886, kcDataEntryDown, _T("Data Entry -1")},
	{1887, kcSample8Bit, _T("Convert to 8-bit / 16-bit")},
	{1888, kcSampleMonoMix, _T("Convert to Mono (Mix)")},
	{1889, kcSampleMonoLeft, _T("Convert to Mono (Left Channel)")},
	{1890, kcSampleMonoRight, _T("Convert to Mono (Right Channel)")},
	{1891, kcSampleMonoSplit, _T("Convert to Mono (Split Sample)")},
	{1892, kcQuantizeSettings, _T("Quantize Settings")},
	{1893, kcDataEntryUpCoarse, _T("Data Entry Up (Coarse)")},
	{1894, kcDataEntryDownCoarse, _T("Data Entry Down (Coarse)")},
	{1895, kcToggleNoteOffRecordPC, _T("Toggle Note Off record (PC keyboard)")},
	{1896, kcToggleNoteOffRecordMIDI, _T("Toggle Note Off record (MIDI)")},
	{1897, kcFindInstrument, _T("Pick up nearest instrument number")},
	{1898, kcPlaySongFromPattern, _T("Play Song from Pattern Start")},
	{1899, kcVSTGUIToggleRecordMIDIOut, _T("Record MIDI Out to Pattern Editor")},
	{1900, kcToggleClipboardManager, _T("Toggle Clipboard Manager")},
	{1901, kcClipboardPrev, _T("Cycle to Previous Clipboard")},
	{1902, kcClipboardNext, _T("Cycle to Next Clipboard")},
	{1903, kcSelectRow, _T("Select Row")},
	{1904, kcSelectEvent, _T("Select Event")},
	{1905, kcEditRedo, _T("Redo")},
	{1906, kcFileAppend, _T("File/Append Module")},
	{1907, kcSampleTransposeUp, _T("Transpose +1")},
	{1908, kcSampleTransposeDown, _T("Transpose -1")},
	{1909, kcSampleTransposeOctUp, _T("Transpose +1 Octave")},
	{1910, kcSampleTransposeOctDown, _T("Transpose -1 Octave")},
	{1911, kcPatternInterpolateInstr, _T("Interpolate Instrument")},
	{1912, kcDummyShortcut, _T("Dummy Shortcut")},
	{1913, kcSampleUpsample, _T("Upsample")},
	{1914, kcSampleDownsample, _T("Downsample")},
	{1915, kcSampleResample, _T("Resample")},
	{1916, kcSampleCenterLoopStart, _T("Center loop start in view")},
	{1917, kcSampleCenterLoopEnd, _T("Center loop end in view")},
	{1918, kcSampleCenterSustainStart, _T("Center sustain loop start in view")},
	{1919, kcSampleCenterSustainEnd, _T("Center sustain loop end in view")},
	{1920, kcInstrumentEnvelopeLoad, _T("Load Envelope")},
	{1921, kcInstrumentEnvelopeSave, _T("Save Envelope")},
	{1922, kcChannelTranspose, _T("Transpose Channel")},
	{1923, kcChannelDuplicate, _T("Duplicate Channel")},
	// Reserved range 1924...1949 for kcStartSampleCues...kcEndSampleCues (generated below)
	{1950, kcOrderlistEditCopyOrders, _T("Copy Orders")},
	{KeyCommand::Hidden, kcTreeViewStopPreview, _T("Stop sample preview")},
	{1952, kcSampleDuplicate, _T("Duplicate Sample")},
	{1953, kcSampleSlice, _T("Slice at cue points")},
	{1954, kcInstrumentEnvelopeScale, _T("Scale Envelope Points")},
	{1955, kcInsNoteMapRemove, _T("Remove All Samples")},
	{1956, kcInstrumentEnvelopeSelectLoopStart, _T("Select Envelope Loop Start")},
	{1957, kcInstrumentEnvelopeSelectLoopEnd, _T("Select Envelope Loop End")},
	{1958, kcInstrumentEnvelopeSelectSustainStart, _T("Select Envelope Sustain Start")},
	{1959, kcInstrumentEnvelopeSelectSustainEnd, _T("Select Envelope Sustain End")},
	{1960, kcInstrumentEnvelopePointMoveLeftCoarse, _T("Move envelope point left (Coarse)")},
	{1961, kcInstrumentEnvelopePointMoveRightCoarse, _T("Move envelope point right (Coarse)")},
	{1962, kcSampleCenterSampleStart, _T("Zoom into sample start")},
	{1963, kcSampleCenterSampleEnd, _T("Zoom into sample end")},
	{1964, kcSampleTrimToLoopEnd, _T("Trim to loop end")},
	{1965, kcLockPlaybackToRows, _T("Lock Playback to Rows")},
	{1966, kcSwitchToInstrLibrary, _T("Switch To Instrument Library")},
	{1967, kcPatternSetInstrumentNotEmpty, _T("Apply current instrument to existing only")},
	{1968, kcSelectColumn, _T("Select Column")},
	{1969, kcSampleStereoSep, _T("Change Stereo Separation")},
	{1970, kcTransposeCustomQuick, _T("Transpose Custom (Quick)")},
	{1971, kcPrevEntryInColumn, _T("Jump to previous entry in column")},
	{1972, kcNextEntryInColumn, _T("Jump to next entry in column")},
	{1973, kcViewTempoSwing, _T("View Global Tempo Swing Settings")},
	{1974, kcChordEditor, _T("Show Chord Editor")},
	{1975, kcToggleLoopSong, _T("Toggle Loop Song")},
	{1976, kcInstrumentEnvelopeSwitchToVolume, _T("Switch to Volume Envelope")},
	{1977, kcInstrumentEnvelopeSwitchToPanning, _T("Switch to Panning Envelope")},
	{1978, kcInstrumentEnvelopeSwitchToPitch, _T("Switch to Pitch / Filter Envelope")},
	{1979, kcInstrumentEnvelopeToggleVolume, _T("Toggle Volume Envelope")},
	{1980, kcInstrumentEnvelopeTogglePanning, _T("Toggle Panning Envelope")},
	{1981, kcInstrumentEnvelopeTogglePitch, _T("Toggle Pitch Envelope")},
	{1982, kcInstrumentEnvelopeToggleFilter, _T("Toggle Filter Envelope")},
	{1983, kcInstrumentEnvelopeToggleLoop, _T("Toggle Envelope Loop")},
	{1984, kcInstrumentEnvelopeToggleSustain, _T("Toggle Envelope Sustain Loop")},
	{1985, kcInstrumentEnvelopeToggleCarry, _T("Toggle Envelope Carry")},
	{1986, kcSampleInitializeOPL, _T("Initialize OPL Instrument")},
	{1987, kcFileSaveCopy, _T("File/Save Copy")},
	{1988, kcMergePatterns, _T("Merge Patterns")},
	{1989, kcSplitPattern, _T("Split Pattern")},
	{1990, kcSampleToggleDrawing, _T("Toggle Sample Drawing")},
	{1991, kcSampleResize, _T("Add Silence / Create Sample")},
	{1992, kcSampleGrid, _T("Configure Sample Grid")},
	{1993, kcLoseSelection, _T("Lose Selection")},
	{1994, kcCutPatternChannel, _T("Cut to Pattern Channel Clipboard")},
	{1995, kcCutPattern, _T("Cut to Pattern Clipboard")},
	{1996, kcCopyPatternChannel, _T("Copy to Pattern Channel Clipboard")},
	{1997, kcCopyPattern, _T("Copy to Pattern Clipboard")},
	{1998, kcPastePatternChannel, _T("Paste from Pattern Channel Clipboard")},
	{1999, kcPastePattern, _T("Paste from Pattern Clipboard")},
	{2000, kcToggleSmpInsList, _T("Toggle between lists")},
	{2001, kcExecuteSmpInsListItem, _T("Open item in editor")},
	{2002, kcDeleteRowGlobal, _T("Delete Row(s) (Global)")},
	{2003, kcDeleteWholeRowGlobal, _T("Delete Row(s) (All Channels, Global)")},
	{2004, kcInsertRowGlobal, _T("Insert Row(s) (Global)")},
	{2005, kcInsertWholeRowGlobal, _T("Insert Row(s) (All Channels, Global)")},
	{2006, kcPrevSequence, _T("Previous Sequence")},
	{2007, kcNextSequence, _T("Next Sequence")},
	{2008, kcChnColorFromPrev , _T("Pick Color from Previous Channel")},
	{2009, kcChnColorFromNext , _T("Pick Color from Next Channel") },
	{2010, kcChannelMoveLeft, _T("Move Channels to Left")},
	{2011, kcChannelMoveRight, _T("Move Channels to Right")},
	{2012, kcSampleConvertPingPongLoop, _T("Convert Ping-Pong Loop to Unidirectional") },
	{2013, kcSampleConvertPingPongSustain, _T("Convert Ping-Pong Sustain Loop to Unidirectional") },
	{2014, kcChannelAddBefore, _T("Add Channel Before Current")},
	{2015, kcChannelAddAfter, _T("Add Channel After Current") },
	{2016, kcChannelRemove, _T("Remove Channel") },
	{2017, kcSetFXFinetune, _T("Finetune") },
	{2018, kcSetFXFinetuneSmooth, _T("Finetune (Smooth)")},
	{2019, kcOrderlistEditInsertSeparator, _T("Insert Separator") },
	{2020, kcTempoIncrease, _T("Increase Tempo")},
	{2021, kcTempoDecrease, _T("Decrease Tempo")},
	{2022, kcTempoIncreaseFine, _T("Increase Tempo (Fine)")},
	{2023, kcTempoDecreaseFine, _T("Decrease Tempo (Fine)")},
	{2024, kcSpeedIncrease, _T("Increase Ticks per Row")},
	{2025, kcSpeedDecrease, _T("Decrease Ticks per Row")},
	{2026, kcRenameSmpInsListItem, _T("Rename Item")},
	{2027, kcShowChannelCtxMenu, _T("Show Channel Context (Right-Click) Menu")},
	{2028, kcShowChannelPluginCtxMenu, _T("Show Channel Plugin Context (Right-Click) Menu")},
	{2029, kcViewToggle, _T("Toggle Between Upper / Lower View") },
	{2030, kcFileSaveOPL, _T("File/Export OPL Register Dump") },
	{2031, kcSampleLoadRaw, _T("Load Raw Sample")},
	{2032, kcTogglePatternPlayRow, _T("Toggle Row Playback when Navigating")},
	{2033, kcInsNoteMapTransposeSamples, _T("Transpose Samples / Reset Map") },
	{KeyCommand::Hidden, kcPrevEntryInColumnSelect, _T("Select to previous entry in column")},
	{KeyCommand::Hidden, kcNextEntryInColumnSelect, _T("Select to next entry in column")},
	{2034, kcTreeViewOpen, _T("Open / View Item")},
	{2035, kcTreeViewPlay, _T("Play Item")},
	{2036, kcTreeViewInsert, _T("Insert Item")},
	{2037, kcTreeViewDuplicate, _T("Duplicate Item")},
	{2038, kcTreeViewDelete, _T("Delete Item")},
	{2039, kcTreeViewRename, _T("Rename Item / Send To Editor")},
	{2040, kcTreeViewFind, _T("Find in Instrument Library")},
	{2041, kcTreeViewSortByName, _T("Sort Instrument Library By Name")},
	{2042, kcTreeViewSortByDate, _T("Sort Instrument Library By Date")},
	{2043, kcTreeViewSortBySize, _T("Sort Instrument Library By Size")},
	{2044, kcTreeViewSendToEditorInsertNew, _T("Send To Editor (Insert New)")},
	{2045, kcPlayStopSong, _T("Play Song / Stop Song")},
	{2046, kcTreeViewDeletePermanently, _T("Delete Item Permanently")},
	{2047, kcSampleToggleFollowPlayCursor, _T("Toggle Follow Sample Play Cursor")},
	{2048, kcPatternScrollLeft, _T("Scroll Left")},
	{2049, kcPatternScrollRight, _T("Scroll Right") },
	{2050, kcPatternScrollUp, _T("Scroll Up")},
	{2051, kcPatternScrollDown, _T("Scroll Down")},
	{2052, kcPlaySongFromCursorPause, _T("Play Song from Cursor / Pause")},
	{2053, kcPlaySongFromPatternPause, _T("Play Song from Pattern Start / Pause")},
	{2054, kcSampleFinetuneUp, _T("Increment Finetune")},
	{2055, kcSampleFinetuneDown, _T("Decrement Finetune")},
	{2056, kcTreeViewSwitchViews, _T("Switch between Upper / Lower Tree View")},
	{2057, kcTreeViewFolderUp, _T("Go to Parent Folder")},
	{KeyCommand::Hidden, kcTransposeUpStop, _T("Stop Transpose +1")},
	{KeyCommand::Hidden, kcTransposeDownStop, _T("Stop Transpose -1")},
	{KeyCommand::Hidden, kcTransposeOctUpStop, _T("Stop Transpose +1 Octave")},
	{KeyCommand::Hidden, kcTransposeOctDownStop, _T("Stop Transpose -1 Octave")},
	{KeyCommand::Hidden, kcTransposeCustomStop, _T("Stop Transpose Custom")},
	{KeyCommand::Hidden, kcTransposeCustomQuickStop, _T("Stop Transpose Custom (Quick)")},
	{KeyCommand::Hidden, kcDataEntryUpStop, _T("Stop Data Entry +1")},
	{KeyCommand::Hidden, kcDataEntryDownStop, _T("Stop Data Entry -1")},
	{KeyCommand::Hidden, kcDataEntryUpCoarseStop, _T("Stop Data Entry Up (Coarse)")},
	{KeyCommand::Hidden, kcDataEntryDownCoarseStop, _T("Stop Data Entry Down (Coarse)")},
	{2058, kcPrevOrderAtMeasureEnd, _T("Previous Order (Transition at end of current measure)")},
	{2059, kcNextOrderAtMeasureEnd, _T("Next Order (Transition at end of current measure)")},
	{2060, kcPrevOrderAtBeatEnd, _T("Previous Order (Transition at end of current beat)")},
	{2061, kcNextOrderAtBeatEnd, _T("Next Order (Transition at end of current beat)")},
	{2062, kcPrevOrderAtRowEnd, _T("Previous Order (Transition at end of current row)")},
	{2063, kcNextOrderAtRowEnd, _T("Next Order (Transition at end of current row)")},
	{2064, kcOrderlistQueueAtPatternEnd, _T("Queue Pattern (Transition at end of current pattern)")},
	{2065, kcOrderlistQueueAtMeasureEnd, _T("Queue Pattern (Transition at end of current measure)")},
	{2066, kcOrderlistQueueAtBeatEnd, _T("Queue Pattern (Transition at end of current beat)")},
	{2067, kcOrderlistQueueAtRowEnd, _T("Queue Pattern (Transition at end of current row)")},
	{2068, kcSampleConvertNormalLoopToSustain, _T("Convert Normal Loop to Sustain Loop")},
	{2069, kcSampleConvertSustainLoopToNormal, _T("Convert Sustain Loop to Normal Loop")},
	{2096, kcGotoNoteColumn, _T("Go to note column")},
	{2097, kcGotoInstrColumn, _T("Go to instrument column")},
	{2098, kcGotoVolumeColumn, _T("Go to volume effect column")},
	{2099, kcGotoCommandColumn, _T("Go to effect command column")},
	{2100, kcGotoParamColumn, _T("Go to effect parameter column")},
	{2101, kcContextMenu, _T("Open Context Menu")},
	{2102, kcOrderlistStreamExport, _T("Stream Export")},
	{2103, kcToggleVisibilityInstrColumn, _T("Toggle Instrument Column Visibility")},
	{2104, kcToggleVisibilityVolumeColumn, _T("Toggle Volume Column Visibility")},
	{2105, kcToggleVisibilityEffectColumn, _T("Toggle Effect Column Visibility")},
	{2106, kcFileOpenTemplate, _T("File/Open Template")},
	{2107, kcSetVolumeA, _T("Set volume digit A")},
	{2108, kcSetVolumeB, _T("Set volume digit B")},
	{2109, kcSetVolumeC, _T("Set volume digit C")},
	{2110, kcSetVolumeD, _T("Set volume digit D")},
	{2111, kcSetVolumeE, _T("Set volume digit E")},
	{2112, kcSetVolumeF, _T("Set volume digit F")},
	{2113, kcToggleOctaveTransposeMIDI, _T("Toggle Apply Octave Transpose to incoming MIDI Notes")},
	{2114, kcToggleContinueSongOnMIDINote, _T("Toggle Continue Song when MIDI Note is received")},
	{2115, kcToggleContinueSongOnMIDIPlayEvents, _T("Toggle Respond to Play / Continue / Stop Song MIDI messages")},
	{2116, kcToggleRecordMIDIVelocity, _T("Toggle Record MIDI Velocity")},
	{2117, kcToggleRecordMIDIPitchBend, _T("Toggle Record MIDI Pitch Bend")},
	{2118, kcToggleRecordMIDICCs, _T("Toggle Record MIDI CCs")},
	{2119, kcToggleMetronome, _T("Toggle Metronome")},
};
// clang-format on


// Get command descriptions etc.. loaded up.
void CCommandSet::SetupCommands()
{
	for(const auto &def : CommandDefinitions)
	{
		m_commands[def.cmd] = {def.uid, def.description};
	}

	for(int j = kcStartSampleCues; j <= kcEndSampleCues; j++)
	{
		CString s = MPT_CFORMAT("Preview / Set Sample Cue {}")(j - kcStartSampleCues + 1);
		m_commands[j] = {static_cast<uint32>(1924 + j - kcStartSampleCues), s};
	}
	static_assert(1924 + kcEndSampleCues - kcStartSampleCues < 1950);

	// Automatically generated note entry keys in non-pattern contexts
	for(const auto &[contextStartNotes, contextStopNotes] : NoteRanges)
	{
		if(contextStartNotes == kcVPStartNotes)
			continue;

		for(int i = kcVPStartNotes; i <= kcVPEndNotes; i++)
		{
			m_commands[i - kcVPStartNotes + contextStartNotes] = {KeyCommand::Hidden, m_commands[i].name};
		}
		for(int i = kcVPStartNoteStops; i <= kcVPEndNoteStops; i++)
		{
			m_commands[i - kcVPStartNoteStops + contextStopNotes] = {KeyCommand::Hidden, m_commands[i].name};
		}
	}

#ifdef MPT_BUILD_DEBUG
	// Ensure that every visible command has a unique ID, and all commands have a valid context
	for(CommandID i = kcFirst; i < kcNumCommands; i = static_cast<CommandID>(i + 1))
	{
		MPT_ASSERT(ContextFromCommand(i) != kCtxUnknownContext);
		if(m_commands[i].ID() != 0 || !m_commands[i].IsHidden())
		{
			for(size_t j = i + 1; j < kcNumCommands; j++)
			{
				if(m_commands[i].ID() == m_commands[j].ID())
				{
					LOG_COMMANDSET(MPT_UFORMAT("Duplicate or unset command UID: {}\n")(m_commands[i].ID()));
					MPT_ASSERT_NOTREACHED();
				}
			}
		}
	}
#endif  // MPT_BUILD_DEBUG
}


// Command Manipulation


CString CCommandSet::Add(KeyCombination kc, CommandID cmd, bool overwrite, int pos, bool checkEventConflict)
{
	kc.Context(ContextFromCommand(cmd));
	auto &kcList = m_commands[cmd].kcList;

	// Avoid duplicate
	if(mpt::contains(kcList, kc))
		return CString{};

	// Check that this keycombination isn't already assigned (in this context), except for dummy keys
	CString report;
	if(auto conflictCmd = IsConflicting(kc, cmd, checkEventConflict); conflictCmd.first != kcNull)
	{
		if(!overwrite)
		{
			return CString{};
		} else
		{
			report = FormatConflict(kc, conflictCmd.first, conflictCmd.second);
			LOG_COMMANDSET(mpt::ToUnicode(report));
		}
	}

	kcList.insert((pos < 0) ? kcList.end() : (kcList.begin() + pos), kc);

	//enfore rules on CommandSet
	EnforceAll(kc, cmd, true);
	return report;
}


CString CCommandSet::FormatConflict(KeyCombination kc, CommandID conflictCommand, KeyCombination conflictCombination) const
{
	if(IsCrossContextConflict(kc, conflictCombination))
		return _T("May conflict with ") + GetCommandText(conflictCommand) + _T(" in ") + conflictCombination.GetContextText();
	else
		return _T("Conflicts with ") + GetCommandText(conflictCommand) + _T(" in same context");
}

std::pair<CommandID, KeyCombination> CCommandSet::IsConflicting(KeyCombination kc, CommandID cmd, bool checkEventConflict, bool checkSameCommand) const
{
	if(m_commands[cmd].IsDummy())  // no need to search if we are adding a dummy key
		return {kcNull, KeyCombination()};
	
	for(int pass = 0; pass < 2; pass++)
	{
		// In the first pass, only look for conflicts in the same context, since
		// such conflicts are errors. Cross-context conflicts only emit warnings.
		for(int curCmd = kcFirst; curCmd < kcNumCommands; curCmd++)
		{
			if(m_commands[curCmd].IsDummy() || (!checkSameCommand && cmd == curCmd))
				continue;

			for(auto &curKc : m_commands[curCmd].kcList)
			{
				if(pass == 0 && curKc.Context() != kc.Context())
					continue;

				if(KeyCombinationConflict(curKc, kc, checkEventConflict))
				{
					return {(CommandID)curCmd, curKc};
				}
			}
		}
	}

	return std::make_pair(kcNull, KeyCombination());
}


void CCommandSet::Remove(int pos, CommandID cmd)
{
	if(pos >= 0 && static_cast<size_t>(pos) < m_commands[cmd].kcList.size())
	{
		Remove(m_commands[cmd].kcList[pos], cmd);
	}

	LOG_COMMANDSET(U_("Failed to remove a key: keychoice out of range."));
}


void CCommandSet::Remove(KeyCombination kc, CommandID cmd)
{
	auto &kcList = m_commands[cmd].kcList;
	auto index = std::find(kcList.begin(), kcList.end(), kc);
	if (index != kcList.end())
	{
		kcList.erase(index);
		LOG_COMMANDSET(U_("Removed a key"));
		EnforceAll(kc, cmd, false);
	} else
	{
		LOG_COMMANDSET(U_("Failed to remove a key as it was not found"));
	}
}


void CCommandSet::EnforceAll(KeyCombination inKc, CommandID inCmd, bool adding)
{
	//World's biggest, most confusing method. :)
	//Needs refactoring. Maybe make lots of Rule subclasses, each with their own Enforce() method?
	KeyCombination curKc;	// for looping through key combinations
	KeyCombination newKc;	// for adding new key combinations

	if(m_enforceRule[krAllowNavigationWithSelection])
	{
		// When we get a new navigation command key, we need to
		// make sure this navigation will work when any selection key is pressed
		if(inCmd >= kcStartPatNavigation && inCmd <= kcEndPatNavigation)
		{//Check that it is a nav cmd
			CommandID cmdNavSelection = (CommandID)(kcStartPatNavigationSelect + (inCmd-kcStartPatNavigation));
			for(auto &kc :m_commands[kcSelect].kcList)
			{//for all selection modifiers
				newKc = inKc;
				newKc.Modifier(kc);	//Add selection modifier's modifiers to this command
				if(adding)
				{
					LOG_COMMANDSET(MPT_UFORMAT("Enforcing rule krAllowNavigationWithSelection - adding key with modifier:{} to command: {}")(newKc.Modifier().GetRaw(), cmdNavSelection));
					Add(newKc, cmdNavSelection, false);
				} else
				{
					LOG_COMMANDSET(MPT_UFORMAT("Enforcing rule krAllowNavigationWithSelection - removing key with modifier:{} to command: {}")(newKc.Modifier().GetRaw(), cmdNavSelection));
					Remove(newKc, cmdNavSelection);
				}
			}
		}
		// Same applies for orderlist navigation
		else if(inCmd >= kcStartOrderlistNavigation && inCmd <= kcEndOrderlistNavigation)
		{//Check that it is a nav cmd
			CommandID cmdNavSelection = (CommandID)(kcStartOrderlistNavigationSelect+ (inCmd-kcStartOrderlistNavigation));
			for(auto &kc : m_commands[kcSelect].kcList)
			{//for all selection modifiers
				newKc = inKc;
				newKc.AddModifier(kc);	//Add selection modifier's modifiers to this command
				if(adding)
				{
					LOG_COMMANDSET(MPT_UFORMAT("Enforcing rule krAllowNavigationWithSelection - adding key with modifier:{} to command: {}")(newKc.Modifier().GetRaw(), cmdNavSelection));
					Add(newKc, cmdNavSelection, false);
				} else
				{
					LOG_COMMANDSET(MPT_UFORMAT("Enforcing rule krAllowNavigationWithSelection - removing key with modifier:{} to command: {}")(newKc.Modifier().GetRaw(), cmdNavSelection));
					Remove(newKc, cmdNavSelection);
				}
			}
		}
		// When we get a new selection key, we need to make sure that
		// all navigation commands will work with this selection key pressed
		else if(inCmd == kcSelect)
		{
			// check that is is a selection
			for(int curCmd=kcStartPatNavigation; curCmd<=kcEndPatNavigation; curCmd++)
			{
				// for all nav commands
				for(auto &kc : m_commands[curCmd].kcList)
				{
					// for all keys for this command
					CommandID cmdNavSelection = (CommandID)(kcStartPatNavigationSelect + (curCmd-kcStartPatNavigation));
					newKc = kc;					// get all properties from the current nav cmd key
					newKc.AddModifier(inKc);	// and the new selection modifier
					if(adding)
					{
						LOG_COMMANDSET(MPT_UFORMAT("Enforcing rule krAllowNavigationWithSelection - adding key:{} with modifier:{} to command: {}")(curCmd, inKc.Modifier().GetRaw(), cmdNavSelection));
						Add(newKc, cmdNavSelection, false);
					} else
					{
						LOG_COMMANDSET(MPT_UFORMAT("Enforcing rule krAllowNavigationWithSelection - removing key:{} with modifier:{} to command: {}")(curCmd, inKc.Modifier().GetRaw(), cmdNavSelection));
						Remove(newKc, cmdNavSelection);
					}
				}
			} // end all nav commands
			for(int curCmd = kcStartOrderlistNavigation; curCmd <= kcEndOrderlistNavigation; curCmd++)
			{// for all nav commands
				for(auto &kc : m_commands[curCmd].kcList)
				{// for all keys for this command
					CommandID cmdNavSelection = (CommandID)(kcStartOrderlistNavigationSelect+ (curCmd-kcStartOrderlistNavigation));
					newKc = kc;					// get all properties from the current nav cmd key
					newKc.AddModifier(inKc);	// and the new selection modifier
					if(adding)
					{
						LOG_COMMANDSET(MPT_UFORMAT("Enforcing rule krAllowNavigationWithSelection - adding key:{} with modifier:{} to command: {}")(curCmd, inKc.Modifier().GetRaw(), cmdNavSelection));
						Add(newKc, cmdNavSelection, false);
					} else
					{
						LOG_COMMANDSET(MPT_UFORMAT("Enforcing rule krAllowNavigationWithSelection - removing key:{} with modifier:{} to command: {}")(curCmd, inKc.Modifier().GetRaw(), cmdNavSelection));
						Remove(newKc, cmdNavSelection);
					}
				}
			} // end all nav commands
		}
	} // end krAllowNavigationWithSelection

	if(m_enforceRule[krAllowSelectionWithNavigation])
	{
		KeyCombination newKcSel;

		// When we get a new navigation command key, we need to ensure
		// all selection keys will work even when this new selection key is pressed
		if(inCmd >= kcStartPatNavigation && inCmd <= kcEndPatNavigation)
		{//if this is a navigation command
			for(auto &kc : m_commands[kcSelect].kcList)
			{//for all deselection modifiers
				newKcSel = kc;				// get all properties from the selection key
				newKcSel.AddModifier(inKc);	// add modifiers from the new nav command
				if(adding)
				{
					LOG_COMMANDSET(U_("Enforcing rule krAllowSelectionWithNavigation: adding  removing kcSelectWithNav and kcSelectOffWithNav"));
					Add(newKcSel, kcSelectWithNav, false);
				} else
				{
					LOG_COMMANDSET(U_("Enforcing rule krAllowSelectionWithNavigation: removing kcSelectWithNav and kcSelectOffWithNav"));
					Remove(newKcSel, kcSelectWithNav);
				}
			}
		}
		// Same for orderlist navigation
		if(inCmd >= kcStartOrderlistNavigation && inCmd <= kcEndOrderlistNavigation)
		{//if this is a navigation command
			for(auto &kc : m_commands[kcSelect].kcList)
			{//for all deselection modifiers
				newKcSel = kc;				// get all properties from the selection key
				newKcSel.AddModifier(inKc);	// add modifiers from the new nav command
				if(adding)
				{
					LOG_COMMANDSET(U_("Enforcing rule krAllowSelectionWithNavigation: adding  removing kcSelectWithNav and kcSelectOffWithNav"));
					Add(newKcSel, kcSelectWithNav, false);
				} else
				{
					LOG_COMMANDSET(U_("Enforcing rule krAllowSelectionWithNavigation: removing kcSelectWithNav and kcSelectOffWithNav"));
					Remove(newKcSel, kcSelectWithNav);
				}
			}
		}
		// When we get a new selection key, we need to ensure it will work even when
		// any navigation key is pressed
		else if(inCmd == kcSelect)
		{
			for(int curCmd = kcStartPatNavigation; curCmd <= kcEndPatNavigation; curCmd++)
			{//for all nav commands
				for(auto &kc : m_commands[curCmd].kcList)
				{// for all keys for this command
					newKcSel = inKc;			// get all properties from the selection key
					newKcSel.AddModifier(kc);	//add the nav keys' modifiers
					if(adding)
					{
						LOG_COMMANDSET(MPT_UFORMAT("Enforcing rule krAllowSelectionWithNavigation - adding key:{} with modifier:{} to command: {}")(curCmd, inKc.Modifier().GetRaw(), kcSelectWithNav));
						Add(newKcSel, kcSelectWithNav, false);
					} else
					{
						LOG_COMMANDSET(MPT_UFORMAT("Enforcing rule krAllowSelectionWithNavigation - removing key:{} with modifier:{} to command: {}")(curCmd, inKc.Modifier().GetRaw(), kcSelectWithNav));
						Remove(newKcSel, kcSelectWithNav);
					}
				}
			} // end all nav commands

			for(int curCmd = kcStartOrderlistNavigation; curCmd <= kcEndOrderlistNavigation; curCmd++)
			{//for all nav commands
				for(auto &kc : m_commands[curCmd].kcList)
				{// for all keys for this command
					newKcSel=inKc;				// get all properties from the selection key
					newKcSel.AddModifier(kc);	//add the nav keys' modifiers
					if(adding)
					{
						LOG_COMMANDSET(MPT_UFORMAT("Enforcing rule krAllowSelectionWithNavigation - adding key:{} with modifier:{} to command: {}")(curCmd, inKc.Modifier().GetRaw(), kcSelectWithNav));
						Add(newKcSel, kcSelectWithNav, false);
					} else
					{
						LOG_COMMANDSET(MPT_UFORMAT("Enforcing rule krAllowSelectionWithNavigation - removing key:{} with modifier:{} to command: {}")(curCmd, inKc.Modifier().GetRaw(), kcSelectWithNav));
						Remove(newKcSel, kcSelectWithNav);
					}
				}
			} // end all nav commands
		}

	}

	// if we add a selector or a copy selector, we need it to switch off when we release the key.
	if (m_enforceRule[krAutoSelectOff])
	{
		KeyCombination newKcDeSel;
		CommandID cmdOff = kcNull;
		switch (inCmd)
		{
			case kcSelect:					cmdOff = kcSelectOff;				break;
			case kcSelectWithNav:			cmdOff = kcSelectOffWithNav;		break;
			case kcCopySelect:				cmdOff = kcCopySelectOff;			break;
			case kcCopySelectWithNav:		cmdOff = kcCopySelectOffWithNav;	break;
			case kcSelectWithCopySelect:	cmdOff = kcSelectOffWithCopySelect; break;
			case kcCopySelectWithSelect:	cmdOff = kcCopySelectOffWithSelect; break;
			default: break;
		}

		if(cmdOff != kcNull)
		{
			newKcDeSel = inKc;
			newKcDeSel.EventType(kKeyEventUp);

			// Register key-up when releasing any of the modifiers.
			// Otherwise, select key combos might get stuck. Example:
			// [Ctrl Down] [Alt Down] [Ctrl Up] [Alt Up] After this action, copy select (Ctrl+Drag) would still be activated without this code.
			const UINT maxMod = TrackerSettings::Instance().MiscDistinguishModifiers ? MaxMod : (MaxMod & ~(HOTKEYF_RSHIFT | HOTKEYF_RCONTROL | HOTKEYF_RALT));
			for(UINT i = 0; i <= maxMod; i++)
			{
				// Avoid Windows key, so that it won't detected as being actively used
				if(i & HOTKEYF_EXT)
					continue;
				newKcDeSel.Modifier(static_cast<Modifiers>(i));
				//newKcDeSel.mod&=~CodeToModifier(inKc.code);		//<-- Need to get rid of right modifier!!

				if (adding)
					Add(newKcDeSel, cmdOff, false);
				else
					Remove(newKcDeSel, cmdOff);
			}
		}

	}
	// Allow combinations of copyselect and select
	if(m_enforceRule[krAllowSelectCopySelectCombos])
	{
		KeyCombination newKcSel, newKcCopySel;
		if(inCmd==kcSelect)
		{
			// On getting a new selection key, make this selection key work with all copy selects' modifiers
			// On getting a new selection key, make all copyselects work with this key's modifiers
			for(auto &kc : m_commands[kcCopySelect].kcList)
			{
				newKcSel=inKc;
				newKcSel.AddModifier(kc);
				newKcCopySel = kc;
				newKcCopySel.AddModifier(inKc);
				LOG_COMMANDSET(U_("Enforcing rule krAllowSelectCopySelectCombos"));
				if(adding)
				{
					Add(newKcSel, kcSelectWithCopySelect, false);
					Add(newKcCopySel, kcCopySelectWithSelect, false);
				} else
				{
					Remove(newKcSel, kcSelectWithCopySelect);
					Remove(newKcCopySel, kcCopySelectWithSelect);
				}
			}
		}
		if(inCmd == kcCopySelect)
		{
			// On getting a new copyselection key, make this copyselection key work with all selects' modifiers
			// On getting a new copyselection key, make all selects work with this key's modifiers
			for(auto &kc : m_commands[kcSelect].kcList)
			{
				newKcSel = kc;
				newKcSel.AddModifier(inKc);
				newKcCopySel = inKc;
				newKcCopySel.AddModifier(kc);
				LOG_COMMANDSET(U_("Enforcing rule krAllowSelectCopySelectCombos"));
				if(adding)
				{
					Add(newKcSel, kcSelectWithCopySelect, false);
					Add(newKcCopySel, kcCopySelectWithSelect, false);
				} else
				{
					Remove(newKcSel, kcSelectWithCopySelect);
					Remove(newKcCopySel, kcCopySelectWithSelect);
				}
			}
		}
	}


	// Lock Notes to Chords
	if (m_enforceRule[krLockNotesToChords])
	{
		if (inCmd>=kcVPStartNotes && inCmd<=kcVPEndNotes)
		{
			int noteOffset = inCmd - kcVPStartNotes;
			for(auto &kc : m_commands[kcChordModifier].kcList)
			{//for all chord modifier keys
				newKc = inKc;
				newKc.AddModifier(kc);
				if (adding)
				{
					LOG_COMMANDSET(U_("Enforcing rule krLockNotesToChords: auto adding in a chord command"));
					Add(newKc, (CommandID)(kcVPStartChords+noteOffset), false);
				}
				else
				{
					LOG_COMMANDSET(U_("Enforcing rule krLockNotesToChords: auto removing a chord command"));
					Remove(newKc, (CommandID)(kcVPStartChords+noteOffset));
				}
			}
		}
		if (inCmd==kcChordModifier)
		{
			int noteOffset;
			for (int curCmd=kcVPStartNotes; curCmd<=kcVPEndNotes; curCmd++)
			{//for all notes
				for(auto kc : m_commands[curCmd].kcList)
				{//for all keys for this note
					noteOffset = curCmd - kcVPStartNotes;
					kc.AddModifier(inKc);
					if (adding)
					{
						LOG_COMMANDSET(U_("Enforcing rule krLockNotesToChords: auto adding in a chord command"));
						Add(kc, (CommandID)(kcVPStartChords+noteOffset), false);
					}
					else
					{
						LOG_COMMANDSET(U_("Enforcing rule krLockNotesToChords: auto removing a chord command"));
						Remove(kc, (CommandID)(kcVPStartChords+noteOffset));
					}
				}
			}
		}
	}


	// Auto set note off on release
	if (m_enforceRule[krNoteOffOnKeyRelease])
	{
		int cmdOffset = int32_min;
		if(inCmd >= kcVPStartNotes && inCmd <= kcVPEndNotes)
			cmdOffset = kcVPStartNoteStops - kcVPStartNotes;
		else if(inCmd >= kcVPStartChords && inCmd <= kcVPEndChords)
			cmdOffset = kcVPStartChordStops - kcVPStartChords;
		else if(inCmd >= kcSetOctave0 && inCmd <= kcSetOctave9)
			cmdOffset = kcSetOctaveStop0 - kcSetOctave0;
		else if(inCmd >= kcBeginTranspose && inCmd <= kcEndTranspose)
			cmdOffset = kcBeginTransposeStop - kcBeginTranspose;

		if(cmdOffset != int32_min)
		{
			newKc = inKc;
			newKc.EventType(kKeyEventUp);
			if (adding)
			{
				LOG_COMMANDSET(U_("Enforcing rule krNoteOffOnKeyRelease: adding note off command"));
				Add(newKc, static_cast<CommandID>(inCmd + cmdOffset), false);
			} else
			{
				LOG_COMMANDSET(U_("Enforcing rule krNoteOffOnKeyRelease: removing note off command"));
				Remove(newKc, static_cast<CommandID>(inCmd + cmdOffset));
			}
		}
	}

	// Reassign freed number keys to octaves
	if (m_enforceRule[krReassignDigitsToOctaves] && !adding)
	{
		if ( (inKc.Modifier() == ModNone) &&	//no modifier
			 ( (inKc.Context() == kCtxViewPatternsNote) || (inKc.Context() == kCtxViewPatterns) ) && //note scope or pattern scope
			 ( ('0'<=inKc.KeyCode() && inKc.KeyCode()<='9') || (VK_NUMPAD0<=inKc.KeyCode() && inKc.KeyCode()<=VK_NUMPAD9) ) )  //is number key
		{
				newKc = KeyCombination(kCtxViewPatternsNote, ModNone, inKc.KeyCode(), kKeyEventDown);
				int offset = ('0'<=inKc.KeyCode() && inKc.KeyCode()<='9') ? newKc.KeyCode()-'0' : newKc.KeyCode()-VK_NUMPAD0;
				Add(newKc, (CommandID)(kcSetOctave0 + (newKc.KeyCode()-offset)), false);
		}
	}
	// Add spacing
	if (m_enforceRule[krAutoSpacing])
	{
		if (inCmd == kcSetSpacing && adding)
		{
			newKc = KeyCombination(kCtxViewPatterns, inKc.Modifier(), 0, kKeyEventDown);
			for (char i = 0; i <= 9; i++)
			{
				newKc.KeyCode('0' + i);
				Add(newKc, (CommandID)(kcSetSpacing0 + i), false);
				newKc.KeyCode(VK_NUMPAD0 + i);
				Add(newKc, (CommandID)(kcSetSpacing0 + i), false);
			}
		} else if (!adding && (inCmd < kcSetSpacing || inCmd > kcSetSpacing9))
		{
			// Re-add combinations that might have been overwritten by another command
			if(('0' <= inKc.KeyCode() && inKc.KeyCode() <= '9') || (VK_NUMPAD0 <= inKc.KeyCode() && inKc.KeyCode() <= VK_NUMPAD9))
			{
				for(const auto &spacing : m_commands[kcSetSpacing].kcList)
				{
					newKc = KeyCombination(kCtxViewPatterns, spacing.Modifier(), inKc.KeyCode(), spacing.EventType());
					if('0' <= inKc.KeyCode() && inKc.KeyCode() <= '9')
						Add(newKc, static_cast<CommandID>(kcSetSpacing0 + inKc.KeyCode() - '0'), false);
					else if(VK_NUMPAD0 <= inKc.KeyCode() && inKc.KeyCode() <= VK_NUMPAD9)
						Add(newKc, static_cast<CommandID>(kcSetSpacing0 + inKc.KeyCode() - VK_NUMPAD0), false);
				}
			}

		}
	}
	if (m_enforceRule[krPropagateNotes])
	{
		if((inCmd >= kcVPStartNotes && inCmd <= kcVPEndNotes) || (inCmd >= kcVPStartNoteStops && inCmd <= kcVPEndNoteStops))
		{
			const bool areNoteStarts = (inCmd >= kcVPStartNotes && inCmd <= kcVPEndNotes);
			const auto startNote = areNoteStarts ? kcVPStartNotes : kcVPStartNoteStops;
			const auto noteOffset = inCmd - startNote;
			for(const auto &range : NoteRanges)
			{
				const auto contextStartNote = areNoteStarts ? range.first : range.second;
				const auto context = ContextFromCommand(contextStartNote);

				if(contextStartNote == startNote)
					continue;

				newKc = inKc;
				newKc.Context(context);

				if(adding)
				{
					LOG_COMMANDSET(U_("Enforcing rule krPropagateNotes: adding Note on/off"));
					Add(newKc, static_cast<CommandID>(contextStartNote + noteOffset), false);
				} else
				{
					LOG_COMMANDSET(U_("Enforcing rule krPropagateNotes: removing Note on/off"));
					Remove(newKc, static_cast<CommandID>(contextStartNote + noteOffset));
				}
			}
		} else if(inCmd == kcNoteCut || inCmd == kcNoteOff || inCmd == kcNoteFade)
		{
			// Stop preview in instrument browser
			KeyCombination newKcTree = inKc;
			newKcTree.Context(kCtxViewTree);
			if(adding)
			{
				Add(newKcTree, kcTreeViewStopPreview, false);
			} else
			{
				Remove(newKcTree, kcTreeViewStopPreview);
			}
		}
	}
	if (m_enforceRule[krCheckModifiers])
	{
		// for all commands that must be modifiers
		for (auto curCmd : ModifierCommands)
		{
			//for all of this command's key combinations
			for (auto &kc : m_commands[curCmd].kcList)
			{
				if(!kc.IsModifierCombination())
				{
					//replace with dummy
					kc.Modifier(ModShift);
					kc.KeyCode(0);
					kc.EventType(kKeyEventNone);
				}
			}

		}
	}
	if (m_enforceRule[krPropagateSampleManipulation])
	{
		static constexpr CommandID propagateCmds[] = {kcSampleLoad, kcSampleSave, kcSampleNew};
		static constexpr CommandID translatedCmds[] = {kcInstrumentLoad, kcInstrumentSave, kcInstrumentNew};
		if(const auto propCmd = std::find(std::begin(propagateCmds), std::end(propagateCmds), inCmd); propCmd != std::end(propagateCmds))
		{
			//propagate to InstrumentView
			const auto newCmd = translatedCmds[std::distance(std::begin(propagateCmds), propCmd)];
			if(kcFirst <= inCmd && inCmd < kcNumCommands)
			{
				m_commands[newCmd].kcList.reserve(m_commands[inCmd].kcList.size());
				for(auto kc : m_commands[inCmd].kcList)
				{
					kc.Context(kCtxViewInstruments);
					m_commands[newCmd].kcList.push_back(kc);
				}
			}
		}

	}

/*	if (enforceRule[krFoldEffectColumnAnd])
	{
		if (inKc.ctx == kCtxViewPatternsFX) {
			KeyCombination newKc = inKc;
			newKc.ctx = kCtxViewPatternsFXparam;
			if (adding)	{
				Add(newKc, inCmd, false);
			} else {
				Remove(newKc, inCmd);
			}
		}
		if (inKc.ctx == kCtxViewPatternsFXparam) {
			KeyCombination newKc = inKc;
			newKc.ctx = kCtxViewPatternsFX;
			if (adding)	{
				Add(newKc, inCmd, false);
			} else {
				Remove(newKc, inCmd);
			}
		}
	}
*/
}


//Generate a keymap from a command set
void CCommandSet::GenKeyMap(KeyMap &km)
{
	std::vector<KeyEventType> eventTypes;
	std::vector<InputTargetContext> contexts;

	km.clear();

	const bool allowDupes = TrackerSettings::Instance().MiscAllowMultipleCommandsPerKey;

	// Copy commandlist content into map:
	for(UINT cmd = kcFirst; cmd < kcNumCommands; cmd++)
	{
		if(m_commands[cmd].IsDummy())
			continue;

		for(auto curKc : m_commands[cmd].kcList)
		{
			eventTypes.clear();
			contexts.clear();

			// Handle keyEventType mask.
			if(curKc.EventType() & kKeyEventDown)
				eventTypes.push_back(kKeyEventDown);
			if(curKc.EventType() & kKeyEventUp)
				eventTypes.push_back(kKeyEventUp);
			if(curKc.EventType() & kKeyEventRepeat)
				eventTypes.push_back(kKeyEventRepeat);
			//ASSERT(eventTypes.GetSize()>0);

			// Handle super-contexts (contexts that represent a set of sub contexts)
			if(curKc.Context() == kCtxViewPatterns)
				contexts.insert(contexts.end(), {kCtxViewPatternsNote, kCtxViewPatternsIns, kCtxViewPatternsVol, kCtxViewPatternsFX, kCtxViewPatternsFXparam});
			else if(curKc.Context() == kCtxCtrlPatterns)
				contexts.push_back(kCtxCtrlOrderlist);
			else
				contexts.push_back(curKc.Context());

			for(auto ctx : contexts)
			{
				for(auto event : eventTypes)
				{
					KeyCombination kc(ctx, curKc.Modifier(), curKc.KeyCode(), event);
					if(!allowDupes)
					{
						KeyMapRange dupes = km.equal_range(kc);
						km.erase(dupes.first, dupes.second);
					}
					km.insert(std::make_pair(kc, static_cast<CommandID>(cmd)));
				}
			}
		}
	}
}


void CCommandSet::Copy(const CCommandSet &source)
{
	m_currentModSpecs = source.m_currentModSpecs;
	std::copy(std::begin(source.m_commands), std::end(source.m_commands), std::begin(m_commands));
}


// Export

bool CCommandSet::SaveFile(const mpt::PathString &filename)
{

/* Layout:
//----( Context1 Text (id) )----
ctx:UID:Description:Modifier:Key:EventMask
ctx:UID:Description:Modifier:Key:EventMask
...
//----( Context2 Text (id) )----
...
*/

	mpt::IO::SafeOutputFile sf(filename, std::ios::out, mpt::IO::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
	mpt::IO::ofstream& f = sf;
	if(!f)
	{
		ErrorBox(IDS_CANT_OPEN_FILE_FOR_WRITING);
		return false;
	}
	f << "//----------------- OpenMPT key binding definition file  ---------------\n"
	     "//- Format is:                                                         -\n"
	     "//- Context:Command ID:Modifiers:Key:KeypressEventType     //Comments  -\n"
	     "//----------------------------------------------------------------------\n"
	     "version:" << mpt::ToCharset(mpt::Charset::ASCII, Version::Current().ToUString()) << "\n";

	const std::vector<HKL> layouts = GetKeyboardLayouts();

	for(int ctx = 0; ctx < kCtxMaxInputContexts; ctx++)
	{
		f << "\n//----( " << mpt::ToCharset(mpt::Charset::UTF8, KeyCombination::GetContextText((InputTargetContext)ctx)) << " )------------\n";

		for(int cmd = kcFirst; cmd < kcNumCommands; cmd++)
		{
			if(m_commands[cmd].IsHidden())
				continue;

			for(const auto &kc : m_commands[cmd].kcList)
			{
				if(kc.Context() != ctx)
					continue;  // Sort by context

				f << ctx << ":"  // Context technically no longer needed here, just kept for backwards compatibility
					<< m_commands[cmd].ID() << ":"
					<< static_cast<int>(kc.Modifier().GetRaw()) << ":"
					<< kc.KeyCode();
				if(cmd >= kcVPStartNotes && cmd <= kcVPEndNotes)
				{
					UINT sc = 0, vk = kc.KeyCode();
					for(auto i = layouts.begin(); i != layouts.end() && sc == 0; i++)
					{
						sc = MapVirtualKeyEx(vk, MAPVK_VK_TO_VSC, *i);
					}
					f << "/" << sc;
				}
				f << ":"
					<< static_cast<int>(kc.EventType().GetRaw()) << "\t\t//"
					<< mpt::ToCharset(mpt::Charset::UTF8, GetCommandText((CommandID)cmd)) << ": "
					<< mpt::ToCharset(mpt::Charset::UTF8, kc.GetKeyText()) << " ("
					<< mpt::ToCharset(mpt::Charset::UTF8, kc.GetKeyEventText()) << ")\n";
			}
		}
	}

	return true;
}


bool CCommandSet::LoadFile(std::istream &iStrm, const mpt::ustring &filenameDescription)
{
	Version keymapVersion;
	KeyCombination kc;
	char s[1024];
	std::string curLine;
	std::vector<std::string> tokens;
	int l = 0;

	for(auto &cmd : m_commands)
		cmd.kcList.clear();

	CString errText;
	int errorCount = 0;

	const std::vector<HKL> layouts = GetKeyboardLayouts();

	const std::string whitespace(" \n\r\t");
	while(iStrm.getline(s, std::size(s)))
	{
		curLine = s;
		l++;

		// Cut everything after a //, trim whitespace
		auto pos = curLine.find("//");
		if(pos != std::string::npos)
			curLine.resize(pos);
		pos = curLine.find_first_not_of(whitespace);
		if(pos == std::string::npos)
			continue;
		curLine.erase(0, pos);
		pos = curLine.find_last_not_of(whitespace);
		if(pos != std::string::npos)
			curLine.resize(pos + 1);

		if (curLine.empty())
			continue;

		tokens = mpt::split(curLine, std::string(":"));
		if(tokens.size() == 2 && !mpt::CompareNoCaseAscii(tokens[0], "version"))
		{
			// This line indicates the version of this keymap file (e.g. "version:1" on older versions, in newer version the OpenMPT version that was used to save the file)
			keymapVersion = Version::Parse(mpt::ToUnicode(mpt::Charset::ASCII, tokens[1]));
			if(keymapVersion > Version::Current())
			{
				errText += MPT_CFORMAT("Keymap was saved with OpenMPT {}, but you are running OpenMPT {}.\n")(keymapVersion, Version::Current());
			}
			continue;
		}

		// Format: ctx:UID:Description:Modifier:Key:EventMask
		CommandID cmd = kcNumCommands;
		if(tokens.size() >= 5)
		{
			cmd = FindCmd(mpt::parse<uint32>(tokens[1]));

			// Modifier
			kc.Modifier(static_cast<Modifiers>(mpt::parse<int>(tokens[2])));

			// Virtual Key code / Scan code
			UINT vk = 0;
			auto scPos = tokens[3].find('/');
			if(scPos != std::string::npos)
			{
				// Scan code present
				UINT sc = mpt::parse<UINT>(tokens[3].substr(scPos + 1));
				for(auto i = layouts.begin(); i != layouts.end() && vk == 0; i++)
				{
					vk = MapVirtualKeyEx(sc, MAPVK_VSC_TO_VK, *i);
				}
			}
			if(vk == 0)
			{
				vk = mpt::parse<UINT>(tokens[3]);
			}
			kc.KeyCode(vk);

			// Event
			kc.EventType(static_cast<KeyEventType>(mpt::parse<int>(tokens[4])));
		}

		// Error checking
		if(cmd < kcFirst || cmd >= kcNumCommands || tokens.size() < 4)
		{
			errorCount++;
			if (errorCount < 10)
			{
				if(tokens.size() < 4)
					errText.AppendFormat(_T("Line %d was not understood.\n"), l);
				else
					errText.AppendFormat(_T("Line %d contained an unknown command.\n"), l);
			} else if (errorCount == 10)
			{
				errText += _T("Too many errors detected, not reporting any more.\n");
			}
		} else
		{
			Add(kc, cmd, true, -1, true);
		}
	}

	ApplyDefaultKeybindings(KeyboardPreset::MPT, keymapVersion);

	// Fix up old keymaps containing legacy commands that have been merged into other commands
	static constexpr std::pair<CommandID, CommandID> MergeCommands[] =
	{
		{kcFileSaveAsMP3, kcFileSaveAsWave},
		{kcNoteCutOld, kcNoteCut},
		{kcNoteOffOld, kcNoteOff},
		{kcNoteFadeOld, kcNoteFade},
	};
	for(const auto [from, to] : MergeCommands)
	{
		m_commands[to].kcList.insert(m_commands[to].kcList.end(), m_commands[from].kcList.begin(), m_commands[from].kcList.end());
		m_commands[from].kcList.clear();
	}

	if(!errText.IsEmpty())
	{
		Reporting::Warning(MPT_CFORMAT("The following problems have been encountered while trying to load the key binding file {}:\n{}")
			(mpt::ToCString(filenameDescription), errText));
	}

	return true;
}


bool CCommandSet::LoadFile(const mpt::PathString &filename)
{
	mpt::ifstream fin(filename);
	if(fin.fail())
	{
		Reporting::Warning(MPT_TFORMAT("Can't open key bindings file {} for reading. Default key bindings will be used.")(filename));
		return false;
	} else
	{
		return LoadFile(fin, filename.ToUnicode());
	}
}


void CCommandSet::LoadDefaultKeymap(KeyboardPreset preset)
{
	for(auto &cmd : m_commands)
		cmd.kcList.clear();
	ApplyDefaultKeybindings(KeyboardPreset::MPT);

	if(preset == KeyboardPreset::MPT)
		return;

	const auto defaults = (preset == KeyboardPreset::IT) ? mpt::as_span(DefaultKeybindingsIT) : mpt::as_span(DefaultKeybindingsFT2);
	// Remove all pre-populated notes
	for(CommandID cmd = kcVPStartNotes; cmd <= kcVPEndNotes; cmd = static_cast<CommandID>(cmd + 1))
	{
		for(const auto &kc : m_commands[cmd].kcList)
		{
			EnforceAll(kc, cmd, false);
		}
		m_commands[cmd].kcList.clear();
	}
	// Also remove any other keys that are going to be overwritten
	for (const auto &key : defaults)
	{
		for(const auto &kc : m_commands[key.cmd].kcList)
		{
			EnforceAll(kc, key.cmd, false);
		}
		m_commands[key.cmd].kcList.clear();
	}
	ApplyDefaultKeybindings(preset);
}


void CCommandSet::ApplyDefaultKeybindings(KeyboardPreset preset, const Version onlyCommandsAfterVersion)
{
	if(m_currentModSpecs)
	{
		const auto specs = m_currentModSpecs;
		m_currentModSpecs = nullptr;
		QuickChange_SetEffects(*specs);
	}

	const std::vector<HKL> layouts = GetKeyboardLayouts();

	mpt::span<const DefaultKeybinding> defaults;
	switch(preset)
	{
	case KeyboardPreset::MPT: defaults = DefaultKeybindings; break;
	case KeyboardPreset::IT: defaults = DefaultKeybindingsIT; break;
	case KeyboardPreset::FT2: defaults = DefaultKeybindingsFT2; break;
	}

	const bool onlyNewShortcuts = onlyCommandsAfterVersion != Version{};
	CommandID lastAdded = kcNull;
	for(const auto &kb : defaults)
	{
		if(onlyNewShortcuts)
		{
			if(kb.addedInVersion <= onlyCommandsAfterVersion)
				continue;

			// Do not map shortcuts that already have custom keys assigned.
			// In particular with default note keys, this can create awkward keymaps when loading
			// e.g. an IT-style keymap and it contains two keys mapped to the same notes.
			if(kb.cmd != lastAdded && GetKeyListSize(kb.cmd) != 0)
				continue;
		}

		KeyCombination kc;
		kc.Context(ContextFromCommand(kb.cmd));
		kc.Modifier(kb.modifiers);
		kc.EventType(kb.events);

		if(kb.key & 0x8000)
		{
			// Virtual Key code / Scan code
			UINT vk = 0;
			for(auto i = layouts.begin(); i != layouts.end() && vk == 0; i++)
			{
				vk = MapVirtualKeyEx(kb.key & 0x7FFF, MAPVK_VSC_TO_VK, *i);
			}
			if(vk)
				kc.KeyCode(vk);
			else
				kc.KeyCode(kb.key & 0x7FFF);
		} else
		{
			kc.KeyCode(kb.key);
		}

		if(auto conflictCmd = IsConflicting(kc, kb.cmd, false); conflictCmd.first != kcNull)
		{
			if(!onlyNewShortcuts && conflictCmd.second.Context() == kc.Context())
				continue;
			// Allow cross-context conflicts in case the newly added shortcut is in a more generic context
			// - unless the conflicting shortcut is the reserved dummy shortcut (which was used to prevent
			// default shortcuts from being added back before default key binding versioning was added).
			if(conflictCmd.first == kcDummyShortcut)
				continue;
			if(onlyNewShortcuts && !m_isParentContext[kc.Context()][conflictCmd.second.Context()])
				continue;
		}

		m_commands[kb.cmd].kcList.push_back(kc);
		EnforceAll(kc, kb.cmd, true);
		lastAdded = kb.cmd;
	}
}


CommandID CCommandSet::FindCmd(uint32 uid) const
{
	for(int i = kcFirst; i < kcNumCommands; i++)
	{
		if(m_commands[i].ID() == uid)
			return static_cast<CommandID>(i);
	}
	return kcNull;
}


CString KeyCombination::GetContextText(InputTargetContext ctx)
{
	switch(ctx)
	{
		case kCtxAllContexts:			return _T("Global Context");
		case kCtxViewGeneral:			return _T("General Context [bottom]");
		case kCtxViewPatterns:			return _T("Pattern Context [bottom]");
		case kCtxViewPatternsNote:		return _T("Pattern Context [bottom] - Note Col");
		case kCtxViewPatternsIns:		return _T("Pattern Context [bottom] - Ins Col");
		case kCtxViewPatternsVol:		return _T("Pattern Context [bottom] - Vol Col");
		case kCtxViewPatternsFX:		return _T("Pattern Context [bottom] - FX Col");
		case kCtxViewPatternsFXparam:	return _T("Pattern Context [bottom] - Param Col");
		case kCtxViewSamples:			return _T("Sample Context [bottom]");
		case kCtxViewInstruments:		return _T("Instrument Context [bottom]");
		case kCtxViewComments:			return _T("Comments Context [bottom]");
		case kCtxViewTree:				return _T("Tree View");
		case kCtxCtrlGeneral:			return _T("General Context [top]");
		case kCtxCtrlPatterns:			return _T("Pattern Context [top]");
		case kCtxCtrlSamples:			return _T("Sample Context [top]");
		case kCtxCtrlInstruments:		return _T("Instrument Context [top]");
		case kCtxCtrlComments:			return _T("Comments Context [top]");
		case kCtxCtrlOrderlist:			return _T("Orderlist");
		case kCtxVSTGUI:				return _T("Plugin GUI Context");
		case kCtxChannelSettings:		return _T("Quick Channel Settings Context");
		case kCtxUnknownContext:
		default:						return _T("Unknown Context");
	}
}


CString KeyCombination::GetKeyEventText(FlagSet<KeyEventType> event)
{
	CString text;

	bool first = true;
	if (event & kKeyEventDown)
	{
		first=false;
		text.Append(_T("KeyDown"));
	}
	if (event & kKeyEventRepeat)
	{
		if (!first) text.Append(_T("|"));
		text.Append(_T("KeyHold"));
		first=false;
	}
	if (event & kKeyEventUp)
	{
		if (!first) text.Append(_T("|"));
		text.Append(_T("KeyUp"));
	}

	return text;
}


CString KeyCombination::GetModifierText(FlagSet<Modifiers> mod)
{
	CString text;
	if (mod[ModShift]) text.Append(_T("Shift+"));
	if (mod[ModCtrl]) text.Append(_T("Ctrl+"));
	if (mod[ModAlt]) text.Append(_T("Alt+"));
	if (mod[ModRShift]) text.Append(_T("RShift+"));
	if (mod[ModRCtrl]) text.Append(_T("RCtrl+"));
	if (mod[ModRAlt]) text.Append(_T("RAlt+"));
	if (mod[ModWin]) text.Append(_T("Win+")); // Feature: use Windows keys as modifier keys
	if (mod[ModMidi]) text.Append(_T("MIDI"));
	return text;
}


CString KeyCombination::GetKeyText(FlagSet<Modifiers> mod, UINT code)
{
	CString keyText = GetModifierText(mod);
	if(mod[ModMidi])
	{
		if(code < 0x80)
			keyText.AppendFormat(_T(" CC %u"), code);
		else
			keyText += MPT_CFORMAT(" {}{}")(mpt::ustring(NoteNamesSharp[(code & 0x7F) % 12]), (code & 0x7F) / 12);
	} else
	{
		keyText.Append(CHotKeyCtrl::GetKeyName(code, IsExtended(code)));
	}
	//HACK:
	if (keyText == _T("Ctrl+CTRL"))			keyText = _T("Ctrl");
	else if (keyText == _T("Alt+ALT"))		keyText = _T("Alt");
	else if (keyText == _T("Shift+SHIFT"))	keyText = _T("Shift");
	else if (keyText == _T("RCtrl+CTRL"))	keyText = _T("RCtrl");
	else if (keyText == _T("RAlt+ALT"))		keyText = _T("RAlt");
	else if (keyText == _T("RShift+SHIFT"))	keyText = _T("RShift");

	return keyText;
}


bool KeyCombination::IsModifierCombination() const
{
	return Modifier() &&
		(KeyCode() == VK_SHIFT || KeyCode() == VK_CONTROL || KeyCode() == VK_MENU || KeyCode() == 0
		|| KeyCode() == VK_LWIN || KeyCode() == VK_RWIN);  // Feature: use Windows keys as modifier keys

}


CString CCommandSet::GetKeyTextFromCommand(CommandID c, UINT key) const
{
	if(key < m_commands[c].kcList.size())
		return m_commands[c].kcList[0].GetKeyText();
	if(key != uint32_max)
		return CString();
	CString keys;
	bool addSeparator = false;
	for(auto &item : m_commands[c].kcList)
	{
		if(addSeparator)
			keys += _T("; ");
		else
			addSeparator = true;
		keys += item.GetKeyText();
	}
	return keys;
}


// Quick Changes - modify many commands with one call.

bool CCommandSet::QuickChange_NotesRepeat(bool repeat)
{
	for (CommandID cmd = kcVPStartNotes; cmd <= kcVPEndNotes; cmd=(CommandID)(cmd + 1))		//for all notes
	{
		for(auto &kc : m_commands[cmd].kcList)
		{
			if(repeat)
				kc.EventType(kc.EventType() | kKeyEventRepeat);
			else
				kc.EventType(kc.EventType() & ~kKeyEventRepeat);
		}
	}
	return true;
}


bool CCommandSet::QuickChange_SetEffects(const CModSpecifications &modSpecs)
{
	// Is this already the active key configuration?
	if(&modSpecs == m_currentModSpecs)
	{
		return false;
	}
	m_currentModSpecs = &modSpecs;

	KeyCombination kc(kCtxViewPatternsFX, ModNone, 0, kKeyEventDown | kKeyEventRepeat);

	for(CommandID cmd = kcSetFXStart; cmd <= kcSetFXEnd; cmd = static_cast<CommandID>(cmd + 1))
	{
		char effect = modSpecs.GetEffectLetter(static_cast<ModCommand::COMMAND>(cmd - kcSetFXStart + 1));
		if(effect >= 'A' && effect <= 'Z')
		{
			// VkKeyScanEx needs lowercase letters
			effect = effect - 'A' + 'a';
		} else if(effect == ' ')
		{
			// We don't want to enter "empty" effects in IT / S3M
			effect = '?';
		} else if(cmd >= kcSetFXuserBegin && cmd <= kcSetFXuserEnd)
		{
			// Don't map effects that use non-alphanumeric effect letters (such as # or \), they are set up manually instead
			continue;
		}

		// Remove all old choices
		for(int p = GetKeyListSize(cmd) - 1; p >= 0; --p)
		{
			Remove(p, cmd);
		}

		if(effect != '?')
		{
			// Hack for situations where a non-latin keyboard layout without A...Z key code mapping may the current layout (e.g. Russian),
			// but a latin layout (e.g. EN-US) is installed as well.
			const std::vector<HKL> layouts = GetKeyboardLayouts();
			SHORT codeNmod = -1;
			for(auto i = layouts.begin(); i != layouts.end() && codeNmod == -1; i++)
			{
				codeNmod = VkKeyScanEx(effect, *i);
			}
			if(codeNmod != -1)
			{
				kc.KeyCode(LOBYTE(codeNmod));
				// Don't add modifier keys, since on French keyboards, numbers are input using Shift.
				// We don't really want that behaviour here, and I'm sure we don't want that in other cases on other layouts as well.
				kc.Modifier(ModNone);
				Add(kc, cmd, true);
			}

			if (effect >= '0' && effect <= '9')		// For numbers, ensure numpad works too
			{
				kc.KeyCode(VK_NUMPAD0 + (effect - '0'));
				Add(kc, cmd, true);
			}
		}
	}

	return true;
}

// Stupid MFC crap: for some reason VK code isn't enough to get correct string with GetKeyName.
// We also need to figure out the correct "extended" bit.
bool KeyCombination::IsExtended(UINT code)
{
	if (code==VK_SNAPSHOT)	//print screen
		return true;
	if (code>=VK_PRIOR && code<=VK_DOWN) //pgup, pg down, home, end,  cursor keys,
		return true;
	if (code>=VK_INSERT && code<=VK_DELETE) // ins, del
		return true;
	if (code>=VK_LWIN && code<=VK_APPS) //winkeys & application key
		return true;
	if (code==VK_DIVIDE)	//Numpad '/'
		return true;
	if (code==VK_NUMLOCK)	//print screen
		return true;
	if (code>=0xA0 && code<=0xA5) //attempt for RL mods
		return true;

	return false;
}


void CCommandSet::SetupContextHierarchy()
{
	// For now much be fully expanded (i.e. don't rely on grandparent relationships).
	m_isParentContext[kCtxAllContexts].set(kCtxViewGeneral);
	m_isParentContext[kCtxAllContexts].set(kCtxViewPatterns);
	m_isParentContext[kCtxAllContexts].set(kCtxViewPatternsNote);
	m_isParentContext[kCtxAllContexts].set(kCtxViewPatternsIns);
	m_isParentContext[kCtxAllContexts].set(kCtxViewPatternsVol);
	m_isParentContext[kCtxAllContexts].set(kCtxViewPatternsFX);
	m_isParentContext[kCtxAllContexts].set(kCtxViewPatternsFXparam);
	m_isParentContext[kCtxAllContexts].set(kCtxViewSamples);
	m_isParentContext[kCtxAllContexts].set(kCtxViewInstruments);
	m_isParentContext[kCtxAllContexts].set(kCtxViewComments);
	m_isParentContext[kCtxAllContexts].set(kCtxViewTree);
	m_isParentContext[kCtxAllContexts].set(kCtxInsNoteMap);
	m_isParentContext[kCtxAllContexts].set(kCtxVSTGUI);
	m_isParentContext[kCtxAllContexts].set(kCtxCtrlGeneral);
	m_isParentContext[kCtxAllContexts].set(kCtxCtrlPatterns);
	m_isParentContext[kCtxAllContexts].set(kCtxCtrlSamples);
	m_isParentContext[kCtxAllContexts].set(kCtxCtrlInstruments);
	m_isParentContext[kCtxAllContexts].set(kCtxCtrlComments);
	m_isParentContext[kCtxAllContexts].set(kCtxCtrlSamples);
	m_isParentContext[kCtxAllContexts].set(kCtxCtrlOrderlist);
	m_isParentContext[kCtxAllContexts].set(kCtxChannelSettings);

	m_isParentContext[kCtxViewPatterns].set(kCtxViewPatternsNote);
	m_isParentContext[kCtxViewPatterns].set(kCtxViewPatternsIns);
	m_isParentContext[kCtxViewPatterns].set(kCtxViewPatternsVol);
	m_isParentContext[kCtxViewPatterns].set(kCtxViewPatternsFX);
	m_isParentContext[kCtxViewPatterns].set(kCtxViewPatternsFXparam);
	m_isParentContext[kCtxCtrlPatterns].set(kCtxCtrlOrderlist);

}


bool CCommandSet::KeyCombinationConflict(KeyCombination kc1, KeyCombination kc2, bool checkEventConflict) const
{
	bool modConflict      = (kc1.Modifier()==kc2.Modifier());
	bool codeConflict     = (kc1.KeyCode()==kc2.KeyCode());
	bool eventConflict    = ((kc1.EventType()&kc2.EventType()));
	bool ctxConflict      = (kc1.Context() == kc2.Context());
	bool crossCxtConflict = IsCrossContextConflict(kc1, kc2);

	bool conflict = modConflict && codeConflict && (eventConflict || !checkEventConflict) &&
		(ctxConflict || crossCxtConflict);

	return conflict;
}


bool CCommandSet::IsCrossContextConflict(KeyCombination kc1, KeyCombination kc2) const
{
	return m_isParentContext[kc1.Context()][kc2.Context()] || m_isParentContext[kc2.Context()][kc1.Context()];
}


InputTargetContext CCommandSet::ContextFromCommand(CommandID cmd)
{
	static constexpr std::tuple<InputTargetContext, CommandID, CommandID> ContextCommandRanges[] =
	{
		{kCtxAllContexts,         kcGlobalStart,              kcGlobalEnd             },
		{kCtxCtrlOrderlist,       kcStartOrderlistCommands,   kcEndOrderlistCommands  },
		{kCtxChannelSettings,     kcStartChnSettingsCommands, kcEndChnSettingsCommands},
		{kCtxViewPatterns,        kcStartPatternGeneral,      kcEndPatternGeneral     },
		{kCtxViewPatternsNote,    kcStartNoteColumn,          kcEndNoteColumn         },
		{kCtxViewPatternsIns,     kcSetIns0,                  kcSetIns9               },
		{kCtxViewPatternsVol,     kcSetVolumeStart,           kcSetVolumeEnd          },
		{kCtxViewPatternsFX,      kcSetFXStart,               kcSetFXEnd              },
		{kCtxViewPatternsFXparam, kcSetFXParam0,              kcSetFXParamF           },
		{kCtxViewSamples,         kcStartSampleView,          kcEndSampleView         },
		{kCtxCtrlInstruments,     kcStartInstrumentCtrl,      kcEndInstrumentCtrl,    },
		{kCtxInsNoteMap,          kcStartInsNoteMap,          kcEndInsNoteMap         },
		{kCtxViewInstruments,     kcStartInsEnvelopeEdit,     kcEndInsEnvelopeEdit    },
		{kCtxViewComments,        kcStartCommentsCommands,    kcEndCommentsCommands   },
		{kCtxVSTGUI,              kcStartVSTGUICommands,      kcEndVSTGUICommands     },
		{kCtxViewTree,            kcStartTreeViewCommands,    kcEndTreeViewCommands   },
	};
	for(const auto &[context, first, last] : ContextCommandRanges)
	{
		if(mpt::is_in_range(cmd, first, last))
			return context;
	}
	MPT_ASSERT_NOTREACHED();
	return kCtxUnknownContext;
}


bool CCommandSet::MustBeModifierKey(CommandID id)
{
	return mpt::contains(ModifierCommands, id);
}


OPENMPT_NAMESPACE_END
