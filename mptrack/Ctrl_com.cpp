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
#include "Mptrack.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Globals.h"
#include "Ctrl_com.h"
#include "view_com.h"
#include "InputHandler.h"
#include "../soundlib/mod_specifications.h"


OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(CCtrlComments, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlComments)
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
	charWidth = 0;
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
	UINT margin = Util::ScalePixels(4, m_EditComments.m_hWnd);
	m_EditComments.SetMargins(margin, 0);
	UpdateView(CommentHint().ModType());
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
		int cxmax = 80 * charWidth;
		int cx = rect.Width(), cy = rect.Height();
		if (cx > cxmax && cxmax != 0) cx = cxmax;
		if ((cx != cx0) || (cy != cy0)) m_EditComments.SetWindowPos(NULL, 0,0, cx, cy, SWP_NOMOVE|SWP_NOZORDER|SWP_DRAWFRAME);
	}
}


void CCtrlComments::UpdateView(UpdateHint hint, CObject *pHint)
//-------------------------------------------------------------
{
	CommentHint commentHint = hint.ToType<CommentHint>();
	if (pHint == this || !commentHint.GetType()[HINT_MODCOMMENTS | HINT_MPTOPTIONS | HINT_MODTYPE]) return;
	if (m_nLockCount) return;
	m_nLockCount++;

	static FontSetting previousFont;
	FontSetting font = TrackerSettings::Instance().commentsFont;
	// Point size to pixels
	int32_t fontSize = -MulDiv(font.size, m_nDPIy, 720);
	charWidth = (-fontSize + 1) / 2;
	if(previousFont != font)
	{
		previousFont = font;
		CMainFrame::GetCommentsFont() = ::CreateFont(fontSize, charWidth, 0, 0, font.flags[FontSetting::Bold] ? FW_BOLD : FW_NORMAL,
			font.flags[FontSetting::Italic] ? TRUE :FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_RASTER_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			FIXED_PITCH | FF_MODERN, font.name.c_str());
	}
	m_EditComments.SendMessage(WM_SETFONT, (WPARAM)CMainFrame::GetCommentsFont());
	RecalcLayout();

	m_EditComments.SetRedraw(FALSE);
	m_EditComments.SetSel(0, -1, TRUE);
	m_EditComments.ReplaceSel(_T(""));
	if(!m_sndFile.m_songMessage.empty())
	{
		TCHAR s[256], c;
		const char *p = m_sndFile.m_songMessage.c_str();
		UINT ln = 0;
		while ((c = *p++) != NULL)
		{
			if ((ln >= LINE_LENGTH-1) || (!*p))
			{
				if (((uint32)c) > _T(' ')) s[ln++] = c;
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
				if (((uint32)c) < _T(' ')) c = _T(' ');
				s[ln++] = c;
			}
		}
		m_EditComments.SetSel(0, 0);
		m_EditComments.SetModify(FALSE);
	}
	if (commentHint.GetType() & HINT_MODTYPE)
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

	TCHAR s[LINE_LENGTH + 2];

	// Updating comments
	{
		UINT n = m_EditComments.GetLineCount();
		TCHAR *p = new (std::nothrow) TCHAR[n * LINE_LENGTH + 1];
		if (!p)
		{
			return;
		}
		p[0] = 0;

		for (UINT i=0; i<n; i++)
		{
			int ln = m_EditComments.GetLine(i, s, LINE_LENGTH);
			if (ln < 0) ln = 0;
			if (ln > LINE_LENGTH - 1) ln = LINE_LENGTH - 1;
			s[ln] = 0;
			while ((ln > 0) && (((uint32)s[ln-1]) <= _T(' '))) s[--ln] = 0;
			if (i+1 < n)
			{
				size_t l = _tcslen(s);
				s[l++] = SongMessage::InternalLineEnding;
				s[l] = _T('\0');
			}
			_tcscat(p, s);
		}
		size_t len = _tcslen(p);
		while ((len > 0) && ((p[len-1] == ' ') || (p[len - 1] == SongMessage::InternalLineEnding)))
		{
			len--;
			p[len] = 0;
		}

		m_EditComments.SetModify(FALSE);
		if(p != m_sndFile.m_songMessage)
		{
			m_sndFile.m_songMessage.assign(p);
			m_modDoc.SetModified();
			m_modDoc.UpdateAllViews(nullptr, CommentHint(), this);
		}

		delete[] p;
	}
}


BOOL CCtrlComments::PreTranslateMessage(MSG *pMsg)
//------------------------------------------------
{
	if(pMsg->message == WM_KEYDOWN && pMsg->wParam == 'A' && GetKeyState(VK_CONTROL) < 0)
	{
		// Ctrl-A is not handled by multiline edit boxes
		if(::GetFocus() == m_EditComments.m_hWnd)
			m_EditComments.SetSel(0, -1);
	}
	return CModControlDlg::PreTranslateMessage(pMsg);
}


OPENMPT_NAMESPACE_END
