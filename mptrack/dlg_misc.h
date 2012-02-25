/*
 * dlg_misc.h
 * ----------
 * Purpose: Implementation for various OpenMPT dialogs.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

class CSoundFile;
class CModDoc;
class CDLSBank;

//===============================
class CModTypeDlg: public CDialog
//===============================
{
public:
	CComboBox m_TypeBox, m_ChannelsBox, m_TempoModeBox, m_PlugMixBox;
	CButton m_CheckBox1, m_CheckBox2, m_CheckBox3, m_CheckBox4, m_CheckBox5, m_CheckBoxPT1x;
	CSoundFile *m_pSndFile;
	CHANNELINDEX m_nChannels;
	MODTYPE m_nType;

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	CButton m_CheckBox6;
// -! NEW_FEATURE#0023

public:
	CModTypeDlg(CSoundFile *pSndFile, CWnd *parent):CDialog(IDD_MODDOC_MODTYPE, parent) { m_pSndFile = pSndFile; m_nType = MOD_TYPE_NONE; m_nChannels = 0; }
	bool VerifyData();
	void UpdateDialog();

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


//===============================
class CShowLogDlg: public CDialog
//===============================
{
public:
	LPCSTR m_lpszLog, m_lpszTitle;
	CEdit m_EditLog;

public:
	CShowLogDlg(CWnd *parent=NULL):CDialog(IDD_SHOWLOG, parent) { m_lpszLog = NULL; m_lpszTitle = NULL; }
	UINT ShowLog(LPCSTR pszLog, LPCSTR lpszTitle=NULL);

protected:
	//{{AFX_VIRTUAL(CShowLogDlg)
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL
	DECLARE_MESSAGE_MAP()
};


//======================================
class CRemoveChannelsDlg: public CDialog
//======================================
{
public:
	CSoundFile *m_pSndFile;
	vector<bool> m_bKeepMask;
	CHANNELINDEX m_nChannels, m_nRemove;
	CListBox m_RemChansList;		//rewbs.removeChansDlgCleanup
	bool m_ShowCancel;

public:
	CRemoveChannelsDlg(CSoundFile *pSndFile, CHANNELINDEX nChns, bool showCancel = true, CWnd *parent=NULL):CDialog(IDD_REMOVECHANNELS, parent)
		{ m_pSndFile = pSndFile; 
		  m_nChannels = m_pSndFile->GetNumChannels(); 
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

public:
	CSoundBankProperties(CDLSBank *pBank, CWnd *parent=NULL);
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
	};
protected:
	HWND m_hParent;
	UINT m_nOctaves;
	int m_nSelection;
	BOOL m_bCapture, m_bCursorNotify;
	BYTE KeyFlags[NOTE_MAX]; // 10 octaves max

public:
	CKeyboardControl() { m_hParent = NULL; m_nOctaves = 1; m_nSelection = -1; m_bCapture = FALSE; }

public:
	void Init(HWND parent, UINT nOctaves=1, BOOL bCursNotify=FALSE) { m_hParent = parent; 
	m_nOctaves = nOctaves; m_bCursorNotify = bCursNotify; MemsetZero(KeyFlags); }
	void SetFlags(UINT key, UINT flags) { if (key < NOTE_MAX) KeyFlags[key] = (BYTE)flags; }
	UINT GetFlags(UINT key) const { return (key < NOTE_MAX) ? KeyFlags[key] : 0; }
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
	CKeyboardControl m_Keyboard;
	CComboBox m_CbnSample;
	CSliderCtrl m_SbOctave;
	CSoundFile *m_pSndFile;
	UINT m_nInstrument;
	SAMPLEINDEX KeyboardMap[NOTE_MAX];

public:
	CSampleMapDlg(CSoundFile *pSndFile, UINT nInstr, CWnd *parent=NULL):CDialog(IDD_EDITSAMPLEMAP, parent)
		{ m_pSndFile = pSndFile; m_nInstrument = nInstr; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual VOID OnOK();
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
	UINT ShowLog(LPCSTR pszLog, LPCSTR lpszTitle=NULL);

protected:
	virtual BOOL OnInitDialog();
	virtual VOID OnOK();
	afx_msg void OnClearHistory();
	DECLARE_MESSAGE_MAP()
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
