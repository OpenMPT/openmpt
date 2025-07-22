/*
 * MIDIMappingDialog.cpp
 * ---------------------
 * Purpose: Implementation of OpenMPT's MIDI mapping dialog, for mapping incoming MIDI messages to plugin parameters.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "MIDIMappingDialog.h"
#include "InputHandler.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Mptrack.h"
#include "Reporting.h"
#include "resource.h"
#include "WindowMessages.h"
#include "../common/mptStringBuffer.h"
#include "../soundlib/MIDIEvents.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/plugins/PlugInterface.h"

#ifndef NO_PLUGINS

OPENMPT_NAMESPACE_BEGIN


CMIDIMappingDialog::CMIDIMappingDialog(CWnd *pParent, CSoundFile &sndfile)
	: ResizableDialog{IDD_MIDIPARAMCONTROL, pParent}
	, m_sndFile{sndfile}
	, m_rMIDIMapper{m_sndFile.GetMIDIMapper()}
{
	CMainFrame::GetInputHandler()->Bypass(true);
	oldMIDIRecondWnd = CMainFrame::GetMainFrame()->GetMidiRecordWnd();
}


CMIDIMappingDialog::~CMIDIMappingDialog()
{
	CMainFrame::GetMainFrame()->SetMidiRecordWnd(oldMIDIRecondWnd);
	CMainFrame::GetInputHandler()->Bypass(false);
}


void CMIDIMappingDialog::DoDataExchange(CDataExchange *pDX)
{
	ResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_CONTROLLER, m_ControllerCBox);
	DDX_Control(pDX, IDC_COMBO_PLUGIN, m_PluginCBox);
	DDX_Control(pDX, IDC_COMBO_PARAM, m_PlugParamCBox);
	DDX_Control(pDX, IDC_LIST1, m_List);
	DDX_Control(pDX, IDC_COMBO_CHANNEL, m_ChannelCBox);
	DDX_Control(pDX, IDC_COMBO_EVENT, m_EventCBox);
	DDX_Control(pDX, IDC_SPINMOVEMAPPING, m_SpinMoveMapping);
}


BEGIN_MESSAGE_MAP(CMIDIMappingDialog, ResizableDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CMIDIMappingDialog::OnSelectionChanged)
	ON_BN_CLICKED(IDC_CHECKACTIVE, &CMIDIMappingDialog::OnBnClickedCheckactive)
	ON_BN_CLICKED(IDC_CHECKCAPTURE, &CMIDIMappingDialog::OnBnClickedCheckCapture)
	ON_CBN_SELCHANGE(IDC_COMBO_CONTROLLER, &CMIDIMappingDialog::OnCbnSelchangeComboController)
	ON_CBN_SELCHANGE(IDC_COMBO_CHANNEL, &CMIDIMappingDialog::OnCbnSelchangeComboChannel)
	ON_CBN_SELCHANGE(IDC_COMBO_PLUGIN, &CMIDIMappingDialog::OnCbnSelchangeComboPlugin)
	ON_CBN_SELCHANGE(IDC_COMBO_PARAM, &CMIDIMappingDialog::OnCbnSelchangeComboParam)
	ON_CBN_SELCHANGE(IDC_COMBO_EVENT, &CMIDIMappingDialog::OnCbnSelchangeComboEvent)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CMIDIMappingDialog::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REPLACE, &CMIDIMappingDialog::OnBnClickedButtonReplace)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CMIDIMappingDialog::OnBnClickedButtonRemove)
	ON_MESSAGE(WM_MOD_MIDIMSG,		&CMIDIMappingDialog::OnMidiMsg)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPINMOVEMAPPING, &CMIDIMappingDialog::OnDeltaposSpinmovemapping)
	ON_BN_CLICKED(IDC_CHECK_PATRECORD, &CMIDIMappingDialog::OnBnClickedCheckPatRecord)
END_MESSAGE_MAP()


LRESULT CMIDIMappingDialog::OnMidiMsg(WPARAM dwMidiDataParam, LPARAM)
{
	uint32 midiData = static_cast<uint32>(dwMidiDataParam);
	if(IsDlgButtonChecked(IDC_CHECK_MIDILEARN))
	{
		for(int i = 0; i < m_EventCBox.GetCount(); i++)
		{
			if(static_cast<MIDIEvents::EventType>(m_EventCBox.GetItemData(i)) == MIDIEvents::GetTypeFromEvent(midiData))
			{
				m_ChannelCBox.SetCurSel(1 + MIDIEvents::GetChannelFromEvent(midiData));
				m_EventCBox.SetCurSel(i);
				if(MIDIEvents::GetTypeFromEvent(midiData) == MIDIEvents::evControllerChange)
				{
					uint8 cc = MIDIEvents::GetDataByte1FromEvent(midiData);
					if(m_lastCC >= 32 || cc != m_lastCC + 32)
					{
						// Ignore second CC message of 14-bit CC.
						m_ControllerCBox.SetCurSel(cc);
					}
					m_lastCC = cc;
				}
				OnCbnSelchangeComboChannel();
				OnCbnSelchangeComboEvent();
				OnCbnSelchangeComboController();
				break;
			}
		}
	}
	return 1;
}


BOOL CMIDIMappingDialog::OnInitDialog()
{
	ResizableDialog::OnInitDialog();

	// Add events
	m_EventCBox.SetItemData(m_EventCBox.AddString(_T("Controller Change")), MIDIEvents::evControllerChange);
	m_EventCBox.SetItemData(m_EventCBox.AddString(_T("Polyphonic Aftertouch")), MIDIEvents::evPolyAftertouch);
	m_EventCBox.SetItemData(m_EventCBox.AddString(_T("Channel Aftertouch")), MIDIEvents::evChannelAftertouch);
	
	// Add controller names
	CString s;
	for(uint8 i = MIDIEvents::MIDICC_start; i <= MIDIEvents::MIDICC_end; i++)
	{
		s.Format(_T("%3u "), i);
		s += mpt::ToCString(mpt::Charset::UTF8, MIDIEvents::MidiCCNames[i]);
		m_ControllerCBox.AddString(s);
	}

	// Add plugin names
	m_PluginCBox.Update(PluginComboBox::Config{PluginComboBox::ShowEmptySlots}, m_sndFile);

	// Initialize mapping table
	static constexpr CListCtrlEx::Header headers[] =
	{
		{ _T("Channel"),            58,  LVCFMT_LEFT },
		{ _T("Event / Controller"), 176, LVCFMT_LEFT },
		{ _T("Plugin"),             120, LVCFMT_LEFT },
		{ _T("Parameter"),          120, LVCFMT_LEFT },
		{ _T("Capture"),            40,  LVCFMT_LEFT },
		{ _T("Pattern Record"),     40,  LVCFMT_LEFT }
	};
	m_List.SetHeaders(headers);
	m_List.SetExtendedStyle(m_List.GetExtendedStyle() | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

	// Add directives to list
	for(size_t i = 0; i < m_rMIDIMapper.GetCount(); i++)
	{
		InsertItem(m_rMIDIMapper.GetDirective(i), int(i));
	}

	if(m_rMIDIMapper.GetCount() > 0 && m_Setting.IsDefault())
	{
		SelectItem(0);
		OnSelectionChanged();
	} else
	{
		UpdateDialog();
	}

	GetDlgItem(IDC_CHECK_PATRECORD)->EnableWindow((m_sndFile.GetType() == MOD_TYPE_MPT) ? TRUE : FALSE);

	CMainFrame::GetMainFrame()->SetMidiRecordWnd(GetSafeHwnd());

	CheckDlgButton(IDC_CHECK_MIDILEARN, BST_CHECKED);

	return TRUE;  // return TRUE unless you set the focus to a control
}


int CMIDIMappingDialog::InsertItem(const CMIDIMappingDirective &m, int insertAt)
{
	CString s;
	if(m.GetAnyChannel())
		s = _T("Any");
	else
		s.Format(_T("Ch %u"), m.GetChannel());

	insertAt = m_List.InsertItem(insertAt, s);
	if(insertAt == -1)
		return -1;
	m_List.SetCheck(insertAt, m.IsActive() ? TRUE : FALSE);

	switch(m.GetEvent())
	{
	case MIDIEvents::evControllerChange:
		s.Format(_T("CC %u: "), m.GetController());
		if(m.GetController() <= MIDIEvents::MIDICC_end) s += mpt::ToCString(mpt::Charset::UTF8, MIDIEvents::MidiCCNames[m.GetController()]);
		break;
	case MIDIEvents::evPolyAftertouch:
		s = _T("Polyphonic Aftertouch"); break;
	case MIDIEvents::evChannelAftertouch:
		s = _T("Channel Aftertouch"); break;
	default:
		s.Format(_T("0x%02X"), m.GetEvent()); break;
	}
	m_List.SetItemText(insertAt, 1, s);

	const PLUGINDEX plugindex = m.GetPlugIndex();
	if(plugindex > 0 && plugindex < MAX_MIXPLUGINS)
	{
		const SNDMIXPLUGIN &plug = m_sndFile.m_MixPlugins[plugindex - 1];
		s.Format(_T("FX%u: "), plugindex);
		s += mpt::ToCString(plug.GetName());
		m_List.SetItemText(insertAt, 2, s);
		if(plug.pMixPlugin != nullptr)
			s = plug.pMixPlugin->GetFormattedParamName(m.GetParamIndex());
		else
			s.Empty();
		m_List.SetItemText(insertAt, 3, s);
	}
	m_List.SetItemText(insertAt, 4, m.GetCaptureMIDI() ? _T("Capt") : _T(""));
	m_List.SetItemText(insertAt, 5, m.GetAllowPatternEdit() ? _T("Rec") : _T(""));

	return insertAt;
}


void CMIDIMappingDialog::SelectItem(int i)
{
	m_List.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	m_List.SetSelectionMark(i);
}


void CMIDIMappingDialog::UpdateDialog(int selItem)
{
	CheckDlgButton(IDC_CHECKACTIVE, m_Setting.IsActive() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECKCAPTURE, m_Setting.GetCaptureMIDI() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK_PATRECORD, m_Setting.GetAllowPatternEdit() ? BST_CHECKED : BST_UNCHECKED);

	m_ChannelCBox.SetCurSel(m_Setting.GetChannel());

	m_EventCBox.SetCurSel(-1);
	for(int i = 0; i < m_EventCBox.GetCount(); i++)
	{
		if(m_EventCBox.GetItemData(i) == m_Setting.GetEvent())
		{
			m_EventCBox.SetCurSel(i);
			break;
		}
	}

	m_ControllerCBox.SetCurSel(m_Setting.GetController());
	if(PLUGINDEX plug = m_Setting.GetPlugIndex(); plug > 0)
		m_PluginCBox.SetSelection(plug - 1);
	else
		m_PluginCBox.SetSelection(PLUGINDEX_INVALID);
	m_PlugParamCBox.SetCurSel(m_Setting.GetParamIndex());

	UpdateEvent();
	UpdateParameters();

	bool enableMover = selItem >= 0;
	if(enableMover)
	{
		const bool previousEqual = (selItem > 0 && m_rMIDIMapper.AreOrderEqual(selItem - 1, selItem));
		const bool nextEqual = (selItem + 1 < m_List.GetItemCount() && m_rMIDIMapper.AreOrderEqual(selItem, selItem + 1));
		enableMover = previousEqual || nextEqual;
	}
	m_SpinMoveMapping.EnableWindow(enableMover);
}


void CMIDIMappingDialog::UpdateEvent()
{
	m_ControllerCBox.EnableWindow(m_Setting.GetEvent() == MIDIEvents::evControllerChange ? TRUE : FALSE);
	if(m_Setting.GetEvent() != MIDIEvents::evControllerChange)
		m_ControllerCBox.SetCurSel(0);
}


void CMIDIMappingDialog::UpdateParameters()
{
	m_PlugParamCBox.SetRedraw(FALSE);
	m_PlugParamCBox.ResetContent();
	AddPluginParameternamesToCombobox(m_PlugParamCBox, m_sndFile.m_MixPlugins[m_Setting.GetPlugIndex() - 1]);
	m_PlugParamCBox.SetCurSel(m_Setting.GetParamIndex());
	m_PlugParamCBox.SetRedraw(TRUE);
	m_PlugParamCBox.Invalidate();
}


void CMIDIMappingDialog::OnSelectionChanged(NMHDR *pNMHDR, LRESULT * /*pResult*/)
{
	int i;
	if(pNMHDR != nullptr)
	{
		// cppcheck-suppress dangerousTypeCast
		NMLISTVIEW *nmlv = (NMLISTVIEW *)pNMHDR;

		if(((nmlv->uOldState ^ nmlv->uNewState) & INDEXTOSTATEIMAGEMASK(3)) != 0 && nmlv->uOldState != 0)
		{
			// Check box status changed
			CMIDIMappingDirective m = m_rMIDIMapper.GetDirective(nmlv->iItem);
			m.SetActive(nmlv->uNewState == INDEXTOSTATEIMAGEMASK(2));
			m_rMIDIMapper.SetDirective(nmlv->iItem, m);
			SetModified();
			if(nmlv->iItem == m_List.GetSelectionMark())
				CheckDlgButton(IDC_CHECKACTIVE, nmlv->uNewState == INDEXTOSTATEIMAGEMASK(2) ? BST_CHECKED : BST_UNCHECKED);
		}

		if(nmlv->uNewState & LVIS_SELECTED)
			i = nmlv->iItem;
		else
			return;
	} else
	{
		i = m_List.GetSelectionMark();
	}
	if(i < 0 || (size_t)i >= m_rMIDIMapper.GetCount()) return;
	m_Setting = m_rMIDIMapper.GetDirective(i);
	UpdateDialog(i);
}


void CMIDIMappingDialog::OnBnClickedCheckactive()
{
	m_Setting.SetActive(IsDlgButtonChecked(IDC_CHECKACTIVE) == BST_CHECKED);
}


void CMIDIMappingDialog::OnBnClickedCheckCapture()
{
	m_Setting.SetCaptureMIDI(IsDlgButtonChecked(IDC_CHECKCAPTURE) == BST_CHECKED);
}


void CMIDIMappingDialog::OnBnClickedCheckPatRecord()
{
	m_Setting.SetAllowPatternEdit(IsDlgButtonChecked(IDC_CHECK_PATRECORD) == BST_CHECKED);
}


void CMIDIMappingDialog::OnCbnSelchangeComboController()
{
	m_Setting.SetController(m_ControllerCBox.GetCurSel());
}


void CMIDIMappingDialog::OnCbnSelchangeComboChannel()
{
	m_Setting.SetChannel(m_ChannelCBox.GetCurSel());
}


void CMIDIMappingDialog::OnCbnSelchangeComboPlugin()
{
	PLUGINDEX i = m_PluginCBox.GetSelection().value_or(PLUGINDEX_INVALID);
	if(i >= MAX_MIXPLUGINS)
		return;
	m_Setting.SetPlugIndex(i + 1);
	UpdateParameters();
}


void CMIDIMappingDialog::OnCbnSelchangeComboParam()
{
	m_Setting.SetParamIndex(m_PlugParamCBox.GetCurSel());
}


void CMIDIMappingDialog::OnCbnSelchangeComboEvent()
{
	uint8 eventType = static_cast<uint8>(m_EventCBox.GetItemData(m_EventCBox.GetCurSel()));
	m_Setting.SetEvent(eventType);
	UpdateEvent();
}


void CMIDIMappingDialog::OnBnClickedButtonAdd()
{
	if(m_sndFile.GetModSpecifications().MIDIMappingDirectivesMax <= m_rMIDIMapper.GetCount())
	{
		Reporting::Information("Maximum amount of MIDI Mapping directives reached.");
	} else
	{
		const size_t i = m_rMIDIMapper.AddDirective(m_Setting);
		SetModified();

		SelectItem(InsertItem(m_Setting, static_cast<int>(i)));
		OnSelectionChanged();
	}
}


void CMIDIMappingDialog::OnBnClickedButtonReplace()
{
	const int i = m_List.GetSelectionMark();
	if(i >= 0 && (size_t)i < m_rMIDIMapper.GetCount())
	{
		const size_t newIndex = m_rMIDIMapper.SetDirective(i, m_Setting);
		SetModified();

		m_List.DeleteItem(i);
		SelectItem(InsertItem(m_Setting, static_cast<int>(newIndex)));
		OnSelectionChanged();
	}
}


void CMIDIMappingDialog::OnBnClickedButtonRemove()
{
	int i = m_List.GetSelectionMark();
	if(i >= 0 && (size_t)i < m_rMIDIMapper.GetCount())
	{
		m_rMIDIMapper.RemoveDirective(i);
		SetModified();

		m_List.DeleteItem(i);
		if(m_List.GetItemCount() > 0)
		{
			if(i < m_List.GetItemCount())
				SelectItem(i);
			else
				SelectItem(i - 1);
		}
		i = m_List.GetSelectionMark();
		if(i >= 0 && (size_t)i < m_rMIDIMapper.GetCount())
			m_Setting = m_rMIDIMapper.GetDirective(i);

		OnSelectionChanged();
	}
}


void CMIDIMappingDialog::OnDeltaposSpinmovemapping(NMHDR *pNMHDR, LRESULT *pResult)
{
	const int index = m_List.GetSelectionMark();
	if(index < 0 || index >= m_List.GetItemCount()) return;

	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	int newIndex = -1;
	if(pNMUpDown->iDelta < 0) //Up
	{
		if(index - 1 >= 0 && m_rMIDIMapper.AreOrderEqual(index-1, index))
		{
			newIndex = index - 1;
		}
	} else //Down
	{
		if(index + 1 < m_List.GetItemCount() && m_rMIDIMapper.AreOrderEqual(index, index+1))
		{
			newIndex = index + 1;
		}
	}

	if(newIndex != -1)
	{
		m_rMIDIMapper.Swap(size_t(newIndex), size_t(index));
		m_List.DeleteItem(index);
		InsertItem(m_rMIDIMapper.GetDirective(newIndex), newIndex);
		SelectItem(newIndex);
	}
	
	*pResult = 0;
}


CString CMIDIMappingDialog::GetToolTipText(UINT id, HWND) const
{
	const TCHAR *text = _T("");
	switch(id)
	{
	case IDC_CHECKCAPTURE:
		text = _T("The event is not passed to any further MIDI mappings or recording facilities.");
		break;
	case IDC_CHECKACTIVE:
		text = _T("The MIDI mapping is enabled and can be processed.");
		break;
	case IDC_CHECK_PATRECORD:
		text = _T("Parameter changes are recorded into patterns as Parameter Control events.");
		break;
	case IDC_CHECK_MIDILEARN:
		text = _T("Listens to incoming MIDI data to automatically fill in the appropriate data.");
		break;
	case IDC_SPINMOVEMAPPING:
		text = _T("Change the processing order of the current selected MIDI mapping.");
		break;
	case IDC_COMBO_CHANNEL:
		text = _T("The MIDI channel to listen on for this event.");
		break;
	case IDC_COMBO_EVENT:
		text = _T("The MIDI event to listen for.");
		break;
	case IDC_COMBO_CONTROLLER:
		text = _T("The MIDI controler to listen for.");
		break;
	}

	return text;
}


void CMIDIMappingDialog::SetModified()
{
	if(m_sndFile.GetpModDoc() != nullptr)
		m_sndFile.GetpModDoc()->SetModified();
}


OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
