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
#include "../common/mptStringBuffer.h"
#include "Mainfrm.h"
#include "Mptrack.h"
#include "resource.h"
#include "MIDIMacroDialog.h"
#include "../soundlib/MIDIEvents.h"
#include "../soundlib/plugins/PlugInterface.h"


OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(CMidiMacroSetup, CDialog)
	ON_COMMAND(IDC_BUTTON1,			&CMidiMacroSetup::OnSetAsDefault)
	ON_COMMAND(IDC_BUTTON2,			&CMidiMacroSetup::OnResetCfg)
	ON_COMMAND(IDC_BUTTON3,			&CMidiMacroSetup::OnMacroHelp)
	ON_CBN_SELCHANGE(IDC_COMBO1,	&CMidiMacroSetup::OnSFxChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	&CMidiMacroSetup::OnSFxPresetChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,	&CMidiMacroSetup::OnZxxPresetChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,	&CMidiMacroSetup::UpdateZxxSelection)
	ON_CBN_SELCHANGE(IDC_MACROPLUG, &CMidiMacroSetup::OnPlugChanged)
	ON_CBN_SELCHANGE(IDC_MACROPARAM,&CMidiMacroSetup::OnPlugParamChanged)
	ON_CBN_SELCHANGE(IDC_MACROCC,	&CMidiMacroSetup::OnCCChanged)
	ON_EN_CHANGE(IDC_EDIT1,			&CMidiMacroSetup::OnSFxEditChanged)
	ON_EN_CHANGE(IDC_EDIT2,			&CMidiMacroSetup::OnZxxEditChanged)
	ON_COMMAND_RANGE(ID_PLUGSELECT, ID_PLUGSELECT + NUM_MACROS - 1, &CMidiMacroSetup::OnViewAllParams)
	ON_COMMAND_RANGE(ID_PLUGSELECT + NUM_MACROS, ID_PLUGSELECT + NUM_MACROS + NUM_MACROS - 1, &CMidiMacroSetup::OnSetSFx)
END_MESSAGE_MAP()


void CMidiMacroSetup::DoDataExchange(CDataExchange* pDX)
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
{
	CString s;
	CDialog::OnInitDialog();
	m_EditSFx.SetLimitText(MACRO_LENGTH - 1);
	m_EditZxx.SetLimitText(MACRO_LENGTH - 1);

	// Parametered macro selection
	for(int i = 0; i < 16; i++)
	{
		s.Format(_T("%d (SF%X)"), i, i);
		m_CbnSFx.AddString(s);
	}

	// Parametered macro presets
	m_CbnSFx.SetCurSel(0);
	for(int i = 0; i < kSFxMax; i++)
	{
		m_CbnSFxPreset.SetItemData(m_CbnSFxPreset.AddString(m_MidiCfg.GetParameteredMacroName(static_cast<ParameteredMacro>(i))), i);
	}
	OnSFxChanged();

	// MIDI CC selection box
	for (int cc = MIDIEvents::MIDICC_start; cc <= MIDIEvents::MIDICC_end; cc++)
	{
		s.Format(_T("CC %02d "), cc);
		s += mpt::ToCString(mpt::Charset::UTF8, MIDIEvents::MidiCCNames[cc]);
		m_CbnMacroCC.SetItemData(m_CbnMacroCC.AddString(s), cc);
	}

	// Z80...ZFF box
	for(int zxx = 0x80; zxx <= 0xFF; zxx++)
	{
		s.Format(_T("Z%02X"), zxx);
		m_CbnZxx.AddString(s);
	}

	// Fixed macro presets
	m_CbnZxx.SetCurSel(0);
	for(int i = 0; i < kZxxMax; i++)
	{
		m_CbnZxxPreset.SetItemData(m_CbnZxxPreset.AddString(m_MidiCfg.GetFixedMacroName(static_cast<FixedMacro>(i))), i);
	}
	m_CbnZxxPreset.SetCurSel(m_MidiCfg.GetFixedMacroType());

	UpdateDialog();

#define ScalePixels(x) Util::ScalePixels(x, m_hWnd)
	int offsetx = ScalePixels(19), offsety = ScalePixels(30), separatorx = ScalePixels(4), separatory = ScalePixels(2);
	int height = ScalePixels(18), widthMacro = ScalePixels(30), widthVal = ScalePixels(179), widthType = ScalePixels(135), widthBtn = ScalePixels(70);
#undef ScalePixels

	for(UINT m = 0; m < NUM_MACROS; m++)
	{
		m_EditMacro[m].Create(_T(""), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			CRect(offsetx, offsety + m * (separatory + height), offsetx + widthMacro, offsety + m * (separatory + height) + height), this, ID_PLUGSELECT + NUM_MACROS + m);
		m_EditMacro[m].SetFont(GetFont());

		m_EditMacroType[m].Create(ES_READONLY | WS_CHILD| WS_VISIBLE | WS_TABSTOP | WS_BORDER, 
			CRect(offsetx + separatorx + widthMacro, offsety + m * (separatory + height), offsetx + widthMacro + widthType, offsety + m * (separatory + height) + height), this, ID_PLUGSELECT + NUM_MACROS + m);
		m_EditMacroType[m].SetFont(GetFont());

		m_EditMacroValue[m].Create(ES_CENTER | ES_READONLY | WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, 
			CRect(offsetx + separatorx + widthType + widthMacro, offsety + m * (separatory + height), offsetx + widthMacro + widthType + widthVal, offsety + m * (separatory + height) + height), this, ID_PLUGSELECT + NUM_MACROS + m);
		m_EditMacroValue[m].SetFont(GetFont());

		m_BtnMacroShowAll[m].Create(_T("Show All..."), WS_CHILD | WS_TABSTOP | WS_VISIBLE,
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
			s.Format(_T("FX%d: "), i + 1);
			s += mpt::ToCString(plugin.GetName());
			m_CbnMacroPlug.SetItemData(m_CbnMacroPlug.AddString(s), i);
		}
	}
	m_CbnMacroPlug.SetCurSel(0);
	OnPlugChanged();
#endif  // NO_PLUGINS
	return FALSE;
}


// macro == -1 for updating all macros at once
void CMidiMacroSetup::UpdateMacroList(int macro)
{
	if(!m_EditMacro[0])
	{
		// GUI not yet initialized
		return;
	}

	int start, end;

	if(macro >= 0 && macro < 16)
	{
		start = end = macro;
	} else
	{
		start = 0;
		end = NUM_MACROS - 1;
	}

	CString s;
	const int selectedMacro = m_CbnSFx.GetCurSel();

	for(int m = start; m <= end; m++)
	{
		// SFx
		s.Format(_T("SF%X"), static_cast<unsigned int>(m));
		m_EditMacro[m].SetWindowText(s);

		// Macro value:
		m_EditMacroValue[m].SetWindowText(mpt::ToCString(mpt::Charset::ASCII, m_MidiCfg.szMidiSFXExt[m]));
		m_EditMacroValue[m].SetBackColor(m == selectedMacro ? RGB(200, 200, 225) : RGB(245, 245, 245));

		// Macro Type:
		const ParameteredMacro macroType = m_MidiCfg.GetParameteredMacroType(m);
		switch(macroType)
		{
		case kSFxPlugParam:
			s.Format(_T("Control Plugin Param %u"), static_cast<unsigned int>(m_MidiCfg.MacroToPlugParam(m)));
			break;

		default:
			s = m_MidiCfg.GetParameteredMacroName(m);
			break;
		}
		m_EditMacroType[m].SetWindowText(s);
		m_EditMacroType[m].SetBackColor(m == selectedMacro ? RGB(200,200,225) : RGB(245,245,245));

		// Param details button:
		m_BtnMacroShowAll[m].ShowWindow((macroType == kSFxPlugParam) ? SW_SHOW : SW_HIDE);
	}
}


void CMidiMacroSetup::UpdateDialog()
{
	UINT sfx = m_CbnSFx.GetCurSel();
	UINT sfx_preset = static_cast<UINT>(m_CbnSFxPreset.GetItemData(m_CbnSFxPreset.GetCurSel()));
	if(sfx < std::size(m_MidiCfg.szMidiSFXExt))
	{
		ToggleBoxes(sfx_preset, sfx);
		m_EditSFx.SetWindowText(mpt::ToCString(mpt::Charset::ASCII, m_MidiCfg.szMidiSFXExt[sfx]));
	}

	UpdateZxxSelection();
	UpdateMacroList();
}


void CMidiMacroSetup::OnSetAsDefault()
{
	theApp.SetDefaultMidiMacro(m_MidiCfg);
}


void CMidiMacroSetup::OnResetCfg()
{
	theApp.GetDefaultMidiMacro(m_MidiCfg);
	m_CbnZxxPreset.SetCurSel(0);
	OnSFxChanged();
}


void CMidiMacroSetup::OnMacroHelp()
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
		"h - Pattern channel\n"
		"m - Sample loop direction\n"
		"o - Last sample offset (Oxx / 9xx)\n"
		"s - SysEx checksum (Roland)\n\n"
		"z - Zxx parameter (00-7F)\n\n"
		"Macros can be up to 31 characters long and contain multiple MIDI messages. SysEx messages are automatically terminated if not specified by the user."),
		_T("OpenMPT MIDI Macro quick reference"));
}


void CMidiMacroSetup::OnSFxChanged()
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
{
	UINT sfx = m_CbnSFx.GetCurSel();
	ParameteredMacro sfx_preset = static_cast<ParameteredMacro>(m_CbnSFxPreset.GetItemData(m_CbnSFxPreset.GetCurSel()));

	if (sfx < std::size(m_MidiCfg.szMidiSFXExt))
	{
		if(sfx_preset != kSFxCustom)
		{
			m_MidiCfg.CreateParameteredMacro(sfx, sfx_preset);
		}
		UpdateDialog();
	}
}


void CMidiMacroSetup::OnZxxPresetChanged()
{
	FixedMacro zxxPreset = static_cast<FixedMacro>(m_CbnZxxPreset.GetItemData(m_CbnZxxPreset.GetCurSel()));

	if (zxxPreset != kZxxCustom)
	{
		m_MidiCfg.CreateFixedMacro(zxxPreset);
		UpdateDialog();
	}
}


void CMidiMacroSetup::UpdateZxxSelection()
{
	UINT zxx = m_CbnZxx.GetCurSel();
	if(zxx < std::size(m_MidiCfg.szMidiZXXExt))
	{
		m_EditZxx.SetWindowText(mpt::ToCString(mpt::Charset::ASCII, m_MidiCfg.szMidiZXXExt[zxx]));
	}
}


void CMidiMacroSetup::OnSFxEditChanged()
{
	UINT sfx = m_CbnSFx.GetCurSel();
	if (sfx < std::size(m_MidiCfg.szMidiSFXExt))
	{
		if(ValidateMacroString(m_EditSFx, m_MidiCfg.szMidiSFXExt[sfx], true))
		{
			CString s;
			m_EditSFx.GetWindowText(s);
			mpt::String::WriteAutoBuf(m_MidiCfg.szMidiSFXExt[sfx]) = mpt::ToCharset(mpt::Charset::ASCII, s);

			int sfx_preset = m_MidiCfg.GetParameteredMacroType(sfx);
			m_CbnSFxPreset.SetCurSel(sfx_preset);
			ToggleBoxes(sfx_preset, sfx);
			UpdateMacroList(sfx);
		}
	}
}


void CMidiMacroSetup::OnZxxEditChanged()
{
	UINT zxx = m_CbnZxx.GetCurSel();
	if (zxx < std::size(m_MidiCfg.szMidiZXXExt))
	{
		if(ValidateMacroString(m_EditZxx, m_MidiCfg.szMidiZXXExt[zxx], false))
		{
			CString s;
			m_EditZxx.GetWindowText(s);
			mpt::String::WriteAutoBuf(m_MidiCfg.szMidiZXXExt[zxx]) = mpt::ToCharset(mpt::Charset::ASCII, s);
			m_CbnZxxPreset.SetCurSel(m_MidiCfg.GetFixedMacroType());
		}
	}
}

void CMidiMacroSetup::OnSetSFx(UINT id)
{
	m_CbnSFx.SetCurSel(id - (ID_PLUGSELECT + NUM_MACROS));
	OnSFxChanged();
}

void CMidiMacroSetup::OnViewAllParams(UINT id)
{
#ifndef NO_PLUGINS
	CString message, plugName;
	int sfx = id - ID_PLUGSELECT;
	int param = m_MidiCfg.MacroToPlugParam(sfx);
	message.Format(_T("These are the parameters that can be controlled by macro SF%X:\n\n"), sfx);

	for(PLUGINDEX plug = 0; plug < MAX_MIXPLUGINS; plug++)
	{
		IMixPlugin *pVstPlugin = m_SndFile.m_MixPlugins[plug].pMixPlugin;
		if(pVstPlugin && param < pVstPlugin->GetNumParameters())
		{
			plugName = mpt::ToCString(m_SndFile.m_MixPlugins[plug].GetName());
			message.AppendFormat(_T("FX%d: "), plug + 1);
			message += plugName + _T("\t") + pVstPlugin->GetFormattedParamName(param) + _T("\n");
		}
	}

	Reporting::Notification(message, _T("Macro -> Parameters"));
#endif // NO_PLUGINS
}

void CMidiMacroSetup::OnPlugChanged()
{
#ifndef NO_PLUGINS
	DWORD_PTR plug = m_CbnMacroPlug.GetItemData(m_CbnMacroPlug.GetCurSel());

	if(plug >= MAX_MIXPLUGINS)
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
{
	int param = static_cast<int>(m_CbnMacroParam.GetItemData(m_CbnMacroParam.GetCurSel()));

	if(param < 384)
	{
		const std::string macroText = m_MidiCfg.CreateParameteredMacro(kSFxPlugParam, param);
		m_EditSFx.SetWindowText(mpt::ToCString(mpt::Charset::ASCII, macroText));
	} else
	{
		Reporting::Notification("Only parameters 0 to 383 can be controlled using MIDI Macros. Use Parameter Control Events to automate higher parameters.");
	}
}

void CMidiMacroSetup::OnCCChanged()
{
	int cc = static_cast<int>(m_CbnMacroCC.GetItemData(m_CbnMacroCC.GetCurSel()));
	const std::string macroText = m_MidiCfg.CreateParameteredMacro(kSFxCC, cc);
	m_EditSFx.SetWindowText(mpt::ToCString(mpt::Charset::ASCII, macroText));
}

void CMidiMacroSetup::ToggleBoxes(UINT sfxPreset, UINT sfx)
{

	if (sfxPreset == kSFxPlugParam)
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

	if (sfxPreset == kSFxCC)
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
{
	CString macroStrT;
	wnd.GetWindowText(macroStrT);
	std::string macroStr = mpt::ToCharset(mpt::Charset::ASCII, macroStrT);

	bool allowed = true, caseChange = false;
	for(char &c : macroStr)
	{
		if(c == 'k' || c == 'K')		// Previously, 'K' was used for MIDI channel
		{
			caseChange = true;
			c = 'c';
		} else if(c >= 'd' && c <= 'f')	// abc have special meanings, but def can be fixed
		{
			caseChange = true;
			c = c - 'a' + 'A';
		} else if(c == 'M' || c == 'N' || c == 'O' || c == 'P' || c == 'S' || c == 'U' || c == 'V' || c == 'X' || c == 'Y' || c == 'Z')
		{
			caseChange = true;
			c = c - 'A' + 'a';
		} else if(!(
			(c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'c') ||
			(c == 'h' || c == 'm' || c == 'n' || c == 'o' || c == 'p' || c == 's' ||c == 'u' || c == 'v' || c == 'x' || c == 'y' || c == ' ') ||
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
		wnd.SetWindowText(mpt::ToCString(mpt::Charset::ASCII, lastMacro));
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
			wnd.SetWindowText(mpt::ToCString(mpt::Charset::ASCII, macroStr));
			wnd.SetSel(start, end, true);
		}
		return true;
	}
}


OPENMPT_NAMESPACE_END
