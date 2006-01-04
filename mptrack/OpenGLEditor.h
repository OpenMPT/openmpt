#pragma once
#include "globals.h"
#include "OpenGLControl.h"

class CViewPattern;

class COpenGLEditor :
	public CDialog
{
	//DECLARE_DYNAMIC(COpenGLEditor)
public:

	COpenGLEditor(CViewPattern *pViewPattern);
	~COpenGLEditor(void);
	
	void DoClose();
	void OpenEditor(CWnd *parent);
	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();	
	virtual BOOL OnInitDialog();

private:
	CViewPattern* m_pViewPattern;
	COpenGLControl openGLControl;
public:
	afx_msg void OnPaint();
};
