#ifndef MPTRACK_OPTIONS_H
#define MPTRACK_OPTIONS_H

class COptionsKeyboard;

// Tab Order
enum {
	OPTIONS_PAGE_GENERAL=0,
	OPTIONS_PAGE_SOUNDCARD,
	OPTIONS_PAGE_PLAYER,
	OPTIONS_PAGE_EQ,
	OPTIONS_PAGE_KEYBOARD,
	OPTIONS_PAGE_COLORS,
	OPTIONS_PAGE_MIDI,
};



//==========================
class CEditKey: public CEdit
//==========================
{
protected:
	UINT m_nId;
	COptionsKeyboard *m_pParent;

public:
	CEditKey() { m_pParent = NULL; }
	void SetParent(COptionsKeyboard *parent, UINT id) { m_pParent = parent; m_nId = id; }
	void SetId(UINT id) { m_nId = id; }
	UINT GetId() const { return m_nId; }

protected:
	virtual BOOL PreTranslateMessage(MSG *pMsg);
	afx_msg void OnKeyDown(UINT nChar, UINT, UINT);
	DECLARE_MESSAGE_MAP()
};


//=========================================
class COptionsGeneral: public CPropertyPage
//=========================================
{
protected:
	CCheckListBox m_CheckList;

public:
	COptionsGeneral():CPropertyPage(IDD_OPTIONS_GENERAL) {}
	void BrowseForFolder(UINT nID);

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void OnOptionSelChanged();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnBrowseSongs()	{ BrowseForFolder(IDC_EDIT1); }
	afx_msg void OnBrowseSamples()	{ BrowseForFolder(IDC_EDIT2); }
	afx_msg void OnBrowseInstruments() { BrowseForFolder(IDC_EDIT3); }
	DECLARE_MESSAGE_MAP();
};


//=====================================
class CNotifyHotKey: public CHotKeyCtrl
//=====================================
{
protected:
	HWND m_hParent;
	UINT m_nCtrlId;

public:
	CNotifyHotKey() { m_hParent = NULL; }
	VOID SetParent(HWND h, UINT nID) { m_hParent = h; m_nCtrlId = nID; }
	virtual BOOL PreTranslateMessage(MSG *pMsg);
};


//==========================================
class COptionsKeyboard: public CPropertyPage
//==========================================
{
protected:
	CNotifyHotKey m_HotKey;
	CListBox m_lbnHotKeys;
	CButton m_bnReset;
	DWORD KeyboardMap[KEYBOARDMAP_LENGTH];
	CEditKey edits[12+2];
	DWORD CustomKeys[MAX_MPTHOTKEYS];
	UINT m_nKeyboardCfg;
	int m_nCurHotKey, m_nCurKeyboard;

public:
	COptionsKeyboard():CPropertyPage(IDD_OPTIONS_KEYBOARD) { m_nKeyboardCfg = 0; }
	BOOL SetKey(UINT nId, UINT nChar, UINT nFlags);
	BOOL ActivateEdit(UINT nId);

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void UpdateDialog();
	afx_msg void OnKeyboardChanged();
	afx_msg void OnOctaveChanged();
	afx_msg void OnHotKeySelChanged();
	afx_msg void OnHotKeyChanged();
	afx_msg void OnHotKeyReset();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	DECLARE_MESSAGE_MAP();
};


//========================================
class COptionsColors: public CPropertyPage
//========================================
{
protected:
	COLORREF CustomColors[MAX_MODCOLORS];
	UINT m_nColorItem;
	CComboBox m_ComboItem;
	CButton m_BtnColor1, m_BtnColor2, m_BtnColor3, m_BtnPreview;
	CStatic m_TxtColor1, m_TxtColor2, m_TxtColor3;
	LPMODPLUGDIB m_pPreviewDib;

public:
	COptionsColors():CPropertyPage(IDD_OPTIONS_COLORS) { m_nColorItem = 0; m_pPreviewDib = NULL; }
	~COptionsColors() { if (m_pPreviewDib) delete m_pPreviewDib; m_pPreviewDib = NULL; }
	void SelectColor(COLORREF *);

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnSetActive();
	afx_msg void OnUpdateDialog();
	afx_msg void OnDrawItem(int nIdCtl, LPDRAWITEMSTRUCT lpdis);
	afx_msg void OnColorSelChanged();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnSelectColor1();
	afx_msg void OnSelectColor2();
	afx_msg void OnSelectColor3();
	afx_msg void OnPresetMPT();
	afx_msg void OnPresetFT2();
	afx_msg void OnPresetIT();
	afx_msg void OnPresetBuzz();
	afx_msg void OnPreviewChanged();
	DECLARE_MESSAGE_MAP();
};


#endif

