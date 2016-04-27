/*
 * MidiInOutEditor.h
 * -----------------
 * Purpose: Editor interface for the MidiInOut plugin.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifdef MODPLUG_TRACKER

#include "../../mptrack/AbstractVstEditor.h"
#include <portmidi/pm_common/portmidi.h>

OPENMPT_NAMESPACE_BEGIN


//===============================================
class MidiInOutEditor : public CAbstractVstEditor
//===============================================
{
protected:
	CComboBox m_inputCombo, m_outputCombo;
	CButton m_latencyCheck;

public:

	MidiInOutEditor(IMixPlugin &plugin);

	// Refresh current input / output device in GUI
	void SetCurrentDevice(bool asInputDevice, PmDeviceID device)
	{
		CComboBox &combo = asInputDevice ? m_inputCombo : m_outputCombo;
		SetCurrentDevice(combo, device);
	}

	virtual bool OpenEditor(CWnd *parent);
	virtual bool IsResizable() const { return false; }
	virtual bool SetSize(int, int) { return false; }

protected:

	// Update lists of available input / output devices
	void PopulateLists();
	// Refresh current input / output device in GUI
	void SetCurrentDevice(CComboBox &combo, PmDeviceID device);

	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg void OnInputChanged();
	afx_msg void OnOutputChanged();
	afx_msg void OnLatencyToggled();

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END

#endif // MODPLUG_TRACKER
