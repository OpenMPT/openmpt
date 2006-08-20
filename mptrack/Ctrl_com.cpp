#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "ctrl_com.h"
#include "view_com.h"

BEGIN_MESSAGE_MAP(CCtrlComments, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlComments)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,	OnCustomKeyMsg)	//rewbs.customKeys
	ON_EN_CHANGE(IDC_EDIT_COMMENTS,		OnCommentsChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CCtrlComments::DoDataExchange(CDataExchange* pDX)
//----------------------------------------------------
{
	CModControlDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCtrlComments)
	DDX_Control(pDX, IDC_EDIT_COMMENTS, m_EditComments);
	//}}AFX_DATA_MAP
}


CCtrlComments::CCtrlComments()
//----------------------------
{
	m_nLockCount = 0;
	m_hFont = NULL;
}


CRuntimeClass *CCtrlComments::GetAssociatedViewClass()
//----------------------------------------------------
{
	return RUNTIME_CLASS(CViewComments);
}


void CCtrlComments::OnActivatePage(LPARAM)
//----------------------------------------
{
	CMainFrame::EnableLowLatencyMode(FALSE);
}


void CCtrlComments::OnDeactivatePage()
//------------------------------------
{
	CModControlDlg::OnDeactivatePage();
}


BOOL CCtrlComments::OnInitDialog()
//--------------------------------
{
	CModControlDlg::OnInitDialog();
	// Initialize comments
	m_hFont = NULL;
	m_EditComments.SetMargins(4, 0);
	UpdateView(HINT_MODTYPE|HINT_MODCOMMENTS|HINT_MPTOPTIONS, NULL);
	m_EditComments.SetFocus();
	m_bInitialized = TRUE;
	return FALSE;
}


void CCtrlComments::RecalcLayout()
//--------------------------------
{
	CRect rcClient, rect;
	int cx0, cy0;
	
	if ((!m_hWnd) || (!m_EditComments.m_hWnd)) return;
	GetClientRect(&rcClient);
	m_EditComments.GetWindowRect(&rect);
	ScreenToClient(&rect);
	cx0 = rect.Width();
	cy0 = rect.Height();
	rect.bottom = rcClient.bottom - 3;
	rect.right = rcClient.right - rect.left;
	if ((rect.right > rect.left) && (rect.bottom > rect.top))
	{
		int cxmax = (CMainFrame::m_dwPatternSetup & PATTERN_LARGECOMMENTS) ? 80*8 : 80*6;
		int cx = rect.Width(), cy = rect.Height();
		if (cx > cxmax) cx = cxmax;
		if ((cx != cx0) || (cy != cy0)) m_EditComments.SetWindowPos(NULL, 0,0, cx, cy, SWP_NOMOVE|SWP_NOZORDER|SWP_DRAWFRAME);
	}
}


void CCtrlComments::UpdateView(DWORD dwHint, CObject *pHint)
//----------------------------------------------------------
{
	if ((pHint == this) || (!m_pSndFile) || (!(dwHint & (HINT_MODCOMMENTS|HINT_MPTOPTIONS|HINT_MODTYPE)))) return;
	if (m_nLockCount) return;
	m_nLockCount++;
	m_EditComments.SetRedraw(FALSE);
	HFONT newfont;
	if (CMainFrame::m_dwPatternSetup & PATTERN_LARGECOMMENTS)
		newfont = CMainFrame::GetLargeFixedFont();
	else
		newfont = CMainFrame::GetFixedFont();
	if (newfont != m_hFont)
	{
		m_hFont = newfont;
		m_EditComments.SendMessage(WM_SETFONT, (WPARAM)newfont);
	}
	m_EditComments.SetSel(0, -1, TRUE);
	m_EditComments.ReplaceSel("");
	if (m_pSndFile->m_lpszSongComments)
	{
		CHAR s[256], *p = m_pSndFile->m_lpszSongComments, c;
		UINT ln = 0;
		while ((c = *p++) != NULL)
		{
			if ((ln >= LINE_LENGTH-1) || (!*p))
			{
				if (((BYTE)c) > ' ') s[ln++] = c;
				c = 0x0D;
			}
			if (c == 0x0D)
			{
				s[ln] = 0x0D;
				s[ln+1] = 0x0A;
				s[ln+2] = 0;
				m_EditComments.SetSel(65000, 65000, TRUE);
				m_EditComments.ReplaceSel(s);
				ln = 0;
			} else
			{
				if (((BYTE)c) < ' ') c = ' ';
				s[ln++] = c;
			}
		}
		m_EditComments.SetSel(0, 0);
		m_EditComments.SetModify(FALSE);
	}
	if (dwHint & HINT_MODTYPE)
	{
		m_EditComments.SetReadOnly((m_pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_S3M)) ? TRUE : FALSE);
	}

	m_EditComments.SetRedraw(TRUE);
	m_nLockCount--;
}

void CCtrlComments::OnCommentsChanged()
//-------------------------------------
{
	CHAR s[256], *oldcomments = NULL;
	
	if ((m_nLockCount) || (!m_pSndFile)
	 || (m_pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_S3M))) return;
	if ((!m_bInitialized) || (!m_EditComments.m_hWnd) || (!m_EditComments.GetModify())) return;
	if (m_pSndFile->m_lpszSongComments)
	{
		oldcomments = m_pSndFile->m_lpszSongComments;
		m_pSndFile->m_lpszSongComments = NULL;
	}
	// Updating comments
	{


		UINT n = m_EditComments.GetLineCount();
		LPSTR p = new char[n*LINE_LENGTH+1];
		p[0] = 0;
		if (!p) return;
		for (UINT i=0; i<n; i++)
		{
			int ln = m_EditComments.GetLine(i, s, LINE_LENGTH);
			if (ln < 0) ln = 0;
			if (ln > LINE_LENGTH-1) ln = LINE_LENGTH-1;
			s[ln] = 0;
			while ((ln > 0) && (((BYTE)s[ln-1]) <= ' ')) s[--ln] = 0;
			if (i+1 < n) strcat(s, "\x0D");
			strcat(p, s);
		}
		UINT len = strlen(p);
		while ((len > 0) && ((p[len-1] == ' ') || (p[len-1] == '\x0D')))
		{
			len--;
			p[len] = 0;
		}
		if (p[0])
			m_pSndFile->m_lpszSongComments = p;
		else
			delete[] p;
		if (oldcomments)
		{
			BOOL bSame = FALSE;
			if ((m_pSndFile->m_lpszSongComments)
			 && (!strcmp(m_pSndFile->m_lpszSongComments, oldcomments))) bSame = TRUE;
			delete[] oldcomments;
			if (bSame) return;
		} else
		{
			if (!m_pSndFile->m_lpszSongComments) return;
		}
		if (m_pModDoc)
		{
			m_EditComments.SetModify(FALSE);
			m_pModDoc->SetModified();
			m_pModDoc->UpdateAllViews(NULL, HINT_MODCOMMENTS, this);
		}
	}
}

//rewbs.customKeys
LRESULT CCtrlComments::OnCustomKeyMsg(WPARAM wParam, LPARAM lParam)
{
	if (wParam == kcNull)
		return NULL;
	//currently no specific custom keys for this context
	return wParam;
}
//end rewbs.customKeys