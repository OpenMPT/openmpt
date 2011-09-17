/*
 * TrackerSettings.h
 * -----------------
 * Purpose: Header file for application setting handling.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 */


#ifndef TRACKERSETTINGS_H
#define TRACKERSETTINGS_H
#pragma once

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


//===================
class TrackerSettings
//===================
{

public:

	BOOL gbMdiMaximize;
	bool gbShowHackControls;
	LONG glTreeWindowWidth, glTreeSplitRatio;
	LONG glGeneralWindowHeight, glPatternWindowHeight, glSampleWindowHeight, 
		glInstrumentWindowHeight, glCommentsWindowHeight, glGraphWindowHeight; //rewbs.varWindowSize
	CString gcsPreviousVersion;
	CString gcsInstallGUID;
	MODTYPE defaultModType;

	// Audio Setup
	DWORD m_dwSoundSetup, m_dwRate, m_dwQuality, m_nSrcMode, m_nBitsPerSample, m_nPreAmp, gbLoopSong, m_nChannels;
	LONG m_nWaveDevice; // use the SNDDEV_GET_NUMBER and SNDDEV_GET_TYPE macros to decode
	LONG m_nMidiDevice;
	DWORD m_nBufferLength;
	EQPRESET m_EqSettings;
	// Pattern Setup
	UINT gnPatternSpacing;
	BOOL gbPatternVUMeters, gbPatternPluginNames, gbPatternRecord;
	DWORD m_dwPatternSetup, m_dwMidiSetup, m_nKeyboardCfg;
	DWORD m_nRowHighlightMeasures, m_nRowHighlightBeats;	// primary (measures) and secondary (beats) highlight
	bool m_bHideUnavailableCtxMenuItems;
	int orderlistMargins;

	// Sample Editor Setup
	UINT m_nSampleUndoMaxBuffer;

	// key config
	TCHAR m_szKbdFile[_MAX_PATH];
	COLORREF rgbCustomColors[MAX_MODCOLORS];

	//rewbs.resamplerConf
	double gdWFIRCutoff;
	BYTE gbWFIRType;
	long glVolumeRampUpSamples, glVolumeRampDownSamples;
	//end rewbs.resamplerConf
	UINT gnAutoChordWaitTime;

	uint32 gnMsgBoxVisiblityFlags;

	// MIDI Import
	int midiImportSpeed, midiImportPatternLen;

	// Chords
	MPTCHORD Chords[3 * 12]; // 3 octaves

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

protected:

	void LoadINISettings(const CString &iniFile);

	void LoadRegistryEQ(HKEY key, LPCSTR pszName, EQPRESET *pEqSettings);
	bool LoadRegistrySettings();

	void SetDirectory(const LPCTSTR szFilenameFrom, Directory dir, TCHAR (&pDirs)[NUM_DIRS][_MAX_PATH], bool bStripFilename);

};

#endif // TRACKERSETTINGS_H