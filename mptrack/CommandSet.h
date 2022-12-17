/*
 * CommandSet.h
 * ------------
 * Purpose: Header file for custom key handling: List of supported keyboard shortcuts and class for them.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include <string>
#include "openmpt/base/FlagSet.hpp"
#include <map>
#include <bitset>

OPENMPT_NAMESPACE_BEGIN

#define HOTKEYF_MIDI 0x10      // modifier mask for MIDI CCs
#define HOTKEYF_RSHIFT 0x20    // modifier mask for right Shift key
#define HOTKEYF_RCONTROL 0x40  // modifier mask for right Ctrl key
#define HOTKEYF_RALT 0x80      // modifier mask for right Alt key

enum InputTargetContext : int8
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

enum KeyEventType : int8
{
	kKeyEventNone   = 0,
	kKeyEventDown   = 1 << 0,
	kKeyEventUp     = 1 << 1,
	kKeyEventRepeat = 1 << 2,
	kNumKeyEvents   = 1 << 3
};
DECLARE_FLAGSET(KeyEventType)


enum CommandID
{
	kcCommandSetNumNotes = 33,  // kcVPEndNotes - kcVPStartNotes

	kcNull = -1,
	kcFirst,

	//Global
	kcGlobalStart = kcFirst,
	kcStartFile = kcGlobalStart,
	kcFileNew = kcStartFile,
	kcFileOpen,
	kcFileAppend,
	kcFileClose,
	kcFileCloseAll,
	kcFileSave,
	kcFileSaveAs,
	kcFileSaveCopy,
	kcFileSaveTemplate,
	kcFileSaveAsWave,
	kcFileSaveAsMP3,
	kcFileSaveMidi,
	kcFileSaveOPL,
	kcFileExportCompat,
	kcPrevDocument,
	kcNextDocument,
	kcFileImportMidiLib,
	kcFileAddSoundBank,
	kcEndFile = kcFileAddSoundBank,

	kcStartPlayCommands,
	kcPlayPauseSong = kcStartPlayCommands,
	kcPlayStopSong,
	kcPauseSong,
	kcStopSong,
	kcPlaySongFromStart,
	kcPlaySongFromCursor,
	kcPlaySongFromPattern,
	kcPlayPatternFromStart,
	kcPlayPatternFromCursor,
	kcToggleLoopSong,
	kcPanic,
	kcEstimateSongLength,
	kcApproxRealBPM,
	kcMidiRecord,
	kcTempoIncrease,
	kcTempoDecrease,
	kcTempoIncreaseFine,
	kcTempoDecreaseFine,
	kcSpeedIncrease,
	kcSpeedDecrease,
	kcEndPlayCommands = kcSpeedDecrease,

	kcStartEditCommands,
	kcEditUndo = kcStartEditCommands,
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
	kcEndEditCommands = kcEditFindNext,
/*
	kcWindowNew,
	kcWindowCascade,
	kcWindowTileHorz,
	kcWindowTileVert,
*/
	kcStartView,
	kcViewGeneral = kcStartView,
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
	kcViewTempoSwing,
	kcShowMacroConfig,
	kcViewMIDImapping,
	kcViewEditHistory,
	kcViewToggle,
	kcSwitchToInstrLibrary,
	kcHelp,
	kcEndView = kcHelp,

	kcStartMisc,
	kcPrevInstrument = kcStartMisc,
	kcNextInstrument,
	kcPrevOctave,
	kcNextOctave,
	kcPrevOrder,
	kcNextOrder,
	kcEndMisc = kcNextOrder,

	kcDummyShortcut,
	kcGlobalEnd = kcDummyShortcut,

	//Pattern Navigation
	kcStartPatNavigation,
	kcStartJumpSnap = kcStartPatNavigation,
	kcPatternJumpDownh1 = kcStartJumpSnap,
	kcPatternJumpUph1,
	kcPatternJumpDownh2,
	kcPatternJumpUph2,
	kcPatternSnapDownh1,
	kcPatternSnapUph1,
	kcPatternSnapDownh2,
	kcPatternSnapUph2,
	kcPrevEntryInColumn,
	kcNextEntryInColumn,
	kcEndJumpSnap = kcNextEntryInColumn,
	kcStartPlainNavigate,
	kcNavigateDown = kcStartPlainNavigate,
	kcNavigateUp,
	kcNavigateDownBySpacing,
	kcNavigateUpBySpacing,
	kcNavigateLeft,
	kcNavigateRight,
	kcNavigateNextChan,
	kcNavigatePrevChan,
	kcEndPlainNavigate = kcNavigatePrevChan,
	kcStartHomeEnd,
	kcHomeHorizontal = kcStartHomeEnd,
	kcHomeVertical,
	kcHomeAbsolute,
	kcEndHorizontal,
	kcEndVertical,
	kcEndAbsolute,
	kcEndHomeEnd = kcEndAbsolute,
	kcEndPatNavigation = kcEndHomeEnd,

	// with select. Order must match above.
	kcStartPatNavigationSelect,
	kcPatternJumpDownh1Select = kcStartPatNavigationSelect,
	kcPatternJumpUph1Select,
	kcPatternJumpDownh2Select,
	kcPatternJumpUph2Select,
	kcPatternSnapDownh1Select,
	kcPatternSnapUph1Select,
	kcPatternSnapDownh2Select,
	kcPatternSnapUph2Select,
	kcPrevEntryInColumnSelect,
	kcNextEntryInColumnSelect,
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
	kcEndPatNavigationSelect = kcEndAbsoluteSelect,

	//Pattern Editing
	kcStartPatternEditing,
	kcStartSelect = kcStartPatternEditing,
	kcSelect = kcStartSelect,
	kcSelectWithNav,
	kcSelectOff,
	kcSelectOffWithNav,
	kcSelectWithCopySelect,
	kcSelectOffWithCopySelect,
	kcCopySelect,
	kcCopySelectOff,
	kcCopySelectWithNav,
	kcCopySelectOffWithNav,
	kcCopySelectWithSelect,
	kcCopySelectOffWithSelect,
	kcSelectChannel,
	kcSelectColumn,
	kcSelectRow,
	kcSelectEvent,
	kcSelectBeat,
	kcSelectMeasure,
	kcLoseSelection,
	kcCopyAndLoseSelection,
	kcEndSelect = kcCopyAndLoseSelection,

	kcStartPatternClipboard,
	kcCutPatternChannel = kcStartPatternClipboard,
	kcCutPattern,
	kcCopyPatternChannel,
	kcCopyPattern,
	kcPastePatternChannel,
	kcPastePattern,
	kcToggleClipboardManager,
	kcClipboardPrev,
	kcClipboardNext,
	kcEndPatternClipboard = kcClipboardNext,

	kcStartPatternEditMisc,
	kcToggleFollowSong = kcStartPatternEditMisc,
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
	kcSplitPattern,
	kcPatternEditPCNotePlugin,
	kcTogglePluginEditor,
	kcShowNoteProperties,
	kcShowPatternProperties,
	kcShowSplitKeyboardSettings,
	kcChordEditor,
	kcChangeLoopStatus,
	kcShowEditMenu,
	kcShowChannelCtxMenu,
	kcShowChannelPluginCtxMenu,
	kcTimeAtRow,
	kcLockPlaybackToRows,
	kcQuantizeSettings,
	kcTogglePatternPlayRow,
	kcToggleOverflowPaste,
	kcToggleNoteOffRecordPC,
	kcToggleNoteOffRecordMIDI,
	kcEndPatternEditMisc = kcToggleNoteOffRecordMIDI,

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
	kcChannelAddBefore,
	kcChannelAddAfter,
	kcChannelRemove,
	kcChannelMoveLeft,
	kcChannelMoveRight,
	kcChannelSettings,
	kcEndChannelKeys = kcChannelSettings,
	kcBeginTranspose,
	kcTransposeUp = kcBeginTranspose,
	kcTransposeDown,
	kcTransposeOctUp,
	kcTransposeOctDown,
	kcTransposeCustom,
	kcTransposeCustomQuick,
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
	kcPatternSetInstrumentNotEmpty,
	kcPatternGrowSelection,
	kcPatternShrinkSelection,
	//	kcClearSelection,
	kcClearRow,
	kcClearField,
	kcClearFieldITStyle,
	kcClearRowStep,
	kcClearFieldStep,
	kcClearFieldStepITStyle,
	kcDeleteRow,
	kcDeleteWholeRow,
	kcDeleteRowGlobal,
	kcDeleteWholeRowGlobal,
	kcInsertRow,
	kcInsertWholeRow,
	kcInsertRowGlobal,
	kcInsertWholeRowGlobal,
	kcPrevPattern,
	kcNextPattern,
	kcPrevSequence,
	kcNextSequence,
	kcEndPatternEditing = kcNextSequence,

	//Notes
	kcVPStartNotes,
	kcVPNoteC_0 = kcVPStartNotes,
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
	kcVPEndNotes = kcVPNoteA_3,

	//Note stops
	kcVPStartNoteStops,
	kcVPNoteStopC_0 = kcVPStartNoteStops,
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
	kcVPEndNoteStops = kcVPNoteStopA_3,

	//Chords
	kcVPStartChords,
	kcVPChordC_0 = kcVPStartChords,
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
	kcVPEndChords = kcVPChordA_3,

	//Chord Stops
	kcVPStartChordStops,
	kcVPChordStopC_0 = kcVPStartChordStops,
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
	kcVPEndChordStops = kcVPChordStopA_3,

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
	kcChordModifier = kcStartNoteMisc,
	kcNoteCutOld,
	kcNoteOffOld,
	kcNoteFadeOld,
	kcNoteCut,
	kcNoteOff,
	kcNoteFade,
	kcNotePC,
	kcNotePCS,
	kcEndNoteMisc = kcNotePCS,


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
	kcSetVolume0 = kcSetVolumeStart,
	kcSetVolume1,
	kcSetVolume2,
	kcSetVolume3,
	kcSetVolume4,
	kcSetVolume5,
	kcSetVolume6,
	kcSetVolume7,
	kcSetVolume8,
	kcSetVolume9,
	kcSetVolumeVol,           //v
	kcSetVolumePan,           //p
	kcSetVolumeVolSlideUp,    //c
	kcSetVolumeVolSlideDown,  //d
	kcSetVolumeFineVolUp,     //a
	kcSetVolumeFineVolDown,   //b
	kcSetVolumeVibratoSpd,    //u
	kcSetVolumeVibrato,       //h
	kcSetVolumeXMPanLeft,     //l
	kcSetVolumeXMPanRight,    //r
	kcSetVolumePortamento,    //g
	kcSetVolumeITPortaUp,     //f
	kcSetVolumeITPortaDown,   //e
	kcSetVolumeITUnused,      //:
	kcSetVolumeITOffset,      //o
	kcSetVolumeEnd = kcSetVolumeITOffset,

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

	//Effect commands. ORDER IS CRUCIAL and must match EffectCommand enum!
	kcSetFXStart,
	kcFixedFXStart = kcSetFXStart,
	kcSetFXarp = kcSetFXStart,  //0,j
	kcSetFXportUp,              //1,f
	kcSetFXportDown,            //2,e
	kcSetFXport,                //3,g
	kcSetFXvibrato,             //4,h
	kcSetFXportSlide,           //5,l
	kcSetFXvibSlide,            //6,k
	kcSetFXtremolo,             //7,r
	kcSetFXpan,                 //8,x
	kcSetFXoffset,              //9,o
	kcSetFXvolSlide,            //a,d
	kcSetFXgotoOrd,             //b,b
	kcSetFXsetVol,              //c,?
	kcSetFXgotoRow,             //d,c
	kcSetFXretrig,              //r,q
	kcSetFXspeed,               //?,a
	kcSetFXtempo,               //f,t
	kcSetFXtremor,              //t,i
	kcSetFXextendedMOD,         //e,?
	kcSetFXextendedS3M,         //?,s
	kcSetFXchannelVol,          //?,m
	kcSetFXchannelVols,         //?,n
	kcSetFXglobalVol,           //g,v
	kcSetFXglobalVols,          //h,w
	kcSetFXkeyoff,              //k,?
	kcSetFXfineVib,             //?,u
	kcSetFXpanbrello,           //y,y
	kcSetFXextendedXM,          //x,?
	kcSetFXpanSlide,            //p,p
	kcSetFXsetEnvPos,           //l,?
	kcSetFXmacro,               //z,z
	kcFixedFXend = kcSetFXmacro,
	kcSetFXmacroSlide,      //?,\ ,
	kcSetFXdelaycut,        //?,:
	kcSetFXextension,       //?,#
	kcSetFXFinetune,        //?,+
	kcSetFXFinetuneSmooth,  //?,*
	kcSetFXEnd = kcSetFXFinetuneSmooth,

	kcStartInstrumentMisc,
	kcInstrumentLoad = kcStartInstrumentMisc,
	kcInstrumentSave,
	kcInstrumentNew,
	kcInstrumentEnvelopeLoad,
	kcInstrumentEnvelopeSave,
	kcInstrumentEnvelopeZoomIn,
	kcInstrumentEnvelopeZoomOut,
	kcInstrumentEnvelopeScale,
	kcInstrumentEnvelopeSwitchToVolume,
	kcInstrumentEnvelopeSwitchToPanning,
	kcInstrumentEnvelopeSwitchToPitch,
	kcInstrumentEnvelopeToggleVolume,
	kcInstrumentEnvelopeTogglePanning,
	kcInstrumentEnvelopeTogglePitch,
	kcInstrumentEnvelopeToggleFilter,
	kcInstrumentEnvelopeToggleLoop,
	kcInstrumentEnvelopeSelectLoopStart,
	kcInstrumentEnvelopeSelectLoopEnd,
	kcInstrumentEnvelopeToggleSustain,
	kcInstrumentEnvelopeSelectSustainStart,
	kcInstrumentEnvelopeSelectSustainEnd,
	kcInstrumentEnvelopeToggleCarry,
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
	kcEndInstrumentMisc = kcInstrumentEnvelopeToggleReleaseNode,

	kcStartInstrumentCtrlMisc,
	kcInstrumentCtrlLoad = kcStartInstrumentCtrlMisc,
	kcInstrumentCtrlSave,
	kcInstrumentCtrlNew,
	kcInstrumentCtrlDuplicate,
	kcInsNoteMapEditSampleMap,
	kcInsNoteMapEditSample,
	kcInsNoteMapCopyCurrentNote,
	kcInsNoteMapCopyCurrentSample,
	kcInsNoteMapReset,
	kcInsNoteMapTransposeSamples,
	kcInsNoteMapRemove,
	kcInsNoteMapTransposeUp,
	kcInsNoteMapTransposeDown,
	kcInsNoteMapTransposeOctUp,
	kcInsNoteMapTransposeOctDown,
	kcEndInstrumentCtrlMisc = kcInsNoteMapTransposeOctDown,

	kcStartSampleMisc,
	kcSampleLoad = kcStartSampleMisc,
	kcSampleLoadRaw,
	kcSampleSave,
	kcSampleNew,
	kcSampleDuplicate,
	kcSampleInitializeOPL,
	kcSampleTransposeUp,
	kcSampleTransposeDown,
	kcSampleTransposeOctUp,
	kcSampleTransposeOctDown,
	kcSampleToggleFollowPlayCursor,
	kcEndSampleMisc = kcSampleToggleFollowPlayCursor,

	kcStartSampleEditing,
	kcSampleTrim = kcStartSampleEditing,
	kcSampleTrimToLoopEnd,
	kcSampleSilence,
	kcSampleNormalize,
	kcSampleAmplify,
	kcSampleReverse,
	kcSampleDelete,
	kcSampleToggleDrawing,
	kcSampleResize,
	kcSampleGrid,
	kcSampleZoomUp,
	kcSampleZoomDown,
	kcSampleZoomSelection,
	kcSampleCenterSampleStart,
	kcSampleCenterSampleEnd,
	kcSampleCenterLoopStart,
	kcSampleCenterLoopEnd,
	kcSampleCenterSustainStart,
	kcSampleCenterSustainEnd,
	kcSampleConvertPingPongLoop,
	kcSampleConvertPingPongSustain,
	kcSample8Bit,
	kcSampleMonoMix,
	kcSampleMonoLeft,
	kcSampleMonoRight,
	kcSampleMonoSplit,
	kcSampleStereoSep,
	kcSampleUpsample,
	kcSampleDownsample,
	kcSampleResample,
	kcSampleInvert,
	kcSampleSignUnsign,
	kcSampleRemoveDCOffset,
	kcSampleQuickFade,
	kcSampleXFade,
	kcSampleAutotune,
	kcEndSampleEditing = kcSampleAutotune,

	kcStartSampleCues,
	kcEndSampleCues = kcStartSampleCues + 8,
	kcSampleSlice,
	kcEndSampleCueGroup = kcSampleSlice,

	kcSampStartNotes,
	kcSampEndNotes = kcSampStartNotes + kcCommandSetNumNotes,
	kcSampStartNoteStops,
	kcSampEndNoteStops = kcSampStartNoteStops + kcCommandSetNumNotes,

	kcInstrumentStartNotes,
	kcInstrumentEndNotes = kcInstrumentStartNotes + kcCommandSetNumNotes,
	kcInstrumentStartNoteStops,
	kcInstrumentEndNoteStops = kcInstrumentStartNoteStops + kcCommandSetNumNotes,

	kcTreeViewStartNotes,
	kcTreeViewEndNotes = kcTreeViewStartNotes + kcCommandSetNumNotes,
	kcTreeViewStartNoteStops,
	kcTreeViewEndNoteStops = kcTreeViewStartNoteStops + kcCommandSetNumNotes,

	kcInsNoteMapStartNotes,
	kcInsNoteMapEndNotes = kcInsNoteMapStartNotes + kcCommandSetNumNotes,
	kcInsNoteMapStartNoteStops,
	kcInsNoteMapEndNoteStops = kcInsNoteMapStartNoteStops + kcCommandSetNumNotes,

	kcVSTGUIStartNotes,
	kcVSTGUIEndNotes = kcVSTGUIStartNotes + kcCommandSetNumNotes,
	kcVSTGUIStartNoteStops,
	kcVSTGUIEndNoteStops = kcVSTGUIStartNoteStops + kcCommandSetNumNotes,

	kcCommentsStartNotes,
	kcCommentsEndNotes = kcCommentsStartNotes + kcCommandSetNumNotes,
	kcCommentsStartNoteStops,
	kcCommentsEndNoteStops = kcCommentsStartNoteStops + kcCommandSetNumNotes,

	kcStartTreeViewCommands,
	kcTreeViewStopPreview = kcStartTreeViewCommands,
	kcTreeViewOpen,
	kcTreeViewPlay,
	kcTreeViewInsert,
	kcTreeViewDuplicate,
	kcTreeViewDelete,
	kcTreeViewDeletePermanently,
	kcTreeViewRename,
	kcTreeViewSendToEditorInsertNew,
	kcTreeViewFind,
	kcTreeViewSortByName,
	kcTreeViewSortByDate,
	kcTreeViewSortBySize,
	kcEndTreeViewCommands = kcTreeViewSortBySize,

	kcStartVSTGUICommands,
	kcVSTGUIPrevPreset = kcStartVSTGUICommands,
	kcVSTGUINextPreset,
	kcVSTGUIPrevPresetJump,
	kcVSTGUINextPresetJump,
	kcVSTGUIRandParams,
	kcVSTGUIToggleRecordParams,
	kcVSTGUIToggleRecordMIDIOut,
	kcVSTGUIToggleSendKeysToPlug,
	kcVSTGUIBypassPlug,
	kcEndVSTGUICommands = kcVSTGUIBypassPlug,

	kcStartOrderlistCommands,
	// Orderlist edit
	kcStartOrderlistEdit = kcStartOrderlistCommands,
	kcOrderlistEditDelete = kcStartOrderlistEdit,
	kcOrderlistEditInsert,
	kcOrderlistEditInsertSeparator,
	kcOrderlistEditCopyOrders,
	kcMergePatterns,
	kcOrderlistEditPattern,
	kcOrderlistSwitchToPatternView,
	kcEndOrderlistEdit = kcOrderlistSwitchToPatternView,
	// Orderlist navigation
	kcStartOrderlistNavigation,
	kcOrderlistNavigateLeft = kcStartOrderlistNavigation,
	kcOrderlistNavigateRight,
	kcOrderlistNavigateFirst,
	kcOrderlistNavigateLast,
	kcEndOrderlistNavigation = kcOrderlistNavigateLast,
	// with selection key(must match order above)
	kcStartOrderlistNavigationSelect,
	kcOrderlistNavigateLeftSelect = kcStartOrderlistNavigationSelect,
	kcOrderlistNavigateRightSelect,
	kcOrderlistNavigateFirstSelect,
	kcOrderlistNavigateLastSelect,
	kcEndOrderlistNavigationSelect = kcOrderlistNavigateLastSelect,
	// Orderlist pattern list edit
	kcStartOrderlistNum,
	kcOrderlistPat0 = kcStartOrderlistNum,
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
	kcEndOrderlistNum = kcOrderlistPatInvalid,
	// Playback lock
	kcStartOrderlistLock,
	kcOrderlistLockPlayback = kcStartOrderlistLock,
	kcOrderlistUnlockPlayback,
	kcEndOrderlistLock = kcOrderlistUnlockPlayback,
	kcEndOrderlistCommands = kcEndOrderlistLock,

	kcStartChnSettingsCommands,
	kcChnSettingsPrev = kcStartChnSettingsCommands,
	kcChnSettingsNext,
	kcChnColorFromPrev,
	kcChnColorFromNext,
	kcChnSettingsClose,
	kcEndChnSettingsCommands = kcChnSettingsClose,

	kcStartCommentsCommands,
	kcToggleSmpInsList = kcStartCommentsCommands,
	kcExecuteSmpInsListItem,
	kcRenameSmpInsListItem,
	kcEndCommentsCommands = kcRenameSmpInsListItem,

	kcNumCommands,
};


enum Modifiers : uint8
{
	ModNone = 0,
	ModShift = HOTKEYF_SHIFT,
	ModCtrl = HOTKEYF_CONTROL,
	ModAlt = HOTKEYF_ALT,
	ModWin = HOTKEYF_EXT,
	ModMidi = HOTKEYF_MIDI,
	ModRShift = HOTKEYF_RSHIFT,
	ModRCtrl = HOTKEYF_RCONTROL,
	ModRAlt = HOTKEYF_RALT,
	MaxMod = ModShift | ModCtrl | ModAlt | ModWin | ModMidi | ModRShift | ModRCtrl | ModRAlt,
};
DECLARE_FLAGSET(Modifiers)


struct KeyCombination
{
protected:
	InputTargetContext ctx;  // TODO: This should probably rather be a member of CommandStruct and not of the individual key combinations for consistency's sake.
	FlagSet<Modifiers> mod;
	uint8 code;
	FlagSet<KeyEventType> event;

	constexpr uint32 AsUint32() const
	{
		static_assert(sizeof(KeyCombination) == sizeof(uint32));
		return (static_cast<uint32>(ctx) << 0) |
		       (static_cast<uint32>(mod.GetRaw()) << 8) |
		       (static_cast<uint32>(code) << 16) |
		       (static_cast<uint32>(event.GetRaw()) << 24);
	}

public:
	KeyCombination(InputTargetContext context = kCtxAllContexts, FlagSet<Modifiers> modifier = ModNone, UINT key = 0, FlagSet<KeyEventType> type = kKeyEventNone)
	    : ctx(context)
	    , mod(modifier)
	    , code(static_cast<uint8>(key))
	    , event(type)
	{
	}

	constexpr bool operator==(const KeyCombination &other) const
	{
		return ctx == other.ctx && mod == other.mod && code == other.code && event == other.event;
	}

	constexpr bool operator<(const KeyCombination &kc) const
	{
		return AsUint32() < kc.AsUint32();
	}

	// Getters / Setters
	void Context(InputTargetContext context) { ctx = context; }
	constexpr InputTargetContext Context() const { return ctx; }

	void Modifier(FlagSet<Modifiers> modifier) { mod = modifier; }
	constexpr FlagSet<Modifiers> Modifier() const { return mod; }
	void Modifier(const KeyCombination &other) { mod = other.mod; }
	void AddModifier(FlagSet<Modifiers> modifier) { mod |= modifier; }
	void AddModifier(const KeyCombination &other) { mod |= other.mod; }

	void KeyCode(UINT key) { code = static_cast<uint8>(key); }
	constexpr UINT KeyCode() const { return code; }

	void EventType(FlagSet<KeyEventType> type) { event = type; }
	constexpr FlagSet<KeyEventType> EventType() const { return event; }

	static KeyCombination FromLPARAM(LPARAM lParam)
	{
		return KeyCombination(
		    static_cast<InputTargetContext>((lParam >> 0) & 0xFF),
		    static_cast<Modifiers>((lParam >> 8) & 0xFF),
		    static_cast<UINT>((lParam >> 16) & 0xFF),
		    static_cast<KeyEventType>((lParam >> 24) & 0xFF));
	}
	LPARAM AsLPARAM() const { return AsUint32(); }

	// Key combination to string
	static CString GetContextText(InputTargetContext ctx);
	CString GetContextText() const { return GetContextText(Context()); }

	static CString GetModifierText(FlagSet<Modifiers> mod);
	CString GetModifierText() const { return GetModifierText(Modifier()); }

	static CString GetKeyText(FlagSet<Modifiers> mod, UINT code);
	CString GetKeyText() const { return GetKeyText(Modifier(), KeyCode()); }

	static CString GetKeyEventText(FlagSet<KeyEventType> event);
	CString GetKeyEventText() const { return GetKeyEventText(EventType()); }

	static bool IsExtended(UINT code);
};

using KeyMap = std::multimap<KeyCombination, CommandID>;
using KeyMapRange = std::pair<KeyMap::const_iterator, KeyMap::const_iterator>;

//KeyMap

struct KeyCommand
{
	static constexpr uint32 Dummy = 1u << 31;
	static constexpr uint32 Hidden = 1u << 30;
	static constexpr uint32 UIDMask = Hidden - 1u;

	std::vector<KeyCombination> kcList;
	CString Message;

protected:
	uint32 UID = 0;

public:
	KeyCommand() = default;
	KeyCommand(uint32 uid, const TCHAR *message = _T(""), std::vector<KeyCombination> keys = {});

	// Unique ID for on-disk keymap format.
	// Note that hidden commands do not have a unique ID, because they are never written to keymap files.
	uint32 ID() const noexcept { return UID & UIDMask; }
	// e.g. Chord modifier is a dummy command, which serves only to automatically
	// generate a set of key combinations for chords
	bool IsDummy() const noexcept { return (UID & Dummy) != 0; }
	// Hidden commands are not configurable by the user (e.g. derived from dummy commands or note entry keys duplicated into other contexts)
	bool IsHidden() const noexcept { return (UID & Hidden) != 0; }
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

struct CModSpecifications;

class CCommandSet
{
protected:
	//util
	void SetupCommands();
	void SetupContextHierarchy();
	CString EnforceAll(KeyCombination kc, CommandID cmd, bool adding);

	CommandID FindCmd(uint32 uid) const;
	bool KeyCombinationConflict(KeyCombination kc1, KeyCombination kc2, bool checkEventConflict = true) const;

	void ApplyDefaultKeybindings(const Version onlyCommandsAfterVersion = {});

public:
	CCommandSet();

	// Population
	CString Add(KeyCombination kc, CommandID cmd, bool overwrite, int pos = -1, bool checkEventConflict = true);
	CString Remove(KeyCombination kc, CommandID cmd);
	CString Remove(int pos, CommandID cmd);

	std::pair<CommandID, KeyCombination> IsConflicting(KeyCombination kc, CommandID cmd, bool checkEventConflict = true) const;
	bool IsCrossContextConflict(KeyCombination kc1, KeyCombination kc2) const;

	// Tranformation
	bool QuickChange_SetEffects(const CModSpecifications &modSpecs);
	bool QuickChange_NotesRepeat(bool repeat);

	// Communication
	KeyCombination GetKey(CommandID cmd, UINT key) const { return m_commands[cmd].kcList[key]; }
	bool isHidden(UINT c) const { return m_commands[c].IsHidden(); }
	int GetKeyListSize(CommandID cmd) const { return (cmd != kcNull) ? static_cast<int>(m_commands[cmd].kcList.size()) : 0; }
	CString GetCommandText(CommandID cmd) const { return m_commands[cmd].Message; }
	CString GetKeyTextFromCommand(CommandID c, UINT key) const;

	// Pululation ;)
	void Copy(const CCommandSet *source);  // Copy the contents of a commandset into this command set
	void GenKeyMap(KeyMap &km);            // Generate a keymap from this command set
	bool SaveFile(const mpt::PathString &filename);
	bool LoadFile(const mpt::PathString &filename);
	bool LoadFile(std::istream &iStrm, const mpt::ustring &filenameDescription);
	void LoadDefaultKeymap();

protected:
	const CModSpecifications *m_oldSpecs = nullptr;
	KeyCommand m_commands[kcNumCommands];
	std::bitset<kCtxMaxInputContexts> m_isParentContext[kCtxMaxInputContexts];
	std::bitset<kNumRules> m_enforceRule;
};

OPENMPT_NAMESPACE_END
