/*
 * MidiInOutEditor.cpp
 * -------------------
 * Purpose: Editor interface for the MidiInOut plugin.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#ifdef MODPLUG_TRACKER
#include "MidiInOut.h"
#include "MidiInOutEditor.h"
#include "../../mptrack/Mptrack.h"
#include "../../mptrack/resource.h"
#include <rtmidi/RtMidi.h>


OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(MidiInOutEditor, CAbstractVstEditor)
	//{{AFX_MSG_MAP(MidiInOutEditor)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnInputChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnOutputChanged)
	ON_EN_CHANGE(IDC_EDIT1,			OnLatencyChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void MidiInOutEditor::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(MidiInOutEditor)
	DDX_Control(pDX, IDC_COMBO1,	m_inputCombo);
	DDX_Control(pDX, IDC_COMBO2,	m_outputCombo);
	DDX_Control(pDX, IDC_EDIT1,		m_latencyEdit);
	DDX_Control(pDX, IDC_SPIN1,		m_latencySpin);
	//}}AFX_DATA_MAP
}


MidiInOutEditor::MidiInOutEditor(MidiInOut &plugin)
	: CAbstractVstEditor(plugin)
	, m_locked(true)
//-------------------------------------------------
{
}


bool MidiInOutEditor::OpenEditor(CWnd *parent)
//--------------------------------------------
{
	Create(IDD_MIDI_IO_PLUGIN, parent);
	SetDlgItemInt(IDC_EDIT1, Util::Round<int>(static_cast<MidiInOut &>(m_VstPlugin).m_latency * 1000.0), TRUE);
	m_latencySpin.SetRange32(Util::Round<int>(m_VstPlugin.GetOutputLatency() * -1000.0), int32_max);
	PopulateLists();
	m_locked = false;
	return CAbstractVstEditor::OpenEditor(parent);
}


// Update lists of available input / output devices
void MidiInOutEditor::PopulateLists()
//-----------------------------------
{
	// Clear lists
	m_inputCombo.SetRedraw(FALSE);
	m_outputCombo.SetRedraw(FALSE);
	m_inputCombo.ResetContent();
	m_outputCombo.ResetContent();

	// Add dummy devices
	m_inputCombo.SetItemData(m_inputCombo.AddString(_T("<none>")), static_cast<DWORD_PTR>(MidiInOut::kNoDevice));
	m_outputCombo.SetItemData(m_outputCombo.AddString(_T("<none>")), static_cast<DWORD_PTR>(MidiInOut::kNoDevice));

	int selectInputItem = 0;
	int selectOutputItem = 0;
	MidiInOut &plugin = static_cast<MidiInOut &>(m_VstPlugin);

	// Go through all RtMidi devices
	unsigned int ports = plugin.m_midiIn.getPortCount();
	std::string portName;
	for(unsigned int i = 0; i < ports; i++)
	{
		try
		{
			portName = theApp.GetFriendlyMIDIPortName(mpt::ToCString(mpt::CharsetUTF8, plugin.m_inputDevice.GetPortName(i)), true);
			int result = m_inputCombo.AddString(portName.c_str());
			m_inputCombo.SetItemData(result, i);

			if(result != CB_ERR && i == plugin.m_inputDevice.index)
				selectInputItem = result;
		}
		catch(RtMidiError &)
		{
		}
	}

	ports = plugin.m_midiOut.getPortCount();
	for(unsigned int i = 0; i < ports; i++)
	{
		try
		{
			portName = theApp.GetFriendlyMIDIPortName(mpt::ToCString(mpt::CharsetUTF8, plugin.m_outputDevice.GetPortName(i)), false);
			int result = m_outputCombo.AddString(portName.c_str());
			m_outputCombo.SetItemData(result, i);

			if(result != CB_ERR && i == plugin.m_outputDevice.index)
				selectOutputItem = result;
		} catch(RtMidiError &)
		{
		}
	}

	// Set selections to current devices
	m_inputCombo.SetCurSel(selectInputItem);
	m_outputCombo.SetCurSel(selectOutputItem);
	m_inputCombo.SetRedraw(TRUE);
	m_outputCombo.SetRedraw(TRUE);
}


// Refresh current input / output device in GUI
void MidiInOutEditor::SetCurrentDevice(CComboBox &combo, MidiDevice::ID device)
//-----------------------------------------------------------------------------
{
	int items = combo.GetCount();
	for(int i = 0; i < items; i++)
	{
		if(static_cast<MidiDevice::ID>(combo.GetItemData(i)) == device)
		{
			combo.SetCurSel(i);
			break;
		}
	}
}


static void IOChanged(MidiInOut &plugin, CComboBox &combo, PlugParamIndex param)
//------------------------------------------------------------------------------
{
	// Update device ID and notify plugin.
	MidiDevice::ID newDevice = static_cast<MidiDevice::ID>(combo.GetItemData(combo.GetCurSel()));
	plugin.SetParameter(param, MidiInOut::DeviceIDToParameter(newDevice));
	plugin.AutomateParameter(param);
}


void MidiInOutEditor::OnInputChanged()
//------------------------------------
{
	IOChanged(static_cast<MidiInOut &>(m_VstPlugin), m_inputCombo, MidiInOut::kInputParameter);
}


void MidiInOutEditor::OnOutputChanged()
//-------------------------------------
{
	IOChanged(static_cast<MidiInOut &>(m_VstPlugin), m_outputCombo, MidiInOut::kOutputParameter);
}


void MidiInOutEditor::OnLatencyChanged()
//--------------------------------------
{
	MidiInOut &plugin = static_cast<MidiInOut &>(m_VstPlugin);
	BOOL success = FALSE;
	int value = static_cast<int>(GetDlgItemInt(IDC_EDIT1, &success, TRUE));
	if(success && !m_locked)
	{
		plugin.m_latency = value * (1.0 / 1000.0);
		plugin.SetModified();
	}
}


OPENMPT_NAMESPACE_END

#endif // MODPLUG_TRACKER
