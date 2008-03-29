#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "view_pat.h"
#include ".\opengleditor.h"


//IMPLEMENT_DYNAMIC(COpenGLEditor, CDialog)
COpenGLEditor::COpenGLEditor(CViewPattern *pViewPattern)
{
	m_pViewPattern = pViewPattern;
}

COpenGLEditor::~COpenGLEditor(void)
{
}

void COpenGLEditor::OpenEditor(CWnd *parent)
//---------------------------------------
{
	Create(IDD_EFFECTVISUALIZER, parent);
	ShowWindow(SW_SHOW);
	return;
}

BEGIN_MESSAGE_MAP(COpenGLEditor, CDialog)
	ON_WM_CLOSE()
	ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL COpenGLEditor::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect rect;
	GetDlgItem(IDC_RENDERZONE)->GetWindowRect(rect);
	ScreenToClient(rect);
	openGLControl.Create(rect,this);

	return TRUE;
}


void COpenGLEditor::DoClose()
{
	OnClose();
}


void COpenGLEditor::OnClose()
{
	CDialog::OnClose();
	if (m_hWnd) {
		DestroyWindow();
	}
	m_hWnd = NULL;
}

void COpenGLEditor::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // Gerätekontext für Zeichnen

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Symbol in Client-Rechteck zentrieren
		//int cxIcon = GetSystemMetrics(SM_CXICON);
		//int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		//int x = (rect.Width() - cxIcon + 1) / 2;
		//int y = (rect.Height() - cyIcon + 1) / 2;

		// Symbol zeichnen
		//dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}
