#include "stdafx.h"
#include ".\CommandSet.h"
#include ".\InputHandler.h"
#include ".\keyboardsettings.h"


CKeyboardSettings::CKeyboardSettings(CInputHandler *ih)
{
	pInputHandler=ih;
}

CKeyboardSettings::~CKeyboardSettings(void)
{
}

int CKeyboardSettings::AsciiToScancode(char ch)
{
	HKL hkl = GetKeyboardLayout(0);
	BYTE test =  VkKeyScanEx(ch, hkl) & 0xff;
	return test;
}
void CKeyboardSettings::LoadDefaults(CInputHandler *inputHandler)
{
	/*
	//Auto reassign:
	//Set octave from Note field - Should be restored if keys are free'd up.
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave0, 0, '0', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave0, 0, VK_NUMPAD0, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave1, 0, '1', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave1, 0, VK_NUMPAD1, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave2, 0, '2', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave2, 0, VK_NUMPAD2, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave3, 0, '3', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave3, 0, VK_NUMPAD3, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave4, 0, '4', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave4, 0, VK_NUMPAD4, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave5, 0, '5', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave5, 0, VK_NUMPAD5, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave6, 0, '6', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave6, 0, VK_NUMPAD6, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave7, 0, '7', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave7, 0, VK_NUMPAD7, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave8, 0, '8', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave8, 0, VK_NUMPAD8, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave9, 0, '9', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcSetOctave9, 0, VK_NUMPAD9, kKeyEventDown);


    
	//GENERAL
	inputHandler->SetCommand(kCtxAllContexts, kcViewGeneral, 0, VK_F11, kKeyEventDown);
	inputHandler->SetCommand(kCtxAllContexts, kcViewGeneral, 0, VK_F11, kKeyEventDown);
	inputHandler->SetCommand(kCtxAllContexts, kcViewPattern, 0, VK_F2, kKeyEventDown);
	inputHandler->SetCommand(kCtxAllContexts, kcViewPattern, 0, VK_F3, kKeyEventDown);
	inputHandler->SetCommand(kCtxAllContexts, kcViewSamples, 0, VK_F3, kKeyEventDown);
	inputHandler->SetCommand(kCtxAllContexts, kcViewInstruments, 0, VK_F4, kKeyEventDown);
	inputHandler->SetCommand(kCtxAllContexts, kcViewComments, 0, VK_F12, kKeyEventDown);

	inputHandler->SetCommand(kCtxAllContexts, kcNextInstrument, 0, ']', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxAllContexts, kcPrevInstrument, 0, '[', kKeyEventDown|kKeyEventRepeat);

	inputHandler->SetCommand(kCtxAllContexts, kcNextOrder, HOTKEYF_CONTROL, VK_RIGHT, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxAllContexts, kcPrevOrder, HOTKEYF_CONTROL, VK_LEFT, kKeyEventDown|kKeyEventRepeat);
	
	inputHandler->SetCommand(kCtxAllContexts, kcNextOctave, 0, VK_MULTIPLY, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxAllContexts, kcPrevOctave, 0, VK_DIVIDE, kKeyEventDown|kKeyEventRepeat);

	inputHandler->SetCommand(kCtxAllContexts, kcPlaySong, HOTKEYF_CONTROL, VK_F5, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxAllContexts, kcRestartSong, 0, VK_F5, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxAllContexts, kcPlayPattern, HOTKEYF_CONTROL, VK_F7, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxAllContexts, kcRestartPattern, 0, VK_F6, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxAllContexts, kcPlayPatternNoLoop, 0, VK_F7, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxAllContexts, kcPauseSong, 0, VK_F8, kKeyEventDown|kKeyEventRepeat);
	
	//Pattern Editor Specific
	inputHandler->SetCommand(kCtxViewPatterns, kcPatternRecord,  HOTKEYF_CONTROL, VK_SPACE, kKeyEventDown);		//
	inputHandler->SetCommand(kCtxViewPatterns, kcPatternPlayRow, 0, '8', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcPatternPlayRow, 0, VK_NUMPAD8, kKeyEventDown|kKeyEventRepeat);

	inputHandler->SetCommand(kCtxViewPatterns, kcCursorCopy, 0, VK_RETURN, kKeyEventDown|kKeyEventRepeat);	//
	inputHandler->SetCommand(kCtxViewPatterns, kcCursorPaste, 0, VK_SPACE, kKeyEventDown|kKeyEventRepeat);

	inputHandler->SetCommand(kCtxViewPatterns, kcChannelMute, HOTKEYF_ALT , VK_F9, kKeyEventDown);	//
	inputHandler->SetCommand(kCtxViewPatterns, kcChannelSolo, HOTKEYF_ALT , VK_F10, kKeyEventDown);	//

	inputHandler->SetCommand(kCtxViewPatterns, kcTransposeUp,      HOTKEYF_ALT , 'Q', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcTransposeDown,    HOTKEYF_ALT , 'A', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcTransposeOctUp,   HOTKEYF_ALT|HOTKEYF_CONTROL, 'Q', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcTransposeOctDown, HOTKEYF_ALT|HOTKEYF_CONTROL, 'A', kKeyEventDown|kKeyEventRepeat);

	inputHandler->SetCommand(kCtxViewPatterns, kcSelectColumn, HOTKEYF_ALT , 'L', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcPatternAmplify, HOTKEYF_ALT , 'J', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcPatternSetInstrument, HOTKEYF_ALT , 'S', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcPatternInterpolateVol, HOTKEYF_ALT , 'K', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcPatternInterpolateEffect, HOTKEYF_ALT , 'X', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcPatternVisualizeEffect, HOTKEYF_ALT , 'B', kKeyEventDown);

	//Should be "cloned" automatically to set off when key is released and to cope when other select is pushed
	inputHandler->SetCommand(kCtxViewPatterns, kcSelect, HOTKEYF_SHIFT, VK_SHIFT, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcCopySelect, HOTKEYF_CONTROL, VK_CONTROL, kKeyEventDown);


	// Pattern navigation (should be "cloned" to cope with navigation while selection key is pushed.
	inputHandler->SetCommand(kCtxViewPatterns, kcPatternJumpDown, HOTKEYF_CONTROL, VK_NEXT, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcPatternJumpUp, HOTKEYF_CONTROL, VK_PRIOR, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcPatternSnapDown, 0, VK_NEXT, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcPatternSnapUp, 0, VK_PRIOR, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcNavigateDown, 0, VK_DOWN, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcNavigateUp, 0, VK_UP, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcNavigateLeft, 0, VK_LEFT, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcNavigateRight, 0, VK_RIGHT, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcNavigateNextChan, 0, VK_TAB, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcNavigatePrevChan, HOTKEYF_CONTROL, VK_TAB, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcHomeHorizontal, 0, VK_HOME, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcHomeVertical, HOTKEYF_CONTROL, VK_HOME, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcHomeAbsolute, HOTKEYF_CONTROL|HOTKEYF_ALT , VK_HOME, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcEndHorizontal, 0, VK_END, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcEndVertical, HOTKEYF_CONTROL, VK_END, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcEndAbsolute, HOTKEYF_CONTROL|HOTKEYF_ALT , VK_END, kKeyEventDown);


	inputHandler->SetCommand(kCtxViewPatterns, kcNextPattern, 0, VK_ADD, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcPrevPattern, 0, VK_SUBTRACT, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcWipeSelection, 0,	VK_DELETE, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcWipeRow, HOTKEYF_SHIFT,	'.', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcWipeField, 0,	'.', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcDeleteRows, 0, VK_BACK, kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatterns, kcShowNoteProperties, 0, VK_APPS, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcShowEditMenu, 0, VK_LWIN, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcShowEditMenu, 0, VK_RWIN, kKeyEventDown);

	//Set instrument from pattern
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns0, 0, '0', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns0, 0, VK_NUMPAD0, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns1, 0, '1', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns1, 0, VK_NUMPAD1, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns2, 0, '2', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns2, 0, VK_NUMPAD2, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns3, 0, '3', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns3, 0, VK_NUMPAD3, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns4, 0, '4', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns4, 0, VK_NUMPAD4, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns5, 0, '5', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns5, 0, VK_NUMPAD5, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns6, 0, '6', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns6, 0, VK_NUMPAD6, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns7, 0, '7', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns7, 0, VK_NUMPAD7, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns8, 0, '8', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns8, 0, VK_NUMPAD8, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns9, 0, '9', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsIns, kcSetIns9, 0, VK_NUMPAD9, kKeyEventDown);


	//Notes; note ups and chords should be genn'd automatically
	inputHandler->SetCommand(kCtxViewPatternsNote, kcChordModifier, HOTKEYF_SHIFT, VK_SHIFT, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteC_0, 0, 'Z', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteCS0, 0, 'S', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteD_0, 0, 'X', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteDS0, 0, 'D', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteE_0, 0, 'C', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteF_0, 0, 'V', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteFS0, 0, 'G', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteG_0, 0, 'B', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteGS0, 0, 'H', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteA_1, 0, 'N', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteAS1, 0, 'J', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteB_1, 0, 'M', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteC_1, 0, 'Q', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteCS1, 0, '2', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteD_1, 0, 'W', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteDS1, 0, '3', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteE_1, 0, 'E', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteF_1, 0, 'R', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteFS1, 0, '5', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteG_1, 0, 'T', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteGS1, 0, '6', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteA_2, 0, 'Y', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteAS2, 0, '7', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteB_2, 0, 'U', kKeyEventDown|kKeyEventRepeat);
//	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteC_2, 0, 'I', kKeyEventDown|kKeyEventRepeat);
//	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteCS2, 0, '9', kKeyEventDown|kKeyEventRepeat);
//	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteD_2, 0, 'O', kKeyEventDown|kKeyEventRepeat);
//	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteDS2, 0, '0', kKeyEventDown|kKeyEventRepeat);
//	inputHandler->SetCommand(kCtxViewPatternsNote, kcVPNoteE_2, 0, 'P', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcNoteCut, 0, '`', kKeyEventDown|kKeyEventRepeat);
	inputHandler->SetCommand(kCtxViewPatternsNote, kcNoteOff, 0, '=', kKeyEventDown|kKeyEventRepeat);

	//Vol digits
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume0, 0, '0', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume0, 0, VK_NUMPAD0, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume1, 0, '1', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume1, 0, VK_NUMPAD1, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume2, 0, '2', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume2, 0, VK_NUMPAD2, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume3, 0, '3', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume3, 0, VK_NUMPAD3, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume4, 0, '4', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume4, 0, VK_NUMPAD4, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume5, 0, '5', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume5, 0, VK_NUMPAD5, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume6, 0, '6', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume6, 0, VK_NUMPAD6, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume7, 0, '7', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume7, 0, VK_NUMPAD7, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume8, 0, '8', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume8, 0, VK_NUMPAD8, kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume9, 0, '9', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolume9, 0, VK_NUMPAD9, kKeyEventDown);

	//IT2 Vol commands
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeVol, 0, 'V', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumePan, 0, 'P', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeVolSlideUp, 0, 'C', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeVolSlideDown, 0, 'D', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeFineVolUp, 0, 'A', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeFineVolDown, 0, 'B', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeVibratoSpd, 0, 'U', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeVibrato, 0, 'H', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeXMPanLeft, 0, 'L', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeXMPanRight, 0, 'R', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumePortamento, 0, 'G', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeITPortaUp, 0, 'F', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeITPortaDown, 0, 'E', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeITVelocity, 0, ':', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeITVelocity, HOTKEYF_SHIFT, ':', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsVol, kcSetVolumeITOffset, 0, 'O', kKeyEventDown);	

	//FX Params
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam0, 0, '0', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam0, 0, VK_NUMPAD0, kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam1, 0, '1', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam1, 0, VK_NUMPAD1, kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam2, 0, '2', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam2, 0, VK_NUMPAD2, kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam3, 0, '3', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam3, 0, VK_NUMPAD3, kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam4, 0, '4', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam4, 0, VK_NUMPAD4, kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam5, 0, '5', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam5, 0, VK_NUMPAD5, kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam6, 0, '6', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam6, 0, VK_NUMPAD6, kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam7, 0, '7', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam7, 0, VK_NUMPAD7, kKeyEventDown);		
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam8, 0, '8', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam8, 0, VK_NUMPAD8, kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam9, 0, '9', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParam9, 0, VK_NUMPAD9, kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParamA, 0, 'A', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParamB, 0, 'B', kKeyEventDown);		
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParamC, 0, 'C', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParamD, 0, 'D', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParamE, 0, 'E', kKeyEventDown);	
	inputHandler->SetCommand(kCtxViewPatternsFXparam, kcSetFXParamF, 0, 'F', kKeyEventDown);	

	//IT2 FX
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXarp, 0, 'J', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXportUp, 0, 'F', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXportDown, 0, 'E', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXport, 0, 'G', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXvibrato, 0, 'H', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXportSlide, 0, 'L', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXvibSlide, 0, 'K', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXtremolo, 0, 'R', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXpan, 0, 'X', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXoffset, 0, 'O', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXvolSlide, 0, 'D', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXgotoOrd, 0, 'B', kKeyEventDown);
	//inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXsetVol, 0, '?', kKeyEventDown); XM only
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXgotoRow, 0, 'C', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXretrig, 0, 'Q', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXspeed, 0, 'A', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXtempo, 0, 'T', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXtremor, 0, 'I', kKeyEventDown);
	//inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXextendedMOD, 0, '?', kKeyEventDown); XM only
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXextendedS3M, 0, 'S', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXchannelVol, 0, 'M', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXchannelVols, 0, 'N', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXglobalVol, 0, 'V', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXglobalVols, 0, 'W', kKeyEventDown);
	//inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXkeyoff, 0, '?', kKeyEventDown); XM only
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXfineVib, 0, 'U', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXpanbrello, 0, 'Y', kKeyEventDown);
	//inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXextendedXM, 0, '?', kKeyEventDown); XM only
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXpanSlide, 0, 'P', kKeyEventDown);
	//inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXsetEnvPos, 0, '?', kKeyEventDown); XM only
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXmacro, 0, 'Z', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXmacroSlide, 0, '\\', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXvelocity, 0, ':', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatternsFX, kcSetFXvelocity, HOTKEYF_SHIFT, ':', kKeyEventDown);

	inputHandler->SetCommand(kCtxViewPatterns, kcSetSpacing, HOTKEYF_ALT , '0', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcSetSpacing, HOTKEYF_ALT , '1', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcSetSpacing, HOTKEYF_ALT , '2', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcSetSpacing, HOTKEYF_ALT , '3', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcSetSpacing, HOTKEYF_ALT , '4', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcSetSpacing, HOTKEYF_ALT , '5', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcSetSpacing, HOTKEYF_ALT , '6', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcSetSpacing, HOTKEYF_ALT , '7', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcSetSpacing, HOTKEYF_ALT , '8', kKeyEventDown);
	inputHandler->SetCommand(kCtxViewPatterns, kcSetSpacing, HOTKEYF_ALT , '9', kKeyEventDown);
	*/

	//inputHandler->activeCommandSet->LoadFile("C:\\Documents and Settings\\Robin\\Desktop\\kbd\\myKeyConfigMyChanges.mkb");
}
