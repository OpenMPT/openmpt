/*
 * MidiInOutEditor.cpp
 * -------------------
 * Purpose: Editor interface for the MidiInOut plugin.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "MidiInOutEditor.h"
#include "MidiInOut.h"
#define VC_EXTRALEAN
#define NOMINMAX
#include "windows.h"


// Retrieve editor size
bool MidiInOutEditor::getRect(ERect **rect)
//-----------------------------------------
{
	if(rect == nullptr)
	{
		return false;
	}

	editRect.top = editRect.left = 0;
	editRect.bottom = 105;
	editRect.right = 350;
	*rect = &editRect;

	return true;
}


// Retrieve editor size
bool MidiInOutEditor::open(void *ptr)
//-----------------------------------
{
	AEffEditor::open(ptr);
	InitializeWindow(ptr);

	HWND parent = static_cast<HWND>(ptr);

	inputLabel.Create(parent, "MIDI Input Device (Sends MIDI data to host):", 10, 5, 330, 20);
	inputCombo.Create(parent, 10, 25, 330, 20);

	outputLabel.Create(parent, "MIDI Output Device (Receives MIDI data from host):", 10, 55, 330, 20);
	outputCombo.Create(parent, 10, 75, 330, 20);

	PopulateLists();

	return true;
}


// Close editor size
void MidiInOutEditor::close()
//---------------------------
{
	inputLabel.Destroy();
	inputCombo.Destroy();

	outputLabel.Destroy();
	outputCombo.Destroy();

	DestroyWindow();
	AEffEditor::close();
}


// Update lists of available input / output devices
void MidiInOutEditor::PopulateLists()
//-----------------------------------
{
	// Clear lists
	inputCombo.ResetContent();
	outputCombo.ResetContent();

	// Add dummy devices
	inputCombo.AddString("<none>", reinterpret_cast<void *>(MidiInOut::noDevice));
	outputCombo.AddString("<none>", reinterpret_cast<void *>(MidiInOut::noDevice));

	int selectInputItem = 0;
	int selectOutputItem = 0;
	MidiInOut *realEffect = dynamic_cast<MidiInOut *>(effect);

	PmDeviceID i = 0;
	const PmDeviceInfo *device;

	// Go through all PortMidi devices
	while((device = Pm_GetDeviceInfo(i)) != nullptr)
	{
		std::string deviceName = std::string(device->name) + std::string(" [") + std::string(device->interf) + std::string("]");
		// We could make use of Pm_GetDefaultInputDeviceID / Pm_GetDefaultOutputDeviceID here, but is it useful to show those actually?

		if(device->input)
		{
			// We can actually receive MIDI data on this device.
			int result = inputCombo.AddString(static_cast<LPCTSTR>(deviceName.c_str()), reinterpret_cast<void *>(i));

			if(realEffect != nullptr && result != -1 && i == realEffect->inputDevice.index)
			{
				selectInputItem = result;
			}
		}

		if(device->output)
		{
			// We can actually output MIDI data on this device.
			int result = outputCombo.AddString(static_cast<LPCTSTR>(deviceName.c_str()), reinterpret_cast<void *>(i));

			if(realEffect != nullptr && result != -1 && i == realEffect->outputDevice.index)
			{
				selectOutputItem = result;
			}
		}

		i++;
	}

	// Set selections to current devices
	inputCombo.SetSelection(selectInputItem);
	outputCombo.SetSelection(selectOutputItem);
}


// Refresh current input / output device in GUI
void MidiInOutEditor::SetCurrentDevice(ComboBox &combo, PmDeviceID device)
//------------------------------------------------------------------------
{
	MidiInOut *realEffect = dynamic_cast<MidiInOut *>(effect);

	if(isOpen() && realEffect != nullptr)
	{
		int items = combo.GetCount();
		for(int i = 0; i < items; i++)
		{
			if(reinterpret_cast<PmDeviceID>(combo.GetItemData(i)) == device)
			{
				combo.SetSelection(i);
				break;
			}
		}
	}
}


// Window processing callback function
void MidiInOutEditor::WindowCallback(int message, void *param1, void *param2)
//---------------------------------------------------------------------------
{
	MidiInOut *realEffect = dynamic_cast<MidiInOut *>(effect);

	if(message == WM_COMMAND && HIWORD(param1) == CBN_SELCHANGE && realEffect != nullptr)
	{
		// A combo box selection has been changed by the user.
		bool isInputBox = reinterpret_cast<HWND>(param2) == inputCombo.GetHwnd();
		const ComboBox &combo = isInputBox ? inputCombo : outputCombo;

		// Update device ID and notify plugin.
		PmDeviceID newDevice = reinterpret_cast<PmDeviceID>(combo.GetSelectionData());
		realEffect->setParameterAutomated(isInputBox ? MidiInOut::inputParameter : MidiInOut::outputParameter, realEffect->DeviceIDToParameter(newDevice));
	}
}
