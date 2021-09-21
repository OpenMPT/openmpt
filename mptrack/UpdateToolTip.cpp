/*
 * UpdateToolTip.cpp
 * -----------------
 * Purpose: Implementation of the update tooltip in the main toolbar.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"


OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(UpdateToolTip, CToolTipCtrl)
	ON_WM_LBUTTONUP()
	ON_NOTIFY_REFLECT(TTN_POP, &UpdateToolTip::OnPop)
	ON_NOTIFY_REFLECT(TTN_SHOW, &UpdateToolTip::OnShow)
	ON_NOTIFY_REFLECT(TTN_LINKCLICK, &UpdateToolTip::OnLinkClick)
END_MESSAGE_MAP()


bool UpdateToolTip::ShowUpdate(CWnd &parent, const CString &newVersion, const CString &infoURL, const CRect rectClient, const CPoint ptScreen, const int buttonID)
{
	if(m_hWnd)
		DestroyWindow();
	Create(&parent, TTS_NOPREFIX | TTS_BALLOON | TTS_CLOSE | TTS_NOFADE);

	m_infoURL = infoURL;

	CString message = MPT_CFORMAT("OpenMPT {} has been released.\n<a>Click here to see what's new.</a>")(newVersion);
	TOOLINFO ti{};
	ti.cbSize = TTTOOLINFO_V1_SIZE;
	ti.uFlags = TTF_TRACK | TTF_PARSELINKS;
	ti.hwnd = parent;
	ti.lpszText = message.GetBuffer();
	ti.uId = buttonID;
	ti.rect = rectClient;
	if(!SendMessage(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti)))
		return false;

	SetTitle(TTI_INFO, _T("Update Available"));
	SendMessage(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptScreen.x, ptScreen.y)));
	SendMessage(TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&ti));
	return true;
}


void UpdateToolTip::SetResult(PopAction action)
{
	m_popAction = action;
	SendMessage(TTM_TRACKACTIVATE, FALSE, 0);
	if(action != PopAction::CloseButton)
		CMainFrame::GetMainFrame()->SendMessage(WM_MOD_UPDATENOTIFY, static_cast<WPARAM>(action));
}


void UpdateToolTip::OnLButtonUp(UINT nFlags, CPoint point)
{
	CToolTipCtrl::OnLButtonUp(nFlags, point);
	if(m_popAction == PopAction::Undetermined)
		SetResult(PopAction::ClickBubble);
}


void UpdateToolTip::OnPop(NMHDR *pNMHDR, LRESULT *)
{
	if(pNMHDR->idFrom == UINT_PTR(-1))
		SetResult(PopAction::CloseButton);
}


void UpdateToolTip::OnShow(NMHDR *, LRESULT *)
{
	m_popAction = PopAction::Undetermined;
}


void UpdateToolTip::OnLinkClick(NMHDR *, LRESULT *)
{
	CTrackApp::OpenURL(m_infoURL);
	SetResult(PopAction::ClickLink);
}


OPENMPT_NAMESPACE_END
