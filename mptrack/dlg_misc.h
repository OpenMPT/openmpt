#ifndef _MPT_DLG_MISC_H_
#define _MPT_DLG_MISC_H_

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
	UINT m_nChannels;
	MODTYPE m_nType;
	DWORD m_dwSongFlags;

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	CButton m_CheckBox6;
// -! NEW_FEATURE#0023

public:
	CModTypeDlg(CSoundFile *pSndFile, CWnd *parent):CDialog(IDD_MODDOC_MODTYPE, parent) { m_pSndFile = pSndFile; m_nType = MOD_TYPE_NONE; m_nChannels = 0; }
	bool VerifyData();
	void UpdateDialog();

private:
	void UpdateChannelCBox();

protected:
	//{{AFX_VIRTUAL(CModTypeDlg)
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	
	//}}AFX_VIRTUAL

	BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);

	//{{AFX_MSG(CModTypeDlg)
	afx_msg void OnCheck1();
	afx_msg void OnCheck2();
	afx_msg void OnCheck3();
	afx_msg void OnCheck4();
	afx_msg void OnCheck5();
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	afx_msg void OnCheck6();
// -! NEW_FEATURE#0023
	afx_msg void OnCheckPT1x();
	//}}AFX_MSG
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
	bool m_bChnMask[MAX_BASECHANNELS];
	UINT m_nChannels, m_nRemove;
	CListBox m_RemChansList;		//rewbs.removeChansDlgCleanup
	bool m_ShowCancel;

public:
	CRemoveChannelsDlg(CSoundFile *pSndFile, UINT nChns, bool showCancel = true, CWnd *parent=NULL):CDialog(IDD_REMOVECHANNELS, parent)
		{ m_pSndFile = pSndFile; 
		  m_nChannels = m_pSndFile->m_nChannels; 
		  m_nRemove = nChns; 
		  memset(m_bChnMask, false, sizeof(m_bChnMask));
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


////////////////////////////////////////////////////////////////////////
// Midi Macros (Zxx)

#define NMACROS 16

//class CColourEdit;
#include "ColourEdit.h"


//===================================
class CMidiMacroSetup: public CDialog
//===================================
{
public:
	CMidiMacroSetup(MODMIDICFG *pcfg, BOOL bEmbed, CWnd *parent=NULL);
	BOOL m_bEmbed;
	MODMIDICFG m_MidiCfg;


protected:
	CComboBox m_CbnSFx, m_CbnSFxPreset, m_CbnZxx, m_CbnZxxPreset, m_CbnMacroPlug, m_CbnMacroParam, m_CbnMacroCC;
	CEdit m_EditSFx, m_EditZxx;
	CColourEdit m_EditMacroValue[NMACROS], m_EditMacroType[NMACROS]; //rewbs.macroGUI
	CButton m_EditMacro[NMACROS], m_BtnMacroShowAll[NMACROS];
	CSoundFile *m_pSndFile;
	CModDoc *m_pModDoc;

	void UpdateMacroList(int macro=-1);
	void ToggleBoxes(UINT preset, UINT sfx);
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void UpdateDialog();
	afx_msg void OnSetAsDefault();
	afx_msg void OnResetCfg();
	afx_msg void OnEmbedMidiCfg();
	afx_msg void OnSFxChanged();
	afx_msg void OnSFxPresetChanged();
	afx_msg void OnZxxPresetChanged();
	afx_msg void OnSFxEditChanged();
	afx_msg void OnZxxEditChanged();
	afx_msg void OnPlugChanged();
	afx_msg void OnPlugParamChanged();
	afx_msg void OnCCChanged();
	
	afx_msg void OnViewAllParams(UINT id);
	afx_msg void OnSetSFx(UINT id);
	DECLARE_MESSAGE_MAP()
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
	m_nOctaves = nOctaves; m_bCursorNotify = bCursNotify; memset(KeyFlags, 0, sizeof(KeyFlags)); }
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
	WORD KeyboardMap[NOTE_MAX];

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
	ModCompatibilityExportTip		= 0,
	ItCompatibilityExportTip		= 1,
	ConfirmSignUnsignWhenPlaying	= 2,
	XMCompatibilityExportTip		= 3,
	enMsgBoxHidableMessage_count
};

void MsgBoxHidable(enMsgBoxHidableMessage enMsg);

#endif
