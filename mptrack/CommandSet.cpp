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
#include "resource.h"
#include "Mptrack.h"	// For ErrorBox
#include "../soundlib/mod_specifications.h"
#include "../mptrack/Reporting.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/mptFileIO.h"
#include "../common/mptBufferIO.h"
#include "TrackerSettings.h"


OPENMPT_NAMESPACE_BEGIN

enum { KEYMAP_VERSION = 1 };	// Version of the .mkb format


#define ENABLE_LOGGING 0

#if(ENABLE_LOGGING)
	//
#else
	#undef Log
	#define Log
#endif


CCommandSet::CCommandSet()
//------------------------
{
	// Which keybinding rules to enforce?
	enforceRule[krPreventDuplicate]				= true;
	enforceRule[krDeleteOldOnConflict]			= true;
	enforceRule[krAllowNavigationWithSelection]	= true;
	enforceRule[krAllowSelectionWithNavigation] = true;
	enforceRule[krAllowSelectCopySelectCombos]	= true;
	enforceRule[krLockNotesToChords]			= true;
	enforceRule[krNoteOffOnKeyRelease]			= true;
	enforceRule[krPropagateNotes]				= true;
	enforceRule[krReassignDigitsToOctaves]		= false;
	enforceRule[krAutoSelectOff]				= true;
	enforceRule[krAutoSpacing]					= true;
	enforceRule[krCheckModifiers]				= true;
	enforceRule[krPropagateSampleManipulation]  = true;
//	enforceRule[krCheckContextHierarchy]		= true;

	SetupCommands();
	SetupContextHierarchy();

	oldSpecs = nullptr;
}


//-------------------------------------------------------
// Setup
//-------------------------------------------------------

// Helper function for setting up commands
void CCommandSet::DefineKeyCommand(CommandID kc, UINT uid, const TCHAR *message, enmKcVisibility visibility, enmKcDummy dummy)
//----------------------------------------------------------------------------------------------------------------------------
{
	commands[kc].UID = uid;
	commands[kc].isHidden = (visibility == kcHidden);
	commands[kc].isDummy = (dummy == kcDummy);
	commands[kc].Message = message;
}


//Get command descriptions etc.. loaded up.
void CCommandSet::SetupCommands()
//-------------------------------
{	//TODO: make this hideous list a bit nicer, with a constructor or somthing.
	//NOTE: isHidden implies automatically set, since the user will not be able to see it.

	DefineKeyCommand(kcPatternRecord, 1001, _T("Enable Recording"));
	DefineKeyCommand(kcPatternPlayRow, 1002, _T("Play Row"));
	DefineKeyCommand(kcCursorCopy, 1003, _T("Quick Copy"));
	DefineKeyCommand(kcCursorPaste, 1004, _T("Quick Paste"));
	DefineKeyCommand(kcChannelMute, 1005, _T("Mute Current Channel"));
	DefineKeyCommand(kcChannelSolo, 1006, _T("Solo Current Channel"));
	DefineKeyCommand(kcTransposeUp, 1007, _T("Transpose +1"));
	DefineKeyCommand(kcTransposeDown, 1008, _T("Transpose -1"));
	DefineKeyCommand(kcTransposeOctUp, 1009, _T("Transpose +1 Octave"));
	DefineKeyCommand(kcTransposeOctDown, 1010, _T("Transpose -1 Octave"));
	DefineKeyCommand(kcSelectChannel, 1011, _T("Select Channel / Select All"));
	DefineKeyCommand(kcPatternAmplify, 1012, _T("Amplify selection"));
	DefineKeyCommand(kcPatternSetInstrument, 1013, _T("Apply current instrument"));
	DefineKeyCommand(kcPatternInterpolateVol, 1014, _T("Interpolate Volume"));
	DefineKeyCommand(kcPatternInterpolateEffect, 1015, _T("Interpolate Effect"));
	DefineKeyCommand(kcPatternVisualizeEffect, 1016, _T("Open Effect Visualizer"));
	DefineKeyCommand(kcPatternJumpDownh1, 1017, _T("Jump down by measure"));
	DefineKeyCommand(kcPatternJumpUph1, 1018, _T("Jump up by measure"));
	DefineKeyCommand(kcPatternSnapDownh1, 1019, _T("Snap down to measure"));
	DefineKeyCommand(kcPatternSnapUph1, 1020, _T("Snap up to measure"));
	DefineKeyCommand(kcViewGeneral, 1021, _T("View General"));
	DefineKeyCommand(kcViewPattern, 1022, _T("View Pattern"));
	DefineKeyCommand(kcViewSamples, 1023, _T("View Samples"));
	DefineKeyCommand(kcViewInstruments, 1024, _T("View Instruments"));
	DefineKeyCommand(kcViewComments, 1025, _T("View Comments"));
	DefineKeyCommand(kcPlayPatternFromCursor, 1026, _T("Play Pattern from Cursor"));
	DefineKeyCommand(kcPlayPatternFromStart, 1027, _T("Play Pattern from Start"));
	DefineKeyCommand(kcPlaySongFromCursor, 1028, _T("Play Song from Cursor"));
	DefineKeyCommand(kcPlaySongFromStart, 1029, _T("Play Song from Start"));
	DefineKeyCommand(kcPlayPauseSong, 1030, _T("Play Song / Pause song"));
	DefineKeyCommand(kcPauseSong, 1031, _T("Pause Song"));
	DefineKeyCommand(kcPrevInstrument, 1032, _T("Previous Instrument"));
	DefineKeyCommand(kcNextInstrument, 1033, _T("Next Instrument"));
	DefineKeyCommand(kcPrevOrder, 1034, _T("Previous Order"));
	DefineKeyCommand(kcNextOrder, 1035, _T("Next Order"));
	DefineKeyCommand(kcPrevOctave, 1036, _T("Previous Octave"));
	DefineKeyCommand(kcNextOctave, 1037, _T("Next Octave"));
	DefineKeyCommand(kcNavigateDown, 1038, _T("Navigate down by 1 row"));
	DefineKeyCommand(kcNavigateUp, 1039, _T("Navigate up by 1 row"));
	DefineKeyCommand(kcNavigateLeft, 1040, _T("Navigate left"));
	DefineKeyCommand(kcNavigateRight, 1041, _T("Navigate right"));
	DefineKeyCommand(kcNavigateNextChan, 1042, _T("Navigate to next channel"));
	DefineKeyCommand(kcNavigatePrevChan, 1043, _T("Navigate to previous channel"));
	DefineKeyCommand(kcHomeHorizontal, 1044, _T("Go to first channel"));
	DefineKeyCommand(kcHomeVertical, 1045, _T("Go to first row"));
	DefineKeyCommand(kcHomeAbsolute, 1046, _T("Go to first row of first channel"));
	DefineKeyCommand(kcEndHorizontal, 1047, _T("Go to last channel"));
	DefineKeyCommand(kcEndVertical, 1048, _T("Go to last row"));
	DefineKeyCommand(kcEndAbsolute, 1049, _T("Go to last row of last channel"));
	DefineKeyCommand(kcSelect, 1050, _T("Selection key"));
	DefineKeyCommand(kcCopySelect, 1051, _T("Copy select key"));
	DefineKeyCommand(kcSelectOff, 1052, _T("Deselect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcCopySelectOff, 1053, _T("Copy deselect key"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcNextPattern, 1054, _T("Next pattern"));
	DefineKeyCommand(kcPrevPattern, 1055, _T("Previous pattern"));
	//DefineKeyCommand(kcClearSelection, 1056, _T("Wipe selection"));
	DefineKeyCommand(kcClearRow, 1057, _T("Clear row"));
	DefineKeyCommand(kcClearField, 1058, _T("Clear field"));
	DefineKeyCommand(kcClearRowStep, 1059, _T("Clear row and step"));
	DefineKeyCommand(kcClearFieldStep, 1060, _T("Clear field and step"));
	DefineKeyCommand(kcDeleteRows, 1061, _T("Delete rows"));
	DefineKeyCommand(kcShowNoteProperties, 1062, _T("Show Note Properties"));
	DefineKeyCommand(kcShowEditMenu, 1063, _T("Show Context (Right-Click) Menu"));
	DefineKeyCommand(kcVPNoteC_0, 1064, _T("Base octave C"));
	DefineKeyCommand(kcVPNoteCS0, 1065, _T("Base octave C#"));
	DefineKeyCommand(kcVPNoteD_0, 1066, _T("Base octave D"));
	DefineKeyCommand(kcVPNoteDS0, 1067, _T("Base octave D#"));
	DefineKeyCommand(kcVPNoteE_0, 1068, _T("Base octave E"));
	DefineKeyCommand(kcVPNoteF_0, 1069, _T("Base octave F"));
	DefineKeyCommand(kcVPNoteFS0, 1070, _T("Base octave F#"));
	DefineKeyCommand(kcVPNoteG_0, 1071, _T("Base octave G"));
	DefineKeyCommand(kcVPNoteGS0, 1072, _T("Base octave G#"));
	DefineKeyCommand(kcVPNoteA_1, 1073, _T("Base octave A"));
	DefineKeyCommand(kcVPNoteAS1, 1074, _T("Base octave A#"));
	DefineKeyCommand(kcVPNoteB_1, 1075, _T("Base octave B"));
	DefineKeyCommand(kcVPNoteC_1, 1076, _T("Base octave +1 C"));
	DefineKeyCommand(kcVPNoteCS1, 1077, _T("Base octave +1 C#"));
	DefineKeyCommand(kcVPNoteD_1, 1078, _T("Base octave +1 D"));
	DefineKeyCommand(kcVPNoteDS1, 1079, _T("Base octave +1 D#"));
	DefineKeyCommand(kcVPNoteE_1, 1080, _T("Base octave +1 E"));
	DefineKeyCommand(kcVPNoteF_1, 1081, _T("Base octave +1 F"));
	DefineKeyCommand(kcVPNoteFS1, 1082, _T("Base octave +1 F#"));
	DefineKeyCommand(kcVPNoteG_1, 1083, _T("Base octave +1 G"));
	DefineKeyCommand(kcVPNoteGS1, 1084, _T("Base octave +1 G#"));
	DefineKeyCommand(kcVPNoteA_2, 1085, _T("Base octave +1 A"));
	DefineKeyCommand(kcVPNoteAS2, 1086, _T("Base octave +1 A#"));
	DefineKeyCommand(kcVPNoteB_2, 1087, _T("Base octave +1 B"));
	DefineKeyCommand(kcVPNoteC_2, 1088, _T("Base octave +2 C"));
	DefineKeyCommand(kcVPNoteCS2, 1089, _T("Base octave +2 C#"));
	DefineKeyCommand(kcVPNoteD_2, 1090, _T("Base octave +2 D"));
	DefineKeyCommand(kcVPNoteDS2, 1091, _T("Base octave +2 D#"));
	DefineKeyCommand(kcVPNoteE_2, 1092, _T("Base octave +2 E"));
	DefineKeyCommand(kcVPNoteF_2, 1093, _T("Base octave +2 F"));
	DefineKeyCommand(kcVPNoteFS2, 1094, _T("Base octave +2 F#"));
	DefineKeyCommand(kcVPNoteG_2, 1095, _T("Base octave +2 G"));
	DefineKeyCommand(kcVPNoteGS2, 1096, _T("Base octave +2 G#"));
	DefineKeyCommand(kcVPNoteA_3, 1097, _T("Base octave +2 A"));
	DefineKeyCommand(kcVPNoteStopC_0, 1098, _T("Stop base octave C"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopCS0, 1099, _T("Stop base octave C#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopD_0, 1100, _T("Stop base octave D"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopDS0, 1101, _T("Stop base octave D#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopE_0, 1102, _T("Stop base octave E"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopF_0, 1103, _T("Stop base octave F"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopFS0, 1104, _T("Stop base octave F#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopG_0, 1105, _T("Stop base octave G"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopGS0, 1106, _T("Stop base octave G#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopA_1, 1107, _T("Stop base octave +1 A"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopAS1, 1108, _T("Stop base octave +1 A#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopB_1, 1109, _T("Stop base octave +1 B"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopC_1, 1110, _T("Stop base octave +1 C"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopCS1, 1111, _T("Stop base octave +1 C#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopD_1, 1112, _T("Stop base octave +1 D"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopDS1, 1113, _T("Stop base octave +1 D#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopE_1, 1114, _T("Stop base octave +1 E"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopF_1, 1115, _T("Stop base octave +1 F"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopFS1, 1116, _T("Stop base octave +1 F#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopG_1, 1117, _T("Stop base octave +1 G"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopGS1, 1118, _T("Stop base octave +1 G#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopA_2, 1119, _T("Stop base octave +2 A"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopAS2, 1120, _T("Stop base octave +2 A#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopB_2, 1121, _T("Stop base octave +2 B"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopC_2, 1122, _T("Stop base octave +2 C"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopCS2, 1123, _T("Stop base octave +2 C#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopD_2, 1124, _T("Stop base octave +2 D"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopDS2, 1125, _T("Stop base octave +2 D#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopE_2, 1126, _T("Stop base octave +2 E"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopF_2, 1127, _T("Stop base octave +2 F"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopFS2, 1128, _T("Stop base octave +2 F#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopG_2, 1129, _T("Stop base octave +2 G"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopGS2, 1130, _T("Stop base octave +2 G#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPNoteStopA_3, 1131, _T("Stop base octave +3 A"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordC_0, 1132, _T("base octave chord C"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordCS0, 1133, _T("base octave chord C#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordD_0, 1134, _T("base octave chord D"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordDS0, 1135, _T("base octave chord D#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordE_0, 1136, _T("base octave chord E"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordF_0, 1137, _T("base octave chord F"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordFS0, 1138, _T("base octave chord F#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordG_0, 1139, _T("base octave chord G"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordGS0, 1140, _T("base octave chord G#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordA_1, 1141, _T("base octave +1 chord A"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordAS1, 1142, _T("base octave +1 chord A#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordB_1, 1143, _T("base octave +1 chord B"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordC_1, 1144, _T("base octave +1 chord C"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordCS1, 1145, _T("base octave +1 chord C#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordD_1, 1146, _T("base octave +1 chord D"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordDS1, 1147, _T("base octave +1 chord D#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordE_1, 1148, _T("base octave +1 chord E"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordF_1, 1149, _T("base octave +1 chord F"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordFS1, 1150, _T("base octave +1 chord F#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordG_1, 1151, _T("base octave +1 chord G"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordGS1, 1152, _T("base octave +1 chord G#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordA_2, 1153, _T("base octave +2 chord A"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordAS2, 1154, _T("base octave +2 chord A#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordB_2, 1155, _T("base octave +2 chord B"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordC_2, 1156, _T("base octave +2 chord C"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordCS2, 1157, _T("base octave +2 chord C#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordD_2, 1158, _T("base octave +2 chord D"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordDS2, 1159, _T("base octave +2 chord D#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordE_2, 1160, _T("base octave +2 chord E"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordF_2, 1161, _T("base octave +2 chord F"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordFS2, 1162, _T("base octave +2 chord F#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordG_2, 1163, _T("base octave +2 chord G"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordGS2, 1164, _T("base octave +2 chord G#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordA_3, 1165, _T("base octave chord +3 A"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopC_0, 1166, _T("Stop base octave chord C"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopCS0, 1167, _T("Stop base octave chord C#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopD_0, 1168, _T("Stop base octave chord D"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopDS0, 1169, _T("Stop base octave chord D#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopE_0, 1170, _T("Stop base octave chord E"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopF_0, 1171, _T("Stop base octave chord F"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopFS0, 1172, _T("Stop base octave chord F#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopG_0, 1173, _T("Stop base octave chord G"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopGS0, 1174, _T("Stop base octave chord G#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopA_1, 1175, _T("Stop base octave +1 chord A"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopAS1, 1176, _T("Stop base octave +1 chord A#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopB_1, 1177, _T("Stop base octave +1 chord B"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopC_1, 1178, _T("Stop base octave +1 chord C"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopCS1, 1179, _T("Stop base octave +1 chord C#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopD_1, 1180, _T("Stop base octave +1 chord D"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopDS1, 1181, _T("Stop base octave +1 chord D#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopE_1, 1182, _T("Stop base octave +1 chord E"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopF_1, 1183, _T("Stop base octave +1 chord F"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopFS1, 1184, _T("Stop base octave +1 chord F#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopG_1, 1185, _T("Stop base octave +1 chord G"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopGS1, 1186, _T("Stop base octave +1 chord G#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopA_2, 1187, _T("Stop base octave +2 chord A"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopAS2, 1188, _T("Stop base octave +2 chord A#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopB_2, 1189, _T("Stop base octave +2 chord B"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopC_2, 1190, _T("Stop base octave +2 chord C"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopCS2, 1191, _T("Stop base octave +2 chord C#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopD_2, 1192, _T("Stop base octave +2 chord D"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopDS2, 1193, _T("Stop base octave +2 chord D#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopE_2, 1194, _T("Stop base octave +2 chord E"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopF_2, 1195, _T("Stop base octave +2 chord F"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopFS2, 1196, _T("Stop base octave +2 chord F#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopG_2, 1197, _T("Stop base octave +2 chord G"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopGS2, 1198, _T("Stop base octave +2 chord G#"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcVPChordStopA_3, 1199, _T("Stop base octave +3 chord  A"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcNoteCut, 1200, _T("Note Cut"));
	DefineKeyCommand(kcNoteOff, 1201, _T("Note Off"));
	DefineKeyCommand(kcSetIns0, 1202, _T("Set instrument digit 0"));
	DefineKeyCommand(kcSetIns1, 1203, _T("Set instrument digit 1"));
	DefineKeyCommand(kcSetIns2, 1204, _T("Set instrument digit 2"));
	DefineKeyCommand(kcSetIns3, 1205, _T("Set instrument digit 3"));
	DefineKeyCommand(kcSetIns4, 1206, _T("Set instrument digit 4"));
	DefineKeyCommand(kcSetIns5, 1207, _T("Set instrument digit 5"));
	DefineKeyCommand(kcSetIns6, 1208, _T("Set instrument digit 6"));
	DefineKeyCommand(kcSetIns7, 1209, _T("Set instrument digit 7"));
	DefineKeyCommand(kcSetIns8, 1210, _T("Set instrument digit 8"));
	DefineKeyCommand(kcSetIns9, 1211, _T("Set instrument digit 9"));
	DefineKeyCommand(kcSetOctave0, 1212, _T("Set octave 0"));
	DefineKeyCommand(kcSetOctave1, 1213, _T("Set octave 1"));
	DefineKeyCommand(kcSetOctave2, 1214, _T("Set octave 2"));
	DefineKeyCommand(kcSetOctave3, 1215, _T("Set octave 3"));
	DefineKeyCommand(kcSetOctave4, 1216, _T("Set octave 4"));
	DefineKeyCommand(kcSetOctave5, 1217, _T("Set octave 5"));
	DefineKeyCommand(kcSetOctave6, 1218, _T("Set octave 6"));
	DefineKeyCommand(kcSetOctave7, 1219, _T("Set octave 7"));
	DefineKeyCommand(kcSetOctave8, 1220, _T("Set octave 8"));
	DefineKeyCommand(kcSetOctave9, 1221, _T("Set octave 9"));
	DefineKeyCommand(kcSetVolume0, 1222, _T("Set volume digit 0"));
	DefineKeyCommand(kcSetVolume1, 1223, _T("Set volume digit 1"));
	DefineKeyCommand(kcSetVolume2, 1224, _T("Set volume digit 2"));
	DefineKeyCommand(kcSetVolume3, 1225, _T("Set volume digit 3"));
	DefineKeyCommand(kcSetVolume4, 1226, _T("Set volume digit 4"));
	DefineKeyCommand(kcSetVolume5, 1227, _T("Set volume digit 5"));
	DefineKeyCommand(kcSetVolume6, 1228, _T("Set volume digit 6"));
	DefineKeyCommand(kcSetVolume7, 1229, _T("Set volume digit 7"));
	DefineKeyCommand(kcSetVolume8, 1230, _T("Set volume digit 8"));
	DefineKeyCommand(kcSetVolume9, 1231, _T("Set volume digit 9"));
	DefineKeyCommand(kcSetVolumeVol, 1232, _T("Volume Command - Volume"));
	DefineKeyCommand(kcSetVolumePan, 1233, _T("Volume Command - Panning"));
	DefineKeyCommand(kcSetVolumeVolSlideUp, 1234, _T("Volume Command - Volume Slide Up"));
	DefineKeyCommand(kcSetVolumeVolSlideDown, 1235, _T("Volume Command - Volume Slide Down"));
	DefineKeyCommand(kcSetVolumeFineVolUp, 1236, _T("Volume Command - Fine Volume Slide Up"));
	DefineKeyCommand(kcSetVolumeFineVolDown, 1237, _T("Volume Command - Fine Volume Slide Down"));
	DefineKeyCommand(kcSetVolumeVibratoSpd, 1238, _T("Volume Command - Vibrato Speed"));
	DefineKeyCommand(kcSetVolumeVibrato, 1239, _T("Volume Command - Vibrato Depth"));
	DefineKeyCommand(kcSetVolumeXMPanLeft, 1240, _T("Volume Command - XM Pan Slide Left"));
	DefineKeyCommand(kcSetVolumeXMPanRight, 1241, _T("Volume Command - XM Pan Slide Right"));
	DefineKeyCommand(kcSetVolumePortamento, 1242, _T("Volume Command - Portamento"));
	DefineKeyCommand(kcSetVolumeITPortaUp, 1243, _T("Volume Command - Portamento Up"));
	DefineKeyCommand(kcSetVolumeITPortaDown, 1244, _T("Volume Command - Portamento Down"));
	DefineKeyCommand(kcSetVolumeITUnused, 1245, _T("Volume Command - Unused"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetVolumeITOffset, 1246, _T("Volume Command - Offset"));
	DefineKeyCommand(kcSetFXParam0, 1247, _T("Effect Parameter Digit 0"));
	DefineKeyCommand(kcSetFXParam1, 1248, _T("Effect Parameter Digit 1"));
	DefineKeyCommand(kcSetFXParam2, 1249, _T("Effect Parameter Digit 2"));
	DefineKeyCommand(kcSetFXParam3, 1250, _T("Effect Parameter Digit 3"));
	DefineKeyCommand(kcSetFXParam4, 1251, _T("Effect Parameter Digit 4"));
	DefineKeyCommand(kcSetFXParam5, 1252, _T("Effect Parameter Digit 5"));
	DefineKeyCommand(kcSetFXParam6, 1253, _T("Effect Parameter Digit 6"));
	DefineKeyCommand(kcSetFXParam7, 1254, _T("Effect Parameter Digit 7"));
	DefineKeyCommand(kcSetFXParam8, 1255, _T("Effect Parameter Digit 8"));
	DefineKeyCommand(kcSetFXParam9, 1256, _T("Effect Parameter Digit 9"));
	DefineKeyCommand(kcSetFXParamA, 1257, _T("Effect Parameter Digit A"));
	DefineKeyCommand(kcSetFXParamB, 1258, _T("Effect Parameter Digit B"));
	DefineKeyCommand(kcSetFXParamC, 1259, _T("Effect Parameter Digit C"));
	DefineKeyCommand(kcSetFXParamD, 1260, _T("Effect Parameter Digit D"));
	DefineKeyCommand(kcSetFXParamE, 1261, _T("Effect Parameter Digit E"));
	DefineKeyCommand(kcSetFXParamF, 1262, _T("Effect Parameter Digit F"));
	DefineKeyCommand(kcSetFXarp, 1263, _T("FX arpeggio"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXportUp, 1264, _T("FX portamentao Up"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXportDown, 1265, _T("FX portamentao Down"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXport, 1266, _T("FX portamentao"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXvibrato, 1267, _T("FX vibrato"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXportSlide, 1268, _T("FX portamento slide"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXvibSlide, 1269, _T("FX vibrato slide"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXtremolo, 1270, _T("FX tremolo"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXpan, 1271, _T("FX pan"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXoffset, 1272, _T("FX offset"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXvolSlide, 1273, _T("FX Volume slide"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXgotoOrd, 1274, _T("FX go to order"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXsetVol, 1275, _T("FX set volume"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXgotoRow, 1276, _T("FX go to row"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXretrig, 1277, _T("FX retrigger"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXspeed, 1278, _T("FX set speed"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXtempo, 1279, _T("FX set tempo"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXtremor, 1280, _T("FX tremor"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXextendedMOD, 1281, _T("FX extended MOD cmds"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXextendedS3M, 1282, _T("FX extended S3M cmds"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXchannelVol, 1283, _T("FX set channel vol"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXchannelVols, 1284, _T("FX channel vol slide"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXglobalVol, 1285, _T("FX set global volume"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXglobalVols, 1286, _T("FX global volume slide"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXkeyoff, 1287, _T("FX Some XM Command :D"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXfineVib, 1288, _T("FX fine vibrato"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXpanbrello, 1289, _T("FX set panbrello"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXextendedXM, 1290, _T("FX extended XM effects "), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXpanSlide, 1291, _T("FX pan slide"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXsetEnvPos, 1292, _T("FX set envelope position (XM only)"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXmacro, 1293, _T("FX MIDI Macro"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetFXmacroSlide, 1294, _T("Smooth MIDI Macro Slide"));
	DefineKeyCommand(kcSetFXdelaycut, 1295, _T("Combined Note Delay and Note Cut"));
	DefineKeyCommand(kcPatternJumpDownh1Select, 1296, _T("kcPatternJumpDownh1Select"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcPatternJumpUph1Select, 1297, _T("kcPatternJumpUph1Select"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcPatternSnapDownh1Select, 1298, _T("kcPatternSnapDownh1Select"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcPatternSnapUph1Select, 1299, _T("kcPatternSnapUph1Select"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcNavigateDownSelect, 1300, _T("kcNavigateDownSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcNavigateUpSelect, 1301, _T("kcNavigateUpSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcNavigateLeftSelect, 1302, _T("kcNavigateLeftSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcNavigateRightSelect, 1303, _T("kcNavigateRightSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcNavigateNextChanSelect, 1304, _T("kcNavigateNextChanSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcNavigatePrevChanSelect, 1305, _T("kcNavigatePrevChanSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcHomeHorizontalSelect, 1306, _T("kcHomeHorizontalSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcHomeVerticalSelect, 1307, _T("kcHomeVerticalSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcHomeAbsoluteSelect, 1308, _T("kcHomeAbsoluteSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcEndHorizontalSelect, 1309, _T("kcEndHorizontalSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcEndVerticalSelect, 1310, _T("kcEndVerticalSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcEndAbsoluteSelect, 1311, _T("kcEndAbsoluteSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSelectWithNav, 1312, _T("kcSelectWithNav"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSelectOffWithNav, 1313, _T("kcSelectOffWithNav"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcCopySelectWithNav, 1314, _T("kcCopySelectWithNav"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcCopySelectOffWithNav, 1315, _T("kcCopySelectOffWithNav"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcChordModifier, 1316, _T("Chord Modifier"), kcVisible, kcDummy);
	DefineKeyCommand(kcSetSpacing, 1317, _T("Set edit step on note entry"), kcVisible, kcDummy);
	DefineKeyCommand(kcSetSpacing0, 1318, _T(""), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetSpacing1, 1319, _T(""), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetSpacing2, 1320, _T(""), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetSpacing3, 1321, _T(""), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetSpacing4, 1322, _T(""), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetSpacing5, 1323, _T(""), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetSpacing6, 1324, _T(""), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetSpacing7, 1325, _T(""), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetSpacing8, 1326, _T(""), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSetSpacing9, 1327, _T(""), kcHidden, kcNoDummy);
	DefineKeyCommand(kcCopySelectWithSelect, 1328, _T("kcCopySelectWithSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcCopySelectOffWithSelect, 1329, _T("kcCopySelectOffWithSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSelectWithCopySelect, 1330, _T("kcSelectWithCopySelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcSelectOffWithCopySelect, 1331, _T("kcSelectOffWithCopySelect"), kcHidden, kcNoDummy);
	/*
	DefineKeyCommand(kcCopy, 1332, _T("Copy pattern data"));
	DefineKeyCommand(kcCut, 1333, _T("Cut pattern data"));
	DefineKeyCommand(kcPaste, 1334, _T("Paste pattern data"));
	DefineKeyCommand(kcMixPaste, 1335, _T("Mix-paste pattern data"));
	DefineKeyCommand(kcSelectAll, 1336, _T("Select all pattern data"));
	DefineKeyCommand(kcSelectCol, 1337, _T("Select channel / select all"), kcHidden, kcNoDummy);
	*/
	DefineKeyCommand(kcPatternJumpDownh2, 1338, _T("Jump down by beat"));
	DefineKeyCommand(kcPatternJumpUph2, 1339, _T("Jump up by beat"));
	DefineKeyCommand(kcPatternSnapDownh2, 1340, _T("Snap down to beat"));
	DefineKeyCommand(kcPatternSnapUph2, 1341, _T("Snap up to beat"));
	DefineKeyCommand(kcPatternJumpDownh2Select, 1342, _T("kcPatternJumpDownh2Select"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcPatternJumpUph2Select, 1343, _T("kcPatternJumpUph2Select"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcPatternSnapDownh2Select, 1344, _T("kcPatternSnapDownh2Select"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcPatternSnapUph2Select, 1345, _T("kcPatternSnapUph2Select"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcFileOpen, 1346, _T("File/Open"));
	DefineKeyCommand(kcFileNew, 1347, _T("File/New"));
	DefineKeyCommand(kcFileClose, 1348, _T("File/Close"));
	DefineKeyCommand(kcFileSave, 1349, _T("File/Save"));
	DefineKeyCommand(kcFileSaveAs, 1350, _T("File/Save As"));
	DefineKeyCommand(kcFileSaveAsWave, 1351, _T("File/Export as lossless (Wave, FLAC)"));
	DefineKeyCommand(kcFileSaveAsMP3, 1352, _T("File/Export as lossy (Opus, Vorbis, MP3"));
	DefineKeyCommand(kcFileSaveMidi, 1353, _T("File/Export as MIDI"));
	DefineKeyCommand(kcFileImportMidiLib, 1354, _T("File/Import MIDI Library"));
	DefineKeyCommand(kcFileAddSoundBank, 1355, _T("File/Add Sound Bank"));
	DefineKeyCommand(kcEditUndo, 1359, _T("Undo"));
	DefineKeyCommand(kcEditCut, 1360, _T("Cut"));
	DefineKeyCommand(kcEditCopy, 1361, _T("Copy"));
	DefineKeyCommand(kcEditPaste, 1362, _T("Paste"));
	DefineKeyCommand(kcEditMixPaste, 1363, _T("Mix Paste"));
	DefineKeyCommand(kcEditSelectAll, 1364, _T("Select All"));
	DefineKeyCommand(kcEditFind, 1365, _T("Find / Replace"));
	DefineKeyCommand(kcEditFindNext, 1366, _T("Find Next"));
	DefineKeyCommand(kcViewMain, 1367, _T("Toggle Main Toolbar"));
	DefineKeyCommand(kcViewTree, 1368, _T("Toggle Tree View"));
	DefineKeyCommand(kcViewOptions, 1369, _T("View Options"));
	DefineKeyCommand(kcHelp, 1370, _T("Help"));
	/*
	DefineKeyCommand(kcWindowNew, 1370, _T("New Window"));
	DefineKeyCommand(kcWindowCascade, 1371, _T("Cascade Windows"));
	DefineKeyCommand(kcWindowTileHorz, 1372, _T("Tile Windows Horizontally"));
	DefineKeyCommand(kcWindowTileVert, 1373, _T("Tile Windows Vertically"));
	*/
	DefineKeyCommand(kcEstimateSongLength, 1374, _T("Estimate Song Length"));
	DefineKeyCommand(kcStopSong, 1375, _T("Stop Song"));
	DefineKeyCommand(kcMidiRecord, 1376, _T("Toggle MIDI Record"));
	DefineKeyCommand(kcDeleteAllRows, 1377, _T("Delete all rows"));
	DefineKeyCommand(kcInsertRow, 1378, _T("Insert Row"));
	DefineKeyCommand(kcInsertAllRows, 1379, _T("Insert All Rows"));
	DefineKeyCommand(kcSampleTrim, 1380, _T("Trim sample around loop points"));
	DefineKeyCommand(kcSampleReverse, 1381, _T("Reverse Sample"));
	DefineKeyCommand(kcSampleDelete, 1382, _T("Delete Sample Selection"));
	DefineKeyCommand(kcSampleSilence, 1383, _T("Silence Sample Selection"));
	DefineKeyCommand(kcSampleNormalize, 1384, _T("Normalise Sample"));
	DefineKeyCommand(kcSampleAmplify, 1385, _T("Amplify Sample"));
	DefineKeyCommand(kcSampleZoomUp, 1386, _T("Zoom Out"));
	DefineKeyCommand(kcSampleZoomDown, 1387, _T("Zoom In"));
	//time saving HACK:
	for(int j = kcSampStartNotes; j <= kcInsNoteMapEndNoteStops; j++)
	{
		DefineKeyCommand((CommandID)j, 1388 + j - kcSampStartNotes, _T("Auto Note in some context"), kcHidden, kcNoDummy);
	}
	//end hack
	DefineKeyCommand(kcPatternGrowSelection, 1660, _T("Grow selection"));
	DefineKeyCommand(kcPatternShrinkSelection, 1661, _T("Shrink selection"));
	DefineKeyCommand(kcTogglePluginEditor, 1662, _T("Toggle channel's plugin editor"));
	DefineKeyCommand(kcToggleFollowSong, 1663, _T("Toggle follow song"));
	DefineKeyCommand(kcClearFieldITStyle, 1664, _T("Clear field (IT Style)"));
	DefineKeyCommand(kcClearFieldStepITStyle, 1665, _T("Clear field and step (IT Style)"));
	DefineKeyCommand(kcSetFXextension, 1666, _T("Parameter Extension Command"));
	DefineKeyCommand(kcNoteCutOld, 1667, _T("Note Cut"), kcHidden);	// Legacy
	DefineKeyCommand(kcNoteOffOld, 1668, _T("Note Off"), kcHidden);	// Legacy
	DefineKeyCommand(kcViewAddPlugin, 1669, _T("View Plugin Manager"));
	DefineKeyCommand(kcViewChannelManager, 1670, _T("View Channel Manager"));
	DefineKeyCommand(kcCopyAndLoseSelection, 1671, _T("Copy and lose selection"));
	DefineKeyCommand(kcNewPattern, 1672, _T("Insert new pattern"));
	DefineKeyCommand(kcSampleLoad, 1673, _T("Load Sample"));
	DefineKeyCommand(kcSampleSave, 1674, _T("Save Sample"));
	DefineKeyCommand(kcSampleNew, 1675, _T("New Sample"));
	//DefineKeyCommand(kcSampleCtrlLoad, 1676, _T("Load Sample"), kcHidden);
	//DefineKeyCommand(kcSampleCtrlSave, 1677, _T("Save Sample"), kcHidden);
	//DefineKeyCommand(kcSampleCtrlNew, 1678, _T("New Sample"), kcHidden);
	DefineKeyCommand(kcInstrumentLoad, 1679, _T("Load Instrument"), kcHidden);
	DefineKeyCommand(kcInstrumentSave, 1680, _T("Save Instrument"), kcHidden);
	DefineKeyCommand(kcInstrumentNew, 1681, _T("New Instrument"), kcHidden);
	DefineKeyCommand(kcInstrumentCtrlLoad, 1682, _T("Load Instrument"), kcHidden);
	DefineKeyCommand(kcInstrumentCtrlSave, 1683, _T("Save Instrument"), kcHidden);
	DefineKeyCommand(kcInstrumentCtrlNew, 1684, _T("New Instrument"), kcHidden);
	DefineKeyCommand(kcSwitchToOrderList, 1685, _T("Switch to Order List"));
	DefineKeyCommand(kcEditMixPasteITStyle, 1686, _T("Mix Paste (old IT Style)"));
	DefineKeyCommand(kcApproxRealBPM, 1687, _T("Show approx. real BPM"));
	DefineKeyCommand(kcNavigateDownBySpacingSelect, 1689, _T("kcNavigateDownBySpacingSelect"), kcHidden);
	DefineKeyCommand(kcNavigateUpBySpacingSelect, 1690, _T("kcNavigateUpBySpacingSelect"), kcHidden);
	DefineKeyCommand(kcNavigateDownBySpacing, 1691, _T("Navigate down by spacing"));
	DefineKeyCommand(kcNavigateUpBySpacing, 1692, _T("Navigate up by spacing"));
	DefineKeyCommand(kcPrevDocument, 1693, _T("Previous Document"));
	DefineKeyCommand(kcNextDocument, 1694, _T("Next Document"));
	//time saving HACK:
	for(int j = kcVSTGUIStartNotes; j <= kcVSTGUIEndNoteStops; j++)
	{
		DefineKeyCommand((CommandID)j, 1695 + j - kcVSTGUIStartNotes, _T("Auto Note in some context"), kcHidden, kcNoDummy);
	}
	//end hack
	DefineKeyCommand(kcVSTGUIPrevPreset, 1763, _T("Previous Plugin Preset"));
	DefineKeyCommand(kcVSTGUINextPreset, 1764, _T("Next Plugin Preset"));
	DefineKeyCommand(kcVSTGUIRandParams, 1765, _T("Randomize Plugin Parameters"));
	DefineKeyCommand(kcPatternGoto, 1766, _T("Go to row/channel/..."));
	DefineKeyCommand(kcPatternOpenRandomizer, 1767, _T("Pattern Randomizer"), kcHidden, kcNoDummy); // while there's not randomizer yet, let's just disable it for now
	DefineKeyCommand(kcPatternInterpolateNote, 1768, _T("Interpolate Note"));
	//rewbs.graph
	DefineKeyCommand(kcViewGraph, 1769, _T("View Graph"), kcHidden, kcNoDummy);  // while there's no graph yet, let's just disable it for now
	//end rewbs.graph
	DefineKeyCommand(kcToggleChanMuteOnPatTransition, 1770, _T("(Un)mute channel on pattern transition"));
	DefineKeyCommand(kcChannelUnmuteAll, 1771, _T("Unmute all channels"));
	DefineKeyCommand(kcShowPatternProperties, 1772, _T("Show Pattern Properties"));
	DefineKeyCommand(kcShowMacroConfig, 1773, _T("View Zxx Macro Configuration"));
	DefineKeyCommand(kcViewSongProperties, 1775, _T("View Song Properties"));
	DefineKeyCommand(kcChangeLoopStatus, 1776, _T("Toggle Loop Pattern"));
	DefineKeyCommand(kcFileExportCompat, 1777, _T("File/Compatibility Export"));
	DefineKeyCommand(kcUnmuteAllChnOnPatTransition, 1778, _T("Unmute all channels on pattern transition"));
	DefineKeyCommand(kcSoloChnOnPatTransition, 1779, _T("Solo channel on pattern transition"));
	DefineKeyCommand(kcTimeAtRow, 1780, _T("Show playback time at current row"));
	DefineKeyCommand(kcViewMIDImapping, 1781, _T("View MIDI Mapping"));
	DefineKeyCommand(kcVSTGUIPrevPresetJump, 1782, _T("Plugin preset backward jump"));
	DefineKeyCommand(kcVSTGUINextPresetJump, 1783, _T("Plugin preset forward jump"));
	DefineKeyCommand(kcSampleInvert, 1784, _T("Invert Sample Phase"));
	DefineKeyCommand(kcSampleSignUnsign, 1785, _T("Signed / Unsigned Conversion"));
	DefineKeyCommand(kcChannelReset, 1786, _T("Reset Channel"));
	DefineKeyCommand(kcToggleOverflowPaste, 1787, _T("Toggle overflow paste"));
	DefineKeyCommand(kcNotePC, 1788, _T("Parameter Control"));
	DefineKeyCommand(kcNotePCS, 1789, _T("Parameter Control (smooth)"));
	DefineKeyCommand(kcSampleRemoveDCOffset, 1790, _T("Remove DC Offset"));
	DefineKeyCommand(kcNoteFade, 1791, _T("Note Fade"));
	DefineKeyCommand(kcNoteFadeOld, 1792, _T("Note Fade"), kcHidden);	// Legacy
	DefineKeyCommand(kcEditPasteFlood, 1793, _T("Paste Flood"));
	DefineKeyCommand(kcOrderlistNavigateLeft, 1794, _T("Previous Order"));
	DefineKeyCommand(kcOrderlistNavigateRight, 1795, _T("Next Order"));
	DefineKeyCommand(kcOrderlistNavigateFirst, 1796, _T("First Order"));
	DefineKeyCommand(kcOrderlistNavigateLast, 1797, _T("Last Order"));
	DefineKeyCommand(kcOrderlistNavigateLeftSelect, 1798, _T("kcOrderlistNavigateLeftSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcOrderlistNavigateRightSelect, 1799, _T("kcOrderlistNavigateRightSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcOrderlistNavigateFirstSelect, 1800, _T("kcOrderlistNavigateFirstSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcOrderlistNavigateLastSelect, 1801, _T("kcOrderlistNavigateLastSelect"), kcHidden, kcNoDummy);
	DefineKeyCommand(kcOrderlistEditDelete, 1802, _T("Delete Order"));
	DefineKeyCommand(kcOrderlistEditInsert, 1803, _T("Insert Order"));
	DefineKeyCommand(kcOrderlistEditPattern, 1804, _T("Edit Pattern"));
	DefineKeyCommand(kcOrderlistSwitchToPatternView, 1805, _T("Switch to pattern editor"));
	DefineKeyCommand(kcDuplicatePattern, 1806, _T("Duplicate Pattern"));
	DefineKeyCommand(kcOrderlistPat0, 1807, _T("Pattern index digit 0"));
	DefineKeyCommand(kcOrderlistPat1, 1808, _T("Pattern index digit 1"));
	DefineKeyCommand(kcOrderlistPat2, 1809, _T("Pattern index digit 2"));
	DefineKeyCommand(kcOrderlistPat3, 1810, _T("Pattern index digit 3"));
	DefineKeyCommand(kcOrderlistPat4, 1811, _T("Pattern index digit 4"));
	DefineKeyCommand(kcOrderlistPat5, 1812, _T("Pattern index digit 5"));
	DefineKeyCommand(kcOrderlistPat6, 1813, _T("Pattern index digit 6"));
	DefineKeyCommand(kcOrderlistPat7, 1814, _T("Pattern index digit 7"));
	DefineKeyCommand(kcOrderlistPat8, 1815, _T("Pattern index digit 8"));
	DefineKeyCommand(kcOrderlistPat9, 1816, _T("Pattern index digit 9"));
	DefineKeyCommand(kcOrderlistPatPlus, 1817, _T("Increase pattern index "));
	DefineKeyCommand(kcOrderlistPatMinus, 1818, _T("Decrease pattern index"));
	DefineKeyCommand(kcShowSplitKeyboardSettings, 1819, _T("Split Keyboard Settings dialog"));
	DefineKeyCommand(kcEditPushForwardPaste, 1820, _T("Push Forward Paste (Insert)"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveLeft, 1821, _T("Move envelope point left"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveRight, 1822, _T("Move envelope point right"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveUp, 1823, _T("Move envelope point up"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveDown, 1824, _T("Move envelope point down"));
	DefineKeyCommand(kcInstrumentEnvelopePointPrev, 1825, _T("Select previous envelope point"));
	DefineKeyCommand(kcInstrumentEnvelopePointNext, 1826, _T("Select next envelope point"));
	DefineKeyCommand(kcInstrumentEnvelopePointInsert, 1827, _T("Insert Envelope Point"));
	DefineKeyCommand(kcInstrumentEnvelopePointRemove, 1828, _T("Remove Envelope Point"));
	DefineKeyCommand(kcInstrumentEnvelopeSetLoopStart, 1829, _T("Set Loop Start"));
	DefineKeyCommand(kcInstrumentEnvelopeSetLoopEnd, 1830, _T("Set Loop End"));
	DefineKeyCommand(kcInstrumentEnvelopeSetSustainLoopStart, 1831, _T("Set sustain loop start"));
	DefineKeyCommand(kcInstrumentEnvelopeSetSustainLoopEnd, 1832, _T("Set sustain loop end"));
	DefineKeyCommand(kcInstrumentEnvelopeToggleReleaseNode, 1833, _T("Toggle release node"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveUp8, 1834, _T("Move envelope point up (Coarse)"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveDown8, 1835, _T("Move envelope point down (Coarse)"));
	DefineKeyCommand(kcPatternEditPCNotePlugin, 1836, _T("Toggle PC Event/instrument plugin editor"));
	DefineKeyCommand(kcInstrumentEnvelopeZoomIn, 1837, _T("Zoom In"));
	DefineKeyCommand(kcInstrumentEnvelopeZoomOut, 1838, _T("Zoom Out"));
	DefineKeyCommand(kcVSTGUIToggleRecordParams, 1839, _T("Toggle Parameter Recording"));
	DefineKeyCommand(kcVSTGUIToggleSendKeysToPlug, 1840, _T("Pass Key Presses to Plugin"));
	DefineKeyCommand(kcVSTGUIBypassPlug, 1841, _T("Bypass Plugin"));
	DefineKeyCommand(kcInsNoteMapTransposeDown, 1842, _T("Transpose -1 (Note Map)"));
	DefineKeyCommand(kcInsNoteMapTransposeUp, 1843, _T("Transpose +1 (Note Map)"));
	DefineKeyCommand(kcInsNoteMapTransposeOctDown, 1844, _T("Transpose -1 Octave (Note Map)"));
	DefineKeyCommand(kcInsNoteMapTransposeOctUp, 1845, _T("Transpose +1 Octave (Note Map)"));
	DefineKeyCommand(kcInsNoteMapCopyCurrentNote, 1846, _T("Map all notes to selected note"));
	DefineKeyCommand(kcInsNoteMapCopyCurrentSample, 1847, _T("Map all notes to selected sample"));
	DefineKeyCommand(kcInsNoteMapReset, 1848, _T("Reset Note Mapping"));
	DefineKeyCommand(kcInsNoteMapEditSample, 1849, _T("Edit Current Sample"));
	DefineKeyCommand(kcInsNoteMapEditSampleMap, 1850, _T("Edit Sample Map"));
	DefineKeyCommand(kcInstrumentCtrlDuplicate, 1851, _T("Duplicate Instrument"));
	DefineKeyCommand(kcPanic, 1852, _T("Panic"));
	DefineKeyCommand(kcOrderlistPatIgnore, 1853, _T("Separator (+++) Index"));
	DefineKeyCommand(kcOrderlistPatInvalid, 1854, _T("Stop (---) Index"));
	DefineKeyCommand(kcViewEditHistory, 1855, _T("View Edit History"));
	DefineKeyCommand(kcSampleQuickFade, 1856, _T("Quick Fade"));
	DefineKeyCommand(kcSampleXFade, 1857, _T("Crossfade Sample Loop"));
	DefineKeyCommand(kcSelectBeat, 1858, _T("Select Beat"));
	DefineKeyCommand(kcSelectMeasure, 1859, _T("Select Measure"));
	DefineKeyCommand(kcFileSaveTemplate, 1860, _T("File/Save As Template"));
	DefineKeyCommand(kcIncreaseSpacing, 1861, _T("Increase Edit Step"));
	DefineKeyCommand(kcDecreaseSpacing, 1862, _T("Decrease Edit Step"));
	DefineKeyCommand(kcSampleAutotune, 1863, _T("Tune Sample to given Note"));
	DefineKeyCommand(kcFileCloseAll, 1864, _T("File/Close All"));
	DefineKeyCommand(kcSetOctaveStop0, 1865, _T(""), kcHidden);
	DefineKeyCommand(kcSetOctaveStop1, 1866, _T(""), kcHidden);
	DefineKeyCommand(kcSetOctaveStop2, 1867, _T(""), kcHidden);
	DefineKeyCommand(kcSetOctaveStop3, 1868, _T(""), kcHidden);
	DefineKeyCommand(kcSetOctaveStop4, 1869, _T(""), kcHidden);
	DefineKeyCommand(kcSetOctaveStop5, 1870, _T(""), kcHidden);
	DefineKeyCommand(kcSetOctaveStop6, 1871, _T(""), kcHidden);
	DefineKeyCommand(kcSetOctaveStop7, 1872, _T(""), kcHidden);
	DefineKeyCommand(kcSetOctaveStop8, 1873, _T(""), kcHidden);
	DefineKeyCommand(kcSetOctaveStop9, 1874, _T(""), kcHidden);
	DefineKeyCommand(kcOrderlistLockPlayback, 1875, _T("Lock Playback to Selection"));
	DefineKeyCommand(kcOrderlistUnlockPlayback, 1876, _T("Unlock Playback"));
	DefineKeyCommand(kcChannelSettings, 1877, _T("Quick Channel Settings"));
	DefineKeyCommand(kcChnSettingsPrev, 1878, _T("Previous Channel"));
	DefineKeyCommand(kcChnSettingsNext, 1879, _T("Next Channel"));
	DefineKeyCommand(kcChnSettingsClose, 1880, _T("Switch to Pattern Editor"));
	DefineKeyCommand(kcTransposeCustom, 1881, _T("Transpose Custom"));
	DefineKeyCommand(kcSampleZoomSelection, 1882, _T("Zoom into Selection"));
	DefineKeyCommand(kcChannelRecordSelect, 1883, _T("Channel Record Select"));
	DefineKeyCommand(kcChannelSplitRecordSelect, 1884, _T("Channel Split Record Select"));
	DefineKeyCommand(kcDataEntryUp, 1885, _T("Data Entry +1"));
	DefineKeyCommand(kcDataEntryDown, 1886, _T("Data Entry -1"));
	DefineKeyCommand(kcSample8Bit, 1887, _T("Convert to 8-bit / 16-bit"));
	DefineKeyCommand(kcSampleMonoMix, 1888, _T("Convert to Mono (Mix)"));
	DefineKeyCommand(kcSampleMonoLeft, 1889, _T("Convert to Mono (Left Channel)"));
	DefineKeyCommand(kcSampleMonoRight, 1890, _T("Convert to Mono (Right Channel)"));
	DefineKeyCommand(kcSampleMonoSplit, 1891, _T("Convert to Mono (Split Sample)"));
	DefineKeyCommand(kcQuantizeSettings, 1892, _T("Quantize Settings"));
	DefineKeyCommand(kcDataEntryUpCoarse, 1893, _T("Data Entry Up (Coarse)"));
	DefineKeyCommand(kcDataEntryDownCoarse, 1894, _T("Data Entry Down (Coarse)"));
	DefineKeyCommand(kcToggleNoteOffRecordPC, 1895, _T("Toggle Note Off record (PC keyboard)"));
	DefineKeyCommand(kcToggleNoteOffRecordMIDI, 1896, _T("Toggle Note Off record (MIDI)"));
	DefineKeyCommand(kcFindInstrument, 1897, _T("Pick up nearest instrument number"));
	DefineKeyCommand(kcPlaySongFromPattern, 1898, _T("Play Song from Pattern Start"));
	DefineKeyCommand(kcVSTGUIToggleRecordMIDIOut, 1899, _T("Record MIDI Out to Pattern Editor"));
	DefineKeyCommand(kcToggleClipboardManager, 1900, _T("Toggle Clipboard Manager"));
	DefineKeyCommand(kcClipboardPrev, 1901, _T("Cycle to Previous Clipboard"));
	DefineKeyCommand(kcClipboardNext, 1902, _T("Cycle to Next Clipboard"));
	DefineKeyCommand(kcSelectRow, 1903, _T("Select Row"));
	DefineKeyCommand(kcSelectEvent, 1904, _T("Select Event"));
	DefineKeyCommand(kcEditRedo, 1905, _T("Redo"));
	DefineKeyCommand(kcFileAppend, 1906, _T("File/Append Module"));
	DefineKeyCommand(kcSampleTransposeUp, 1907, _T("Transpose +1"));
	DefineKeyCommand(kcSampleTransposeDown, 1908, _T("Transpose -1"));
	DefineKeyCommand(kcSampleTransposeOctUp, 1909, _T("Transpose +1 Octave"));
	DefineKeyCommand(kcSampleTransposeOctDown, 1910, _T("Transpose -1 Octave"));
	DefineKeyCommand(kcPatternInterpolateInstr, 1911, _T("Interpolate Instrument"));
	DefineKeyCommand(kcDummyShortcut, 1912, _T("Dummy Shortcut"));
	DefineKeyCommand(kcSampleUpsample, 1913, _T("Upsample"));
	DefineKeyCommand(kcSampleDownsample, 1914, _T("Downsample"));
	DefineKeyCommand(kcSampleResample, 1915, _T("Resample"));
	DefineKeyCommand(kcSampleCenterLoopStart, 1916, _T("Center loop start in view"));
	DefineKeyCommand(kcSampleCenterLoopEnd, 1917, _T("Center loop end in view"));
	DefineKeyCommand(kcSampleCenterSustainStart, 1918, _T("Center sustain loop start in view"));
	DefineKeyCommand(kcSampleCenterSustainEnd, 1919, _T("Center sustain loop end in view"));
	DefineKeyCommand(kcInstrumentEnvelopeLoad, 1920, _T("Load Envelope"));
	DefineKeyCommand(kcInstrumentEnvelopeSave, 1921, _T("Save Envelope"));
	DefineKeyCommand(kcChannelTranspose, 1922, _T("Transpose Channel"));
	DefineKeyCommand(kcChannelDuplicate, 1923, _T("Duplicate Channel"));
	for(int j = kcStartSampleCues; j <= kcEndSampleCues; j++)
	{
		TCHAR s[32];
		wsprintf(s, _T("Preview Sample Cue %u"), j - kcStartSampleCues + 1);
		DefineKeyCommand((CommandID)j, 1924 + j - kcStartSampleCues, s);
	}
	// Safety margin if we want to add more cues
	DefineKeyCommand(kcOrderlistEditCopyOrders, 1950, _T("Copy Orders"));
	DefineKeyCommand(kcTreeViewStopPreview, 1951, _T("Stop sample preview"), kcHidden);
	DefineKeyCommand(kcSampleDuplicate, 1952, _T("Duplicate Sample"));
	DefineKeyCommand(kcSampleSlice, 1953, _T("Slice at cue points"));
	DefineKeyCommand(kcInstrumentEnvelopeScale, 1954, _T("Scale Envelope Points"));
	DefineKeyCommand(kcInsNoteMapRemove, 1955, _T("Remove All Samples"));
	DefineKeyCommand(kcInstrumentEnvelopeSelectLoopStart, 1956, _T("Select Envelope Loop Start"));
	DefineKeyCommand(kcInstrumentEnvelopeSelectLoopEnd, 1957, _T("Select Envelope Loop End"));
	DefineKeyCommand(kcInstrumentEnvelopeSelectSustainStart, 1958, _T("Select Envelope Sustain Start"));
	DefineKeyCommand(kcInstrumentEnvelopeSelectSustainEnd, 1959, _T("Select Envelope Sustain End"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveLeftCoarse, 1960, _T("Move envelope point left (Coarse)"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveRightCoarse, 1961, _T("Move envelope point right (Coarse)"));
	DefineKeyCommand(kcSampleCenterSampleStart, 1962, _T("Zoom into sample start"));
	DefineKeyCommand(kcSampleCenterSampleEnd, 1963, _T("Zoom into sample end"));
	DefineKeyCommand(kcSampleTrimToLoopEnd, 1964, _T("Trim to loop end"));
	DefineKeyCommand(kcLockPlaybackToRows, 1965, _T("Lock Playback to Rows"));
	DefineKeyCommand(kcSwitchToInstrLibrary, 1966, _T("Switch To Instrument Library"));
	DefineKeyCommand(kcPatternSetInstrumentNotEmpty, 1967, _T("Apply current instrument to existing only"));
	DefineKeyCommand(kcSelectColumn, 1968, _T("Select Column"));
	DefineKeyCommand(kcSampleStereoSep, 1969, _T("Change Stereo Separation"));

	// Add new key commands here.

#ifdef _DEBUG
	for(size_t i = 0; i < kcNumCommands; i++)
	{
		if(commands[i].UID != 0)	// ignore unset UIDs
		{
			for(size_t j = i + 1; j < kcNumCommands; j++)
			{
				if(commands[i].UID == commands[j].UID)
				{
					Log("Duplicate command UID: %d\n", commands[i].UID);
					ASSERT(false);
				}
			}
		}
	}
#endif //_DEBUG

}


//-------------------------------------------------------
// Command Manipulation
//-------------------------------------------------------


CString CCommandSet::Add(KeyCombination kc, CommandID cmd, bool overwrite, int pos, bool checkEventConflict)
//----------------------------------------------------------------------------------------------------------
{
	CString report= "";

	//Avoid duplicate
	for(size_t k = 0; k < commands[cmd].kcList.size(); k++)
	{
		KeyCombination curKc=commands[cmd].kcList[k];
		if (curKc==kc)
		{
			//cm'ed out for perf
			//Log("Not adding key:%d; ctx:%d; mod:%d event %d - Duplicate!\n", kc.code, kc.ctx, kc.mod, kc.event);
			return "";
		}
	}

	//Check that this keycombination isn't already assigned (in this context), except for dummy keys
	std::pair<CommandID, KeyCombination> conflictCmd;
	if((conflictCmd = IsConflicting(kc, cmd, checkEventConflict)).first != kcNull)
	{
		if (!overwrite)
		{
			//cm'ed out for perf
			//Log("Not adding key: already exists and overwrite disabled\n");
			return "";
		} else
		{
			if (IsCrossContextConflict(kc, conflictCmd.second))
			{
				report += "The following commands may conflict:\r\n   >" + GetCommandText(conflictCmd.first) + " in " + conflictCmd.second.GetContextText() + "\r\n   >" + GetCommandText(cmd) + " in " + kc.GetContextText() + "\r\n\r\n";
				Log("%s", report);
			} else
			{
				//if(!TrackerSettings::Instance().MiscAllowMultipleCommandsPerKey)
				//	Remove(conflictCmd.second, conflictCmd.first);
				report += "The following commands in same context share the same key combination:\r\n   >" + GetCommandText(conflictCmd.first) + " in " + conflictCmd.second.GetContextText() + "\r\n\r\n";
				Log("%s", report);
			}
		}
	}

	if (pos < 0)
		pos = commands[cmd].kcList.size();
	commands[cmd].kcList.insert(commands[cmd].kcList.begin() + pos, kc);

	//enfore rules on CommandSet
	report+=EnforceAll(kc, cmd, true);
	return report;
}


std::pair<CommandID, KeyCombination> CCommandSet::IsConflicting(KeyCombination kc, CommandID cmd, bool checkEventConflict) const
//------------------------------------------------------------------------------------------------------------------------------
{
	if(IsDummyCommand(cmd))    // no need to search if we are adding a dummy key
		return std::pair<CommandID, KeyCombination>(kcNull, KeyCombination());
	
	for(int pass = 0; pass < 2; pass++)
	{
		// In the first pass, only look for conflicts in the same context, since
		// such conflicts are errors. Cross-context conflicts only emit warnings.
		for(int curCmd = 0; curCmd < kcNumCommands; curCmd++)
		{
			if(IsDummyCommand((CommandID)curCmd))
				continue;

			for(size_t k = 0; k < commands[curCmd].kcList.size(); k++)
			{
				const KeyCombination &curKc = commands[curCmd].kcList[k];
				if(pass == 0 && curKc.Context() != kc.Context())
					continue;

				if(KeyCombinationConflict(curKc, kc, checkEventConflict))
				{
					return std::pair<CommandID, KeyCombination>((CommandID)curCmd, curKc);
				}
			}
		}
	}

	return std::pair<CommandID, KeyCombination>(kcNull, KeyCombination());
}


bool CCommandSet::IsDummyCommand(CommandID cmd) const
//---------------------------------------------------
{
	// e.g. Chord modifier is a dummy command, which serves only to automatically
	// generate a set of keycombinations for chords (I'm not proud of this design).
	return commands[cmd].isDummy;
}


CString CCommandSet::Remove(int pos, CommandID cmd)
//-------------------------------------------------
{
	if (pos>=0 && (size_t)pos<commands[cmd].kcList.size())
	{
		return Remove(commands[cmd].kcList[pos], cmd);
	}

	Log("Failed to remove a key: keychoice out of range.\n");
	return "";
}


CString CCommandSet::Remove(KeyCombination kc, CommandID cmd)
//-----------------------------------------------------------
{

	//find kc in commands[cmd].kcList
	std::vector<KeyCombination>::const_iterator index;
	for(index = commands[cmd].kcList.begin(); index != commands[cmd].kcList.end(); index++)
	{
		if (kc == *index)
			break;
	}
	if (index != commands[cmd].kcList.end())
	{
		commands[cmd].kcList.erase(index);
		Log("Removed a key\n");
		return  EnforceAll(kc, cmd, false);
	}
	else
	{
		Log("Failed to remove a key as it was not found\n");
		return "";
	}

}


CString CCommandSet::EnforceAll(KeyCombination inKc, CommandID inCmd, bool adding)
//--------------------------------------------------------------------------------
{
	//World's biggest, most confusing method. :)
	//Needs refactoring. Maybe make lots of Rule subclasses, each with their own Enforce() method?
	KeyCombination curKc;	// for looping through key combinations
	KeyCombination newKc;	// for adding new key combinations
	CString report;

	if(enforceRule[krAllowNavigationWithSelection])
	{
		// When we get a new navigation command key, we need to
		// make sure this navigation will work when any selection key is pressed
		if(inCmd >= kcStartPatNavigation && inCmd <= kcEndPatNavigation)
		{//Check that it is a nav cmd
			CommandID cmdNavSelection = (CommandID)(kcStartPatNavigationSelect + (inCmd-kcStartPatNavigation));
			for(size_t kSel = 0; kSel < commands[kcSelect].kcList.size(); kSel++)
			{//for all selection modifiers
				curKc=commands[kcSelect].kcList[kSel];
				newKc=inKc;
				newKc.Modifier(curKc);	//Add selection modifier's modifiers to this command
				if(adding)
				{
					Log("Enforcing rule krAllowNavigationWithSelection - adding key:%d with modifier:%d to command: %d\n", kSel, newKc.Modifier(), cmdNavSelection);
					Add(newKc, cmdNavSelection, false);
				} else
				{
					Log("Enforcing rule krAllowNavigationWithSelection - removing key:%d with modifier:%d to command: %d\n", kSel, newKc.Modifier(), cmdNavSelection);
					Remove(newKc, cmdNavSelection);
				}
			}
		}
		// Same applies for orderlist navigation
		else if(inCmd >= kcStartOrderlistNavigation && inCmd <= kcEndOrderlistNavigation)
		{//Check that it is a nav cmd
			CommandID cmdNavSelection = (CommandID)(kcStartOrderlistNavigationSelect+ (inCmd-kcStartOrderlistNavigation));
			for(size_t kSel = 0; kSel < commands[kcSelect].kcList.size(); kSel++)
			{//for all selection modifiers
				curKc=commands[kcSelect].kcList[kSel];
				newKc=inKc;
				newKc.AddModifier(curKc);	//Add selection modifier's modifiers to this command
				if(adding)
				{
					Log("Enforcing rule krAllowNavigationWithSelection - adding key:%d with modifier:%d to command: %d\n", kSel, newKc.Modifier(), cmdNavSelection);
					Add(newKc, cmdNavSelection, false);
				} else
				{
					Log("Enforcing rule krAllowNavigationWithSelection - removing key:%d with modifier:%d to command: %d\n", kSel, newKc.Modifier(), cmdNavSelection);
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
				for(size_t k = 0; k < commands[curCmd].kcList.size(); k++)
				{
					// for all keys for this command
					CommandID cmdNavSelection = (CommandID)(kcStartPatNavigationSelect + (curCmd-kcStartPatNavigation));
					newKc=commands[curCmd].kcList[k];	// get all properties from the current nav cmd key
					newKc.AddModifier(inKc);			// and the new selection modifier
					if(adding)
					{
						Log("Enforcing rule krAllowNavigationWithSelection - adding key:%d with modifier:%d to command: %d\n", curCmd, inKc.Modifier(), cmdNavSelection);
						Add(newKc, cmdNavSelection, false);
					} else
					{
						Log("Enforcing rule krAllowNavigationWithSelection - removing key:%d with modifier:%d to command: %d\n", curCmd, inKc.Modifier(), cmdNavSelection);
						Remove(newKc, cmdNavSelection);
					}
				}
			} // end all nav commands
			for(int curCmd = kcStartOrderlistNavigation; curCmd <= kcEndOrderlistNavigation; curCmd++)
			{// for all nav commands
				for(size_t k = 0; k < commands[curCmd].kcList.size(); k++)
				{// for all keys for this command
					CommandID cmdNavSelection = (CommandID)(kcStartOrderlistNavigationSelect+ (curCmd-kcStartOrderlistNavigation));
					newKc=commands[curCmd].kcList[k];	// get all properties from the current nav cmd key
					newKc.AddModifier(inKc);			// and the new selection modifier
					if(adding)
					{
						Log("Enforcing rule krAllowNavigationWithSelection - adding key:%d with modifier:%d to command: %d\n", curCmd, inKc.Modifier(), cmdNavSelection);
						Add(newKc, cmdNavSelection, false);
					} else
					{
						Log("Enforcing rule krAllowNavigationWithSelection - removing key:%d with modifier:%d to command: %d\n", curCmd, inKc.Modifier(), cmdNavSelection);
						Remove(newKc, cmdNavSelection);
					}
				}
			} // end all nav commands
		}
	} // end krAllowNavigationWithSelection

	if(enforceRule[krAllowSelectionWithNavigation])
	{
		KeyCombination newKcSel;

		// When we get a new navigation command key, we need to ensure
		// all selection keys will work even when this new selection key is pressed
		if(inCmd >= kcStartPatNavigation && inCmd <= kcEndPatNavigation)
		{//if this is a navigation command
			for(size_t kSel = 0; kSel < commands[kcSelect].kcList.size(); kSel++)
			{//for all deselection modifiers
				newKcSel = commands[kcSelect].kcList[kSel];	// get all properties from the selection key
				newKcSel.AddModifier(inKc);					// add modifiers from the new nav command
				if(adding)
				{
					Log("Enforcing rule krAllowSelectionWithNavigation: adding  removing kcSelectWithNav and kcSelectOffWithNav\n");
					Add(newKcSel, kcSelectWithNav, false);
				} else
				{
					Log("Enforcing rule krAllowSelectionWithNavigation: removing kcSelectWithNav and kcSelectOffWithNav\n");
					Remove(newKcSel, kcSelectWithNav);
				}
			}
		}
		// Same for orderlist navigation
		if(inCmd >= kcStartOrderlistNavigation && inCmd <= kcEndOrderlistNavigation)
		{//if this is a navigation command
			for(size_t kSel = 0; kSel < commands[kcSelect].kcList.size(); kSel++)
			{//for all deselection modifiers
				newKcSel=commands[kcSelect].kcList[kSel];	// get all properties from the selection key
				newKcSel.AddModifier(inKc);						// add modifiers from the new nav command
				if(adding)
				{
					Log("Enforcing rule krAllowSelectionWithNavigation: adding  removing kcSelectWithNav and kcSelectOffWithNav\n");
					Add(newKcSel, kcSelectWithNav, false);
				} else
				{
					Log("Enforcing rule krAllowSelectionWithNavigation: removing kcSelectWithNav and kcSelectOffWithNav\n");
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
				for(size_t k = 0; k < commands[curCmd].kcList.size(); k++)
				{// for all keys for this command
					newKcSel=inKc; // get all properties from the selection key
					newKcSel.AddModifier(commands[curCmd].kcList[k]); //add the nav keys' modifiers
					if(adding)
					{
						Log("Enforcing rule krAllowSelectionWithNavigation - adding key:%d with modifier:%d to command: %d\n", curCmd, inKc.Modifier(), kcSelectWithNav);
						Add(newKcSel, kcSelectWithNav, false);
					} else
					{
						Log("Enforcing rule krAllowSelectionWithNavigation - removing key:%d with modifier:%d to command: %d\n", curCmd, inKc.Modifier(), kcSelectWithNav);
						Remove(newKcSel, kcSelectWithNav);
					}
				}
			} // end all nav commands

			for(int curCmd = kcStartOrderlistNavigation; curCmd <= kcEndOrderlistNavigation; curCmd++)
			{//for all nav commands
				for(size_t k = 0; k < commands[curCmd].kcList.size(); k++)
				{// for all keys for this command
					newKcSel=inKc; // get all properties from the selection key
					newKcSel.AddModifier(commands[curCmd].kcList[k]); //add the nav keys' modifiers
					if(adding)
					{
						Log("Enforcing rule krAllowSelectionWithNavigation - adding key:%d with modifier:%d to command: %d\n", curCmd, inKc.Modifier(), kcSelectWithNav);
						Add(newKcSel, kcSelectWithNav, false);
					} else
					{
						Log("Enforcing rule krAllowSelectionWithNavigation - removing key:%d with modifier:%d to command: %d\n", curCmd, inKc.Modifier(), kcSelectWithNav);
						Remove(newKcSel, kcSelectWithNav);
					}
				}
			} // end all nav commands
		}

	}

	// if we add a selector or a copy selector, we need it to switch off when we release the key.
	if (enforceRule[krAutoSelectOff])
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
				newKcDeSel.Modifier(i);
				//newKcDeSel.mod&=~CodeToModifier(inKc.code);		//<-- Need to get rid of right modifier!!

				if (adding)
					Add(newKcDeSel, cmdOff, false);
				else
					Remove(newKcDeSel, cmdOff);
			}
		}

	}
	// Allow combinations of copyselect and select
	if(enforceRule[krAllowSelectCopySelectCombos])
	{
		KeyCombination newKcSel, newKcCopySel;
		if(inCmd==kcSelect)
		{
			// On getting a new selection key, make this selection key work with all copy selects' modifiers
			// On getting a new selection key, make all copyselects work with this key's modifiers
			for(size_t k = 0; k < commands[kcCopySelect].kcList.size(); k++)
			{
				newKcSel=inKc;
				newKcSel.AddModifier(commands[kcCopySelect].kcList[k]);
				newKcCopySel=commands[kcCopySelect].kcList[k];
				newKcCopySel.AddModifier(inKc);
				Log("Enforcing rule krAllowSelectCopySelectCombos\n");
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
			for(size_t k = 0; k < commands[kcSelect].kcList.size(); k++)
			{
				newKcSel=commands[kcSelect].kcList[k];
				newKcSel.AddModifier(inKc);
				newKcCopySel=inKc;
				newKcCopySel.AddModifier(commands[kcSelect].kcList[k]);
				Log("Enforcing rule krAllowSelectCopySelectCombos\n");
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


	//# Lock Notes to Chords
	if (enforceRule[krLockNotesToChords])
	{
		if (inCmd>=kcVPStartNotes && inCmd<=kcVPEndNotes)
		{
			int noteOffset = inCmd - kcVPStartNotes;
			for(size_t k = 0; k < commands[kcChordModifier].kcList.size(); k++)
			{//for all chord modifier keys
				newKc=inKc;
				newKc.AddModifier(commands[kcChordModifier].kcList[k]);
				if (adding)
				{
					Log("Enforcing rule krLockNotesToChords: auto adding in a chord command\n");
					Add(newKc, (CommandID)(kcVPStartChords+noteOffset), false);
				}
				else
				{
					Log("Enforcing rule krLockNotesToChords: auto removing a chord command\n");
					Remove(newKc, (CommandID)(kcVPStartChords+noteOffset));
				}
			}
		}
		if (inCmd==kcChordModifier)
		{
			int noteOffset;
			for (int curCmd=kcVPStartNotes; curCmd<=kcVPEndNotes; curCmd++)
			{//for all notes
				for (size_t k=0; k<commands[curCmd].kcList.size(); k++)
				{//for all keys for this note
					noteOffset = curCmd - kcVPStartNotes;
					newKc=commands[curCmd].kcList[k];
					newKc.AddModifier(inKc);
					if (adding)
					{
						Log("Enforcing rule krLockNotesToChords: auto adding in a chord command\n");
						Add(newKc, (CommandID)(kcVPStartChords+noteOffset), false);
					}
					else
					{
						Log("Enforcing rule krLockNotesToChords: auto removing a chord command\n");
						Remove(newKc, (CommandID)(kcVPStartChords+noteOffset));
					}
				}
			}
		}
	}


	//# Auto set note off on release
	if (enforceRule[krNoteOffOnKeyRelease])
	{
		if (inCmd>=kcVPStartNotes && inCmd<=kcVPEndNotes)
		{
			int noteOffset = inCmd - kcVPStartNotes;
			newKc=inKc;
			newKc.EventType(kKeyEventUp);
			if (adding)
			{
				Log("Enforcing rule krNoteOffOnKeyRelease: adding note off command\n");
				Add(newKc, (CommandID)(kcVPStartNoteStops+noteOffset), false);
			}
			else
			{
				Log("Enforcing rule krNoteOffOnKeyRelease: removing note off command\n");
				Remove(newKc, (CommandID)(kcVPStartNoteStops+noteOffset));
			}
		}
		if (inCmd>=kcVPStartChords && inCmd<=kcVPEndChords)
		{
			int noteOffset = inCmd - kcVPStartChords;
			newKc=inKc;
			newKc.EventType(kKeyEventUp);
			if (adding)
			{
				Log("Enforcing rule krNoteOffOnKeyRelease: adding Chord off command\n");
				Add(newKc, (CommandID)(kcVPStartChordStops+noteOffset), false);
			}
			else
			{
				Log("Enforcing rule krNoteOffOnKeyRelease: removing Chord off command\n");
				Remove(newKc, (CommandID)(kcVPStartChordStops+noteOffset));
			}
		}
		if(inCmd >= kcSetOctave0 && inCmd <= kcSetOctave9)
		{
			int noteOffset = inCmd - kcSetOctave0;
			newKc=inKc;
			newKc.EventType(kKeyEventUp);
			if (adding)
			{
				Log("Enforcing rule krNoteOffOnKeyRelease: adding Chord off command\n");
				Add(newKc, (CommandID)(kcSetOctaveStop0+noteOffset), false);
			}
			else
			{
				Log("Enforcing rule krNoteOffOnKeyRelease: removing Chord off command\n");
				Remove(newKc, (CommandID)(kcSetOctaveStop0+noteOffset));
			}
		}
	}

	//# Reassign freed number keys to octaves
	if (enforceRule[krReassignDigitsToOctaves] && !adding)
	{
		if ( (inKc.Modifier() == 0) &&	//no modifier
			 ( (inKc.Context() == kCtxViewPatternsNote) || (inKc.Context() == kCtxViewPatterns) ) && //note scope or pattern scope
			 ( ('0'<=inKc.KeyCode() && inKc.KeyCode()<='9') || (VK_NUMPAD0<=inKc.KeyCode() && inKc.KeyCode()<=VK_NUMPAD9) ) )  //is number key
		{
				newKc = KeyCombination(kCtxViewPatternsNote, 0, inKc.KeyCode(), kKeyEventDown);
				int offset = ('0'<=inKc.KeyCode() && inKc.KeyCode()<='9') ? newKc.KeyCode()-'0' : newKc.KeyCode()-VK_NUMPAD0;
				Add(newKc, (CommandID)(kcSetOctave0 + (newKc.KeyCode()-offset)), false);
		}
	}
	// Add spacing
	if (enforceRule[krAutoSpacing])
	{
		if (inCmd==kcSetSpacing && adding)
		{
			newKc = KeyCombination(kCtxViewPatterns, inKc.Modifier(), 0, kKeyEventDown);
			for (char i = 0; i <= 9; i++)
			{
				newKc.KeyCode('0' + i);
				Add(newKc, (CommandID)(kcSetSpacing0 + i), false);
				newKc.KeyCode(VK_NUMPAD0 + i);
				Add(newKc, (CommandID)(kcSetSpacing0 + i), false);
			}
		}
		else if (!adding && (inCmd<kcSetSpacing && kcSetSpacing9<inCmd))
		{
			for (size_t k=0; k<commands[kcSetSpacing].kcList.size(); k++)
			{
				KeyCombination spacing = commands[kcSetSpacing].kcList[k];
				if ((('0'<=inKc.KeyCode() && inKc.KeyCode()<='9')||(VK_NUMPAD0<=inKc.KeyCode() && inKc.KeyCode()<=VK_NUMPAD9)) && !adding)
				{
					newKc = KeyCombination(kCtxViewPatterns, spacing.Modifier(), inKc.KeyCode(), spacing.EventType());
					if ('0'<=inKc.KeyCode() && inKc.KeyCode()<='9')
						Add(newKc, (CommandID)(kcSetSpacing0 + inKc.KeyCode() - '0'), false);
					else if (VK_NUMPAD0<=inKc.KeyCode() && inKc.KeyCode()<=VK_NUMPAD9)
						Add(newKc, (CommandID)(kcSetSpacing0 + inKc.KeyCode() - VK_NUMPAD0), false);
				}
			}

		}
	}
	if (enforceRule[krPropagateNotes])
	{
		int noteOffset = 0;
		//newKc=inKc;

		if (inCmd>=kcVPStartNotes && inCmd<=kcVPEndNotes)
		{
			KeyCombination newKcSamp = inKc;
			KeyCombination newKcIns  = inKc;
			KeyCombination newKcTree = inKc;
			KeyCombination newKcInsNoteMap = inKc;
			KeyCombination newKcVSTGUI = inKc;

			newKcSamp.Context(kCtxViewSamples);
			newKcIns.Context(kCtxViewInstruments);
			newKcTree.Context(kCtxViewTree);
			newKcInsNoteMap.Context(kCtxInsNoteMap);
			newKcVSTGUI.Context(kCtxVSTGUI);

			noteOffset = inCmd - kcVPStartNotes;
			if (adding)
			{
				Log("Enforcing rule krPropagateNotesToSampAndIns: adding Note on in samp ctx\n");
				Add(newKcSamp, (CommandID)(kcSampStartNotes+noteOffset), false);
				Add(newKcIns, (CommandID)(kcInstrumentStartNotes+noteOffset), false);
				Add(newKcTree, (CommandID)(kcTreeViewStartNotes+noteOffset), false);
				Add(newKcInsNoteMap, (CommandID)(kcInsNoteMapStartNotes+noteOffset), false);
				Add(newKcVSTGUI, (CommandID)(kcVSTGUIStartNotes+noteOffset), false);
			}
			else
			{
				Log("Enforcing rule krPropagateNotesToSampAndIns: removing Note on in samp ctx\n");
				Remove(newKcSamp, (CommandID)(kcSampStartNotes+noteOffset));
				Remove(newKcIns, (CommandID)(kcInstrumentStartNotes+noteOffset));
				Remove(newKcTree, (CommandID)(kcTreeViewStartNotes+noteOffset));
				Remove(newKcInsNoteMap, (CommandID)(kcInsNoteMapStartNotes+noteOffset));
				Remove(newKcVSTGUI, (CommandID)(kcVSTGUIStartNotes+noteOffset));
			}
		} else if (inCmd>=kcVPStartNoteStops && inCmd<=kcVPEndNoteStops)
		{
			KeyCombination newKcSamp = inKc;
			KeyCombination newKcIns  = inKc;
			KeyCombination newKcTree = inKc;
			KeyCombination newKcInsNoteMap = inKc;
			KeyCombination newKcVSTGUI = inKc;

			newKcSamp.Context(kCtxViewSamples);
			newKcIns.Context(kCtxViewInstruments);
			newKcTree.Context(kCtxViewTree);
			newKcInsNoteMap.Context(kCtxInsNoteMap);
			newKcVSTGUI.Context(kCtxVSTGUI);

			noteOffset = inCmd - kcVPStartNoteStops;
			if (adding)
			{
				Log("Enforcing rule krPropagateNotesToSampAndIns: adding Note stop on in samp ctx\n");
				Add(newKcSamp, (CommandID)(kcSampStartNoteStops+noteOffset), false);
				Add(newKcIns, (CommandID)(kcInstrumentStartNoteStops+noteOffset), false);
				Add(newKcTree, (CommandID)(kcTreeViewStartNoteStops+noteOffset), false);
				Add(newKcInsNoteMap, (CommandID)(kcInsNoteMapStartNoteStops+noteOffset), false);
				Add(newKcVSTGUI, (CommandID)(kcVSTGUIStartNoteStops+noteOffset), false);
			}
			else
			{
				Log("Enforcing rule krPropagateNotesToSampAndIns: removing Note stop on in samp ctx\n");
				Remove(newKcSamp, (CommandID)(kcSampStartNoteStops+noteOffset));
				Remove(newKcIns, (CommandID)(kcInstrumentStartNoteStops+noteOffset));
				Remove(newKcTree, (CommandID)(kcTreeViewStartNoteStops+noteOffset));
				Remove(newKcInsNoteMap, (CommandID)(kcInsNoteMapStartNoteStops+noteOffset));
				Remove(newKcVSTGUI, (CommandID)(kcVSTGUIStartNoteStops+noteOffset));
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
	if (enforceRule[krCheckModifiers])
	{
		// for all commands that must be modifiers
		for (auto curCmd : { kcSelect, kcCopySelect, kcChordModifier, kcSetSpacing })
		{
			//for all of this command's key combinations
			for (auto &kc : commands[curCmd].kcList)
			{
				if ((!kc.Modifier()) || (kc.KeyCode()!=VK_SHIFT && kc.KeyCode()!=VK_CONTROL && kc.KeyCode()!=VK_MENU && kc.KeyCode()!=0 &&
					kc.KeyCode()!=VK_LWIN && kc.KeyCode()!=VK_RWIN )) // Feature: use Windows keys as modifier keys
				{
					report += ("Error! " + GetCommandText((CommandID)curCmd) + " must be a modifier (shift/ctrl/alt), but is currently " + inKc.GetKeyText() + "\r\n");
					//replace with dummy
					kc.Modifier(HOTKEYF_SHIFT);
					kc.KeyCode(0);
					kc.EventType(kKeyEventNone);
				}
			}

		}
	}
	if (enforceRule[krPropagateSampleManipulation])
	{
		if (inCmd>=kcSampleLoad && inCmd<=kcSampleNew)
		{
			int newCmd;
			int offset = inCmd-kcStartSampleMisc;

			//propagate to InstrumentView
			newCmd = kcStartInstrumentMisc+offset;
			commands[newCmd].kcList.resize(commands[inCmd].kcList.size());
			for (size_t k=0; k<commands[inCmd].kcList.size(); k++)
			{
				commands[newCmd].kcList[k] = commands[inCmd].kcList[k];
				commands[newCmd].kcList[k].Context(kCtxViewInstruments);
			}

		}

	}

	// Legacy: Note off commands with and without instrument numbers are now the same.
	commands[kcNoteCut].kcList.insert(commands[kcNoteCut].kcList.end(), commands[kcNoteCutOld].kcList.begin(), commands[kcNoteCutOld].kcList.end());
	commands[kcNoteCutOld].kcList.clear();
	commands[kcNoteOff].kcList.insert(commands[kcNoteOff].kcList.end(), commands[kcNoteOffOld].kcList.begin(), commands[kcNoteOffOld].kcList.end());
	commands[kcNoteOffOld].kcList.clear();
	commands[kcNoteFade].kcList.insert(commands[kcNoteFade].kcList.end(), commands[kcNoteFadeOld].kcList.begin(), commands[kcNoteFadeOld].kcList.end());
	commands[kcNoteFadeOld].kcList.clear();


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
	return report;
}


//-------------------------------------------------------
// Export
//-------------------------------------------------------
//Generate a keymap from a command set
void CCommandSet::GenKeyMap(KeyMap &km)
//-------------------------------------
{
	KeyCombination curKc;
	std::vector<KeyEventType> eventTypes;
	std::vector<InputTargetContext> contexts;

	km.clear();

	const bool allowDupes = TrackerSettings::Instance().MiscAllowMultipleCommandsPerKey;

	// Copy commandlist content into map:
	for(UINT cmd=0; cmd<kcNumCommands; cmd++)
	{
		if(IsDummyCommand((CommandID)cmd))
			continue;

		for(size_t k = 0; k < commands[cmd].kcList.size(); k++)
		{
			eventTypes.clear();
			contexts.clear();
			curKc = commands[cmd].kcList[k];

			// Handle keyEventType mask.
			if (curKc.EventType() & kKeyEventDown)
				eventTypes.push_back(kKeyEventDown);
			if (curKc.EventType() & kKeyEventUp)
				eventTypes.push_back(kKeyEventUp);
			if (curKc.EventType() & kKeyEventRepeat)
				eventTypes.push_back(kKeyEventRepeat);
			//ASSERT(eventTypes.GetSize()>0);

			// Handle super-contexts (contexts that represent a set of sub contexts)
			if (curKc.Context() == kCtxViewPatterns)
			{
				contexts.push_back(kCtxViewPatternsNote);
				contexts.push_back(kCtxViewPatternsIns);
				contexts.push_back(kCtxViewPatternsVol);
				contexts.push_back(kCtxViewPatternsFX);
				contexts.push_back(kCtxViewPatternsFXparam);
			} else if(curKc.Context() == kCtxCtrlPatterns)
			{
				contexts.push_back(kCtxCtrlOrderlist);
			} else
			{
				contexts.push_back(curKc.Context());
			}

			for (size_t cx = 0; cx < contexts.size(); cx++)
			{
				for (size_t ke = 0; ke < eventTypes.size(); ke++)
				{
					KeyCombination kc(contexts[cx], curKc.Modifier(), curKc.KeyCode(), eventTypes[ke]);
					if(!allowDupes)
					{
						KeyMapRange dupes = km.equal_range(kc);
						km.erase(dupes.first, dupes.second);
					}
					km.insert(std::make_pair(kc, (CommandID)cmd));
				}
			}
		}

	}
}


void CCommandSet::Copy(const CCommandSet *source)
//----------------------------------------------
{
	// copy constructors should take care of complexity (I hope)
	oldSpecs = nullptr;
	for (size_t cmd = 0; cmd < CountOf(commands); cmd++)
		commands[cmd] = source->commands[cmd];
}


bool CCommandSet::SaveFile(const mpt::PathString &filename)
//---------------------------------------------------------
{

/* Layout:
//----( Context1 Text (id) )----
ctx:UID:Description:Modifier:Key:EventMask
ctx:UID:Description:Modifier:Key:EventMask
...
//----( Context2 Text (id) )----
...
*/

	mpt::ofstream f(filename);
	if(!f)
	{
		ErrorBox(IDS_CANT_OPEN_FILE_FOR_WRITING);
		return false;
	}
	f << "//----------------- OpenMPT key binding definition file  ---------------\n";
	f << "//- Format is:                                                         -\n";
	f << "//- Context:Command ID:Modifiers:Key:KeypressEventType     //Comments  -\n";
	f << "//----------------------------------------------------------------------\n";
	f << "version:" << KEYMAP_VERSION << "\n";

	std::vector<HKL> layouts(GetKeyboardLayoutList(0, nullptr));
	GetKeyboardLayoutList(static_cast<int>(layouts.size()), layouts.data());

	for (int ctx = 0; ctx < kCtxMaxInputContexts; ctx++)
	{
		f << "\n//----( " << KeyCombination::GetContextText((InputTargetContext)ctx) << " (" << ctx << ") )------------\n";

		for (int cmd=0; cmd<kcNumCommands; cmd++)
		{
			for (int k=0; k<GetKeyListSize((CommandID)cmd); k++)
			{
				KeyCombination kc = GetKey((CommandID)cmd, k);

				if (kc.Context() != ctx)
					continue;		//sort by context

				if (!commands[cmd].isHidden)
				{
					f << ctx << ":"
						<< commands[cmd].UID << ":"
						<< kc.Modifier() << ":"
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
						<< (int)kc.EventType().GetRaw() << "\t\t//"
						<< mpt::ToCharset(mpt::CharsetUTF8, GetCommandText((CommandID)cmd)).c_str() << ": "
						<< mpt::ToCharset(mpt::CharsetUTF8, kc.GetKeyText()).c_str() << " ("
						<< mpt::ToCharset(mpt::CharsetUTF8, kc.GetKeyEventText()).c_str() << ")\n";
				}

			}
		}
	}

	return true;
}


bool CCommandSet::LoadFile(std::istream& iStrm, const std::wstring &filenameDescription, CCommandSet *commandSet)
//---------------------------------------------------------------------------------------------------------------
{
	KeyCombination kc;
	CommandID cmd = kcNumCommands;
	char s[1024];
	std::string curLine;
	std::vector<std::string> tokens;
	int l = 0;
	oldSpecs = nullptr;	// After clearing the key set, need to fix effect letters

	const bool fillExistingSet = commandSet != nullptr;

	// If commandSet is valid, add new commands to it (this is used for adding the default shortcuts to existing keymaps)
	CCommandSet *pTempCS = fillExistingSet ? commandSet : new CCommandSet();

	CString errText;
	int errorCount = 0;

	std::vector<HKL> layouts(GetKeyboardLayoutList(0, nullptr));
	GetKeyboardLayoutList(static_cast<int>(layouts.size()), layouts.data());

	const std::string whitespace(" \n\r\t");
	while(iStrm.getline(s, MPT_ARRAY_COUNT(s)))
	{
		curLine = s;
		l++;

		// Cut everything after a //, trim whitespace
		auto pos = curLine.find("//");
		if(pos != std::string::npos) curLine.resize(pos);
		pos = curLine.find_first_not_of(whitespace);
		if(pos != std::string::npos) curLine.erase(0, pos);
		pos = curLine.find_last_not_of(whitespace);
		if(pos != std::string::npos) curLine.resize(pos + 1);

		if (curLine.empty())
			continue;

		tokens = mpt::String::Split<std::string>(curLine, ":");
		if(tokens.size() == 2 && !mpt::CompareNoCaseAscii(tokens[0], "version"))
		{
			// This line indicates the version of this keymap file instead. (e.g. "version:1")
			int fileVersion = ConvertStrTo<int>(tokens[1]);
			if(fileVersion > KEYMAP_VERSION)
			{
				errText.AppendFormat(_T("File version is %d, but your version of OpenMPT only supports loading files up to version %d.\n"), fileVersion, KEYMAP_VERSION);
			}
			continue;
		}

		// Format: ctx:UID:Description:Modifier:Key:EventMask
		if(tokens.size() >= 5)
		{
			kc.Context(static_cast<InputTargetContext>(ConvertStrTo<int>(tokens[0])));
			cmd = static_cast<CommandID>(FindCmd(ConvertStrTo<int>(tokens[1])));

			// Modifier
			kc.Modifier(ConvertStrTo<UINT>(tokens[2]));

			// Virtual Key code / Scan code
			UINT vk = 0;
			auto scPos = tokens[3].find('/');
			if(scPos != std::string::npos)
			{
				// Scan code present
				UINT sc = ConvertStrTo<UINT>(tokens[3].substr(scPos + 1));
				for(auto i = layouts.begin(); i != layouts.end() && vk == 0; i++)
				{
					vk = MapVirtualKeyEx(sc, MAPVK_VSC_TO_VK, *i);
				}
			}
			if(vk == 0)
			{
				vk = ConvertStrTo<UINT>(tokens[3]);
			}
			kc.KeyCode(vk);

			// Event
			kc.EventType(static_cast<KeyEventType>(ConvertStrTo<int>(tokens[4])));

			if(fillExistingSet && pTempCS->GetKeyListSize(cmd) != 0)
			{
				// Do not map shortcuts that already have custom keys assigned.
				// In particular with default note keys, this can create awkward keymaps when loading
				// e.g. an IT-style keymap and it contains two keys mapped to the same notes.
				continue;
			}
		}

		// Error checking (TODO):
		if (cmd < 0 || cmd >= kcNumCommands || kc.Context() >= kCtxMaxInputContexts || tokens.size() < 4)
		{
			errorCount++;
			if (errorCount < 10)
			{
				if(tokens.size() < 4)
				{
					errText.AppendFormat(_T("Line %d was not understood.\n"), l);
				} else
				{
					errText.AppendFormat(_T("Line %d contained an unknown command.\n"), l);
				}
			} else if (errorCount == 10)
			{
				errText += _T("Too many errors detected, not reporting any more.\n");
			}
		} else
		{
			pTempCS->Add(kc, cmd, !fillExistingSet, -1, !fillExistingSet);
		}
	}

	if(!fillExistingSet)
	{
		// Add the default command set to our freshly loaded command set.
		const char *pData = nullptr;
		HGLOBAL hglob = nullptr;
		size_t nSize = 0;
		if(LoadResource(MAKEINTRESOURCE(IDR_DEFAULT_KEYBINDINGS), TEXT("KEYBINDINGS"), pData, nSize, hglob) != nullptr)
		{
			mpt::istringstream intStrm(std::string(pData, nSize));
			LoadFile(intStrm, std::wstring(), pTempCS);
		}
	} else
	{
		// We were just adding stuff to an existing command set - don't delete it!
		return true;
	}

	if(!errText.IsEmpty())
	{
		std::wstring err = L"The following problems have been encountered while trying to load the key binding file " + filenameDescription + L":\n";
		err += mpt::ToWide(errText);
		Reporting::Warning(err);
	}

	Copy(pTempCS);
	delete pTempCS;

	return true;
}


bool CCommandSet::LoadFile(const mpt::PathString &filename)
//---------------------------------------------------------
{
	mpt::ifstream fin(filename);
	if(fin.fail())
	{
		Reporting::Warning(L"Can't open keybindings file " + filename.ToWide() + L" for reading. Default keybindings will be used.");
		return false;
	} else
	{
		return LoadFile(fin, filename.ToWide());
	}
}


bool CCommandSet::LoadDefaultKeymap()
//-----------------------------------
{
	mpt::istringstream s;
	return LoadFile(s, L"\"executable resource\"");
}


//Could do better search algo but this is not perf critical.
int CCommandSet::FindCmd(int uid) const
//-------------------------------------
{
	for (int i=0; i<kcNumCommands; i++)
	{
		if (commands[i].UID == static_cast<UINT>(uid))
			return i;
	}

	return -1;
}


const TCHAR *KeyCombination::GetContextText(InputTargetContext ctx)
//-----------------------------------------------------------------
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
};


CString KeyCombination::GetKeyEventText(FlagSet<KeyEventType> event)
//------------------------------------------------------------------
{
	CString text = _T("");

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


CString KeyCombination::GetModifierText(UINT mod)
//-----------------------------------------------
{
	CString text = _T("");
	if (mod & HOTKEYF_SHIFT) text.Append(_T("Shift+"));
	if (mod & HOTKEYF_CONTROL) text.Append(_T("Ctrl+"));
	if (mod & HOTKEYF_ALT) text.Append(_T("Alt+"));
	if (mod & HOTKEYF_RSHIFT) text.Append(_T("RShift+"));
	if (mod & HOTKEYF_RCONTROL) text.Append(_T("RCtrl+"));
	if (mod & HOTKEYF_RALT) text.Append(_T("RAlt+"));
	if (mod & HOTKEYF_EXT) text.Append(_T("Win+")); // Feature: use Windows keys as modifier keys
	if (mod & HOTKEYF_MIDI) text.Append(_T("MidiCC:"));
	return text;
}


CString KeyCombination::GetKeyText(UINT mod, UINT code)
//-----------------------------------------------------
{
	CString keyText;
	keyText=GetModifierText(mod);
	if(mod & HOTKEYF_MIDI)
		keyText.AppendFormat(_T("%u"),code);
	else
		keyText.Append(CHotKeyCtrl::GetKeyName(code, IsExtended(code)));
	//HACK:
	if (keyText == _T("Ctrl+CTRL"))			keyText = _T("Ctrl");
	else if (keyText == _T("Alt+ALT"))		keyText = _T("Alt");
	else if (keyText == _T("Shift+SHIFT"))	keyText = _T("Shift");
	else if (keyText == _T("RCtrl+CTRL"))	keyText = _T("RCtrl");
	else if (keyText == _T("RAlt+ALT"))		keyText = _T("RAlt");
	else if (keyText == _T("RShift+SHIFT"))	keyText = _T("RShift");

	return keyText;
}


CString CCommandSet::GetKeyTextFromCommand(CommandID c, UINT key)
//---------------------------------------------------------------
{
	if ( key < commands[c].kcList.size())
		return commands[c].kcList[0].GetKeyText();
	else
		return _T("");
}


//-------------------------------------------------------
// Quick Changes - modify many commands with one call.
//-------------------------------------------------------

bool CCommandSet::QuickChange_NotesRepeat(bool repeat)
//----------------------------------------------------
{
	for (CommandID cmd = kcVPStartNotes; cmd <= kcVPEndNotes; cmd=(CommandID)(cmd + 1))		//for all notes
	{
		for(auto &kc : commands[cmd].kcList)
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
//--------------------------------------------------------------------------
{
	// Is this already the active key configuration?
	if(&modSpecs == oldSpecs)
	{
		return false;
	}
	oldSpecs = &modSpecs;

	int choices = 0;
	KeyCombination kc(kCtxViewPatternsFX, 0, 0, kKeyEventDown | kKeyEventRepeat);

	for(CommandID cmd = kcFixedFXStart; cmd <= kcFixedFXend; cmd = static_cast<CommandID>(cmd + 1))
	{
		//Remove all old choices
		choices = GetKeyListSize(cmd);
		for(int p = choices; p >= 0; --p)
		{
			Remove(p, cmd);
		}

		char effect = modSpecs.GetEffectLetter(static_cast<ModCommand::COMMAND>(cmd - kcSetFXStart + 1));
		if(effect >= 'A' && effect <= 'Z')
		{
			// VkKeyScanEx needs lowercase letters
			effect = effect - 'A' + 'a';
		} else if(effect < '0' || effect > '9')
		{
			// Don't map effects that use "weird" effect letters (such as # or \)
			effect = '?';
		}

		if(effect != '?')
		{
			// Hack for situations where a non-latin keyboard layout without A...Z key code mapping may the current layout (e.g. Russian),
			// but a latin layout (e.g. EN-US) is installed as well.
			std::vector<HKL> layouts(GetKeyboardLayoutList(0, nullptr));
			GetKeyboardLayoutList(static_cast<int>(layouts.size()), layouts.data());
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
				kc.Modifier(0);
				Add(kc, cmd, true);
			}

			if (effect >= '0' && effect <= '9')		//for numbers, ensure numpad works too
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
//----------------------------------------
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
//	m_isParentContext.SetSize(kCtxMaxInputContexts);

	for (UINT nCtx=0; nCtx<kCtxMaxInputContexts; nCtx++)
	{
//		m_isParentContext[nCtx].SetSize(kCtxMaxInputContexts);
		for (UINT nCtx2=0; nCtx2<kCtxMaxInputContexts; nCtx2++)
		{
			m_isParentContext[nCtx][nCtx2] = false;
		}//InputTargetContext
	}

	//For now much be fully expanded (i.e. don't rely on grandparent relationships).
	m_isParentContext[kCtxViewGeneral][kCtxAllContexts] = true;
	m_isParentContext[kCtxViewPatterns][kCtxAllContexts] = true;
	m_isParentContext[kCtxViewPatternsNote][kCtxAllContexts] = true;
	m_isParentContext[kCtxViewPatternsIns][kCtxAllContexts] = true;
	m_isParentContext[kCtxViewPatternsVol][kCtxAllContexts] = true;
	m_isParentContext[kCtxViewPatternsFX][kCtxAllContexts] = true;
	m_isParentContext[kCtxViewPatternsFXparam][kCtxAllContexts] = true;
	m_isParentContext[kCtxViewSamples][kCtxAllContexts] = true;
	m_isParentContext[kCtxViewInstruments][kCtxAllContexts] = true;
	m_isParentContext[kCtxViewComments][kCtxAllContexts] = true;
	m_isParentContext[kCtxViewTree][kCtxAllContexts] = true;
	m_isParentContext[kCtxInsNoteMap][kCtxAllContexts] = true;
	m_isParentContext[kCtxVSTGUI][kCtxAllContexts] = true;
	m_isParentContext[kCtxCtrlGeneral][kCtxAllContexts] = true;
	m_isParentContext[kCtxCtrlPatterns][kCtxAllContexts] = true;
	m_isParentContext[kCtxCtrlSamples][kCtxAllContexts] = true;
	m_isParentContext[kCtxCtrlInstruments][kCtxAllContexts] = true;
	m_isParentContext[kCtxCtrlComments][kCtxAllContexts] = true;
	m_isParentContext[kCtxCtrlSamples][kCtxAllContexts] = true;
	m_isParentContext[kCtxCtrlOrderlist][kCtxAllContexts] = true;
	m_isParentContext[kCtxChannelSettings][kCtxAllContexts] = true;

	m_isParentContext[kCtxViewPatternsNote][kCtxViewPatterns] = true;
	m_isParentContext[kCtxViewPatternsIns][kCtxViewPatterns] = true;
	m_isParentContext[kCtxViewPatternsVol][kCtxViewPatterns] = true;
	m_isParentContext[kCtxViewPatternsFX][kCtxViewPatterns] = true;
	m_isParentContext[kCtxViewPatternsFXparam][kCtxViewPatterns] = true;
	m_isParentContext[kCtxCtrlOrderlist][kCtxCtrlPatterns] = true;

}


bool CCommandSet::KeyCombinationConflict(KeyCombination kc1, KeyCombination kc2, bool checkEventConflict) const
//-------------------------------------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------------
{
	return m_isParentContext[kc1.Context()][kc2.Context()] || m_isParentContext[kc2.Context()][kc1.Context()];
}

OPENMPT_NAMESPACE_END
