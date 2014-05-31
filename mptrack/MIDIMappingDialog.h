/*
 * MIDIMappingDialog.h
 * -------------------
 * Purpose: Implementation of OpenMPT's MIDI mapping dialog, for mapping incoming MIDI messages to plugin parameters.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once
#include "moddoc.h"
#include <vector>
#include "afxwin.h"
#include "afxcmn.h"

OPENMPT_NAMESPACE_BEGIN

// CMIDIMappingDialog dialog

//=======================================
class CMIDIMappingDialog : public CDialog
//=======================================
{
public:
	CMIDIMappingDirective m_Setting;

protected:
	CSoundFile& m_rSndFile;
	CMIDIMapper& m_rMIDIMapper;
	HWND oldMIDIRecondWnd;


	// Dialog Data
	enum { IDD = IDD_MIDIPARAMCONTROL };
	CComboBox m_ControllerCBox;
	CComboBox m_PluginCBox;
	CComboBox m_PlugParamCBox;
	CComboBox m_ChannelCBox;
	CComboBox m_EventCBox;
	CEdit m_EditValue;
	CListBox m_List;
	CSpinButtonCtrl m_SpinMoveMapping;

public:
	CMIDIMappingDialog(CWnd* pParent, CSoundFile& rSndfile);
	~CMIDIMappingDialog();

protected:
	void UpdateDialog();
	void UpdateEvent();
	void UpdateParameters();
	void UpdateString();
	CString CreateListString(const CMIDIMappingDirective& s);

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnLbnSelchangeList1();
	
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
