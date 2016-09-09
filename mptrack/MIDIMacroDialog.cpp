/*
 * MIDIMacroDialog.cpp
 * -------------------
 * Purpose: MIDI Macro Configuration Dialog
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "../mptrack/Reporting.h"
#include "../common/StringFixer.h"
#include "Mainfrm.h"
#include "mptrack.h"
#include "resource.h"
#include "MIDIMacroDialog.h"
#include "../soundlib/MIDIEvents.h"
#include "../soundlib/plugins/PlugInterface.h"


OPENMPT_NAMESPACE_BEGIN


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
	ON_COMMAND_RANGE(ID_PLUGSELECT, ID_PLUGSELECT + NUM_MACROS - 1, OnViewAllParams)
	ON_COMMAND_RANGE(ID_PLUGSELECT + NUM_MACROS, ID_PLUGSELECT + NUM_MACROS + NUM_MACROS - 1, OnSetSFx)
END_MESSAGE_MAP()


void CMidiMacroSetup::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1,	m_CbnSFx);
	DDX_Control(pDX, IDC_COMBO2,	m_CbnSFxPreset);
	DDX_Control(pDX, IDC_COMBO3,	m_CbnZxxPreset);
	DDX_Control(pDX, IDC_COMBO4,	m_CbnZxx);
	DDX_Control(pDX, IDC_EDIT1,		m_EditSFx);
	DDX_Control(pDX, IDC_EDIT2,		m_EditZxx);
	DDX_Control(pDX, IDC_MACROPLUG, m_CbnMacroPlug);
	DDX_Control(pDX, IDC_MACROPARAM, m_CbnMacroParam);
	DDX_Control(pDX, IDC_MACROCC,   m_CbnMacroCC);
}


BOOL CMidiMacroSetup::OnInitDialog()
//----------------------------------
{
	char s[128];
	CDialog::OnInitDialog();
	CheckDlgButton(IDC_CHECK1, m_bEmbed ? BST_CHECKED : BST_UNCHECKED);
	m_EditSFx.SetLimitText(MACRO_LENGTH - 1);
	m_EditZxx.SetLimitText(MACRO_LENGTH - 1);

	// Parametered macro selection
	for(int isfx = 0; isfx < 16; isfx++)
	{
		wsprintf(s, "%d (SF%X)", isfx, isfx);
		m_CbnSFx.AddString(s);
	}

	// Parametered macro presets
	m_CbnSFx.SetCurSel(0);
	for(int i = 0; i < sfx_max; i++)
	{
		m_CbnSFxPreset.SetItemData(m_CbnSFxPreset.AddString(m_MidiCfg.GetParameteredMacroName(static_cast<parameteredMacroType>(i))), i);
	}
	OnSFxChanged();

	// MIDI CC selection box
	for (int cc = MIDIEvents::MIDICC_start; cc <= MIDIEvents::MIDICC_end; cc++)
	{
		wsprintf(s, "CC %02d %s", cc, MIDIEvents::MidiCCNames[cc]);
		m_CbnMacroCC.SetItemData(m_CbnMacroCC.AddString(s), cc);	
	}

	// Z80...ZFF box
	for(int zxx = 0; zxx < 128; zxx++)
	{
		wsprintf(s, "Z%02X", zxx | 0x80);
		m_CbnZxx.AddString(s);
	}

	// Fixed macro presets
	m_CbnZxx.SetCurSel(0);
	for(int i = 0; i < zxx_max; i++)
	{
		m_CbnZxxPreset.SetItemData(m_CbnZxxPreset.AddString(m_MidiCfg.GetFixedMacroName(static_cast<fixedMacroType>(i))), i);
	}
	m_CbnZxxPreset.SetCurSel(m_MidiCfg.GetFixedMacroType());

	UpdateDialog();

#define ScalePixels(x) Util::ScalePixels(x, m_hWnd)
	int offsetx = ScalePixels(108), offsety = ScalePixels(30), separatorx = ScalePixels(4), separatory = ScalePixels(2);
	int height = ScalePixels(18), widthMacro = ScalePixels(30), widthVal = ScalePixels(90), widthType = ScalePixels(135), widthBtn = ScalePixels(70);
#undef ScalePixels

	for(UINT m = 0; m < NUM_MACROS; m++)
	{
		m_EditMacro[m].Create("", /*BS_FLAT |*/ WS_CHILD | WS_VISIBLE | WS_TABSTOP /*| WS_BORDER*/,
			CRect(offsetx, offsety + m * (separatory + height), offsetx + widthMacro, offsety + m * (separatory + height) + height), this, ID_PLUGSELECT + NUM_MACROS + m);
		m_EditMacro[m].SetFont(GetFont());

		m_EditMacroType[m].Create(ES_READONLY | WS_CHILD| WS_VISIBLE | WS_TABSTOP | WS_BORDER, 
			CRect(offsetx + separatorx + widthMacro, offsety + m * (separatory + height), offsetx + widthMacro + widthType, offsety + m * (separatory + height) + height), this, ID_PLUGSELECT + NUM_MACROS + m);
		m_EditMacroType[m].SetFont(GetFont());

		m_EditMacroValue[m].Create(ES_CENTER | ES_READONLY | WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, 
			CRect(offsetx + separatorx + widthType + widthMacro, offsety + m * (separatory + height), offsetx + widthMacro + widthType + widthVal, offsety + m * (separatory + height) + height), this, ID_PLUGSELECT + NUM_MACROS + m);
		m_EditMacroValue[m].SetFont(GetFont());

		m_BtnMacroShowAll[m].Create("Show All...", WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			CRect(offsetx + separatorx + widthType + widthMacro + widthVal, offsety + m * (separatory + height), offsetx + widthMacro + widthType + widthVal + widthBtn, offsety + m * (separatory + height) + height), this, ID_PLUGSELECT + m);
		m_BtnMacroShowAll[m].SetFont(GetFont());
	}
	UpdateMacroList();

#ifndef NO_PLUGINS
	for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
	{
		const SNDMIXPLUGIN &plugin = m_SndFile.m_MixPlugins[i];

		if(plugin.IsValidPlugin())
		{
			wsprintf(s, "FX%d: %s", i + 1, plugin.GetName());
			m_CbnMacroPlug.SetItemData(m_CbnMacroPlug.AddString(s), i);
		}
	}
#endif // NO_PLUGINS
	m_CbnMacroPlug.SetCurSel(0);
	OnPlugChanged();
	return FALSE;
}


// macro == -1 for updating all macros at once
void CMidiMacroSetup::UpdateMacroList(int macro)
//----------------------------------------------
{
	if (!m_EditMacro[0])
	{
		// GUI not yet initialized
		return;
	}

	int start, end;

	if (macro >= 0 && macro < 16)
	{
		start = end = macro;
	} else
	{
		start = 0;
		end = NUM_MACROS - 1;
	}

	CString s;
	const int selectedMacro = m_CbnSFx.GetCurSel();

	for (int m = start; m <= end; m++)
	{
		// SFx
		s.Format(_T("SF%X"), m);
		m_EditMacro[m].SetWindowText(s);

		// Macro value:
		m_EditMacroValue[m].SetWindowText(m_MidiCfg.szMidiSFXExt[m]);
		m_EditMacroValue[m].SetBackColor(m == selectedMacro ? RGB(200, 200, 225) : RGB(245, 245, 245));

		// Macro Type:
		const parameteredMacroType macroType = m_MidiCfg.GetParameteredMacroType(m);
		switch (macroType)
		{
		case sfx_plug: 
			s.Format(_T("Control Plugin Param %u"), m_MidiCfg.MacroToPlugParam(m));
			break;

		default:
			s = m_MidiCfg.GetParameteredMacroName(m);
			break;
		}
		m_EditMacroType[m].SetWindowText(s);
		m_EditMacroType[m].SetBackColor(m == selectedMacro ? RGB(200,200,225) : RGB(245,245,245));

		// Param details button:
		m_BtnMacroShowAll[m].ShowWindow((macroType == sfx_plug) ? SW_SHOW : SW_HIDE);
	}
}


void CMidiMacroSetup::UpdateDialog()
//----------------------------------
{
	char s[MACRO_LENGTH];
	UINT sfx, sfx_preset, zxx;

	sfx = m_CbnSFx.GetCurSel();
	sfx_preset = m_CbnSFxPreset.GetItemData(m_CbnSFxPreset.GetCurSel());
	if(sfx < 16)
	{
		ToggleBoxes(sfx_preset, sfx);
		memcpy(s, m_MidiCfg.szMidiSFXExt[sfx], MACRO_LENGTH);
		mpt::String::SetNullTerminator(s);
		m_EditSFx.SetWindowText(s);
	}

	zxx = m_CbnZxx.GetCurSel();
	if(zxx < 0x80)
	{
		memcpy(s, m_MidiCfg.szMidiZXXExt[zxx], MACRO_LENGTH);
		mpt::String::SetNullTerminator(s);
		m_EditZxx.SetWindowText(s);
	}
	UpdateMacroList();
}


void CMidiMacroSetup::OnSetAsDefault()
//------------------------------------
{
	theApp.SetDefaultMidiMacro(m_MidiCfg);
}


void CMidiMacroSetup::OnResetCfg()
//--------------------------------
{
	theApp.GetDefaultMidiMacro(m_MidiCfg);
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
		int preset = m_MidiCfg.GetParameteredMacroType(sfx);
		m_CbnSFxPreset.SetCurSel(preset);
	}
	UpdateDialog();
}


void CMidiMacroSetup::OnSFxPresetChanged()
//----------------------------------------
{
	UINT sfx = m_CbnSFx.GetCurSel();
	parameteredMacroType sfx_preset = static_cast<parameteredMacroType>(m_CbnSFxPreset.GetItemData(m_CbnSFxPreset.GetCurSel()));

	if (sfx < 16)
	{
		if(sfx_preset != sfx_custom)
		{
			m_MidiCfg.CreateParameteredMacro(sfx, sfx_preset);
		}
		UpdateDialog();
	}
}


void CMidiMacroSetup::OnZxxPresetChanged()
//----------------------------------------
{
	fixedMacroType zxx_preset = static_cast<fixedMacroType>(m_CbnZxxPreset.GetItemData(m_CbnZxxPreset.GetCurSel()));

	if (zxx_preset != zxx_custom)
	{
		m_MidiCfg.CreateFixedMacro(zxx_preset);
		UpdateDialog();
	}
}


void CMidiMacroSetup::OnSFxEditChanged()
//--------------------------------------
{
	UINT sfx = m_CbnSFx.GetCurSel();
	if (sfx < 16)
	{
		if(ValidateMacroString(m_EditSFx, m_MidiCfg.szMidiSFXExt[sfx], true))
		{
			char s[MACRO_LENGTH];
			MemsetZero(s);
			m_EditSFx.GetWindowText(s, MACRO_LENGTH);
			mpt::String::SetNullTerminator(s);
			memcpy(m_MidiCfg.szMidiSFXExt[sfx], s, MACRO_LENGTH);

			int sfx_preset = m_MidiCfg.GetParameteredMacroType(sfx);
			m_CbnSFxPreset.SetCurSel(sfx_preset);
			ToggleBoxes(sfx_preset, sfx);
			UpdateMacroList(sfx);
		}
	}
}


void CMidiMacroSetup::OnZxxEditChanged()
//--------------------------------------
{
	UINT zxx = m_CbnZxx.GetCurSel();
	if (zxx < 128)
	{
		if(ValidateMacroString(m_EditZxx, m_MidiCfg.szMidiZXXExt[zxx], false))
		{
			char s[MACRO_LENGTH];
			MemsetZero(s);
			m_EditZxx.GetWindowText(s, MACRO_LENGTH);
			mpt::String::SetNullTerminator(s);
			memcpy(m_MidiCfg.szMidiZXXExt[zxx], s, MACRO_LENGTH);
			m_CbnZxxPreset.SetCurSel(m_MidiCfg.GetFixedMacroType());
		}
	}
}

void CMidiMacroSetup::OnSetSFx(UINT id)
//-------------------------------------
{
	m_CbnSFx.SetCurSel(id - (ID_PLUGSELECT + NUM_MACROS));
	OnSFxChanged();
}

void CMidiMacroSetup::OnViewAllParams(UINT id)
//--------------------------------------------
{
#ifndef NO_PLUGINS
	CString message, plugName, line;
	int sfx = id - ID_PLUGSELECT;
	int param = m_MidiCfg.MacroToPlugParam(sfx);
	message.Format("These are the parameters that can be controlled by macro SF%X:\n\n", sfx);

	for(PLUGINDEX plug = 0; plug < MAX_MIXPLUGINS; plug++)
	{
		plugName = m_SndFile.m_MixPlugins[plug].GetName();
		if(m_SndFile.m_MixPlugins[plug].Info.dwPluginId1 != 0)
		{
			IMixPlugin *pVstPlugin = m_SndFile.m_MixPlugins[plug].pMixPlugin;
			if(pVstPlugin && param <= pVstPlugin->GetNumParameters())
			{
				line.Format("FX%d: %s\t %s\n", plug + 1, plugName, pVstPlugin->GetFormattedParamName(param));
				message += line;
			}
		}
	}

	Reporting::Notification(message, "Macro -> Params");
#endif // NO_PLUGINS
}

void CMidiMacroSetup::OnPlugChanged()
//-----------------------------------
{
#ifndef NO_PLUGINS
	int plug = m_CbnMacroPlug.GetItemData(m_CbnMacroPlug.GetCurSel());

	if (plug < 0 || plug > MAX_MIXPLUGINS)
		return;

	IMixPlugin *pVstPlugin = m_SndFile.m_MixPlugins[plug].pMixPlugin;
	if (pVstPlugin != nullptr)
	{
		m_CbnMacroParam.SetRedraw(FALSE);
		m_CbnMacroParam.Clear();
		m_CbnMacroParam.ResetContent();
		AddPluginParameternamesToCombobox(m_CbnMacroParam, *pVstPlugin);
		m_CbnMacroParam.SetRedraw(TRUE);

		int param = m_MidiCfg.MacroToPlugParam(m_CbnSFx.GetCurSel());
		m_CbnMacroParam.SetCurSel(param);
	}
#endif // NO_PLUGINS
}

void CMidiMacroSetup::OnPlugParamChanged()
//----------------------------------------
{
	UINT param = m_CbnMacroParam.GetItemData(m_CbnMacroParam.GetCurSel());

	if(param < 384)
	{
		const std::string macroText = m_MidiCfg.CreateParameteredMacro(sfx_plug, param);
		m_EditSFx.SetWindowText(macroText.c_str());
	} else
	{
		Reporting::Notification("Only parameters 0 to 383 can be controlled using MIDI Macros. Use Parameter Control Events to automate higher parameters.");
	}	
}

void CMidiMacroSetup::OnCCChanged()
//---------------------------------
{
	UINT cc = m_CbnMacroCC.GetItemData(m_CbnMacroCC.GetCurSel());
	const std::string macroText = m_MidiCfg.CreateParameteredMacro(sfx_cc, cc);
	m_EditSFx.SetWindowText(macroText.c_str());
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
		SetDlgItemText(IDC_GENMACROLABEL, _T("Plugin/Param"));
		m_CbnMacroParam.SetCurSel(m_MidiCfg.MacroToPlugParam(sfx));
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
		SetDlgItemText(IDC_GENMACROLABEL, _T("MIDI CC"));
		m_CbnMacroCC.SetCurSel(m_MidiCfg.MacroToMidiCC(sfx));
	} else
	{
		m_CbnMacroCC.EnableWindow(FALSE);
	}
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
		} else if(c >= 'd' && c <= 'f')	// abc have special meanings, but def can be fixed
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
	} else
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


OPENMPT_NAMESPACE_END
