//rewbs.customKeys

#include "stdafx.h"
#include ".\commandset.h"
#include <stdio.h>
#include <stdlib.h>

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
//Get command descriptions etc.. loaded up.
void CCommandSet::SetupCommands()
{	//TODO: make this hideous list a bit nicer, with a constructor or somthing.
	//NOTE: isHidden implies automatically set, since the user will not be able to see it.

	commands[kcPatternRecord].UID = 1001;
	commands[kcPatternRecord].isHidden = false;
	commands[kcPatternRecord].isDummy = false;
	commands[kcPatternRecord].Message = "Enable recording";

	commands[kcPatternPlayRow].UID = 1002;
	commands[kcPatternPlayRow].isHidden = false;
	commands[kcPatternPlayRow].isDummy = false;
	commands[kcPatternPlayRow].Message = "Play row";

	commands[kcCursorCopy].UID = 1003;
	commands[kcCursorCopy].isHidden = false;
	commands[kcCursorCopy].isDummy = false;
	commands[kcCursorCopy].Message = "Quick copy";

	commands[kcCursorPaste].UID = 1004;
	commands[kcCursorPaste].isHidden = false;
	commands[kcCursorPaste].isDummy = false;
	commands[kcCursorPaste].Message = "Quick paste";

	commands[kcChannelMute].UID = 1005;
	commands[kcChannelMute].isHidden = false;
	commands[kcChannelMute].isDummy = false;
	commands[kcChannelMute].Message = "Mute current channel";

	commands[kcChannelSolo].UID = 1006;
	commands[kcChannelSolo].isHidden = false;
	commands[kcChannelSolo].isDummy = false;
	commands[kcChannelSolo].Message = "Solo current channel";

	commands[kcTransposeUp].UID = 1007;
	commands[kcTransposeUp].isHidden = false;
	commands[kcTransposeUp].isDummy = false;
	commands[kcTransposeUp].Message = "Transpose +1";

	commands[kcTransposeDown].UID = 1008;
	commands[kcTransposeDown].isHidden = false;
	commands[kcTransposeDown].isDummy = false;
	commands[kcTransposeDown].Message = "Transpose -1";

	commands[kcTransposeOctUp].UID = 1009;
	commands[kcTransposeOctUp].isHidden = false;
	commands[kcTransposeOctUp].isDummy = false;
	commands[kcTransposeOctUp].Message = "Transpose +12";

	commands[kcTransposeOctDown].UID = 1010;
	commands[kcTransposeOctDown].isHidden = false;
	commands[kcTransposeOctDown].isDummy = false;
	commands[kcTransposeOctDown].Message = "Transpose -12";

	commands[kcSelectColumn].UID = 1011;
	commands[kcSelectColumn].isHidden = false;
	commands[kcSelectColumn].isDummy = false;
	commands[kcSelectColumn].Message = "Select channel / Select all";

	commands[kcPatternAmplify].UID = 1012;
	commands[kcPatternAmplify].isHidden = false;
	commands[kcPatternAmplify].isDummy = false;
	commands[kcPatternAmplify].Message = "Amplify selection";

	commands[kcPatternSetInstrument].UID = 1013;
	commands[kcPatternSetInstrument].isHidden = false;
	commands[kcPatternSetInstrument].isDummy = false;
	commands[kcPatternSetInstrument].Message = "Apply current instrument";

	commands[kcPatternInterpolateVol].UID = 1014;
	commands[kcPatternInterpolateVol].isHidden = false;
	commands[kcPatternInterpolateVol].isDummy = false;
	commands[kcPatternInterpolateVol].Message = "Interpolate volume";

	commands[kcPatternInterpolateEffect].UID = 1015;
	commands[kcPatternInterpolateEffect].isHidden = false;
	commands[kcPatternInterpolateEffect].isDummy = false;
	commands[kcPatternInterpolateEffect].Message = "Interpolate effect";

	commands[kcPatternVisualizeEffect].UID = 1016;
	commands[kcPatternVisualizeEffect].isHidden = false;
	commands[kcPatternVisualizeEffect].isDummy = false;
	commands[kcPatternVisualizeEffect].Message = "Open effect visualizer";

	commands[kcPatternJumpDownh1].UID = 1017;
	commands[kcPatternJumpDownh1].isHidden = false;
	commands[kcPatternJumpDownh1].isDummy = false;
	commands[kcPatternJumpDownh1].Message = "Jump down by measure";

	commands[kcPatternJumpUph1].UID = 1018;
	commands[kcPatternJumpUph1].isHidden = false;
	commands[kcPatternJumpUph1].isDummy = false;
	commands[kcPatternJumpUph1].Message = "Jump up by measure";

	commands[kcPatternSnapDownh1].UID = 1019;
	commands[kcPatternSnapDownh1].isHidden = false;
	commands[kcPatternSnapDownh1].isDummy = false;
	commands[kcPatternSnapDownh1].Message = "Snap down to measure";

	commands[kcPatternSnapUph1].UID = 1020;
	commands[kcPatternSnapUph1].isHidden = false;
	commands[kcPatternSnapUph1].isDummy = false;
	commands[kcPatternSnapUph1].Message = "Snap up to measure";

	commands[kcViewGeneral].UID = 1021;
	commands[kcViewGeneral].isHidden = false;
	commands[kcViewGeneral].isDummy = false;
	commands[kcViewGeneral].Message = "View General";

	commands[kcViewPattern].UID = 1022;
	commands[kcViewPattern].isHidden = false;
	commands[kcViewPattern].isDummy = false;
	commands[kcViewPattern].Message = "View Pattern";

	commands[kcViewSamples].UID = 1023;
	commands[kcViewSamples].isHidden = false;
	commands[kcViewSamples].isDummy = false;
	commands[kcViewSamples].Message = "View Samples";

	commands[kcViewInstruments].UID = 1024;
	commands[kcViewInstruments].isHidden = false;
	commands[kcViewInstruments].isDummy = false;
	commands[kcViewInstruments].Message = "View Instruments";

	commands[kcViewComments].UID = 1025;
	commands[kcViewComments].isHidden = false;
	commands[kcViewComments].isDummy = false;
	commands[kcViewComments].Message = "View Comments";

	commands[kcPlayPatternFromCursor].UID = 1026;
	commands[kcPlayPatternFromCursor].isHidden = false;
	commands[kcPlayPatternFromCursor].isDummy = false;
	commands[kcPlayPatternFromCursor].Message = "Play pattern from cursor";

	commands[kcPlayPatternFromStart].UID = 1027;
	commands[kcPlayPatternFromStart].isHidden = false;
	commands[kcPlayPatternFromStart].isDummy = false;
	commands[kcPlayPatternFromStart].Message = "Play pattern from start";

	commands[kcPlaySongFromCursor].UID = 1028;
	commands[kcPlaySongFromCursor].isHidden = false;
	commands[kcPlaySongFromCursor].isDummy = false;
	commands[kcPlaySongFromCursor].Message = "Play song from cursor";

	commands[kcPlaySongFromStart].UID = 1029;
	commands[kcPlaySongFromStart].isHidden = false;
	commands[kcPlaySongFromStart].isDummy = false;
	commands[kcPlaySongFromStart].Message = "Play song from start";

	commands[kcPlayPauseSong].UID = 1030;
	commands[kcPlayPauseSong].isHidden = false;
	commands[kcPlayPauseSong].isDummy = false;
	commands[kcPlayPauseSong].Message = "Play song/Pause song";

	commands[kcPauseSong].UID = 1031;
	commands[kcPauseSong].isHidden = false;
	commands[kcPauseSong].isDummy = false;
	commands[kcPauseSong].Message = "Pause song";

	commands[kcPrevInstrument].UID = 1032;
	commands[kcPrevInstrument].isHidden = false;
	commands[kcPrevInstrument].isDummy = false;
	commands[kcPrevInstrument].Message = "Previous instrument";

	commands[kcNextInstrument].UID = 1033;
	commands[kcNextInstrument].isHidden = false;
	commands[kcNextInstrument].isDummy = false;
	commands[kcNextInstrument].Message = "Next instrument";

	commands[kcPrevOrder].UID = 1034;
	commands[kcPrevOrder].isHidden = false;
	commands[kcPrevOrder].isDummy = false;
	commands[kcPrevOrder].Message = "Previous order";

	commands[kcNextOrder].UID = 1035;
	commands[kcNextOrder].isHidden = false;
	commands[kcNextOrder].isDummy = false;
	commands[kcNextOrder].Message = "Next order";

	commands[kcPrevOctave].UID = 1036;
	commands[kcPrevOctave].isHidden = false;
	commands[kcPrevOctave].isDummy = false;
	commands[kcPrevOctave].Message = "Previous octave";

	commands[kcNextOctave].UID = 1037;
	commands[kcNextOctave].isHidden = false;
	commands[kcNextOctave].isDummy = false;
	commands[kcNextOctave].Message = "Next octave";

	commands[kcNavigateDown].UID = 1038;
	commands[kcNavigateDown].isHidden = false;
	commands[kcNavigateDown].isDummy = false;
	commands[kcNavigateDown].Message = "Navigate down by 1 row";

	commands[kcNavigateUp].UID = 1039;
	commands[kcNavigateUp].isHidden = false;
	commands[kcNavigateUp].isDummy = false;
	commands[kcNavigateUp].Message = "Navigate up by 1 row";

	commands[kcNavigateLeft].UID = 1040;
	commands[kcNavigateLeft].isHidden = false;
	commands[kcNavigateLeft].isDummy = false;
	commands[kcNavigateLeft].Message = "Navigate left";

	commands[kcNavigateRight].UID = 1041;
	commands[kcNavigateRight].isHidden = false;
	commands[kcNavigateRight].isDummy = false;
	commands[kcNavigateRight].Message = "Navigate right";

	commands[kcNavigateNextChan].UID = 1042;
	commands[kcNavigateNextChan].isHidden = false;
	commands[kcNavigateNextChan].isDummy = false;
	commands[kcNavigateNextChan].Message = "Navigate to next channel";

	commands[kcNavigatePrevChan].UID = 1043;
	commands[kcNavigatePrevChan].isHidden = false;
	commands[kcNavigatePrevChan].isDummy = false;
	commands[kcNavigatePrevChan].Message = "Navigate to previous channel";

	commands[kcHomeHorizontal].UID = 1044;
	commands[kcHomeHorizontal].isHidden = false;
	commands[kcHomeHorizontal].isDummy = false;
	commands[kcHomeHorizontal].Message = "Go to first channel";

	commands[kcHomeVertical].UID = 1045;
	commands[kcHomeVertical].isHidden = false;
	commands[kcHomeVertical].isDummy = false;
	commands[kcHomeVertical].Message = "Go to first row";

	commands[kcHomeAbsolute].UID = 1046;
	commands[kcHomeAbsolute].isHidden = false;
	commands[kcHomeAbsolute].isDummy = false;
	commands[kcHomeAbsolute].Message = "Go to first row of first channel";

	commands[kcEndHorizontal].UID = 1047;
	commands[kcEndHorizontal].isHidden = false;
	commands[kcEndHorizontal].isDummy = false;
	commands[kcEndHorizontal].Message = "Go to last channel";

	commands[kcEndVertical].UID = 1048;
	commands[kcEndVertical].isHidden = false;
	commands[kcEndVertical].isDummy = false;
	commands[kcEndVertical].Message = "Go to last row";

	commands[kcEndAbsolute].UID = 1049;
	commands[kcEndAbsolute].isHidden = false;
	commands[kcEndAbsolute].isDummy = false;
	commands[kcEndAbsolute].Message = "Go to last row of last channel";

	commands[kcSelect].UID = 1050;
	commands[kcSelect].isHidden = false;
	commands[kcSelect].isDummy = false;
	commands[kcSelect].Message = "Selection key";

	commands[kcCopySelect].UID = 1051;
	commands[kcCopySelect].isHidden = false;
	commands[kcCopySelect].isDummy = false;
	commands[kcCopySelect].Message = "Copy select key";

	commands[kcSelectOff].UID = 1052;
	commands[kcSelectOff].isHidden = true;
	commands[kcSelectOff].isDummy = false;
	commands[kcSelectOff].Message = "Deselect";

	commands[kcCopySelectOff].UID = 1053;
	commands[kcCopySelectOff].isHidden = true;
	commands[kcCopySelectOff].isDummy = false;
	commands[kcCopySelectOff].Message = "Copy deselect key";

	commands[kcNextPattern].UID = 1054;
	commands[kcNextPattern].isHidden = false;
	commands[kcNextPattern].isDummy = false;
	commands[kcNextPattern].Message = "Next pattern";

	commands[kcPrevPattern].UID = 1055;
	commands[kcPrevPattern].isHidden = false;
	commands[kcPrevPattern].isDummy = false;
	commands[kcPrevPattern].Message = "Previous pattern";

/*	commands[kcClearSelection].UID = 1056;
	commands[kcClearSelection].isHidden = false;
	commands[kcClearSelection].isDummy = false;
	commands[kcClearSelection].Message = "Wipe selection";*/

	commands[kcClearRow].UID = 1057;
	commands[kcClearRow].isHidden = false;
	commands[kcClearRow].isDummy = false;
	commands[kcClearRow].Message = "Clear row";

	commands[kcClearField].UID = 1058;
	commands[kcClearField].isHidden = false;
	commands[kcClearField].isDummy = false;
	commands[kcClearField].Message = "Clear field";

	commands[kcClearRowStep].UID = 1059;
	commands[kcClearRowStep].isHidden = false;
	commands[kcClearRowStep].isDummy = false;
	commands[kcClearRowStep].Message = "Clear row and step";

	commands[kcClearFieldStep].UID = 1060;
	commands[kcClearFieldStep].isHidden = false;
	commands[kcClearFieldStep].isDummy = false;
	commands[kcClearFieldStep].Message = "Clear field and step";

	commands[kcDeleteRows].UID = 1061;
	commands[kcDeleteRows].isHidden = false;
	commands[kcDeleteRows].isDummy = false;
	commands[kcDeleteRows].Message = "Delete rows";

	commands[kcShowNoteProperties].UID = 1062;
	commands[kcShowNoteProperties].isHidden = false;
	commands[kcShowNoteProperties].isDummy = false;
	commands[kcShowNoteProperties].Message = "Show note properties";

	commands[kcShowEditMenu].UID = 1063;
	commands[kcShowEditMenu].isHidden = false;
	commands[kcShowEditMenu].isDummy = false;
	commands[kcShowEditMenu].Message = "Show context (right-click) menu";

	commands[kcVPNoteC_0].UID = 1064;
	commands[kcVPNoteC_0].isHidden = false;
	commands[kcVPNoteC_0].isDummy = false;
	commands[kcVPNoteC_0].Message = "Base octave C";

	commands[kcVPNoteCS0].UID = 1065;
	commands[kcVPNoteCS0].isHidden = false;
	commands[kcVPNoteCS0].isDummy = false;
	commands[kcVPNoteCS0].Message = "Base octave C#";

	commands[kcVPNoteD_0].UID = 1066;
	commands[kcVPNoteD_0].isHidden = false;
	commands[kcVPNoteD_0].isDummy = false;
	commands[kcVPNoteD_0].Message = "Base octave D";

	commands[kcVPNoteDS0].UID = 1067;
	commands[kcVPNoteDS0].isHidden = false;
	commands[kcVPNoteDS0].isDummy = false;
	commands[kcVPNoteDS0].Message = "Base octave D#";

	commands[kcVPNoteE_0].UID = 1068;
	commands[kcVPNoteE_0].isHidden = false;
	commands[kcVPNoteE_0].isDummy = false;
	commands[kcVPNoteE_0].Message = "Base octave E";

	commands[kcVPNoteF_0].UID = 1069;
	commands[kcVPNoteF_0].isHidden = false;
	commands[kcVPNoteF_0].isDummy = false;
	commands[kcVPNoteF_0].Message = "Base octave F";

	commands[kcVPNoteFS0].UID = 1070;
	commands[kcVPNoteFS0].isHidden = false;
	commands[kcVPNoteFS0].isDummy = false;
	commands[kcVPNoteFS0].Message = "Base octave F#";

	commands[kcVPNoteG_0].UID = 1071;
	commands[kcVPNoteG_0].isHidden = false;
	commands[kcVPNoteG_0].isDummy = false;
	commands[kcVPNoteG_0].Message = "Base octave G";

	commands[kcVPNoteGS0].UID = 1072;
	commands[kcVPNoteGS0].isHidden = false;
	commands[kcVPNoteGS0].isDummy = false;
	commands[kcVPNoteGS0].Message = "Base octave G#";

	commands[kcVPNoteA_1].UID = 1073;
	commands[kcVPNoteA_1].isHidden = false;
	commands[kcVPNoteA_1].isDummy = false;
	commands[kcVPNoteA_1].Message = "Base octave +1 A";

	commands[kcVPNoteAS1].UID = 1074;
	commands[kcVPNoteAS1].isHidden = false;
	commands[kcVPNoteAS1].isDummy = false;
	commands[kcVPNoteAS1].Message = "Base octave +1 A#";

	commands[kcVPNoteB_1].UID = 1075;
	commands[kcVPNoteB_1].isHidden = false;
	commands[kcVPNoteB_1].isDummy = false;
	commands[kcVPNoteB_1].Message = "Base octave +1 B";

	commands[kcVPNoteC_1].UID = 1076;
	commands[kcVPNoteC_1].isHidden = false;
	commands[kcVPNoteC_1].isDummy = false;
	commands[kcVPNoteC_1].Message = "Base octave +1 C";

	commands[kcVPNoteCS1].UID = 1077;
	commands[kcVPNoteCS1].isHidden = false;
	commands[kcVPNoteCS1].isDummy = false;
	commands[kcVPNoteCS1].Message = "Base octave +1 C#";

	commands[kcVPNoteD_1].UID = 1078;
	commands[kcVPNoteD_1].isHidden = false;
	commands[kcVPNoteD_1].isDummy = false;
	commands[kcVPNoteD_1].Message = "Base octave +1 D";

	commands[kcVPNoteDS1].UID = 1079;
	commands[kcVPNoteDS1].isHidden = false;
	commands[kcVPNoteDS1].isDummy = false;
	commands[kcVPNoteDS1].Message = "Base octave +1 D#";

	commands[kcVPNoteE_1].UID = 1080;
	commands[kcVPNoteE_1].isHidden = false;
	commands[kcVPNoteE_1].isDummy = false;
	commands[kcVPNoteE_1].Message = "Base octave +1 E";

	commands[kcVPNoteF_1].UID = 1081;
	commands[kcVPNoteF_1].isHidden = false;
	commands[kcVPNoteF_1].isDummy = false;
	commands[kcVPNoteF_1].Message = "Base octave +1 F";

	commands[kcVPNoteFS1].UID = 1082;
	commands[kcVPNoteFS1].isHidden = false;
	commands[kcVPNoteFS1].isDummy = false;
	commands[kcVPNoteFS1].Message = "Base octave +1 F#";

	commands[kcVPNoteG_1].UID = 1083;
	commands[kcVPNoteG_1].isHidden = false;
	commands[kcVPNoteG_1].isDummy = false;
	commands[kcVPNoteG_1].Message = "Base octave +1 G";

	commands[kcVPNoteGS1].UID = 1084;
	commands[kcVPNoteGS1].isHidden = false;
	commands[kcVPNoteGS1].isDummy = false;
	commands[kcVPNoteGS1].Message = "Base octave +1 G#";

	commands[kcVPNoteA_2].UID = 1085;
	commands[kcVPNoteA_2].isHidden = false;
	commands[kcVPNoteA_2].isDummy = false;
	commands[kcVPNoteA_2].Message = "Base octave +2 A";

	commands[kcVPNoteAS2].UID = 1086;
	commands[kcVPNoteAS2].isHidden = false;
	commands[kcVPNoteAS2].isDummy = false;
	commands[kcVPNoteAS2].Message = "Base octave +2 A#";

	commands[kcVPNoteB_2].UID = 1087;
	commands[kcVPNoteB_2].isHidden = false;
	commands[kcVPNoteB_2].isDummy = false;
	commands[kcVPNoteB_2].Message = "Base octave +2 B";

	commands[kcVPNoteC_2].UID = 1088;
	commands[kcVPNoteC_2].isHidden = false;
	commands[kcVPNoteC_2].isDummy = false;
	commands[kcVPNoteC_2].Message = "Base octave +2 C";

	commands[kcVPNoteCS2].UID = 1089;
	commands[kcVPNoteCS2].isHidden = false;
	commands[kcVPNoteCS2].isDummy = false;
	commands[kcVPNoteCS2].Message = "Base octave +2 C#";

	commands[kcVPNoteD_2].UID = 1090;
	commands[kcVPNoteD_2].isHidden = false;
	commands[kcVPNoteD_2].isDummy = false;
	commands[kcVPNoteD_2].Message = "Base octave +2 D";

	commands[kcVPNoteDS2].UID = 1091;
	commands[kcVPNoteDS2].isHidden = false;
	commands[kcVPNoteDS2].isDummy = false;
	commands[kcVPNoteDS2].Message = "Base octave +2 D#";

	commands[kcVPNoteE_2].UID = 1092;
	commands[kcVPNoteE_2].isHidden = false;
	commands[kcVPNoteE_2].isDummy = false;
	commands[kcVPNoteE_2].Message = "Base octave +2 E";

	commands[kcVPNoteF_2].UID = 1093;
	commands[kcVPNoteF_2].isHidden = false;
	commands[kcVPNoteF_2].isDummy = false;
	commands[kcVPNoteF_2].Message = "Base octave +2 F";

	commands[kcVPNoteFS2].UID = 1094;
	commands[kcVPNoteFS2].isHidden = false;
	commands[kcVPNoteFS2].isDummy = false;
	commands[kcVPNoteFS2].Message = "Base octave +2 F#";

	commands[kcVPNoteG_2].UID = 1095;
	commands[kcVPNoteG_2].isHidden = false;
	commands[kcVPNoteG_2].isDummy = false;
	commands[kcVPNoteG_2].Message = "Base octave +2 G";

	commands[kcVPNoteGS2].UID = 1096;
	commands[kcVPNoteGS2].isHidden = false;
	commands[kcVPNoteGS2].isDummy = false;
	commands[kcVPNoteGS2].Message = "Base octave +2 G#";

	commands[kcVPNoteA_3].UID = 1097;
	commands[kcVPNoteA_3].isHidden = false;
	commands[kcVPNoteA_3].isDummy = false;
	commands[kcVPNoteA_3].Message = "Base octave +3 A";

	commands[kcVPNoteStopC_0].UID = 1098;
	commands[kcVPNoteStopC_0].isHidden = true;
	commands[kcVPNoteStopC_0].isDummy = false;
	commands[kcVPNoteStopC_0].Message = "Stop base octave C";

	commands[kcVPNoteStopCS0].UID = 1099;
	commands[kcVPNoteStopCS0].isHidden = true;
	commands[kcVPNoteStopCS0].isDummy = false;
	commands[kcVPNoteStopCS0].Message = "Stop base octave C#";

	commands[kcVPNoteStopD_0].UID = 1100;
	commands[kcVPNoteStopD_0].isHidden = true;
	commands[kcVPNoteStopD_0].isDummy = false;
	commands[kcVPNoteStopD_0].Message = "Stop base octave D";

	commands[kcVPNoteStopDS0].UID = 1101;
	commands[kcVPNoteStopDS0].isHidden = true;
	commands[kcVPNoteStopDS0].isDummy = false;
	commands[kcVPNoteStopDS0].Message = "Stop base octave D#";

	commands[kcVPNoteStopE_0].UID = 1102;
	commands[kcVPNoteStopE_0].isHidden = true;
	commands[kcVPNoteStopE_0].isDummy = false;
	commands[kcVPNoteStopE_0].Message = "Stop base octave E";

	commands[kcVPNoteStopF_0].UID = 1103;
	commands[kcVPNoteStopF_0].isHidden = true;
	commands[kcVPNoteStopF_0].isDummy = false;
	commands[kcVPNoteStopF_0].Message = "Stop base octave F";

	commands[kcVPNoteStopFS0].UID = 1104;
	commands[kcVPNoteStopFS0].isHidden = true;
	commands[kcVPNoteStopFS0].isDummy = false;
	commands[kcVPNoteStopFS0].Message = "Stop base octave F#";

	commands[kcVPNoteStopG_0].UID = 1105;
	commands[kcVPNoteStopG_0].isHidden = true;
	commands[kcVPNoteStopG_0].isDummy = false;
	commands[kcVPNoteStopG_0].Message = "Stop base octave G";

	commands[kcVPNoteStopGS0].UID = 1106;
	commands[kcVPNoteStopGS0].isHidden = true;
	commands[kcVPNoteStopGS0].isDummy = false;
	commands[kcVPNoteStopGS0].Message = "Stop base octave G#";

	commands[kcVPNoteStopA_1].UID = 1107;
	commands[kcVPNoteStopA_1].isHidden = true;
	commands[kcVPNoteStopA_1].isDummy = false;
	commands[kcVPNoteStopA_1].Message = "Stop base octave +1 A";

	commands[kcVPNoteStopAS1].UID = 1108;
	commands[kcVPNoteStopAS1].isHidden = true;
	commands[kcVPNoteStopAS1].isDummy = false;
	commands[kcVPNoteStopAS1].Message = "Stop base octave +1 A#";

	commands[kcVPNoteStopB_1].UID = 1109;
	commands[kcVPNoteStopB_1].isHidden = true;
	commands[kcVPNoteStopB_1].isDummy = false;
	commands[kcVPNoteStopB_1].Message = "Stop base octave +1 B";

	commands[kcVPNoteStopC_1].UID = 1110;
	commands[kcVPNoteStopC_1].isHidden = true;
	commands[kcVPNoteStopC_1].isDummy = false;
	commands[kcVPNoteStopC_1].Message = "Stop base octave +1 C";

	commands[kcVPNoteStopCS1].UID = 1111;
	commands[kcVPNoteStopCS1].isHidden = true;
	commands[kcVPNoteStopCS1].isDummy = false;
	commands[kcVPNoteStopCS1].Message = "Stop base octave +1 C#";

	commands[kcVPNoteStopD_1].UID = 1112;
	commands[kcVPNoteStopD_1].isHidden = true;
	commands[kcVPNoteStopD_1].isDummy = false;
	commands[kcVPNoteStopD_1].Message = "Stop base octave +1 D";

	commands[kcVPNoteStopDS1].UID = 1113;
	commands[kcVPNoteStopDS1].isHidden = true;
	commands[kcVPNoteStopDS1].isDummy = false;
	commands[kcVPNoteStopDS1].Message = "Stop base octave +1 D#";

	commands[kcVPNoteStopE_1].UID = 1114;
	commands[kcVPNoteStopE_1].isHidden = true;
	commands[kcVPNoteStopE_1].isDummy = false;
	commands[kcVPNoteStopE_1].Message = "Stop base octave +1 E";

	commands[kcVPNoteStopF_1].UID = 1115;
	commands[kcVPNoteStopF_1].isHidden = true;
	commands[kcVPNoteStopF_1].isDummy = false;
	commands[kcVPNoteStopF_1].Message = "Stop base octave +1 F";

	commands[kcVPNoteStopFS1].UID = 1116;
	commands[kcVPNoteStopFS1].isHidden = true;
	commands[kcVPNoteStopFS1].isDummy = false;
	commands[kcVPNoteStopFS1].Message = "Stop base octave +1 F#";

	commands[kcVPNoteStopG_1].UID = 1117;
	commands[kcVPNoteStopG_1].isHidden = true;
	commands[kcVPNoteStopG_1].isDummy = false;
	commands[kcVPNoteStopG_1].Message = "Stop base octave +1 G";

	commands[kcVPNoteStopGS1].UID = 1118;
	commands[kcVPNoteStopGS1].isHidden = true;
	commands[kcVPNoteStopGS1].isDummy = false;
	commands[kcVPNoteStopGS1].Message = "Stop base octave +1 G#";

	commands[kcVPNoteStopA_2].UID = 1119;
	commands[kcVPNoteStopA_2].isHidden = true;
	commands[kcVPNoteStopA_2].isDummy = false;
	commands[kcVPNoteStopA_2].Message = "Stop base octave +2 A";

	commands[kcVPNoteStopAS2].UID = 1120;
	commands[kcVPNoteStopAS2].isHidden = true;
	commands[kcVPNoteStopAS2].isDummy = false;
	commands[kcVPNoteStopAS2].Message = "Stop base octave +2 A#";

	commands[kcVPNoteStopB_2].UID = 1121;
	commands[kcVPNoteStopB_2].isHidden = true;
	commands[kcVPNoteStopB_2].isDummy = false;
	commands[kcVPNoteStopB_2].Message = "Stop base octave +2 B";

	commands[kcVPNoteStopC_2].UID = 1122;
	commands[kcVPNoteStopC_2].isHidden = true;
	commands[kcVPNoteStopC_2].isDummy = false;
	commands[kcVPNoteStopC_2].Message = "Stop base octave +2 C";

	commands[kcVPNoteStopCS2].UID = 1123;
	commands[kcVPNoteStopCS2].isHidden = true;
	commands[kcVPNoteStopCS2].isDummy = false;
	commands[kcVPNoteStopCS2].Message = "Stop base octave +2 C#";

	commands[kcVPNoteStopD_2].UID = 1124;
	commands[kcVPNoteStopD_2].isHidden = true;
	commands[kcVPNoteStopD_2].isDummy = false;
	commands[kcVPNoteStopD_2].Message = "Stop base octave +2 D";

	commands[kcVPNoteStopDS2].UID = 1125;
	commands[kcVPNoteStopDS2].isHidden = true;
	commands[kcVPNoteStopDS2].isDummy = false;
	commands[kcVPNoteStopDS2].Message = "Stop base octave +2 D#";

	commands[kcVPNoteStopE_2].UID = 1126;
	commands[kcVPNoteStopE_2].isHidden = true;
	commands[kcVPNoteStopE_2].isDummy = false;
	commands[kcVPNoteStopE_2].Message = "Stop base octave +2 E";

	commands[kcVPNoteStopF_2].UID = 1127;
	commands[kcVPNoteStopF_2].isHidden = true;
	commands[kcVPNoteStopF_2].isDummy = false;
	commands[kcVPNoteStopF_2].Message = "Stop base octave +2 F";

	commands[kcVPNoteStopFS2].UID = 1128;
	commands[kcVPNoteStopFS2].isHidden = true;
	commands[kcVPNoteStopFS2].isDummy = false;
	commands[kcVPNoteStopFS2].Message = "Stop base octave +2 F#";

	commands[kcVPNoteStopG_2].UID = 1129;
	commands[kcVPNoteStopG_2].isHidden = true;
	commands[kcVPNoteStopG_2].isDummy = false;
	commands[kcVPNoteStopG_2].Message = "Stop base octave +2 G";

	commands[kcVPNoteStopGS2].UID = 1130;
	commands[kcVPNoteStopGS2].isHidden = true;
	commands[kcVPNoteStopGS2].isDummy = false;
	commands[kcVPNoteStopGS2].Message = "Stop base octave +2 G#";

	commands[kcVPNoteStopA_3].UID = 1131;
	commands[kcVPNoteStopA_3].isHidden = true;
	commands[kcVPNoteStopA_3].isDummy = false;
	commands[kcVPNoteStopA_3].Message = "Stop base octave +3 A";


	commands[kcVPChordC_0].UID = 1132;
	commands[kcVPChordC_0].isHidden = true;
	commands[kcVPChordC_0].isDummy = false;
	commands[kcVPChordC_0].Message = "base octave chord C";

	commands[kcVPChordCS0].UID = 1133;
	commands[kcVPChordCS0].isHidden = true;
	commands[kcVPChordCS0].isDummy = false;
	commands[kcVPChordCS0].Message = "base octave chord C#";

	commands[kcVPChordD_0].UID = 1134;
	commands[kcVPChordD_0].isHidden = true;
	commands[kcVPChordD_0].isDummy = false;
	commands[kcVPChordD_0].Message = "base octave chord D";

	commands[kcVPChordDS0].UID = 1135;
	commands[kcVPChordDS0].isHidden = true;
	commands[kcVPChordDS0].isDummy = false;
	commands[kcVPChordDS0].Message = "base octave chord D#";

	commands[kcVPChordE_0].UID = 1136;
	commands[kcVPChordE_0].isHidden = true;
	commands[kcVPChordE_0].isDummy = false;
	commands[kcVPChordE_0].Message = "base octave chord E";

	commands[kcVPChordF_0].UID = 1137;
	commands[kcVPChordF_0].isHidden = true;
	commands[kcVPChordF_0].isDummy = false;
	commands[kcVPChordF_0].Message = "base octave chord F";

	commands[kcVPChordFS0].UID = 1138;
	commands[kcVPChordFS0].isHidden = true;
	commands[kcVPChordFS0].isDummy = false;
	commands[kcVPChordFS0].Message = "base octave chord F#";

	commands[kcVPChordG_0].UID = 1139;
	commands[kcVPChordG_0].isHidden = true;
	commands[kcVPChordG_0].isDummy = false;
	commands[kcVPChordG_0].Message = "base octave chord G";

	commands[kcVPChordGS0].UID = 1140;
	commands[kcVPChordGS0].isHidden = true;
	commands[kcVPChordGS0].isDummy = false;
	commands[kcVPChordGS0].Message = "base octave chord G#";

	commands[kcVPChordA_1].UID = 1141;
	commands[kcVPChordA_1].isHidden = true;
	commands[kcVPChordA_1].isDummy = false;
	commands[kcVPChordA_1].Message = "base octave +1 chord A";

	commands[kcVPChordAS1].UID = 1142;
	commands[kcVPChordAS1].isHidden = true;
	commands[kcVPChordAS1].isDummy = false;
	commands[kcVPChordAS1].Message = "base octave +1 chord A#";

	commands[kcVPChordB_1].UID = 1143;
	commands[kcVPChordB_1].isHidden = true;
	commands[kcVPChordB_1].isDummy = false;
	commands[kcVPChordB_1].Message = "base octave +1 chord B";

	commands[kcVPChordC_1].UID = 1144;
	commands[kcVPChordC_1].isHidden = true;
	commands[kcVPChordC_1].isDummy = false;
	commands[kcVPChordC_1].Message = "base octave +1 chord C";

	commands[kcVPChordCS1].UID = 1145;
	commands[kcVPChordCS1].isHidden = true;
	commands[kcVPChordCS1].isDummy = false;
	commands[kcVPChordCS1].Message = "base octave +1 chord C#";

	commands[kcVPChordD_1].UID = 1146;
	commands[kcVPChordD_1].isHidden = true;
	commands[kcVPChordD_1].isDummy = false;
	commands[kcVPChordD_1].Message = "base octave +1 chord D";

	commands[kcVPChordDS1].UID = 1147;
	commands[kcVPChordDS1].isHidden = true;
	commands[kcVPChordDS1].isDummy = false;
	commands[kcVPChordDS1].Message = "base octave +1 chord D#";

	commands[kcVPChordE_1].UID = 1148;
	commands[kcVPChordE_1].isHidden = true;
	commands[kcVPChordE_1].isDummy = false;
	commands[kcVPChordE_1].Message = "base octave +1 chord E";

	commands[kcVPChordF_1].UID = 1149;
	commands[kcVPChordF_1].isHidden = true;
	commands[kcVPChordF_1].isDummy = false;
	commands[kcVPChordF_1].Message = "base octave +1 chord F";

	commands[kcVPChordFS1].UID = 1150;
	commands[kcVPChordFS1].isHidden = true;
	commands[kcVPChordFS1].isDummy = false;
	commands[kcVPChordFS1].Message = "base octave +1 chord F#";

	commands[kcVPChordG_1].UID = 1151;
	commands[kcVPChordG_1].isHidden = true;
	commands[kcVPChordG_1].isDummy = false;
	commands[kcVPChordG_1].Message = "base octave +1 chord G";

	commands[kcVPChordGS1].UID = 1152;
	commands[kcVPChordGS1].isHidden = true;
	commands[kcVPChordGS1].isDummy = false;
	commands[kcVPChordGS1].Message = "base octave +1 chord G#";

	commands[kcVPChordA_2].UID = 1153;
	commands[kcVPChordA_2].isHidden = true;
	commands[kcVPChordA_2].isDummy = false;
	commands[kcVPChordA_2].Message = "base octave +2 chord A";

	commands[kcVPChordAS2].UID = 1154;
	commands[kcVPChordAS2].isHidden = true;
	commands[kcVPChordAS2].isDummy = false;
	commands[kcVPChordAS2].Message = "base octave +2 chord A#";

	commands[kcVPChordB_2].UID = 1155;
	commands[kcVPChordB_2].isHidden = true;
	commands[kcVPChordB_2].isDummy = false;
	commands[kcVPChordB_2].Message = "base octave +2 chord B";

	commands[kcVPChordC_2].UID = 1156;
	commands[kcVPChordC_2].isHidden = true;
	commands[kcVPChordC_2].isDummy = false;
	commands[kcVPChordC_2].Message = "base octave +2 chord C";

	commands[kcVPChordCS2].UID = 1157;
	commands[kcVPChordCS2].isHidden = true;
	commands[kcVPChordCS2].isDummy = false;
	commands[kcVPChordCS2].Message = "base octave +2 chord C#";

	commands[kcVPChordD_2].UID = 1158;
	commands[kcVPChordD_2].isHidden = true;
	commands[kcVPChordD_2].isDummy = false;
	commands[kcVPChordD_2].Message = "base octave +2 chord D";

	commands[kcVPChordDS2].UID = 1159;
	commands[kcVPChordDS2].isHidden = true;
	commands[kcVPChordDS2].isDummy = false;
	commands[kcVPChordDS2].Message = "base octave +2 chord D#";

	commands[kcVPChordE_2].UID = 1160;
	commands[kcVPChordE_2].isHidden = true;
	commands[kcVPChordE_2].isDummy = false;
	commands[kcVPChordE_2].Message = "base octave +2 chord E";

	commands[kcVPChordF_2].UID = 1161;
	commands[kcVPChordF_2].isHidden = true;
	commands[kcVPChordF_2].isDummy = false;
	commands[kcVPChordF_2].Message = "base octave +2 chord F";

	commands[kcVPChordFS2].UID = 1162;
	commands[kcVPChordFS2].isHidden = true;
	commands[kcVPChordFS2].isDummy = false;
	commands[kcVPChordFS2].Message = "base octave +2 chord F#";

	commands[kcVPChordG_2].UID = 1163;
	commands[kcVPChordG_2].isHidden = true;
	commands[kcVPChordG_2].isDummy = false;
	commands[kcVPChordG_2].Message = "base octave +2 chord G";

	commands[kcVPChordGS2].UID = 1164;
	commands[kcVPChordGS2].isHidden = true;
	commands[kcVPChordGS2].isDummy = false;
	commands[kcVPChordGS2].Message = "base octave +2 chord G#";

	commands[kcVPChordA_3].UID = 1165;
	commands[kcVPChordA_3].isHidden = true;
	commands[kcVPChordA_3].isDummy = false;
	commands[kcVPChordA_3].Message = "base octave chord +3 A";


	commands[kcVPChordStopC_0].UID = 1166;
	commands[kcVPChordStopC_0].isHidden = true;
	commands[kcVPChordStopC_0].isDummy = false;
	commands[kcVPChordStopC_0].Message = "Stop base octave chord C";

	commands[kcVPChordStopCS0].UID = 1167;
	commands[kcVPChordStopCS0].isHidden = true;
	commands[kcVPChordStopCS0].isDummy = false;
	commands[kcVPChordStopCS0].Message = "Stop base octave chord C#";

	commands[kcVPChordStopD_0].UID = 1168;
	commands[kcVPChordStopD_0].isHidden = true;
	commands[kcVPChordStopD_0].isDummy = false;
	commands[kcVPChordStopD_0].Message = "Stop base octave chord D";

	commands[kcVPChordStopDS0].UID = 1169;
	commands[kcVPChordStopDS0].isHidden = true;
	commands[kcVPChordStopDS0].isDummy = false;
	commands[kcVPChordStopDS0].Message = "Stop base octave chord D#";

	commands[kcVPChordStopE_0].UID = 1170;
	commands[kcVPChordStopE_0].isHidden = true;
	commands[kcVPChordStopE_0].isDummy = false;
	commands[kcVPChordStopE_0].Message = "Stop base octave chord E";

	commands[kcVPChordStopF_0].UID = 1171;
	commands[kcVPChordStopF_0].isHidden = true;
	commands[kcVPChordStopF_0].isDummy = false;
	commands[kcVPChordStopF_0].Message = "Stop base octave chord F";

	commands[kcVPChordStopFS0].UID = 1172;
	commands[kcVPChordStopFS0].isHidden = true;
	commands[kcVPChordStopFS0].isDummy = false;
	commands[kcVPChordStopFS0].Message = "Stop base octave chord F#";

	commands[kcVPChordStopG_0].UID = 1173;
	commands[kcVPChordStopG_0].isHidden = true;
	commands[kcVPChordStopG_0].isDummy = false;
	commands[kcVPChordStopG_0].Message = "Stop base octave chord G";

	commands[kcVPChordStopGS0].UID = 1174;
	commands[kcVPChordStopGS0].isHidden = true;
	commands[kcVPChordStopGS0].isDummy = false;
	commands[kcVPChordStopGS0].Message = "Stop base octave chord G#";

	commands[kcVPChordStopA_1].UID = 1175;
	commands[kcVPChordStopA_1].isHidden = true;
	commands[kcVPChordStopA_1].isDummy = false;
	commands[kcVPChordStopA_1].Message = "Stop base octave +1 chord A";

	commands[kcVPChordStopAS1].UID = 1176;
	commands[kcVPChordStopAS1].isHidden = true;
	commands[kcVPChordStopAS1].isDummy = false;
	commands[kcVPChordStopAS1].Message = "Stop base octave +1 chord A#";

	commands[kcVPChordStopB_1].UID = 1177;
	commands[kcVPChordStopB_1].isHidden = true;
	commands[kcVPChordStopB_1].isDummy = false;
	commands[kcVPChordStopB_1].Message = "Stop base octave +1 chord B";

	commands[kcVPChordStopC_1].UID = 1178;
	commands[kcVPChordStopC_1].isHidden = true;
	commands[kcVPChordStopC_1].isDummy = false;
	commands[kcVPChordStopC_1].Message = "Stop base octave +1 chord C";

	commands[kcVPChordStopCS1].UID = 1179;
	commands[kcVPChordStopCS1].isHidden = true;
	commands[kcVPChordStopCS1].isDummy = false;
	commands[kcVPChordStopCS1].Message = "Stop base octave +1 chord C#";

	commands[kcVPChordStopD_1].UID = 1180;
	commands[kcVPChordStopD_1].isHidden = true;
	commands[kcVPChordStopD_1].isDummy = false;
	commands[kcVPChordStopD_1].Message = "Stop base octave +1 chord D";

	commands[kcVPChordStopDS1].UID = 1181;
	commands[kcVPChordStopDS1].isHidden = true;
	commands[kcVPChordStopDS1].isDummy = false;
	commands[kcVPChordStopDS1].Message = "Stop base octave +1 chord D#";

	commands[kcVPChordStopE_1].UID = 1182;
	commands[kcVPChordStopE_1].isHidden = true;
	commands[kcVPChordStopE_1].isDummy = false;
	commands[kcVPChordStopE_1].Message = "Stop base octave +1 chord E";

	commands[kcVPChordStopF_1].UID = 1183;
	commands[kcVPChordStopF_1].isHidden = true;
	commands[kcVPChordStopF_1].isDummy = false;
	commands[kcVPChordStopF_1].Message = "Stop base octave +1 chord F";

	commands[kcVPChordStopFS1].UID = 1184;
	commands[kcVPChordStopFS1].isHidden = true;
	commands[kcVPChordStopFS1].isDummy = false;
	commands[kcVPChordStopFS1].Message = "Stop base octave +1 chord F#";

	commands[kcVPChordStopG_1].UID = 1185;
	commands[kcVPChordStopG_1].isHidden = true;
	commands[kcVPChordStopG_1].isDummy = false;
	commands[kcVPChordStopG_1].Message = "Stop base octave +1 chord G";

	commands[kcVPChordStopGS1].UID = 1186;
	commands[kcVPChordStopGS1].isHidden = true;
	commands[kcVPChordStopGS1].isDummy = false;
	commands[kcVPChordStopGS1].Message = "Stop base octave +1 chord G#";

	commands[kcVPChordStopA_2].UID = 1187;
	commands[kcVPChordStopA_2].isHidden = true;
	commands[kcVPChordStopA_2].isDummy = false;
	commands[kcVPChordStopA_2].Message = "Stop base octave +2 chord A";

	commands[kcVPChordStopAS2].UID = 1188;
	commands[kcVPChordStopAS2].isHidden = true;
	commands[kcVPChordStopAS2].isDummy = false;
	commands[kcVPChordStopAS2].Message = "Stop base octave +2 chord A#";

	commands[kcVPChordStopB_2].UID = 1189;
	commands[kcVPChordStopB_2].isHidden = true;
	commands[kcVPChordStopB_2].isDummy = false;
	commands[kcVPChordStopB_2].Message = "Stop base octave +2 chord B";

	commands[kcVPChordStopC_2].UID = 1190;
	commands[kcVPChordStopC_2].isHidden = true;
	commands[kcVPChordStopC_2].isDummy = false;
	commands[kcVPChordStopC_2].Message = "Stop base octave +2 chord C";

	commands[kcVPChordStopCS2].UID = 1191;
	commands[kcVPChordStopCS2].isHidden = true;
	commands[kcVPChordStopCS2].isDummy = false;
	commands[kcVPChordStopCS2].Message = "Stop base octave +2 chord C#";

	commands[kcVPChordStopD_2].UID = 1192;
	commands[kcVPChordStopD_2].isHidden = true;
	commands[kcVPChordStopD_2].isDummy = false;
	commands[kcVPChordStopD_2].Message = "Stop base octave +2 chord D";

	commands[kcVPChordStopDS2].UID = 1193;
	commands[kcVPChordStopDS2].isHidden = true;
	commands[kcVPChordStopDS2].isDummy = false;
	commands[kcVPChordStopDS2].Message = "Stop base octave +2 chord D#";

	commands[kcVPChordStopE_2].UID = 1194;
	commands[kcVPChordStopE_2].isHidden = true;
	commands[kcVPChordStopE_2].isDummy = false;
	commands[kcVPChordStopE_2].Message = "Stop base octave +2 chord E";

	commands[kcVPChordStopF_2].UID = 1195;
	commands[kcVPChordStopF_2].isHidden = true;
	commands[kcVPChordStopF_2].isDummy = false;
	commands[kcVPChordStopF_2].Message = "Stop base octave +2 chord F";

	commands[kcVPChordStopFS2].UID = 1196;
	commands[kcVPChordStopFS2].isHidden = true;
	commands[kcVPChordStopFS2].isDummy = false;
	commands[kcVPChordStopFS2].Message = "Stop base octave +2 chord F#";

	commands[kcVPChordStopG_2].UID = 1197;
	commands[kcVPChordStopG_2].isHidden = true;
	commands[kcVPChordStopG_2].isDummy = false;
	commands[kcVPChordStopG_2].Message = "Stop base octave +2 chord G";

	commands[kcVPChordStopGS2].UID = 1198;
	commands[kcVPChordStopGS2].isHidden = true;
	commands[kcVPChordStopGS2].isDummy = false;
	commands[kcVPChordStopGS2].Message = "Stop base octave +2 chord G#";

	commands[kcVPChordStopA_3].UID = 1199;
	commands[kcVPChordStopA_3].isHidden = true;
	commands[kcVPChordStopA_3].isDummy = false;
	commands[kcVPChordStopA_3].Message = "Stop base octave +3 chord  A";

	commands[kcNoteCut].UID = 1200;
	commands[kcNoteCut].isHidden = false;
	commands[kcNoteCut].isDummy = false;
	commands[kcNoteCut].Message = "Note cut";

	commands[kcNoteOff].UID = 1201;
	commands[kcNoteOff].isHidden = false;
	commands[kcNoteOff].isDummy = false;
	commands[kcNoteOff].Message = "Note off";

	commands[kcSetIns0].UID = 1202;
	commands[kcSetIns0].isHidden = false;
	commands[kcSetIns0].isDummy = false;
	commands[kcSetIns0].Message = "Set instrument digit 0";

	commands[kcSetIns1].UID = 1203;
	commands[kcSetIns1].isHidden = false;
	commands[kcSetIns1].isDummy = false;
	commands[kcSetIns1].Message = "Set instrument digit 1";

	commands[kcSetIns2].UID = 1204;
	commands[kcSetIns2].isHidden = false;
	commands[kcSetIns2].isDummy = false;
	commands[kcSetIns2].Message = "Set instrument digit 2";

	commands[kcSetIns3].UID = 1205;
	commands[kcSetIns3].isHidden = false;
	commands[kcSetIns3].isDummy = false;
	commands[kcSetIns3].Message = "Set instrument digit 3";

	commands[kcSetIns4].UID = 1206;
	commands[kcSetIns4].isHidden = false;
	commands[kcSetIns4].isDummy = false;
	commands[kcSetIns4].Message = "Set instrument digit 4";

	commands[kcSetIns5].UID = 1207;
	commands[kcSetIns5].isHidden = false;
	commands[kcSetIns5].isDummy = false;
	commands[kcSetIns5].Message = "Set instrument digit 5";

	commands[kcSetIns6].UID = 1208;
	commands[kcSetIns6].isHidden = false;
	commands[kcSetIns6].isDummy = false;
	commands[kcSetIns6].Message = "Set instrument digit 6";

	commands[kcSetIns7].UID = 1209;
	commands[kcSetIns7].isHidden = false;
	commands[kcSetIns7].isDummy = false;
	commands[kcSetIns7].Message = "Set instrument digit 7";

	commands[kcSetIns8].UID = 1210;
	commands[kcSetIns8].isHidden = false;
	commands[kcSetIns8].isDummy = false;
	commands[kcSetIns8].Message = "Set instrument digit 8";

	commands[kcSetIns9].UID = 1211;
	commands[kcSetIns9].isHidden = false;
	commands[kcSetIns9].isDummy = false;
	commands[kcSetIns9].Message = "Set instrument digit 9";

	commands[kcSetOctave0].UID = 1212;
	commands[kcSetOctave0].isHidden = false;
	commands[kcSetOctave0].isDummy = false;
	commands[kcSetOctave0].Message = "Set octave 0";

	commands[kcSetOctave1].UID = 1213;
	commands[kcSetOctave1].isHidden = false;
	commands[kcSetOctave1].isDummy = false;
	commands[kcSetOctave1].Message = "Set octave 1";

	commands[kcSetOctave2].UID = 1214;
	commands[kcSetOctave2].isHidden = false;
	commands[kcSetOctave2].isDummy = false;
	commands[kcSetOctave2].Message = "Set octave 2";

	commands[kcSetOctave3].UID = 1215;
	commands[kcSetOctave3].isHidden = false;
	commands[kcSetOctave3].isDummy = false;
	commands[kcSetOctave3].Message = "Set octave 3";

	commands[kcSetOctave4].UID = 1216;
	commands[kcSetOctave4].isHidden = false;
	commands[kcSetOctave4].isDummy = false;
	commands[kcSetOctave4].Message = "Set octave 4";

	commands[kcSetOctave5].UID = 1217;
	commands[kcSetOctave5].isHidden = false;
	commands[kcSetOctave5].isDummy = false;
	commands[kcSetOctave5].Message = "Set octave 5";

	commands[kcSetOctave6].UID = 1218;
	commands[kcSetOctave6].isHidden = false;
	commands[kcSetOctave6].isDummy = false;
	commands[kcSetOctave6].Message = "Set octave 6";

	commands[kcSetOctave7].UID = 1219;
	commands[kcSetOctave7].isHidden = false;
	commands[kcSetOctave7].isDummy = false;
	commands[kcSetOctave7].Message = "Set octave 7";

	commands[kcSetOctave8].UID = 1220;
	commands[kcSetOctave8].isHidden = false;
	commands[kcSetOctave8].isDummy = false;
	commands[kcSetOctave8].Message = "Set octave 8";

	commands[kcSetOctave9].UID = 1221;
	commands[kcSetOctave9].isHidden = false;
	commands[kcSetOctave9].isDummy = false;
	commands[kcSetOctave9].Message = "Set octave 9";

	commands[kcSetVolume0].UID = 1222;
	commands[kcSetVolume0].isHidden = false;
	commands[kcSetVolume0].isDummy = false;
	commands[kcSetVolume0].Message = "Set volume digit 0";

	commands[kcSetVolume1].UID = 1223;
	commands[kcSetVolume1].isHidden = false;
	commands[kcSetVolume1].isDummy = false;
	commands[kcSetVolume1].Message = "Set volume digit 1";

	commands[kcSetVolume2].UID = 1224;
	commands[kcSetVolume2].isHidden = false;
	commands[kcSetVolume2].isDummy = false;
	commands[kcSetVolume2].Message = "Set volume digit 2";

	commands[kcSetVolume3].UID = 1225;
	commands[kcSetVolume3].isHidden = false;
	commands[kcSetVolume3].isDummy = false;
	commands[kcSetVolume3].Message = "Set volume digit 3";

	commands[kcSetVolume4].UID = 1226;
	commands[kcSetVolume4].isHidden = false;
	commands[kcSetVolume4].isDummy = false;
	commands[kcSetVolume4].Message = "Set volume digit 4";

	commands[kcSetVolume5].UID = 1227;
	commands[kcSetVolume5].isHidden = false;
	commands[kcSetVolume5].isDummy = false;
	commands[kcSetVolume5].Message = "Set volume digit 5";

	commands[kcSetVolume6].UID = 1228;
	commands[kcSetVolume6].isHidden = false;
	commands[kcSetVolume6].isDummy = false;
	commands[kcSetVolume6].Message = "Set volume digit 6";

	commands[kcSetVolume7].UID = 1229;
	commands[kcSetVolume7].isHidden = false;
	commands[kcSetVolume7].isDummy = false;
	commands[kcSetVolume7].Message = "Set volume digit 7";

	commands[kcSetVolume8].UID = 1230;
	commands[kcSetVolume8].isHidden = false;
	commands[kcSetVolume8].isDummy = false;
	commands[kcSetVolume8].Message = "Set volume digit 8";

	commands[kcSetVolume9].UID = 1231;
	commands[kcSetVolume9].isHidden = false;
	commands[kcSetVolume9].isDummy = false;
	commands[kcSetVolume9].Message = "Set volume digit 9";

	commands[kcSetVolumeVol].UID = 1232;
	commands[kcSetVolumeVol].isHidden = false;
	commands[kcSetVolumeVol].isDummy = false;
	commands[kcSetVolumeVol].Message = "Vol command - volume";

	commands[kcSetVolumePan].UID = 1233;
	commands[kcSetVolumePan].isHidden = false;
	commands[kcSetVolumePan].isDummy = false;
	commands[kcSetVolumePan].Message = "Vol command - pan";

	commands[kcSetVolumeVolSlideUp].UID = 1234;
	commands[kcSetVolumeVolSlideUp].isHidden = false;
	commands[kcSetVolumeVolSlideUp].isDummy = false;
	commands[kcSetVolumeVolSlideUp].Message = "Vol command - vol slide up";

	commands[kcSetVolumeVolSlideDown].UID = 1235;
	commands[kcSetVolumeVolSlideDown].isHidden = false;
	commands[kcSetVolumeVolSlideDown].isDummy = false;
	commands[kcSetVolumeVolSlideDown].Message = "Vol command - vol slide down";

	commands[kcSetVolumeFineVolUp].UID = 1236;
	commands[kcSetVolumeFineVolUp].isHidden = false;
	commands[kcSetVolumeFineVolUp].isDummy = false;
	commands[kcSetVolumeFineVolUp].Message = "Vol command - vol fine slide up";

	commands[kcSetVolumeFineVolDown].UID = 1237;
	commands[kcSetVolumeFineVolDown].isHidden = false;
	commands[kcSetVolumeFineVolDown].isDummy = false;
	commands[kcSetVolumeFineVolDown].Message = "Vol command - vol fine slide down";

	commands[kcSetVolumeVibratoSpd].UID = 1238;
	commands[kcSetVolumeVibratoSpd].isHidden = false;
	commands[kcSetVolumeVibratoSpd].isDummy = false;
	commands[kcSetVolumeVibratoSpd].Message = "Vol command - vibrato speed";

	commands[kcSetVolumeVibrato].UID = 1239;
	commands[kcSetVolumeVibrato].isHidden = false;
	commands[kcSetVolumeVibrato].isDummy = false;
	commands[kcSetVolumeVibrato].Message = "Vol command - vibrato";

	commands[kcSetVolumeXMPanLeft].UID = 1240;
	commands[kcSetVolumeXMPanLeft].isHidden = false;
	commands[kcSetVolumeXMPanLeft].isDummy = false;
	commands[kcSetVolumeXMPanLeft].Message = "Vol command - XM pan left";

	commands[kcSetVolumeXMPanRight].UID = 1241;
	commands[kcSetVolumeXMPanRight].isHidden = false;
	commands[kcSetVolumeXMPanRight].isDummy = false;
	commands[kcSetVolumeXMPanRight].Message = "Vol command - XM pan right";

	commands[kcSetVolumePortamento].UID = 1242;
	commands[kcSetVolumePortamento].isHidden = false;
	commands[kcSetVolumePortamento].isDummy = false;
	commands[kcSetVolumePortamento].Message = "Vol command - Portamento";

	commands[kcSetVolumeITPortaUp].UID = 1243;
	commands[kcSetVolumeITPortaUp].isHidden = false;
	commands[kcSetVolumeITPortaUp].isDummy = false;
	commands[kcSetVolumeITPortaUp].Message = "Vol command - Portamento Up";

	commands[kcSetVolumeITPortaDown].UID = 1244;
	commands[kcSetVolumeITPortaDown].isHidden = false;
	commands[kcSetVolumeITPortaDown].isDummy = false;
	commands[kcSetVolumeITPortaDown].Message = "Vol command - Portamento Down";

	commands[kcSetVolumeITVelocity].UID = 1245;
	commands[kcSetVolumeITVelocity].isHidden = false;
	commands[kcSetVolumeITVelocity].isDummy = false;
	commands[kcSetVolumeITVelocity].Message = "Vol command - Velocity";

	commands[kcSetVolumeITOffset].UID = 1246;
	commands[kcSetVolumeITOffset].isHidden = false;
	commands[kcSetVolumeITOffset].isDummy = false;
	commands[kcSetVolumeITOffset].Message = "Vol command - Offset";

	commands[kcSetFXParam0].UID = 1247;
	commands[kcSetFXParam0].isHidden = false;
	commands[kcSetFXParam0].isDummy = false;
	commands[kcSetFXParam0].Message = "FX Param digit 0";

	commands[kcSetFXParam1].UID = 1248;
	commands[kcSetFXParam1].isHidden = false;
	commands[kcSetFXParam1].isDummy = false;
	commands[kcSetFXParam1].Message = "FX Param digit 1";

	commands[kcSetFXParam2].UID = 1249;
	commands[kcSetFXParam2].isHidden = false;
	commands[kcSetFXParam2].isDummy = false;
	commands[kcSetFXParam2].Message = "FX Param digit 2";

	commands[kcSetFXParam3].UID = 1250;
	commands[kcSetFXParam3].isHidden = false;
	commands[kcSetFXParam3].isDummy = false;
	commands[kcSetFXParam3].Message = "FX Param digit 3";

	commands[kcSetFXParam4].UID = 1251;
	commands[kcSetFXParam4].isHidden = false;
	commands[kcSetFXParam4].isDummy = false;
	commands[kcSetFXParam4].Message = "FX Param digit 4";

	commands[kcSetFXParam5].UID = 1252;
	commands[kcSetFXParam5].isHidden = false;
	commands[kcSetFXParam5].isDummy = false;
	commands[kcSetFXParam5].Message = "FX Param digit 5";

	commands[kcSetFXParam6].UID = 1253;
	commands[kcSetFXParam6].isHidden = false;
	commands[kcSetFXParam6].isDummy = false;
	commands[kcSetFXParam6].Message = "FX Param digit 6";

	commands[kcSetFXParam7].UID = 1254;
	commands[kcSetFXParam7].isHidden = false;
	commands[kcSetFXParam7].isDummy = false;
	commands[kcSetFXParam7].Message = "FX Param digit 7";

	commands[kcSetFXParam8].UID = 1255;
	commands[kcSetFXParam8].isHidden = false;
	commands[kcSetFXParam8].isDummy = false;
	commands[kcSetFXParam8].Message = "FX Param digit 8";

	commands[kcSetFXParam9].UID = 1256;
	commands[kcSetFXParam9].isHidden = false;
	commands[kcSetFXParam9].isDummy = false;
	commands[kcSetFXParam9].Message = "FX Param digit 9";

	commands[kcSetFXParamA].UID = 1257;
	commands[kcSetFXParamA].isHidden = false;
	commands[kcSetFXParamA].isDummy = false;
	commands[kcSetFXParamA].Message = "FX Param digit A";

	commands[kcSetFXParamB].UID = 1258;
	commands[kcSetFXParamB].isHidden = false;
	commands[kcSetFXParamB].isDummy = false;
	commands[kcSetFXParamB].Message = "FX Param digit B";

	commands[kcSetFXParamC].UID = 1259;
	commands[kcSetFXParamC].isHidden = false;
	commands[kcSetFXParamC].isDummy = false;
	commands[kcSetFXParamC].Message = "FX Param digit C";

	commands[kcSetFXParamD].UID = 1260;
	commands[kcSetFXParamD].isHidden = false;
	commands[kcSetFXParamD].isDummy = false;
	commands[kcSetFXParamD].Message = "FX Param digit D";

	commands[kcSetFXParamE].UID = 1261;
	commands[kcSetFXParamE].isHidden = false;
	commands[kcSetFXParamE].isDummy = false;
	commands[kcSetFXParamE].Message = "FX Param digit E";

	commands[kcSetFXParamF].UID = 1262;
	commands[kcSetFXParamF].isHidden = false;
	commands[kcSetFXParamF].isDummy = false;
	commands[kcSetFXParamF].Message = "FX Param digit F";

	commands[kcSetFXarp].UID = 1263;
	commands[kcSetFXarp].isHidden = true;
	commands[kcSetFXarp].isDummy = false;
	commands[kcSetFXarp].Message = "FX arpeggio";

	commands[kcSetFXportUp].UID = 1264;
	commands[kcSetFXportUp].isHidden = true;
	commands[kcSetFXportUp].isDummy = false;
	commands[kcSetFXportUp].Message = "FX portamentao Up";

	commands[kcSetFXportDown].UID = 1265;
	commands[kcSetFXportDown].isHidden = true;
	commands[kcSetFXportDown].isDummy = false;
	commands[kcSetFXportDown].Message = "FX portamentao Down";

	commands[kcSetFXport].UID = 1266;
	commands[kcSetFXport].isHidden = true;
	commands[kcSetFXport].isDummy = false;
	commands[kcSetFXport].Message = "FX portamentao";

	commands[kcSetFXvibrato].UID = 1267;
	commands[kcSetFXvibrato].isHidden = true;
	commands[kcSetFXvibrato].isDummy = false;
	commands[kcSetFXvibrato].Message = "FX vibrato";

	commands[kcSetFXportSlide].UID = 1268;
	commands[kcSetFXportSlide].isHidden = true;
	commands[kcSetFXportSlide].isDummy = false;
	commands[kcSetFXportSlide].Message = "FX portamento slide";

	commands[kcSetFXvibSlide].UID = 1269;
	commands[kcSetFXvibSlide].isHidden = true;
	commands[kcSetFXvibSlide].isDummy = false;
	commands[kcSetFXvibSlide].Message = "FX vibrato slide";

	commands[kcSetFXtremolo].UID = 1270;
	commands[kcSetFXtremolo].isHidden = true;
	commands[kcSetFXtremolo].isDummy = false;
	commands[kcSetFXtremolo].Message = "FX tremolo";

	commands[kcSetFXpan].UID = 1271;
	commands[kcSetFXpan].isHidden = true;
	commands[kcSetFXpan].isDummy = false;
	commands[kcSetFXpan].Message = "FX pan";

	commands[kcSetFXoffset].UID = 1272;
	commands[kcSetFXoffset].isHidden = true;
	commands[kcSetFXoffset].isDummy = false;
	commands[kcSetFXoffset].Message = "FX offset";

	commands[kcSetFXvolSlide].UID = 1273;
	commands[kcSetFXvolSlide].isHidden = true;
	commands[kcSetFXvolSlide].isDummy = false;
	commands[kcSetFXvolSlide].Message = "FX Volume slide";

	commands[kcSetFXgotoOrd].UID = 1274;
	commands[kcSetFXgotoOrd].isHidden = true;
	commands[kcSetFXgotoOrd].isDummy = false;
	commands[kcSetFXgotoOrd].Message = "FX go to order";

	commands[kcSetFXsetVol].UID = 1275;
	commands[kcSetFXsetVol].isHidden = true;
	commands[kcSetFXsetVol].isDummy = false;
	commands[kcSetFXsetVol].Message = "FX set volume";

	commands[kcSetFXgotoRow].UID = 1276;
	commands[kcSetFXgotoRow].isHidden = true;
	commands[kcSetFXgotoRow].isDummy = false;
	commands[kcSetFXgotoRow].Message = "FX go to row";

	commands[kcSetFXretrig].UID = 1277;
	commands[kcSetFXretrig].isHidden = true;
	commands[kcSetFXretrig].isDummy = false;
	commands[kcSetFXretrig].Message = "FX retrigger";

	commands[kcSetFXspeed].UID = 1278;
	commands[kcSetFXspeed].isHidden = true;
	commands[kcSetFXspeed].isDummy = false;
	commands[kcSetFXspeed].Message = "FX set speed";

	commands[kcSetFXtempo].UID = 1279;
	commands[kcSetFXtempo].isHidden = true;
	commands[kcSetFXtempo].isDummy = false;
	commands[kcSetFXtempo].Message = "FX set tempo";

	commands[kcSetFXtremor].UID = 1280;
	commands[kcSetFXtremor].isHidden = true;
	commands[kcSetFXtremor].isDummy = false;
	commands[kcSetFXtremor].Message = "FX tremor";

	commands[kcSetFXextendedMOD].UID = 1281;
	commands[kcSetFXextendedMOD].isHidden = true;
	commands[kcSetFXextendedMOD].isDummy = false;
	commands[kcSetFXextendedMOD].Message = "FX extended MOD cmds";

	commands[kcSetFXextendedS3M].UID = 1282;
	commands[kcSetFXextendedS3M].isHidden = true;
	commands[kcSetFXextendedS3M].isDummy = false;
	commands[kcSetFXextendedS3M].Message = "FX extended S3M cmds";

	commands[kcSetFXchannelVol].UID = 1283;
	commands[kcSetFXchannelVol].isHidden = true;
	commands[kcSetFXchannelVol].isDummy = false;
	commands[kcSetFXchannelVol].Message = "FX set channel vol";

	commands[kcSetFXchannelVols].UID = 1284;
	commands[kcSetFXchannelVols].isHidden = true;
	commands[kcSetFXchannelVols].isDummy = false;
	commands[kcSetFXchannelVols].Message = "FX channel vol slide";

	commands[kcSetFXglobalVol].UID = 1285;
	commands[kcSetFXglobalVol].isHidden = true;
	commands[kcSetFXglobalVol].isDummy = false;
	commands[kcSetFXglobalVol].Message = "FX set global volume";

	commands[kcSetFXglobalVols].UID = 1286;
	commands[kcSetFXglobalVols].isHidden = true;
	commands[kcSetFXglobalVols].isDummy = false;
	commands[kcSetFXglobalVols].Message = "FX global volume slide";

	commands[kcSetFXkeyoff].UID = 1287;
	commands[kcSetFXkeyoff].isHidden = true;
	commands[kcSetFXkeyoff].isDummy = false;
	commands[kcSetFXkeyoff].Message = "FX Some XM Command :D";

	commands[kcSetFXfineVib].UID = 1288;
	commands[kcSetFXfineVib].isHidden = true;
	commands[kcSetFXfineVib].isDummy = false;
	commands[kcSetFXfineVib].Message = "FX fine vibrato";

	commands[kcSetFXpanbrello].UID = 1289;
	commands[kcSetFXpanbrello].isHidden = true;
	commands[kcSetFXpanbrello].isDummy = false;
	commands[kcSetFXpanbrello].Message = "FX set panbrello";

	commands[kcSetFXextendedXM].UID = 1290;
	commands[kcSetFXextendedXM].isHidden = true;
	commands[kcSetFXextendedXM].isDummy = false;
	commands[kcSetFXextendedXM].Message = "FX extended XM effects ";

	commands[kcSetFXpanSlide].UID = 1291;
	commands[kcSetFXpanSlide].isHidden = true;
	commands[kcSetFXpanSlide].isDummy = false;
	commands[kcSetFXpanSlide].Message = "FX pan slide";

	commands[kcSetFXsetEnvPos].UID = 1292;
	commands[kcSetFXsetEnvPos].isHidden = true;
	commands[kcSetFXsetEnvPos].isDummy = false;
	commands[kcSetFXsetEnvPos].Message = "FX set envelope position (XM only)";

	commands[kcSetFXmacro].UID = 1293;
	commands[kcSetFXmacro].isHidden = true;
	commands[kcSetFXmacro].isDummy = false;
	commands[kcSetFXmacro].Message = "FX midi macro";

	commands[kcSetFXmacroSlide].UID = 1294;
	commands[kcSetFXmacroSlide].isHidden = false;
	commands[kcSetFXmacroSlide].isDummy = false;
	commands[kcSetFXmacroSlide].Message = "FX midi macro slide";

	commands[kcSetFXvelocity].UID = 1295;
	commands[kcSetFXvelocity].isHidden = false;
	commands[kcSetFXvelocity].isDummy = false;
	commands[kcSetFXvelocity].Message = "FX pseudo-velocity (experimental)";

	commands[kcPatternJumpDownh1Select].UID = 1296;
	commands[kcPatternJumpDownh1Select].isHidden = true;
	commands[kcPatternJumpDownh1Select].isDummy = false;
	commands[kcPatternJumpDownh1Select].Message = "kcPatternJumpDownh1Select";

	commands[kcPatternJumpUph1Select].UID = 1297;
	commands[kcPatternJumpUph1Select].isHidden = true;
	commands[kcPatternJumpUph1Select].isDummy = false;
	commands[kcPatternJumpUph1Select].Message = "kcPatternJumpUph1Select";

	commands[kcPatternSnapDownh1Select].UID = 1298;
	commands[kcPatternSnapDownh1Select].isHidden = true;
	commands[kcPatternSnapDownh1Select].isDummy = false;
	commands[kcPatternSnapDownh1Select].Message = "kcPatternSnapDownh1Select";

	commands[kcPatternSnapUph1Select].UID = 1299;
	commands[kcPatternSnapUph1Select].isHidden = true;
	commands[kcPatternSnapUph1Select].isDummy = false;
	commands[kcPatternSnapUph1Select].Message = "kcPatternSnapUph1Select";

	commands[kcNavigateDownSelect].UID = 1300;
	commands[kcNavigateDownSelect].isHidden = true;
	commands[kcNavigateDownSelect].isDummy = false;
	commands[kcNavigateDownSelect].Message = "kcNavigateDownSelect";

	commands[kcNavigateUpSelect].UID = 1301;
	commands[kcNavigateUpSelect].isHidden = true;
	commands[kcNavigateUpSelect].isDummy = false;
	commands[kcNavigateUpSelect].Message = "kcNavigateUpSelect";

	commands[kcNavigateLeftSelect].UID = 1302;
	commands[kcNavigateLeftSelect].isHidden = true;
	commands[kcNavigateLeftSelect].isDummy = false;
	commands[kcNavigateLeftSelect].Message = "kcNavigateLeftSelect";

	commands[kcNavigateRightSelect].UID = 1303;
	commands[kcNavigateRightSelect].isHidden = true;
	commands[kcNavigateRightSelect].isDummy = false;
	commands[kcNavigateRightSelect].Message = "kcNavigateRightSelect";

	commands[kcNavigateNextChanSelect].UID = 1304;
	commands[kcNavigateNextChanSelect].isHidden = true;
	commands[kcNavigateNextChanSelect].isDummy = false;
	commands[kcNavigateNextChanSelect].Message = "kcNavigateNextChanSelect";

	commands[kcNavigatePrevChanSelect].UID = 1305;
	commands[kcNavigatePrevChanSelect].isHidden = true;
	commands[kcNavigatePrevChanSelect].isDummy = false;
	commands[kcNavigatePrevChanSelect].Message = "kcNavigatePrevChanSelect";

	commands[kcHomeHorizontalSelect].UID = 1306;
	commands[kcHomeHorizontalSelect].isHidden = true;
	commands[kcHomeHorizontalSelect].isDummy = false;
	commands[kcHomeHorizontalSelect].Message = "kcHomeHorizontalSelect";

	commands[kcHomeVerticalSelect].UID = 1307;
	commands[kcHomeVerticalSelect].isHidden = true;
	commands[kcHomeVerticalSelect].isDummy = false;
	commands[kcHomeVerticalSelect].Message = "kcHomeVerticalSelect";

	commands[kcHomeAbsoluteSelect].UID = 1308;
	commands[kcHomeAbsoluteSelect].isHidden = true;
	commands[kcHomeAbsoluteSelect].isDummy = false;
	commands[kcHomeAbsoluteSelect].Message = "kcHomeAbsoluteSelect";

	commands[kcEndHorizontalSelect].UID = 1309;
	commands[kcEndHorizontalSelect].isHidden = true;
	commands[kcEndHorizontalSelect].isDummy = false;
	commands[kcEndHorizontalSelect].Message = "kcEndHorizontalSelect";

	commands[kcEndVerticalSelect].UID = 1310;
	commands[kcEndVerticalSelect].isHidden = true;
	commands[kcEndVerticalSelect].isDummy = false;
	commands[kcEndVerticalSelect].Message = "kcEndVerticalSelect";

	commands[kcEndAbsoluteSelect].UID = 1311;
	commands[kcEndAbsoluteSelect].isHidden = true;
	commands[kcEndAbsoluteSelect].isDummy = false;
	commands[kcEndAbsoluteSelect].Message = "kcEndAbsoluteSelect";

	commands[kcSelectWithNav].UID = 1312;
	commands[kcSelectWithNav].isHidden = true;
	commands[kcSelectWithNav].isDummy = false;
	commands[kcSelectWithNav].Message = "kcSelectWithNav";

	commands[kcSelectOffWithNav].UID = 1313;
	commands[kcSelectOffWithNav].isHidden = true;
	commands[kcSelectOffWithNav].isDummy = false;
	commands[kcSelectOffWithNav].Message = "kcSelectOffWithNav";

	commands[kcCopySelectWithNav].UID = 1314;
	commands[kcCopySelectWithNav].isHidden = true;
	commands[kcCopySelectWithNav].isDummy = false;
	commands[kcCopySelectWithNav].Message = "kcCopySelectWithNav";

	commands[kcCopySelectOffWithNav].UID = 1315;
	commands[kcCopySelectOffWithNav].isHidden = true;
	commands[kcCopySelectOffWithNav].isDummy = false;
	commands[kcCopySelectOffWithNav].Message = "kcCopySelectOffWithNav";

	commands[kcChordModifier].UID = 1316;
	commands[kcChordModifier].isHidden = false;
	commands[kcChordModifier].isDummy = true;
	commands[kcChordModifier].Message = "Chord Modifier";

	commands[kcSetSpacing].UID = 1317;
	commands[kcSetSpacing].isHidden = false;
	commands[kcSetSpacing].isDummy = true;
	commands[kcSetSpacing].Message = "Set row jump on note entry";

	commands[kcSetSpacing0].UID = 1318;
	commands[kcSetSpacing0].isHidden = true;
	commands[kcSetSpacing0].isDummy = false;
	commands[kcSetSpacing0].Message = "";

	commands[kcSetSpacing1].UID = 1319;
	commands[kcSetSpacing1].isHidden = true;
	commands[kcSetSpacing1].isDummy = false;
	commands[kcSetSpacing1].Message = "";

	commands[kcSetSpacing2].UID = 1320;
	commands[kcSetSpacing2].isHidden = true;
	commands[kcSetSpacing2].isDummy = false;
	commands[kcSetSpacing2].Message = "";

	commands[kcSetSpacing3].UID = 1321;
	commands[kcSetSpacing3].isHidden = true;
	commands[kcSetSpacing3].isDummy = false;
	commands[kcSetSpacing3].Message = "";

	commands[kcSetSpacing4].UID = 1322;
	commands[kcSetSpacing4].isHidden = true;
	commands[kcSetSpacing4].isDummy = false;
	commands[kcSetSpacing4].Message = "";

	commands[kcSetSpacing5].UID = 1323;
	commands[kcSetSpacing5].isHidden = true;
	commands[kcSetSpacing5].isDummy = false;
	commands[kcSetSpacing5].Message = "";

	commands[kcSetSpacing6].UID = 1324;
	commands[kcSetSpacing6].isHidden = true;
	commands[kcSetSpacing6].isDummy = false;
	commands[kcSetSpacing6].Message = "";

	commands[kcSetSpacing7].UID = 1325;
	commands[kcSetSpacing7].isHidden = true;
	commands[kcSetSpacing7].isDummy = false;
	commands[kcSetSpacing7].Message = "";

	commands[kcSetSpacing8].UID = 1326;
	commands[kcSetSpacing8].isHidden = true;
	commands[kcSetSpacing8].isDummy = false;
	commands[kcSetSpacing8].Message = "";

	commands[kcSetSpacing9].UID = 1327;
	commands[kcSetSpacing9].isHidden = true;
	commands[kcSetSpacing9].isDummy = false;
	commands[kcSetSpacing9].Message = "";

	commands[kcCopySelectWithSelect].UID = 1328;
	commands[kcCopySelectWithSelect].isHidden = true;
	commands[kcCopySelectWithSelect].isDummy = false;
	commands[kcCopySelectWithSelect].Message = "kcCopySelectWithSelect";

	commands[kcCopySelectOffWithSelect].UID = 1329;
	commands[kcCopySelectOffWithSelect].isHidden = true;
	commands[kcCopySelectOffWithSelect].isDummy = false;
	commands[kcCopySelectOffWithSelect].Message = "kcCopySelectOffWithSelect";

	commands[kcSelectWithCopySelect].UID = 1330;
	commands[kcSelectWithCopySelect].isHidden = true;
	commands[kcSelectWithCopySelect].isDummy = false;
	commands[kcSelectWithCopySelect].Message = "kcSelectWithCopySelect";

	commands[kcSelectOffWithCopySelect].UID = 1331;
	commands[kcSelectOffWithCopySelect].isHidden = true;
	commands[kcSelectOffWithCopySelect].isDummy = false;
	commands[kcSelectOffWithCopySelect].Message = "kcSelectOffWithCopySelect";

/*	commands[kcCopy].UID = 1332;
	commands[kcCopy].isHidden = false;
	commands[kcCopy].isDummy = false;
	commands[kcCopy].Message = "Copy pattern data";

	commands[kcCut].UID = 1333;
	commands[kcCut].isHidden = false;
	commands[kcCut].isDummy = false;
	commands[kcCut].Message = "Cut pattern data";

	commands[kcPaste].UID = 1334;
	commands[kcPaste].isHidden = false;
	commands[kcPaste].isDummy = false;
    commands[kcPaste].Message = "Paste pattern data";
	
    commands[kcMixPaste].UID = 1335;
	commands[kcMixPaste].isHidden = false;
	commands[kcMixPaste].isDummy = false;
	commands[kcMixPaste].Message = "Mix-paste pattern data";

	commands[kcSelectAll].UID = 1336;
	commands[kcSelectAll].isHidden = false;
	commands[kcSelectAll].isDummy = false;
	commands[kcSelectAll].Message = "Select all pattern data";

	commands[kcSelectCol].UID = 1337;
	commands[kcSelectCol].isHidden = true;
	commands[kcSelectCol].isDummy = false;
	commands[kcSelectCol].Message = "Select channel / select all";
*/

	commands[kcPatternJumpDownh2].UID = 1338;
	commands[kcPatternJumpDownh2].isHidden = false;
	commands[kcPatternJumpDownh2].isDummy = false;
	commands[kcPatternJumpDownh2].Message = "Jump down by beat";

	commands[kcPatternJumpUph2].UID = 1339;
	commands[kcPatternJumpUph2].isHidden = false;
	commands[kcPatternJumpUph2].isDummy = false;
	commands[kcPatternJumpUph2].Message = "Jump up by beat";

	commands[kcPatternSnapDownh2].UID = 1340;
	commands[kcPatternSnapDownh2].isHidden = false;
	commands[kcPatternSnapDownh2].isDummy = false;
	commands[kcPatternSnapDownh2].Message = "Snap down to beat";

	commands[kcPatternSnapUph2].UID = 1341;
	commands[kcPatternSnapUph2].isHidden = false;
	commands[kcPatternSnapUph2].isDummy = false;
	commands[kcPatternSnapUph2].Message = "Snap up to beat";


	commands[kcPatternJumpDownh2Select].UID = 1342;
	commands[kcPatternJumpDownh2Select].isHidden = true;
	commands[kcPatternJumpDownh2Select].isDummy = false;
	commands[kcPatternJumpDownh2Select].Message = "kcPatternJumpDownh2Select";

	commands[kcPatternJumpUph2Select].UID = 1343;
	commands[kcPatternJumpUph2Select].isHidden = true;
	commands[kcPatternJumpUph2Select].isDummy = false;
	commands[kcPatternJumpUph2Select].Message = "kcPatternJumpUph2Select";

	commands[kcPatternSnapDownh2Select].UID = 1344;
	commands[kcPatternSnapDownh2Select].isHidden = true;
	commands[kcPatternSnapDownh2Select].isDummy = false;
	commands[kcPatternSnapDownh2Select].Message = "kcPatternSnapDownh2Select";

	commands[kcPatternSnapUph2Select].UID = 1345;
	commands[kcPatternSnapUph2Select].isHidden = true;
	commands[kcPatternSnapUph2Select].isDummy = false;
	commands[kcPatternSnapUph2Select].Message = "kcPatternSnapUph2Select";

	commands[kcFileOpen].UID = 1346;
	commands[kcFileOpen].isHidden = false;
	commands[kcFileOpen].isDummy = false;
	commands[kcFileOpen].Message = "File/Open";

	commands[kcFileNew].UID = 1347;
	commands[kcFileNew].Message = "File/New";
	commands[kcFileNew].isHidden = false;
	commands[kcFileNew].isDummy = false;

	commands[kcFileClose].UID = 1348;
	commands[kcFileClose].Message = "File/Close";
	commands[kcFileClose].isHidden = false;
	commands[kcFileClose].isDummy = false;

	commands[kcFileSave].UID = 1349;
	commands[kcFileSave].Message = "File/Save";
	commands[kcFileSave].isHidden = false;
	commands[kcFileSave].isDummy = false;

	commands[kcFileSaveAs].UID = 1350;
	commands[kcFileSaveAs].Message = "File/Save As";
	commands[kcFileSaveAs].isHidden = false;
	commands[kcFileSaveAs].isDummy = false;

	commands[kcFileSaveAsWave].UID = 1351;
	commands[kcFileSaveAsWave].Message = "File/Save as Wave";
	commands[kcFileSaveAsWave].isHidden = false;
	commands[kcFileSaveAsWave].isDummy = false;

	commands[kcFileSaveAsMP3].UID = 1352;
	commands[kcFileSaveAsMP3].Message = "File/Save as MP3";
	commands[kcFileSaveAsMP3].isHidden = false;
	commands[kcFileSaveAsMP3].isDummy = false;

	commands[kcFileSaveMidi].UID = 1353;
	commands[kcFileSaveMidi].Message = "File/Export to Midi";
	commands[kcFileSaveMidi].isHidden = false;
	commands[kcFileSaveMidi].isDummy = false;

	commands[kcFileImportMidiLib].UID = 1354;
	commands[kcFileImportMidiLib].Message = "File/Import Midi Lib";
	commands[kcFileImportMidiLib].isHidden = false;
	commands[kcFileImportMidiLib].isDummy = false;

	commands[kcFileAddSoundBank].UID = 1355;
	commands[kcFileAddSoundBank].Message = "File/Add Sound Bank";
	commands[kcFileAddSoundBank].isHidden = false;
	commands[kcFileAddSoundBank].isDummy = false;


	commands[kcStopSong].UID = 1356;
	commands[kcStopSong].Message = "Stop Song";
	commands[kcStopSong].isHidden = false;
	commands[kcStopSong].isDummy = false;

	commands[kcMidiRecord].UID = 1357;
	commands[kcMidiRecord].Message = "Toggle Midi Record";
	commands[kcMidiRecord].isHidden = false;
	commands[kcMidiRecord].isDummy = false;

	commands[kcEstimateSongLength].UID = 1358;
	commands[kcEstimateSongLength].Message = "Estimate Song Length";
	commands[kcEstimateSongLength].isHidden = false;
	commands[kcEstimateSongLength].isDummy = false;


	commands[kcEditUndo].UID = 1359;
	commands[kcEditUndo].Message = "Undo";
	commands[kcEditUndo].isHidden = false;
	commands[kcEditUndo].isDummy = false;

	commands[kcEditCut].UID = 1360;
	commands[kcEditCut].Message = "Cut";
	commands[kcEditCut].isHidden = false;
	commands[kcEditCut].isDummy = false;

	commands[kcEditCopy].UID = 1361;
	commands[kcEditCopy].Message = "Copy";
	commands[kcEditCopy].isHidden = false;
	commands[kcEditCopy].isDummy = false;

	commands[kcEditPaste].UID = 1362;
	commands[kcEditPaste].Message = "Paste";
	commands[kcEditPaste].isHidden = false;
	commands[kcEditPaste].isDummy = false;

	commands[kcEditMixPaste].UID = 1363;
	commands[kcEditMixPaste].Message = "Mix Paste";
	commands[kcEditMixPaste].isHidden = false;
	commands[kcEditMixPaste].isDummy = false;

	commands[kcEditSelectAll].UID = 1364;
	commands[kcEditSelectAll].Message = "SelectAll";
	commands[kcEditSelectAll].isHidden = false;
	commands[kcEditSelectAll].isDummy = false;

	commands[kcEditFind].UID = 1365;
	commands[kcEditFind].Message = "Find";
	commands[kcEditFind].isHidden = false;
	commands[kcEditFind].isDummy = false;

	commands[kcEditFindNext].UID = 1366;
	commands[kcEditFindNext].Message = "Find Next";
	commands[kcEditFindNext].isHidden = false;
	commands[kcEditFindNext].isDummy = false;


	commands[kcViewMain].UID = 1367;
	commands[kcViewMain].Message = "Toggle Main View";
	commands[kcViewMain].isHidden = false;
	commands[kcViewMain].isDummy = false;

	commands[kcViewTree].UID = 1368;
	commands[kcViewTree].Message = "Toggle Tree View";
	commands[kcViewTree].isHidden = false;
	commands[kcViewTree].isDummy = false;

	commands[kcViewOptions].UID = 1369;
	commands[kcViewOptions].Message = "View Options";
	commands[kcViewOptions].isHidden = false;
	commands[kcViewOptions].isDummy = false;

	commands[kcHelp].UID = 1370;
	commands[kcHelp].Message = "Help (to do)";
	commands[kcHelp].isHidden = false;
	commands[kcHelp].isDummy = false;

/*
	commands[kcWindowNew].UID = 1370;
	commands[kcWindowNew].Message = "New Window";
	commands[kcWindowNew].isHidden = false;
	commands[kcWindowNew].isDummy = false;

	commands[kcWindowCascade].UID = 1371;
	commands[kcWindowCascade].Message = "Cascade Windows";
	commands[kcWindowCascade].isHidden = false;
	commands[kcWindowCascade].isDummy = false;

	commands[kcWindowTileHorz].UID = 1372;
	commands[kcWindowTileHorz].Message = "Tile Windows Horizontally";
	commands[kcWindowTileHorz].isHidden = false;
	commands[kcWindowTileHorz].isDummy = false;

	commands[kcWindowTileVert].UID = 1373;
	commands[kcWindowTileVert].Message = "Tile Windows Vertically";
	commands[kcWindowTileVert].isHidden = false;
	commands[kcWindowTileVert].isDummy = false;
*/
	commands[kcEstimateSongLength].UID = 1374;
	commands[kcEstimateSongLength].Message = "Estimate Song Length";
	commands[kcEstimateSongLength].isHidden = false;
	commands[kcEstimateSongLength].isDummy = false;

	commands[kcStopSong].UID = 1375;
	commands[kcStopSong].Message = "Stop Song";
	commands[kcStopSong].isHidden = false;
	commands[kcStopSong].isDummy = false;

	commands[kcMidiRecord].UID = 1376;
	commands[kcMidiRecord].Message = "Toggle Midi Record";
	commands[kcMidiRecord].isHidden = false;
	commands[kcMidiRecord].isDummy = false;

	commands[kcDeleteAllRows].UID = 1377;
	commands[kcDeleteAllRows].Message = "Delete all rows";
	commands[kcDeleteAllRows].isHidden = false;
	commands[kcDeleteAllRows].isDummy = false;

	commands[kcInsertRow].UID = 1378;
	commands[kcInsertRow].Message = "Insert Row";
	commands[kcInsertRow].isHidden = false;
	commands[kcInsertRow].isDummy = false;

	commands[kcInsertAllRows].UID = 1379;
	commands[kcInsertAllRows].Message = "Insert All Rows";
	commands[kcInsertAllRows].isHidden = false;
	commands[kcInsertAllRows].isDummy = false;

	commands[kcSampleTrim].UID = 1380;
	commands[kcSampleTrim].Message = "Trim sample around loop points";
	commands[kcSampleTrim].isHidden = false;
	commands[kcSampleTrim].isDummy = false;

	commands[kcSampleReverse].UID = 1381;
	commands[kcSampleReverse].Message = "Reverse sample";
	commands[kcSampleReverse].isHidden = false;
	commands[kcSampleReverse].isDummy = false;

	commands[kcSampleDelete].UID = 1382;
	commands[kcSampleDelete].Message = "Delete sample selection";
	commands[kcSampleDelete].isHidden = false;
	commands[kcSampleDelete].isDummy = false;

	commands[kcSampleSilence].UID = 1383;
	commands[kcSampleSilence].Message = "Silence sample selection";
	commands[kcSampleSilence].isHidden = false;
	commands[kcSampleSilence].isDummy = false;

	commands[kcSampleNormalize].UID = 1384;
	commands[kcSampleNormalize].Message = "Normalise Sample";
	commands[kcSampleNormalize].isHidden = false;
	commands[kcSampleNormalize].isDummy = false;

	commands[kcSampleAmplify].UID = 1385;
	commands[kcSampleAmplify].Message = "Amplify Sample";
	commands[kcSampleAmplify].isHidden = false;
	commands[kcSampleAmplify].isDummy = false;

	commands[kcSampleZoomUp].UID = 1386;
	commands[kcSampleZoomUp].Message = "Zoom Out";
	commands[kcSampleZoomUp].isHidden = false;
	commands[kcSampleZoomUp].isDummy = false;

	commands[kcSampleZoomDown].UID = 1387;
	commands[kcSampleZoomDown].Message = "Zoom In";
	commands[kcSampleZoomDown].isHidden = false;
	commands[kcSampleZoomDown].isDummy = false;

	//time saving HACK:
	for (int j=kcSampStartNotes; j<=kcInsNoteMapEndNoteStops; j++)
	{
		commands[j].UID = 1388+j-kcSampStartNotes;
		commands[j].Message = "Auto Note in some context";
		commands[j].isHidden = true;
		commands[j].isDummy = false;
	}
	//end hack

	commands[kcPatternGrowSelection].UID = 1660;
	commands[kcPatternGrowSelection].Message = "Grow selection";
	commands[kcPatternGrowSelection].isHidden = false;
	commands[kcPatternGrowSelection].isDummy = false;

	commands[kcPatternShrinkSelection].UID = 1661;
	commands[kcPatternShrinkSelection].Message = "Shrink selection";
	commands[kcPatternShrinkSelection].isHidden = false;
	commands[kcPatternShrinkSelection].isDummy = false;

	commands[kcTogglePluginEditor].UID = 1662;
	commands[kcTogglePluginEditor].isHidden = false;
	commands[kcTogglePluginEditor].isDummy = false;
	commands[kcTogglePluginEditor].Message = "Toggle channel's plugin editor";

	commands[kcToggleFollowSong].UID = 1663;
	commands[kcToggleFollowSong].isHidden = false;
	commands[kcToggleFollowSong].isDummy = false;
	commands[kcToggleFollowSong].Message = "Toggle follow song";

	commands[kcClearFieldITStyle].UID = 1664;
	commands[kcClearFieldITStyle].isHidden = false;
	commands[kcClearFieldITStyle].isDummy = false;
	commands[kcClearFieldITStyle].Message = "Clear field (IT Style)";

	commands[kcClearFieldStepITStyle].UID = 1665;
	commands[kcClearFieldStepITStyle].isHidden = false;
	commands[kcClearFieldStepITStyle].isDummy = false;
	commands[kcClearFieldStepITStyle].Message = "Clear field and step (IT Style)";

	commands[kcSetFXextension].UID = 1666;
	commands[kcSetFXextension].isHidden = false;
	commands[kcSetFXextension].isDummy = false;
	commands[kcSetFXextension].Message = "FX parameter extension command";
	

	commands[kcNoteCutOld].UID = 1667;
	commands[kcNoteCutOld].isHidden = false;
	commands[kcNoteCutOld].isDummy = false;
	commands[kcNoteCutOld].Message = "Note cut (don't remember instrument)";

	commands[kcNoteOffOld].UID = 1668;
	commands[kcNoteOffOld].isHidden = false;
	commands[kcNoteOffOld].isDummy = false;
	commands[kcNoteOffOld].Message = "Note off (don't remember instrument)";

	commands[kcViewAddPlugin].UID = 1669;
	commands[kcViewAddPlugin].Message = "View Plugin Manager";
	commands[kcViewAddPlugin].isHidden = false;
	commands[kcViewAddPlugin].isDummy = false;

	commands[kcViewChannelManager].UID = 1670;
	commands[kcViewChannelManager].Message = "View Channel Manager";
	commands[kcViewChannelManager].isHidden = false;
	commands[kcViewChannelManager].isDummy = false;

	commands[kcCopyAndLoseSelection].UID = 1671;
	commands[kcCopyAndLoseSelection].Message = "Copy and lose selection";
	commands[kcCopyAndLoseSelection].isHidden = false;
	commands[kcCopyAndLoseSelection].isDummy = false;
	
	commands[kcNewPattern].UID = 1672;
	commands[kcNewPattern].isHidden = false;
	commands[kcNewPattern].isDummy = false;
	commands[kcNewPattern].Message = "Insert new pattern";

	commands[kcSampleLoad].UID = 1673;
	commands[kcSampleLoad].isHidden = false;
	commands[kcSampleLoad].isDummy = false;
	commands[kcSampleLoad].Message = "Load a Sample";

	commands[kcSampleSave].UID = 1674;
	commands[kcSampleSave].isHidden = false;
	commands[kcSampleSave].isDummy = false;
	commands[kcSampleSave].Message = "Save Sample";

	commands[kcSampleNew].UID = 1675;
	commands[kcSampleNew].isHidden = false;
	commands[kcSampleNew].isDummy = false;
	commands[kcSampleNew].Message = "New Sample";


	commands[kcSampleCtrlLoad].UID = 1676;
	commands[kcSampleCtrlLoad].isHidden = true;
	commands[kcSampleCtrlLoad].isDummy = false;
	commands[kcSampleCtrlLoad].Message = "Load a Sample";

	commands[kcSampleCtrlSave].UID = 1677;
	commands[kcSampleCtrlSave].isHidden = true;
	commands[kcSampleCtrlSave].isDummy = false;
	commands[kcSampleCtrlSave].Message = "Save Sample";

	commands[kcSampleCtrlNew].UID = 1678;
	commands[kcSampleCtrlNew].isHidden = true;
	commands[kcSampleCtrlNew].isDummy = false;
	commands[kcSampleCtrlNew].Message = "New Sample";

	commands[kcInstrumentLoad].UID = 1679;
	commands[kcInstrumentLoad].isHidden = true;
	commands[kcInstrumentLoad].isDummy = false;
	commands[kcInstrumentLoad].Message = "Load an instrument";

	commands[kcInstrumentSave].UID = 1680;
	commands[kcInstrumentSave].isHidden = true;
	commands[kcInstrumentSave].isDummy = false;
	commands[kcInstrumentSave].Message = "Save instrument";

	commands[kcInstrumentNew].UID = 1681;
	commands[kcInstrumentNew].isHidden = true;
	commands[kcInstrumentNew].isDummy = false;
	commands[kcInstrumentNew].Message = "New instrument";

	commands[kcInstrumentCtrlLoad].UID = 1682;
	commands[kcInstrumentCtrlLoad].isHidden = true;
	commands[kcInstrumentCtrlLoad].isDummy = false;
	commands[kcInstrumentCtrlLoad].Message = "Load an instrument";

	commands[kcInstrumentCtrlSave].UID = 1683;
	commands[kcInstrumentCtrlSave].isHidden = true;
	commands[kcInstrumentCtrlSave].isDummy = false;
	commands[kcInstrumentCtrlSave].Message = "Save instrument";

	commands[kcInstrumentCtrlNew].UID = 1684;
	commands[kcInstrumentCtrlNew].isHidden = true;
	commands[kcInstrumentCtrlNew].isDummy = false;
	commands[kcInstrumentCtrlNew].Message = "New instrument";

	commands[kcSwitchToOrderList].UID = 1685;
	commands[kcSwitchToOrderList].isHidden = false;
	commands[kcSwitchToOrderList].isDummy = false;
	commands[kcSwitchToOrderList].Message = "Switch to order list";

	commands[kcEditMixPasteITStyle].UID = 1686;
	commands[kcEditMixPasteITStyle].Message = "Mix Paste (old IT Style)";
	commands[kcEditMixPasteITStyle].isHidden = false;
	commands[kcEditMixPasteITStyle].isDummy = false;

	commands[kcApproxRealBPM].UID = 1687;
	commands[kcApproxRealBPM].Message = "Show approx. real BPM";
	commands[kcApproxRealBPM].isHidden = false;
	commands[kcApproxRealBPM].isDummy = false;

	commands[kcNavigateDownBySpacingSelect].UID = 1689;
	commands[kcNavigateDownBySpacingSelect].isHidden = true;
	commands[kcNavigateDownBySpacingSelect].isDummy = false;
	commands[kcNavigateDownBySpacingSelect].Message = "kcNavigateDownBySpacingSelect";

	commands[kcNavigateUpBySpacingSelect].UID = 1690;
	commands[kcNavigateUpBySpacingSelect].isHidden = true;
	commands[kcNavigateUpBySpacingSelect].isDummy = false;
	commands[kcNavigateUpBySpacingSelect].Message = "kcNavigateUpBySpacingSelect";

	commands[kcNavigateDownBySpacing].UID = 1691;
	commands[kcNavigateDownBySpacing].isHidden = false;
	commands[kcNavigateDownBySpacing].isDummy = false;
	commands[kcNavigateDownBySpacing].Message = "Navigate down by spacing";

	commands[kcNavigateUpBySpacing].UID = 1692;
	commands[kcNavigateUpBySpacing].isHidden = false;
	commands[kcNavigateUpBySpacing].isDummy = false;
	commands[kcNavigateUpBySpacing].Message = "Navigate up by spacing";

	commands[kcPrevDocument].UID = 1693;
	commands[kcPrevDocument].Message = "Previous Document";
	commands[kcPrevDocument].isHidden = false;
	commands[kcPrevDocument].isDummy = false;
	
	commands[kcNextDocument].UID = 1694;
	commands[kcNextDocument].Message = "Next Document";
	commands[kcNextDocument].isHidden = false;
	commands[kcNextDocument].isDummy = false;


	//time saving HACK:
	for (int j=kcVSTGUIStartNotes; j<=kcVSTGUINoteStopA_3; j++)
	{
		commands[j].UID = 1695+j-kcVSTGUIStartNotes;
		commands[j].Message = "Auto Note in some context";
		commands[j].isHidden = true;
		commands[j].isDummy = false;
	}
	//end hack

	commands[kcVSTGUIPrevPreset].UID = 1763;
	commands[kcVSTGUIPrevPreset].Message = "Previous plugin preset";
	commands[kcVSTGUIPrevPreset].isHidden = false;
	commands[kcVSTGUIPrevPreset].isDummy = false;

	commands[kcVSTGUINextPreset].UID = 1764;
	commands[kcVSTGUINextPreset].Message = "Next plugin preset";
	commands[kcVSTGUINextPreset].isHidden = false;
	commands[kcVSTGUINextPreset].isDummy = false;

	commands[kcVSTGUIRandParams].UID = 1765;
	commands[kcVSTGUIRandParams].Message = "Randomize plugin parameters";
	commands[kcVSTGUIRandParams].isHidden = false;
	commands[kcVSTGUIRandParams].isDummy = false;

	commands[kcPatternGoto].UID = 1766;
	commands[kcPatternGoto].Message = "Go to row/channel/...";
	commands[kcPatternGoto].isHidden = false;
	commands[kcPatternGoto].isDummy = false;

	commands[kcPatternOpenRandomizer].UID = 1767;
	commands[kcPatternOpenRandomizer].isHidden = false;
	commands[kcPatternOpenRandomizer].isDummy = false;
	commands[kcPatternOpenRandomizer].Message = "Open pattern randomizer";

	commands[kcPatternInterpolateNote].UID = 1768;
	commands[kcPatternInterpolateNote].isHidden = false;
	commands[kcPatternInterpolateNote].isDummy = false;
	commands[kcPatternInterpolateNote].Message = "Interpolate note";

	//rewbs.graph
	commands[kcViewGraph].UID = 1769;
	commands[kcViewGraph].isHidden = false;
	commands[kcViewGraph].isDummy = false;
	commands[kcViewGraph].Message = "View Graph";
	//end rewbs.graph

	commands[kcToggleChanMuteOnPatTransition].UID = 1770;
	commands[kcToggleChanMuteOnPatTransition].isHidden = false;
	commands[kcToggleChanMuteOnPatTransition].isDummy = false;
	commands[kcToggleChanMuteOnPatTransition].Message = "(Un)mute chan on pat transition";

	commands[kcChannelUnmuteAll].UID = 1771;
	commands[kcChannelUnmuteAll].isHidden = false;
	commands[kcChannelUnmuteAll].isDummy = false;
	commands[kcChannelUnmuteAll].Message = "Unmute all channels";

	commands[kcShowPatternProperties].UID = 1772;
	commands[kcShowPatternProperties].isHidden = false;
	commands[kcShowPatternProperties].isDummy = false;
	commands[kcShowPatternProperties].Message = "Show pattern properties window";

	commands[kcShowMacroConfig].UID = 1773;
	commands[kcShowMacroConfig].isHidden = false;
	commands[kcShowMacroConfig].isDummy = false;
	commands[kcShowMacroConfig].Message = "Show macro configuration";

	commands[kcViewSongProperties].UID = 1775;
	commands[kcViewSongProperties].isHidden = false;
	commands[kcViewSongProperties].isDummy = false;
	commands[kcViewSongProperties].Message = "Show song properties window";
	
	commands[kcChangeLoopStatus].UID = 1776;
	commands[kcChangeLoopStatus].isHidden = false;
	commands[kcChangeLoopStatus].isDummy = false;
	commands[kcChangeLoopStatus].Message = "Toggle loop pattern";

	commands[kcFileExportCompat].UID = 1777;
	commands[kcFileExportCompat].Message = "File/Export to standard IT/XM";
	commands[kcFileExportCompat].isHidden = false;
	commands[kcFileExportCompat].isDummy = false;

	commands[kcUnmuteAllChnOnPatTransition].UID = 1778;
	commands[kcUnmuteAllChnOnPatTransition].isHidden = false;
	commands[kcUnmuteAllChnOnPatTransition].isDummy = false;
	commands[kcUnmuteAllChnOnPatTransition].Message = "Unmute all channels on pattern transition";

	commands[kcSoloChnOnPatTransition].UID = 1779;
	commands[kcSoloChnOnPatTransition].isHidden = false;
	commands[kcSoloChnOnPatTransition].isDummy = false;
	commands[kcSoloChnOnPatTransition].Message = "Solo channel on pattern transition";

	commands[kcTimeAtRow].UID = 1780;
	commands[kcTimeAtRow].isHidden = false;
	commands[kcTimeAtRow].isDummy = false;
	commands[kcTimeAtRow].Message = "Show playback time at current row";

	commands[kcViewMIDImapping].UID = 1781;
	commands[kcViewMIDImapping].isHidden = false;
	commands[kcViewMIDImapping].isDummy = false;
	commands[kcViewMIDImapping].Message = "View MIDI mapping";

	commands[kcVSTGUIPrevPresetJump].UID = 1782;
	commands[kcVSTGUIPrevPresetJump].isHidden = false;
	commands[kcVSTGUIPrevPresetJump].isDummy = false;
	commands[kcVSTGUIPrevPresetJump].Message = "Plugin preset backward jump";

	commands[kcVSTGUINextPresetJump].UID = 1783;
	commands[kcVSTGUINextPresetJump].isHidden = false;
	commands[kcVSTGUINextPresetJump].isDummy = false;
	commands[kcVSTGUINextPresetJump].Message = "Plugin preset forward jump";

	commands[kcNotePC].UID = 1784;
	commands[kcNotePC].isHidden = false;
	commands[kcNotePC].isDummy = false;
	commands[kcNotePC].Message = "Parameter control(MPTm only)";

	commands[kcNotePCS].UID = 1785;
	commands[kcNotePCS].isHidden = false;
	commands[kcNotePCS].isDummy = false;
	commands[kcNotePCS].Message = "Parameter control(smooth)(MPTm only)";

	#ifdef _DEBUG
	for (int i=0; i<kcNumCommands; i++)	{
		if (commands[i].UID != 0) {	// ignore unset UIDs
			for (int j=i+1; j<kcNumCommands; j++) {
				if (commands[i].UID == commands[j].UID) {
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
	bool removing = !adding; //for attempt to salvage readability.. 
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
		// When we get a new slection key, we need to make sure that 
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
		}		

	}
	
	//if we add a selector or a copy selector, we need it to switch off when we release the key.
	if (enforceRule[krAutoSelectOff])
	{
		KeyCombination newKcDeSel;
		bool ruleApplies=true;
		CommandID cmdOff;

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

		for (UINT k=0; k<commands[cmd].kcList.GetSize(); k++)
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
			if (curKc.ctx == kCtxViewPatterns) {
				contexts.Add(kCtxViewPatternsNote);
				contexts.Add(kCtxViewPatternsIns);
				contexts.Add(kCtxViewPatternsVol);
				contexts.Add(kCtxViewPatternsFX);
				contexts.Add(kCtxViewPatternsFXparam);
			}
			else {
				contexts.Add(curKc.ctx);
			}

			long label = 0;
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
	BYTE keCode   = (BYTE)ke;

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
		::MessageBox(NULL, "Can't open file for writing.", "", MB_ICONEXCLAMATION|MB_OK);
		return false;
	}
	fprintf(outStream, "//-------- OpenMPT key binding definition file  -------\n"); 
	fprintf(outStream, "//-Format is:                                                          -\n"); 	
	fprintf(outStream, "//- Context:Command ID:Modifiers:Key:KeypressEventType     //Comments  -\n"); 
	fprintf(outStream, "//----------------------------------------------------------------------\n"); 

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

bool CCommandSet::LoadFile(CString fileName)
{

	FILE *inStream;
	KeyCombination kc;
	CommandID cmd=kcNumCommands;
	char s[1024];
	CString curLine, token;
	int commentStart;
	CCommandSet *pTempCS;
	int l=0;

	pTempCS = new CCommandSet();
	

	if( (inStream  = fopen( fileName, "r" )) == NULL )
	{
		::MessageBox(NULL, "Can't open file keyboard config file " + fileName + "  for reading.", "", MB_ICONEXCLAMATION|MB_OK);
		delete pTempCS;
		return false;
	}

	int errorCount=0;
	while(fgets(s,1024,inStream))
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
			//ctx:UID:Description:Modifier:Key:EventMask
			int spos = 0;

			//context
			token=curLine.Tokenize(":",spos);
			kc.ctx = (InputTargetContext) atoi(token);

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

			//Error checking (TODO):
			if (cmd<0 || cmd>=kcNumCommands || spos==-1)
			{
				errorCount++;
				CString err;
				if (errorCount<10) {
					err.Format("Line %d in key binding file %s was not understood.", l, fileName);
					if(s_bShowErrorOnUnknownKeybinding) ::MessageBox(NULL, err, "", MB_ICONEXCLAMATION|MB_OK);
					Log(err);
				} else if (errorCount==10) {
					err.Format("Too many errors detected, not reporting any more.");
					if(s_bShowErrorOnUnknownKeybinding) ::MessageBox(NULL, err, "", MB_ICONEXCLAMATION|MB_OK);
					Log(err);
				}
			}
			else
			{
				pTempCS->Add(kc, cmd, true);
			}

		}

		l++;
	}

	Copy(pTempCS);
	delete pTempCS;

	return true;
}



//Could do better search algo but this is not perf critical.
int CCommandSet::FindCmd(int uid)
{
	for (int i=0; i<kcNumCommands; i++)
	{
		if (commands[i].UID == uid)
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
	if (key < commands[c].kcList.GetSize())
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
	//parentList.RemoveAll();

	//for (InputTargetContext parent; parent<kCtxMaxInputContexts; parent++) {
	//	if (m_isParentContext[child][parent]) {
	//		parentList.Add(parent);
	//	}
	//}
}

void CCommandSet::GetChildContexts(InputTargetContext parent, CArray<InputTargetContext, InputTargetContext> childList)
{
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

	m_isParentContext[kCtxViewPatternsNote][kCtxViewPatterns] = true;
	m_isParentContext[kCtxViewPatternsIns][kCtxViewPatterns] = true;
	m_isParentContext[kCtxViewPatternsVol][kCtxViewPatterns] = true;
	m_isParentContext[kCtxViewPatternsFX][kCtxViewPatterns] = true;
	m_isParentContext[kCtxViewPatternsFXparam][kCtxViewPatterns] = true;

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
