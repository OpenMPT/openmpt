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
#include "../Mptrack.h"
#include "../resource.h"
#include <rtmidi/RtMidi.h>


OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(MidiInOutEditor, CAbstractVstEditor)
	//{{AFX_MSG_MAP(MidiInOutEditor)
	ON_CBN_SELCHANGE(IDC_COMBO1,	&MidiInOutEditor::OnInputChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	&MidiInOutEditor::OnOutputChanged)
	ON_EN_CHANGE(IDC_EDIT1,			&MidiInOutEditor::OnLatencyChanged)
	ON_COMMAND(IDC_CHECK1,			&MidiInOutEditor::OnTimingMessagesChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void MidiInOutEditor::DoDataExchange(CDataExchange* pDX)
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
{
}


bool MidiInOutEditor::OpenEditor(CWnd *parent)
{
	Create(IDD_MIDI_IO_PLUGIN, parent);
	MidiInOut &plugin = static_cast<MidiInOut &>(m_VstPlugin);
	m_latencyEdit.AllowFractions(true);
	m_latencyEdit.AllowNegative(true);
	m_latencyEdit.SetDecimalValue(plugin.m_latency * 1000.0, 4);
	m_latencySpin.SetRange32(mpt::saturate_round<int>(plugin.GetOutputLatency() * -1000.0), int32_max);
	PopulateList(m_inputCombo, plugin.m_midiIn,  plugin.m_inputDevice, true);
	PopulateList(m_outputCombo, plugin.m_midiOut, plugin.m_outputDevice, false);
	CheckDlgButton(IDC_CHECK1, plugin.m_sendTimingInfo ? BST_CHECKED : BST_UNCHECKED);
	m_locked = false;
	return CAbstractVstEditor::OpenEditor(parent);
}


// Update lists of available input / output devices
void MidiInOutEditor::PopulateList(CComboBox &combo, RtMidi &rtDevice, MidiDevice &midiDevice, bool isInput)
{
	combo.SetRedraw(FALSE);
	combo.ResetContent();

	// Add dummy device
	combo.SetItemData(combo.AddString(_T("<none>")), static_cast<DWORD_PTR>(MidiInOut::kNoDevice));

	// Go through all RtMidi devices
	auto ports = rtDevice.getPortCount();
	int selectedItem = 0;
	CString portName;
	for(unsigned int i = 0; i < ports; i++)
	{
		try
		{
			portName = theApp.GetFriendlyMIDIPortName(mpt::ToCString(mpt::Charset::UTF8, midiDevice.GetPortName(i)), isInput);
			int result = combo.AddString(portName);
			combo.SetItemData(result, i);

			if(result != CB_ERR && i == midiDevice.index)
				selectedItem = result;
		} catch(RtMidiError &)
		{
		}
	}

	combo.SetCurSel(selectedItem);
	combo.SetRedraw(TRUE);
}


// Refresh current input / output device in GUI
void MidiInOutEditor::SetCurrentDevice(CComboBox &combo, MidiDevice::ID device)
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
{
	// Update device ID and notify plugin.
	MidiDevice::ID newDevice = static_cast<MidiDevice::ID>(combo.GetItemData(combo.GetCurSel()));
	plugin.SetParameter(param, MidiInOut::DeviceIDToParameter(newDevice));
	plugin.AutomateParameter(param);
}


void MidiInOutEditor::OnInputChanged()
{
	IOChanged(static_cast<MidiInOut &>(m_VstPlugin), m_inputCombo, MidiInOut::kInputParameter);
}


void MidiInOutEditor::OnOutputChanged()
{
	IOChanged(static_cast<MidiInOut &>(m_VstPlugin), m_outputCombo, MidiInOut::kOutputParameter);
}


void MidiInOutEditor::OnLatencyChanged()
{
	MidiInOut &plugin = static_cast<MidiInOut &>(m_VstPlugin);
	double latency = 0.0;
	if(!m_locked && m_latencyEdit.GetDecimalValue(latency))
	{
		plugin.m_latency = latency * (1.0 / 1000.0);
		plugin.SetModified();
	}
}


void MidiInOutEditor::OnTimingMessagesChanged()
{
	if(!m_locked)
	{
		MidiInOut &plugin = static_cast<MidiInOut &>(m_VstPlugin);
		plugin.m_sendTimingInfo = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;
		plugin.SetModified();
	}
}


OPENMPT_NAMESPACE_END

#endif // MODPLUG_TRACKER
