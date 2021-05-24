/*
 * MIDIMappingDialog.h
 * -------------------
 * Purpose: Implementation of OpenMPT's MIDI mapping dialog, for mapping incoming MIDI messages to plugin parameters.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#ifndef NO_PLUGINS

#include "MIDIMapping.h"
#include "CListCtrl.h"


OPENMPT_NAMESPACE_BEGIN

class CSoundFile;
class CMIDIMapper;

class CMIDIMappingDialog : public CDialog
{
public:
	CMIDIMappingDirective m_Setting;

protected:
	CSoundFile &m_sndFile;
	CMIDIMapper &m_rMIDIMapper;
	HWND oldMIDIRecondWnd;

	// Dialog Data
	CComboBox m_ControllerCBox;
	CComboBox m_PluginCBox;
	CComboBox m_PlugParamCBox;
	CComboBox m_ChannelCBox;
	CComboBox m_EventCBox;
	CListCtrlEx m_List;
	CSpinButtonCtrl m_SpinMoveMapping;

	uint8 m_lastCC = uint8_max;

public:
	CMIDIMappingDialog(CWnd *pParent, CSoundFile &rSndfile);
	~CMIDIMappingDialog();

protected:
	void UpdateDialog(int selItem = -1);
	void UpdateEvent();
	void UpdateParameters();
	int InsertItem(const CMIDIMappingDirective &m, int insertAt);
	void SelectItem(int i);

	void SetModified();

	BOOL OnInitDialog() override;
	void DoDataExchange(CDataExchange* pDX) override;	// DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnSelectionChanged(NMHDR *pNMHDR = nullptr, LRESULT *pResult = nullptr);
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	
	afx_msg void OnBnClickedCheckactive();
	afx_msg void OnBnClickedCheckCapture();
	afx_msg void OnCbnSelchangeComboController();
	afx_msg void OnCbnSelchangeComboChannel();
	afx_msg void OnCbnSelchangeComboPlugin();
	afx_msg void OnCbnSelchangeComboParam();
	afx_msg void OnCbnSelchangeComboEvent();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonReplace();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
	afx_msg void OnDeltaposSpinmovemapping(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCheckPatRecord();
};

OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
