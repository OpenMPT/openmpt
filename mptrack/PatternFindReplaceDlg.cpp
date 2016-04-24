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
#include "Mptrack.h"
#include "Mainfrm.h"
#include "View_pat.h"
#include "PatternFindReplace.h"
#include "PatternFindReplaceDlg.h"


OPENMPT_NAMESPACE_BEGIN

// CFindRangeDlg: Find a range of values.

//==================================
class CFindRangeDlg : public CDialog
//==================================
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
	CFindRangeDlg(CWnd *parent, int minVal, int minDefault, int maxVal, int maxDefault, DisplayMode displayMode) : CDialog(IDD_FIND_RANGE, parent)
		, m_minVal(minVal)
		, m_minDefault(minDefault)
		, m_maxVal(maxVal)
		, m_maxDefault(maxDefault)
		, m_displayMode(displayMode)
	{ }

	int GetMinVal() const { return m_minVal; }
	int GetMaxVal() const { return m_maxVal; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_COMBO1, m_cbnMin);
		DDX_Control(pDX, IDC_COMBO2, m_cbnMax);
	}

	virtual BOOL OnInitDialog()
	{
		CDialog::OnInitDialog();

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

	virtual void OnOK()
	{
		CDialog::OnOK();
		m_minVal = m_cbnMin.GetItemData(m_cbnMin.GetCurSel());
		m_maxVal = m_cbnMax.GetItemData(m_cbnMax.GetCurSel());
		if(m_maxVal < m_minVal)
			std::swap(m_minVal, m_maxVal);
	}
};


BEGIN_MESSAGE_MAP(CFindReplaceTab, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnInstrChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,	OnVolCmdChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,	OnVolumeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5,	OnEffectChanged)
	ON_CBN_SELCHANGE(IDC_COMBO6,	OnParamChanged)

	ON_COMMAND(IDC_CHECK1,			OnCheckNote)
	ON_COMMAND(IDC_CHECK2,			OnCheckInstr)
	ON_COMMAND(IDC_CHECK3,			OnCheckVolCmd)
	ON_COMMAND(IDC_CHECK4,			OnCheckVolume)
	ON_COMMAND(IDC_CHECK5,			OnCheckEffect)
	ON_COMMAND(IDC_CHECK6,			OnCheckParam)

	ON_COMMAND(IDC_CHECK7,			OnCheckChannelSearch)
END_MESSAGE_MAP()


void CFindReplaceTab::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_cbnNote);
	DDX_Control(pDX, IDC_COMBO2, m_cbnInstr);
	DDX_Control(pDX, IDC_COMBO3, m_cbnVolCmd);
	DDX_Control(pDX, IDC_COMBO4, m_cbnVolume);
	DDX_Control(pDX, IDC_COMBO5, m_cbnCommand);
	DDX_Control(pDX, IDC_COMBO6, m_cbnParam);
}


BOOL CFindReplaceTab::OnInitDialog()
//----------------------------------
{
	TCHAR s[256];

	CPropertyPage::OnInitDialog();
	// Search flags
	FlagSet<FindReplace::Flags> flags = m_isReplaceTab ? m_settings.replaceFlags : m_settings.findFlags;

	CheckDlgButton(IDC_CHECK1, flags[FindReplace::Note]    ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK2, flags[FindReplace::Instr]   ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK3, flags[FindReplace::VolCmd]  ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK4, flags[FindReplace::Volume]  ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK5, flags[FindReplace::Command] ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK6, flags[FindReplace::Param]   ? BST_CHECKED : BST_UNCHECKED);
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
				case  -1: sel = 1; break;
				case   1: sel = 2; break;
				case -12: sel = 3; break;
				case  12: sel = 4; break;
				default:  sel = 5; break;
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
	// Instrument
	{
		int sel = (m_settings.findInstrMin == 0) ? 0 : -1;
		m_cbnInstr.SetRedraw(FALSE);
		m_cbnInstr.SetItemData(m_cbnInstr.AddString(_T("..")), 0);
		if (m_isReplaceTab)
		{
			m_cbnInstr.SetItemData(m_cbnInstr.AddString(_T("Instrument -1")), kReplaceInstrumentMinusOne);
			m_cbnInstr.SetItemData(m_cbnInstr.AddString(_T("Instrument +1")), kReplaceInstrumentPlusOne);
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
		for(INSTRUMENTINDEX n = 1; n < MAX_INSTRUMENTS; n++)
		{
			if(m_sndFile.GetNumInstruments())
			{
				_stprintf(s, _T("%03d:%s"), n, m_sndFile.GetInstrumentName(n));
			} else
			{
				_stprintf(s, _T("%03d:%s"), n, m_sndFile.m_szNames[n]);
			}
			int i = m_cbnInstr.AddString(s);
			m_cbnInstr.SetItemData(i, n);
			if(m_settings.findInstrMin == n && sel == -1)
				sel = i;
		}
		m_cbnInstr.SetCurSel(sel);
		m_cbnInstr.SetRedraw(TRUE);
	}
	// Volume Command
	{
		m_cbnVolCmd.SetRedraw(FALSE);
		m_cbnVolCmd.InitStorage(m_effectInfo.GetNumVolCmds(), 15);
		m_cbnVolCmd.SetItemData(m_cbnVolCmd.AddString(" None"), (DWORD_PTR)-1);
		UINT count = m_effectInfo.GetNumVolCmds();
		for (UINT n=0; n<count; n++)
		{
			if(m_effectInfo.GetVolCmdInfo(n, s) && s[0])
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
	}
	// Command
	{
		m_cbnCommand.SetRedraw(FALSE);
		m_cbnCommand.InitStorage(m_effectInfo.GetNumEffects(), 20);
		m_cbnCommand.SetItemData(m_cbnCommand.AddString(" None"), (DWORD_PTR)-1);
		UINT count = m_effectInfo.GetNumEffects();
		for (UINT n=0; n<count; n++)
		{
			if(m_effectInfo.GetEffectInfo(n, s, true) && s[0])
			{
				m_cbnCommand.SetItemData(m_cbnCommand.AddString(s), n);
			}
		}
		m_cbnCommand.SetCurSel(0);
		UINT fxndx = m_effectInfo.GetIndexFromEffect(m_isReplaceTab ? m_settings.replaceCommand : m_settings.findCommand, m_isReplaceTab ? static_cast<ModCommand::PARAM>(m_settings.replaceParam) : m_settings.findParamMin);
		for (UINT i=0; i<=count; i++) if (fxndx == m_cbnCommand.GetItemData(i))
		{
			m_cbnCommand.SetCurSel(i);
			break;
		}
		m_cbnCommand.SetRedraw(TRUE);
	}
	ChangeEffect();
	ChangeVolCmd();
	OnCheckChannelSearch();
	return TRUE;
}


void CFindReplaceTab::ChangeEffect()
//----------------------------------
{
	int effectIndex = m_cbnCommand.GetItemData(m_cbnCommand.GetCurSel());
	ModCommand::PARAM n = 0; // unused parameter adjustment
	ModCommand::COMMAND cmd = m_effectInfo.GetEffectFromIndex(effectIndex, n);
	if(m_isReplaceTab)
		m_settings.replaceCommand = cmd;
	else
		m_settings.findCommand = cmd;

	// Update Param range
	int sel = -1;
	int oldcount = m_cbnParam.GetCount();
	int newcount = m_effectInfo.IsExtendedEffect(effectIndex) ? 16 : 256;
	if(oldcount != newcount)
	{
		TCHAR s[16];
		int newpos;
		if(oldcount)
			newpos = m_cbnParam.GetItemData(m_cbnParam.GetCurSel()) % newcount;
		else
			newpos = (m_isReplaceTab ? m_settings.replaceParam : m_settings.findParamMin) % newcount;
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
		} else
		{
			m_cbnParam.SetItemData(m_cbnParam.AddString(_T("Range")), kFindRange);
			if(m_settings.findParamMin < m_settings.findParamMax)
				sel = 0;
		}

		for(int param = 0; param < newcount; param++)
		{
			wsprintf(s, (newcount == 256) ? _T("%02X") : _T("%X"), param);
			int i = m_cbnParam.AddString(s);
			m_cbnParam.SetItemData(i, param);
			if(param == newpos && sel == -1)
				sel = i;
		}
		m_cbnParam.SetCurSel(sel);
		m_cbnParam.SetRedraw(TRUE);
	}
}


void CFindReplaceTab::ChangeVolCmd()
//----------------------------------
{
	int effectIndex = m_cbnVolCmd.GetItemData(m_cbnVolCmd.GetCurSel());
	ModCommand::VOLCMD cmd = m_effectInfo.GetVolCmdFromIndex(effectIndex);
	if(m_isReplaceTab)
		m_settings.replaceVolCmd = cmd;
	else
		m_settings.findVolCmd = cmd;

	// Update Param range
	int sel = -1;
	ModCommand::VOL rangeMin, rangeMax;
	if(!m_effectInfo.GetVolCmdInfo(effectIndex, nullptr, &rangeMin, &rangeMax))
	{
		rangeMin = 0;
		rangeMax = 64;
	}
	int oldcount = m_cbnVolume.GetCount();
	int newcount = rangeMax - rangeMin + 1;
	if (oldcount != newcount)
	{
		TCHAR s[16];
		int newpos;
		if (oldcount)
			newpos = m_cbnVolume.GetItemData(m_cbnVolume.GetCurSel()) % newcount;
		else
			newpos = (m_isReplaceTab ? m_settings.replaceVolume : m_settings.findVolumeMin) % newcount;
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

		for (int vol = rangeMin; vol <= rangeMax; vol++)
		{
			_stprintf(s, (rangeMax < 10) ? _T("%d") : _T("%02d"), vol);
			int i = m_cbnVolume.AddString(s);
			m_cbnVolume.SetItemData(i, vol);
			if(vol == newpos && sel == -1)
				sel = i;
		}
		m_cbnVolume.SetCurSel(sel);
		m_cbnVolume.SetRedraw(TRUE);
	}
}


void CFindReplaceTab::OnNoteChanged()
//-----------------------------------
{
	CheckOnChange(IDC_CHECK1);
	int item = m_cbnNote.GetItemData(m_cbnNote.GetCurSel());
	if(m_isReplaceTab)
	{
		m_settings.replaceNoteAction = FindReplace::ReplaceRelative;
		switch(item)
		{
		case kReplaceNoteMinusOne:    m_settings.replaceNote =  -1; break;
		case kReplaceNotePlusOne:     m_settings.replaceNote =   1; break;
		case kReplaceNoteMinusOctave: m_settings.replaceNote = -12; break;	// TODO custom tunings!
		case kReplaceNotePlusOctave:  m_settings.replaceNote =  12; break;	// TODO custom tunings!

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
}


void CFindReplaceTab::OnInstrChanged()
//------------------------------------
{
	CheckOnChange(IDC_CHECK2);
	int item = m_cbnInstr.GetItemData(m_cbnInstr.GetCurSel());
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
}


void CFindReplaceTab::RelativeOrMultiplyPrompt(CComboBox &comboBox, FindReplace::ReplaceMode &action, int &value, int range)
//--------------------------------------------------------------------------------------------------------------------------
{
	int sel = comboBox.GetCurSel();
	int item = comboBox.GetItemData(sel);
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

		CInputDlg dlg(this, prompt, -range, range, value);
		if(dlg.DoModal() == IDOK)
		{
			value = dlg.resultAsInt;
			action = act;

			TCHAR s[32];
			_stprintf(s, format, value);
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
//-------------------------------------
{
	CheckOnChange(IDC_CHECK4);
	int item = m_cbnVolume.GetItemData(m_cbnVolume.GetCurSel());
	if(m_isReplaceTab)
	{
		RelativeOrMultiplyPrompt(m_cbnVolume, m_settings.replaceVolumeAction, m_settings.replaceVolume, 64);
	} else
	{
		if(item == kFindRange)
		{
			CFindRangeDlg dlg(this, 0, m_settings.findVolumeMin, 64, m_settings.findVolumeMax, CFindRangeDlg::kDecimal);
			if(dlg.DoModal() == IDOK)
			{
				m_settings.findVolumeMin = static_cast<ModCommand::VOL>(dlg.GetMinVal());
				m_settings.findVolumeMax = static_cast<ModCommand::VOL>(dlg.GetMaxVal());
			} else
			{
				// TODO undo selection
			}
		} else
		{
			m_settings.findVolumeMin = m_settings.findVolumeMax = static_cast<ModCommand::VOL>(item);
		}
	}
}


void CFindReplaceTab::OnParamChanged()
//------------------------------------
{
	CheckOnChange(IDC_CHECK6);
	int item = m_cbnParam.GetItemData(m_cbnParam.GetCurSel());

	// Apply parameter value mask if required (e.g. SDx has mask D0).
	int effectIndex = m_cbnCommand.GetItemData(m_cbnCommand.GetCurSel());
	UINT mask = (effectIndex > -1) ? m_effectInfo.GetEffectMaskFromIndex(effectIndex) : 0;

	if(m_isReplaceTab)
	{
		RelativeOrMultiplyPrompt(m_cbnParam, m_settings.replaceParamAction, m_settings.replaceParam, 256);
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
				m_settings.findParamMin = static_cast<ModCommand::PARAM>(dlg.GetMinVal() | mask);
				m_settings.findParamMax = static_cast<ModCommand::PARAM>(dlg.GetMaxVal() | mask);
			} else
			{
				// TODO undo selection
			}
		} else
		{
			m_settings.findParamMin = m_settings.findParamMax = static_cast<ModCommand::PARAM>(item | mask);
		}
	}
}


void CFindReplaceTab::OnCheckChannelSearch()
//------------------------------------------
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
//--------------------------
{
	// Search flags
	FlagSet<FindReplace::Flags> &flags = m_isReplaceTab ? m_settings.replaceFlags : m_settings.findFlags;
	flags.reset();
	flags.set(FindReplace::Note, !!IsDlgButtonChecked(IDC_CHECK1));
	flags.set(FindReplace::Instr, !!IsDlgButtonChecked(IDC_CHECK2));
	flags.set(FindReplace::VolCmd, !!IsDlgButtonChecked(IDC_CHECK3));
	flags.set(FindReplace::Volume, !!IsDlgButtonChecked(IDC_CHECK4));
	flags.set(FindReplace::Command, !!IsDlgButtonChecked(IDC_CHECK5));
	flags.set(FindReplace::Param, !!IsDlgButtonChecked(IDC_CHECK6));
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
