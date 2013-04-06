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

#include "../soundlib/MixerSettings.h"
#include "../soundlib/Resampler.h"
#include "../sounddsp/EQ.h"

/////////////////////////////////////////////////////////////////////////
// Default directories

enum Directory
{
	DIR_MODS = 0,
	DIR_SAMPLES,
	DIR_INSTRUMENTS,
	DIR_PLUGINS,
	DIR_PLUGINPRESETS,
	DIR_EXPORT,
	DIR_TUNING,
	DIR_TEMPLATE_FILES_USER,
	NUM_DIRS
};


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
	MAX_MODCOLORS,
	// Internal color codes (not saved to color preset files)
	MODCOLOR_2NDHIGHLIGHT,
	MODCOLOR_DEFAULTVOLUME,
	MAX_MODPALETTECOLORS
};


// Pattern Setup (contains also non-pattern related settings)
// Feel free to replace the deprecated flags by new flags, but be sure to
// update TrackerSettings::LoadINISettings() / TrackerSettings::LoadRegistrySettings() as well.
#define PATTERN_PLAYNEWNOTE			0x01		// play new notes while recording
#define PATTERN_LARGECOMMENTS		0x02		// use large font in comments
#define PATTERN_STDHIGHLIGHT		0x04		// enable primary highlight (measures)
#define PATTERN_SMALLFONT			0x08		// use small font in pattern editor
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
#define PATTERN_MIDIRECORD			0x100000	// MIDI Record on by default
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


// Midi Setup
#define MIDISETUP_RECORDVELOCITY			0x01	// Record MIDI velocity
#define MIDISETUP_TRANSPOSEKEYBOARD			0x02	// Apply transpose value to MIDI Notes
#define MIDISETUP_MIDITOPLUG				0x04	// Pass MIDI messages to plugins
#define MIDISETUP_MIDIVOL_TO_NOTEVOL		0x08	// Combine MIDI volume to note velocity
#define MIDISETUP_RECORDNOTEOFF				0x10	// Record MIDI Note Off to pattern
#define MIDISETUP_RESPONDTOPLAYCONTROLMSGS	0x20	// Respond to Restart/Continue/Stop MIDI commands
#define MIDISETUP_MIDIMACROCONTROL			0x80	// Record MIDI controller changes a MIDI macro changes in pattern
#define MIDISETUP_PLAYPATTERNONMIDIIN		0x100	// Play pattern if MIDI Note is received and playback is paused


#define SOUNDSETUP_SECONDARY   SNDMIX_SECONDARY
#define SOUNDSETUP_RESTARTMASK SOUNDSETUP_SECONDARY


// EQ
struct EQPreset
{
	char szName[12];
	UINT Gains[MAX_EQ_BANDS];
	UINT Freqs[MAX_EQ_BANDS];
};


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


//===================
class TrackerSettings
//===================
{

public:

	// MIDI recording
	enum RecordAftertouchOptions
	{
		atDoNotRecord = 0,
		atRecordAsVolume,
		atRecordAsMacro,
	};

	bool m_ShowSplashScreen;
	BOOL gbMdiMaximize;
	bool gbShowHackControls;
	LONG glTreeWindowWidth, glTreeSplitRatio;
	LONG glGeneralWindowHeight, glPatternWindowHeight, glSampleWindowHeight, 
		glInstrumentWindowHeight, glCommentsWindowHeight, glGraphWindowHeight; //rewbs.varWindowSize
	/*MptVersion::VersionNum*/ uint32 gcsPreviousVersion;
	CString gcsInstallGUID;
	MODTYPE defaultModType;

	// Audio Setup
	DWORD m_nPreAmp, gbLoopSong;
	LONG m_nWaveDevice; // use the SNDDEV_GET_NUMBER and SNDDEV_GET_TYPE macros to decode
	DWORD m_LatencyMS;
	DWORD m_UpdateIntervalMS;
#ifndef NO_EQ
	EQPreset m_EqSettings;
#endif

	// MIDI Setup
	LONG m_nMidiDevice;
	DWORD m_dwMidiSetup;
	RecordAftertouchOptions aftertouchBehaviour;
	uint16 midiVelocityAmp;

	// Pattern Setup
	UINT gnPatternSpacing;
	BOOL gbPatternVUMeters, gbPatternPluginNames, gbPatternRecord;
	DWORD m_dwPatternSetup, m_nKeyboardCfg;
	DWORD m_nRowHighlightMeasures, m_nRowHighlightBeats;	// primary (measures) and secondary (beats) highlight
	ROWINDEX recordQuantizeRows;
	bool m_bHideUnavailableCtxMenuItems;
	int orderlistMargins;
	int rowDisplayOffset;

	// Sample Editor Setup
	UINT m_nSampleUndoMaxBuffer;

	// key config
	TCHAR m_szKbdFile[_MAX_PATH];
	COLORREF rgbCustomColors[MAX_MODCOLORS];

	MixerSettings m_MixerSettings;
	CResamplerSettings m_ResamplerSettings;

	UINT gnAutoChordWaitTime;

	uint32 gnMsgBoxVisiblityFlags;

	// MIDI Import
	int midiImportSpeed, midiImportPatternLen;

	// Chords
	MPTChords Chords;

	// Directory Arrays (default dir + last dir)
	TCHAR m_szDefaultDirectory[NUM_DIRS][_MAX_PATH];
	TCHAR m_szWorkingDirectory[NUM_DIRS][_MAX_PATH];
	// Directory to INI setting translation
	static const TCHAR *m_szDirectoryToSettingsName[NUM_DIRS];

	int gnPlugWindowX;
	int gnPlugWindowY;
	int gnPlugWindowWidth;
	int gnPlugWindowHeight;
	DWORD gnPlugWindowLast;	// Last selected plugin ID

public:

	TrackerSettings();
	void LoadSettings();
	void SaveSettings();
	static void GetDefaultColourScheme(COLORREF (&colours)[MAX_MODCOLORS]);

	// access to default + working directories
	void SetWorkingDirectory(const LPCTSTR szFilenameFrom, Directory dir, bool bStripFilename = false);
	LPCTSTR GetWorkingDirectory(Directory dir) const;
	void SetDefaultDirectory(const LPCTSTR szFilenameFrom, Directory dir, bool bStripFilename = false);
	LPCTSTR GetDefaultDirectory(Directory dir) const;

	static MPTChords &GetChords() { return Instance().Chords; }

	// Get settings object singleton
	static TrackerSettings &Instance() { return settings; }

protected:

	void LoadINISettings(const CString &iniFile);

	void LoadRegistryEQ(HKEY key, LPCSTR pszName, EQPreset *pEqSettings);
	bool LoadRegistrySettings();

	void LoadChords(MPTChords &chords);
	void SaveChords(MPTChords &chords);

	void SetDirectory(const LPCTSTR szFilenameFrom, Directory dir, TCHAR (&pDirs)[NUM_DIRS][_MAX_PATH], bool bStripFilename);

	// The one and only settings object
	static TrackerSettings settings;

};
