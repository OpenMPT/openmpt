/*
 * MidiInOutEditor.h
 * -----------------
 * Purpose: Editor interface for the MidiInOut plugin.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <vstsdk2.4/public.sdk/source/vst2.x/aeffeditor.h>
#include <portmidi/pm_common/portmidi.h>
#include "../common/Window.h"
#include "../common/Label.h"
#include "../common/ComboBox.h"


//======================================================
class MidiInOutEditor : public AEffEditor, public Window
//======================================================
{
protected:
	ERect editRect;

	Label inputLabel, outputLabel;
	ComboBox inputCombo, outputCombo;

public:

	MidiInOutEditor(AudioEffect *effect) : AEffEditor(effect), Window() { };

	~MidiInOutEditor()
	{
		AEffEditor::~AEffEditor();
		Window::~Window();
	}

	// Retrieve editor size
	virtual bool getRect(ERect **rect);
	// Open editor window
	virtual bool open(void *ptr);
	// Close editor size
	virtual void close();

	// Refresh current input / output device in GUI
	void SetCurrentDevice(bool asInputDevice, PmDeviceID device)
	{
		ComboBox &combo = asInputDevice ? inputCombo : outputCombo;
		SetCurrentDevice(combo, device);
	}

protected:

	// Update lists of available input / output devices
	void PopulateLists();
	// Refresh current input / output device in GUI
	void SetCurrentDevice(ComboBox &combo, PmDeviceID device);

	// Window processing callback function
	virtual void WindowCallback(int message, void *param1, void *param2);
};
