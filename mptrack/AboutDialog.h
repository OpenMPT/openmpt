#pragma once

#include "CreditStatic.h"

OPENMPT_NAMESPACE_BEGIN

namespace PNG { struct Bitmap; }

//===============================
class CRippleBitmap: public CWnd
//===============================
{

public:

	static const DWORD UPDATE_INTERVAL = 15; // milliseconds

protected:

	BITMAPINFOHEADER bi;
	PNG::Bitmap *bitmapSrc, *bitmapTarget;
	std::vector<int32_t> offset1, offset2;
	int32_t *frontBuf, *backBuf;
	DWORD lastFrame;	// Time of last frame
	DWORD lastRipple;	// Time of last added ripple
	bool frame;			// Backbuffer toggle
	bool damp;			// Ripple damping status
	bool activity;		// There are actually some ripples
	bool showMouse;

public:

	CRippleBitmap();
	~CRippleBitmap();
	bool Animate();

protected:
	virtual void OnPaint();
	virtual BOOL OnEraseBkgnd(CDC *) { return TRUE; }

	DECLARE_MESSAGE_MAP()
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnMouseHover(UINT nFlags, CPoint point) { OnMouseMove(nFlags, point); }
	void OnMouseLeave();
};


//=============================
class CAboutDlg: public CDialog
//=============================
{
protected:
	CRippleBitmap m_bmp;
	CCreditStatic m_static;
	CTabCtrl m_Tab;
	CEdit m_TabEdit;
	CButton m_CheckScroll;
	UINT_PTR m_TimerID;
	static const UINT_PTR TIMERID_ABOUT_DEFAULT = 3;

public:
	static CAboutDlg *instance;

	CAboutDlg();
	~CAboutDlg();

	// Implementation
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void OnTabChange(NMHDR *pNMHDR, LRESULT *pResult);
	void OnTimer(UINT_PTR nIDEvent);
	void OnCheckScroll();
	mpt::ustring GetTabText(int tab);
};

OPENMPT_NAMESPACE_END
