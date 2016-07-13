/*
 * dlg_misc.h
 * ----------
 * Purpose: Implementation for various OpenMPT dialogs.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "CDecimalSupport.h"

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;
class CModDoc;
class CDLSBank;

//===============================
class CModTypeDlg: public CDialog
//===============================
{
public:
	CComboBox m_TypeBox, m_ChannelsBox, m_TempoModeBox, m_PlugMixBox;
	CButton m_CheckBox1, m_CheckBox2, m_CheckBox3, m_CheckBox4, m_CheckBox5, m_CheckBoxPT1x, m_CheckBoxFt2VolRamp, m_CheckBoxAmigaLimits;
	HICON m_warnIcon;

	CSoundFile &sndFile;
	TempoSwing m_tempoSwing;
	PlayBehaviourSet m_playBehaviour;
	CHANNELINDEX m_nChannels;
	MODTYPE m_nType;
	bool initialized;

public:
	CModTypeDlg(CSoundFile &sf, CWnd *parent) : CDialog(IDD_MODDOC_MODTYPE, parent), sndFile(sf) { m_nType = MOD_TYPE_NONE; m_nChannels = 0; }
	bool VerifyData();
	void UpdateDialog();
	void OnPTModeChanged();
	void OnTempoModeChanged();
	void OnTempoSwing();
	void OnLegacyPlaybackSettings();
	void OnDefaultBehaviour();

protected:
	void UpdateChannelCBox();
	CString FormatVersionNumber(DWORD version);

protected:
	//{{AFX_VIRTUAL(CModTypeDlg)
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	
	//}}AFX_VIRTUAL

	BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};


//===============================================
class CLegacyPlaybackSettingsDlg : public CDialog
//===============================================
{
protected:
	CCheckListBox m_CheckList;
	PlayBehaviourSet &m_playBehaviour;
	MODTYPE m_modType;

public:
	CLegacyPlaybackSettingsDlg(CWnd *parent, PlayBehaviourSet &playBehaviour, MODTYPE modType)
		: CDialog(IDD_LEGACY_PLAYBACK, parent)
		, m_playBehaviour(playBehaviour)
		, m_modType(modType)
	{ }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	afx_msg void OnSelectDefaults();
	afx_msg void UpdateSelectDefaults();

	DECLARE_MESSAGE_MAP()
};


//===============================
class CShowLogDlg: public CDialog
//===============================
{
public:
	LPCSTR m_lpszLog, m_lpszTitle;

public:
	CShowLogDlg(CWnd *parent = nullptr):CDialog(IDD_SHOWLOG, parent) { m_lpszLog = NULL; m_lpszTitle = NULL; }
	UINT ShowLog(LPCSTR pszLog, LPCSTR lpszTitle=NULL);

protected:
	virtual BOOL OnInitDialog();
};


//======================================
class CRemoveChannelsDlg: public CDialog
//======================================
{
public:
	CSoundFile &sndFile;
	std::vector<bool> m_bKeepMask;
	CHANNELINDEX m_nChannels, m_nRemove;
	CListBox m_RemChansList;		//rewbs.removeChansDlgCleanup
	bool m_ShowCancel;

public:
	CRemoveChannelsDlg(CSoundFile &sf, CHANNELINDEX nChns, bool showCancel = true, CWnd *parent=NULL) : CDialog(IDD_REMOVECHANNELS, parent), sndFile(sf)
	{
		m_nChannels = sndFile.GetNumChannels(); 
		m_nRemove = nChns;
		m_bKeepMask.assign(m_nChannels, true);
		m_ShowCancel = showCancel;
	}

protected:
	//{{AFX_VIRTUAL(CRemoveChannelsDlg)
	virtual void DoDataExchange(CDataExchange* pDX); //rewbs.removeChansDlgCleanup
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_VIRTUAL
	//{{AFX_MSG(CRemoveChannelsDlg)
	afx_msg void OnChannelChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
};


////////////////////////////////////////////////////////////////////////
// Sound Banks

//========================================
class CSoundBankProperties: public CDialog
//========================================
{
protected:
	CHAR m_szInfo[4096];
	mpt::PathString fileName;

public:
	CSoundBankProperties(CDLSBank &bank, CWnd *parent = nullptr);
	virtual BOOL OnInitDialog();
};


/////////////////////////////////////////////////////////////////////////
// Keyboard control

enum
{
	KBDNOTIFY_MOUSEMOVE=0,
	KBDNOTIFY_LBUTTONDOWN,
	KBDNOTIFY_LBUTTONUP,
};

//=================================
class CKeyboardControl: public CWnd
//=================================
{
public:
	enum
	{
		KEYFLAG_NORMAL=0,
		KEYFLAG_REDDOT,
		KEYFLAG_BRIGHTDOT,
		KEYFLAG_MAX
	};
protected:
	HWND m_hParent;
	CFont m_font;
	UINT m_nOctaves;
	int m_nSelection;
	bool m_bCapture, m_bCursorNotify;
	uint8 KeyFlags[NOTE_MAX]; // 10 octaves max
	SAMPLEINDEX m_sampleNum[NOTE_MAX];

public:
	CKeyboardControl() { m_hParent = NULL; m_nOctaves = 1; m_nSelection = -1; m_bCapture = FALSE; }

public:
	void Init(HWND parent, UINT nOctaves=1, bool cursNotify = false);
	void SetFlags(UINT key, uint8 flags) { if (key < NOTE_MAX) KeyFlags[key] = flags; }
	uint8 GetFlags(UINT key) const { return (key < NOTE_MAX) ? KeyFlags[key] : 0; }
	void SetSample(UINT key, SAMPLEINDEX sample) { if (key < NOTE_MAX) m_sampleNum[key] = sample; }
	SAMPLEINDEX GetSample(UINT key) const { return (key < NOTE_MAX) ? m_sampleNum[key] : 0; }
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Sample Map

//=================================
class CSampleMapDlg: public CDialog
//=================================
{
protected:
	enum MouseAction
	{
		mouseUnknown,	// Didn't mouse-down yet
		mouseSet,		// Set selected sample
		mouseUnset,		// Unset (revert to original keymap)
		mouseZero,		// Set to zero
	};

	CKeyboardControl m_Keyboard;
	CComboBox m_CbnSample;
	CSliderCtrl m_SbOctave;
	CSoundFile &sndFile;
	INSTRUMENTINDEX m_nInstrument;
	SAMPLEINDEX KeyboardMap[NOTE_MAX];
	MouseAction mouseAction;

public:
	CSampleMapDlg(CSoundFile &sf, INSTRUMENTINDEX nInstr, CWnd *parent=NULL) : CDialog(IDD_EDITSAMPLEMAP, parent), mouseAction(mouseUnknown), sndFile(sf)
		{ m_nInstrument = nInstr; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnUpdateSamples();
	afx_msg void OnUpdateKeyboard();
	afx_msg void OnUpdateOctave();
	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	afx_msg LRESULT OnKeyboardNotify(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Edit history dialog

//===================================
class CEditHistoryDlg: public CDialog
//===================================
{

protected:
	CModDoc *m_pModDoc;

public:
	CEditHistoryDlg(CWnd *parent, CModDoc *pModDoc) : CDialog(IDD_EDITHISTORY, parent) { m_pModDoc = pModDoc; }

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnClearHistory();
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Generic input dialog

//=============================
class CInputDlg: public CDialog
//=============================
{
protected:
	CNumberEdit m_edit;
	CSpinButtonCtrl m_spin;
	CString m_description;
	double m_minValueDbl, m_maxValueDbl;
	int32 m_minValueInt, m_maxValueInt;

public:
	CString resultAsString;
	double resultAsDouble;
	int32 resultAsInt;

public:
	// Initialize text input box
	CInputDlg(CWnd *parent, const TCHAR *desc, const TCHAR *defaultString) : CDialog(IDD_INPUT, parent)
		, m_description(desc)
		, resultAsString(defaultString)
		, m_minValueDbl(0), m_maxValueDbl(0), resultAsDouble(0)
		, m_minValueInt(0), m_maxValueInt(0), resultAsInt(0)
	{ }
	// Initialize numeric input box (float)
	CInputDlg(CWnd *parent, const TCHAR *desc, double minVal, double maxVal, double defaultNumber) : CDialog(IDD_INPUT, parent)
		, m_description(desc)
		, m_minValueDbl(minVal), m_maxValueDbl(maxVal), resultAsDouble(defaultNumber)
		, m_minValueInt(0), m_maxValueInt(0), resultAsInt(0)
	{ }
	CInputDlg(CWnd *parent, const TCHAR *desc, float minVal, float maxVal, float defaultNumber) : CDialog(IDD_INPUT, parent)
		, m_description(desc)
		, m_minValueDbl(minVal), m_maxValueDbl(maxVal), resultAsDouble(defaultNumber)
		, m_minValueInt(0), m_maxValueInt(0), resultAsInt(0)
	{ }
	// Initialize numeric input box (int)
	CInputDlg(CWnd *parent, const TCHAR *desc, int32 minVal, int32 maxVal, int32 defaultNumber) : CDialog(IDD_INPUT, parent)
		, m_description(desc)
		, m_minValueDbl(0), m_maxValueDbl(0), resultAsDouble(0)
		, m_minValueInt(minVal), m_maxValueInt(maxVal), resultAsInt(defaultNumber)
	{ }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
};


/////////////////////////////////////////////////////////////////////////
// Messagebox with 'don't show again'-option.

// Enums for message entries. See dlg_misc.cpp for the array of entries.
enum enMsgBoxHidableMessage
{
	ModSaveHint						= 0,
	ItCompatibilityExportTip		= 1,
	ConfirmSignUnsignWhenPlaying	= 2,
	XMCompatibilityExportTip		= 3,
	CompatExportDefaultWarning		= 4,
	enMsgBoxHidableMessage_count
};

void MsgBoxHidable(enMsgBoxHidableMessage enMsg);

OPENMPT_NAMESPACE_END
