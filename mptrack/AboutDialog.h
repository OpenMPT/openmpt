/*
 * AboutDialog.h
 * -------------
 * Purpose: About dialog with credits, system information and a fancy demo effect.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

class RawGDIDIB;

class CRippleBitmap: public CWnd
{

public:

	static constexpr DWORD UPDATE_INTERVAL = 15; // milliseconds

protected:

	BITMAPINFOHEADER m_bi;
	std::unique_ptr<RawGDIDIB> m_bitmapSrc, m_bitmapTarget;
	std::vector<int32> m_offset1, m_offset2;
	int32 *m_frontBuf, *m_backBuf;
	DWORD m_lastFrame = 0;	// Time of last frame
	DWORD m_lastRipple = 0;	// Time of last added ripple
	bool m_frame = false;		// Backbuffer toggle
	bool m_damp = true;		// Ripple damping status
	bool m_activity = true;	// There are actually some ripples
	bool m_showMouse = true;

public:

	CRippleBitmap();
	~CRippleBitmap();
	bool Animate();

protected:
	void OnPaint();
	BOOL OnEraseBkgnd(CDC *) { return TRUE; }

	void OnMouseMove(UINT nFlags, CPoint point);
	void OnMouseHover(UINT nFlags, CPoint point) { OnMouseMove(nFlags, point); }
	void OnMouseLeave();

	DECLARE_MESSAGE_MAP()
};


class CAboutDlg: public CDialog
{
protected:
	CRippleBitmap m_bmp;
	CTabCtrl m_Tab;
	CEdit m_TabEdit;
	UINT_PTR m_TimerID = 0;
	static constexpr UINT_PTR TIMERID_ABOUT_DEFAULT = 3;

public:
	static CAboutDlg *instance;

	~CAboutDlg();

	// Implementation
protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	void OnCancel() override;
	DECLARE_MESSAGE_MAP();
	void DoDataExchange(CDataExchange* pDX) override;
	afx_msg void OnTabChange(NMHDR *pNMHDR, LRESULT *pResult);
	void OnTimer(UINT_PTR nIDEvent);
public:
	static mpt::ustring GetTabText(int tab);
};

OPENMPT_NAMESPACE_END
