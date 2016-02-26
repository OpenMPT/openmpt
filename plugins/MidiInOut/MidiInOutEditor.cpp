/*
 * MidiInOutEditor.cpp
 * -------------------
 * Purpose: Editor interface for the MidiInOut plugin.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#define MODPLUG_TRACKER
#define OPENMPT_NAMESPACE
#define OPENMPT_NAMESPACE_BEGIN
#define OPENMPT_NAMESPACE_END
#define MPT_ENABLE_THREAD
#define VC_EXTRALEAN
#define NOMINMAX
#ifdef _MSC_VER
#if (_MSC_VER < 1600)
#define nullptr 0
#endif
#endif

#include "MidiInOutEditor.h"
#include "MidiInOut.h"
#include "windows.h"


OPENMPT_NAMESPACE_BEGIN


MidiInOutEditor::MidiInOutEditor(AudioEffect *effect) : AEffEditor(effect)
//------------------------------------------------------------------------
{
	editRect.top = editRect.left = 0;
	editRect.bottom = 130;
	editRect.right = 350;

	editRectDPI.top = editRectDPI.left = 0;
	editRectDPI.bottom = WindowBase::ScaleY(NULL, editRect.bottom);
	editRectDPI.right = WindowBase::ScaleX(NULL, editRect.right);
}


// Retrieve editor size
bool MidiInOutEditor::getRect(ERect **rect)
//-----------------------------------------
{
	if(rect == nullptr)
	{
		return false;
	}

	*rect = &editRectDPI;
	return true;
}


// Retrieve editor size
bool MidiInOutEditor::open(void *ptr)
//-----------------------------------
{
	AEffEditor::open(ptr);
	HWND parent = static_cast<HWND>(ptr);

	Create(parent, editRect, _T("MIDIInputOutputEditor"));

	inputLabel.Create(hwnd, "MIDI Input Device (Sends MIDI data to host):", 10, 5, 330, 20);
	inputCombo.Create(hwnd, 10, 25, 330, 20);

	outputLabel.Create(hwnd, "MIDI Output Device (Receives MIDI data from host):", 10, 55, 330, 20);
	outputCombo.Create(hwnd, 10, 75, 330, 20);

	latencyCheck.Create(hwnd, "Compensate for host output latency", 10, 105, 330, 20);
	MidiInOut *realEffect = static_cast<MidiInOut *>(effect);
	latencyCheck.SetState(realEffect != nullptr ? realEffect->latencyCompensation : true);

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

	Destroy();
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
	MidiInOut *realEffect = static_cast<MidiInOut *>(effect);

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
	MidiInOut *realEffect = static_cast<MidiInOut *>(effect);

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
intptr_t MidiInOutEditor::WindowCallback(int message, void *param1, void *param2)
//-------------------------------------------------------------------------------
{
	MidiInOut *realEffect = static_cast<MidiInOut *>(effect);
	HWND hwnd = static_cast<HWND>(param2);

	if(message == WM_COMMAND && realEffect != nullptr)
	{
		switch(HIWORD(param1))
		{
		case CBN_SELCHANGE:
			if(hwnd == inputCombo.GetHwnd() || hwnd == outputCombo.GetHwnd())
			{
				// A combo box selection has been changed by the user.
				bool isInputBox = hwnd == inputCombo.GetHwnd();
				const ComboBox &combo = isInputBox ? inputCombo : outputCombo;

				// Update device ID and notify plugin.
				PmDeviceID newDevice = reinterpret_cast<PmDeviceID>(combo.GetSelectionData());
				VstInt32 paramID = isInputBox ? MidiInOut::inputParameter : MidiInOut::outputParameter;
				realEffect->beginEdit(paramID);
				realEffect->setParameterAutomated(paramID, realEffect->DeviceIDToParameter(newDevice));
				realEffect->endEdit(paramID);
			}
			return 0;

		case CBN_DROPDOWN:
			if(hwnd == inputCombo.GetHwnd() || hwnd == outputCombo.GetHwnd())
			{
				// Combo box is about to drop down -> dynamically size the dropdown list
				bool isInputBox = hwnd == inputCombo.GetHwnd();
				ComboBox &combo = isInputBox ? inputCombo : outputCombo;
				combo.SizeDropdownList();
			}
			return 0;

		case BN_CLICKED:
			if(hwnd == latencyCheck.GetHwnd())
			{
				// Update latency compensation
				realEffect->CloseDevice(realEffect->outputDevice);
				realEffect->latencyCompensation = latencyCheck.GetState();
				realEffect->OpenDevice(realEffect->outputDevice.index, false);
			}
			return 0;
		}
	}
	return Window::WindowCallback(message, param1, param2);
}


OPENMPT_NAMESPACE_END
