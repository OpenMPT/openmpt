/*
 * MidiInOutEditor.h
 * -----------------
 * Purpose: Editor interface for the MidiInOut plugin.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#ifdef MODPLUG_TRACKER

#include "../AbstractVstEditor.h"
#include "../CDecimalSupport.h"

OPENMPT_NAMESPACE_BEGIN

class MidiInOut;

class MidiInOutEditor : public CAbstractVstEditor
{
protected:
	CComboBox m_inputCombo, m_outputCombo;
	CNumberEdit m_latencyEdit;
	CSpinButtonCtrl m_latencySpin;
	bool m_locked = true;

public:

	MidiInOutEditor(MidiInOut &plugin);

	// Refresh current input / output device in GUI
	void SetCurrentDevice(bool asInputDevice, MidiDevice::ID device)
	{
		CComboBox &combo = asInputDevice ? m_inputCombo : m_outputCombo;
		SetCurrentDevice(combo, device);
	}

	bool OpenEditor(CWnd *parent) override;
	bool IsResizable() const override { return false; }
	bool SetSize(int, int) override { return false; }

protected:

	// Update lists of available input / output devices
	static void PopulateList(CComboBox &combo, RtMidi &rtDevice, MidiDevice &midiDevice, bool isInput);
	// Refresh current input / output device in GUI
	void SetCurrentDevice(CComboBox &combo, MidiDevice::ID device);

	void DoDataExchange(CDataExchange* pDX) override;

	afx_msg void OnInputChanged();
	afx_msg void OnOutputChanged();
	afx_msg void OnLatencyChanged();
	afx_msg void OnTimingMessagesChanged();

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END

#endif // MODPLUG_TRACKER
