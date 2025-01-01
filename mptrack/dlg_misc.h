/*
 * dlg_misc.h
 * ----------
 * Purpose: Implementation for various OpenMPT dialogs.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "CDecimalSupport.h"
#include "DialogBase.h"
#include "ResizableDialog.h"
#include "../soundlib/Sndfile.h"

OPENMPT_NAMESPACE_BEGIN

class Version;
class CModDoc;
class CDLSBank;

class CModTypeDlg : public DialogBase
{
protected:
	CComboBox m_TypeBox, m_ChannelsBox, m_TempoModeBox, m_PlugMixBox;
	CButton m_CheckBox1, m_CheckBox2, m_CheckBox3, m_CheckBox4, m_CheckBox5, m_CheckBoxPT1x, m_CheckBoxFt2VolRamp, m_CheckBoxAmigaLimits;
	HICON m_warnIcon = nullptr;

	CSoundFile &sndFile;
public:
	TempoSwing m_tempoSwing;
	PlayBehaviourSet m_playBehaviour;
	CHANNELINDEX m_nChannels = 0;
	MODTYPE m_nType = MOD_TYPE_NONE;
	bool m_showWarning = false;
	bool m_initialized = false;

public:
	CModTypeDlg(CSoundFile &sf, CWnd *parent);
	bool VerifyData();
	void UpdateDialog();
	void OnPTModeChanged();
	void OnTempoModeChanged();
	void OnTempoSwing();
	void OnLegacyPlaybackSettings();
	void OnDefaultBehaviour();

protected:
	void UpdateChannelCBox();
	CString FormatVersionNumber(Version version);

protected:
	//{{AFX_VIRTUAL(CModTypeDlg)
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	void OnDPIChanged() override;
	void OnOK() override;
	CString GetToolTipText(UINT id, HWND HWND) const override;
	//}}AFX_VIRTUAL

	DECLARE_MESSAGE_MAP()
};


class CLegacyPlaybackSettingsDlg : public ResizableDialog
{
protected:
	CCheckListBox m_CheckList;
	PlayBehaviourSet m_playBehaviour;
	MODTYPE m_modType;

public:
	CLegacyPlaybackSettingsDlg(CWnd *parent, PlayBehaviourSet &playBehaviour, MODTYPE modType);

	PlayBehaviourSet GetPlayBehaviour() const { return m_playBehaviour; }

protected:
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;

	afx_msg void OnSelectDefaults();
	afx_msg void UpdateSelectDefaults();
	afx_msg void OnFilterStringChanged();

	DECLARE_MESSAGE_MAP()
};


class CRemoveChannelsDlg : public DialogBase
{
public:
	CSoundFile &sndFile;
	std::vector<bool> m_bKeepMask;
	CHANNELINDEX m_nRemove;
	CListBox m_RemChansList;
	bool m_ShowCancel;

public:
	CRemoveChannelsDlg(CSoundFile &sf, CHANNELINDEX toRemove, bool showCancel = true, CWnd *parent = nullptr);

protected:
	//{{AFX_VIRTUAL(CRemoveChannelsDlg)
	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;
	//}}AFX_VIRTUAL
	//{{AFX_MSG(CRemoveChannelsDlg)
	afx_msg void OnChannelChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
};


class InfoDialog : protected ResizableDialog
{
private:
	mpt::winstring m_caption, m_content;

public:
	InfoDialog(CWnd *parent = nullptr);
	void SetCaption(mpt::winstring caption);
	void SetContent(mpt::winstring content);
	using ResizableDialog::DoModal;

protected:
	BOOL OnInitDialog() override;
};

////////////////////////////////////////////////////////////////////////
// Sound Banks

class CSoundBankProperties : public InfoDialog
{
public:
	CSoundBankProperties(const CDLSBank &bank, CWnd *parent = nullptr);
};


/////////////////////////////////////////////////////////////////////////
// Keyboard control

enum
{
	KBDNOTIFY_MOUSEMOVE=0,
	KBDNOTIFY_LBUTTONDOWN,
	KBDNOTIFY_LBUTTONUP,
};

class CKeyboardControl: public CWnd
{
public:
	enum
	{
		KEYFLAG_NORMAL    = 0x00,
		KEYFLAG_REDDOT    = 0x01,
		KEYFLAG_BRIGHTDOT = 0x02,
	};
protected:
	CWnd *m_parent = nullptr;
	CFont m_font;
	int m_nOctaves = 1;
	int m_nSelection = -1;
	bool m_mouseCapture = false, m_cursorNotify = false;
	bool m_mouseDown = false;

	std::array<uint8, NOTE_MAX> KeyFlags; // 10 octaves max
	std::array<SAMPLEINDEX, NOTE_MAX> m_sampleNum;

public:
	CKeyboardControl() = default;

public:
	void Init(CWnd *parent, int octaves = 1, bool cursorNotify = false);
	void SetFlags(UINT key, uint8 flags) { if (key < NOTE_MAX) KeyFlags[key] = flags; }
	uint8 GetFlags(UINT key) const { return (key < NOTE_MAX) ? KeyFlags[key] : 0; }
	void SetSample(UINT key, SAMPLEINDEX sample) { if (key < NOTE_MAX) m_sampleNum[key] = sample; }
	SAMPLEINDEX GetSample(UINT key) const { return (key < NOTE_MAX) ? m_sampleNum[key] : 0; }

protected:
	void DrawKey(CPaintDC &dc, const CRect rect, int key, bool black) const;

	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Sample Map

class CSampleMapDlg : public DialogBase
{
protected:
	enum class MouseAction
	{
		Unknown,  // Didn't mouse-down yet
		Set,      // Set selected sample
		Unset,    // Unset (revert to original keymap)
		Zero,     // Set to zero
	};

	CKeyboardControl m_Keyboard;
	CComboBox m_CbnSample;
	CSliderCtrl m_SbOctave;

	CSoundFile &sndFile;
	const INSTRUMENTINDEX m_nInstrument;
	std::array<SAMPLEINDEX, NOTE_MAX - NOTE_MIN + 1> KeyboardMap;
	MouseAction mouseAction = MouseAction::Unknown;

public:
	CSampleMapDlg(CSoundFile &sf, INSTRUMENTINDEX nInstr, CWnd *parent = nullptr);

protected:
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;
	afx_msg void OnUpdateSamples();
	afx_msg void OnUpdateKeyboard();
	afx_msg void OnUpdateOctave();
	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	afx_msg LRESULT OnKeyboardNotify(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Edit history dialog

class CEditHistoryDlg: public ResizableDialog
{
protected:
	CModDoc &m_modDoc;

public:
	CEditHistoryDlg(CWnd *parent, CModDoc &modDoc);

protected:
	BOOL OnInitDialog() override;
	afx_msg void OnClearHistory();
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Generic input dialog

class CInputDlg : public DialogBase
{
protected:
	CNumberEdit m_edit;
	CSpinButtonCtrl m_spin;
	const CString m_description;
	const double m_minValueDbl = 0.0, m_maxValueDbl = 0.0;
	const int32 m_minValueInt = 0, m_maxValueInt = 0;
	const int32 m_maxLength = 0;

public:
	int32 resultAsInt = 0;
	double resultAsDouble = 0.0;
	CString resultAsString;

protected:
	CInputDlg(CWnd *parent, const TCHAR *desc, const TCHAR *defaultString, int32 maxLength, double minValDbl, double maxValDbl = 0.0, double defaultDbl = 0.0, int32 minValInt = 0, int32 maxValInt = 0, int32 defaultInt = 0);

public:
	// Initialize text input box
	CInputDlg(CWnd *parent, const TCHAR *desc, const TCHAR *defaultString, int32 maxLength = -1) : CInputDlg{parent, desc, defaultString, maxLength, 0.0} { }
	// Initialize numeric input box (float)
	CInputDlg(CWnd *parent, const TCHAR *desc, double minVal, double maxVal, double defaultNumber) : CInputDlg{parent, desc, {}, -1, minVal, maxVal, defaultNumber} { }
	CInputDlg(CWnd *parent, const TCHAR *desc, float minVal, float maxVal, float defaultNumber) : CInputDlg{parent, desc, {}, -1, minVal, maxVal, defaultNumber } { }
	// Initialize numeric input box (int)
	CInputDlg(CWnd *parent, const TCHAR *desc, int32 minVal, int32 maxVal, int32 defaultNumber) : CInputDlg{parent, desc, {}, -1, 0.0, 0.0, 0.0, minVal, maxVal, defaultNumber } { }

protected:
	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;
};


/////////////////////////////////////////////////////////////////////////
// Messagebox with 'don't show again'-option.

// Enums for message entries. See dlg_misc.cpp for the array of entries.
enum enMsgBoxHidableMessage
{
	ModSaveHint                = 0,
	ItCompatibilityExportTip   = 1,
	XMCompatibilityExportTip   = 2,
	CompatExportDefaultWarning = 3,
	enMsgBoxHidableMessage_count
};

void MsgBoxHidable(enMsgBoxHidableMessage enMsg);

OPENMPT_NAMESPACE_END
