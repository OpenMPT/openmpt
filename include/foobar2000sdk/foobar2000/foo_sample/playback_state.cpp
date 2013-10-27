#include "stdafx.h"
#include "resource.h"

class CPlaybackStateDemo : public CDialogImpl<CPlaybackStateDemo>, private play_callback_impl_base {
public:
	enum {IDD = IDD_PLAYBACK_STATE};

	BEGIN_MSG_MAP(CPlaybackStateDemo)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_PATTERN, EN_CHANGE, OnPatternChange)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
		COMMAND_HANDLER_EX(IDC_PLAY, BN_CLICKED, OnPlayClicked)
		COMMAND_HANDLER_EX(IDC_PAUSE, BN_CLICKED, OnPauseClicked)
		COMMAND_HANDLER_EX(IDC_STOP, BN_CLICKED, OnStopClicked)
		COMMAND_HANDLER_EX(IDC_PREV, BN_CLICKED, OnPrevClicked)
		COMMAND_HANDLER_EX(IDC_NEXT, BN_CLICKED, OnNextClicked)
		COMMAND_HANDLER_EX(IDC_RAND, BN_CLICKED, OnRandClicked)
		MSG_WM_CONTEXTMENU(OnContextMenu)
	END_MSG_MAP()
private:

	// Playback callback methods.
	void on_playback_starting(play_control::t_track_command p_command,bool p_paused) {update();}
	void on_playback_new_track(metadb_handle_ptr p_track) {update();}
	void on_playback_stop(play_control::t_stop_reason p_reason) {update();}
	void on_playback_seek(double p_time) {update();}
	void on_playback_pause(bool p_state) {update();}
	void on_playback_edited(metadb_handle_ptr p_track) {update();}
	void on_playback_dynamic_info(const file_info & p_info) {update();}
	void on_playback_dynamic_info_track(const file_info & p_info) {update();}
	void on_playback_time(double p_time) {update();}
	void on_volume_change(float p_new_val) {}

	void update();

	void OnPatternChange(UINT, int, CWindow);
	void OnCancel(UINT, int, CWindow);

	void OnPlayClicked(UINT, int, CWindow) {m_playback_control->start();}
	void OnStopClicked(UINT, int, CWindow) {m_playback_control->stop();}
	void OnPauseClicked(UINT, int, CWindow) {m_playback_control->toggle_pause();}
	void OnPrevClicked(UINT, int, CWindow) {m_playback_control->start(playback_control::track_command_prev);}
	void OnNextClicked(UINT, int, CWindow) {m_playback_control->start(playback_control::track_command_next);}
	void OnRandClicked(UINT, int, CWindow) {m_playback_control->start(playback_control::track_command_rand);}
	
	void OnContextMenu(CWindow wnd, CPoint point);

	BOOL OnInitDialog(CWindow, LPARAM);

	titleformat_object::ptr m_script;

	static_api_ptr_t<playback_control> m_playback_control;
};

void CPlaybackStateDemo::OnCancel(UINT, int, CWindow) {
	DestroyWindow();
}

void CPlaybackStateDemo::OnPatternChange(UINT, int, CWindow) {
	m_script.release(); // pattern has changed, force script recompilation
	update();
}

BOOL CPlaybackStateDemo::OnInitDialog(CWindow, LPARAM) {
	update();
	SetDlgItemText(IDC_PATTERN, _T("%codec% | %bitrate% kbps | %samplerate% Hz | %channels% | %playback_time%[ / %length%]$if(%ispaused%, | paused,)"));
	::ShowWindowCentered(*this,GetParent()); // Function declared in SDK helpers.
	return TRUE;
}

void CPlaybackStateDemo::update() {
	if (m_script.is_empty()) {
		pfc::string8 pattern;
		uGetDlgItemText(*this, IDC_PATTERN, pattern);
		static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(m_script, pattern);
	}
	pfc::string_formatter state;
	if (m_playback_control->playback_format_title(NULL, state, m_script, NULL, playback_control::display_level_all)) {
		//Succeeded already.
	} else if (m_playback_control->is_playing()) {
		//Starting playback but not done opening the first track yet.
		state = "Opening...";
	} else {
		state = "Stopped.";
	}
	uSetDlgItemText(*this, IDC_STATE, state);
}

void CPlaybackStateDemo::OnContextMenu(CWindow wnd, CPoint point) {
	try {
		if (wnd == GetDlgItem(IDC_CONTEXTMENU)) {
			
			// handle the context menu key case - center the menu
			if (point == CPoint(-1, -1)) {
				CRect rc;
				WIN32_OP(wnd.GetWindowRect(&rc));
				point = rc.CenterPoint();
			}
			
			metadb_handle_list items;
			
			{ // note: we would normally just use contextmenu_manager::init_context_now_playing(), but we go the "make the list ourselves" route to demonstrate how to invoke the menu for arbitrary items.
				metadb_handle_ptr item;
				if (m_playback_control->get_now_playing(item)) items += item;
			}

			CMenuDescriptionHybrid menudesc(*this); //this class manages all the voodoo necessary for descriptions of our menu items to show in the status bar.

			static_api_ptr_t<contextmenu_manager> api;
			CMenu menu;
			WIN32_OP(menu.CreatePopupMenu());
			enum {
				ID_TESTCMD = 1,
				ID_CM_BASE,
			};
			menu.AppendMenu(MF_STRING, ID_TESTCMD, _T("Test command"));
			menudesc.Set(ID_TESTCMD, "This is a test command.");
			menu.AppendMenu(MF_SEPARATOR);
			
			if (items.get_count() > 0) {
				api->init_context(items, 0);
				api->win32_build_menu(menu, ID_CM_BASE, ~0);
				menudesc.SetCM(api.get_ptr(), ID_CM_BASE, ~0);
			} else {
				menu.AppendMenu(MF_STRING|MF_GRAYED|MF_DISABLED, (UINT)0, _T("No items selected"));
			}
			
			int cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,point.x,point.y,menudesc,0);
			if (cmd > 0) {
				if (cmd >= ID_CM_BASE) {
					api->execute_by_id(cmd - ID_CM_BASE);
				} else switch(cmd) {
					case ID_TESTCMD:
						popup_message::g_show("Blah!", "Test");
						break;
				}
			}
			
		}
	} catch(std::exception const & e) {
		console::complain("Context menu failure", e); //rare
	}
}

void RunPlaybackStateDemo() {
	try {
		// ImplementModelessTracking registers our dialog to receive dialog messages thru main app loop's IsDialogMessage().
		// CWindowAutoLifetime creates the window in the constructor (taking the parent window as a parameter) and deletes the object when the window has been destroyed (through WTL's OnFinalMessage).
		new CWindowAutoLifetime<ImplementModelessTracking<CPlaybackStateDemo> >(core_api::get_main_window());
	} catch(std::exception const & e) {
		popup_message::g_complain("Dialog creation failure", e);
	}
}
