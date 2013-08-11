/*
 * ctrl_com.cpp
 * ------------
 * Purpose: Song comments tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


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


CCtrlComments::CCtrlComments(CModControlView &parent, CModDoc &document) : CModControlDlg(parent, document)
//---------------------------------------------------------------------------------------------------------
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
	// Don't stop generating VU meter messages
	m_modDoc.SetNotifications(Notification::Default);
	m_modDoc.SetFollowWnd(m_hWnd);
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
		int cxmax = (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_LARGECOMMENTS) ? 80*8 : 80*6;
		int cx = rect.Width(), cy = rect.Height();
		if (cx > cxmax) cx = cxmax;
		if ((cx != cx0) || (cy != cy0)) m_EditComments.SetWindowPos(NULL, 0,0, cx, cy, SWP_NOMOVE|SWP_NOZORDER|SWP_DRAWFRAME);
	}
}


void CCtrlComments::UpdateView(DWORD dwHint, CObject *pHint)
//----------------------------------------------------------
{
	if ((pHint == this) || (!(dwHint & (HINT_MODCOMMENTS|HINT_MPTOPTIONS|HINT_MODTYPE)))) return;
	if (m_nLockCount) return;
	m_nLockCount++;
	HFONT newfont;
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_LARGECOMMENTS)
		newfont = CMainFrame::GetLargeFixedFont();
	else
		newfont = CMainFrame::GetFixedFont();
	if (newfont != m_hFont)
	{
		m_hFont = newfont;
		m_EditComments.SendMessage(WM_SETFONT, (WPARAM)newfont);
		RecalcLayout();
	}
	m_EditComments.SetRedraw(FALSE);
	m_EditComments.SetSel(0, -1, TRUE);
	m_EditComments.ReplaceSel("");
	if(!m_sndFile.songMessage.empty())
	{
		CHAR s[256], c;
		const char *p = m_sndFile.songMessage.c_str();
		UINT ln = 0;
		while ((c = *p++) != NULL)
		{
			if ((ln >= LINE_LENGTH-1) || (!*p))
			{
				if (((BYTE)c) > ' ') s[ln++] = c;
				c = SongMessage::InternalLineEnding;
			}
			if (c == SongMessage::InternalLineEnding)
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
		m_EditComments.SetReadOnly(!m_sndFile.GetModSpecifications().hasComments);
	}

	m_EditComments.SetRedraw(TRUE);
	m_nLockCount--;
}

void CCtrlComments::OnCommentsChanged()
//-------------------------------------
{
	if ((m_nLockCount)
		|| !m_sndFile.GetModSpecifications().hasComments) return;
	if ((!m_bInitialized) || (!m_EditComments.m_hWnd) || (!m_EditComments.GetModify())) return;

	CHAR s[LINE_LENGTH + 2];

	// Updating comments
	{
		UINT n = m_EditComments.GetLineCount();
		LPSTR p = new char[n * LINE_LENGTH + 1];
		if (!p)
		{
			return;
		}
		p[0] = 0;

		for (UINT i=0; i<n; i++)
		{
			int ln = m_EditComments.GetLine(i, s, LINE_LENGTH);
			if (ln < 0) ln = 0;
			if (ln > LINE_LENGTH-1) ln = LINE_LENGTH-1;
			s[ln] = 0;
			while ((ln > 0) && (((BYTE)s[ln-1]) <= ' ')) s[--ln] = 0;
			if (i+1 < n)
			{
				size_t l = strlen(s);
				s[l++] = SongMessage::InternalLineEnding;
				s[l] = '\0';
			}
			strcat(p, s);
		}
		size_t len = strlen(p);
		while ((len > 0) && ((p[len-1] == ' ') || (p[len - 1] == SongMessage::InternalLineEnding)))
		{
			len--;
			p[len] = 0;
		}

		m_EditComments.SetModify(FALSE);
		if(p != m_sndFile.songMessage)
		{
			m_sndFile.songMessage.assign(p);
			m_modDoc.SetModified();
			m_modDoc.UpdateAllViews(NULL, HINT_MODCOMMENTS, this);
		}

		delete[] p;
	}
}

//rewbs.customKeys
LRESULT CCtrlComments::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
{
	if (wParam == kcNull)
		return NULL;
	//currently no specific custom keys for this context
	return wParam;
}
//end rewbs.customKeys