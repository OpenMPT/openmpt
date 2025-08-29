/*
 * PatternFindReplaceDlg.cpp
 * -------------------------
 * Purpose: The find/replace dialog for pattern data.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "PatternFindReplaceDlg.h"
#include "DialogBase.h"
#include "Mptrack.h"
#include "PatternFindReplace.h"
#include "resource.h"
#include "View_pat.h"
#include "mpt/parse/parse.hpp"


OPENMPT_NAMESPACE_BEGIN

// CFindRangeDlg: Find a range of values.

class CFindRangeDlg : public DialogBase
{
public:
	enum DisplayMode
	{
		kDecimal,
		kHex,
		kNotes,
	};
protected:
	CComboBox m_cbnMin, m_cbnMax;
	int m_minVal, m_minDefault;
	int m_maxVal, m_maxDefault;
	DisplayMode m_displayMode;

public:
	CFindRangeDlg(CWnd *parent, int minVal, int minDefault, int maxVal, int maxDefault, DisplayMode displayMode) : DialogBase(IDD_FIND_RANGE, parent)
		, m_minVal(minVal)
		, m_minDefault(minDefault)
		, m_maxVal(maxVal)
		, m_maxDefault(maxDefault)
		, m_displayMode(displayMode)
	{ }

	int GetMinVal() const { return m_minVal; }
	int GetMaxVal() const { return m_maxVal; }

protected:
	void DoDataExchange(CDataExchange* pDX) override
	{
		DialogBase::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_COMBO1, m_cbnMin);
		DDX_Control(pDX, IDC_COMBO2, m_cbnMax);
	}

	BOOL OnInitDialog() override
	{
		DialogBase::OnInitDialog();

		if(m_displayMode == kNotes)
		{
			AppendNotesToControl(m_cbnMin, static_cast<ModCommand::NOTE>(m_minVal), static_cast<ModCommand::NOTE>(m_maxVal));
			AppendNotesToControl(m_cbnMax, static_cast<ModCommand::NOTE>(m_minVal), static_cast<ModCommand::NOTE>(m_maxVal));
		} else
		{
			m_cbnMin.InitStorage(m_minVal - m_maxVal + 1, 4);
			m_cbnMax.InitStorage(m_minVal - m_maxVal + 1, 4);
			const TCHAR *formatString;
			if(m_displayMode == kHex && m_maxVal <= 0x0F)
				formatString = _T("%01X");
			else if(m_displayMode == kHex)
				formatString = _T("%02X");
			else
				formatString = _T("%d");
			for(int i = m_minVal; i <= m_maxVal; i++)
			{
				TCHAR s[16];
				wsprintf(s, formatString, i);
				m_cbnMin.SetItemData(m_cbnMin.AddString(s), i);
				m_cbnMax.SetItemData(m_cbnMax.AddString(s), i);
			}
		}
		if(m_minDefault < m_minVal || m_minDefault > m_maxDefault)
		{
			m_minDefault = m_minVal;
			m_maxDefault = m_maxVal;
		}
		m_cbnMin.SetCurSel(m_minDefault - m_minVal);
		m_cbnMax.SetCurSel(m_maxDefault - m_minVal);

		return TRUE;
	}

	void OnOK() override
	{
		DialogBase::OnOK();
		m_minVal = static_cast<int>(m_cbnMin.GetItemData(m_cbnMin.GetCurSel()));
		m_maxVal = static_cast<int>(m_cbnMax.GetItemData(m_cbnMax.GetCurSel()));
		if(m_maxVal < m_minVal)
			std::swap(m_minVal, m_maxVal);
	}
};


BEGIN_MESSAGE_MAP(CFindReplaceTab, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_COMBO1,	&CFindReplaceTab::OnNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	&CFindReplaceTab::OnInstrChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,	&CFindReplaceTab::OnVolCmdChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,	&CFindReplaceTab::OnVolumeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5,	&CFindReplaceTab::OnEffectChanged)
	ON_CBN_SELCHANGE(IDC_COMBO6,	&CFindReplaceTab::OnParamChanged)
	ON_CBN_SELCHANGE(IDC_COMBO7,	&CFindReplaceTab::OnPCParamChanged)

	ON_CBN_EDITCHANGE(IDC_COMBO4,	&CFindReplaceTab::OnVolumeChanged)
	ON_CBN_EDITCHANGE(IDC_COMBO6,	&CFindReplaceTab::OnParamChanged)

	ON_COMMAND(IDC_CHECK1,			&CFindReplaceTab::OnCheckNote)
	ON_COMMAND(IDC_CHECK2,			&CFindReplaceTab::OnCheckInstr)
	ON_COMMAND(IDC_CHECK3,			&CFindReplaceTab::OnCheckVolCmd)
	ON_COMMAND(IDC_CHECK4,			&CFindReplaceTab::OnCheckVolume)
	ON_COMMAND(IDC_CHECK5,			&CFindReplaceTab::OnCheckEffect)
	ON_COMMAND(IDC_CHECK6,			&CFindReplaceTab::OnCheckParam)

	ON_COMMAND(IDC_CHECK7,			&CFindReplaceTab::OnCheckChannelSearch)
END_MESSAGE_MAP()


void CFindReplaceTab::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_cbnNote);
	DDX_Control(pDX, IDC_COMBO2, m_cbnInstr);
	DDX_Control(pDX, IDC_COMBO3, m_cbnVolCmd);
	DDX_Control(pDX, IDC_COMBO4, m_cbnVolume);
	DDX_Control(pDX, IDC_COMBO5, m_cbnCommand);
	DDX_Control(pDX, IDC_COMBO6, m_cbnParam);
	DDX_Control(pDX, IDC_COMBO7, m_cbnPCParam);
}


BOOL CFindReplaceTab::OnInitDialog()
{
	CString s;

	CPropertyPage::OnInitDialog();
	// Search flags
	FlagSet<FindReplace::Flags> flags = m_isReplaceTab ? m_settings.replaceFlags : m_settings.findFlags;

	COMBOBOXINFO info;
	info.cbSize = sizeof(info);
	if(m_cbnVolume.GetComboBoxInfo(&info))
	{
		::SetWindowLong(info.hwndItem, GWL_STYLE, ::GetWindowLong(info.hwndItem, GWL_STYLE) | ES_NUMBER);
		::SendMessage(info.hwndItem, EM_SETLIMITTEXT, 4, 0);
	}
	if(m_cbnParam.GetComboBoxInfo(&info))
	{
		// Might need to enter hex values
		//::SetWindowLong(info.hwndItem, GWL_STYLE, ::GetWindowLong(info.hwndItem, GWL_STYLE) | ES_NUMBER);
		::SendMessage(info.hwndItem, EM_SETLIMITTEXT, 4, 0);
	}

	CheckDlgButton(IDC_CHECK1, flags[FindReplace::Note]                           ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK2, flags[FindReplace::Instr]                          ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK3, flags[FindReplace::VolCmd | FindReplace::PCParam]  ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK4, flags[FindReplace::Volume | FindReplace::PCValue]  ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK5, flags[FindReplace::Command]                        ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK6, flags[FindReplace::Param]                          ? BST_CHECKED : BST_UNCHECKED);
	if(m_isReplaceTab)
	{
		CheckDlgButton(IDC_CHECK7, flags[FindReplace::Replace]    ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_CHECK8, flags[FindReplace::ReplaceAll] ? BST_CHECKED : BST_UNCHECKED);
	} else
	{
		CheckDlgButton(IDC_CHECK7, flags[FindReplace::InChannels] ? BST_CHECKED : BST_UNCHECKED);
		int nButton = IDC_RADIO1;
		if(flags[FindReplace::FullSearch])
			nButton = IDC_RADIO2;
		else if(flags[FindReplace::InPatSelection])
			nButton = IDC_RADIO3;
		
		CheckRadioButton(IDC_RADIO1, IDC_RADIO3, nButton);
		GetDlgItem(IDC_RADIO3)->EnableWindow(flags[FindReplace::InPatSelection] ? TRUE : FALSE);
		SetDlgItemInt(IDC_EDIT1, m_settings.findChnMin + 1);
		SetDlgItemInt(IDC_EDIT2, m_settings.findChnMax + 1);
		static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN1))->SetRange32(1, m_sndFile.GetNumChannels());
		static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN2))->SetRange32(1, m_sndFile.GetNumChannels());

		// Pre-fill with selected pattern data
		if(!flags[FindReplace::Note] && m_initialValues.note != NOTE_NONE)
		{
			m_settings.findNoteMin = m_settings.findNoteMax = m_initialValues.note;
		}
		if(!flags[FindReplace::Instr] && m_initialValues.instr != 0)
		{
			m_settings.findInstrMin = m_settings.findInstrMax = m_initialValues.instr;
		}
		if(IsPCEvent())
		{
			if(!flags[FindReplace::PCParam] && m_initialValues.GetValueVolCol() != 0)
				m_settings.findParamMin = m_settings.findParamMax = m_initialValues.GetValueVolCol();
			if(!flags[FindReplace::PCValue] && m_initialValues.GetValueEffectCol() != 0)
				m_settings.findVolumeMin = m_settings.findVolumeMax = m_initialValues.GetValueEffectCol();
		} else
		{
			if(!flags[FindReplace::VolCmd] && m_initialValues.volcmd != VOLCMD_NONE)
				m_settings.findVolCmd = m_initialValues.volcmd;
			if(!flags[FindReplace::Volume] && m_initialValues.volcmd != VOLCMD_NONE)
				m_settings.findVolumeMin = m_settings.findVolumeMax = m_initialValues.vol;
			if(!flags[FindReplace::Command] && m_initialValues.command != CMD_NONE)
				m_settings.findCommand = m_initialValues.command;
			if(!flags[FindReplace::Param] && m_initialValues.command != CMD_NONE)
				m_settings.findParamMin = m_settings.findParamMax = m_initialValues.param;
		}
	}

	// Note
	{
		int sel = -1;
		m_cbnNote.SetRedraw(FALSE);
		m_cbnNote.InitStorage(150, 6);
		m_cbnNote.SetItemData(m_cbnNote.AddString(_T("...")), NOTE_NONE);
		if (m_isReplaceTab)
		{
			m_cbnNote.SetItemData(m_cbnNote.AddString(_T("note -1")), kReplaceNoteMinusOne);
			m_cbnNote.SetItemData(m_cbnNote.AddString(_T("note +1")), kReplaceNotePlusOne);
			m_cbnNote.SetItemData(m_cbnNote.AddString(_T("-1 oct")), kReplaceNoteMinusOctave);
			m_cbnNote.SetItemData(m_cbnNote.AddString(_T("+1 oct")), kReplaceNotePlusOctave);
			m_cbnNote.SetItemData(m_cbnNote.AddString(_T("Transpose...")), kReplaceRelative);
			if(m_settings.replaceNoteAction == FindReplace::ReplaceRelative)
			{
				switch(m_settings.replaceNote)
				{
				case -1: sel = 1; break;
				case  1: sel = 2; break;
				case FindReplace::ReplaceOctaveDown: sel = 3; break;
				case FindReplace::ReplaceOctaveUp  : sel = 4; break;
				default: sel = 5; break;
				}
			}
		} else
		{
			m_cbnNote.SetItemData(m_cbnNote.AddString(_T("any")), kFindAny);
			m_cbnNote.SetItemData(m_cbnNote.AddString(_T("Range...")), kFindRange);
			if(m_settings.findNoteMin == NOTE_MIN && m_settings.findNoteMax == NOTE_MAX)
				sel = 1;
			else if(m_settings.findNoteMin < m_settings.findNoteMax)
				sel = 2;
		}
		AppendNotesToControlEx(m_cbnNote, m_sndFile);

		if(sel == -1)
		{
			DWORD_PTR searchNote = m_isReplaceTab ? m_settings.replaceNote : m_settings.findNoteMin;
			int ncount = m_cbnNote.GetCount();
			for(int i = 0; i < ncount; i++) if(searchNote == m_cbnNote.GetItemData(i))
			{
				sel = i;
				break;
			}
		}
		m_cbnNote.SetCurSel(sel);
		m_cbnNote.SetRedraw(TRUE);
	}

	// Volume Command
	m_cbnVolCmd.SetRedraw(FALSE);
	m_cbnVolCmd.InitStorage(m_effectInfo.GetNumVolCmds(), 15);
	m_cbnVolCmd.SetItemData(m_cbnVolCmd.AddString(_T(" None")), (DWORD_PTR)-1);
	UINT count = m_effectInfo.GetNumVolCmds();
	for (UINT n=0; n<count; n++)
	{
		if(m_effectInfo.GetVolCmdInfo(n, &s) && !s.IsEmpty())
		{
			m_cbnVolCmd.SetItemData(m_cbnVolCmd.AddString(s), n);
		}
	}
	m_cbnVolCmd.SetCurSel(0);
	UINT fxndx = m_effectInfo.GetIndexFromVolCmd(m_isReplaceTab ? m_settings.replaceVolCmd : m_settings.findVolCmd);
	for (UINT i=0; i<=count; i++) if (fxndx == m_cbnVolCmd.GetItemData(i))
	{
		m_cbnVolCmd.SetCurSel(i);
		break;
	}
	m_cbnVolCmd.SetRedraw(TRUE);
	m_cbnVolCmd.ShowWindow(SW_SHOW);
	m_cbnPCParam.ShowWindow(SW_HIDE);

	// Command
	{
		m_cbnCommand.SetRedraw(FALSE);
		m_cbnCommand.InitStorage(m_effectInfo.GetNumEffects(), 20);
		m_cbnCommand.SetItemData(m_cbnCommand.AddString(_T(" None")), (DWORD_PTR)-1);
		count = m_effectInfo.GetNumEffects();
		for (UINT n=0; n<count; n++)
		{
			if(m_effectInfo.GetEffectInfo(n, &s, true) && !s.IsEmpty())
			{
				m_cbnCommand.SetItemData(m_cbnCommand.AddString(s), n);
			}
		}
		m_cbnCommand.SetCurSel(0);
		fxndx = m_effectInfo.GetIndexFromEffect(m_isReplaceTab ? m_settings.replaceCommand : m_settings.findCommand, static_cast<ModCommand::PARAM>(m_isReplaceTab ? m_settings.replaceParam : m_settings.findParamMin));
		for (UINT i=0; i<=count; i++) if (fxndx == m_cbnCommand.GetItemData(i))
		{
			m_cbnCommand.SetCurSel(i);
			break;
		}
		m_cbnCommand.SetRedraw(TRUE);
	}
	UpdateInstrumentList();
	UpdateVolumeList();
	UpdateParamList();
	OnCheckChannelSearch();
	return TRUE;
}


bool CFindReplaceTab::IsPCEvent() const
{
	if(m_isReplaceTab)
	{
		if(m_settings.replaceFlags[FindReplace::Note] && m_settings.replaceNoteAction == FindReplace::ReplaceValue)
			return ModCommand::IsPcNote(static_cast<ModCommand::NOTE>(m_settings.replaceNote));
	}
	return ModCommand::IsPcNote(m_settings.findNoteMin);
}


void CFindReplaceTab::UpdateInstrumentList()
{
	const bool isPCEvent = IsPCEvent();
	if(m_cbnInstr.GetCount() != 0 && !!GetWindowLongPtr(m_cbnInstr.m_hWnd, GWLP_USERDATA) == isPCEvent)
		return;
	SetWindowLongPtr(m_cbnInstr.m_hWnd, GWLP_USERDATA, isPCEvent);

	int oldSelection = (m_isReplaceTab ? m_settings.replaceInstr : m_settings.findInstrMin);
	int sel = (oldSelection == 0) ? 0 : -1;
	m_cbnInstr.SetRedraw(FALSE);
	m_cbnInstr.ResetContent();
	m_cbnInstr.InitStorage((isPCEvent ? MAX_MIXPLUGINS : MAX_INSTRUMENTS) + 3, 32);
	m_cbnInstr.SetItemData(m_cbnInstr.AddString(_T("..")), 0);
	if (m_isReplaceTab)
	{
		m_cbnInstr.SetItemData(m_cbnInstr.AddString(isPCEvent ? _T("Plugin -1") : _T("Instrument -1")), kReplaceInstrumentMinusOne);
		m_cbnInstr.SetItemData(m_cbnInstr.AddString(isPCEvent ? _T("Plugin +1") : _T("Instrument +1")), kReplaceInstrumentPlusOne);
		m_cbnInstr.SetItemData(m_cbnInstr.AddString(_T("Other...")), kReplaceRelative);
		if(m_settings.replaceInstrAction == FindReplace::ReplaceRelative)
		{
			switch(m_settings.replaceInstr)
			{
			case -1: sel = 1; break;
			case  1: sel = 2; break;
			default: sel = 3; break;
			}
		}
	} else
	{
		m_cbnInstr.SetItemData(m_cbnInstr.AddString(_T("Range...")), kFindRange);
		if(m_settings.findInstrMin < m_settings.findInstrMax)
			sel = 1;
	}
	// Did we switch from an instrument index > MAX_MIXPLUGINS to plugin list?
	const bool updateInstr = (sel == -1 && isPCEvent && oldSelection > MAX_MIXPLUGINS);
	if(sel == -1)
		sel = m_cbnInstr.GetCount() + oldSelection - 1;

	if(isPCEvent)
	{
		m_cbnInstr.Update(PluginComboBox::Config{PluginComboBox::ShowEmptySlots | PluginComboBox::DoNotResetContent}, m_sndFile);
	} else
	{
		CString s;
		for(INSTRUMENTINDEX n = 1; n < MAX_INSTRUMENTS; n++)
		{
			s.Format(_T("%03d:"), n);
			if(m_sndFile.GetNumInstruments())
				s += mpt::ToCString(m_sndFile.GetCharsetInternal(), m_sndFile.GetInstrumentName(n));
			else
				s += mpt::ToCString(m_sndFile.GetCharsetInternal(), m_sndFile.m_szNames[n]);
			m_cbnInstr.SetItemData(m_cbnInstr.AddString(s), n);
		}
	}
	m_cbnInstr.SetRawSelection(sel);
	m_cbnInstr.SetRedraw(TRUE);
	m_cbnInstr.Invalidate(FALSE);
	if(updateInstr)
		OnInstrChanged();
}


void CFindReplaceTab::UpdateParamList()
{
	const bool isPCEvent = IsPCEvent();
	if(m_cbnInstr.GetCount() == 0 || !!GetWindowLongPtr(m_cbnInstr.m_hWnd, GWLP_USERDATA) != isPCEvent)
	{
		SetWindowLongPtr(m_cbnInstr.m_hWnd, GWLP_USERDATA, isPCEvent);
	}

	int effectIndex = static_cast<int>(m_cbnCommand.GetItemData(m_cbnCommand.GetCurSel()));
	ModCommand::PARAM n = 0; // unused parameter adjustment
	ModCommand::COMMAND cmd = m_effectInfo.GetEffectFromIndex(effectIndex, n);
	const UINT mask = m_effectInfo.GetEffectMaskFromIndex(effectIndex);
	if(m_isReplaceTab)
		m_settings.replaceCommand = cmd;
	else
		m_settings.findCommand = cmd;

	// Update Param range
	const bool isExtended = m_effectInfo.IsExtendedEffect(effectIndex);
	int sel = -1;
	int oldcount = m_cbnParam.GetCount();
	int newcount = isExtended ? 16 : 256;
	if(oldcount)
		oldcount -= m_isReplaceTab ? 2 : 1;

	auto findParam = m_isReplaceTab ? m_settings.replaceParam : m_settings.findParamMin;
	if(isExtended)
	{
		findParam &= 0x0F;
		if(!m_isReplaceTab && !IsDlgButtonChecked(IDC_CHECK6))
		{
			m_settings.findParamMin = (m_settings.findParamMin & 0x0F) | mask;
			m_settings.findParamMax = (m_settings.findParamMax & 0x0F) | mask;
		} else if(m_isReplaceTab)
		{
			m_settings.replaceParam |= mask;
		}
	}

	if(oldcount != newcount)
	{
		TCHAR s[16];
		int newpos;
		if(oldcount && m_cbnParam.GetCurSel() != CB_ERR)
			newpos = static_cast<int>(m_cbnParam.GetItemData(m_cbnParam.GetCurSel()));
		else
			newpos = findParam;
		Limit(newpos, 0, newcount - 1);
		m_cbnParam.SetRedraw(FALSE);
		m_cbnParam.ResetContent();
		m_cbnParam.InitStorage(newcount + 2, 4);

		if(m_isReplaceTab)
		{
			wsprintf(s, _T("+ %d"), m_settings.replaceParam);
			m_cbnParam.SetItemData(m_cbnParam.AddString(s), kReplaceRelative);
			wsprintf(s, _T("* %d%%"), m_settings.replaceParam);
			m_cbnParam.SetItemData(m_cbnParam.AddString(s), kReplaceMultiply);
			if(m_settings.replaceParamAction == FindReplace::ReplaceRelative)
				sel = 0;
			else if(m_settings.replaceParamAction == FindReplace::ReplaceMultiply)
				sel = 1;

			m_settings.replaceParam = newpos;
			if(isExtended)
			{
				m_settings.replaceParam = (m_settings.replaceParam & 0x0F) | mask;
			}
		} else
		{
			m_cbnParam.SetItemData(m_cbnParam.AddString(_T("Range")), kFindRange);
			if(m_settings.findParamMin < m_settings.findParamMax)
				sel = 0;
		}

		if(sel == -1)
			sel = m_cbnParam.GetCount() + newpos;
		for(int param = 0; param < newcount; param++)
		{
			wsprintf(s, (newcount == 256) ? _T("%02X") : _T("%X"), param);
			int i = m_cbnParam.AddString(s);
			m_cbnParam.SetItemData(i, param);
		}
		m_cbnParam.SetCurSel(sel);
		m_cbnParam.SetRedraw(TRUE);
		m_cbnParam.Invalidate(FALSE);
	}
}


void CFindReplaceTab::UpdateVolumeList()
{
	TCHAR s[256];
	const bool isPCEvent = IsPCEvent();

	BOOL enable = isPCEvent ? FALSE : TRUE;
	GetDlgItem(IDC_CHECK5)->EnableWindow(enable);
	GetDlgItem(IDC_CHECK6)->EnableWindow(enable);
	m_cbnCommand.EnableWindow(enable);
	m_cbnParam.EnableWindow(enable);

	// Update plugin parameter list
	PLUGINDEX plug = PLUGINDEX_INVALID;
	int plugData = 0;
	if(int sel = m_cbnInstr.GetRawSelection(); sel >= 0 && m_cbnInstr.GetItemData(sel) < kBeginSpecial)
	{
		plug = m_cbnInstr.GetSelection().value_or(PLUGINDEX_INVALID);
		plugData = (plug != PLUGINDEX_INVALID) ? plug + 1 : 0;
	}
	if(isPCEvent && (m_cbnPCParam.GetCount() == 0 || GetWindowLongPtr(m_cbnPCParam.m_hWnd, GWLP_USERDATA) != plugData))
	{
		SetWindowLongPtr(m_cbnPCParam.m_hWnd, GWLP_USERDATA, plugData);

		CheckDlgButton(IDC_CHECK5, BST_UNCHECKED);
		CheckDlgButton(IDC_CHECK6, BST_UNCHECKED);

		int sel = m_isReplaceTab ? m_settings.replaceParam : m_settings.findParamMin;
		m_cbnPCParam.SetRedraw(FALSE);
		m_cbnPCParam.ResetContent();
		if(plug < MAX_MIXPLUGINS && m_sndFile.m_MixPlugins[plug].pMixPlugin != nullptr)
		{
			AddPluginParameternamesToCombobox(m_cbnPCParam, *m_sndFile.m_MixPlugins[plug].pMixPlugin);
		} else
		{
			m_cbnPCParam.InitStorage(ModCommand::maxColumnValue + 1, 20);
			for(int i = 0; i <= ModCommand::maxColumnValue; i++)
			{
				wsprintf(s, _T("%02u: Parameter %02u"), static_cast<unsigned int>(i), static_cast<unsigned int>(i));
				m_cbnPCParam.SetItemData(m_cbnPCParam.AddString(s), i);
			}
		}
		m_cbnPCParam.SetCurSel(sel);
		m_cbnPCParam.SetRedraw(TRUE);
		m_cbnPCParam.Invalidate(FALSE);
	}

	m_cbnVolCmd.ShowWindow(isPCEvent ? SW_HIDE : SW_SHOW);
	m_cbnPCParam.ShowWindow(isPCEvent ? SW_SHOW : SW_HIDE);

	int rangeMin, rangeMax, curVal;

	if(isPCEvent)
	{
		rangeMin = 0;
		rangeMax = ModCommand::maxColumnValue;
		curVal = (m_isReplaceTab ? m_settings.replaceVolume : m_settings.findVolumeMin);
	} else
	{
		int effectIndex = static_cast<int>(m_cbnVolCmd.GetItemData(m_cbnVolCmd.GetCurSel()));
		ModCommand::VOLCMD cmd = m_effectInfo.GetVolCmdFromIndex(effectIndex);
		if(m_isReplaceTab)
			m_settings.replaceVolCmd = cmd;
		else
			m_settings.findVolCmd = cmd;

		// Update Param range
		ModCommand::VOL volMin, volMax;
		if(!m_effectInfo.GetVolCmdInfo(effectIndex, nullptr, &volMin, &volMax))
		{
			volMin = 0;
			volMax = 64;
		}
		rangeMin = volMin;
		rangeMax = volMax;
		curVal = (m_isReplaceTab ? m_settings.replaceVolume : m_settings.findVolumeMin);
	}

	int oldcount = m_cbnVolume.GetCount();
	int newcount = rangeMax - rangeMin + 1;
	if (oldcount != newcount)
	{
		int sel = -1;
		int newpos;
		if (oldcount)
			newpos = static_cast<int>(m_cbnVolume.GetItemData(m_cbnVolume.GetCurSel()));
		else
			newpos = curVal;
		Limit(newpos, 0, newcount - 1);
		m_cbnVolume.SetRedraw(FALSE);
		m_cbnVolume.ResetContent();
		m_cbnVolume.InitStorage(newcount + 2, 4);
		if(m_isReplaceTab)
		{
			wsprintf(s, _T("+ %d"), m_settings.replaceVolume);
			m_cbnVolume.SetItemData(m_cbnVolume.AddString(s), kReplaceRelative);
			wsprintf(s, _T("* %d%%"), m_settings.replaceVolume);
			m_cbnVolume.SetItemData(m_cbnVolume.AddString(s), kReplaceMultiply);
			if(m_settings.replaceVolumeAction == FindReplace::ReplaceRelative)
				sel = 0;
			else if(m_settings.replaceVolumeAction == FindReplace::ReplaceMultiply)
				sel = 1;
		} else
		{
			m_cbnVolume.SetItemData(m_cbnVolume.AddString(_T("Range...")), kFindRange);
			if(m_settings.findVolumeMin < m_settings.findVolumeMax)
				sel = 0;
		}

		if(sel == -1)
			sel = m_cbnVolume.GetCount() + newpos - rangeMin;
		for (int vol = rangeMin; vol <= rangeMax; vol++)
		{
			wsprintf(s, (rangeMax < 10 || rangeMax > 99) ? _T("%d") : _T("%02d"), vol);
			int i = m_cbnVolume.AddString(s);
			m_cbnVolume.SetItemData(i, vol);
		}
		m_cbnVolume.SetCurSel(sel);
		m_cbnVolume.SetRedraw(TRUE);
		m_cbnVolume.Invalidate(FALSE);
	}
}


void CFindReplaceTab::OnNoteChanged()
{
	CheckOnChange(IDC_CHECK1);
	int item = static_cast<int>(m_cbnNote.GetItemData(m_cbnNote.GetCurSel()));
	if(m_isReplaceTab)
	{
		m_settings.replaceNoteAction = FindReplace::ReplaceRelative;
		switch(item)
		{
		case kReplaceNoteMinusOne:    m_settings.replaceNote = -1; break;
		case kReplaceNotePlusOne:     m_settings.replaceNote =  1; break;
		case kReplaceNoteMinusOctave: m_settings.replaceNote = FindReplace::ReplaceOctaveDown; break;
		case kReplaceNotePlusOctave:  m_settings.replaceNote = FindReplace::ReplaceOctaveUp; break;

		case kReplaceRelative:
			{
				CInputDlg dlg(this, _T("Custom Transpose Amount:"), -120, 120, m_settings.replaceNote);
				if(dlg.DoModal() == IDOK)
				{
					m_settings.replaceNote = dlg.resultAsInt;
				} else
				{
					// TODO undo selection
				}
			}
			break;

		default:
			m_settings.replaceNote = item;
			m_settings.replaceNoteAction = FindReplace::ReplaceValue;
		}
	} else
	{
		if(item == kFindRange)
		{
			CFindRangeDlg dlg(this, NOTE_MIN, m_settings.findNoteMin, NOTE_MAX, m_settings.findNoteMax, CFindRangeDlg::kNotes);
			if(dlg.DoModal() == IDOK)
			{
				m_settings.findNoteMin = static_cast<ModCommand::NOTE>(dlg.GetMinVal());
				m_settings.findNoteMax = static_cast<ModCommand::NOTE>(dlg.GetMaxVal());
			}
		} else if(item == kFindAny)
		{
			m_settings.findNoteMin = NOTE_MIN;
			m_settings.findNoteMax = NOTE_MAX;
		} else
		{
			m_settings.findNoteMin = m_settings.findNoteMax = static_cast<ModCommand::NOTE>(item);
		}
	}
	UpdateInstrumentList();
	UpdateVolumeList();
}


void CFindReplaceTab::OnInstrChanged()
{
	CheckOnChange(IDC_CHECK2);
	int item = static_cast<int>(m_cbnInstr.GetItemData(m_cbnInstr.GetRawSelection()));
	if(item < kBeginSpecial && IsPCEvent())
	{
		if(auto sel = m_cbnInstr.GetSelection(); sel)
			item = *sel + 1;
		else
			item = 0;
	}

	if(m_isReplaceTab)
	{
		m_settings.replaceInstrAction = FindReplace::ReplaceRelative;
		switch(item)
		{
		case kReplaceInstrumentMinusOne: m_settings.replaceInstr = -1; break;
		case kReplaceInstrumentPlusOne:  m_settings.replaceInstr =  1; break;

		case kReplaceRelative:
			{
				CInputDlg dlg(this, _T("Custom Replacement Amount:"), -255, 255, m_settings.replaceInstr);
				if(dlg.DoModal() == IDOK)
				{
					m_settings.replaceInstrAction = FindReplace::ReplaceRelative;
					m_settings.replaceInstr = dlg.resultAsInt;
				} else
				{
					// TODO undo selection
				}
			}
			break;

		default:
			m_settings.replaceInstrAction = FindReplace::ReplaceValue;
			m_settings.replaceInstr = item;
			break;
		}
	} else
	{
		if(item == kFindRange)
		{
			CFindRangeDlg dlg(this, 1, m_settings.findInstrMin, MAX_INSTRUMENTS - 1, m_settings.findInstrMax, CFindRangeDlg::kDecimal);
			if(dlg.DoModal() == IDOK)
			{
				m_settings.findInstrMin = static_cast<ModCommand::INSTR>(dlg.GetMinVal());
				m_settings.findInstrMax = static_cast<ModCommand::INSTR>(dlg.GetMaxVal());
			}
		} else
		{
			m_settings.findInstrMin = m_settings.findInstrMax = static_cast<ModCommand::INSTR>(item);
		}
	}
	if(IsPCEvent())
		UpdateVolumeList();
}


void CFindReplaceTab::OnVolCmdChanged()
{
	CheckOnChange(IDC_CHECK3);
	UpdateVolumeList();
};


void CFindReplaceTab::RelativeOrMultiplyPrompt(CComboBox &comboBox, FindReplace::ReplaceMode &action, int &value, int range, bool isHex)
{
	int sel = comboBox.GetCurSel();
	int item = static_cast<int>(comboBox.GetItemData(sel));

	if(sel == CB_ERR)
	{
		item = 0;
		CString s;
		comboBox.GetWindowText(s);
		s.TrimLeft();
		if(s.GetLength() >= 1)
		{
			TCHAR first = s[0];
			if(first == _T('+'))
			{
				item = kReplaceRelative;
				sel = 0;
			} else if(first == _T('*'))
			{
				item = kReplaceMultiply;
				sel = 1;
			}
		}
		if(!item)
		{
			if(isHex)
			{
				int len = ::GetWindowTextLengthA(m_cbnParam);
				std::string sHex(len, 0);
				::GetWindowTextA(m_cbnParam, &sHex[0], len + 1);
				item = mpt::parse_hex<unsigned int>(sHex);
			} else
			{
				item = mpt::parse<int>(s);
			}
		}
	}

	if(item == kReplaceRelative || item == kReplaceMultiply)
	{
		const TCHAR *prompt, *format;
		FindReplace::ReplaceMode act;
		if(item == kReplaceRelative)
		{
			act = FindReplace::ReplaceRelative;
			prompt = _T("Amount to add or subtract:");
			format = _T("+ %d");
		} else
		{
			act = FindReplace::ReplaceMultiply;
			prompt = _T("Multiply by percentage:");
			format = _T("* %d%%");
		}

		range *= 100;
		CInputDlg dlg(this, prompt, -range, range, value);
		if(dlg.DoModal() == IDOK)
		{
			value = dlg.resultAsInt;
			action = act;

			TCHAR s[32];
			wsprintf(s, format, value);
			comboBox.DeleteString(sel);
			comboBox.InsertString(sel, s);
			comboBox.SetItemData(sel, item);
			comboBox.SetCurSel(sel);
		} else
		{
			// TODO undo selection
		}
	} else
	{
		action = FindReplace::ReplaceValue;
		value = item;
	}
}


void CFindReplaceTab::OnVolumeChanged()
{
	CheckOnChange(IDC_CHECK4);
	int item = m_cbnVolume.GetCurSel();
	if(item != CB_ERR)
		item = static_cast<int>(m_cbnVolume.GetItemData(item));
	else
		item = GetDlgItemInt(IDC_COMBO4);

	int rangeMax = IsPCEvent() ? ModCommand::maxColumnValue : 64;
	if(m_isReplaceTab)
	{
		RelativeOrMultiplyPrompt(m_cbnVolume, m_settings.replaceVolumeAction, m_settings.replaceVolume, rangeMax, false);
	} else
	{
		if(item == kFindRange)
		{
			CFindRangeDlg dlg(this, 0, m_settings.findVolumeMin, rangeMax, m_settings.findVolumeMax, CFindRangeDlg::kDecimal);
			if(dlg.DoModal() == IDOK)
			{
				m_settings.findVolumeMin = dlg.GetMinVal();
				m_settings.findVolumeMax = dlg.GetMaxVal();
			} else
			{
				// TODO undo selection
			}
		} else
		{
			m_settings.findVolumeMin = m_settings.findVolumeMax = item;
		}
	}
}


void CFindReplaceTab::OnEffectChanged()
{
	CheckOnChange(IDC_CHECK5);
	UpdateParamList();
};


void CFindReplaceTab::OnParamChanged()
{
	CheckOnChange(IDC_CHECK6);
	int item = m_cbnParam.GetCurSel();
	if(item != CB_ERR)
	{
		item = static_cast<int>(m_cbnParam.GetItemData(item));
	} else
	{
		int len = ::GetWindowTextLengthA(m_cbnParam);
		std::string s(len, 0);
		::GetWindowTextA(m_cbnParam, &s[0], len + 1);
		item = mpt::parse_hex<unsigned int>(s);
	}

	// Apply parameter value mask if required (e.g. SDx has mask D0).
	int effectIndex = static_cast<int>(m_cbnCommand.GetItemData(m_cbnCommand.GetCurSel()));
	UINT mask = (effectIndex > -1) ? m_effectInfo.GetEffectMaskFromIndex(effectIndex) : 0;

	if(m_isReplaceTab)
	{
		RelativeOrMultiplyPrompt(m_cbnParam, m_settings.replaceParamAction, m_settings.replaceParam, 256, true);
		if(m_settings.replaceParamAction == FindReplace::ReplaceValue && effectIndex > -1)
		{
			m_settings.replaceParam |= mask;
		}
	} else
	{
		if(item == kFindRange)
		{
			CFindRangeDlg dlg(this, 0, m_settings.findParamMin & ~mask, m_cbnParam.GetCount() - 2, m_settings.findParamMax & ~mask, CFindRangeDlg::kHex);
			if(dlg.DoModal() == IDOK)
			{
				m_settings.findParamMin = dlg.GetMinVal() | mask;
				m_settings.findParamMax = dlg.GetMaxVal() | mask;
			} else
			{
				// TODO undo selection
			}
		} else
		{
			m_settings.findParamMin = m_settings.findParamMax = (item | mask);
		}
	}
}


void CFindReplaceTab::OnPCParamChanged()
{
	CheckOnChange(IDC_CHECK3);
	int item = static_cast<int>(m_cbnPCParam.GetItemData(m_cbnPCParam.GetCurSel()));

	if(m_isReplaceTab)
	{
		RelativeOrMultiplyPrompt(m_cbnPCParam, m_settings.replaceParamAction, m_settings.replaceParam, 256, false);
	} else
	{
		if(item == kFindRange)
		{
			CFindRangeDlg dlg(this, 0, m_settings.findParamMin, ModCommand::maxColumnValue, m_settings.findParamMax, CFindRangeDlg::kDecimal);
			if(dlg.DoModal() == IDOK)
			{
				m_settings.findParamMin = dlg.GetMinVal();
				m_settings.findParamMax = dlg.GetMaxVal();
			} else
			{
				// TODO undo selection
			}
		} else
		{
			m_settings.findParamMin = m_settings.findParamMax = item;
		}
	}
}


void CFindReplaceTab::OnCheckNote()
{
	CheckReplace(IDC_CHECK1);
	if(m_isReplaceTab && IsPCEvent() != !!GetWindowLongPtr(m_cbnInstr.m_hWnd, GWLP_USERDATA))
	{
		UpdateInstrumentList();
		UpdateVolumeList();
	}
}

void CFindReplaceTab::OnCheckInstr() { CheckReplace(IDC_CHECK2); }
void CFindReplaceTab::OnCheckVolCmd() { CheckReplace(IDC_CHECK3); }
void CFindReplaceTab::OnCheckVolume() { CheckReplace(IDC_CHECK4); }
void CFindReplaceTab::OnCheckEffect() { CheckReplace(IDC_CHECK5); }
void CFindReplaceTab::OnCheckParam() { CheckReplace(IDC_CHECK6); }


void CFindReplaceTab::CheckReplace(int buttonID)
{
	if(m_isReplaceTab && IsDlgButtonChecked(buttonID))
		CheckDlgButton(IDC_CHECK7, BST_CHECKED);

	if(buttonID == IDC_CHECK1)
	{
		FlagSet<FindReplace::Flags> &flags = m_isReplaceTab ? m_settings.replaceFlags : m_settings.findFlags;
		flags.set(FindReplace::Note, !!IsDlgButtonChecked(IDC_CHECK1));
	}
};


void CFindReplaceTab::OnCheckChannelSearch()
{
	if (!m_isReplaceTab)
	{
		BOOL b = IsDlgButtonChecked(IDC_CHECK7);
		GetDlgItem(IDC_EDIT1)->EnableWindow(b);
		GetDlgItem(IDC_SPIN1)->EnableWindow(b);
		GetDlgItem(IDC_EDIT2)->EnableWindow(b);
		GetDlgItem(IDC_SPIN2)->EnableWindow(b);
	}
}


void CFindReplaceTab::OnOK()
{
	// Search flags
	FlagSet<FindReplace::Flags> &flags = m_isReplaceTab ? m_settings.replaceFlags : m_settings.findFlags;
	flags.reset();
	flags.set(FindReplace::Note, !!IsDlgButtonChecked(IDC_CHECK1));
	flags.set(FindReplace::Instr, !!IsDlgButtonChecked(IDC_CHECK2));
	if(IsPCEvent())
	{
		flags.set(FindReplace::PCParam, !!IsDlgButtonChecked(IDC_CHECK3));
		flags.set(FindReplace::PCValue, !!IsDlgButtonChecked(IDC_CHECK4));
	} else
	{
		flags.set(FindReplace::VolCmd, !!IsDlgButtonChecked(IDC_CHECK3));
		flags.set(FindReplace::Volume, !!IsDlgButtonChecked(IDC_CHECK4));
		flags.set(FindReplace::Command, !!IsDlgButtonChecked(IDC_CHECK5));
		flags.set(FindReplace::Param, !!IsDlgButtonChecked(IDC_CHECK6));
	}
	if(m_isReplaceTab)
	{
		flags.set(FindReplace::Replace, !!IsDlgButtonChecked(IDC_CHECK7));
		flags.set(FindReplace::ReplaceAll, !!IsDlgButtonChecked(IDC_CHECK8));
	} else
	{
		flags.set(FindReplace::InChannels, !!IsDlgButtonChecked(IDC_CHECK7));
		flags.set(FindReplace::FullSearch, !!IsDlgButtonChecked(IDC_RADIO2));
		flags.set(FindReplace::InPatSelection, !!IsDlgButtonChecked(IDC_RADIO3));
	}

	// Min/Max channels
	if (!m_isReplaceTab)
	{
		m_settings.findChnMin = static_cast<CHANNELINDEX>(GetDlgItemInt(IDC_EDIT1) - 1);
		m_settings.findChnMax = static_cast<CHANNELINDEX>(GetDlgItemInt(IDC_EDIT2) - 1);
		if (m_settings.findChnMax < m_settings.findChnMin)
		{
			std::swap(m_settings.findChnMin, m_settings.findChnMax);
		}
	}
	CPropertyPage::OnOK();
}

OPENMPT_NAMESPACE_END
