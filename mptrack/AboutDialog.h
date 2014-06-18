#pragma once

#include "CreditStatic.h"

OPENMPT_NAMESPACE_BEGIN

namespace PNG { struct Bitmap; }

//===============================
class CRippleBitmap: public CWnd
//===============================
{
protected:
	BITMAPINFOHEADER bi;
	PNG::Bitmap *bitmapSrc, *bitmapTarget;
	std::vector<int32_t> offset1, offset2;
	int32_t *frontBuf, *backBuf;
	DWORD lastFrame;	// Time of last frame
	bool frame;			// Backbuffer toggle
	bool damp;			// Ripple damping status
	bool activity;		// There are actually some ripples
	bool showMouse;

public:
	static CRippleBitmap *instance;

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

public:
	static CAboutDlg *instance;

	~CAboutDlg();

	// Implementation
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
};

OPENMPT_NAMESPACE_END
