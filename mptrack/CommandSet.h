/*
 * CommandSet.h
 * ------------
 * Purpose: Header file for custom key handling: List of supported keyboard shortcuts and class for them.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once
#include <string>
#include "../common/FlagSet.h"
#include <map>

OPENMPT_NAMESPACE_BEGIN

struct CModSpecifications;

#define HOTKEYF_MIDI 0x10		// modifier mask for MIDI CCs
#define HOTKEYF_RSHIFT 0x20		// modifier mask for right Shift key
#define HOTKEYF_RCONTROL 0x40	// modifier mask for right Ctrl key
#define HOTKEYF_RALT 0x80		// modifier mask for right Alt key

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
	kCtxChannelSettings,
	kCtxMaxInputContexts
};

enum KeyEventType
{
	kKeyEventNone	= 0,
	kKeyEventDown	= 1 << 0,
	kKeyEventUp		= 1 << 1,
	kKeyEventRepeat	= 1 << 2,
	kNumKeyEvents	= 1 << 3
};
template <> struct enum_traits<KeyEventType> { typedef uint8 store_type; };
DECLARE_FLAGSET(KeyEventType)


enum CommandID
{
	kcNull = -1,

	//Global
	kcGlobalStart,
	kcStartFile=kcGlobalStart,
	kcFileNew=kcGlobalStart,
	kcFileOpen,
	kcFileAppend,
	kcFileClose,
	kcFileCloseAll,
	kcFileSave,
	kcFileSaveAs,
	kcFileSaveTemplate,
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
	kcPlaySongFromPattern,
	kcPlayPatternFromStart,
	kcPlayPatternFromCursor,
	kcPanic,
	kcEstimateSongLength,
	kcApproxRealBPM,
	kcMidiRecord,
	kcEndPlayCommands=kcMidiRecord,

	kcStartEditCommands,
	kcEditUndo=kcStartEditCommands,
	kcEditRedo,
	kcEditCut,
	kcEditCopy,
	kcEditPaste,
	kcEditMixPaste,
	kcEditMixPasteITStyle,
	kcEditPasteFlood,
	kcEditPushForwardPaste,
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
	kcShowMacroConfig,
	kcViewMIDImapping,
	kcViewEditHistory,
	kcSwitchToInstrLibrary,
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

	kcDummyShortcut,
	kcGlobalEnd=kcDummyShortcut,

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
	kcSelectRow,
	kcSelectEvent,
	kcSelectBeat,
	kcSelectMeasure,
	kcEndSelect=kcSelectMeasure,

	kcStartPatternEditMisc,
	kcToggleFollowSong=kcStartPatternEditMisc,
	kcCursorCopy,
	kcCursorPaste,
	kcFindInstrument,
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
	kcIncreaseSpacing,
	kcDecreaseSpacing,
	kcSwitchToOrderList,
	kcNewPattern,
	kcDuplicatePattern,
	kcPatternEditPCNotePlugin,
	kcTogglePluginEditor,
	kcShowNoteProperties,
	kcShowPatternProperties,
	kcShowSplitKeyboardSettings,
	kcChangeLoopStatus,
	kcShowEditMenu,
	kcTimeAtRow,
	kcLockPlaybackToRows,
	kcQuantizeSettings,
	kcToggleOverflowPaste,
	kcToggleNoteOffRecordPC,
	kcToggleNoteOffRecordMIDI,
	kcEndPatternEditMisc=kcToggleNoteOffRecordMIDI,

	kcStartPatternClipboard,
	kcToggleClipboardManager = kcStartPatternClipboard,
	kcClipboardPrev,
	kcClipboardNext,
	kcEndPatternClipboard = kcClipboardNext,

	kcStartChannelKeys,
	kcChannelMute = kcStartChannelKeys,
	kcChannelSolo,
	kcChannelUnmuteAll,
	kcToggleChanMuteOnPatTransition,
	kcUnmuteAllChnOnPatTransition,
	kcSoloChnOnPatTransition,
	kcChannelRecordSelect,
	kcChannelSplitRecordSelect,
	kcChannelReset,
	kcChannelTranspose,
	kcChannelDuplicate,
	kcChannelSettings,
	kcEndChannelKeys = kcChannelSettings,
	kcCopyAndLoseSelection,
	kcBeginTranspose,
	kcTransposeUp = kcBeginTranspose,
	kcTransposeDown,
	kcTransposeOctUp,
	kcTransposeOctDown,
	kcTransposeCustom,
	kcDataEntryUp,
	kcDataEntryDown,
	kcDataEntryUpCoarse,
	kcDataEntryDownCoarse,
	kcEndTranspose = kcDataEntryDownCoarse,
	kcPatternAmplify,
	kcPatternInterpolateNote,
	kcPatternInterpolateInstr,
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

	// Release set octave key
	kcSetOctaveStop0,
	kcSetOctaveStop1,
	kcSetOctaveStop2,
	kcSetOctaveStop3,
	kcSetOctaveStop4,
	kcSetOctaveStop5,
	kcSetOctaveStop6,
	kcSetOctaveStop7,
	kcSetOctaveStop8,
	kcSetOctaveStop9,

	//Note Misc
	kcStartNoteMisc,
	kcChordModifier=kcStartNoteMisc,
	kcNoteCutOld,
	kcNoteOffOld,
	kcNoteFadeOld,
	kcNoteCut,
	kcNoteOff,
	kcNoteFade,
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
	kcSetVolumeITUnused,		//:
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
	kcSetFXdelaycut,	//?,:
	kcSetFXextension,	//?,#
	kcSetFXEnd=kcSetFXextension,

	kcStartInstrumentMisc,
	// Note: Order must be the same as kcStartSampleMisc because commands are propagated!
	kcInstrumentLoad=kcStartInstrumentMisc,
	kcInstrumentSave,
	kcInstrumentNew,
	kcInstrumentEnvelopeLoad,
	kcInstrumentEnvelopeSave,
	kcInstrumentEnvelopeZoomIn,
	kcInstrumentEnvelopeZoomOut,
	kcInstrumentEnvelopeScale,
	kcInstrumentEnvelopeSelectLoopStart,
	kcInstrumentEnvelopeSelectLoopEnd,
	kcInstrumentEnvelopeSelectSustainStart,
	kcInstrumentEnvelopeSelectSustainEnd,
	kcInstrumentEnvelopePointPrev,
	kcInstrumentEnvelopePointNext,
	kcInstrumentEnvelopePointMoveLeft,
	kcInstrumentEnvelopePointMoveRight,
	kcInstrumentEnvelopePointMoveLeftCoarse,
	kcInstrumentEnvelopePointMoveRightCoarse,
	kcInstrumentEnvelopePointMoveUp,
	kcInstrumentEnvelopePointMoveUp8,
	kcInstrumentEnvelopePointMoveDown,
	kcInstrumentEnvelopePointMoveDown8,
	kcInstrumentEnvelopePointInsert,
	kcInstrumentEnvelopePointRemove,
	kcInstrumentEnvelopeSetLoopStart,
	kcInstrumentEnvelopeSetLoopEnd,
	kcInstrumentEnvelopeSetSustainLoopStart,
	kcInstrumentEnvelopeSetSustainLoopEnd,
	kcInstrumentEnvelopeToggleReleaseNode,
	kcEndInstrumentMisc=kcInstrumentEnvelopeToggleReleaseNode,

	kcStartInstrumentCtrlMisc,
	kcInstrumentCtrlLoad=kcStartInstrumentCtrlMisc,
	kcInstrumentCtrlSave,
	kcInstrumentCtrlNew,
	kcInstrumentCtrlDuplicate,
	kcInsNoteMapEditSampleMap,
	kcInsNoteMapEditSample,
	kcInsNoteMapCopyCurrentNote,
	kcInsNoteMapCopyCurrentSample,
	kcInsNoteMapReset,
	kcInsNoteMapRemove,
	kcInsNoteMapTransposeUp,
	kcInsNoteMapTransposeDown,
	kcInsNoteMapTransposeOctUp,
	kcInsNoteMapTransposeOctDown,
	kcEndInstrumentCtrlMisc=kcInsNoteMapTransposeOctDown,

	kcStartSampleMisc,
	kcSampleLoad=kcStartSampleMisc,
	kcSampleSave,
	kcSampleNew,
	kcSampleDuplicate,
	kcSampleTransposeUp,
	kcSampleTransposeDown,
	kcSampleTransposeOctUp,
	kcSampleTransposeOctDown,
	kcEndSampleMisc=kcSampleTransposeOctDown,

	kcStartSampleEditing,
	kcSampleTrim=kcStartSampleEditing,
	kcSampleTrimToLoopEnd,
	kcSampleSilence,
	kcSampleNormalize,
	kcSampleAmplify,
	kcSampleReverse,
	kcSampleDelete,
	kcSampleZoomUp,
	kcSampleZoomDown,
	kcSampleZoomSelection,
	kcSampleCenterSampleStart,
	kcSampleCenterSampleEnd,
	kcSampleCenterLoopStart,
	kcSampleCenterLoopEnd,
	kcSampleCenterSustainStart,
	kcSampleCenterSustainEnd,
	kcSample8Bit,
	kcSampleMonoMix,
	kcSampleMonoLeft,
	kcSampleMonoRight,
	kcSampleMonoSplit,
	kcSampleUpsample,
	kcSampleDownsample,
	kcSampleResample,
	kcSampleInvert,
	kcSampleSignUnsign,
	kcSampleRemoveDCOffset,
	kcSampleQuickFade,
	kcSampleXFade,
	kcSampleAutotune,
	kcEndSampleEditing=kcSampleAutotune,

	kcStartSampleCues,
	kcEndSampleCues = kcStartSampleCues + 8,
	kcSampleSlice,
	kcEndSampleCueGroup = kcSampleSlice,

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

	kcTreeViewStopPreview,

	kcStartVSTGUICommands,
	kcVSTGUIPrevPreset=kcStartVSTGUICommands,
	kcVSTGUINextPreset,
	kcVSTGUIPrevPresetJump,
	kcVSTGUINextPresetJump,
	kcVSTGUIRandParams,
	kcVSTGUIToggleRecordParams,
	kcVSTGUIToggleRecordMIDIOut,
	kcVSTGUIToggleSendKeysToPlug,
	kcVSTGUIBypassPlug,
	kcEndVSTGUICommands=kcVSTGUIBypassPlug,

	kcStartOrderlistCommands,
		// Orderlist edit
	kcStartOrderlistEdit=kcStartOrderlistCommands,
	kcOrderlistEditDelete=kcStartOrderlistEdit,
	kcOrderlistEditInsert,
	kcOrderlistEditCopyOrders,
	kcOrderlistEditPattern,
	kcOrderlistSwitchToPatternView,
	kcEndOrderlistEdit=kcOrderlistSwitchToPatternView,
		// Orderlist navigation
	kcStartOrderlistNavigation,
	kcOrderlistNavigateLeft=kcStartOrderlistNavigation,
	kcOrderlistNavigateRight,
	kcOrderlistNavigateFirst,
	kcOrderlistNavigateLast,
	kcEndOrderlistNavigation= kcOrderlistNavigateLast,
		// with selection key(must match order above)
	kcStartOrderlistNavigationSelect,
	kcOrderlistNavigateLeftSelect=kcStartOrderlistNavigationSelect,
	kcOrderlistNavigateRightSelect,
	kcOrderlistNavigateFirstSelect,
	kcOrderlistNavigateLastSelect,
	kcEndOrderlistNavigationSelect = kcOrderlistNavigateLastSelect,
		// Orderlist pattern list edit
	kcStartOrderlistNum,
	kcOrderlistPat0=kcStartOrderlistNum,
	kcOrderlistPat1,
	kcOrderlistPat2,
	kcOrderlistPat3,
	kcOrderlistPat4,
	kcOrderlistPat5,
	kcOrderlistPat6,
	kcOrderlistPat7,
	kcOrderlistPat8,
	kcOrderlistPat9,
	kcOrderlistPatPlus,
	kcOrderlistPatMinus,
	kcOrderlistPatIgnore,
	kcOrderlistPatInvalid,
	kcEndOrderlistNum=kcOrderlistPatInvalid,
		// Playback lock
	kcStartOrderlistLock,
	kcOrderlistLockPlayback = kcStartOrderlistLock,
	kcOrderlistUnlockPlayback,
	kcEndOrderlistLock = kcOrderlistUnlockPlayback,
	kcEndOrderlistCommands=kcEndOrderlistLock,

	kcStartChnSettingsCommands,
	kcChnSettingsPrev = kcStartChnSettingsCommands,
	kcChnSettingsNext,
	kcChnSettingsClose,
	kcEndChnSettingsCommands = kcChnSettingsClose,

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
	MaxMod = HOTKEYF_ALT | HOTKEYF_CONTROL | HOTKEYF_EXT | HOTKEYF_SHIFT | HOTKEYF_MIDI | HOTKEYF_RSHIFT | HOTKEYF_RCONTROL | HOTKEYF_RALT,
};


#define MAINKEYS 256

struct KeyCombination
{
protected:
	uint8 ctx;	// TODO: This should probably rather be a member of CommandStruct and not of the individual key combinations for consistency's sake.
	uint8 mod;
	uint8 code;
	uint8 event;

	STATIC_ASSERT(static_cast<uint8>(kCtxMaxInputContexts - 1) == kCtxMaxInputContexts - 1);
	STATIC_ASSERT(static_cast<uint8>(MaxMod) == MaxMod);
	STATIC_ASSERT(static_cast<uint8>(MAINKEYS - 1) == MAINKEYS - 1);
	STATIC_ASSERT(static_cast<uint8>(kNumKeyEvents - 1) == kNumKeyEvents - 1);

	uint32 AsUint32() const
	{
		STATIC_ASSERT(sizeof(KeyCombination) == sizeof(uint32));
		return static_cast<uint32>(0)
			| (static_cast<uint32>(ctx  ) << 24)
			| (static_cast<uint32>(mod  ) << 16)
			| (static_cast<uint32>(code ) <<  8)
			| (static_cast<uint32>(event) <<  0)
			;
	}

public:
	KeyCombination(InputTargetContext context = kCtxAllContexts, UINT modifier = 0, UINT key = 0, FlagSet<KeyEventType> type = kKeyEventNone)
		: ctx(static_cast<uint8>(context))
		, mod(static_cast<uint8>(modifier))
		, code(static_cast<uint8>(key))
		, event(type.GetRaw())
	{ }

	bool operator== (const KeyCombination &other) const
	{
		return ctx == other.ctx && mod == other.mod && code == other.code && event == other.event;
	}

	bool operator < (const KeyCombination &kc) const
	{
		return AsUint32() < kc.AsUint32();
	}

	// Getters / Setters
	void Context(InputTargetContext context) { ctx = static_cast<uint8>(context); }
	InputTargetContext Context() const { return static_cast<InputTargetContext>(ctx); }

	void Modifier(UINT modifier) { mod = static_cast<uint8>(modifier); }
	UINT Modifier() const { return static_cast<UINT>(mod); }
	void Modifier(const KeyCombination &other) { mod = other.mod; }
	void AddModifier(UINT modifier) { mod |= static_cast<uint8>(modifier); }
	void AddModifier(const KeyCombination &other) { mod |= other.mod; }

	void KeyCode(UINT key) { code = static_cast<uint8>(key); }
	UINT KeyCode() const { return static_cast<UINT>(code); }

	void EventType(FlagSet<KeyEventType> type) { event = type.GetRaw(); }
	FlagSet<KeyEventType> EventType() const { FlagSet<KeyEventType> result; result.SetRaw(event); return result; }

	// Key combination to string
	static const TCHAR *GetContextText(InputTargetContext ctx);
	const TCHAR *GetContextText() const { return GetContextText(Context()); }

	static CString GetModifierText(UINT mod);
	CString GetModifierText() const { return GetModifierText(Modifier()); }

	static CString GetKeyText(UINT mod, UINT code);
	CString GetKeyText() const { return GetKeyText(Modifier(), KeyCode()); }

	static CString GetKeyEventText(FlagSet<KeyEventType> event);
	CString GetKeyEventText() const { return GetKeyEventText(EventType()); }

	static bool IsExtended(UINT code);
};

typedef std::multimap<KeyCombination, CommandID> KeyMap;
typedef std::pair<KeyMap::const_iterator, KeyMap::const_iterator> KeyMapRange;

//KeyMap

struct CommandStruct
{
	std::vector<KeyCombination> kcList;
	CString Message;
	UINT UID;
	bool isDummy;
	bool isHidden;
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

// For defining key commands
enum enmKcVisibility
{
	kcVisible,
	kcHidden
};

enum enmKcDummy
{
	kcNoDummy,
	kcDummy
};

struct CModSpecifications;

class CCommandSet
{
protected:
	//util
	inline void DefineKeyCommand(CommandID kc, UINT uid, const TCHAR *message, enmKcVisibility visible = kcVisible, enmKcDummy dummy = kcNoDummy);
	void SetupCommands();
	void SetupContextHierarchy();
	bool IsDummyCommand(CommandID cmd) const;
	CString EnforceAll(KeyCombination kc, CommandID cmd, bool adding);

	int FindCmd(int uid) const;
	bool KeyCombinationConflict(KeyCombination kc1, KeyCombination kc2, bool checkEventConflict = true) const;

	const CModSpecifications *oldSpecs;
	CommandStruct commands[kcNumCommands];
	bool m_isParentContext[kCtxMaxInputContexts][kCtxMaxInputContexts];
	bool enforceRule[kNumRules];

public:

	CCommandSet();

	//Population
	CString Add(KeyCombination kc, CommandID cmd, bool overwrite, int pos = -1, bool checkEventConflict = true);
	CString Remove(KeyCombination kc, CommandID cmd);
	CString Remove(int pos, CommandID cmd);

	std::pair<CommandID, KeyCombination> IsConflicting(KeyCombination kc, CommandID cmd, bool checkEventConflict = true) const;
	bool IsCrossContextConflict(KeyCombination kc1, KeyCombination kc2) const;

	//Tranformation
	bool QuickChange_SetEffects(const CModSpecifications &modSpecs);
	bool QuickChange_NotesRepeat(bool repeat);

	//Communication
	KeyCombination GetKey(CommandID cmd, UINT key) const { return commands[cmd].kcList[key]; }
	bool isHidden(UINT c) const { return commands[c].isHidden; }
	int GetKeyListSize(CommandID cmd) const { return (int)commands[cmd].kcList.size(); }
	CString GetCommandText(CommandID cmd) const { return commands[cmd].Message; }
	CString GetKeyTextFromCommand(CommandID c, UINT key);

	//Pululation ;)
	void Copy(const CCommandSet *source);	// copy the contents of a commandset into this command set
	void GenKeyMap(KeyMap &km);		// Generate a keymap from this command set
	bool SaveFile(const mpt::PathString &filename);
	bool LoadFile(const mpt::PathString &filename);
	bool LoadFile(std::istream& iStrm, const std::wstring &filenameDescription, CCommandSet *commandSet = nullptr);
	bool LoadDefaultKeymap();
};

OPENMPT_NAMESPACE_END
