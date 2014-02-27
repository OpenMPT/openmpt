/*
 * MIDIMappingDialog.cpp
 * ---------------------
 * Purpose: Implementation of OpenMPT's MIDI mapping dialog, for mapping incoming MIDI messages to plugin parameters.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "MIDIMappingDialog.h"
#include "../soundlib/MIDIEvents.h"
#include "mainfrm.h"


// CMIDIMappingDialog dialog

CMIDIMappingDialog::CMIDIMappingDialog(CWnd* pParent /*=NULL*/, CSoundFile& rSndfile)
	: CDialog(CMIDIMappingDialog::IDD, pParent), m_rSndFile(rSndfile), 
	  m_rMIDIMapper(m_rSndFile.GetMIDIMapper())
//---------------------------------------------------------------------
{
	oldMIDIRecondWnd = CMainFrame::GetMainFrame()->GetMidiRecordWnd();
}


CMIDIMappingDialog::~CMIDIMappingDialog()
//---------------------------------------
{
	CMainFrame::GetMainFrame()->SetMidiRecordWnd(oldMIDIRecondWnd);
}


void CMIDIMappingDialog::DoDataExchange(CDataExchange* pDX)
//---------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_CONTROLLER, m_ControllerCBox);
	DDX_Control(pDX, IDC_COMBO_PLUGIN, m_PluginCBox);
	DDX_Control(pDX, IDC_COMBO_PARAM, m_PlugParamCBox);
	DDX_Control(pDX, IDC_EDIT1, m_EditValue);
	DDX_Control(pDX, IDC_LIST1, m_List);
	DDX_Control(pDX, IDC_COMBO_CHANNEL, m_ChannelCBox);
	DDX_Control(pDX, IDC_COMBO_EVENT, m_EventCBox);
	DDX_Control(pDX, IDC_SPINMOVEMAPPING, m_SpinMoveMapping);
}


BEGIN_MESSAGE_MAP(CMIDIMappingDialog, CDialog)
	ON_LBN_SELCHANGE(IDC_LIST1, OnLbnSelchangeList1)
	ON_BN_CLICKED(IDC_CHECKACTIVE, OnBnClickedCheckactive)
	ON_BN_CLICKED(IDC_CHECKCAPTURE, OnBnClickedCheckCapture)
	ON_CBN_SELCHANGE(IDC_COMBO_CONTROLLER, OnCbnSelchangeComboController)
	ON_CBN_SELCHANGE(IDC_COMBO_CHANNEL, OnCbnSelchangeComboChannel)
	ON_CBN_SELCHANGE(IDC_COMBO_PLUGIN, OnCbnSelchangeComboPlugin)
	ON_CBN_SELCHANGE(IDC_COMBO_PARAM, OnCbnSelchangeComboParam)
	ON_CBN_SELCHANGE(IDC_COMBO_EVENT, OnCbnSelchangeComboEvent)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REPLACE, OnBnClickedButtonReplace)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnBnClickedButtonRemove)
	ON_MESSAGE(WM_MOD_MIDIMSG,		OnMidiMsg)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPINMOVEMAPPING, OnDeltaposSpinmovemapping)
	ON_BN_CLICKED(IDC_CHECK_PATRECORD, OnBnClickedCheckPatRecord)
END_MESSAGE_MAP()


LRESULT CMIDIMappingDialog::OnMidiMsg(WPARAM dwMidiDataParam, LPARAM)
//-------------------------------------------------------------------
{
	if(IsDlgButtonChecked(IDC_CHECK_MIDILEARN))
	{
		for(int i = 0; i < m_EventCBox.GetCount(); i++)
		{
			if(static_cast<MIDIEvents::EventType>(m_EventCBox.GetItemData(i)) == MIDIEvents::GetTypeFromEvent(dwMidiDataParam))
			{
				m_ChannelCBox.SetCurSel(1 + MIDIEvents::GetChannelFromEvent(dwMidiDataParam));
				m_EventCBox.SetCurSel(i);
				if(MIDIEvents::GetTypeFromEvent(dwMidiDataParam) == MIDIEvents::evControllerChange)
				{
					m_ControllerCBox.SetCurSel(MIDIEvents::GetDataByte1FromEvent(dwMidiDataParam));
				}
				OnCbnSelchangeComboChannel();
				OnCbnSelchangeComboEvent();
				OnCbnSelchangeComboController();
				UpdateString();
				break;
			}
		}
	}
	return 1;
}


BOOL CMIDIMappingDialog::OnInitDialog()
//-------------------------------------
{
	CDialog::OnInitDialog();

	// Add events
	m_EventCBox.SetItemData(m_EventCBox.AddString("Controller change (0xB)"), MIDIEvents::evControllerChange);
	m_EventCBox.SetItemData(m_EventCBox.AddString("Polyphonic aftertouch (0xA)"), MIDIEvents::evPolyAftertouch);
	m_EventCBox.SetItemData(m_EventCBox.AddString("Channel aftertouch (0xD)"), MIDIEvents::evChannelAftertouch);
	
	// Add controller names
	for(size_t i = MIDIEvents::MIDICC_start; i <= MIDIEvents::MIDICC_end; i++)
	{
		CString temp;
		temp.Format("%3d %s", i, MIDIEvents::MidiCCNames[i]);
		m_ControllerCBox.AddString(temp);
	}

	// Add plugin names
	AddPluginNamesToCombobox(m_PluginCBox, m_rSndFile.m_MixPlugins);

	// Add directives to list
	typedef CMIDIMapper::const_iterator CITER;
	for(CITER iter = m_rMIDIMapper.Begin(); iter != m_rMIDIMapper.End(); iter++)
	{
		m_List.AddString(CreateListString(*iter));
	}

	if(m_rMIDIMapper.GetCount() > 0 && m_Setting.IsDefault())
	{
		m_List.SetCurSel(0);
		OnLbnSelchangeList1();
	} else
	{
		UpdateDialog();
	}

	GetDlgItem(IDC_CHECK_PATRECORD)->EnableWindow((m_rSndFile.GetType() == MOD_TYPE_MPT) ? TRUE : FALSE);

	UpdateString();

	CMainFrame::GetMainFrame()->SetMidiRecordWnd(GetSafeHwnd());

	CheckDlgButton(IDC_CHECK_MIDILEARN, BST_CHECKED);
	
	return TRUE;  // return TRUE unless you set the focus to a control
}


void CMIDIMappingDialog::UpdateDialog()
//-------------------------------------
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
	m_PluginCBox.SetCurSel(m_Setting.GetPlugIndex() - 1);
	m_PlugParamCBox.SetCurSel(m_Setting.GetParamIndex());
	SetDlgItemText(IDC_EDIT1, m_Setting.ToString().c_str());

	UpdateEvent();
	UpdateParameters();

	const int i = m_List.GetCurSel();
	const bool enableMover = i >= 0 && ( (i > 0 && m_rMIDIMapper.AreOrderEqual(i - 1, i)) || (i + 1 < m_List.GetCount() && m_rMIDIMapper.AreOrderEqual(i, i + 1)));
	m_SpinMoveMapping.EnableWindow(enableMover);
}


void CMIDIMappingDialog::UpdateEvent()
//------------------------------------
{
	m_ControllerCBox.EnableWindow(m_Setting.GetEvent() == MIDIEvents::evControllerChange ? TRUE : FALSE);
	if(m_Setting.GetEvent() != MIDIEvents::evControllerChange)
		m_ControllerCBox.SetCurSel(0);
}


void CMIDIMappingDialog::UpdateParameters()
//-----------------------------------------
{
	m_PlugParamCBox.SetRedraw(FALSE);
	m_PlugParamCBox.ResetContent();
	AddPluginParameternamesToCombobox(m_PlugParamCBox, m_rSndFile.m_MixPlugins[m_Setting.GetPlugIndex() - 1]);
	m_PlugParamCBox.SetCurSel(m_Setting.GetParamIndex());
	m_PlugParamCBox.SetRedraw(TRUE);
	m_PlugParamCBox.Invalidate();
}


void CMIDIMappingDialog::OnLbnSelchangeList1()
//--------------------------------------------
{
	const int i = m_List.GetCurSel();
	if(i < 0 || (size_t)i >= m_rMIDIMapper.GetCount()) return;
	m_Setting = m_rMIDIMapper.GetDirective(i);
	UpdateDialog();
}


void CMIDIMappingDialog::OnBnClickedCheckactive()
//-----------------------------------------------
{
	m_Setting.SetActive(IsDlgButtonChecked(IDC_CHECKACTIVE) == BST_CHECKED);
	UpdateString();
}


void CMIDIMappingDialog::OnBnClickedCheckCapture()
//------------------------------------------------
{
	m_Setting.SetCaptureMIDI(IsDlgButtonChecked(IDC_CHECKCAPTURE) == BST_CHECKED);
	UpdateString();
}


void CMIDIMappingDialog::OnBnClickedCheckPatRecord()
//--------------------------------------------------
{
	m_Setting.SetAllowPatternEdit(IsDlgButtonChecked(IDC_CHECK_PATRECORD) == BST_CHECKED);
	UpdateString();
}


void CMIDIMappingDialog::UpdateString()
//-------------------------------------
{
	SetDlgItemText(IDC_EDIT1, m_Setting.ToString().c_str());
}


void CMIDIMappingDialog::OnCbnSelchangeComboController()
//------------------------------------------------------
{
	m_Setting.SetController(m_ControllerCBox.GetCurSel());
	UpdateString();
}


void CMIDIMappingDialog::OnCbnSelchangeComboChannel()
//---------------------------------------------------
{
	m_Setting.SetChannel(m_ChannelCBox.GetCurSel());
	UpdateString();
}


void CMIDIMappingDialog::OnCbnSelchangeComboPlugin()
//--------------------------------------------------
{
	int i = m_PluginCBox.GetCurSel();
	if(i < 0 || i >= MAX_MIXPLUGINS) return;
	m_Setting.SetPlugIndex(i+1);
	UpdateParameters();
	UpdateString();
}


void CMIDIMappingDialog::OnCbnSelchangeComboParam()
//-------------------------------------------------
{
	m_Setting.SetParamIndex(m_PlugParamCBox.GetCurSel());
	UpdateString();
}


void CMIDIMappingDialog::OnCbnSelchangeComboEvent()
//-------------------------------------------------
{
	uint8 eventType = static_cast<uint8>(m_EventCBox.GetItemData(m_EventCBox.GetCurSel()));
	m_Setting.SetEvent(eventType);
	UpdateEvent();
	UpdateString();
}


void CMIDIMappingDialog::OnBnClickedButtonAdd()
//---------------------------------------------
{
	if(m_rSndFile.GetModSpecifications().MIDIMappingDirectivesMax <= m_rMIDIMapper.GetCount())
	{
		Reporting::Information("Maximum amount of MIDI Mapping directives reached.");
	}
	else
	{
		const size_t i = m_rMIDIMapper.AddDirective(m_Setting);
		if(m_rSndFile.GetpModDoc())
			m_rSndFile.GetpModDoc()->SetModified();

		m_List.InsertString(i, CreateListString(m_Setting));
		m_List.SetCurSel(i);
		OnLbnSelchangeList1();
	}
}


void CMIDIMappingDialog::OnBnClickedButtonReplace()
//-------------------------------------------------
{
	const int i = m_List.GetCurSel();
	if(i >= 0 && (size_t)i < m_rMIDIMapper.GetCount())
	{
		const size_t newIndex = m_rMIDIMapper.SetDirective(i, m_Setting);
		if(m_rSndFile.GetpModDoc())
			m_rSndFile.GetpModDoc()->SetModified();

		m_List.DeleteString(i);
		m_List.InsertString(newIndex, CreateListString(m_Setting));
		m_List.SetCurSel(newIndex);
		OnLbnSelchangeList1();
	}
}


void CMIDIMappingDialog::OnBnClickedButtonRemove()
//------------------------------------------------
{
	int i = m_List.GetCurSel();
	if(i >= 0 && (size_t)i < m_rMIDIMapper.GetCount())
	{
		m_rMIDIMapper.RemoveDirective(i);
		if(m_rSndFile.GetpModDoc())
			m_rSndFile.GetpModDoc()->SetModified();

		m_List.DeleteString(i);
		if(m_List.GetCount() > 0)
		{
			if(i < m_List.GetCount())
				m_List.SetCurSel(i);
			else
				m_List.SetCurSel(i-1);
		}
		i = m_List.GetCurSel();
		if(i >= 0 && (size_t)i < m_rMIDIMapper.GetCount())
			m_Setting = m_rMIDIMapper.GetDirective(i);

		OnLbnSelchangeList1();
		
	}
}


CString CMIDIMappingDialog::CreateListString(const CMIDIMappingDirective& s)
//--------------------------------------------------------------------------
{
	CString str;
	str.Preallocate(100);
	str = s.ToString().c_str();
	const BYTE plugindex = s.GetPlugIndex();

	//Short ID name
	while(str.GetLength() < 19) str.AppendChar('.');
	str.AppendChar('.');

	//Controller name
	if(s.GetController() <= MIDIEvents::MIDICC_end && s.GetEvent() == MIDIEvents::evControllerChange)
	{
		CString tstr;
		tstr.Format("%d %s", s.GetController(), MIDIEvents::MidiCCNames[s.GetController()]);
		str.Insert(20, tstr);
	}

	while(str.GetLength() < 54) str.AppendChar('.');
	str.AppendChar('.');

	//Plugname
	if(plugindex > 0 && plugindex < MAX_MIXPLUGINS && m_rSndFile.m_MixPlugins[plugindex-1].pMixPlugin)
	{
		str.Insert(55, m_rSndFile.m_MixPlugins[plugindex-1].GetName());
		while(str.GetLength() < 79) str.AppendChar('.');
		str.AppendChar('.');

		//Param name
		str.Insert(80, m_rSndFile.m_MixPlugins[plugindex-1].GetParamName(s.GetParamIndex()).c_str());
	}
	else
		str.Insert(55, "No plugin");

	return str;
}


void CMIDIMappingDialog::OnDeltaposSpinmovemapping(NMHDR *pNMHDR, LRESULT *pResult)
//---------------------------------------------------------------------------------
{
	const int index = m_List.GetCurSel();
	if(index < 0 || index >= m_List.GetCount()) return;

	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	
	if(pNMUpDown->iDelta < 0) //Up
	{
		if(index - 1 >= 0 && m_rMIDIMapper.AreOrderEqual(index-1, index))
		{
			CString str;
			m_List.GetText(index, str);
			m_List.DeleteString(index);
			m_List.InsertString(index-1, str);
			m_rMIDIMapper.Swap(size_t(index-1), size_t(index));
			m_List.SetCurSel(index-1);
		}
	}
	else //Down
	{
		if(index + 1 < m_List.GetCount() && m_rMIDIMapper.AreOrderEqual(index, index+1))
		{
			CString str;
			m_List.GetText(index, str);
			m_List.DeleteString(index);
			m_List.InsertString(index+1, str);
			m_rMIDIMapper.Swap(size_t(index), size_t(index+1));
			m_List.SetCurSel(index+1);
		}
	}
	
	
	*pResult = 0;
}
