/*
 * ChannelManagerDlg.h
 * -------------------
 * Purpose: Dialog class for moving, removing, managing channels
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

class CModDoc;

//======================================
class CChannelManagerDlg: public CDialog
//======================================
{
public:

	static CChannelManagerDlg * sharedInstance(bool autoCreate = true);
	static void DestroySharedInstance() {delete sharedInstance_; sharedInstance_ = nullptr;}
	void SetDocument(CModDoc *modDoc);
	CModDoc *GetDocument() const { return m_ModDoc; }
	bool IsDisplayed();
	void Update();
	void Show();
	void Hide();

private:
	static CChannelManagerDlg *sharedInstance_;

protected:

	enum ButtonAction
	{
		kUndetermined,
		kAction1,
		kAction2,
	};

	enum MouseButton
	{
		CM_BT_NONE,
		CM_BT_LEFT,
		CM_BT_RIGHT,
	};

	CChannelManagerDlg(void);
	~CChannelManagerDlg(void);

	CHANNELINDEX memory[4][MAX_BASECHANNELS];
	CHANNELINDEX pattern[MAX_BASECHANNELS];
	std::bitset<MAX_BASECHANNELS> removed;
	std::bitset<MAX_BASECHANNELS> select;
	std::bitset<MAX_BASECHANNELS> state;
	CRect move[MAX_BASECHANNELS];
	CRect m_drawableArea;
	CModDoc *m_ModDoc;
	CHANNELINDEX nChannelsOld;
	int currentTab;
	HBITMAP bkgnd;
	int omx, omy;
	int mx, my;
	int buttonHeight;
	ButtonAction m_buttonAction;
	bool rightButton : 1;
	bool leftButton : 1;
	bool moveRect : 1;
	bool show : 1;

	bool ButtonHit(CPoint point, CHANNELINDEX * id, CRect * invalidate);
	void MouseEvent(UINT nFlags,CPoint point, MouseButton button);
	void ResetState(bool bSelection = true, bool bMove = true, bool bButton = true, bool bInternal = true, bool bOrder = false);

	void DrawChannelButton(HDC hdc, LPRECT lpRect, LPCSTR lpszText, bool activate, bool enable, DWORD dwFlags);

	//{{AFX_VIRTUAL(CChannelManagerDlg)
	BOOL OnInitDialog();
	void OnApply();
	void OnClose();
	void OnSelectAll();
	void OnInvert();
	void OnAction1();
	void OnAction2();
	void OnStore();
	void OnRestore();
	//}}AFX_VIRTUAL
	//{{AFX_MSG(CChannelManagerDlg)
	afx_msg void OnTabSelchange(NMHDR*, LRESULT* pResult);
	afx_msg void OnPaint();
	afx_msg void OnActivate(UINT nState,CWnd* pWndOther,BOOL bMinimized);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType,int cx,int cy);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnMouseMove(UINT nFlags,CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags,CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags,CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags,CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags,CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags,CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
public:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
};

OPENMPT_NAMESPACE_END
