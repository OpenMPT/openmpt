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


//#define MPT_COMMENTS_LONG_LINES_WRAP
//#define MPT_COMMENTS_LONG_LINES_TRUNCATE


#define MPT_COMMENTS_MARGIN 4



OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(CCtrlComments, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlComments)
	ON_EN_UPDATE(IDC_EDIT_COMMENTS,		OnCommentsUpdated)
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
	m_Reformatting = false;
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
	UINT margin = Util::ScalePixels(MPT_COMMENTS_MARGIN, m_EditComments.m_hWnd);
	m_EditComments.SetMargins(margin, margin);
	UpdateView(CommentHint().ModType());
	m_EditComments.SetFocus();
	m_EditComments.FmtLines(FALSE);
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
		int cx = rect.Width(), cy = rect.Height();
		if(m_sndFile.GetModSpecifications().commentLineLengthMax != 0)
		{
			int cxmax = Util::ScalePixels(GetSystemMetrics(SM_CXBORDER) + MPT_COMMENTS_MARGIN + m_sndFile.GetModSpecifications().commentLineLengthMax * charWidth + MPT_COMMENTS_MARGIN + GetSystemMetrics(SM_CXVSCROLL) + GetSystemMetrics(SM_CXBORDER) - 1, m_EditComments.m_hWnd);
			if (cx > cxmax && cxmax != 0) cx = cxmax;
			//SetWindowLong(m_EditComments.m_hWnd, GWL_STYLE, GetWindowLong(m_EditComments.m_hWnd, GWL_STYLE) & ~WS_HSCROLL);
		} else
		{
			//SetWindowLong(m_EditComments.m_hWnd, GWL_STYLE, GetWindowLong(m_EditComments.m_hWnd, GWL_STYLE) | WS_HSCROLL);
		}
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
	if(previousFont != font)
	{
		previousFont = font;
		CMainFrame::GetCommentsFont() = ::CreateFont(fontSize, 0, 0, 0, font.flags[FontSetting::Bold] ? FW_BOLD : FW_NORMAL,
			font.flags[FontSetting::Italic] ? TRUE :FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			FIXED_PITCH | FF_MODERN, font.name.c_str());
	}
	m_EditComments.SendMessage(WM_SETFONT, (WPARAM)CMainFrame::GetCommentsFont());
	CDC * pDC = m_EditComments.GetDC();
	pDC->SelectObject(CMainFrame::GetCommentsFont());
	TEXTMETRIC tm;
	pDC->GetTextMetrics(&tm);
	charWidth = tm.tmAveCharWidth;
	m_EditComments.ReleaseDC(pDC);

	RecalcLayout();

	m_EditComments.SetRedraw(FALSE);

	std::string text = m_sndFile.m_songMessage.GetFormatted(SongMessage::leCRLF);
	for(std::size_t i = 0; i < text.length(); ++i)
	{
		// replace control characters
		char c = text[i];
		if(c > '\0' && c < ' ' && c != '\r' && c != '\n')
		{
			c = ' ';
		}
		text[i] = c;
	}
	CString new_text = text.c_str();
	CString old_text;
	m_EditComments.GetWindowText(old_text);
	if(new_text != old_text)
	{
		m_EditComments.SetWindowText(new_text);
	}

	if(commentHint.GetType() & HINT_MODTYPE)
	{
		m_EditComments.SetReadOnly(!m_sndFile.GetModSpecifications().hasComments);
	}

	m_EditComments.SetRedraw(TRUE);
	m_nLockCount--;
}


void CCtrlComments::OnCommentsUpdated()
//-------------------------------------
{

#if defined(MPT_COMMENTS_LONG_LINES_TRUNCATE) || defined(MPT_COMMENTS_LONG_LINES_WRAP)

	if(m_Reformatting)
	{
		return;
	}

	if(!m_sndFile.GetModSpecifications().hasComments)
	{
		return;
	}
	if(m_sndFile.GetModSpecifications().commentLineLengthMax == 0)
	{
		return;
	}

	m_Reformatting = true;
	const std::size_t maxline = m_sndFile.GetModSpecifications().commentLineLengthMax;
	int beg = 0;
	int end = 0;
	m_EditComments.GetSel(beg, end);
	CString text;
	m_EditComments.GetWindowText(text);
	std::string lines_new;
	lines_new.reserve(text.GetLength());
	bool modified = false;
	std::size_t pos = 0;

#if defined(MPT_COMMENTS_LONG_LINES_WRAP)

	std::string lines = text.GetString();
	std::size_t line_length = 0;
	for(std::size_t i = 0; i < lines.length(); ++i)
	{
		if(lines[i] == '\r')
		{
			// nothing
		} else if (lines[i] == '\n')
		{
			line_length = 0;
		} else
		{
			line_length += 1;
		}
		if(line_length > maxline)
		{
			modified = true;
			lines_new.push_back('\r');
			lines_new.push_back('\n');
			if(beg >= 0)
			{
				if(beg >= pos)
				{
					beg += 2;
				}
			}
			if(end >= 0)
			{
				if(end >= pos)
				{
					end += 2;
				}
			}
			pos += 2;
			line_length = 1;
		}
		lines_new.push_back(lines[i]);
		pos++;
	}

#elif defined(MPT_COMMENTS_LONG_LINES_TRUNCATE)

	std::vector<std::string> lines = mpt::String::Split<std::string>(std::string(text.GetString()), std::string("\r\n"));
	for(std::size_t i = 0; i < lines.size(); ++i)
	{
		if(i > 0)
		{
			pos += 2;
		}
		if(lines[i].length() > maxline)
		{
			modified = true;
			pos += maxline;
			for(std::size_t n = 0; n < lines[i].length() - maxline; ++n)
			{
				if(beg >= 0)
				{
					if(beg > pos)
					{
						beg--;
					}
				}
				if(end >= 0)
				{
					if(end > pos)
					{
						end--;
					}
				}
			}
			lines[i] = lines[i].substr(0, maxline);
		} else
		{
			pos += lines[i].length();
		}
	}
	lines_new = mpt::String::Combine(lines, std::string("\r\n"));

#endif

	if(modified)
	{
		text = lines_new.c_str();
		m_EditComments.SetWindowText(text);
		m_EditComments.SetSel(beg, end);
	}
	m_Reformatting = false;

#endif

}


void CCtrlComments::OnCommentsChanged()
//-------------------------------------
{
	if(m_nLockCount)
		return;
	if ((!m_bInitialized) || (!m_EditComments.m_hWnd) || (!m_EditComments.GetModify())) return;
	CString text;
	m_EditComments.GetWindowText(text);
	m_EditComments.SetModify(FALSE);
	if(m_sndFile.m_songMessage.SetFormatted(text.GetString(), SongMessage::leCRLF))
	{
		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(nullptr, CommentHint(), this);
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
	} else if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB && CMainFrame::GetMainFrame()->GetInputHandler()->GetModifierMask() == 0)
	{
		CString tabs(_T(' '), 4 - (m_EditComments.LineIndex() % 4));
		m_EditComments.ReplaceSel(tabs, TRUE);
		m_EditComments.SetSel(-1, -1, TRUE);
		return TRUE;
	}
	return CModControlDlg::PreTranslateMessage(pMsg);
}


OPENMPT_NAMESPACE_END
