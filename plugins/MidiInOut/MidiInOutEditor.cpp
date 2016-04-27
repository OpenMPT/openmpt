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
#include "../../mptrack/resource.h"


OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(MidiInOutEditor, CAbstractVstEditor)
	//{{AFX_MSG_MAP(MidiInOutEditor)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnInputChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnOutputChanged)
	ON_COMMAND(IDC_CHECK1,			OnLatencyToggled)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void MidiInOutEditor::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(MidiInOutEditor)
	DDX_Control(pDX, IDC_COMBO1,	m_inputCombo);
	DDX_Control(pDX, IDC_COMBO2,	m_outputCombo);
	DDX_Control(pDX, IDC_CHECK1,	m_latencyCheck);
	//}}AFX_DATA_MAP
}


MidiInOutEditor::MidiInOutEditor(MidiInOut &plugin)
	: CAbstractVstEditor(plugin)
//-------------------------------------------------
{
}


bool MidiInOutEditor::OpenEditor(CWnd *parent)
//--------------------------------------------
{
	Create(IDD_MIDI_IO_PLUGIN, parent);
	m_latencyCheck.SetCheck(static_cast<MidiInOut &>(m_VstPlugin).latencyCompensation ? BST_CHECKED : BST_UNCHECKED);
	PopulateLists();
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

	const PmDeviceInfo *device;

	// Go through all PortMidi devices
	for(PmDeviceID i = 0; (device = Pm_GetDeviceInfo(i)) != nullptr; i++)
	{
		std::string deviceName = std::string(device->name) + std::string(" [") + std::string(device->interf) + std::string("]");
		// We could make use of Pm_GetDefaultInputDeviceID / Pm_GetDefaultOutputDeviceID here, but is it useful to show those actually?

		if(device->input)
		{
			// We can actually receive MIDI data on this device.
			int result = m_inputCombo.AddString(deviceName.c_str());
			m_inputCombo.SetItemData(result, i);

			if(result != CB_ERR && i == plugin.inputDevice.index)
				selectInputItem = result;
		}

		if(device->output)
		{
			// We can actually output MIDI data on this device.
			int result = m_outputCombo.AddString(deviceName.c_str());
			m_outputCombo.SetItemData(result, i);

			if(result != CB_ERR && i == plugin.outputDevice.index)
				selectOutputItem = result;
		}
	}

	// Set selections to current devices
	m_inputCombo.SetCurSel(selectInputItem);
	m_outputCombo.SetCurSel(selectOutputItem);
	m_inputCombo.SetRedraw(TRUE);
	m_outputCombo.SetRedraw(TRUE);
}


// Refresh current input / output device in GUI
void MidiInOutEditor::SetCurrentDevice(CComboBox &combo, PmDeviceID device)
//-------------------------------------------------------------------------
{
	int items = combo.GetCount();
	for(int i = 0; i < items; i++)
	{
		if(static_cast<PmDeviceID>(combo.GetItemData(i)) == device)
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
	PmDeviceID newDevice = static_cast<PmDeviceID>(combo.GetItemData(combo.GetCurSel()));
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


void MidiInOutEditor::OnLatencyToggled()
//--------------------------------------
{
	MidiInOut &plugin = static_cast<MidiInOut &>(m_VstPlugin);
	plugin.CloseDevice(plugin.outputDevice);
	plugin.latencyCompensation = (m_latencyCheck.GetCheck() != BST_UNCHECKED);
	plugin.OpenDevice(plugin.outputDevice.index, false);
}


OPENMPT_NAMESPACE_END

#endif // MODPLUG_TRACKER
