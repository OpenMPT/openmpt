#pragma once

typedef CWinTraits<WS_POPUP,WS_EX_TRANSPARENT|WS_EX_LAYERED|WS_EX_TOPMOST|WS_EX_TOOLWINDOW> CFlashWindowTraits;

class CFlashWindow : public CWindowImpl<CFlashWindow,CWindow,CFlashWindowTraits> {
public:
	void Activate(CWindow parent) {
		ShowAbove(parent);
		m_tickCount = 0;
		SetTimer(KTimerID, 500);
	}
	void Deactivate() throw() {
		ShowWindow(SW_HIDE); KillTimer(KTimerID);
	}

	void ShowAbove(CWindow parent) {
		if (m_hWnd == NULL) {
			WIN32_OP( Create(NULL) != NULL );
		}
		CRect rect;
		WIN32_OP_D( parent.GetWindowRect(rect) );
		WIN32_OP_D( SetWindowPos(NULL,rect,SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW) );
		m_parent = parent;
	}

	void CleanUp() throw() {
		if (m_hWnd != NULL) DestroyWindow();
	}

	BEGIN_MSG_MAP_EX(CFlashWindow)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_TIMER(OnTimer)
		MSG_WM_DESTROY(OnDestroy)
	END_MSG_MAP()

	DECLARE_WND_CLASS_EX(TEXT("{2E124D52-131F-4004-A569-2316615BE63F}"),0,COLOR_HIGHLIGHT);
private:
	void OnDestroy() throw() {
		KillTimer(KTimerID);
	}
	enum {
		KTimerID = 0x47f42dd0
	};
	void OnTimer(WPARAM id) {
		if (id == KTimerID) {
			switch(++m_tickCount) {
				case 1:
					ShowWindow(SW_HIDE);
					break;
				case 2:
					ShowAbove(m_parent);
					break;
				case 3:
					ShowWindow(SW_HIDE);
					KillTimer(KTimerID);
					break;
			}
		}
	}
	LRESULT OnCreate(LPCREATESTRUCT) throw() {
		SetLayeredWindowAttributes(*this,0,128,LWA_ALPHA);
		return 0;
	}
	CWindow m_parent;
	t_uint32 m_tickCount;
};

