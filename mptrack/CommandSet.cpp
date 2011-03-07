//rewbs.customKeys

#include "stdafx.h"
#include ".\commandset.h"
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#define ENABLE_LOGGING 0

#if(ENABLE_LOGGING)
	//
#else
	#define Log
#endif


bool CCommandSet::s_bShowErrorOnUnknownKeybinding = true;

CCommandSet::CCommandSet(void)
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
	
	commands.SetSize(kcNumCommands);
	SetupCommands();
	SetupContextHierarchy();
}

CCommandSet::~CCommandSet(void)
{
	//CHAR s[64];
	//wsprintf(s, "pointer = %lX",plocalCmdSet);
	//::MessageBox(NULL, "about to remove all", NULL, MB_OK|MB_ICONEXCLAMATION); //disabled by rewbs
	//commands.RemoveAll();
}


//-------------------------------------------------------
// Setup
//-------------------------------------------------------

// Helper function for setting up commands
inline void CCommandSet::DefineKeyCommand(CommandID kc, UINT uid, enmKcVisibility visibility, enmKcDummy dummy, CString message)
//------------------------------------------------------------------------------------------------------------------------------
{
	commands[kc].UID = uid;
	commands[kc].isHidden = (visibility == kcHidden) ? true : false;
	commands[kc].isDummy = (dummy == kcDummy) ? true : false;
	commands[kc].Message = message;
}

//Get command descriptions etc.. loaded up.
void CCommandSet::SetupCommands()
//-------------------------------
{	//TODO: make this hideous list a bit nicer, with a constructor or somthing.
	//NOTE: isHidden implies automatically set, since the user will not be able to see it.

	DefineKeyCommand(kcPatternRecord, 1001, kcVisible, kcNoDummy, _T("Enable recording"));
	DefineKeyCommand(kcPatternPlayRow, 1002, kcVisible, kcNoDummy, _T("Play row"));
	DefineKeyCommand(kcCursorCopy, 1003, kcVisible, kcNoDummy, _T("Quick copy"));
	DefineKeyCommand(kcCursorPaste, 1004, kcVisible, kcNoDummy, _T("Quick paste"));
	DefineKeyCommand(kcChannelMute, 1005, kcVisible, kcNoDummy, _T("Mute current channel"));
	DefineKeyCommand(kcChannelSolo, 1006, kcVisible, kcNoDummy, _T("Solo current channel"));
	DefineKeyCommand(kcTransposeUp, 1007, kcVisible, kcNoDummy, _T("Transpose +1"));
	DefineKeyCommand(kcTransposeDown, 1008, kcVisible, kcNoDummy, _T("Transpose -1"));
	DefineKeyCommand(kcTransposeOctUp, 1009, kcVisible, kcNoDummy, _T("Transpose +12"));
	DefineKeyCommand(kcTransposeOctDown, 1010, kcVisible, kcNoDummy, _T("Transpose -12"));
	DefineKeyCommand(kcSelectColumn, 1011, kcVisible, kcNoDummy, _T("Select channel / Select all"));
	DefineKeyCommand(kcPatternAmplify, 1012, kcVisible, kcNoDummy, _T("Amplify selection"));
	DefineKeyCommand(kcPatternSetInstrument, 1013, kcVisible, kcNoDummy, _T("Apply current instrument"));
	DefineKeyCommand(kcPatternInterpolateVol, 1014, kcVisible, kcNoDummy, _T("Interpolate volume"));
	DefineKeyCommand(kcPatternInterpolateEffect, 1015, kcVisible, kcNoDummy, _T("Interpolate effect"));
	DefineKeyCommand(kcPatternVisualizeEffect, 1016, kcVisible, kcNoDummy, _T("Open effect visualizer"));
	DefineKeyCommand(kcPatternJumpDownh1, 1017, kcVisible, kcNoDummy, _T("Jump down by measure"));
	DefineKeyCommand(kcPatternJumpUph1, 1018, kcVisible, kcNoDummy, _T("Jump up by measure"));
	DefineKeyCommand(kcPatternSnapDownh1, 1019, kcVisible, kcNoDummy, _T("Snap down to measure"));
	DefineKeyCommand(kcPatternSnapUph1, 1020, kcVisible, kcNoDummy, _T("Snap up to measure"));
	DefineKeyCommand(kcViewGeneral, 1021, kcVisible, kcNoDummy, _T("View General"));
	DefineKeyCommand(kcViewPattern, 1022, kcVisible, kcNoDummy, _T("View Pattern"));
	DefineKeyCommand(kcViewSamples, 1023, kcVisible, kcNoDummy, _T("View Samples"));
	DefineKeyCommand(kcViewInstruments, 1024, kcVisible, kcNoDummy, _T("View Instruments"));
	DefineKeyCommand(kcViewComments, 1025, kcVisible, kcNoDummy, _T("View Comments"));
	DefineKeyCommand(kcPlayPatternFromCursor, 1026, kcVisible, kcNoDummy, _T("Play pattern from cursor"));
	DefineKeyCommand(kcPlayPatternFromStart, 1027, kcVisible, kcNoDummy, _T("Play pattern from start"));
	DefineKeyCommand(kcPlaySongFromCursor, 1028, kcVisible, kcNoDummy, _T("Play song from cursor"));
	DefineKeyCommand(kcPlaySongFromStart, 1029, kcVisible, kcNoDummy, _T("Play song from start"));
	DefineKeyCommand(kcPlayPauseSong, 1030, kcVisible, kcNoDummy, _T("Play song/Pause song"));
	DefineKeyCommand(kcPauseSong, 1031, kcVisible, kcNoDummy, _T("Pause song"));
	DefineKeyCommand(kcPrevInstrument, 1032, kcVisible, kcNoDummy, _T("Previous instrument"));
	DefineKeyCommand(kcNextInstrument, 1033, kcVisible, kcNoDummy, _T("Next instrument"));
	DefineKeyCommand(kcPrevOrder, 1034, kcVisible, kcNoDummy, _T("Previous order"));
	DefineKeyCommand(kcNextOrder, 1035, kcVisible, kcNoDummy, _T("Next order"));
	DefineKeyCommand(kcPrevOctave, 1036, kcVisible, kcNoDummy, _T("Previous octave"));
	DefineKeyCommand(kcNextOctave, 1037, kcVisible, kcNoDummy, _T("Next octave"));
	DefineKeyCommand(kcNavigateDown, 1038, kcVisible, kcNoDummy, _T("Navigate down by 1 row"));
	DefineKeyCommand(kcNavigateUp, 1039, kcVisible, kcNoDummy, _T("Navigate up by 1 row"));
	DefineKeyCommand(kcNavigateLeft, 1040, kcVisible, kcNoDummy, _T("Navigate left"));
	DefineKeyCommand(kcNavigateRight, 1041, kcVisible, kcNoDummy, _T("Navigate right"));
	DefineKeyCommand(kcNavigateNextChan, 1042, kcVisible, kcNoDummy, _T("Navigate to next channel"));
	DefineKeyCommand(kcNavigatePrevChan, 1043, kcVisible, kcNoDummy, _T("Navigate to previous channel"));
	DefineKeyCommand(kcHomeHorizontal, 1044, kcVisible, kcNoDummy, _T("Go to first channel"));
	DefineKeyCommand(kcHomeVertical, 1045, kcVisible, kcNoDummy, _T("Go to first row"));
	DefineKeyCommand(kcHomeAbsolute, 1046, kcVisible, kcNoDummy, _T("Go to first row of first channel"));
	DefineKeyCommand(kcEndHorizontal, 1047, kcVisible, kcNoDummy, _T("Go to last channel"));
	DefineKeyCommand(kcEndVertical, 1048, kcVisible, kcNoDummy, _T("Go to last row"));
	DefineKeyCommand(kcEndAbsolute, 1049, kcVisible, kcNoDummy, _T("Go to last row of last channel"));
	DefineKeyCommand(kcSelect, 1050, kcVisible, kcNoDummy, _T("Selection key"));
	DefineKeyCommand(kcCopySelect, 1051, kcVisible, kcNoDummy, _T("Copy select key"));
	DefineKeyCommand(kcSelectOff, 1052, kcHidden, kcNoDummy, _T("Deselect"));
	DefineKeyCommand(kcCopySelectOff, 1053, kcHidden, kcNoDummy, _T("Copy deselect key"));
	DefineKeyCommand(kcNextPattern, 1054, kcVisible, kcNoDummy, _T("Next pattern"));
	DefineKeyCommand(kcPrevPattern, 1055, kcVisible, kcNoDummy, _T("Previous pattern"));
	//DefineKeyCommand(kcClearSelection, 1056, kcVisible, kcNoDummy, _T("Wipe selection"));
	DefineKeyCommand(kcClearRow, 1057, kcVisible, kcNoDummy, _T("Clear row"));
	DefineKeyCommand(kcClearField, 1058, kcVisible, kcNoDummy, _T("Clear field"));
	DefineKeyCommand(kcClearRowStep, 1059, kcVisible, kcNoDummy, _T("Clear row and step"));
	DefineKeyCommand(kcClearFieldStep, 1060, kcVisible, kcNoDummy, _T("Clear field and step"));
	DefineKeyCommand(kcDeleteRows, 1061, kcVisible, kcNoDummy, _T("Delete rows"));
	DefineKeyCommand(kcShowNoteProperties, 1062, kcVisible, kcNoDummy, _T("Show note properties"));
	DefineKeyCommand(kcShowEditMenu, 1063, kcVisible, kcNoDummy, _T("Show context (right-click) menu"));
	DefineKeyCommand(kcVPNoteC_0, 1064, kcVisible, kcNoDummy, _T("Base octave C"));
	DefineKeyCommand(kcVPNoteCS0, 1065, kcVisible, kcNoDummy, _T("Base octave C#"));
	DefineKeyCommand(kcVPNoteD_0, 1066, kcVisible, kcNoDummy, _T("Base octave D"));
	DefineKeyCommand(kcVPNoteDS0, 1067, kcVisible, kcNoDummy, _T("Base octave D#"));
	DefineKeyCommand(kcVPNoteE_0, 1068, kcVisible, kcNoDummy, _T("Base octave E"));
	DefineKeyCommand(kcVPNoteF_0, 1069, kcVisible, kcNoDummy, _T("Base octave F"));
	DefineKeyCommand(kcVPNoteFS0, 1070, kcVisible, kcNoDummy, _T("Base octave F#"));
	DefineKeyCommand(kcVPNoteG_0, 1071, kcVisible, kcNoDummy, _T("Base octave G"));
	DefineKeyCommand(kcVPNoteGS0, 1072, kcVisible, kcNoDummy, _T("Base octave G#"));
	DefineKeyCommand(kcVPNoteA_1, 1073, kcVisible, kcNoDummy, _T("Base octave A"));
	DefineKeyCommand(kcVPNoteAS1, 1074, kcVisible, kcNoDummy, _T("Base octave A#"));
	DefineKeyCommand(kcVPNoteB_1, 1075, kcVisible, kcNoDummy, _T("Base octave B"));
	DefineKeyCommand(kcVPNoteC_1, 1076, kcVisible, kcNoDummy, _T("Base octave +1 C"));
	DefineKeyCommand(kcVPNoteCS1, 1077, kcVisible, kcNoDummy, _T("Base octave +1 C#"));
	DefineKeyCommand(kcVPNoteD_1, 1078, kcVisible, kcNoDummy, _T("Base octave +1 D"));
	DefineKeyCommand(kcVPNoteDS1, 1079, kcVisible, kcNoDummy, _T("Base octave +1 D#"));
	DefineKeyCommand(kcVPNoteE_1, 1080, kcVisible, kcNoDummy, _T("Base octave +1 E"));
	DefineKeyCommand(kcVPNoteF_1, 1081, kcVisible, kcNoDummy, _T("Base octave +1 F"));
	DefineKeyCommand(kcVPNoteFS1, 1082, kcVisible, kcNoDummy, _T("Base octave +1 F#"));
	DefineKeyCommand(kcVPNoteG_1, 1083, kcVisible, kcNoDummy, _T("Base octave +1 G"));
	DefineKeyCommand(kcVPNoteGS1, 1084, kcVisible, kcNoDummy, _T("Base octave +1 G#"));
	DefineKeyCommand(kcVPNoteA_2, 1085, kcVisible, kcNoDummy, _T("Base octave +1 A"));
	DefineKeyCommand(kcVPNoteAS2, 1086, kcVisible, kcNoDummy, _T("Base octave +1 A#"));
	DefineKeyCommand(kcVPNoteB_2, 1087, kcVisible, kcNoDummy, _T("Base octave +1 B"));
	DefineKeyCommand(kcVPNoteC_2, 1088, kcVisible, kcNoDummy, _T("Base octave +2 C"));
	DefineKeyCommand(kcVPNoteCS2, 1089, kcVisible, kcNoDummy, _T("Base octave +2 C#"));
	DefineKeyCommand(kcVPNoteD_2, 1090, kcVisible, kcNoDummy, _T("Base octave +2 D"));
	DefineKeyCommand(kcVPNoteDS2, 1091, kcVisible, kcNoDummy, _T("Base octave +2 D#"));
	DefineKeyCommand(kcVPNoteE_2, 1092, kcVisible, kcNoDummy, _T("Base octave +2 E"));
	DefineKeyCommand(kcVPNoteF_2, 1093, kcVisible, kcNoDummy, _T("Base octave +2 F"));
	DefineKeyCommand(kcVPNoteFS2, 1094, kcVisible, kcNoDummy, _T("Base octave +2 F#"));
	DefineKeyCommand(kcVPNoteG_2, 1095, kcVisible, kcNoDummy, _T("Base octave +2 G"));
	DefineKeyCommand(kcVPNoteGS2, 1096, kcVisible, kcNoDummy, _T("Base octave +2 G#"));
	DefineKeyCommand(kcVPNoteA_3, 1097, kcVisible, kcNoDummy, _T("Base octave +2 A"));
	DefineKeyCommand(kcVPNoteStopC_0, 1098, kcHidden, kcNoDummy, _T("Stop base octave C"));
	DefineKeyCommand(kcVPNoteStopCS0, 1099, kcHidden, kcNoDummy, _T("Stop base octave C#"));
	DefineKeyCommand(kcVPNoteStopD_0, 1100, kcHidden, kcNoDummy, _T("Stop base octave D"));
	DefineKeyCommand(kcVPNoteStopDS0, 1101, kcHidden, kcNoDummy, _T("Stop base octave D#"));
	DefineKeyCommand(kcVPNoteStopE_0, 1102, kcHidden, kcNoDummy, _T("Stop base octave E"));
	DefineKeyCommand(kcVPNoteStopF_0, 1103, kcHidden, kcNoDummy, _T("Stop base octave F"));
	DefineKeyCommand(kcVPNoteStopFS0, 1104, kcHidden, kcNoDummy, _T("Stop base octave F#"));
	DefineKeyCommand(kcVPNoteStopG_0, 1105, kcHidden, kcNoDummy, _T("Stop base octave G"));
	DefineKeyCommand(kcVPNoteStopGS0, 1106, kcHidden, kcNoDummy, _T("Stop base octave G#"));
	DefineKeyCommand(kcVPNoteStopA_1, 1107, kcHidden, kcNoDummy, _T("Stop base octave +1 A"));
	DefineKeyCommand(kcVPNoteStopAS1, 1108, kcHidden, kcNoDummy, _T("Stop base octave +1 A#"));
	DefineKeyCommand(kcVPNoteStopB_1, 1109, kcHidden, kcNoDummy, _T("Stop base octave +1 B"));
	DefineKeyCommand(kcVPNoteStopC_1, 1110, kcHidden, kcNoDummy, _T("Stop base octave +1 C"));
	DefineKeyCommand(kcVPNoteStopCS1, 1111, kcHidden, kcNoDummy, _T("Stop base octave +1 C#"));
	DefineKeyCommand(kcVPNoteStopD_1, 1112, kcHidden, kcNoDummy, _T("Stop base octave +1 D"));
	DefineKeyCommand(kcVPNoteStopDS1, 1113, kcHidden, kcNoDummy, _T("Stop base octave +1 D#"));
	DefineKeyCommand(kcVPNoteStopE_1, 1114, kcHidden, kcNoDummy, _T("Stop base octave +1 E"));
	DefineKeyCommand(kcVPNoteStopF_1, 1115, kcHidden, kcNoDummy, _T("Stop base octave +1 F"));
	DefineKeyCommand(kcVPNoteStopFS1, 1116, kcHidden, kcNoDummy, _T("Stop base octave +1 F#"));
	DefineKeyCommand(kcVPNoteStopG_1, 1117, kcHidden, kcNoDummy, _T("Stop base octave +1 G"));
	DefineKeyCommand(kcVPNoteStopGS1, 1118, kcHidden, kcNoDummy, _T("Stop base octave +1 G#"));
	DefineKeyCommand(kcVPNoteStopA_2, 1119, kcHidden, kcNoDummy, _T("Stop base octave +2 A"));
	DefineKeyCommand(kcVPNoteStopAS2, 1120, kcHidden, kcNoDummy, _T("Stop base octave +2 A#"));
	DefineKeyCommand(kcVPNoteStopB_2, 1121, kcHidden, kcNoDummy, _T("Stop base octave +2 B"));
	DefineKeyCommand(kcVPNoteStopC_2, 1122, kcHidden, kcNoDummy, _T("Stop base octave +2 C"));
	DefineKeyCommand(kcVPNoteStopCS2, 1123, kcHidden, kcNoDummy, _T("Stop base octave +2 C#"));
	DefineKeyCommand(kcVPNoteStopD_2, 1124, kcHidden, kcNoDummy, _T("Stop base octave +2 D"));
	DefineKeyCommand(kcVPNoteStopDS2, 1125, kcHidden, kcNoDummy, _T("Stop base octave +2 D#"));
	DefineKeyCommand(kcVPNoteStopE_2, 1126, kcHidden, kcNoDummy, _T("Stop base octave +2 E"));
	DefineKeyCommand(kcVPNoteStopF_2, 1127, kcHidden, kcNoDummy, _T("Stop base octave +2 F"));
	DefineKeyCommand(kcVPNoteStopFS2, 1128, kcHidden, kcNoDummy, _T("Stop base octave +2 F#"));
	DefineKeyCommand(kcVPNoteStopG_2, 1129, kcHidden, kcNoDummy, _T("Stop base octave +2 G"));
	DefineKeyCommand(kcVPNoteStopGS2, 1130, kcHidden, kcNoDummy, _T("Stop base octave +2 G#"));
	DefineKeyCommand(kcVPNoteStopA_3, 1131, kcHidden, kcNoDummy, _T("Stop base octave +3 A"));
	DefineKeyCommand(kcVPChordC_0, 1132, kcHidden, kcNoDummy, _T("base octave chord C"));
	DefineKeyCommand(kcVPChordCS0, 1133, kcHidden, kcNoDummy, _T("base octave chord C#"));
	DefineKeyCommand(kcVPChordD_0, 1134, kcHidden, kcNoDummy, _T("base octave chord D"));
	DefineKeyCommand(kcVPChordDS0, 1135, kcHidden, kcNoDummy, _T("base octave chord D#"));
	DefineKeyCommand(kcVPChordE_0, 1136, kcHidden, kcNoDummy, _T("base octave chord E"));
	DefineKeyCommand(kcVPChordF_0, 1137, kcHidden, kcNoDummy, _T("base octave chord F"));
	DefineKeyCommand(kcVPChordFS0, 1138, kcHidden, kcNoDummy, _T("base octave chord F#"));
	DefineKeyCommand(kcVPChordG_0, 1139, kcHidden, kcNoDummy, _T("base octave chord G"));
	DefineKeyCommand(kcVPChordGS0, 1140, kcHidden, kcNoDummy, _T("base octave chord G#"));
	DefineKeyCommand(kcVPChordA_1, 1141, kcHidden, kcNoDummy, _T("base octave +1 chord A"));
	DefineKeyCommand(kcVPChordAS1, 1142, kcHidden, kcNoDummy, _T("base octave +1 chord A#"));
	DefineKeyCommand(kcVPChordB_1, 1143, kcHidden, kcNoDummy, _T("base octave +1 chord B"));
	DefineKeyCommand(kcVPChordC_1, 1144, kcHidden, kcNoDummy, _T("base octave +1 chord C"));
	DefineKeyCommand(kcVPChordCS1, 1145, kcHidden, kcNoDummy, _T("base octave +1 chord C#"));
	DefineKeyCommand(kcVPChordD_1, 1146, kcHidden, kcNoDummy, _T("base octave +1 chord D"));
	DefineKeyCommand(kcVPChordDS1, 1147, kcHidden, kcNoDummy, _T("base octave +1 chord D#"));
	DefineKeyCommand(kcVPChordE_1, 1148, kcHidden, kcNoDummy, _T("base octave +1 chord E"));
	DefineKeyCommand(kcVPChordF_1, 1149, kcHidden, kcNoDummy, _T("base octave +1 chord F"));
	DefineKeyCommand(kcVPChordFS1, 1150, kcHidden, kcNoDummy, _T("base octave +1 chord F#"));
	DefineKeyCommand(kcVPChordG_1, 1151, kcHidden, kcNoDummy, _T("base octave +1 chord G"));
	DefineKeyCommand(kcVPChordGS1, 1152, kcHidden, kcNoDummy, _T("base octave +1 chord G#"));
	DefineKeyCommand(kcVPChordA_2, 1153, kcHidden, kcNoDummy, _T("base octave +2 chord A"));
	DefineKeyCommand(kcVPChordAS2, 1154, kcHidden, kcNoDummy, _T("base octave +2 chord A#"));
	DefineKeyCommand(kcVPChordB_2, 1155, kcHidden, kcNoDummy, _T("base octave +2 chord B"));
	DefineKeyCommand(kcVPChordC_2, 1156, kcHidden, kcNoDummy, _T("base octave +2 chord C"));
	DefineKeyCommand(kcVPChordCS2, 1157, kcHidden, kcNoDummy, _T("base octave +2 chord C#"));
	DefineKeyCommand(kcVPChordD_2, 1158, kcHidden, kcNoDummy, _T("base octave +2 chord D"));
	DefineKeyCommand(kcVPChordDS2, 1159, kcHidden, kcNoDummy, _T("base octave +2 chord D#"));
	DefineKeyCommand(kcVPChordE_2, 1160, kcHidden, kcNoDummy, _T("base octave +2 chord E"));
	DefineKeyCommand(kcVPChordF_2, 1161, kcHidden, kcNoDummy, _T("base octave +2 chord F"));
	DefineKeyCommand(kcVPChordFS2, 1162, kcHidden, kcNoDummy, _T("base octave +2 chord F#"));
	DefineKeyCommand(kcVPChordG_2, 1163, kcHidden, kcNoDummy, _T("base octave +2 chord G"));
	DefineKeyCommand(kcVPChordGS2, 1164, kcHidden, kcNoDummy, _T("base octave +2 chord G#"));
	DefineKeyCommand(kcVPChordA_3, 1165, kcHidden, kcNoDummy, _T("base octave chord +3 A"));
	DefineKeyCommand(kcVPChordStopC_0, 1166, kcHidden, kcNoDummy, _T("Stop base octave chord C"));
	DefineKeyCommand(kcVPChordStopCS0, 1167, kcHidden, kcNoDummy, _T("Stop base octave chord C#"));
	DefineKeyCommand(kcVPChordStopD_0, 1168, kcHidden, kcNoDummy, _T("Stop base octave chord D"));
	DefineKeyCommand(kcVPChordStopDS0, 1169, kcHidden, kcNoDummy, _T("Stop base octave chord D#"));
	DefineKeyCommand(kcVPChordStopE_0, 1170, kcHidden, kcNoDummy, _T("Stop base octave chord E"));
	DefineKeyCommand(kcVPChordStopF_0, 1171, kcHidden, kcNoDummy, _T("Stop base octave chord F"));
	DefineKeyCommand(kcVPChordStopFS0, 1172, kcHidden, kcNoDummy, _T("Stop base octave chord F#"));
	DefineKeyCommand(kcVPChordStopG_0, 1173, kcHidden, kcNoDummy, _T("Stop base octave chord G"));
	DefineKeyCommand(kcVPChordStopGS0, 1174, kcHidden, kcNoDummy, _T("Stop base octave chord G#"));
	DefineKeyCommand(kcVPChordStopA_1, 1175, kcHidden, kcNoDummy, _T("Stop base octave +1 chord A"));
	DefineKeyCommand(kcVPChordStopAS1, 1176, kcHidden, kcNoDummy, _T("Stop base octave +1 chord A#"));
	DefineKeyCommand(kcVPChordStopB_1, 1177, kcHidden, kcNoDummy, _T("Stop base octave +1 chord B"));
	DefineKeyCommand(kcVPChordStopC_1, 1178, kcHidden, kcNoDummy, _T("Stop base octave +1 chord C"));
	DefineKeyCommand(kcVPChordStopCS1, 1179, kcHidden, kcNoDummy, _T("Stop base octave +1 chord C#"));
	DefineKeyCommand(kcVPChordStopD_1, 1180, kcHidden, kcNoDummy, _T("Stop base octave +1 chord D"));
	DefineKeyCommand(kcVPChordStopDS1, 1181, kcHidden, kcNoDummy, _T("Stop base octave +1 chord D#"));
	DefineKeyCommand(kcVPChordStopE_1, 1182, kcHidden, kcNoDummy, _T("Stop base octave +1 chord E"));
	DefineKeyCommand(kcVPChordStopF_1, 1183, kcHidden, kcNoDummy, _T("Stop base octave +1 chord F"));
	DefineKeyCommand(kcVPChordStopFS1, 1184, kcHidden, kcNoDummy, _T("Stop base octave +1 chord F#"));
	DefineKeyCommand(kcVPChordStopG_1, 1185, kcHidden, kcNoDummy, _T("Stop base octave +1 chord G"));
	DefineKeyCommand(kcVPChordStopGS1, 1186, kcHidden, kcNoDummy, _T("Stop base octave +1 chord G#"));
	DefineKeyCommand(kcVPChordStopA_2, 1187, kcHidden, kcNoDummy, _T("Stop base octave +2 chord A"));
	DefineKeyCommand(kcVPChordStopAS2, 1188, kcHidden, kcNoDummy, _T("Stop base octave +2 chord A#"));
	DefineKeyCommand(kcVPChordStopB_2, 1189, kcHidden, kcNoDummy, _T("Stop base octave +2 chord B"));
	DefineKeyCommand(kcVPChordStopC_2, 1190, kcHidden, kcNoDummy, _T("Stop base octave +2 chord C"));
	DefineKeyCommand(kcVPChordStopCS2, 1191, kcHidden, kcNoDummy, _T("Stop base octave +2 chord C#"));
	DefineKeyCommand(kcVPChordStopD_2, 1192, kcHidden, kcNoDummy, _T("Stop base octave +2 chord D"));
	DefineKeyCommand(kcVPChordStopDS2, 1193, kcHidden, kcNoDummy, _T("Stop base octave +2 chord D#"));
	DefineKeyCommand(kcVPChordStopE_2, 1194, kcHidden, kcNoDummy, _T("Stop base octave +2 chord E"));
	DefineKeyCommand(kcVPChordStopF_2, 1195, kcHidden, kcNoDummy, _T("Stop base octave +2 chord F"));
	DefineKeyCommand(kcVPChordStopFS2, 1196, kcHidden, kcNoDummy, _T("Stop base octave +2 chord F#"));
	DefineKeyCommand(kcVPChordStopG_2, 1197, kcHidden, kcNoDummy, _T("Stop base octave +2 chord G"));
	DefineKeyCommand(kcVPChordStopGS2, 1198, kcHidden, kcNoDummy, _T("Stop base octave +2 chord G#"));
	DefineKeyCommand(kcVPChordStopA_3, 1199, kcHidden, kcNoDummy, _T("Stop base octave +3 chord  A"));
	DefineKeyCommand(kcNoteCut, 1200, kcVisible, kcNoDummy, _T("Note Cut"));
	DefineKeyCommand(kcNoteOff, 1201, kcVisible, kcNoDummy, _T("Note Off"));
	DefineKeyCommand(kcSetIns0, 1202, kcVisible, kcNoDummy, _T("Set instrument digit 0"));
	DefineKeyCommand(kcSetIns1, 1203, kcVisible, kcNoDummy, _T("Set instrument digit 1"));
	DefineKeyCommand(kcSetIns2, 1204, kcVisible, kcNoDummy, _T("Set instrument digit 2"));
	DefineKeyCommand(kcSetIns3, 1205, kcVisible, kcNoDummy, _T("Set instrument digit 3"));
	DefineKeyCommand(kcSetIns4, 1206, kcVisible, kcNoDummy, _T("Set instrument digit 4"));
	DefineKeyCommand(kcSetIns5, 1207, kcVisible, kcNoDummy, _T("Set instrument digit 5"));
	DefineKeyCommand(kcSetIns6, 1208, kcVisible, kcNoDummy, _T("Set instrument digit 6"));
	DefineKeyCommand(kcSetIns7, 1209, kcVisible, kcNoDummy, _T("Set instrument digit 7"));
	DefineKeyCommand(kcSetIns8, 1210, kcVisible, kcNoDummy, _T("Set instrument digit 8"));
	DefineKeyCommand(kcSetIns9, 1211, kcVisible, kcNoDummy, _T("Set instrument digit 9"));
	DefineKeyCommand(kcSetOctave0, 1212, kcVisible, kcNoDummy, _T("Set octave 0"));
	DefineKeyCommand(kcSetOctave1, 1213, kcVisible, kcNoDummy, _T("Set octave 1"));
	DefineKeyCommand(kcSetOctave2, 1214, kcVisible, kcNoDummy, _T("Set octave 2"));
	DefineKeyCommand(kcSetOctave3, 1215, kcVisible, kcNoDummy, _T("Set octave 3"));
	DefineKeyCommand(kcSetOctave4, 1216, kcVisible, kcNoDummy, _T("Set octave 4"));
	DefineKeyCommand(kcSetOctave5, 1217, kcVisible, kcNoDummy, _T("Set octave 5"));
	DefineKeyCommand(kcSetOctave6, 1218, kcVisible, kcNoDummy, _T("Set octave 6"));
	DefineKeyCommand(kcSetOctave7, 1219, kcVisible, kcNoDummy, _T("Set octave 7"));
	DefineKeyCommand(kcSetOctave8, 1220, kcVisible, kcNoDummy, _T("Set octave 8"));
	DefineKeyCommand(kcSetOctave9, 1221, kcVisible, kcNoDummy, _T("Set octave 9"));
	DefineKeyCommand(kcSetVolume0, 1222, kcVisible, kcNoDummy, _T("Set volume digit 0"));
	DefineKeyCommand(kcSetVolume1, 1223, kcVisible, kcNoDummy, _T("Set volume digit 1"));
	DefineKeyCommand(kcSetVolume2, 1224, kcVisible, kcNoDummy, _T("Set volume digit 2"));
	DefineKeyCommand(kcSetVolume3, 1225, kcVisible, kcNoDummy, _T("Set volume digit 3"));
	DefineKeyCommand(kcSetVolume4, 1226, kcVisible, kcNoDummy, _T("Set volume digit 4"));
	DefineKeyCommand(kcSetVolume5, 1227, kcVisible, kcNoDummy, _T("Set volume digit 5"));
	DefineKeyCommand(kcSetVolume6, 1228, kcVisible, kcNoDummy, _T("Set volume digit 6"));
	DefineKeyCommand(kcSetVolume7, 1229, kcVisible, kcNoDummy, _T("Set volume digit 7"));
	DefineKeyCommand(kcSetVolume8, 1230, kcVisible, kcNoDummy, _T("Set volume digit 8"));
	DefineKeyCommand(kcSetVolume9, 1231, kcVisible, kcNoDummy, _T("Set volume digit 9"));
	DefineKeyCommand(kcSetVolumeVol, 1232, kcVisible, kcNoDummy, _T("Vol command - volume"));
	DefineKeyCommand(kcSetVolumePan, 1233, kcVisible, kcNoDummy, _T("Vol command - pan"));
	DefineKeyCommand(kcSetVolumeVolSlideUp, 1234, kcVisible, kcNoDummy, _T("Vol command - vol slide up"));
	DefineKeyCommand(kcSetVolumeVolSlideDown, 1235, kcVisible, kcNoDummy, _T("Vol command - vol slide down"));
	DefineKeyCommand(kcSetVolumeFineVolUp, 1236, kcVisible, kcNoDummy, _T("Vol command - vol fine slide up"));
	DefineKeyCommand(kcSetVolumeFineVolDown, 1237, kcVisible, kcNoDummy, _T("Vol command - vol fine slide down"));
	DefineKeyCommand(kcSetVolumeVibratoSpd, 1238, kcVisible, kcNoDummy, _T("Vol command - vibrato speed"));
	DefineKeyCommand(kcSetVolumeVibrato, 1239, kcVisible, kcNoDummy, _T("Vol command - vibrato"));
	DefineKeyCommand(kcSetVolumeXMPanLeft, 1240, kcVisible, kcNoDummy, _T("Vol command - XM pan left"));
	DefineKeyCommand(kcSetVolumeXMPanRight, 1241, kcVisible, kcNoDummy, _T("Vol command - XM pan right"));
	DefineKeyCommand(kcSetVolumePortamento, 1242, kcVisible, kcNoDummy, _T("Vol command - Portamento"));
	DefineKeyCommand(kcSetVolumeITPortaUp, 1243, kcVisible, kcNoDummy, _T("Vol command - Portamento Up"));
	DefineKeyCommand(kcSetVolumeITPortaDown, 1244, kcVisible, kcNoDummy, _T("Vol command - Portamento Down"));
	DefineKeyCommand(kcSetVolumeITUnused, 1245, kcHidden, kcNoDummy, _T("Vol command - Unused"));
	DefineKeyCommand(kcSetVolumeITOffset, 1246, kcVisible, kcNoDummy, _T("Vol command - Offset"));
	DefineKeyCommand(kcSetFXParam0, 1247, kcVisible, kcNoDummy, _T("FX Param digit 0"));
	DefineKeyCommand(kcSetFXParam1, 1248, kcVisible, kcNoDummy, _T("FX Param digit 1"));
	DefineKeyCommand(kcSetFXParam2, 1249, kcVisible, kcNoDummy, _T("FX Param digit 2"));
	DefineKeyCommand(kcSetFXParam3, 1250, kcVisible, kcNoDummy, _T("FX Param digit 3"));
	DefineKeyCommand(kcSetFXParam4, 1251, kcVisible, kcNoDummy, _T("FX Param digit 4"));
	DefineKeyCommand(kcSetFXParam5, 1252, kcVisible, kcNoDummy, _T("FX Param digit 5"));
	DefineKeyCommand(kcSetFXParam6, 1253, kcVisible, kcNoDummy, _T("FX Param digit 6"));
	DefineKeyCommand(kcSetFXParam7, 1254, kcVisible, kcNoDummy, _T("FX Param digit 7"));
	DefineKeyCommand(kcSetFXParam8, 1255, kcVisible, kcNoDummy, _T("FX Param digit 8"));
	DefineKeyCommand(kcSetFXParam9, 1256, kcVisible, kcNoDummy, _T("FX Param digit 9"));
	DefineKeyCommand(kcSetFXParamA, 1257, kcVisible, kcNoDummy, _T("FX Param digit A"));
	DefineKeyCommand(kcSetFXParamB, 1258, kcVisible, kcNoDummy, _T("FX Param digit B"));
	DefineKeyCommand(kcSetFXParamC, 1259, kcVisible, kcNoDummy, _T("FX Param digit C"));
	DefineKeyCommand(kcSetFXParamD, 1260, kcVisible, kcNoDummy, _T("FX Param digit D"));
	DefineKeyCommand(kcSetFXParamE, 1261, kcVisible, kcNoDummy, _T("FX Param digit E"));
	DefineKeyCommand(kcSetFXParamF, 1262, kcVisible, kcNoDummy, _T("FX Param digit F"));
	DefineKeyCommand(kcSetFXarp, 1263, kcHidden, kcNoDummy, _T("FX arpeggio"));
	DefineKeyCommand(kcSetFXportUp, 1264, kcHidden, kcNoDummy, _T("FX portamentao Up"));
	DefineKeyCommand(kcSetFXportDown, 1265, kcHidden, kcNoDummy, _T("FX portamentao Down"));
	DefineKeyCommand(kcSetFXport, 1266, kcHidden, kcNoDummy, _T("FX portamentao"));
	DefineKeyCommand(kcSetFXvibrato, 1267, kcHidden, kcNoDummy, _T("FX vibrato"));
	DefineKeyCommand(kcSetFXportSlide, 1268, kcHidden, kcNoDummy, _T("FX portamento slide"));
	DefineKeyCommand(kcSetFXvibSlide, 1269, kcHidden, kcNoDummy, _T("FX vibrato slide"));
	DefineKeyCommand(kcSetFXtremolo, 1270, kcHidden, kcNoDummy, _T("FX tremolo"));
	DefineKeyCommand(kcSetFXpan, 1271, kcHidden, kcNoDummy, _T("FX pan"));
	DefineKeyCommand(kcSetFXoffset, 1272, kcHidden, kcNoDummy, _T("FX offset"));
	DefineKeyCommand(kcSetFXvolSlide, 1273, kcHidden, kcNoDummy, _T("FX Volume slide"));
	DefineKeyCommand(kcSetFXgotoOrd, 1274, kcHidden, kcNoDummy, _T("FX go to order"));
	DefineKeyCommand(kcSetFXsetVol, 1275, kcHidden, kcNoDummy, _T("FX set volume"));
	DefineKeyCommand(kcSetFXgotoRow, 1276, kcHidden, kcNoDummy, _T("FX go to row"));
	DefineKeyCommand(kcSetFXretrig, 1277, kcHidden, kcNoDummy, _T("FX retrigger"));
	DefineKeyCommand(kcSetFXspeed, 1278, kcHidden, kcNoDummy, _T("FX set speed"));
	DefineKeyCommand(kcSetFXtempo, 1279, kcHidden, kcNoDummy, _T("FX set tempo"));
	DefineKeyCommand(kcSetFXtremor, 1280, kcHidden, kcNoDummy, _T("FX tremor"));
	DefineKeyCommand(kcSetFXextendedMOD, 1281, kcHidden, kcNoDummy, _T("FX extended MOD cmds"));
	DefineKeyCommand(kcSetFXextendedS3M, 1282, kcHidden, kcNoDummy, _T("FX extended S3M cmds"));
	DefineKeyCommand(kcSetFXchannelVol, 1283, kcHidden, kcNoDummy, _T("FX set channel vol"));
	DefineKeyCommand(kcSetFXchannelVols, 1284, kcHidden, kcNoDummy, _T("FX channel vol slide"));
	DefineKeyCommand(kcSetFXglobalVol, 1285, kcHidden, kcNoDummy, _T("FX set global volume"));
	DefineKeyCommand(kcSetFXglobalVols, 1286, kcHidden, kcNoDummy, _T("FX global volume slide"));
	DefineKeyCommand(kcSetFXkeyoff, 1287, kcHidden, kcNoDummy, _T("FX Some XM Command :D"));
	DefineKeyCommand(kcSetFXfineVib, 1288, kcHidden, kcNoDummy, _T("FX fine vibrato"));
	DefineKeyCommand(kcSetFXpanbrello, 1289, kcHidden, kcNoDummy, _T("FX set panbrello"));
	DefineKeyCommand(kcSetFXextendedXM, 1290, kcHidden, kcNoDummy, _T("FX extended XM effects "));
	DefineKeyCommand(kcSetFXpanSlide, 1291, kcHidden, kcNoDummy, _T("FX pan slide"));
	DefineKeyCommand(kcSetFXsetEnvPos, 1292, kcHidden, kcNoDummy, _T("FX set envelope position (XM only)"));
	DefineKeyCommand(kcSetFXmacro, 1293, kcHidden, kcNoDummy, _T("FX midi macro"));
	DefineKeyCommand(kcSetFXmacroSlide, 1294, kcVisible, kcNoDummy, _T("FX midi macro slide"));
	DefineKeyCommand(kcSetFXdelaycut, 1295, kcVisible, kcNoDummy, _T("FX combined note delay and note cut"));
	DefineKeyCommand(kcPatternJumpDownh1Select, 1296, kcHidden, kcNoDummy, _T("kcPatternJumpDownh1Select"));
	DefineKeyCommand(kcPatternJumpUph1Select, 1297, kcHidden, kcNoDummy, _T("kcPatternJumpUph1Select"));
	DefineKeyCommand(kcPatternSnapDownh1Select, 1298, kcHidden, kcNoDummy, _T("kcPatternSnapDownh1Select"));
	DefineKeyCommand(kcPatternSnapUph1Select, 1299, kcHidden, kcNoDummy, _T("kcPatternSnapUph1Select"));
	DefineKeyCommand(kcNavigateDownSelect, 1300, kcHidden, kcNoDummy, _T("kcNavigateDownSelect"));
	DefineKeyCommand(kcNavigateUpSelect, 1301, kcHidden, kcNoDummy, _T("kcNavigateUpSelect"));
	DefineKeyCommand(kcNavigateLeftSelect, 1302, kcHidden, kcNoDummy, _T("kcNavigateLeftSelect"));
	DefineKeyCommand(kcNavigateRightSelect, 1303, kcHidden, kcNoDummy, _T("kcNavigateRightSelect"));
	DefineKeyCommand(kcNavigateNextChanSelect, 1304, kcHidden, kcNoDummy, _T("kcNavigateNextChanSelect"));
	DefineKeyCommand(kcNavigatePrevChanSelect, 1305, kcHidden, kcNoDummy, _T("kcNavigatePrevChanSelect"));
	DefineKeyCommand(kcHomeHorizontalSelect, 1306, kcHidden, kcNoDummy, _T("kcHomeHorizontalSelect"));
	DefineKeyCommand(kcHomeVerticalSelect, 1307, kcHidden, kcNoDummy, _T("kcHomeVerticalSelect"));
	DefineKeyCommand(kcHomeAbsoluteSelect, 1308, kcHidden, kcNoDummy, _T("kcHomeAbsoluteSelect"));
	DefineKeyCommand(kcEndHorizontalSelect, 1309, kcHidden, kcNoDummy, _T("kcEndHorizontalSelect"));
	DefineKeyCommand(kcEndVerticalSelect, 1310, kcHidden, kcNoDummy, _T("kcEndVerticalSelect"));
	DefineKeyCommand(kcEndAbsoluteSelect, 1311, kcHidden, kcNoDummy, _T("kcEndAbsoluteSelect"));
	DefineKeyCommand(kcSelectWithNav, 1312, kcHidden, kcNoDummy, _T("kcSelectWithNav"));
	DefineKeyCommand(kcSelectOffWithNav, 1313, kcHidden, kcNoDummy, _T("kcSelectOffWithNav"));
	DefineKeyCommand(kcCopySelectWithNav, 1314, kcHidden, kcNoDummy, _T("kcCopySelectWithNav"));
	DefineKeyCommand(kcCopySelectOffWithNav, 1315, kcHidden, kcNoDummy, _T("kcCopySelectOffWithNav"));
	DefineKeyCommand(kcChordModifier, 1316, kcVisible, kcDummy, _T("Chord Modifier"));
	DefineKeyCommand(kcSetSpacing, 1317, kcVisible, kcDummy, _T("Set row jump on note entry"));
	DefineKeyCommand(kcSetSpacing0, 1318, kcHidden, kcNoDummy, _T(""));
	DefineKeyCommand(kcSetSpacing1, 1319, kcHidden, kcNoDummy, _T(""));
	DefineKeyCommand(kcSetSpacing2, 1320, kcHidden, kcNoDummy, _T(""));
	DefineKeyCommand(kcSetSpacing3, 1321, kcHidden, kcNoDummy, _T(""));
	DefineKeyCommand(kcSetSpacing4, 1322, kcHidden, kcNoDummy, _T(""));
	DefineKeyCommand(kcSetSpacing5, 1323, kcHidden, kcNoDummy, _T(""));
	DefineKeyCommand(kcSetSpacing6, 1324, kcHidden, kcNoDummy, _T(""));
	DefineKeyCommand(kcSetSpacing7, 1325, kcHidden, kcNoDummy, _T(""));
	DefineKeyCommand(kcSetSpacing8, 1326, kcHidden, kcNoDummy, _T(""));
	DefineKeyCommand(kcSetSpacing9, 1327, kcHidden, kcNoDummy, _T(""));
	DefineKeyCommand(kcCopySelectWithSelect, 1328, kcHidden, kcNoDummy, _T("kcCopySelectWithSelect"));
	DefineKeyCommand(kcCopySelectOffWithSelect, 1329, kcHidden, kcNoDummy, _T("kcCopySelectOffWithSelect"));
	DefineKeyCommand(kcSelectWithCopySelect, 1330, kcHidden, kcNoDummy, _T("kcSelectWithCopySelect"));
	DefineKeyCommand(kcSelectOffWithCopySelect, 1331, kcHidden, kcNoDummy, _T("kcSelectOffWithCopySelect"));
	/*
	DefineKeyCommand(kcCopy, 1332, kcVisible, kcNoDummy, _T("Copy pattern data"));
	DefineKeyCommand(kcCut, 1333, kcVisible, kcNoDummy, _T("Cut pattern data"));
	DefineKeyCommand(kcPaste, 1334, kcVisible, kcNoDummy, _T("Paste pattern data"));
	DefineKeyCommand(kcMixPaste, 1335, kcVisible, kcNoDummy, _T("Mix-paste pattern data"));
	DefineKeyCommand(kcSelectAll, 1336, kcVisible, kcNoDummy, _T("Select all pattern data"));
	DefineKeyCommand(kcSelectCol, 1337, kcHidden, kcNoDummy, _T("Select channel / select all"));
	*/
	DefineKeyCommand(kcPatternJumpDownh2, 1338, kcVisible, kcNoDummy, _T("Jump down by beat"));
	DefineKeyCommand(kcPatternJumpUph2, 1339, kcVisible, kcNoDummy, _T("Jump up by beat"));
	DefineKeyCommand(kcPatternSnapDownh2, 1340, kcVisible, kcNoDummy, _T("Snap down to beat"));
	DefineKeyCommand(kcPatternSnapUph2, 1341, kcVisible, kcNoDummy, _T("Snap up to beat"));
	DefineKeyCommand(kcPatternJumpDownh2Select, 1342, kcHidden, kcNoDummy, _T("kcPatternJumpDownh2Select"));
	DefineKeyCommand(kcPatternJumpUph2Select, 1343, kcHidden, kcNoDummy, _T("kcPatternJumpUph2Select"));
	DefineKeyCommand(kcPatternSnapDownh2Select, 1344, kcHidden, kcNoDummy, _T("kcPatternSnapDownh2Select"));
	DefineKeyCommand(kcPatternSnapUph2Select, 1345, kcHidden, kcNoDummy, _T("kcPatternSnapUph2Select"));
	DefineKeyCommand(kcFileOpen, 1346, kcVisible, kcNoDummy, _T("File/Open"));
	DefineKeyCommand(kcFileNew, 1347, kcVisible, kcNoDummy, _T("File/New"));
	DefineKeyCommand(kcFileClose, 1348, kcVisible, kcNoDummy, _T("File/Close"));
	DefineKeyCommand(kcFileSave, 1349, kcVisible, kcNoDummy, _T("File/Save"));
	DefineKeyCommand(kcFileSaveAs, 1350, kcVisible, kcNoDummy, _T("File/Save As"));
	DefineKeyCommand(kcFileSaveAsWave, 1351, kcVisible, kcNoDummy, _T("File/Export as Wave"));
	DefineKeyCommand(kcFileSaveAsMP3, 1352, kcVisible, kcNoDummy, _T("File/Export as MP3"));
	DefineKeyCommand(kcFileSaveMidi, 1353, kcVisible, kcNoDummy, _T("File/Export as MIDI"));
	DefineKeyCommand(kcFileImportMidiLib, 1354, kcVisible, kcNoDummy, _T("File/Import Midi Lib"));
	DefineKeyCommand(kcFileAddSoundBank, 1355, kcVisible, kcNoDummy, _T("File/Add Sound Bank"));
	DefineKeyCommand(kcStopSong, 1356, kcVisible, kcNoDummy, _T("Stop Song"));
	DefineKeyCommand(kcMidiRecord, 1357, kcVisible, kcNoDummy, _T("Toggle Midi Record"));
	DefineKeyCommand(kcEstimateSongLength, 1358, kcVisible, kcNoDummy, _T("Estimate Song Length"));
	DefineKeyCommand(kcEditUndo, 1359, kcVisible, kcNoDummy, _T("Undo"));
	DefineKeyCommand(kcEditCut, 1360, kcVisible, kcNoDummy, _T("Cut"));
	DefineKeyCommand(kcEditCopy, 1361, kcVisible, kcNoDummy, _T("Copy"));
	DefineKeyCommand(kcEditPaste, 1362, kcVisible, kcNoDummy, _T("Paste"));
	DefineKeyCommand(kcEditMixPaste, 1363, kcVisible, kcNoDummy, _T("Mix Paste"));
	DefineKeyCommand(kcEditSelectAll, 1364, kcVisible, kcNoDummy, _T("SelectAll"));
	DefineKeyCommand(kcEditFind, 1365, kcVisible, kcNoDummy, _T("Find"));
	DefineKeyCommand(kcEditFindNext, 1366, kcVisible, kcNoDummy, _T("Find Next"));
	DefineKeyCommand(kcViewMain, 1367, kcVisible, kcNoDummy, _T("Toggle Main View"));
	DefineKeyCommand(kcViewTree, 1368, kcVisible, kcNoDummy, _T("Toggle Tree View"));
	DefineKeyCommand(kcViewOptions, 1369, kcVisible, kcNoDummy, _T("View Options"));
	DefineKeyCommand(kcHelp, 1370, kcVisible, kcNoDummy, _T("Help (to do)"));
	/*
	DefineKeyCommand(kcWindowNew, 1370, kcVisible, kcNoDummy, _T("New Window"));
	DefineKeyCommand(kcWindowCascade, 1371, kcVisible, kcNoDummy, _T("Cascade Windows"));
	DefineKeyCommand(kcWindowTileHorz, 1372, kcVisible, kcNoDummy, _T("Tile Windows Horizontally"));
	DefineKeyCommand(kcWindowTileVert, 1373, kcVisible, kcNoDummy, _T("Tile Windows Vertically"));
	*/
	DefineKeyCommand(kcEstimateSongLength, 1374, kcVisible, kcNoDummy, _T("Estimate Song Length"));
	DefineKeyCommand(kcStopSong, 1375, kcVisible, kcNoDummy, _T("Stop Song"));
	DefineKeyCommand(kcMidiRecord, 1376, kcVisible, kcNoDummy, _T("Toggle Midi Record"));
	DefineKeyCommand(kcDeleteAllRows, 1377, kcVisible, kcNoDummy, _T("Delete all rows"));
	DefineKeyCommand(kcInsertRow, 1378, kcVisible, kcNoDummy, _T("Insert Row"));
	DefineKeyCommand(kcInsertAllRows, 1379, kcVisible, kcNoDummy, _T("Insert All Rows"));
	DefineKeyCommand(kcSampleTrim, 1380, kcVisible, kcNoDummy, _T("Trim sample around loop points"));
	DefineKeyCommand(kcSampleReverse, 1381, kcVisible, kcNoDummy, _T("Reverse sample"));
	DefineKeyCommand(kcSampleDelete, 1382, kcVisible, kcNoDummy, _T("Delete sample selection"));
	DefineKeyCommand(kcSampleSilence, 1383, kcVisible, kcNoDummy, _T("Silence sample selection"));
	DefineKeyCommand(kcSampleNormalize, 1384, kcVisible, kcNoDummy, _T("Normalise Sample"));
	DefineKeyCommand(kcSampleAmplify, 1385, kcVisible, kcNoDummy, _T("Amplify Sample"));
	DefineKeyCommand(kcSampleZoomUp, 1386, kcVisible, kcNoDummy, _T("Zoom Out"));
	DefineKeyCommand(kcSampleZoomDown, 1387, kcVisible, kcNoDummy, _T("Zoom In"));
	//time saving HACK:
	for(size_t j = kcSampStartNotes; j <= kcInsNoteMapEndNoteStops; j++)
	{
		DefineKeyCommand((CommandID)j, 1388 + j - kcSampStartNotes, kcHidden, kcNoDummy, _T("Auto Note in some context"));
	}
	//end hack
	DefineKeyCommand(kcPatternGrowSelection, 1660, kcVisible, kcNoDummy, _T("Grow selection"));
	DefineKeyCommand(kcPatternShrinkSelection, 1661, kcVisible, kcNoDummy, _T("Shrink selection"));
	DefineKeyCommand(kcTogglePluginEditor, 1662, kcVisible, kcNoDummy, _T("Toggle channel's plugin editor"));
	DefineKeyCommand(kcToggleFollowSong, 1663, kcVisible, kcNoDummy, _T("Toggle follow song"));
	DefineKeyCommand(kcClearFieldITStyle, 1664, kcVisible, kcNoDummy, _T("Clear field (IT Style)"));
	DefineKeyCommand(kcClearFieldStepITStyle, 1665, kcVisible, kcNoDummy, _T("Clear field and step (IT Style)"));
	DefineKeyCommand(kcSetFXextension, 1666, kcVisible, kcNoDummy, _T("FX parameter extension command"));
	DefineKeyCommand(kcNoteCutOld, 1667, kcVisible, kcNoDummy, _T("Note Cut (don't remember instrument)"));
	DefineKeyCommand(kcNoteOffOld, 1668, kcVisible, kcNoDummy, _T("Note Off (don't remember instrument)"));
	DefineKeyCommand(kcViewAddPlugin, 1669, kcVisible, kcNoDummy, _T("View Plugin Manager"));
	DefineKeyCommand(kcViewChannelManager, 1670, kcVisible, kcNoDummy, _T("View Channel Manager"));
	DefineKeyCommand(kcCopyAndLoseSelection, 1671, kcVisible, kcNoDummy, _T("Copy and lose selection"));
	DefineKeyCommand(kcNewPattern, 1672, kcVisible, kcNoDummy, _T("Insert new pattern"));
	DefineKeyCommand(kcSampleLoad, 1673, kcVisible, kcNoDummy, _T("Load a Sample"));
	DefineKeyCommand(kcSampleSave, 1674, kcVisible, kcNoDummy, _T("Save Sample"));
	DefineKeyCommand(kcSampleNew, 1675, kcVisible, kcNoDummy, _T("New Sample"));
	DefineKeyCommand(kcSampleCtrlLoad, 1676, kcHidden, kcNoDummy, _T("Load a Sample"));
	DefineKeyCommand(kcSampleCtrlSave, 1677, kcHidden, kcNoDummy, _T("Save Sample"));
	DefineKeyCommand(kcSampleCtrlNew, 1678, kcHidden, kcNoDummy, _T("New Sample"));
	DefineKeyCommand(kcInstrumentLoad, 1679, kcHidden, kcNoDummy, _T("Load an instrument"));
	DefineKeyCommand(kcInstrumentSave, 1680, kcHidden, kcNoDummy, _T("Save instrument"));
	DefineKeyCommand(kcInstrumentNew, 1681, kcHidden, kcNoDummy, _T("New instrument"));
	DefineKeyCommand(kcInstrumentCtrlLoad, 1682, kcHidden, kcNoDummy, _T("Load an instrument"));
	DefineKeyCommand(kcInstrumentCtrlSave, 1683, kcHidden, kcNoDummy, _T("Save instrument"));
	DefineKeyCommand(kcInstrumentCtrlNew, 1684, kcHidden, kcNoDummy, _T("New instrument"));
	DefineKeyCommand(kcSwitchToOrderList, 1685, kcVisible, kcNoDummy, _T("Switch to order list"));
	DefineKeyCommand(kcEditMixPasteITStyle, 1686, kcVisible, kcNoDummy, _T("Mix Paste (old IT Style)"));
	DefineKeyCommand(kcApproxRealBPM, 1687, kcVisible, kcNoDummy, _T("Show approx. real BPM"));
	DefineKeyCommand(kcNavigateDownBySpacingSelect, 1689, kcHidden, kcNoDummy, _T("kcNavigateDownBySpacingSelect"));
	DefineKeyCommand(kcNavigateUpBySpacingSelect, 1690, kcHidden, kcNoDummy, _T("kcNavigateUpBySpacingSelect"));
	DefineKeyCommand(kcNavigateDownBySpacing, 1691, kcVisible, kcNoDummy, _T("Navigate down by spacing"));
	DefineKeyCommand(kcNavigateUpBySpacing, 1692, kcVisible, kcNoDummy, _T("Navigate up by spacing"));
	DefineKeyCommand(kcPrevDocument, 1693, kcVisible, kcNoDummy, _T("Previous Document"));
	DefineKeyCommand(kcNextDocument, 1694, kcVisible, kcNoDummy, _T("Next Document"));
	//time saving HACK:
	for(size_t j = kcVSTGUIStartNotes; j <= kcVSTGUIEndNoteStops; j++)
	{
		DefineKeyCommand((CommandID)j, 1695 + j - kcVSTGUIStartNotes, kcHidden, kcNoDummy, _T("Auto Note in some context"));
	}
	//end hack
	DefineKeyCommand(kcVSTGUIPrevPreset, 1763, kcVisible, kcNoDummy, _T("Previous plugin preset"));
	DefineKeyCommand(kcVSTGUINextPreset, 1764, kcVisible, kcNoDummy, _T("Next plugin preset"));
	DefineKeyCommand(kcVSTGUIRandParams, 1765, kcVisible, kcNoDummy, _T("Randomize plugin parameters"));
	DefineKeyCommand(kcPatternGoto, 1766, kcVisible, kcNoDummy, _T("Go to row/channel/..."));
	DefineKeyCommand(kcPatternOpenRandomizer, 1767, kcHidden, kcNoDummy, _T("Go to row/channel/...")); // while there's not randomizer yet, let's just disable it for now
	DefineKeyCommand(kcPatternInterpolateNote, 1768, kcVisible, kcNoDummy, _T("Interpolate note"));
	//rewbs.graph
	DefineKeyCommand(kcViewGraph, 1769, kcHidden, kcNoDummy, _T("View Graph"));  // while there's no graph yet, let's just disable it for now
	//end rewbs.graph
	DefineKeyCommand(kcToggleChanMuteOnPatTransition, 1770, kcVisible, kcNoDummy, _T("(Un)mute chan on pat transition"));
	DefineKeyCommand(kcChannelUnmuteAll, 1771, kcVisible, kcNoDummy, _T("Unmute all channels"));
	DefineKeyCommand(kcShowPatternProperties, 1772, kcVisible, kcNoDummy, _T("Show pattern properties window"));
	DefineKeyCommand(kcShowMacroConfig, 1773, kcVisible, kcNoDummy, _T("Show macro configuration"));
	DefineKeyCommand(kcViewSongProperties, 1775, kcVisible, kcNoDummy, _T("Show song properties window"));
	DefineKeyCommand(kcChangeLoopStatus, 1776, kcVisible, kcNoDummy, _T("Toggle loop pattern"));
	DefineKeyCommand(kcFileExportCompat, 1777, kcVisible, kcNoDummy, _T("File/Export to standard IT/XM/S3M"));
	DefineKeyCommand(kcUnmuteAllChnOnPatTransition, 1778, kcVisible, kcNoDummy, _T("Unmute all channels on pattern transition"));
	DefineKeyCommand(kcSoloChnOnPatTransition, 1779, kcVisible, kcNoDummy, _T("Solo channel on pattern transition"));
	DefineKeyCommand(kcTimeAtRow, 1780, kcVisible, kcNoDummy, _T("Show playback time at current row"));
	DefineKeyCommand(kcViewMIDImapping, 1781, kcVisible, kcNoDummy, _T("View MIDI mapping"));
	DefineKeyCommand(kcVSTGUIPrevPresetJump, 1782, kcVisible, kcNoDummy, _T("Plugin preset backward jump"));
	DefineKeyCommand(kcVSTGUINextPresetJump, 1783, kcVisible, kcNoDummy, _T("Plugin preset forward jump"));
	DefineKeyCommand(kcSampleInvert, 1784, kcVisible, kcNoDummy, _T("Invert sample phase"));
	DefineKeyCommand(kcSampleSignUnsign, 1785, kcVisible, kcNoDummy, _T("Signed/Unsigned conversion"));
	DefineKeyCommand(kcChannelReset, 1786, kcVisible, kcNoDummy, _T("Reset channel"));
	DefineKeyCommand(kcSwitchOverflowPaste, 1787, kcVisible, kcNoDummy, _T("Toggle overflow paste"));
	DefineKeyCommand(kcNotePC, 1788, kcVisible, kcNoDummy, _T("Parameter control(MPTm only)"));
	DefineKeyCommand(kcNotePCS, 1789, kcVisible, kcNoDummy, _T("Parameter control(smooth)(MPTm only)"));
	DefineKeyCommand(kcSampleRemoveDCOffset, 1790, kcVisible, kcNoDummy, _T("Remove DC Offset"));
	DefineKeyCommand(kcNoteFade, 1791, kcVisible, kcNoDummy, _T("Note Fade"));
	DefineKeyCommand(kcNoteFadeOld, 1792, kcVisible, kcNoDummy, _T("Note Fade (don't remember instrument)"));
	DefineKeyCommand(kcEditPasteFlood, 1793, kcVisible, kcNoDummy, _T("Paste Flood"));
	DefineKeyCommand(kcOrderlistNavigateLeft, 1794, kcVisible, kcNoDummy, _T("Previous Order"));
	DefineKeyCommand(kcOrderlistNavigateRight, 1795, kcVisible, kcNoDummy, _T("Next Order"));
	DefineKeyCommand(kcOrderlistNavigateFirst, 1796, kcVisible, kcNoDummy, _T("First Order"));
	DefineKeyCommand(kcOrderlistNavigateLast, 1797, kcVisible, kcNoDummy, _T("Last Order"));
	DefineKeyCommand(kcOrderlistNavigateLeftSelect, 1798, kcHidden, kcNoDummy, _T("kcOrderlistNavigateLeftSelect"));
	DefineKeyCommand(kcOrderlistNavigateRightSelect, 1799, kcHidden, kcNoDummy, _T("kcOrderlistNavigateRightSelect"));
	DefineKeyCommand(kcOrderlistNavigateFirstSelect, 1800, kcHidden, kcNoDummy, _T("kcOrderlistNavigateFirstSelect"));
	DefineKeyCommand(kcOrderlistNavigateLastSelect, 1801, kcHidden, kcNoDummy, _T("kcOrderlistNavigateLastSelect"));
	DefineKeyCommand(kcOrderlistEditDelete, 1802, kcVisible, kcNoDummy, _T("Delete Order"));
	DefineKeyCommand(kcOrderlistEditInsert, 1803, kcVisible, kcNoDummy, _T("Insert Order"));
	DefineKeyCommand(kcOrderlistEditPattern, 1804, kcVisible, kcNoDummy, _T("Edit Pattern"));
	DefineKeyCommand(kcOrderlistSwitchToPatternView, 1805, kcVisible, kcNoDummy, _T("Switch to pattern editor"));
	DefineKeyCommand(kcDuplicatePattern, 1806, kcVisible, kcNoDummy, _T("Duplicate pattern"));
	DefineKeyCommand(kcOrderlistPat0, 1807, kcVisible, kcNoDummy, _T("Pattern index digit 0"));
	DefineKeyCommand(kcOrderlistPat1, 1808, kcVisible, kcNoDummy, _T("Pattern index digit 1"));
	DefineKeyCommand(kcOrderlistPat2, 1809, kcVisible, kcNoDummy, _T("Pattern index digit 2"));
	DefineKeyCommand(kcOrderlistPat3, 1810, kcVisible, kcNoDummy, _T("Pattern index digit 3"));
	DefineKeyCommand(kcOrderlistPat4, 1811, kcVisible, kcNoDummy, _T("Pattern index digit 4"));
	DefineKeyCommand(kcOrderlistPat5, 1812, kcVisible, kcNoDummy, _T("Pattern index digit 5"));
	DefineKeyCommand(kcOrderlistPat6, 1813, kcVisible, kcNoDummy, _T("Pattern index digit 6"));
	DefineKeyCommand(kcOrderlistPat7, 1814, kcVisible, kcNoDummy, _T("Pattern index digit 7"));
	DefineKeyCommand(kcOrderlistPat8, 1815, kcVisible, kcNoDummy, _T("Pattern index digit 8"));
	DefineKeyCommand(kcOrderlistPat9, 1816, kcVisible, kcNoDummy, _T("Pattern index digit 9"));
	DefineKeyCommand(kcOrderlistPatPlus, 1817, kcVisible, kcNoDummy, _T("Increase pattern index "));
	DefineKeyCommand(kcOrderlistPatMinus, 1818, kcVisible, kcNoDummy, _T("Decrease pattern index"));
	DefineKeyCommand(kcShowSplitKeyboardSettings, 1819, kcVisible, kcNoDummy, _T("Split Keyboard Settings dialog"));
	DefineKeyCommand(kcEditPushForwardPaste, 1820, kcVisible, kcNoDummy, _T("Push Forward Paste"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveLeft, 1821, kcVisible, kcNoDummy, _T("Move envelope point left"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveRight, 1822, kcVisible, kcNoDummy, _T("Move envelope point right"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveUp, 1823, kcVisible, kcNoDummy, _T("Move envelope point up"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveDown, 1824, kcVisible, kcNoDummy, _T("Move envelope point down"));
	DefineKeyCommand(kcInstrumentEnvelopePointPrev, 1825, kcVisible, kcNoDummy, _T("Select previous envelope point"));
	DefineKeyCommand(kcInstrumentEnvelopePointNext, 1826, kcVisible, kcNoDummy, _T("Select next envelope point"));
	DefineKeyCommand(kcInstrumentEnvelopePointInsert, 1827, kcVisible, kcNoDummy, _T("Insert envelope point"));
	DefineKeyCommand(kcInstrumentEnvelopePointRemove, 1828, kcVisible, kcNoDummy, _T("Remove envelope point"));
	DefineKeyCommand(kcInstrumentEnvelopeSetLoopStart, 1829, kcVisible, kcNoDummy, _T("Set loop start"));
	DefineKeyCommand(kcInstrumentEnvelopeSetLoopEnd, 1830, kcVisible, kcNoDummy, _T("Set loop end"));
	DefineKeyCommand(kcInstrumentEnvelopeSetSustainLoopStart, 1831, kcVisible, kcNoDummy, _T("Set sustain loop start"));
	DefineKeyCommand(kcInstrumentEnvelopeSetSustainLoopEnd, 1832, kcVisible, kcNoDummy, _T("Set sustain loop end"));
	DefineKeyCommand(kcInstrumentEnvelopeToggleReleaseNode, 1833, kcVisible, kcNoDummy, _T("Toggle release node"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveUp8, 1834, kcVisible, kcNoDummy, _T("Move envelope point up (big step)"));
	DefineKeyCommand(kcInstrumentEnvelopePointMoveDown8, 1835, kcVisible, kcNoDummy, _T("Move envelope point down (big step)"));
	DefineKeyCommand(kcPatternEditPCNotePlugin, 1836, kcVisible, kcNoDummy, _T("Edit plugin assigned to PC note"));
	DefineKeyCommand(kcInstrumentEnvelopeZoomIn, 1837, kcVisible, kcNoDummy, _T("Zoom In"));
	DefineKeyCommand(kcInstrumentEnvelopeZoomOut, 1838, kcVisible, kcNoDummy, _T("Zoom Out"));
	DefineKeyCommand(kcVSTGUIToggleRecordParams, 1839, kcVisible, kcNoDummy, _T("Toggle parameter recording"));
	DefineKeyCommand(kcVSTGUIToggleSendKeysToPlug, 1840, kcVisible, kcNoDummy, _T("Pass key presses to plugin"));
	DefineKeyCommand(kcVSTGUIBypassPlug, 1841, kcVisible, kcNoDummy, _T("Bypass plugin"));
	DefineKeyCommand(kcInsNoteMapTransposeDown, 1842, kcVisible, kcNoDummy, _T("Transpose -1 (note map)"));
	DefineKeyCommand(kcInsNoteMapTransposeUp, 1843, kcVisible, kcNoDummy, _T("Transpose +1 (note map)"));
	DefineKeyCommand(kcInsNoteMapTransposeOctDown, 1844, kcVisible, kcNoDummy, _T("Transpose -12 (note map)"));
	DefineKeyCommand(kcInsNoteMapTransposeOctUp, 1845, kcVisible, kcNoDummy, _T("Transpose +12 (note map)"));
	DefineKeyCommand(kcInsNoteMapCopyCurrentNote, 1846, kcVisible, kcNoDummy, _T("Map all notes to selected note"));
	DefineKeyCommand(kcInsNoteMapCopyCurrentSample, 1847, kcVisible, kcNoDummy, _T("Map all notes to selected sample"));
	DefineKeyCommand(kcInsNoteMapReset, 1848, kcVisible, kcNoDummy, _T("Reset note mapping"));
	DefineKeyCommand(kcInsNoteMapEditSample, 1849, kcVisible, kcNoDummy, _T("Edit current sample"));
	DefineKeyCommand(kcInsNoteMapEditSampleMap, 1850, kcVisible, kcNoDummy, _T("Edit sample map"));
	DefineKeyCommand(kcInstrumentCtrlDuplicate, 1851, kcVisible, kcNoDummy, _T("Duplicate instrument"));
	DefineKeyCommand(kcPanic, 1852, kcVisible, kcNoDummy, _T("Panic"));
	DefineKeyCommand(kcOrderlistPatIgnore, 1853, kcVisible, kcNoDummy, _T("Ignore (+++) Index"));
	DefineKeyCommand(kcOrderlistPatInvalid, 1854, kcVisible, kcNoDummy, _T("Invalid (---) Index"));
	DefineKeyCommand(kcViewEditHistory, 1855, kcVisible, kcNoDummy, _T("View Edit History"));
	DefineKeyCommand(kcSampleQuickFade, 1856, kcVisible, kcNoDummy, _T("Quick fade"));
	DefineKeyCommand(kcSampleXFade, 1857, kcVisible, kcNoDummy, _T("Crossfade sample loop"));
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
//-----------------------------------------

//-------------------------------------------------------
// Command Manipulation
//-------------------------------------------------------

CString CCommandSet::Add(KeyCombination kc, CommandID cmd, bool overwrite)
{
	return Add(kc, cmd, overwrite, -1);
}
CString CCommandSet::Add(KeyCombination kc, CommandID cmd, bool overwrite, int pos)
{
	CString report= "";
	
	KeyCombination curKc;
	//Avoid duplicate
	for (int k=0; k<commands[cmd].kcList.GetSize(); k++) 
	{
		curKc=commands[cmd].kcList[k];
		if (curKc==kc)
		{
			//cm'ed out for perf
			//Log("Not adding key:%d; ctx:%d; mod:%d event %d - Duplicate!\n", kc.code, kc.ctx, kc.mod, kc.event); 
			return "";
		}
	}
	//Check that this keycombination isn't already assigned (in this context), except for dummy keys
	for (int curCmd=0; curCmd<kcNumCommands; curCmd++)	
	{ //search all commands
		if (IsDummyCommand(cmd))    // no need to search if we are adding a dummy key
			break;								 
		if (IsDummyCommand((CommandID)curCmd)) //no need to check against a dummy key
			continue;
		for (int k=0; k<commands[curCmd].kcList.GetSize(); k++)
		{ //search all keys for curCommand
			curKc=commands[curCmd].kcList[k];
			bool crossContext=false;
			if (KeyCombinationConflict(curKc, kc, crossContext))
			{
				if (!overwrite)
				{
					//cm'ed out for perf
					//Log("Not adding key: already exists and overwrite disabled\n");
					return "";
				}
				else
				{
					if (crossContext) { 
						report += "Warning! the following commands may conflict:\r\n   >" + GetCommandText((CommandID)curCmd) + " in " + GetContextText(curKc.ctx) + "\r\n   >" + GetCommandText((CommandID)cmd) + " in " + GetContextText(kc.ctx) + "\r\n\r\n";
						Log("%s",report);
					} else {
						Remove(curKc, (CommandID)curCmd);
						report += "Removed due to conflict in same context:\r\n   >" + GetCommandText((CommandID)curCmd) + " in " + GetContextText(curKc.ctx) + "\r\n\r\n";
						Log("%s",report);
					}
				}
			}
		}
	}
	
	if (pos>=0)
		commands[cmd].kcList.InsertAt(pos, kc);
	else
		commands[cmd].kcList.Add(kc);

	//enfore rules on CommandSet
	report+=EnforceAll(kc, cmd, true);
	return report;
}

bool CCommandSet::IsDummyCommand(CommandID cmd)
{	// e.g. Chord modifier is a dummy command, which serves only to automatically 
    // generate a set of keycombinations for chords (I'm not proud of this design).
	return commands[cmd].isDummy;
}


CString CCommandSet::Remove(int pos, CommandID cmd)
{
	if (pos>=0 && pos<commands[cmd].kcList.GetSize())
	{
		return Remove(commands[cmd].kcList[pos], cmd);
	}

	Log("Failed to remove a key: keychoice out of range.\n"); 
	return "";
}

CString CCommandSet::Remove(KeyCombination kc, CommandID cmd)
{
	//find kc in commands[cmd].kcList
	int index=-1;
	for (index=0; index<commands[cmd].kcList.GetSize(); index++)
	{
		if (kc==commands[cmd].kcList[index])
			break;
	}
	if (index>=0 && index<commands[cmd].kcList.GetSize())
	{
		commands[cmd].kcList.RemoveAt(index);
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
{
	//World's biggest, most confusing method. :)
	//Needs refactoring. Maybe make lots of Rule subclasses, each with their own Enforce() method?
	//bool removing = !adding; //for attempt to salvage readability.. 
	KeyCombination curKc;	// for looping through key combinations
	KeyCombination newKc;	// for adding new key combinations
	CString report="";

	if (enforceRule[krAllowNavigationWithSelection])
	{//------------------------------------------------------------
		// When we get a new navigation command key, we need to 
		// make sure this navigation will work when any selection key is pressed
		if (inCmd>=kcStartPatNavigation && inCmd<=kcEndPatNavigation)
		{//Check that it is a nav cmd
			CommandID cmdNavSelection = (CommandID)(kcStartPatNavigationSelect + (inCmd-kcStartPatNavigation));
			for (int kSel=0; kSel<commands[kcSelect].kcList.GetSize(); kSel++)
			{//for all selection modifiers
				curKc=commands[kcSelect].kcList[kSel];
				newKc=inKc;
				newKc.mod|=curKc.mod;	//Add selection modifier's modifiers to this command
				if (adding)
				{
					Log("Enforcing rule krAllowNavigationWithSelection - adding key:%d with modifier:%d to command: %d\n", kSel, newKc.mod, cmdNavSelection);
					Add(newKc, cmdNavSelection, false);
				}
				else
				{					
					Log("Enforcing rule krAllowNavigationWithSelection - removing key:%d with modifier:%d to command: %d\n", kSel, newKc.mod, cmdNavSelection); 
					Remove(newKc, cmdNavSelection);
				}
			}
		}
		// Same applies for orderlist navigation
		else if (inCmd>=kcStartOrderlistNavigation && inCmd<=kcEndOrderlistNavigation)
		{//Check that it is a nav cmd
			CommandID cmdNavSelection = (CommandID)(kcStartOrderlistNavigationSelect+ (inCmd-kcStartOrderlistNavigation));
			for (int kSel=0; kSel<commands[kcSelect].kcList.GetSize(); kSel++)
			{//for all selection modifiers
				curKc=commands[kcSelect].kcList[kSel];
				newKc=inKc;
				newKc.mod|=curKc.mod;	//Add selection modifier's modifiers to this command
				if (adding)
				{
					Log("Enforcing rule krAllowNavigationWithSelection - adding key:%d with modifier:%d to command: %d\n", kSel, newKc.mod, cmdNavSelection);
					Add(newKc, cmdNavSelection, false);
				}
				else
				{					
					Log("Enforcing rule krAllowNavigationWithSelection - removing key:%d with modifier:%d to command: %d\n", kSel, newKc.mod, cmdNavSelection); 
					Remove(newKc, cmdNavSelection);
				}
			}
		}
		// When we get a new selection key, we need to make sure that 
		// all navigation commands will work with this selection key pressed
		else if (inCmd==kcSelect)
		{// check that is is a selection
			for (int curCmd=kcStartPatNavigation; curCmd<=kcEndPatNavigation; curCmd++)
			{// for all nav commands
				for (int k=0; k<commands[curCmd].kcList.GetSize(); k++)
				{// for all keys for this command
					CommandID cmdNavSelection = (CommandID)(kcStartPatNavigationSelect + (curCmd-kcStartPatNavigation));
					newKc=commands[curCmd].kcList[k]; // get all properties from the current nav cmd key
					newKc.mod|=inKc.mod;			  // and the new selection modifier
					if (adding)
					{
						Log("Enforcing rule krAllowNavigationWithSelection - adding key:%d with modifier:%d to command: %d\n", curCmd, inKc.mod, cmdNavSelection);
						Add(newKc, cmdNavSelection, false);
					}
					else
					{	
						Log("Enforcing rule krAllowNavigationWithSelection - removing key:%d with modifier:%d to command: %d\n", curCmd, inKc.mod, cmdNavSelection);				
						Remove(newKc, cmdNavSelection);
					}
				}
			} // end all nav commands
			for (int curCmd=kcStartOrderlistNavigation; curCmd<=kcEndOrderlistNavigation; curCmd++)
			{// for all nav commands
				for (int k=0; k<commands[curCmd].kcList.GetSize(); k++)
				{// for all keys for this command
					CommandID cmdNavSelection = (CommandID)(kcStartOrderlistNavigationSelect+ (curCmd-kcStartOrderlistNavigation));
					newKc=commands[curCmd].kcList[k]; // get all properties from the current nav cmd key
					newKc.mod|=inKc.mod;			  // and the new selection modifier
					if (adding)
					{
						Log("Enforcing rule krAllowNavigationWithSelection - adding key:%d with modifier:%d to command: %d\n", curCmd, inKc.mod, cmdNavSelection);
						Add(newKc, cmdNavSelection, false);
					}
					else
					{	
						Log("Enforcing rule krAllowNavigationWithSelection - removing key:%d with modifier:%d to command: %d\n", curCmd, inKc.mod, cmdNavSelection);				
						Remove(newKc, cmdNavSelection);
					}
				}
			} // end all nav commands
		}
	} // end krAllowNavigationWithSelection

	if (enforceRule[krAllowSelectionWithNavigation])
	{//-----------------------------------------------------------
		KeyCombination newKcSel;
		
		// When we get a new navigation command key, we need to ensure
		// all selection keys will work even when this new selection key is pressed
		if (inCmd>=kcStartPatNavigation && inCmd<=kcEndPatNavigation)
		{//if this is a navigation command
			for (int kSel=0; kSel<commands[kcSelect].kcList.GetSize(); kSel++)
			{//for all deselection modifiers
				newKcSel=commands[kcSelect].kcList[kSel];	// get all properties from the selection key
				newKcSel.mod|=inKc.mod;						// add modifiers from the new nav command
				if (adding)	{
					Log("Enforcing rule krAllowSelectionWithNavigation: adding  removing kcSelectWithNav and kcSelectOffWithNav\n"); 
					Add(newKcSel, kcSelectWithNav, false);
				}
				else {	
					Log("Enforcing rule krAllowSelectionWithNavigation: removing kcSelectWithNav and kcSelectOffWithNav\n"); 				
					Remove(newKcSel, kcSelectWithNav);
				}
			}
		}
		// Same for orderlist navigation
		if (inCmd>=kcStartOrderlistNavigation&& inCmd<=kcEndOrderlistNavigation)
		{//if this is a navigation command
			for (int kSel=0; kSel<commands[kcSelect].kcList.GetSize(); kSel++)
			{//for all deselection modifiers
				newKcSel=commands[kcSelect].kcList[kSel];	// get all properties from the selection key
				newKcSel.mod|=inKc.mod;						// add modifiers from the new nav command
				if (adding)	{
					Log("Enforcing rule krAllowSelectionWithNavigation: adding  removing kcSelectWithNav and kcSelectOffWithNav\n"); 
					Add(newKcSel, kcSelectWithNav, false);
				}
				else {	
					Log("Enforcing rule krAllowSelectionWithNavigation: removing kcSelectWithNav and kcSelectOffWithNav\n"); 				
					Remove(newKcSel, kcSelectWithNav);
				}
			}
		}
		// When we get a new selection key, we need to ensure it will work even when
		// any navigation key is pressed
		else if (inCmd==kcSelect)
		{
			for (int curCmd=kcStartPatNavigation; curCmd<=kcEndPatNavigation; curCmd++)
			{//for all nav commands
				for (int k=0; k<commands[curCmd].kcList.GetSize(); k++)
				{// for all keys for this command
					newKcSel=inKc; // get all properties from the selection key
					newKcSel.mod|=commands[curCmd].kcList[k].mod; //add the nav keys' modifiers
					if (adding)	{
						Log("Enforcing rule krAllowSelectionWithNavigation - adding key:%d with modifier:%d to command: %d\n", curCmd, inKc.mod, kcSelectWithNav);
						Add(newKcSel, kcSelectWithNav, false);
					}
					else {	
						Log("Enforcing rule krAllowSelectionWithNavigation - removing key:%d with modifier:%d to command: %d\n", curCmd, inKc.mod, kcSelectWithNav);				
						Remove(newKcSel, kcSelectWithNav);
					}
				}
			} // end all nav commands
			for (int curCmd=kcStartOrderlistNavigation; curCmd<=kcEndOrderlistNavigation; curCmd++)
			{//for all nav commands
				for (int k=0; k<commands[curCmd].kcList.GetSize(); k++)
				{// for all keys for this command
					newKcSel=inKc; // get all properties from the selection key
					newKcSel.mod|=commands[curCmd].kcList[k].mod; //add the nav keys' modifiers
					if (adding)	{
						Log("Enforcing rule krAllowSelectionWithNavigation - adding key:%d with modifier:%d to command: %d\n", curCmd, inKc.mod, kcSelectWithNav);
						Add(newKcSel, kcSelectWithNav, false);
					}
					else {	
						Log("Enforcing rule krAllowSelectionWithNavigation - removing key:%d with modifier:%d to command: %d\n", curCmd, inKc.mod, kcSelectWithNav);				
						Remove(newKcSel, kcSelectWithNav);
					}
				}
			} // end all nav commands
		}		

	}
	
	//if we add a selector or a copy selector, we need it to switch off when we release the key.
	if (enforceRule[krAutoSelectOff])
	{
		KeyCombination newKcDeSel;
		bool ruleApplies=true;
		CommandID cmdOff = kcNull;

		switch (inCmd)
		{
			case kcSelect:				 cmdOff=kcSelectOff;	break;
			case kcSelectWithNav:		 cmdOff=kcSelectOffWithNav;	break;	
			case kcCopySelect:			 cmdOff=kcCopySelectOff;	break;	
			case kcCopySelectWithNav:	 cmdOff=kcCopySelectOffWithNav;	break;	
			case kcSelectWithCopySelect: cmdOff=kcSelectOffWithCopySelect; break;
			case kcCopySelectWithSelect: cmdOff=kcCopySelectOffWithSelect; break;
			default: ruleApplies=false;
		}
	
		if (ruleApplies)
		{
			newKcDeSel=inKc;
			newKcDeSel.mod&=~CodeToModifier(inKc.code);		//<-- Need to get rid of right modifier!!
			newKcDeSel.event=kKeyEventUp;

			if (adding)
				Add(newKcDeSel, cmdOff, false);
			else
				Remove(newKcDeSel, cmdOff);
		}
	
	}


	// Allow combinations of copyselect and select
	if (enforceRule[krAllowSelectCopySelectCombos])
	{
		KeyCombination newKcSel, newKcCopySel;
		if (inCmd==kcSelect)
		{  //On getting a new selection key, make this selection key work with all copy selects' modifiers
   		   //On getting a new selection key, make all copyselects work with this key's modifiers
			for (int k=0; k<commands[kcCopySelect].kcList.GetSize(); k++)
			{
				newKcSel=inKc;
				newKcSel.mod|=commands[kcCopySelect].kcList[k].mod;
				newKcCopySel=commands[kcCopySelect].kcList[k];
				newKcCopySel.mod|=inKc.mod;
				Log("Enforcing rule krAllowSelectCopySelectCombos\n"); 
                if (adding)
				{
					Add(newKcSel, kcSelectWithCopySelect, false);
					Add(newKcCopySel, kcCopySelectWithSelect, false);
				}
				else
				{
					Remove(newKcSel, kcSelectWithCopySelect);
					Remove(newKcCopySel, kcCopySelectWithSelect);
				}
			}
		}
		if (inCmd==kcCopySelect)
		{  //On getting a new copyselection key, make this copyselection key work with all selects' modifiers
   		   //On getting a new copyselection key, make all selects work with this key's modifiers
			for (int k=0; k<commands[kcSelect].kcList.GetSize(); k++)
			{
				newKcSel=commands[kcSelect].kcList[k];
				newKcSel.mod|=inKc.mod;
				newKcCopySel=inKc;
				newKcCopySel.mod|=commands[kcSelect].kcList[k].mod;
				Log("Enforcing rule krAllowSelectCopySelectCombos\n"); 
                if (adding)
				{
					Add(newKcSel, kcSelectWithCopySelect, false);
					Add(newKcCopySel, kcCopySelectWithSelect, false);
				}
				else
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
			for (int k=0; k<commands[kcChordModifier].kcList.GetSize(); k++)
			{//for all chord modifier keys
				newKc=inKc;
				newKc.mod|=commands[kcChordModifier].kcList[k].mod;
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
				for (int k=0; k<commands[curCmd].kcList.GetSize(); k++)
				{//for all keys for this note
					noteOffset = curCmd - kcVPStartNotes;
					newKc=commands[curCmd].kcList[k];
					newKc.mod|=inKc.mod;
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
			newKc.event=kKeyEventUp;
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
			newKc.event=kKeyEventUp;
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
	}
	
	//# Reassign freed number keys to octaves
	if (enforceRule[krReassignDigitsToOctaves] && !adding)
	{  
		  if ( (inKc.mod == 0) &&	//no modifier
			 ( (inKc.ctx == kCtxViewPatternsNote) || (inKc.ctx == kCtxViewPatterns) ) && //note scope or pattern scope
			 ( ('0'<=inKc.code && inKc.code<='9') || (VK_NUMPAD0<=inKc.code && inKc.code<=VK_NUMPAD9) ) ) {  //is number key 
				newKc.ctx=kCtxViewPatternsNote;
				newKc.mod=0;
				newKc.event= (KeyEventType)1;
				newKc.code=inKc.code;
				int offset = ('0'<=inKc.code && inKc.code<='9') ? newKc.code-'0' : newKc.code-VK_NUMPAD0;
				Add(newKc, (CommandID)(kcSetOctave0 + (newKc.code-offset)), false);		
			 }
	}
	// Add spacing
	if (enforceRule[krAutoSpacing])
	{
		if (inCmd==kcSetSpacing && adding)
		{
			newKc.ctx=kCtxViewPatterns;
			newKc.mod=inKc.mod;
			newKc.event= kKeyEventDown;
			for (newKc.code='0'; newKc.code<='9'; newKc.code++)
			{
				Add(newKc, (CommandID)(kcSetSpacing0 + (newKc.code-'0')), false);		
			}
			for (newKc.code=VK_NUMPAD0; newKc.code<=VK_NUMPAD9; newKc.code++)
			{
				Add(newKc, (CommandID)(kcSetSpacing0 + (newKc.code-VK_NUMPAD0)), false);		
			}			
		}
		else if (!adding && (inCmd<kcSetSpacing && kcSetSpacing9<inCmd))
		{
			KeyCombination spacing;
			for (int k=0; k<commands[kcSetSpacing].kcList.GetSize(); k++)
			{
				spacing = commands[kcSetSpacing].kcList[k];
				if ((('0'<=inKc.code && inKc.code<='9')||(VK_NUMPAD0<=inKc.code && inKc.code<=VK_NUMPAD9)) && !adding)
				{
					newKc.ctx= kCtxViewPatterns;
					newKc.mod= spacing.mod;
					newKc.event= spacing.event;
					newKc.code=inKc.code;
					if ('0'<=inKc.code && inKc.code<='9')
						Add(newKc, (CommandID)(kcSetSpacing0 + inKc.code - '0'), false);
					else if (VK_NUMPAD0<=inKc.code && inKc.code<=VK_NUMPAD9)
						Add(newKc, (CommandID)(kcSetSpacing0 + inKc.code - VK_NUMPAD0), false);
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

			newKcSamp.ctx=kCtxViewSamples;
			newKcIns.ctx=kCtxViewInstruments;
			newKcTree.ctx=kCtxViewTree;
			newKcInsNoteMap.ctx=kCtxInsNoteMap;
			newKcVSTGUI.ctx=kCtxVSTGUI;

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
		}
		if (inCmd>=kcVPStartNoteStops && inCmd<=kcVPEndNoteStops)
		{
			KeyCombination newKcSamp = inKc;
			KeyCombination newKcIns  = inKc;
			KeyCombination newKcTree = inKc;
			KeyCombination newKcInsNoteMap = inKc;
			KeyCombination newKcVSTGUI = inKc;

			newKcSamp.ctx=kCtxViewSamples;
			newKcIns.ctx=kCtxViewInstruments;
			newKcTree.ctx=kCtxViewTree;
			newKcInsNoteMap.ctx=kCtxInsNoteMap;
			newKcVSTGUI.ctx=kCtxVSTGUI;

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

		}
	}
	if (enforceRule[krCheckModifiers])
	{
		const int nForcedModifiers = 4;
		CommandID forcedModifiers[nForcedModifiers] = {kcSelect, kcCopySelect, kcChordModifier, kcSetSpacing};
		CommandID curCmd;

		// for all commands that must be modifiers
		for (int i=0; i<nForcedModifiers; i++)
		{
			curCmd=forcedModifiers[i];
			
			//for all of this command's key combinations
			for (int k=0; k<commands[curCmd].kcList.GetSize(); k++)
			{
				curKc = commands[curCmd].kcList[k];
				if ((!curKc.mod) || (curKc.code!=VK_SHIFT && curKc.code!=VK_CONTROL && curKc.code!=VK_MENU && curKc.code!=0 &&
					curKc.code!=VK_LWIN && curKc.code!=VK_RWIN )) // Feature: use Windows keys as modifier keys
				{
					report+="Error! " + GetCommandText((CommandID)curCmd) + " must be a modifier (shift/ctrl/alt), but is currently " + GetKeyText(inKc.mod, inKc.code) + "\r\n";
					//replace with dummy
					commands[curCmd].kcList[k].mod=1;
					commands[curCmd].kcList[k].code=0;
					commands[curCmd].kcList[k].event=(KeyEventType)0;
					//commands[curCmd].kcList[k].ctx;
				}
			}
		
		}
	}
	if (enforceRule[krPropagateSampleManipulation])
	{
		if (inCmd>=kcStartSampleMisc && inCmd<=kcEndSampleMisc)
		{
			int newCmd;
			int offset = inCmd-kcStartSampleMisc;

			//propagate to InstrumentView
			newCmd = kcStartInstrumentMisc+offset;
			commands[newCmd].kcList.SetSize(commands[inCmd].kcList.GetSize());
			for (int k=0; k<commands[inCmd].kcList.GetSize(); k++)
			{
				commands[newCmd].kcList[k].mod = commands[inCmd].kcList[k].mod;
				commands[newCmd].kcList[k].code = commands[inCmd].kcList[k].code;
				commands[newCmd].kcList[k].event = commands[inCmd].kcList[k].event;
				commands[newCmd].kcList[k].ctx = kCtxViewInstruments;
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
	return report;
}

UINT CCommandSet::CodeToModifier(UINT code)
{
	switch(code)
	{
		case VK_SHIFT:	 return HOTKEYF_SHIFT;
		case VK_MENU:	 return HOTKEYF_ALT;
		case VK_CONTROL: return HOTKEYF_CONTROL;
		case VK_LWIN: case VK_RWIN: return HOTKEYF_EXT; // Feature: use Windows keys as modifier keys
		default:		 /*DEBUG: ASSERT(false);*/ return 0;		//can only get modifier for modifier key
	}
	
}

//-------------------------------------------------------
// Export
//-------------------------------------------------------
//Generate a keymap from a command set
void CCommandSet::GenKeyMap(KeyMap &km)
{
	KeyCombination curKc;
	CArray<KeyEventType, KeyEventType> eventTypes;
	CArray<InputTargetContext, InputTargetContext> contexts;
	
	//Clear map
	memset(km, -1, sizeof(kcNull)*KeyMapSize);

    //Copy commandlist content into map:
	for (UINT cmd=0; cmd<kcNumCommands; cmd++)
	{	
		if (IsDummyCommand((CommandID)cmd))
			continue;

		for (INT_PTR k=0; k<commands[cmd].kcList.GetSize(); k++)
		{
			contexts.RemoveAll();
			eventTypes.RemoveAll();
			curKc = commands[cmd].kcList[k];
			
			//Handle keyEventType mask.
			if (curKc.event & kKeyEventDown)
				eventTypes.Add(kKeyEventDown);
			if (curKc.event & kKeyEventUp)
				eventTypes.Add(kKeyEventUp);
			if (curKc.event & kKeyEventRepeat)
				eventTypes.Add(kKeyEventRepeat);
			//ASSERT(eventTypes.GetSize()>0);

			//Handle super-contexts (contexts that represent a set of sub contexts)
			if (curKc.ctx == kCtxViewPatterns)
			{
				contexts.Add(kCtxViewPatternsNote);
				contexts.Add(kCtxViewPatternsIns);
				contexts.Add(kCtxViewPatternsVol);
				contexts.Add(kCtxViewPatternsFX);
				contexts.Add(kCtxViewPatternsFXparam);
			}
			else if(curKc.ctx == kCtxCtrlPatterns)
			{
				contexts.Add(kCtxCtrlOrderlist);
			}
			else
			{
				contexts.Add(curKc.ctx);
			}

			//long label = 0;
			for (int cx=0; cx<contexts.GetSize(); cx++)	{
				for (int ke=0; ke<eventTypes.GetSize(); ke++) {
					km[contexts[cx]][curKc.mod][curKc.code][eventTypes[ke]] = (CommandID)cmd;
				}
			}
		}

	}
}
//-------------------------------------


DWORD CCommandSet::GetKeymapLabel(InputTargetContext ctx, UINT mod, UINT code, KeyEventType ke)
{ //Unused
	ASSERT((long)ctx<0xFF);
	ASSERT((long)mod<0xFF);
	ASSERT((long)code<0xFF);
	ASSERT((long)ke<0xFF);

	BYTE ctxCode  = (BYTE)ctx;
	BYTE modCode  = (BYTE)mod;
	BYTE codeCode = (BYTE)code;
	//BYTE keCode   = (BYTE)ke;

	DWORD label = ctxCode | (modCode<<8) | (codeCode<<16) | (ke<<24);

	return label;
}

void CCommandSet::Copy(CCommandSet *source)
{
	// copy constructors should take care of complexity (I hope)
	for (int cmd=0; cmd<commands.GetSize(); cmd++)
		commands[cmd] = source->commands[cmd];
}

KeyCombination CCommandSet::GetKey(CommandID cmd, UINT key)
{
	return commands[cmd].kcList[key];
}



int CCommandSet::GetKeyListSize(CommandID cmd)
{
	return  commands[cmd].kcList.GetSize();
}

CString CCommandSet::GetCommandText(CommandID cmd)
{
	return commands[cmd].Message;
}

bool CCommandSet::SaveFile(CString fileName, bool debug)
{ //TODO: Make C++

/* Layout:
----( Context1 Text (id) )----
ctx:UID:Description:Modifier:Key:EventMask
ctx:UID:Description:Modifier:Key:EventMask
...
----( Context2 Text (id) )----
...
*/

	FILE *outStream;
	KeyCombination kc;

	if( (outStream  = fopen( fileName, "w" )) == NULL )
	{
		AfxMessageBox(IDS_CANT_OPEN_FILE_FOR_WRITING, MB_ICONEXCLAMATION|MB_OK);
		return false;
	}
	fprintf(outStream, "//-------- OpenMPT key binding definition file  -------\n"); 
	fprintf(outStream, "//-Format is:                                                          -\n"); 	
	fprintf(outStream, "//- Context:Command ID:Modifiers:Key:KeypressEventType     //Comments  -\n"); 
	fprintf(outStream, "//----------------------------------------------------------------------\n"); 
	fprintf(outStream, "version:%u\n", KEYMAP_VERSION);

	for (int ctx=0; ctx<kCtxMaxInputContexts; ctx++)
	{
		fprintf(outStream, "\n//----( %s (%d) )------------\n", GetContextText((InputTargetContext)ctx), ctx); 

		for (int cmd=0; cmd<kcNumCommands; cmd++)
		{
			for (int k=0; k<GetKeyListSize((CommandID)cmd); k++)
			{
				kc = GetKey((CommandID)cmd, k);
				
				if (kc.ctx != ctx)
					continue;		//sort by context
				
				if (debug || !commands[cmd].isHidden)
				{
					fprintf(outStream, "%d:%d:%d:%d:%d\t\t//%s: %s (%s)\n", 
							ctx, commands[cmd].UID,	kc.mod, kc.code, kc.event, 
							GetCommandText((CommandID)cmd), GetKeyText(kc.mod,kc.code), GetKeyEventText(kc.event)); 
				}

			}
		}
	}

	fclose(outStream);

	return true;
}


bool CCommandSet::LoadFile(std::istream& iStrm, LPCTSTR szFilename)
{
	KeyCombination kc;
	CommandID cmd=kcNumCommands;
	char s[1024];
	CString curLine, token;
	int commentStart;
	CCommandSet *pTempCS;
	int l=0;
	int fileVersion = 0;

	pTempCS = new CCommandSet();

	int errorCount=0;
	CString errText = "";

	while(iStrm.getline(s, sizeof(s)))
	{
		//::MessageBox(NULL, s, "", MB_ICONEXCLAMATION|MB_OK);
		curLine = s;
		
		
		//Cut everything after a //
		commentStart = curLine.Find("//");
		if (commentStart>=0)
			curLine = curLine.Left(commentStart);
		curLine.Trim();	//remove whitespace
		
		if (!curLine.IsEmpty() && curLine.Compare("\n") !=0)
		{
			bool ignoreLine = false;
			
			//ctx:UID:Description:Modifier:Key:EventMask
			int spos = 0;

			//context
			token=curLine.Tokenize(":",spos);
			kc.ctx = (InputTargetContext) atoi(token);

			// this line indicates the version of this keymap file instead. (e.g. "version:1")
			if((token.Trim().CompareNoCase("version") == 0) && (spos != -1))
			{
				fileVersion = atoi(curLine.Mid(spos));
				if(fileVersion > KEYMAP_VERSION)
				{
					CString err;
					err.Format("File version is %d, but your version of OpenMPT only supports loading files up to version %d.", fileVersion, KEYMAP_VERSION);
					errText += err + "\n";
					Log(err);
				}
				spos = -1;
				ignoreLine = true;
			}

			//UID
			if (spos != -1)
			{
				token=curLine.Tokenize(":",spos);
				cmd= (CommandID) FindCmd(atoi(token));
			}

			//modifier
			if (spos != -1)
			{
				token=curLine.Tokenize(":",spos);
				kc.mod = atoi(token);
			}

			//scancode
			if (spos != -1)
			{
				token=curLine.Tokenize(":",spos);
				kc.code = atoi(token);
			}

			//event
			if (spos != -1)
			{
				token=curLine.Tokenize(":",spos);
				kc.event = (KeyEventType) atoi(token);
			}

			if(!ignoreLine)
			{
				//Error checking (TODO):
				if (cmd < 0 || cmd >= kcNumCommands || spos == -1)
				{
					errorCount++;
					CString err;
					if (errorCount < 10)
					{
						if(spos == -1)
						{
							err.Format("Line %d was not understood.", l);
						} else
						{
							err.Format("Line %d contained an unknown command.", l);
						}
						errText += err + "\n";
						Log(err);
					} else if (errorCount == 10)
					{
						err = "Too many errors detected, not reporting any more.";
						errText += err + "\n";
						Log(err);
					}
				}
				else
				{
					pTempCS->Add(kc, cmd, true);
				}
			}

		}

		l++;
	}
	if(s_bShowErrorOnUnknownKeybinding && !errText.IsEmpty())
	{
		CString err;
		err.Format("The following problems have been encountered while trying to load the key binding file %s:\n", szFilename);
		err += errText;
		::MessageBox(NULL, err, "", MB_ICONEXCLAMATION|MB_OK);
	}

	if(fileVersion < KEYMAP_VERSION) UpgradeKeymap(pTempCS, fileVersion);

	Copy(pTempCS);
	delete pTempCS;

	return true;
}

bool CCommandSet::LoadFile(CString fileName)
{
	std::ifstream fin(fileName);
	if (fin.fail())
	{
		CString strMsg;
		AfxFormatString1(strMsg, IDS_CANT_OPEN_KEYBINDING_FILE, fileName);
		AfxMessageBox(strMsg, MB_ICONEXCLAMATION|MB_OK);
		return false;
	}
	else
		return LoadFile(fin, fileName);
}


// Fix outdated keymap files
void CCommandSet::UpgradeKeymap(CCommandSet *pCommands, int oldVersion)
//---------------------------------------------------------------------
{
	KeyCombination kc;

	// no orderlist context
	if(oldVersion == 0)
	{
		kc.ctx = kCtxCtrlOrderlist;
		kc.event = (KeyEventType) (kKeyEventDown | kKeyEventRepeat);
		kc.mod = 0;

		kc.code = VK_DELETE;
		pCommands->Add(kc, kcOrderlistEditDelete, false);

		kc.code = VK_INSERT;
		pCommands->Add(kc, kcOrderlistEditInsert, false);

		kc.code = VK_RETURN;
		pCommands->Add(kc, kcOrderlistEditPattern, false);

		kc.code = VK_TAB;
		pCommands->Add(kc, kcOrderlistSwitchToPatternView, false);

		kc.code = VK_LEFT;
		pCommands->Add(kc, kcOrderlistNavigateLeft, false);
		kc.code = VK_UP;
		pCommands->Add(kc, kcOrderlistNavigateLeft, false);

		kc.code = VK_RIGHT;
		pCommands->Add(kc, kcOrderlistNavigateRight, false);
		kc.code = VK_DOWN;
		pCommands->Add(kc, kcOrderlistNavigateRight, false);

		kc.code = VK_HOME;
		pCommands->Add(kc, kcOrderlistNavigateFirst, false);

		kc.code = VK_END;
		pCommands->Add(kc, kcOrderlistNavigateLast, false);

		kc.code = VK_ADD;
		pCommands->Add(kc, kcOrderlistPatPlus, false);
		kc.code = VK_OEM_PLUS;
		pCommands->Add(kc, kcOrderlistPatPlus, false);

		kc.code = VK_SUBTRACT;
		pCommands->Add(kc, kcOrderlistPatMinus, false);
		kc.code = VK_OEM_MINUS;
		pCommands->Add(kc, kcOrderlistPatMinus, false);

		kc.code = '0';
		pCommands->Add(kc, kcOrderlistPat0, false);
		kc.code = VK_NUMPAD0;
		pCommands->Add(kc, kcOrderlistPat0, false);

		kc.code = '1';
		pCommands->Add(kc, kcOrderlistPat1, false);
		kc.code = VK_NUMPAD1;
		pCommands->Add(kc, kcOrderlistPat1, false);

		kc.code = '2';
		pCommands->Add(kc, kcOrderlistPat2, false);
		kc.code = VK_NUMPAD2;
		pCommands->Add(kc, kcOrderlistPat2, false);

		kc.code = '3';
		pCommands->Add(kc, kcOrderlistPat3, false);
		kc.code = VK_NUMPAD3;
		pCommands->Add(kc, kcOrderlistPat3, false);

		kc.code = '4';
		pCommands->Add(kc, kcOrderlistPat4, false);
		kc.code = VK_NUMPAD4;
		pCommands->Add(kc, kcOrderlistPat4, false);

		kc.code = '5';
		pCommands->Add(kc, kcOrderlistPat5, false);
		kc.code = VK_NUMPAD5;
		pCommands->Add(kc, kcOrderlistPat5, false);

		kc.code = '6';
		pCommands->Add(kc, kcOrderlistPat6, false);
		kc.code = VK_NUMPAD6;
		pCommands->Add(kc, kcOrderlistPat6, false);

		kc.code = '7';
		pCommands->Add(kc, kcOrderlistPat7, false);
		kc.code = VK_NUMPAD7;
		pCommands->Add(kc, kcOrderlistPat7, false);

		kc.code = '8';
		pCommands->Add(kc, kcOrderlistPat8, false);
		kc.code = VK_NUMPAD8;
		pCommands->Add(kc, kcOrderlistPat8, false);

		kc.code = '9';
		pCommands->Add(kc, kcOrderlistPat9, false);
		kc.code = VK_NUMPAD9;
		pCommands->Add(kc, kcOrderlistPat9, false);

	}
}


//Could do better search algo but this is not perf critical.
int CCommandSet::FindCmd(int uid)
{
	for (int i=0; i<kcNumCommands; i++)
	{
		if (commands[i].UID == static_cast<UINT>(uid))
			return i;
	}

	return -1;
}

CString CCommandSet::GetContextText(InputTargetContext ctx)
{
	switch(ctx)
	{
	
		case kCtxAllContexts:			return "Global Context";
		case kCtxViewGeneral:			return "General Context [bottom]";
		case kCtxViewPatterns:			return "Pattern Context [bottom]";
		case kCtxViewPatternsNote:		return "Pattern Context [bottom] - Note Col";
		case kCtxViewPatternsIns:		return "Pattern Context [bottom] - Ins Col";
		case kCtxViewPatternsVol:		return "Pattern Context [bottom] - Vol Col";
		case kCtxViewPatternsFX:		return "Pattern Context [bottom] - FX Col";
		case kCtxViewPatternsFXparam:	return "Pattern Context [bottom] - Param Col";
		case kCtxViewSamples:			return "Sample Context [bottom]";
		case kCtxViewInstruments:		return "Instrument Context [bottom]";
		case kCtxViewComments:			return "Comments Context [bottom]";
		case kCtxCtrlGeneral:			return "General Context [top]";
		case kCtxCtrlPatterns:			return "Pattern Context [top]";
		case kCtxCtrlSamples:			return "Sample Context [top]";
		case kCtxCtrlInstruments:		return "Instrument Context [top]";
		case kCtxCtrlComments:			return "Comments Context [top]";
		case kCtxCtrlOrderlist:			return "Orderlist";
		case kCtxVSTGUI:				return "Plugin GUI Context";
	    case kCtxUnknownContext:
		default:						return "Unknown Context";
	}
};
CString CCommandSet::GetKeyEventText(KeyEventType ke)
{
	CString text="";

	bool first = true;
	if (ke & kKeyEventDown)
	{
		first=false;
		text.Append("KeyDown");
	}
	if (ke & kKeyEventRepeat)
	{
		if (!first) text.Append("|");
		text.Append("KeyHold");
		first=false;
	}
	if (ke & kKeyEventUp)
	{
		if (!first) text.Append("|");
		text.Append("KeyUp");
	}

	return text;
}

CString CCommandSet::GetModifierText(UINT mod)
{
	CString text = "";
	if (mod & HOTKEYF_SHIFT) text.Append("Shift+");
	if (mod & HOTKEYF_CONTROL) text.Append("Ctrl+");
	if (mod & HOTKEYF_ALT) text.Append("Alt+");
	if (mod & HOTKEYF_EXT) text.Append("Win+"); // Feature: use Windows keys as modifier keys
	return text;
}

CString CCommandSet::GetKeyText(UINT mod, UINT code)
{
	CString keyText;
	keyText=GetModifierText(mod);
	keyText.Append(CHotKeyCtrl::GetKeyName(code, IsExtended(code)));
	//HACK:
	if (keyText == "Ctrl+CTRL")		keyText="Ctrl";
	if (keyText == "Alt+ALT")		keyText="Alt";
	if (keyText == "Shift+SHIFT")	keyText="Shift";

	return keyText;
}

CString CCommandSet::GetKeyTextFromCommand(CommandID c, UINT key)
{
	if ( static_cast<INT_PTR>(key) < commands[c].kcList.GetSize())
		return GetKeyText(commands[c].kcList[0].mod, commands[c].kcList[0].code);
	else 
		return "";
}

bool CCommandSet::isHidden(UINT c)
{
	return commands[c].isHidden;
}





//-------------------------------------------------------
// Quick Changes - modify many commands with one call.
//-------------------------------------------------------
	
bool CCommandSet::QuickChange_NotesRepeat()
//-----------------------------------------
{
	KeyCombination kc;
	int choices;
	for (CommandID cmd=kcVPStartNotes; cmd<=kcVPEndNotes; cmd=(CommandID)(cmd+1))		//for all notes
	{
		choices = GetKeyListSize(cmd);
		for (int p=0; p<choices; p++)							//for all choices
		{
			kc = GetKey(cmd, p);
			kc.event = (KeyEventType)((UINT)kc.event | (UINT)kKeyEventRepeat);
			//Remove(p, cmd);		//out with the old
			Add(kc, cmd, true, p);		//in with the new
		}
	}
	return true;
}

bool CCommandSet::QuickChange_NoNotesRepeat()
//-----------------------------------------
{
	KeyCombination kc;
	int choices;
	for (CommandID cmd=kcVPStartNotes; cmd<=kcVPEndNotes; cmd=(CommandID)(cmd+1))		//for all notes
	{
		choices = GetKeyListSize(cmd);
		for (int p=0; p<choices; p++)							//for all choices
		{
			kc = GetKey(cmd, p);
			kc.event = (KeyEventType)((UINT)kc.event & ~(UINT)kKeyEventRepeat);
			//Remove(p, cmd);		//out with the old
			Add(kc, cmd, true, p);		//in with the new
		}
	}
	return true;
}

bool CCommandSet::QuickChange_SetEffectsXM()
//-----------------------------------------
{
	return QuickChange_SetEffects("0123456789abcdr?fte???ghk?yxplz");
}

bool CCommandSet::QuickChange_SetEffectsIT()
//-----------------------------------------
{
	return QuickChange_SetEffects("jfeghlkrxodb?cqati?smnvw?uy?p?z");

}

bool CCommandSet::QuickChange_SetEffects(char comList[kcSetFXEnd-kcSetFXStart])
//-----------------------------------------
{
	int choices=0;
	KeyCombination kc;
	kc.ctx = kCtxViewPatternsFX;
	kc.event = kKeyEventDown;
	
	for (CommandID cmd=kcFixedFXStart; cmd<=kcFixedFXend; cmd=(CommandID)(cmd+1))
	{
		choices = GetKeyListSize(cmd);				//Remove all old choices
		for (int p=choices; p>=0; --p)
			Remove(p, cmd);
		
		if (comList[cmd-kcSetFXStart] != '?')	//? is used an non existant command.
		{
			SHORT codeNmod = VkKeyScanEx(comList[cmd-kcFixedFXStart], GetKeyboardLayout(0));
			kc.code = LOBYTE(codeNmod);			
			kc.mod = HIBYTE(codeNmod) & 0x07;	//We're only interest in the bottom 3 bits.
			Add(kc, cmd, true);
			
			if (kc.code >= '0' && kc.code <= '9')		//for numbers, ensure numpad works too
			{
				kc.code = VK_NUMPAD0 + (kc.code-'0');
				Add(kc, cmd, true);
			}
		}
	}
	return true;
}

// Stupid MFC crap: for some reason VK code isn't enough to get correct string with GetKeyName.
// We also need to figure out the correct "extended" bit. 
bool CCommandSet::IsExtended(UINT code)
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


void CCommandSet::GetParentContexts(InputTargetContext child, CArray<InputTargetContext, InputTargetContext> parentList)
{
	UNREFERENCED_PARAMETER(child);
	//parentList.RemoveAll();

	//for (InputTargetContext parent; parent<kCtxMaxInputContexts; parent++) {
	//	if (m_isParentContext[child][parent]) {
	//		parentList.Add(parent);
	//	}
	//}
}

void CCommandSet::GetChildContexts(InputTargetContext parent, CArray<InputTargetContext, InputTargetContext> childList)
{
	UNREFERENCED_PARAMETER(parent);
	//childList.RemoveAll();

	//for (InputTargetContext child; child<kCtxMaxInputContexts; child++) {
	//	if (m_isParentContext[child][parent]) {
	//		childList.Add(child);
	//	}
	//}
}


void CCommandSet::SetupContextHierarchy() 
{
//	m_isParentContext.SetSize(kCtxMaxInputContexts);
	
	for (UINT nCtx=0; nCtx<kCtxMaxInputContexts; nCtx++) {
//		m_isParentContext[nCtx].SetSize(kCtxMaxInputContexts);
		for (UINT nCtx2=0; nCtx2<kCtxMaxInputContexts; nCtx2++) {
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

	m_isParentContext[kCtxViewPatternsNote][kCtxViewPatterns] = true;
	m_isParentContext[kCtxViewPatternsIns][kCtxViewPatterns] = true;
	m_isParentContext[kCtxViewPatternsVol][kCtxViewPatterns] = true;
	m_isParentContext[kCtxViewPatternsFX][kCtxViewPatterns] = true;
	m_isParentContext[kCtxViewPatternsFXparam][kCtxViewPatterns] = true;
	m_isParentContext[kCtxCtrlOrderlist][kCtxCtrlPatterns] = true;

}

bool CCommandSet::KeyCombinationConflict(KeyCombination kc1, KeyCombination kc2, bool &crossCxtConflict) 
{
	bool modConflict     = (kc1.mod==kc2.mod);
	bool codeConflict    = (kc1.code==kc2.code);
	bool eventConflict   = ((kc1.event&kc2.event)!=0);
	bool ctxConflict     = (kc1.ctx == kc2.ctx);
	crossCxtConflict     = m_isParentContext[kc1.ctx][kc2.ctx] || m_isParentContext[kc2.ctx][kc1.ctx];
		

	bool conflict = modConflict && codeConflict && eventConflict && 
		(ctxConflict || crossCxtConflict);

    return conflict;
}

//end rewbs.customKeys
