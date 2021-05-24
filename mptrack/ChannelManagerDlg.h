/*
 * ChannelManagerDlg.h
 * -------------------
 * Purpose: Dialog class for moving, removing, managing channels
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "PatternEditorDialogs.h"

OPENMPT_NAMESPACE_BEGIN

class CModDoc;

class CChannelManagerDlg: public CDialog
{
	enum Tab
	{
		kSoloMute      = 0,
		kRecordSelect  = 1,
		kPluginState   = 2,
		kReorderRemove = 3,
	};

public:

	static CChannelManagerDlg * sharedInstance() { return sharedInstance_; }
	static CChannelManagerDlg * sharedInstanceCreate();
	static void DestroySharedInstance() { delete sharedInstance_; sharedInstance_ = nullptr; }
	void SetDocument(CModDoc *modDoc);
	CModDoc *GetDocument() const { return m_ModDoc; }
	bool IsDisplayed() const;
	void Update(UpdateHint hint, CObject* pHint);
	void Show();
	void Hide();

private:
	static CChannelManagerDlg *sharedInstance_;
	QuickChannelProperties m_quickChannelProperties;

protected:

	enum ButtonAction : uint8
	{
		kUndetermined,
		kAction1,
		kAction2,
	};

	enum MouseButton : uint8
	{
		CM_BT_NONE,
		CM_BT_LEFT,
		CM_BT_RIGHT,
	};

	CChannelManagerDlg();
	~CChannelManagerDlg();

	CHANNELINDEX memory[4][MAX_BASECHANNELS];
	std::array<CHANNELINDEX, MAX_BASECHANNELS> pattern;
	std::bitset<MAX_BASECHANNELS> removed;
	std::bitset<MAX_BASECHANNELS> select;
	std::bitset<MAX_BASECHANNELS> state;
	CRect move[MAX_BASECHANNELS];
	CRect m_drawableArea;
	CModDoc *m_ModDoc = nullptr;
	HBITMAP m_bkgnd = nullptr;
	Tab m_currentTab = kSoloMute;
	int m_downX = 0, m_downY = 0;
	int m_moveX = 0, m_moveY = 0;
	int m_buttonHeight = 0;
	ButtonAction m_buttonAction;
	bool m_leftButton = false;
	bool m_rightButton = false;
	bool m_moveRect = false;
	bool m_show = false;

	bool ButtonHit(CPoint point, CHANNELINDEX *id, CRect *invalidate) const;
	void MouseEvent(UINT nFlags, CPoint point, MouseButton button);
	void ResetState(bool bSelection = true, bool bMove = true, bool bButton = true, bool bInternal = true, bool bOrder = false);
	void ResizeWindow();

	//{{AFX_VIRTUAL(CChannelManagerDlg)
	BOOL OnInitDialog() override;
	//}}AFX_VIRTUAL
	//{{AFX_MSG(CChannelManagerDlg)
	afx_msg void OnApply();
	afx_msg void OnClose();
	afx_msg void OnSelectAll();
	afx_msg void OnInvert();
	afx_msg void OnAction1();
	afx_msg void OnAction2();
	afx_msg void OnStore();
	afx_msg void OnRestore();
	afx_msg void OnTabSelchange(NMHDR*, LRESULT* pResult);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags,CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags,CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags,CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags,CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags,CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags,CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
};

OPENMPT_NAMESPACE_END
