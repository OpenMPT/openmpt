/*
 * MIDIMacros.cpp
 * --------------
 * Purpose: Helper functions / classes for MIDI Macro functionality, including the MIDI Macro editor.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */

#include "stdafx.h"
#include "../common/Reporting.h"
#include "../common/StringFixer.h"
#include "Mainfrm.h"
#include "mptrack.h"
#include "Vstplug.h"
#include "resource.h"
#include "MIDIMacros.h"



enmParameteredMacroType MIDIMacroTools::GetMacroType(CString value)
//-----------------------------------------------------------------
{
	value.Remove(' ');
	if (value.Compare("") == 0) return sfx_unused;
	if (value.Compare("F0F000z") == 0) return sfx_cutoff;
	if (value.Compare("F0F001z") == 0) return sfx_reso;
	if (value.Compare("F0F002z") == 0) return sfx_mode;
	if (value.Compare("F0F003z") == 0) return sfx_drywet;
	if (value.Compare("Bc00z") >= 0 && value.Compare("BcFFz") <= 0 && value.GetLength() == 5)
		return sfx_cc;
	if (value.Compare("F0F080z") >= 0 && value.Compare("F0F1FFz") <= 0 && value.GetLength() == 7)
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

			if(plugin < MAX_MIXPLUGINS)
			{
				CVstPlugin *pPlug = (CVstPlugin*)m_SndFile.m_MixPlugins[plugin].pMixPlugin;
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


////////////////////////////////////////////////////////////////////////
// MIDI Macro Configuration Dialog

BEGIN_MESSAGE_MAP(CMidiMacroSetup, CDialog)
	ON_COMMAND(IDC_CHECK1,			OnEmbedMidiCfg)
	ON_COMMAND(IDC_BUTTON1,			OnSetAsDefault)
	ON_COMMAND(IDC_BUTTON2,			OnResetCfg)
	ON_COMMAND(IDC_BUTTON3,			OnMacroHelp)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnSFxChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnSFxPresetChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,	OnZxxPresetChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,	UpdateDialog)
	ON_CBN_SELCHANGE(IDC_MACROPLUG, OnPlugChanged)
	ON_CBN_SELCHANGE(IDC_MACROPARAM,OnPlugParamChanged)
	ON_CBN_SELCHANGE(IDC_MACROCC,	OnCCChanged)
	ON_EN_CHANGE(IDC_EDIT1,			OnSFxEditChanged)
	ON_EN_CHANGE(IDC_EDIT2,			OnZxxEditChanged)
	ON_COMMAND_RANGE(ID_PLUGSELECT, ID_PLUGSELECT + NUM_MACROS - 1, OnViewAllParams) //rewbs.patPlugName
	ON_COMMAND_RANGE(ID_PLUGSELECT + NUM_MACROS, ID_PLUGSELECT + NUM_MACROS + NUM_MACROS - 1, OnSetSFx) //rewbs.patPlugName
END_MESSAGE_MAP()


void CMidiMacroSetup::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModTypeDlg)
	DDX_Control(pDX, IDC_COMBO1,	m_CbnSFx);
	DDX_Control(pDX, IDC_COMBO2,	m_CbnSFxPreset);
	DDX_Control(pDX, IDC_COMBO3,	m_CbnZxxPreset);
	DDX_Control(pDX, IDC_COMBO4,	m_CbnZxx);
	DDX_Control(pDX, IDC_EDIT1,		m_EditSFx);
	DDX_Control(pDX, IDC_EDIT2,		m_EditZxx);
	DDX_Control(pDX, IDC_MACROPLUG, m_CbnMacroPlug);
	DDX_Control(pDX, IDC_MACROPARAM, m_CbnMacroParam);
	DDX_Control(pDX, IDC_MACROCC,   m_CbnMacroCC);
	//}}AFX_DATA_MAP
}


BOOL CMidiMacroSetup::OnInitDialog()
//----------------------------------
{
	CHAR s[128];
	CDialog::OnInitDialog();
	CheckDlgButton(IDC_CHECK1, m_bEmbed ? BST_CHECKED : BST_UNCHECKED);
	m_EditSFx.SetLimitText(MACRO_LENGTH - 1);
	m_EditZxx.SetLimitText(MACRO_LENGTH - 1);

	for (UINT isfx=0; isfx<16; isfx++)
	{
		wsprintf(s, "%d (SF%X)", isfx, isfx);
		m_CbnSFx.AddString(s);
	}
	m_CbnSFx.SetCurSel(0);
	for(int i = 0; i < sfx_max; i++)
	{
		m_CbnSFxPreset.SetItemData(m_CbnSFxPreset.AddString(macroTools.GetMacroName(static_cast<enmParameteredMacroType>(i))), i);
	}
	OnSFxChanged();

	for (int cc = MIDICC_start; cc <= MIDICC_end; cc++)
	{
		wsprintf(s, "CC %02d %s", cc, MidiCCNames[cc]);
		m_CbnMacroCC.SetItemData(m_CbnMacroCC.AddString(s), cc);	
	}

	for (int zxx = 0; zxx < 128; zxx++)
	{
		wsprintf(s, "Z%02X", zxx | 0x80);
		m_CbnZxx.AddString(s);
	}
	m_CbnZxx.SetCurSel(0);
	m_CbnZxxPreset.AddString("Custom");
	m_CbnZxxPreset.AddString("Z80-Z8F controls resonance");
	m_CbnZxxPreset.AddString("Z80-ZFF controls resonance");
	m_CbnZxxPreset.AddString("Z80-ZFF controls cutoff");
	m_CbnZxxPreset.AddString("Z80-ZFF controls filter mode");
	m_CbnZxxPreset.AddString("Z80-Z9F controls resonance+mode");
	m_CbnZxxPreset.SetCurSel(macroTools.GetZxxType(m_MidiCfg.szMidiZXXExt));
	UpdateDialog();

	int offsetx=108, offsety=30, separatorx=4, separatory=2, 
		height=18, widthMacro=30, widthVal=90, widthType=135, widthBtn=70;

	for (UINT m = 0; m < NUM_MACROS; m++)
	{
		m_EditMacro[m].Create("", /*BS_FLAT |*/ WS_CHILD | WS_VISIBLE | WS_TABSTOP /*| WS_BORDER*/,
			CRect(offsetx, offsety + m * (separatory + height), offsetx + widthMacro, offsety + m * (separatory + height) + height), this, ID_PLUGSELECT + NUM_MACROS + m);
		m_EditMacro[m].SetFont(GetFont());

		m_EditMacroType[m].Create(ES_READONLY | WS_CHILD| WS_VISIBLE | WS_TABSTOP | WS_BORDER, 
			CRect(offsetx + separatorx + widthMacro, offsety + m* (separatory + height), offsetx + widthMacro + widthType, offsety + m * (separatory + height) + height), this, ID_PLUGSELECT + NUM_MACROS + m);
		m_EditMacroType[m].SetFont(GetFont());

		m_EditMacroValue[m].Create(ES_CENTER | ES_READONLY | WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, 
			CRect(offsetx + separatorx + widthType + widthMacro, offsety + m * (separatory + height), offsetx + widthMacro + widthType + widthVal, offsety + m * (separatory + height) + height), this, ID_PLUGSELECT + NUM_MACROS + m);
		m_EditMacroValue[m].SetFont(GetFont());

		m_BtnMacroShowAll[m].Create("Show All...", WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			CRect(offsetx + separatorx + widthType + widthMacro + widthVal, offsety + m *(separatory + height), offsetx + widthMacro + widthType + widthVal + widthBtn, offsety + m * (separatory + height) + height), this, ID_PLUGSELECT + m);
		m_BtnMacroShowAll[m].SetFont(GetFont());
	}
	UpdateMacroList();

	for (UINT plug=0; plug<MAX_MIXPLUGINS; plug++)
	{
		PSNDMIXPLUGIN p = &(m_SndFile.m_MixPlugins[plug]);
		StringFixer::SetNullTerminator(p->Info.szLibraryName);
		if (p->Info.szLibraryName[0])
		{
			wsprintf(s, "FX%d: %s", plug+1, p->Info.szName);
			m_CbnMacroPlug.SetItemData(m_CbnMacroPlug.AddString(s), plug);
		}
	}
	m_CbnMacroPlug.SetCurSel(0);
	OnPlugChanged();
	return FALSE;
}


void CMidiMacroSetup::UpdateMacroList(int macro) //-1 for all macros
//----------------------------------------------
{
	if (!m_EditMacro[0])
		return; //GUI not yet initialized

	CString s, macroText;
	UINT start, end, macroType;
	int selectedMacro=m_CbnSFx.GetCurSel();

	if (macro >= 0 && macro < 16)
	{
		start=macro;
		end=macro;
	} else
	{
		start=0;
		end=NUM_MACROS;
	}

	for (int m=0; m<NUM_MACROS; m++)
	{
		//SFx
		s.Format("SF%X", m);
		m_EditMacro[m].SetWindowText(s);

		//Macro value:
		CString macroText = m_MidiCfg.szMidiSFXExt[m];
		m_EditMacroValue[m].SetWindowText(macroText);
		m_EditMacroValue[m].SetBackColor(m == selectedMacro ? RGB(200, 200, 225) : RGB(245, 245, 245));

		//Macro Type:
		macroType = macroTools.GetMacroType(macroText);
		switch (macroType)
		{
		case sfx_unused: s = "Unused"; break;
		case sfx_cutoff: s = "Set Filter Cutoff"; break;
		case sfx_reso: s = "Set Filter Resonance"; break;
		case sfx_mode: s = "Set Filter Mode"; break;
		case sfx_drywet: s = "Set Plugin dry/wet ratio"; break;
		case sfx_cc:
			s.Format("MIDI CC %d", macroTools.MacroToMidiCC(macroText)); 
			break;
		case sfx_plug: 
			s.Format("Control Plugin Param %d", macroTools.MacroToPlugParam(macroText)); 
			break;
		case sfx_custom: 
		default: s = "Custom";
		}
		m_EditMacroType[m].SetWindowText(s);
		m_EditMacroType[m].SetBackColor(m == selectedMacro ? RGB(200,200,225) : RGB(245,245,245) );

		//Param details button:
		if (macroType == sfx_plug)
			m_BtnMacroShowAll[m].ShowWindow(SW_SHOW);
		else 
			m_BtnMacroShowAll[m].ShowWindow(SW_HIDE);
	}
}


void CMidiMacroSetup::UpdateDialog()
//----------------------------------
{
	CHAR s[MACRO_LENGTH];
	UINT sfx, sfx_preset, zxx;

	sfx = m_CbnSFx.GetCurSel();
	sfx_preset = m_CbnSFxPreset.GetItemData(m_CbnSFxPreset.GetCurSel());
	if (sfx < 16)
	{
		ToggleBoxes(sfx_preset, sfx);
		memcpy(s, m_MidiCfg.szMidiSFXExt[sfx], MACRO_LENGTH);
		StringFixer::SetNullTerminator(s);
		m_EditSFx.SetWindowText(s);
	}

	zxx = m_CbnZxx.GetCurSel();
	if (zxx < 0x80)
	{
		memcpy(s, m_MidiCfg.szMidiZXXExt[zxx], MACRO_LENGTH);
		StringFixer::SetNullTerminator(s);
		m_EditZxx.SetWindowText(s);
	}
	UpdateMacroList();
}


void CMidiMacroSetup::OnSetAsDefault()
//------------------------------------
{
	theApp.SetDefaultMidiMacro(&m_MidiCfg);
}


void CMidiMacroSetup::OnResetCfg()
//--------------------------------
{
	theApp.GetDefaultMidiMacro(&m_MidiCfg);
	m_CbnZxxPreset.SetCurSel(0);
	OnSFxChanged();
}


void CMidiMacroSetup::OnMacroHelp()
//---------------------------------
{
	Reporting::Information(_T("Valid characters in macros:\n\n"
		"0-9, A-F - Raw hex data (4-Bit value)\n"
		"c - MIDI channel (4-Bit value)\n"
		"n - Note value\n\n"
		"v - Note velocity\n"
		"u - Computed note volume (including envelopes)\n\n"
		"x - Note panning\n"
		"y - Computed panning (including envelopes)\n\n"
		"a - High byte of bank select\n"
		"b - Low byte of bank select\n"
		"p - Program select\n\n"
		"z - Zxx parameter (00-7F)\n\n"
		"Macros can be up to 31 characters long and contain multiple MIDI messages. SysEx messages are automatically terminated if not specified by the user."),
		_T("OpenMPT MIDI Macro quick reference"));
}


void CMidiMacroSetup::OnEmbedMidiCfg()
//------------------------------------
{
	m_bEmbed = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;
}


void CMidiMacroSetup::OnSFxChanged()
//----------------------------------
{
	UINT sfx = m_CbnSFx.GetCurSel();
	if (sfx < 16)
	{
		CString macroText;
		memcpy(macroText.GetBuffer(MACRO_LENGTH), m_MidiCfg.szMidiSFXExt[sfx], MACRO_LENGTH);
		int preset = macroTools.GetMacroType(macroText);
		m_CbnSFxPreset.SetCurSel(preset);
	}
	UpdateDialog();
}


void CMidiMacroSetup::OnSFxPresetChanged()
//----------------------------------------
{
	UINT sfx = m_CbnSFx.GetCurSel();
	UINT sfx_preset = m_CbnSFxPreset.GetItemData(m_CbnSFxPreset.GetCurSel());

	if (sfx < 16)
	{
		char *pmacro = m_MidiCfg.szMidiSFXExt[sfx];
		switch(sfx_preset)
		{
		case sfx_unused:	strcpy(pmacro, ""); break;			// unused
		case sfx_cutoff:	strcpy(pmacro, "F0F000z"); break;	// cutoff
		case sfx_reso:		strcpy(pmacro, "F0F001z"); break;   // reso
		case sfx_mode:		strcpy(pmacro, "F0F002z"); break;   // mode
		case sfx_drywet:	strcpy(pmacro, "F0F003z"); break;   
		case sfx_cc:		strcpy(pmacro, "Bc00z"); break;		// MIDI cc - TODO: get value from other menus
		case sfx_plug:		strcpy(pmacro, "F0F080z"); break;	// plug param - TODO: get value from other menus
		case sfx_custom:	/*strcpy(pmacro, "z");*/ break;		// custom - leave as is.
		}
		UpdateDialog();
	}
}


void CMidiMacroSetup::OnZxxPresetChanged()
//----------------------------------------
{
	enmFixedMacroType zxx_preset = static_cast<enmFixedMacroType>(m_CbnZxxPreset.GetCurSel());

	if (zxx_preset != zxx_custom)
	{
		macroTools.CreateZxxFromType(m_MidiCfg.szMidiZXXExt, zxx_preset);
		UpdateDialog();
	}
}


void CMidiMacroSetup::OnSFxEditChanged()
//--------------------------------------
{
	CHAR s[MACRO_LENGTH];
	UINT sfx = m_CbnSFx.GetCurSel();
	if (sfx < 16)
	{
		if(ValidateMacroString(m_EditSFx, m_MidiCfg.szMidiSFXExt[sfx], true))
		{
			MemsetZero(s);
			m_EditSFx.GetWindowText(s, MACRO_LENGTH);
			StringFixer::SetNullTerminator(s);
			memcpy(m_MidiCfg.szMidiSFXExt[sfx], s, MACRO_LENGTH);

			int sfx_preset = macroTools.GetMacroType(m_MidiCfg.szMidiSFXExt[sfx]);
			//int param = macroTools.MacroToPlugParam(m_MidiCfg.szMidiSFXExt[sfx]);

			m_CbnSFxPreset.SetCurSel(sfx_preset);
			ToggleBoxes(sfx_preset, sfx);
			UpdateMacroList(sfx);
		}
	}
}


void CMidiMacroSetup::OnZxxEditChanged()
//--------------------------------------
{
	CHAR s[MACRO_LENGTH];
	UINT zxx = m_CbnZxx.GetCurSel();
	if (zxx < 128)
	{
		if(ValidateMacroString(m_EditZxx, m_MidiCfg.szMidiZXXExt[zxx], false))
		{
			MemsetZero(s);
			m_EditZxx.GetWindowText(s, MACRO_LENGTH);
			StringFixer::SetNullTerminator(s);
			memcpy(m_MidiCfg.szMidiZXXExt[zxx], s, MACRO_LENGTH);
		}
	}
}

void CMidiMacroSetup::OnSetSFx(UINT id)
//-------------------------------------
{
	m_CbnSFx.SetCurSel(id-(ID_PLUGSELECT + NUM_MACROS));
	OnSFxChanged();
}

void CMidiMacroSetup::OnViewAllParams(UINT id)
//--------------------------------------------
{
	CString message, plugName, line;
	int sfx = id-ID_PLUGSELECT;
	int param = macroTools.MacroToPlugParam(m_MidiCfg.szMidiSFXExt[sfx]);
	CVstPlugin *pVstPlugin; 
	message.Format("These are the parameters that can be controlled by macro SF%X:\n\n",sfx);

	for (UINT plug=0; plug<MAX_MIXPLUGINS; plug++)
	{
		plugName = m_SndFile.m_MixPlugins[plug].Info.szName;
		if (plugName != "")
		{
			pVstPlugin=(CVstPlugin*) m_SndFile.m_MixPlugins[plug].pMixPlugin;
			if (pVstPlugin && param <= pVstPlugin->GetNumParameters())
			{
				line.Format("FX%d: %s\t %s\n", plug + 1, plugName, pVstPlugin->GetFormattedParamName(param));
				message += line;
			}
		}
	}

	Reporting::Notification(message, "Macro -> Params");
}

void CMidiMacroSetup::OnPlugChanged()
//-----------------------------------
{
	int plug = m_CbnMacroPlug.GetItemData(m_CbnMacroPlug.GetCurSel());

	if (plug < 0 || plug > MAX_MIXPLUGINS)
		return;

	PSNDMIXPLUGIN pPlugin = &m_SndFile.m_MixPlugins[plug];
	CVstPlugin *pVstPlugin = (pPlugin->pMixPlugin) ? (CVstPlugin *)pPlugin->pMixPlugin : NULL;

	if (pVstPlugin)
	{
		m_CbnMacroParam.SetRedraw(FALSE);
		m_CbnMacroParam.Clear();
		m_CbnMacroParam.ResetContent();
		AddPluginParameternamesToCombobox(m_CbnMacroParam, *pVstPlugin);
		m_CbnMacroParam.SetRedraw(TRUE);

		int param = macroTools.MacroToPlugParam(m_MidiCfg.szMidiSFXExt[m_CbnSFx.GetCurSel()]);
		m_CbnMacroParam.SetCurSel(param);
	}
	//OnPlugParamChanged();
}

void CMidiMacroSetup::OnPlugParamChanged()
//----------------------------------------
{
	CString macroText;
	UINT param = m_CbnMacroParam.GetItemData(m_CbnMacroParam.GetCurSel());

	if (param < 128)
	{
		macroText.Format("F0F0%02Xz",param + 128);
		m_EditSFx.SetWindowText(macroText);
	} else if (param < 384)
	{
		macroText.Format("F0F1%02Xz",param - 128);
		m_EditSFx.SetWindowText(macroText);
	} else
	{
		Reporting::Notification("MPT can only assign macros to parameters 0 to 383. Use Parameter Control Notes to automate higher parameters.");
	}	
}

void CMidiMacroSetup::OnCCChanged()
//---------------------------------
{
	CString macroText;
	UINT cc = m_CbnMacroCC.GetItemData(m_CbnMacroCC.GetCurSel());
	macroText.Format("Bc%02Xz", cc & 0xFF);
	m_EditSFx.SetWindowText(macroText);
}

void CMidiMacroSetup::ToggleBoxes(UINT sfx_preset, UINT sfx)
//----------------------------------------------------------
{

	if (sfx_preset == sfx_plug)
	{
		m_CbnMacroCC.ShowWindow(FALSE);
		m_CbnMacroPlug.ShowWindow(TRUE);
		m_CbnMacroParam.ShowWindow(TRUE);
		m_CbnMacroPlug.EnableWindow(TRUE);
		m_CbnMacroParam.EnableWindow(TRUE);
		SetDlgItemText(IDC_GENMACROLABEL, "Plug/Param");
		m_CbnMacroParam.SetCurSel(macroTools.MacroToPlugParam(m_MidiCfg.szMidiSFXExt[sfx]));
	} else
	{
		m_CbnMacroPlug.EnableWindow(FALSE);
		m_CbnMacroParam.EnableWindow(FALSE);
	}

	if (sfx_preset == sfx_cc)
	{
		m_CbnMacroCC.EnableWindow(TRUE);
		m_CbnMacroCC.ShowWindow(TRUE);
		m_CbnMacroPlug.ShowWindow(FALSE);
		m_CbnMacroParam.ShowWindow(FALSE);
		SetDlgItemText(IDC_GENMACROLABEL, "MIDI CC");
		m_CbnMacroCC.SetCurSel(macroTools.MacroToMidiCC(m_MidiCfg.szMidiSFXExt[sfx]));
	} else
	{
		m_CbnMacroCC.EnableWindow(FALSE);
	}

	//m_EditSFx.EnableWindow((sfx_preset == sfx_unused) ? FALSE : TRUE);

}


bool CMidiMacroSetup::ValidateMacroString(CEdit &wnd, char *lastMacro, bool isParametric)
//---------------------------------------------------------------------------------------
{
	CString macroStr;
	wnd.GetWindowText(macroStr);

	bool allowed = true, caseChange = false;
	for(int i = 0; i < macroStr.GetLength(); i++)
	{
		char c = macroStr.GetAt(i);
		if(c == 'k' || c == 'K')			// Previously, 'K' was used for MIDI channel
		{
			caseChange = true;
			macroStr.SetAt(i, 'c');
		} else if (c >= 'd' && c <= 'f')	// abc have special meanings, but def can be fixed
		{
			caseChange = true;
			macroStr.SetAt(i, c - 'a' + 'A');
		} else if(c == 'N' || c == 'V' || c == 'U' || c == 'X' || c == 'Y' || c == 'Z' || c == 'P')
		{
			caseChange = true;
			macroStr.SetAt(i, c - 'A' + 'a');
		} else if(!(
			(c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'c') ||
			(c == 'v' || c == 'u' || c == 'x' || c == 'y' || c == 'p' || c == 'n' || c == ' ') ||
			(c == 'z' && isParametric)))
		{
			allowed = false;
			break;
		}
	}

	if(!allowed)
	{
		// Replace text and keep cursor position if we just typed in an invalid character
		int start, end;
		wnd.GetSel(start, end);
		wnd.SetWindowText(lastMacro);
		wnd.SetSel(start - 1, end - 1, true);
		MessageBeep(MB_OK);
		return false;
	}
	else
	{
		if(caseChange)
		{
			// Replace text and keep cursor position if there was a case conversion
			int start, end;
			wnd.GetSel(start, end);
			wnd.SetWindowText(macroStr);
			wnd.SetSel(start, end, true);
		}
		return true;
	}
}
