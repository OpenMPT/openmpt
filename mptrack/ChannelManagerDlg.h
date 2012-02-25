/*
 * ChannelManagerDlg.h
 * -------------------
 * Purpose: Dialog class for moving, removing, managing channels
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#define CM_BT_LEFT		1
#define CM_BT_RIGHT		2
#define CM_NB_COLS		8
#define CM_BT_HEIGHT	22

//======================================
class CChannelManagerDlg: public CDialog
//======================================
{
public:

	static CChannelManagerDlg * sharedInstance(BOOL autoCreate = TRUE);
	static void DestroySharedInstance() {delete sharedInstance_; sharedInstance_ = NULL;}
	void SetDocument(void * parent);
	BOOL IsOwner(void * ctrl);
	BOOL IsDisplayed(void);
	void Update(void);
	BOOL Show(void);
	BOOL Hide(void);

private:

	static CChannelManagerDlg * sharedInstance_;

protected:

	CChannelManagerDlg(void);
	~CChannelManagerDlg(void);

	CRITICAL_SECTION applying;
	CHANNELINDEX memory[4][MAX_BASECHANNELS];
	CHANNELINDEX pattern[MAX_BASECHANNELS];
	bool removed[MAX_BASECHANNELS];
	bool select[MAX_BASECHANNELS];
	bool state[MAX_BASECHANNELS];
	CRect move[MAX_BASECHANNELS];
	void * parentCtrl;
	bool mouseTracking;
	CHANNELINDEX nChannelsOld;
	bool rightButton;
	bool leftButton;
	int currentTab;
	bool moveRect;
	HBITMAP bkgnd;
	int omx,omy;
	bool show;
	int mx,my;

	bool ButtonHit(CPoint point, CHANNELINDEX * id, CRect * invalidate);
	void MouseEvent(UINT nFlags,CPoint point, BYTE button);
	void ResetState(bool bSelection = true, bool bMove = true, bool bButton = true, bool bInternal = true, bool bOrder = false);

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
	afx_msg LRESULT OnMouseLeave(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnMouseHover(WPARAM wparam, LPARAM lparam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
public:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
};
