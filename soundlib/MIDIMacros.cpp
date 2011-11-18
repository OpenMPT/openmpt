/*
 * MIDIMacros.cpp
 * --------------
 * Purpose: Helper functions / classes for MIDI Macro functionality.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */

#include "stdafx.h"
#include "midi.h"
#include "Sndfile.h"
#include "MIDIMacros.h"
#ifndef NO_VST
#include "../mptrack/Vstplug.h"
#endif // NO_VST


enmParameteredMacroType MIDIMacroTools::GetMacroType(CString value)
//-----------------------------------------------------------------
{
	value.Remove(' ');
	for(size_t i = 0; i < sfx_max; i++)
	{
		enmParameteredMacroType sfx = static_cast<enmParameteredMacroType>(i);
		if(sfx != sfx_custom)
		{
			if(value.Compare(CreateParameteredMacroFromType(sfx)) == 0) return sfx;
		}
	}
	// Special macros with additional "parameter":
	if (value.Compare(CreateParameteredMacroFromType(sfx_cc, MIDICC_start)) >= 0 && value.Compare(CreateParameteredMacroFromType(sfx_cc, MIDICC_end)) <= 0 && value.GetLength() == 5)
		return sfx_cc;
	if (value.Compare(CreateParameteredMacroFromType(sfx_plug, 0)) >= 0 && value.Compare(CreateParameteredMacroFromType(sfx_plug, 0x17F)) <= 0 && value.GetLength() == 7)
		return sfx_plug; 
	return sfx_custom;	// custom / unknown
}


// Returns macro description including plugin parameter / MIDI CC information
CString MIDIMacroTools::GetMacroName(CString value, PLUGINDEX plugin) const
//-------------------------------------------------------------------------
{
	const enmParameteredMacroType macroType = GetMacroType(value);

	switch(macroType)
	{
	case sfx_plug:
		{
			const int param = MacroToPlugParam(value);
			CString paramName;

#ifndef NO_VST
			if(plugin < MAX_MIXPLUGINS)
			{
				CVstPlugin *pPlug = reinterpret_cast<CVstPlugin *>(m_SndFile.m_MixPlugins[plugin].pMixPlugin);
				if(pPlug)
				{
					paramName = pPlug->GetParamName(param);
				}
				if (paramName.IsEmpty())
				{
					return _T("N/A");
				}

				CString formattedName;
				formattedName.Format(_T("Param %d (%s)"), param, paramName);
				return CString(formattedName);
			} else
#endif // NO_VST
			{
				return _T("N/A - No Plugin");
			}
		}

	case sfx_cc:
		{
			CString formattedCC;
			formattedCC.Format(_T("MIDI CC %d"), MacroToMidiCC(value));
			return formattedCC;
		}

	default:
		return GetMacroName(macroType);
	}
}


// Returns generic macro description.
CString MIDIMacroTools::GetMacroName(enmParameteredMacroType macro)
//-----------------------------------------------------------------
{
	switch(macro)
	{
	case sfx_unused:
		return _T("Unused");
	case sfx_cutoff:
		return _T("Set Filter Cutoff");
	case sfx_reso:
		return _T("Set Filter Resonance");
	case sfx_mode:
		return _T("Set Filter Mode");
	case sfx_drywet:
		return _T("Set Plugin Dry/Wet Ratio");
	case sfx_plug:
		return _T("Control Plugin Parameter...");
	case sfx_cc:
		return _T("MIDI CC...");
	case sfx_custom:
	default:
		return _T("Custom");
	}
}


int MIDIMacroTools::MacroToPlugParam(CString macro)
//-------------------------------------------------
{
	macro.Remove(' ');
	int code=0;
	char* param = (char *) (LPCTSTR) macro;
	param += 4;
	if ((param[0] >= '0') && (param[0] <= '9')) code = (param[0] - '0') << 4; else
		if ((param[0] >= 'A') && (param[0] <= 'F')) code = (param[0] - 'A' + 0x0A) << 4;
	if ((param[1] >= '0') && (param[1] <= '9')) code += (param[1] - '0'); else
		if ((param[1] >= 'A') && (param[1] <= 'F')) code += (param[1] - 'A' + 0x0A);

	if (macro.GetLength() >= 4 && macro.GetAt(3) == '0')
		return (code - 128);
	else
		return (code + 128);
}


int MIDIMacroTools::MacroToMidiCC(CString macro)
//----------------------------------------------
{
	macro.Remove(' ');
	int code=0;
	char* param = (char *) (LPCTSTR) macro;
	param += 2;
	if ((param[0] >= '0') && (param[0] <= '9')) code = (param[0] - '0') << 4; else
		if ((param[0] >= 'A') && (param[0] <= 'F')) code = (param[0] - 'A' + 0x0A) << 4;
	if ((param[1] >= '0') && (param[1] <= '9')) code += (param[1] - '0'); else
		if ((param[1] >= 'A') && (param[1] <= 'F')) code += (param[1] - 'A' + 0x0A);

	return code;
}


int MIDIMacroTools::FindMacroForParam(long param) const
//-----------------------------------------------------
{
	for (size_t macro = 0; macro < NUM_MACROS; macro++)
	{
		CString macroString = m_SndFile.m_MidiCfg.szMidiSFXExt[macro];
		if (GetMacroType(macroString) == sfx_plug && MacroToPlugParam(macroString) == param)
		{
			return macro;
		}
	}

	return -1;
}


// Retrieve Zxx (Z80-ZFF) type from current macro configuration
enmFixedMacroType MIDIMacroTools::GetZxxType(const char (&szMidiZXXExt)[128][MACRO_LENGTH])
//-----------------------------------------------------------------------------------------
{
	// Compare with all possible preset patterns
	for(size_t i = 1; i < zxx_max; i++)
	{
		// Prepare pattern to compare
		char szPatterns[128][MACRO_LENGTH];
		CreateZxxFromType(szPatterns, static_cast<enmFixedMacroType>(i));

		bool bFound = true;
		for(size_t j = 0; j < 128; j++)
		{
			if(strncmp(szPatterns[j], szMidiZXXExt[j], MACRO_LENGTH))
			{
				bFound = false;
				break;
			}
		}
		if(bFound) return static_cast<enmFixedMacroType>(i);
	}
	return zxx_custom; // Custom setup
}


// Create Zxx (Z80 - ZFF) from one out of five presets
void MIDIMacroTools::CreateZxxFromType(char (&szMidiZXXExt)[128][MACRO_LENGTH], enmFixedMacroType iZxxType)
//---------------------------------------------------------------------------------------------------------
{
	for(size_t i = 0; i < 128; i++)
	{
		switch(iZxxType)
		{
		case zxx_reso4Bit:
			// Type 1 - Z80 - Z8F controls resonance
			if (i < 16) wsprintf(szMidiZXXExt[i], "F0F001%02X", i * 8);
			else strcpy(szMidiZXXExt[i], "");
			break;

		case zxx_reso7Bit:
			// Type 2 - Z80 - ZFF controls resonance
			wsprintf(szMidiZXXExt[i], "F0F001%02X", i);
			break;

		case zxx_cutoff:
			// Type 3 - Z80 - ZFF controls cutoff
			wsprintf(szMidiZXXExt[i], "F0F000%02X", i);
			break;

		case zxx_mode:
			// Type 4 - Z80 - ZFF controls filter mode
			wsprintf(szMidiZXXExt[i], "F0F002%02X", i);
			break;

		case zxx_resomode:
			// Type 5 - Z80 - Z9F controls resonance + filter mode
			if (i < 16) wsprintf(szMidiZXXExt[i], "F0F001%02X", i * 8);
			else if (i < 32) wsprintf(szMidiZXXExt[i], "F0F002%02X", (i - 16) * 8);
			else strcpy(szMidiZXXExt[i], "");
			break;
		}
	}
}


CString MIDIMacroTools::CreateParameteredMacroFromType(enmParameteredMacroType type, int subType)
//-----------------------------------------------------------------------------------------------
{
	CString result;

	switch(type)
	{
	case sfx_unused:
		result = "";
		break;
	case sfx_cutoff:
		result = "F0F000z";
		break;
	case sfx_reso:
		result = "F0F001z";
		break;
	case sfx_mode:
		result = "F0F002z";
		break;
	case sfx_drywet:
		result = "F0F003z";
		break;
	case sfx_cc:
		result.Format("Bc%02Xz", subType);
		break;
	case sfx_plug:
		result.Format("F0F%03Xz", subType + 0x80);
		break;
	}
	return result;
}


// Check if the MIDI Macro configuration used is the default one,
// i.e. the configuration that is assumed when loading a file that has no macros embedded.
bool MIDIMacroTools::IsMacroDefaultSetupUsed() const
//--------------------------------------------------
{
	// TODO - Global macros

	// SF0: Z00-Z7F controls cutoff
	if(GetMacroType(m_SndFile.m_MidiCfg.szMidiSFXExt[0]) != sfx_cutoff)
	{
		return false;
	}
	// Z80-Z8F controls resonance
	if(GetZxxType(m_SndFile.m_MidiCfg.szMidiZXXExt) != zxx_reso4Bit)
	{
		return false;
	}
	// All other parametered macros are unused
	for(size_t i = 1; i < NUM_MACROS; i++)
	{
		if(GetMacroType(m_SndFile.m_MidiCfg.szMidiSFXExt[i]) != sfx_unused)
		{
			return false;
		}
	}
	return true;
}
