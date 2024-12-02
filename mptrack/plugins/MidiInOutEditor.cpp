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
#include "MidiInOutEditor.h"
#include "MidiInOut.h"
#include "../FileDialog.h"
#include "../Mptrack.h"
#include "../resource.h"
#include "../UpdateHints.h"
#include "../../soundlib/MIDIEvents.h"
#include "../../soundlib/MIDIMacroParser.h"
#include <rtmidi/RtMidi.h>


OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(MidiInOutEditor, CAbstractVstEditor)
	//{{AFX_MSG_MAP(MidiInOutEditor)
	ON_CBN_SELCHANGE(IDC_COMBO1, &MidiInOutEditor::OnInputChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2, &MidiInOutEditor::OnOutputChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3, &MidiInOutEditor::OnParamChanged)
	ON_EN_CHANGE(IDC_EDIT1,      &MidiInOutEditor::OnLatencyChanged)
	ON_EN_CHANGE(IDC_EDIT2,      &MidiInOutEditor::OnMidiDumpChanged)
	ON_EN_CHANGE(IDC_EDIT3,      &MidiInOutEditor::OnMacroChanged)
	ON_COMMAND(IDC_CHECK1,       &MidiInOutEditor::OnTimingMessagesChanged)
	ON_COMMAND(IDC_CHECK2,       &MidiInOutEditor::OnAlwaysSendDumpChanged)
	ON_COMMAND(IDC_BUTTON1,      &MidiInOutEditor::OnLoadMidiDump)
	ON_COMMAND(IDC_BUTTON2,      &MidiInOutEditor::OnSendDumpNow)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void MidiInOutEditor::DoDataExchange(CDataExchange* pDX)
{
	CAbstractVstEditor::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(MidiInOutEditor)
	DDX_Control(pDX, IDC_COMBO1, m_inputCombo);
	DDX_Control(pDX, IDC_COMBO2, m_outputCombo);
	DDX_Control(pDX, IDC_COMBO3, m_paramCombo);
	DDX_Control(pDX, IDC_EDIT1,  m_latencyEdit);
	DDX_Control(pDX, IDC_EDIT2,  m_dumpEdit);
	DDX_Control(pDX, IDC_EDIT3 , m_paramEdit);
	DDX_Control(pDX, IDC_SPIN1,  m_latencySpin);
	//}}AFX_DATA_MAP
}


MidiInOutEditor::MidiInOutEditor(MidiInOut &plugin)
	: CAbstractVstEditor{plugin}
{
	m_latencyEdit.SetAccessibleSuffix(_T("milliseconds"));
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
	UpdateOutputPlugin();
	CheckDlgButton(IDC_CHECK1, plugin.m_sendTimingInfo ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK2, plugin.m_alwaysSendInitialDump ? BST_UNCHECKED : BST_CHECKED);
	UpdateMidiDump();
	m_paramCombo.SetRedraw(FALSE);
	for(unsigned int i = MidiInOut::kMacroParamMin; i <= MidiInOut::kMacroParamMax; i++)
	{
		m_paramCombo.AddString(MPT_CFORMAT("Parameter {}")(i));
	}
	m_paramCombo.SetRedraw(TRUE);
	m_paramCombo.SetCurSel(0);
	if(!plugin.m_parameterMacros.empty())
		m_paramEdit.SetWindowText(mpt::ToCString(mpt::Charset::ASCII, plugin.m_parameterMacros[0].first));
	m_locked = false;
	return CAbstractVstEditor::OpenEditor(parent);
}


void MidiInOutEditor::UpdateView(UpdateHint hint)
{
	CAbstractVstEditor::UpdateView(hint);
	PluginHint pluginHint = hint.ToType<PluginHint>();
	if(pluginHint.GetType()[HINT_MODTYPE | HINT_PLUGINNAMES] || pluginHint.GetPlugin() == (m_VstPlugin.GetSlot() + 1))
		UpdateOutputPlugin();
}


void MidiInOutEditor::UpdateMidiDump()
{
	MidiInOut &plugin = static_cast<MidiInOut &>(m_VstPlugin);
	m_formattedDump.clear();
	m_formattedDump.reserve(plugin.m_initialMidiDump.size() * 3);
	MIDIMacroParser parser{mpt::as_span(plugin.m_initialMidiDump)};
	mpt::span<uint8> midiMsg;
	bool firstLine = true;
	while(parser.NextMessage(midiMsg, false))
	{
		bool firstChar = true;
		for(uint8 c : midiMsg)
		{
			if(firstChar && !firstLine)
				m_formattedDump += "\r\n";
			else if(!firstChar)
				m_formattedDump += ' ';
			firstChar = firstLine = false;
			m_formattedDump += mpt::afmt::HEX0<2>(c);
		}
	}
	m_dumpEdit.SetWindowText(mpt::ToCString(mpt::Charset::ASCII, m_formattedDump));
}


// Update lists of available input / output devices
void MidiInOutEditor::PopulateList(CComboBox &combo, RtMidi &rtDevice, MidiDevice &midiDevice, bool isInput)
{
	combo.SetRedraw(FALSE);
	combo.ResetContent();

	// Add dummy device
	combo.SetItemData(combo.AddString(_T("<none>")), static_cast<DWORD_PTR>(MidiInOut::kNoDevice));
	if(!isInput)
	{
		combo.SetItemData(combo.AddString(_T("Internal OpenMPT Output")), static_cast<DWORD_PTR>(MidiInOut::kInternalDevice));
	}

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


void MidiInOutEditor::UpdateOutputPlugin()
{
	MPT_ASSERT(m_outputCombo.GetItemData(1) == MidiInOut::kInternalDevice);
	const int sel = m_outputCombo.GetCurSel();
	CString outputPlugin;
	if(std::vector<IMixPlugin *> plug; m_VstPlugin.GetOutputPlugList(plug) && plug.front() != nullptr)
		outputPlugin = MPT_CFORMAT("FX{}: {}")(mpt::cfmt::dec0<2>(plug.front()->GetSlot() + 1), mpt::ToCString(m_VstPlugin.GetSoundFile().m_MixPlugins[plug.front()->GetSlot()].GetName()));
	else
		outputPlugin = _T("No Plugin");
	m_outputCombo.DeleteString(1);
	m_outputCombo.SetItemData(m_outputCombo.InsertString(1, MPT_CFORMAT("Internal OpenMPT Output ({})")(outputPlugin)), static_cast<DWORD_PTR>(MidiInOut::kInternalDevice));
	m_outputCombo.SetCurSel(sel);
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


void MidiInOutEditor::OnInputChanged()
{
	MidiDevice::ID newDevice = static_cast<MidiDevice::ID>(m_inputCombo.GetItemData(m_inputCombo.GetCurSel()));
	static_cast<MidiInOut &>(m_VstPlugin).OpenDevice(newDevice, true);
}


void MidiInOutEditor::OnOutputChanged()
{
	MidiDevice::ID newDevice = static_cast<MidiDevice::ID>(m_outputCombo.GetItemData(m_outputCombo.GetCurSel()));
	static_cast<MidiInOut &>(m_VstPlugin).OpenDevice(newDevice, false);
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


void MidiInOutEditor::OnMidiDumpChanged()
{
	if(m_locked)
		return;
	if(ValidateMacroString(m_dumpEdit, m_formattedDump, false, false, true))
	{
		CString dumpText;
		m_dumpEdit.GetWindowText(dumpText);
		m_formattedDump = mpt::ToCharset(mpt::Charset::ASCII, dumpText);
		std::vector<uint8> dump;
		dump.reserve(static_cast<size_t>(m_formattedDump.size() / 3));
		bool firstNibble = true;
		uint8 b = 0;
		for(const char c : m_formattedDump)
		{
			if(c >= '0' && c <= '9')
				b |= static_cast<uint8>(c - '0');
			else if(c >= 'A' && c <= 'F')
				b |= static_cast<uint8>(c - 'A' + 0x0A);
			else if(c >= 'a' && c <= 'f')
				b |= static_cast<uint8>(c - 'a' + 0x0A);
			else
				continue;

			if(firstNibble)
			{
				b <<= 4;
			} else
			{
				dump.push_back(b);
				b = 0;
			}
			firstNibble = !firstNibble;
		}
		static_cast<MidiInOut &>(m_VstPlugin).SetInitialMidiDump(std::move(dump));
	}
}


void MidiInOutEditor::OnAlwaysSendDumpChanged()
{
	MidiInOut &plugin = static_cast<MidiInOut &>(m_VstPlugin);
	plugin.m_alwaysSendInitialDump = IsDlgButtonChecked(IDC_CHECK2) == BST_UNCHECKED;
	plugin.SetModified();
}


void MidiInOutEditor::OnLoadMidiDump()
{
	FileDialog dlg = OpenFileDialog()
						 .DefaultExtension(U_("syx"))
						 .ExtensionFilter(U_("SysEx Dumps (*.syx)|*.syx||"));
	if(!dlg.Show(this))
		return;
	mpt::IO::InputFile f(dlg.GetFirstFile());
	if(!f.IsValid())
		return;
	FileReader file = GetFileReader(f);
	std::vector<uint8> dump;
	file.ReadVector(dump, file.GetLength());
	static_cast<MidiInOut &>(m_VstPlugin).SetInitialMidiDump(std::move(dump));
	m_locked = true;
	UpdateMidiDump();
	m_locked = false;
}


void MidiInOutEditor::OnSendDumpNow()
{
	static_cast<MidiInOut &>(m_VstPlugin).m_initialDumpSent = false;
}


void MidiInOutEditor::OnParamChanged()
{
	int i = m_paramCombo.GetCurSel();
	if(i < 0)
		return;
	MidiInOut &plugin = static_cast<MidiInOut &>(m_VstPlugin);
	m_locked = true;
	if(static_cast<size_t>(i) < plugin.m_parameterMacros.size())
		m_paramEdit.SetWindowText(mpt::ToCString(mpt::Charset::ASCII, plugin.m_parameterMacros[i].first));
	else
		m_paramEdit.SetWindowText(_T(""));
	m_locked = false;
}


void MidiInOutEditor::OnMacroChanged()
{
	int i = m_paramCombo.GetCurSel();
	if(m_locked || i < 0)
		return;
	MidiInOut &plugin = static_cast<MidiInOut &>(m_VstPlugin);
	std::string_view prevMacro;
	if(static_cast<size_t>(i) < plugin.m_parameterMacros.size())
		prevMacro = plugin.m_parameterMacros[i].first;
	if(ValidateMacroString(m_paramEdit, prevMacro, true, true, false))
	{
		CString macroText;
		m_paramEdit.GetWindowText(macroText);
		static_cast<MidiInOut &>(m_VstPlugin).SetMacro(static_cast<size_t>(MidiInOut::kMacroParamMin + i), mpt::ToCharset(mpt::Charset::ASCII, macroText));
	}
}


OPENMPT_NAMESPACE_END

#endif // MODPLUG_TRACKER
