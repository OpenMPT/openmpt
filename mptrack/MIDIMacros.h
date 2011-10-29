/*
 * MIDIMacros.h
 * ------------
 * Purpose: Header file for MIDI macro helper functions / classes
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */

#pragma once
#ifndef MIDIMACROS_H
#define MIDIMACROS_H

// parametered macro presets:
enum enmParameteredMacroType
{
	sfx_unused = 0,
	sfx_cutoff,			// Type 1 - Z00 - Z7F controls resonant filter cutoff
	sfx_reso,			// Type 2 - Z00 - Z7F controls resonant filter resonance
	sfx_mode,			// Type 3 - Z00 - Z7F controls resonant filter mode (lowpass / highpass)
	sfx_drywet,			// Type 4 - Z00 - Z7F controls plugin Dry / Wet ratio
	sfx_plug,			// Type 5 - Z00 - Z7F controls a plugin parameter
	sfx_cc,				// Type 6 - Z00 - Z7F controls MIDI CC
	sfx_custom,

	sfx_max
};


// fixed macro presets:
enum enmFixedMacroType
{
	zxx_custom = 0,
	zxx_reso4Bit,		// Type 1 - Z80 - Z8F controls resonant filter resonance
	zxx_reso7Bit,		// Type 2 - Z80 - ZFF controls resonant filter resonance
	zxx_cutoff,			// Type 3 - Z80 - ZFF controls resonant filter cutoff
	zxx_mode,			// Type 4 - Z80 - ZFF controls resonant filter mode (lowpass / highpass)
	zxx_resomode,		// Type 5 - Z80 - Z9F controls resonance + filter mode

	zxx_max
};


class MIDIMacroTools
{
protected:
	CSoundFile &m_SndFile;

public:
	// Get macro type from a macro string
	static enmParameteredMacroType GetMacroType(CString value);
	static enmFixedMacroType GetZxxType(const char (&szMidiZXXExt)[128][MACRO_LENGTH]);

	// Translate macro type or macro string to macro name
	static CString GetMacroName(enmParameteredMacroType macro);
	CString GetMacroName(CString value, PLUGINDEX plugin) const;

	// Extract information from a macro string.
	static int MacroToPlugParam(CString value);
	static int MacroToMidiCC(CString value);

	// Check if any macro can automate a given plugin parameter
	int FindMacroForParam(long param) const;

	// Create a new Zxx macro
	static void CreateZxxFromType(char (&szMidiZXXExt)[128][MACRO_LENGTH], enmFixedMacroType iZxxType);
	
	// Check if a given set of macros is the default IT macro set.
	bool IsMacroDefaultSetupUsed() const;

	MIDIMacroTools(CSoundFile &sndFile) : m_SndFile(sndFile) { };
};


#ifdef MODPLUG_TRACKER

////////////////////////////////////////////////////////////////////////
// MIDI Macro Configuration Dialog

//class CColourEdit;
#include "ColourEdit.h"


//===================================
class CMidiMacroSetup: public CDialog
//===================================
{
public:
	CMidiMacroSetup(CSoundFile &sndFile, CWnd *parent = NULL) : CDialog(IDD_MIDIMACRO, parent), m_SndFile(sndFile), macroTools(sndFile), m_MidiCfg(sndFile.m_MidiCfg)
	{
		m_bEmbed = (m_SndFile.m_dwSongFlags & SONG_EMBEDMIDICFG) != 0;
	}

	bool m_bEmbed;
	MODMIDICFG m_MidiCfg;


protected:
	CComboBox m_CbnSFx, m_CbnSFxPreset, m_CbnZxx, m_CbnZxxPreset, m_CbnMacroPlug, m_CbnMacroParam, m_CbnMacroCC;
	CEdit m_EditSFx, m_EditZxx;
	CColourEdit m_EditMacroValue[NUM_MACROS], m_EditMacroType[NUM_MACROS]; //rewbs.macroGUI
	CButton m_EditMacro[NUM_MACROS], m_BtnMacroShowAll[NUM_MACROS];

	CSoundFile &m_SndFile;
	MIDIMacroTools macroTools;

	bool ValidateMacroString(CEdit &wnd, char *lastMacro, bool isParametric);

	void UpdateMacroList(int macro=-1);
	void ToggleBoxes(UINT preset, UINT sfx);
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void UpdateDialog();
	afx_msg void OnSetAsDefault();
	afx_msg void OnResetCfg();
	afx_msg void OnMacroHelp();
	afx_msg void OnEmbedMidiCfg();
	afx_msg void OnSFxChanged();
	afx_msg void OnSFxPresetChanged();
	afx_msg void OnZxxPresetChanged();
	afx_msg void OnSFxEditChanged();
	afx_msg void OnZxxEditChanged();
	afx_msg void OnPlugChanged();
	afx_msg void OnPlugParamChanged();
	afx_msg void OnCCChanged();

	afx_msg void OnViewAllParams(UINT id);
	afx_msg void OnSetSFx(UINT id);
	DECLARE_MESSAGE_MAP()
};

#endif // MODPLUG_TRACKER

#endif // MIDIMACROS_H
