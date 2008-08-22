//rewbs.customKeys

#pragma once
#include "afxtempl.h"
#include <string>

//#define VK_ALT 0x12

enum InputTargetContext
{
	kCtxUnknownContext = -1,	
	kCtxAllContexts = 0,

	kCtxViewGeneral,
	kCtxViewPatterns,
	kCtxViewPatternsNote,
	kCtxViewPatternsIns,
	kCtxViewPatternsVol,
	kCtxViewPatternsFX,
	kCtxViewPatternsFXparam,
	kCtxViewSamples,
	kCtxViewInstruments,
	kCtxViewComments,
	kCtxViewTree,
	kCtxInsNoteMap,
	kCtxVSTGUI,

	kCtxCtrlGeneral,
	kCtxCtrlPatterns,
	kCtxCtrlSamples,
	kCtxCtrlInstruments,
	kCtxCtrlComments,
	kCtxCtrlOrderlist,	
	kCtxMaxInputContexts
};

enum KeyEventType
{
	kKeyEventDown	= 1 << 0,
	kKeyEventUp		= 1 << 1,
	kKeyEventRepeat	= 1 << 2,
	kNumKeyEvents	= 1 << 3
};

enum CommandID
{
	kcNull = -1,

	//Global
	kcGlobalStart,
	kcStartFile=kcGlobalStart,
	kcFileNew=kcGlobalStart,
	kcFileOpen,
	kcFileClose,
	kcFileSave,
	kcFileSaveAs,
	kcFileSaveAsWave,
	kcFileSaveAsMP3,
	kcFileSaveMidi,
	kcFileExportCompat,
	kcPrevDocument,
	kcNextDocument,
	kcFileImportMidiLib,
	kcFileAddSoundBank,
	kcEndFile=kcFileAddSoundBank,
	
	kcStartPlayCommands,
	kcPlayPauseSong=kcStartPlayCommands,
	kcPauseSong,
	kcStopSong,
	kcPlaySongFromStart,
	kcPlaySongFromCursor,
	kcPlayPatternFromStart,
	kcPlayPatternFromCursor,
	kcEstimateSongLength,
	kcApproxRealBPM,
	kcMidiRecord,
	kcEndPlayCommands=kcMidiRecord,

	kcStartEditCommands,
	kcEditUndo=kcStartEditCommands,
	kcEditCut,
	kcEditCopy,
	kcEditPaste,
	kcEditMixPaste,
	kcEditMixPasteITStyle,
	kcEditSelectAll,
	kcEditFind,
	kcEditFindNext,
	kcEndEditCommands=kcEditFindNext,
/*
	kcWindowNew,
	kcWindowCascade,
	kcWindowTileHorz,
	kcWindowTileVert,
*/
	kcStartView,
	kcViewGeneral=kcStartView,
	kcViewPattern,
	kcViewSamples,
	kcViewInstruments,
	kcViewComments,
	kcViewGraph,
	kcViewMain,
	kcViewTree,
	kcViewOptions,
	kcViewChannelManager,
	kcViewAddPlugin,
	kcViewSongProperties,
	kcViewMIDImapping,
	kcHelp,
	kcEndView=kcHelp,

	kcStartMisc,
	kcPrevInstrument=kcStartMisc,	
	kcNextInstrument,
	kcPrevOctave,
	kcNextOctave,
	kcPrevOrder,
	kcNextOrder,
    kcEndMisc=kcNextOrder,
	kcGlobalEnd=kcNextOrder,

	//Pattern Navigation
	kcStartPatNavigation,
	kcStartJumpSnap=kcStartPatNavigation,
	kcPatternJumpDownh1=kcStartJumpSnap,
	kcPatternJumpUph1,
	kcPatternJumpDownh2,
	kcPatternJumpUph2,
	kcPatternSnapDownh1,
	kcPatternSnapUph1,
	kcPatternSnapDownh2,
	kcPatternSnapUph2,
	kcEndJumpSnap=kcPatternSnapUph2,
	kcStartPlainNavigate,
	kcNavigateDown=kcStartPlainNavigate,
	kcNavigateUp,
	kcNavigateDownBySpacing,
	kcNavigateUpBySpacing,
	kcNavigateLeft,
	kcNavigateRight,
	kcNavigateNextChan,
	kcNavigatePrevChan,
	kcEndPlainNavigate=kcNavigatePrevChan,
	kcStartHomeEnd,
	kcHomeHorizontal=kcStartHomeEnd,
	kcHomeVertical,
	kcHomeAbsolute,
	kcEndHorizontal,
	kcEndVertical,
	kcEndAbsolute,
	kcEndHomeEnd=kcEndAbsolute,
	kcEndPatNavigation=kcEndHomeEnd,
	kcStartPatNavigationSelect,
		//with select. Order must match above.
	kcPatternJumpDownh1Select=kcStartPatNavigationSelect,
	kcPatternJumpUph1Select,
	kcPatternJumpDownh2Select,
	kcPatternJumpUph2Select,
	kcPatternSnapDownh1Select,
	kcPatternSnapUph1Select,
	kcPatternSnapDownh2Select,
	kcPatternSnapUph2Select,
	kcNavigateDownSelect,
	kcNavigateUpSelect,
	kcNavigateDownBySpacingSelect,
	kcNavigateUpBySpacingSelect,
	kcNavigateLeftSelect,
	kcNavigateRightSelect,
	kcNavigateNextChanSelect,
	kcNavigatePrevChanSelect,
	kcHomeHorizontalSelect,
	kcHomeVerticalSelect,
	kcHomeAbsoluteSelect,
	kcEndHorizontalSelect,
	kcEndVerticalSelect,
	kcEndAbsoluteSelect,
	kcEndPatNavigationSelect=kcEndAbsoluteSelect,

	//Pattern Editing
	kcStartPatternEditing,
    kcStartSelect=kcStartPatternEditing,
	kcSelect=kcStartSelect,
	kcSelectWithNav,
	kcSelectOff,
	kcSelectOffWithNav,
	kcSelectWithCopySelect,
	kcSelectOffWithCopySelect,
	kcCopySelect,
	kcCopySelectWithNav,
	kcCopySelectOff,
	kcCopySelectOffWithNav,
	kcCopySelectWithSelect,
	kcCopySelectOffWithSelect,
	kcSelectColumn,
	kcEndSelect=kcSelectColumn,

	kcStartPatternEditMisc,
	kcToggleFollowSong=kcStartPatternEditMisc,
	kcCursorCopy,
	kcCursorPaste,
	kcPatternRecord,
	kcPatternPlayRow,
	kcSetSpacing,
	kcSetSpacing0,
	kcSetSpacing1,
	kcSetSpacing2,
	kcSetSpacing3,
	kcSetSpacing4,
	kcSetSpacing5,
	kcSetSpacing6,
	kcSetSpacing7,
	kcSetSpacing8,
	kcSetSpacing9,
	kcSwitchToOrderList,
	kcNewPattern,
	kcTogglePluginEditor,
	kcShowNoteProperties,
	kcShowPatternProperties,
	kcShowMacroConfig,
	kcChangeLoopStatus,
	kcShowEditMenu,
	kcTimeAtRow,
	kcEndPatternEditMisc=kcTimeAtRow,

	kcChannelMute,
	kcChannelSolo,
	kcChannelUnmuteAll,
	kcToggleChanMuteOnPatTransition,
	kcUnmuteAllChnOnPatTransition,
	kcSoloChnOnPatTransition,
	kcCopyAndLoseSelection,
	kcTransposeUp,
	kcTransposeDown,
	kcTransposeOctUp,
	kcTransposeOctDown,
	kcPatternAmplify,
	kcPatternInterpolateNote,
	kcPatternInterpolateVol,
	kcPatternInterpolateEffect,
	kcPatternVisualizeEffect,
	kcPatternOpenRandomizer,
	kcPatternGoto,
	kcPatternSetInstrument,
	kcPatternGrowSelection,
	kcPatternShrinkSelection,
	//	kcClearSelection,					
	kcClearRow,				
	kcClearField,
	kcClearFieldITStyle,
	kcClearRowStep,				
	kcClearFieldStep,
	kcClearFieldStepITStyle,
	kcDeleteRows,
	kcDeleteAllRows,
	kcInsertRow,
	kcInsertAllRows,
	kcPrevPattern,
	kcNextPattern,
	kcEndPatternEditing=kcNextPattern,

	//Notes
	kcVPStartNotes,
	kcVPNoteC_0=kcVPStartNotes,
	kcVPNoteCS0,
	kcVPNoteD_0,
	kcVPNoteDS0,
	kcVPNoteE_0,
	kcVPNoteF_0,
	kcVPNoteFS0,
	kcVPNoteG_0,
	kcVPNoteGS0,
	kcVPNoteA_1,
	kcVPNoteAS1,
	kcVPNoteB_1,
	kcVPNoteC_1,
	kcVPNoteCS1,
	kcVPNoteD_1,
	kcVPNoteDS1,
	kcVPNoteE_1,
	kcVPNoteF_1,
	kcVPNoteFS1,
	kcVPNoteG_1,
	kcVPNoteGS1,
	kcVPNoteA_2,
	kcVPNoteAS2,
	kcVPNoteB_2,
	kcVPNoteC_2,
	kcVPNoteCS2,
	kcVPNoteD_2,
	kcVPNoteDS2,
	kcVPNoteE_2,
	kcVPNoteF_2,
	kcVPNoteFS2,
	kcVPNoteG_2,
	kcVPNoteGS2,
	kcVPNoteA_3,
	kcVPEndNotes=kcVPNoteA_3,

	//Note stops
	kcVPStartNoteStops,
	kcVPNoteStopC_0=kcVPStartNoteStops,
	kcVPNoteStopCS0,
	kcVPNoteStopD_0,
	kcVPNoteStopDS0,
	kcVPNoteStopE_0,
	kcVPNoteStopF_0,
	kcVPNoteStopFS0,
	kcVPNoteStopG_0,
	kcVPNoteStopGS0,
	kcVPNoteStopA_1,
	kcVPNoteStopAS1,
	kcVPNoteStopB_1,
	kcVPNoteStopC_1,
	kcVPNoteStopCS1,
	kcVPNoteStopD_1,
	kcVPNoteStopDS1,
	kcVPNoteStopE_1,
	kcVPNoteStopF_1,
	kcVPNoteStopFS1,
	kcVPNoteStopG_1,
	kcVPNoteStopGS1,
	kcVPNoteStopA_2,
	kcVPNoteStopAS2,
	kcVPNoteStopB_2,
	kcVPNoteStopC_2,
	kcVPNoteStopCS2,
	kcVPNoteStopD_2,
	kcVPNoteStopDS2,
	kcVPNoteStopE_2,
	kcVPNoteStopF_2,
	kcVPNoteStopFS2,
	kcVPNoteStopG_2,
	kcVPNoteStopGS2,
	kcVPNoteStopA_3,
	kcVPEndNoteStops=kcVPNoteStopA_3,

	//Chords
	kcVPStartChords,
	kcVPChordC_0=kcVPStartChords,
	kcVPChordCS0,
	kcVPChordD_0,
	kcVPChordDS0,
	kcVPChordE_0,
	kcVPChordF_0,
	kcVPChordFS0,
	kcVPChordG_0,
	kcVPChordGS0,
	kcVPChordA_1,
	kcVPChordAS1,
	kcVPChordB_1,
	kcVPChordC_1,
	kcVPChordCS1,
	kcVPChordD_1,
	kcVPChordDS1,
	kcVPChordE_1,
	kcVPChordF_1,
	kcVPChordFS1,
	kcVPChordG_1,
	kcVPChordGS1,
	kcVPChordA_2,
	kcVPChordAS2,
	kcVPChordB_2,
	kcVPChordC_2,
	kcVPChordCS2,
	kcVPChordD_2,
	kcVPChordDS2,
	kcVPChordE_2,
	kcVPChordF_2,
	kcVPChordFS2,
	kcVPChordG_2,
	kcVPChordGS2,
	kcVPChordA_3,
	kcVPEndChords=kcVPChordA_3,

	//Chord Stops
	kcVPStartChordStops,
	kcVPChordStopC_0=kcVPStartChordStops,
	kcVPChordStopCS0,
	kcVPChordStopD_0,
	kcVPChordStopDS0,
	kcVPChordStopE_0,
	kcVPChordStopF_0,
	kcVPChordStopFS0,
	kcVPChordStopG_0,
	kcVPChordStopGS0,
	kcVPChordStopA_1,
	kcVPChordStopAS1,
	kcVPChordStopB_1,
	kcVPChordStopC_1,
	kcVPChordStopCS1,
	kcVPChordStopD_1,
	kcVPChordStopDS1,
	kcVPChordStopE_1,
	kcVPChordStopF_1,
	kcVPChordStopFS1,
	kcVPChordStopG_1,
	kcVPChordStopGS1,
	kcVPChordStopA_2,
	kcVPChordStopAS2,
	kcVPChordStopB_2,
	kcVPChordStopC_2,
	kcVPChordStopCS2,
	kcVPChordStopD_2,
	kcVPChordStopDS2,
	kcVPChordStopE_2,
	kcVPChordStopF_2,
	kcVPChordStopFS2,
	kcVPChordStopG_2,
	kcVPChordStopGS2,
	kcVPChordStopA_3,
	kcVPEndChordStops=kcVPChordStopA_3,
	
	//Set octave from note column
	kcSetOctave0,
	kcSetOctave1,
	kcSetOctave2,
	kcSetOctave3,
	kcSetOctave4,
	kcSetOctave5,
	kcSetOctave6,
	kcSetOctave7,
	kcSetOctave8,
	kcSetOctave9,
	
	//Note Misc
	kcStartNoteMisc,
	kcChordModifier=kcStartNoteMisc,
	kcNoteCut,
	kcNoteOff,
	kcNoteCutOld,
	kcNoteOffOld,
	kcNotePC,
	kcNotePCS,
	kcEndNoteMisc=kcNotePCS,


	//Set instruments
	kcSetIns0,
	kcSetIns1,
	kcSetIns2,
	kcSetIns3,
	kcSetIns4,
	kcSetIns5,
	kcSetIns6,
	kcSetIns7,
	kcSetIns8,
	kcSetIns9,

	//Volume stuff
	kcSetVolumeStart,
	kcSetVolume0=kcSetVolumeStart,
	kcSetVolume1,
	kcSetVolume2,
	kcSetVolume3,
	kcSetVolume4,
	kcSetVolume5,
	kcSetVolume6,
	kcSetVolume7,
	kcSetVolume8,
	kcSetVolume9,
	kcSetVolumeVol,				//v
	kcSetVolumePan,				//p
	kcSetVolumeVolSlideUp,		//c
	kcSetVolumeVolSlideDown,	//d
	kcSetVolumeFineVolUp,		//a
	kcSetVolumeFineVolDown,		//b
	kcSetVolumeVibratoSpd,		//u
	kcSetVolumeVibrato,			//h
	kcSetVolumeXMPanLeft,		//l
	kcSetVolumeXMPanRight,		//r
	kcSetVolumePortamento,		//g
	kcSetVolumeITPortaUp,		//f
	kcSetVolumeITPortaDown,		//e
	kcSetVolumeITVelocity,		//:
	kcSetVolumeITOffset,		//o
	kcSetVolumeEnd=kcSetVolumeITOffset,		

	//Effect params
	kcSetFXParam0,
	kcSetFXParam1,
	kcSetFXParam2,
	kcSetFXParam3,
	kcSetFXParam4,
	kcSetFXParam5,
	kcSetFXParam6,
	kcSetFXParam7,
	kcSetFXParam8,
	kcSetFXParam9,
	kcSetFXParamA,
	kcSetFXParamB,
	kcSetFXParamC,
	kcSetFXParamD,
	kcSetFXParamE,
	kcSetFXParamF,

	//Effect commands. ORDER IS CRUCIAL.
	kcSetFXStart,
	kcFixedFXStart=kcSetFXStart,
	kcSetFXarp=kcSetFXStart,	//0,j
	kcSetFXportUp,		//1,f
	kcSetFXportDown,	//2,e
	kcSetFXport,		//3,g
	kcSetFXvibrato,		//4,h
	kcSetFXportSlide,	//5,l
	kcSetFXvibSlide,	//6,k
	kcSetFXtremolo,		//7,r
	kcSetFXpan,			//8,x
	kcSetFXoffset,		//9,o
	kcSetFXvolSlide,	//a,d
	kcSetFXgotoOrd,		//b,b
	kcSetFXsetVol,		//c,?
	kcSetFXgotoRow,		//d,c
	kcSetFXretrig,		//r,q
	kcSetFXspeed,		//?,a
	kcSetFXtempo,		//f,t
	kcSetFXtremor,		//t,i
	kcSetFXextendedMOD,	//e,?
	kcSetFXextendedS3M,	//?,s
	kcSetFXchannelVol,	//?,m
	kcSetFXchannelVols,	//?,n
	kcSetFXglobalVol,	//g,v
	kcSetFXglobalVols,	//h,w
	kcSetFXkeyoff,		//k,?
	kcSetFXfineVib,		//?,u
	kcSetFXpanbrello,	//y,y
	kcSetFXextendedXM,	//x,?
	kcSetFXpanSlide,	//p,p
	kcSetFXsetEnvPos,	//l,?
	kcSetFXmacro,		//z,z
	kcFixedFXend=kcSetFXmacro,
	kcSetFXmacroSlide,	//?,\ 
	kcSetFXvelocity,	//?,:
	kcSetFXextension,	//?,#
	kcSetFXEnd=kcSetFXextension,

	kcStartInstrumentMisc,
	kcInstrumentLoad=kcStartInstrumentMisc,
	kcInstrumentSave,
	kcInstrumentNew,
	kcEndInstrumentMisc=kcInstrumentNew,

	kcStartInstrumentCtrlMisc,
	kcInstrumentCtrlLoad=kcStartInstrumentCtrlMisc,
	kcInstrumentCtrlSave,
	kcInstrumentCtrlNew,
	kcEndInstrumentCtrlMisc=kcInstrumentCtrlNew,

	kcStartSampleMisc,
	kcSampleLoad=kcStartSampleMisc,
	kcSampleSave,
	kcSampleNew,
	kcEndSampleMisc=kcSampleNew,

	kcStartSampleCtrlMisc,
	kcSampleCtrlLoad=kcStartSampleCtrlMisc,
	kcSampleCtrlSave,
	kcSampleCtrlNew,
	kcEndSampleCtrlMisc=kcSampleCtrlNew,

	kcStartSampleEditing,
	kcSampleTrim=kcStartSampleEditing,
	kcSampleSilence,
	kcSampleNormalize,
	kcSampleAmplify,
	kcSampleReverse,
	kcSampleDelete,
	kcSampleZoomUp, 
	kcSampleZoomDown, 
	kcEndSampleEditing=kcSampleZoomDown,

	//kcSampStartNotes to kcInsNoteMapEndNoteStops must be contiguous.
	kcSampStartNotes,
	kcSampNoteC_0=kcSampStartNotes,
	kcSampNoteCS0,
	kcSampNoteD_0,
	kcSampNoteDS0,
	kcSampNoteE_0,
	kcSampNoteF_0,
	kcSampNoteFS0,
	kcSampNoteG_0,
	kcSampNoteGS0,
	kcSampNoteA_1,
	kcSampNoteAS1,
	kcSampNoteB_1,
	kcSampNoteC_1,
	kcSampNoteCS1,
	kcSampNoteD_1,
	kcSampNoteDS1,
	kcSampNoteE_1,
	kcSampNoteF_1,
	kcSampNoteFS1,
	kcSampNoteG_1,
	kcSampNoteGS1,
	kcSampNoteA_2,
	kcSampNoteAS2,
	kcSampNoteB_2,
	kcSampNoteC_2,
	kcSampNoteCS2,
	kcSampNoteD_2,
	kcSampNoteDS2,
	kcSampNoteE_2,
	kcSampNoteF_2,
	kcSampNoteFS2,
	kcSampNoteG_2,
	kcSampNoteGS2,
	kcSampNoteA_3,
	kcSampEndNotes=kcSampNoteA_3,

	//Note stops
	kcSampStartNoteStops,
	kcSampNoteStopC_0=kcSampStartNoteStops,
	kcSampNoteStopCS0,
	kcSampNoteStopD_0,
	kcSampNoteStopDS0,
	kcSampNoteStopE_0,
	kcSampNoteStopF_0,
	kcSampNoteStopFS0,
	kcSampNoteStopG_0,
	kcSampNoteStopGS0,
	kcSampNoteStopA_1,
	kcSampNoteStopAS1,
	kcSampNoteStopB_1,
	kcSampNoteStopC_1,
	kcSampNoteStopCS1,
	kcSampNoteStopD_1,
	kcSampNoteStopDS1,
	kcSampNoteStopE_1,
	kcSampNoteStopF_1,
	kcSampNoteStopFS1,
	kcSampNoteStopG_1,
	kcSampNoteStopGS1,
	kcSampNoteStopA_2,
	kcSampNoteStopAS2,
	kcSampNoteStopB_2,
	kcSampNoteStopC_2,
	kcSampNoteStopCS2,
	kcSampNoteStopD_2,
	kcSampNoteStopDS2,
	kcSampNoteStopE_2,
	kcSampNoteStopF_2,
	kcSampNoteStopFS2,
	kcSampNoteStopG_2,
	kcSampNoteStopGS2,
	kcSampNoteStopA_3,
	kcSampEndNoteStops=kcSampNoteStopA_3,

	kcInstrumentStartNotes,
	kcInstrumentNoteC_0=kcInstrumentStartNotes,
	kcInstrumentNoteCS0,
	kcInstrumentNoteD_0,
	kcInstrumentNoteDS0,
	kcInstrumentNoteE_0,
	kcInstrumentNoteF_0,
	kcInstrumentNoteFS0,
	kcInstrumentNoteG_0,
	kcInstrumentNoteGS0,
	kcInstrumentNoteA_1,
	kcInstrumentNoteAS1,
	kcInstrumentNoteB_1,
	kcInstrumentNoteC_1,
	kcInstrumentNoteCS1,
	kcInstrumentNoteD_1,
	kcInstrumentNoteDS1,
	kcInstrumentNoteE_1,
	kcInstrumentNoteF_1,
	kcInstrumentNoteFS1,
	kcInstrumentNoteG_1,
	kcInstrumentNoteGS1,
	kcInstrumentNoteA_2,
	kcInstrumentNoteAS2,
	kcInstrumentNoteB_2,
	kcInstrumentNoteC_2,
	kcInstrumentNoteCS2,
	kcInstrumentNoteD_2,
	kcInstrumentNoteDS2,
	kcInstrumentNoteE_2,
	kcInstrumentNoteF_2,
	kcInstrumentNoteFS2,
	kcInstrumentNoteG_2,
	kcInstrumentNoteGS2,
	kcInstrumentNoteA_3,
	kcInstrumentEndNotes=kcInstrumentNoteA_3,

	//Note stops
	kcInstrumentStartNoteStops,
	kcInstrumentNoteStopC_0=kcInstrumentStartNoteStops,
	kcInstrumentNoteStopCS0,
	kcInstrumentNoteStopD_0,
	kcInstrumentNoteStopDS0,
	kcInstrumentNoteStopE_0,
	kcInstrumentNoteStopF_0,
	kcInstrumentNoteStopFS0,
	kcInstrumentNoteStopG_0,
	kcInstrumentNoteStopGS0,
	kcInstrumentNoteStopA_1,
	kcInstrumentNoteStopAS1,
	kcInstrumentNoteStopB_1,
	kcInstrumentNoteStopC_1,
	kcInstrumentNoteStopCS1,
	kcInstrumentNoteStopD_1,
	kcInstrumentNoteStopDS1,
	kcInstrumentNoteStopE_1,
	kcInstrumentNoteStopF_1,
	kcInstrumentNoteStopFS1,
	kcInstrumentNoteStopG_1,
	kcInstrumentNoteStopGS1,
	kcInstrumentNoteStopA_2,
	kcInstrumentNoteStopAS2,
	kcInstrumentNoteStopB_2,
	kcInstrumentNoteStopC_2,
	kcInstrumentNoteStopCS2,
	kcInstrumentNoteStopD_2,
	kcInstrumentNoteStopDS2,
	kcInstrumentNoteStopE_2,
	kcInstrumentNoteStopF_2,
	kcInstrumentNoteStopFS2,
	kcInstrumentNoteStopG_2,
	kcInstrumentNoteStopGS2,
	kcInstrumentNoteStopA_3,
	kcInstrumentEndNoteStops=kcInstrumentNoteStopA_3,

	kcTreeViewStartNotes,
	kcTreeViewNoteC_0=kcTreeViewStartNotes,
	kcTreeViewNoteCS0,
	kcTreeViewNoteD_0,
	kcTreeViewNoteDS0,
	kcTreeViewNoteE_0,
	kcTreeViewNoteF_0,
	kcTreeViewNoteFS0,
	kcTreeViewNoteG_0,
	kcTreeViewNoteGS0,
	kcTreeViewNoteA_1,
	kcTreeViewNoteAS1,
	kcTreeViewNoteB_1,
	kcTreeViewNoteC_1,
	kcTreeViewNoteCS1,
	kcTreeViewNoteD_1,
	kcTreeViewNoteDS1,
	kcTreeViewNoteE_1,
	kcTreeViewNoteF_1,
	kcTreeViewNoteFS1,
	kcTreeViewNoteG_1,
	kcTreeViewNoteGS1,
	kcTreeViewNoteA_2,
	kcTreeViewNoteAS2,
	kcTreeViewNoteB_2,
	kcTreeViewNoteC_2,
	kcTreeViewNoteCS2,
	kcTreeViewNoteD_2,
	kcTreeViewNoteDS2,
	kcTreeViewNoteE_2,
	kcTreeViewNoteF_2,
	kcTreeViewNoteFS2,
	kcTreeViewNoteG_2,
	kcTreeViewNoteGS2,
	kcTreeViewNoteA_3,
	kcTreeViewEndNotes=kcTreeViewNoteA_3,

	//Note stops
	kcTreeViewStartNoteStops,
	kcTreeViewNoteStopC_0=kcTreeViewStartNoteStops,
	kcTreeViewNoteStopCS0,
	kcTreeViewNoteStopD_0,
	kcTreeViewNoteStopDS0,
	kcTreeViewNoteStopE_0,
	kcTreeViewNoteStopF_0,
	kcTreeViewNoteStopFS0,
	kcTreeViewNoteStopG_0,
	kcTreeViewNoteStopGS0,
	kcTreeViewNoteStopA_1,
	kcTreeViewNoteStopAS1,
	kcTreeViewNoteStopB_1,
	kcTreeViewNoteStopC_1,
	kcTreeViewNoteStopCS1,
	kcTreeViewNoteStopD_1,
	kcTreeViewNoteStopDS1,
	kcTreeViewNoteStopE_1,
	kcTreeViewNoteStopF_1,
	kcTreeViewNoteStopFS1,
	kcTreeViewNoteStopG_1,
	kcTreeViewNoteStopGS1,
	kcTreeViewNoteStopA_2,
	kcTreeViewNoteStopAS2,
	kcTreeViewNoteStopB_2,
	kcTreeViewNoteStopC_2,
	kcTreeViewNoteStopCS2,
	kcTreeViewNoteStopD_2,
	kcTreeViewNoteStopDS2,
	kcTreeViewNoteStopE_2,
	kcTreeViewNoteStopF_2,
	kcTreeViewNoteStopFS2,
	kcTreeViewNoteStopG_2,
	kcTreeViewNoteStopGS2,
	kcTreeViewNoteStopA_3,
	kcTreeViewEndNoteStops=kcTreeViewNoteStopA_3,

	kcInsNoteMapStartNotes,
	kcInsNoteMapNoteC_0=kcInsNoteMapStartNotes,
	kcInsNoteMapNoteCS0,
	kcInsNoteMapNoteD_0,
	kcInsNoteMapNoteDS0,
	kcInsNoteMapNoteE_0,
	kcInsNoteMapNoteF_0,
	kcInsNoteMapNoteFS0,
	kcInsNoteMapNoteG_0,
	kcInsNoteMapNoteGS0,
	kcInsNoteMapNoteA_1,
	kcInsNoteMapNoteAS1,
	kcInsNoteMapNoteB_1,
	kcInsNoteMapNoteC_1,
	kcInsNoteMapNoteCS1,
	kcInsNoteMapNoteD_1,
	kcInsNoteMapNoteDS1,
	kcInsNoteMapNoteE_1,
	kcInsNoteMapNoteF_1,
	kcInsNoteMapNoteFS1,
	kcInsNoteMapNoteG_1,
	kcInsNoteMapNoteGS1,
	kcInsNoteMapNoteA_2,
	kcInsNoteMapNoteAS2,
	kcInsNoteMapNoteB_2,
	kcInsNoteMapNoteC_2,
	kcInsNoteMapNoteCS2,
	kcInsNoteMapNoteD_2,
	kcInsNoteMapNoteDS2,
	kcInsNoteMapNoteE_2,
	kcInsNoteMapNoteF_2,
	kcInsNoteMapNoteFS2,
	kcInsNoteMapNoteG_2,
	kcInsNoteMapNoteGS2,
	kcInsNoteMapNoteA_3,
	kcInsNoteMapEndNotes=kcInsNoteMapNoteA_3,

	//Note stops
	kcInsNoteMapStartNoteStops,
	kcInsNoteMapNoteStopC_0=kcInsNoteMapStartNoteStops,
	kcInsNoteMapNoteStopCS0,
	kcInsNoteMapNoteStopD_0,
	kcInsNoteMapNoteStopDS0,
	kcInsNoteMapNoteStopE_0,
	kcInsNoteMapNoteStopF_0,
	kcInsNoteMapNoteStopFS0,
	kcInsNoteMapNoteStopG_0,
	kcInsNoteMapNoteStopGS0,
	kcInsNoteMapNoteStopA_1,
	kcInsNoteMapNoteStopAS1,
	kcInsNoteMapNoteStopB_1,
	kcInsNoteMapNoteStopC_1,
	kcInsNoteMapNoteStopCS1,
	kcInsNoteMapNoteStopD_1,
	kcInsNoteMapNoteStopDS1,
	kcInsNoteMapNoteStopE_1,
	kcInsNoteMapNoteStopF_1,
	kcInsNoteMapNoteStopFS1,
	kcInsNoteMapNoteStopG_1,
	kcInsNoteMapNoteStopGS1,
	kcInsNoteMapNoteStopA_2,
	kcInsNoteMapNoteStopAS2,
	kcInsNoteMapNoteStopB_2,
	kcInsNoteMapNoteStopC_2,
	kcInsNoteMapNoteStopCS2,
	kcInsNoteMapNoteStopD_2,
	kcInsNoteMapNoteStopDS2,
	kcInsNoteMapNoteStopE_2,
	kcInsNoteMapNoteStopF_2,
	kcInsNoteMapNoteStopFS2,
	kcInsNoteMapNoteStopG_2,
	kcInsNoteMapNoteStopGS2,
	kcInsNoteMapNoteStopA_3,
	kcInsNoteMapEndNoteStops=kcInsNoteMapNoteStopA_3,

	kcVSTGUIStartNotes,
	kcVSTGUINoteC_0=kcVSTGUIStartNotes,
	kcVSTGUINoteCS0,
	kcVSTGUINoteD_0,
	kcVSTGUINoteDS0,
	kcVSTGUINoteE_0,
	kcVSTGUINoteF_0,
	kcVSTGUINoteFS0,
	kcVSTGUINoteG_0,
	kcVSTGUINoteGS0,
	kcVSTGUINoteA_1,
	kcVSTGUINoteAS1,
	kcVSTGUINoteB_1,
	kcVSTGUINoteC_1,
	kcVSTGUINoteCS1,
	kcVSTGUINoteD_1,
	kcVSTGUINoteDS1,
	kcVSTGUINoteE_1,
	kcVSTGUINoteF_1,
	kcVSTGUINoteFS1,
	kcVSTGUINoteG_1,
	kcVSTGUINoteGS1,
	kcVSTGUINoteA_2,
	kcVSTGUINoteAS2,
	kcVSTGUINoteB_2,
	kcVSTGUINoteC_2,
	kcVSTGUINoteCS2,
	kcVSTGUINoteD_2,
	kcVSTGUINoteDS2,
	kcVSTGUINoteE_2,
	kcVSTGUINoteF_2,
	kcVSTGUINoteFS2,
	kcVSTGUINoteG_2,
	kcVSTGUINoteGS2,
	kcVSTGUINoteA_3,
	kcVSTGUIEndNotes=kcVSTGUINoteA_3,

	//Note stops
	kcVSTGUIStartNoteStops,
	kcVSTGUINoteStopC_0=kcVSTGUIStartNoteStops,
	kcVSTGUINoteStopCS0,
	kcVSTGUINoteStopD_0,
	kcVSTGUINoteStopDS0,
	kcVSTGUINoteStopE_0,
	kcVSTGUINoteStopF_0,
	kcVSTGUINoteStopFS0,
	kcVSTGUINoteStopG_0,
	kcVSTGUINoteStopGS0,
	kcVSTGUINoteStopA_1,
	kcVSTGUINoteStopAS1,
	kcVSTGUINoteStopB_1,
	kcVSTGUINoteStopC_1,
	kcVSTGUINoteStopCS1,
	kcVSTGUINoteStopD_1,
	kcVSTGUINoteStopDS1,
	kcVSTGUINoteStopE_1,
	kcVSTGUINoteStopF_1,
	kcVSTGUINoteStopFS1,
	kcVSTGUINoteStopG_1,
	kcVSTGUINoteStopGS1,
	kcVSTGUINoteStopA_2,
	kcVSTGUINoteStopAS2,
	kcVSTGUINoteStopB_2,
	kcVSTGUINoteStopC_2,
	kcVSTGUINoteStopCS2,
	kcVSTGUINoteStopD_2,
	kcVSTGUINoteStopDS2,
	kcVSTGUINoteStopE_2,
	kcVSTGUINoteStopF_2,
	kcVSTGUINoteStopFS2,
	kcVSTGUINoteStopG_2,
	kcVSTGUINoteStopGS2,
	kcVSTGUINoteStopA_3,
	kcVSTGUIEndNoteStops=kcVSTGUINoteStopA_3,

	kcStartVSTGUICommands,
	kcVSTGUIPrevPreset=kcStartVSTGUICommands,
	kcVSTGUINextPreset,
	kcVSTGUIPrevPresetJump,
	kcVSTGUINextPresetJump,
	kcVSTGUIRandParams,
	kcEndVSTGUICommands=kcVSTGUIRandParams,

	kcNumCommands,
};


enum Modifiers
{
/*	NoModifier = 0,
	LShift   = 1<<0,
	RShift   = 1<<1,
	LControl = 1<<2,
	RControl = 1<<3,
	LAlt     = 1<<4,
	RAlt     = 1<<5,
	MaxMod   = 1<<6,
*/
	MaxMod = 15 // HOTKEYF_ALT | HOTKEYF_CONTROL | HOTKEYF_EXT | HOTKEYF_SHIFT
};

#define MAINKEYS 256
#define KeyMapSize kCtxMaxInputContexts*MaxMod*MAINKEYS*kNumKeyEvents
typedef CommandID KeyMap[kCtxMaxInputContexts][MaxMod][MAINKEYS][kNumKeyEvents];
//typedef CMap<long, long, CommandID, CommandID> KeyMap;

//KeyMap

struct KeyCombination
{
	UINT mod;
	UINT code;
	InputTargetContext ctx;
	KeyEventType event;
	bool operator==(const KeyCombination &other)
		{return (mod==other.mod && code==other.code && ctx==other.ctx && event==other.event);}
};

struct CommandStruct
{
//public:
	UINT UID;
	bool isDummy;
	bool isHidden;
	CString Message;
	CArray <KeyCombination,KeyCombination> kcList;
	//KeyCombination kcList[10];
	
	bool operator=(const CommandStruct &other) {
		UID=other.UID;
		Message=other.Message;
		isDummy=other.isDummy;
		isHidden=other.isHidden;
		kcList.Copy(other.kcList);
		return true;
	}

};

struct Rule
{
	UINT ID;
	CString desc;
    bool enforce;

};

enum RuleID
{
	krPreventDuplicate,
	krDeleteOldOnConflict,

	krAllowNavigationWithSelection,
	krAllowSelectionWithNavigation,
	krAutoSelectOff,
	krAllowSelectCopySelectCombos,
	krLockNotesToChords,
	krNoteOffOnKeyRelease,
	krPropagateNotes,
	krReassignDigitsToOctaves,
	krAutoSpacing,
	krCheckModifiers,
	krPropagateSampleManipulation,
	kNumRules
};

class CCommandSet
{
protected:
	//util
	void SetupCommands();
	void SetupContextHierarchy();
	bool IsDummyCommand(CommandID cmd);
	UINT CodeToModifier(UINT code);
	CString EnforceAll(KeyCombination kc, CommandID cmd, bool adding);
	void GetParentContexts(InputTargetContext child, CArray<InputTargetContext, InputTargetContext> ctxList);
	void GetChildContexts(InputTargetContext child, CArray<InputTargetContext, InputTargetContext> ctxList);

	bool IsExtended(UINT code);
	int FindCmd(int uid);
	bool KeyCombinationConflict(KeyCombination kc1, KeyCombination kc2, bool &crossCxtConflict);

	//members
	CArray <CommandStruct,CommandStruct> commands;
	//CArray<CArray<bool,bool>, CArray<bool,bool> > m_isParentContext;
	bool m_isParentContext[kCtxMaxInputContexts][kCtxMaxInputContexts];
	bool enforceRule[kNumRules];
	
public:
	static bool s_bShowErrorOnUnknownKeybinding;

	CCommandSet(void);
	~CCommandSet(void);
	
	//Population
	CString Add(KeyCombination kc, CommandID cmd, bool overwrite);
	CString Add(KeyCombination kc, CommandID cmd, bool overwrite, int pos);
	CString Remove(KeyCombination kc, CommandID cmd);
	CString Remove(int pos, CommandID cmd);

	//Tranformation
	bool QuickChange_SetEffectsXM();
	bool QuickChange_SetEffectsIT();
	bool QuickChange_SetEffects(char comList[kcSetFXEnd-kcSetFXStart]);
	bool QuickChange_NotesRepeat();
	bool QuickChange_NoNotesRepeat();

	//Communication
	KeyCombination GetKey(CommandID cmd, UINT key);
	bool isHidden(UINT c);
	int GetKeyListSize(CommandID cmd);
	CString GetCommandText(CommandID cmd);
	CString GetKeyText(UINT mod, UINT code);
	CString GetKeyTextFromCommand(CommandID c, UINT key);
	CString GetContextText(InputTargetContext ctx);
	CString GetKeyEventText(KeyEventType ke);
	CString GetModifierText(UINT mod);

	//Pululation ;)
	void Copy(CCommandSet *source);	// copy the contents of a commandset into this command set
	void GenKeyMap(KeyMap &km);		// Generate a keymap from this command set
	bool SaveFile(CString FileName, bool debug);
	bool LoadFile(CString FileName);

	static DWORD GetKeymapLabel(InputTargetContext ctx, UINT mod, UINT code, KeyEventType ke);
	
};
//end rewbs.customKeys
