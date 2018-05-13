#include "stdafx.h"

#if FOOBAR2000_TARGET_VERSION >= 79

#include "ui_element_helpers.h"
#include "../helpers/IDataObjectUtils.h"
#include "misc.h"

namespace ui_element_helpers {

	ui_element_instance_ptr instantiate_dummy(HWND p_parent,ui_element_config::ptr cfg, ui_element_instance_callback_ptr p_callback) {
		service_ptr_t<ui_element> ptr;
		if (!find(ptr,pfc::guid_null)) uBugCheck();
		return ptr->instantiate(p_parent,cfg,p_callback);
	}
	ui_element_instance_ptr instantiate(HWND p_parent,ui_element_config::ptr cfg,ui_element_instance_callback_ptr p_callback) {
		try {
			service_ptr_t<ui_element> ptr;
			if (!find(ptr,cfg->get_guid())) throw exception_io_data("UI Element Not Found");
			auto ret = ptr->instantiate(p_parent,cfg,p_callback);
			if (ret.is_empty()) throw std::runtime_error("Null UI Element returned");
			return std::move(ret);
		} catch(std::exception const & e) {
			console::complain("UI Element instantiation failure",e);
			return instantiate_dummy(p_parent,cfg,p_callback);
		}
	}

	ui_element_instance_ptr update(ui_element_instance_ptr p_element,HWND p_parent,ui_element_config::ptr cfg,ui_element_instance_callback_ptr p_callback) {
		if (p_element.is_valid() && cfg->get_guid() == p_element->get_guid()) {
			p_element->set_configuration(cfg);
			return p_element;
		} else {
			return instantiate(p_parent,cfg,p_callback);
		}
	}

	bool find(service_ptr_t<ui_element> & p_out,const GUID & p_guid) {
		return service_by_guid(p_out,p_guid);
	}

	ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) {
		service_ptr_t<ui_element> ptr;
		if (!find(ptr,cfg->get_guid())) return NULL;
		try {
			return ptr->enumerate_children(cfg);
		} catch(exception_io_data) {
			return NULL;
		}
	}
};

void ui_element_helpers::replace_with_new_element(ui_element_instance_ptr & p_item,const GUID & p_guid,HWND p_parent,ui_element_instance_callback_ptr p_callback) {
	abort_callback_dummy l_abort;
	ui_element_config::ptr cfg;
	try {
		if (p_item.is_empty()) {
			service_ptr_t<ui_element> ptr;
			if (!find(ptr,p_guid)) throw exception_io_data("UI Element Not Found");
			cfg = ptr->get_default_configuration();
			p_item = ptr->instantiate(p_parent,cfg,p_callback);
		} else if (p_item->get_guid() != p_guid) {
			service_ptr_t<ui_element> ptr;
			if (!find(ptr,p_guid)) throw exception_io_data("UI Element Not Found");
			cfg = ptr->import(p_item->get_configuration());
			//p_item.release();
			if (cfg.is_empty()) cfg = ptr->get_default_configuration();
			p_item = ptr->instantiate(p_parent,cfg,p_callback);
		}
	} catch(std::exception const & e) {
		console::complain("UI Element instantiation failure",e);
		if (cfg.is_empty()) cfg = ui_element_config::g_create_empty();
		p_item = instantiate_dummy(p_parent,cfg,p_callback);
	}
}



namespace {
	class CMyMenuSelectionReceiver : public CMenuSelectionReceiver {
	public:
		CMyMenuSelectionReceiver(HWND p_wnd,ui_element_helpers::ui_element_edit_tools * p_host,ui_element_instance_ptr p_client,unsigned p_client_id,unsigned p_host_base,unsigned p_client_base) : CMenuSelectionReceiver(p_wnd), m_host(p_host), m_client(p_client), m_client_id(p_client_id), m_host_base(p_host_base), m_client_base(p_client_base) {}
		bool QueryHint(unsigned p_id,pfc::string_base & p_out) {
			if (p_id >= m_client_base) {
				return m_client->edit_mode_context_menu_get_description(p_id,m_client_base,p_out);
			} else if (p_id >= m_host_base) {
				return m_host->host_edit_mode_context_menu_get_description(m_client_id,p_id,m_host_base,p_out);
			} else {
				const char * msg = ui_element_helpers::ui_element_edit_tools::description_from_menu_command(p_id);
				if (msg == NULL) return false;
				p_out = msg;
				return true;
			}
		}
	private:
		ui_element_helpers::ui_element_edit_tools * const m_host;
		ui_element_instance_ptr const m_client;
		unsigned const m_client_id,m_host_base,m_client_base;
	};
};

namespace HostHelperIDs {
	enum {ID_LABEL, ID_REPLACE = 1, ID_ADD_NEW, ID_CUT, ID_COPY, ID_PASTE, ID_CUSTOM_BASE};
}

void ui_element_helpers::ui_element_edit_tools::standard_edit_context_menu(LPARAM p_point,ui_element_instance_ptr p_item,unsigned p_id,HWND p_parent) {
	using namespace HostHelperIDs;
	static_api_ptr_t<ui_element_common_methods> api;
	POINT pt;
	bool fromkeyboard = false;
	if (p_point == (LPARAM)(-1)) {
		fromkeyboard = true;
		if (!p_item->edit_mode_context_menu_get_focus_point(pt)) {
			CRect rect;
			WIN32_OP_D( CWindow(p_item->get_wnd()).GetWindowRect(&rect) );
			pt = rect.CenterPoint();
		}
	} else {
		pt.x = (short)LOWORD(p_point);
		pt.y = (short)HIWORD(p_point);
	}
	
	CMenu menu;
	WIN32_OP( menu.CreatePopupMenu() );

	const GUID sourceItemGuid = p_item->get_guid();
	const bool sourceItemEmpty = !!(sourceItemGuid == pfc::guid_null);
	
	if (sourceItemEmpty) {
		WIN32_OP_D( menu.AppendMenu(MF_STRING,ID_ADD_NEW,TEXT(AddNewUIElementCommand)) );
		WIN32_OP_D( menu.SetMenuDefaultItem(ID_ADD_NEW) );
	} else {
		service_ptr_t<ui_element> elem;
		pfc::string8 name;
		if (find(elem,sourceItemGuid)) {
			elem->get_name(name);
		} else {
			name = "<unknown element>";
		}
		WIN32_OP_D( menu.AppendMenu(MF_STRING | MF_DISABLED,ID_LABEL,pfc::stringcvt::string_os_from_utf8(name)) );
		WIN32_OP_D( menu.AppendMenu(MF_SEPARATOR,(UINT_PTR)0,TEXT("")) );
		WIN32_OP_D( menu.AppendMenu(MF_STRING,ID_REPLACE,TEXT(ReplaceUIElementCommand)) );
	}
	WIN32_OP_D( menu.AppendMenu(MF_SEPARATOR,(UINT_PTR)0,TEXT("")) );
	
	//menu.AppendMenu(MF_STRING,ID_REPLACE,TEXT(ReplaceUIElementCommand));
	WIN32_OP_D( menu.AppendMenu(MF_STRING | (sourceItemEmpty ? (MF_DISABLED|MF_GRAYED) : 0),ID_CUT,TEXT(CutUIElementCommand)) );
	WIN32_OP_D( menu.AppendMenu(MF_STRING | (sourceItemEmpty ? (MF_DISABLED|MF_GRAYED) : 0),ID_COPY,TEXT(CopyUIElementCommand)) );
	WIN32_OP_D( menu.AppendMenu(MF_STRING | (api->is_paste_available() ? 0 : (MF_DISABLED|MF_GRAYED)),ID_PASTE,TEXT(PasteUIElementCommand)) );

	unsigned custom_walk = ID_CUSTOM_BASE;
	unsigned custom_base_host = ~0, custom_base_client = ~0;

	if (host_edit_mode_context_menu_test(p_id,pt,fromkeyboard)) {
		menu.AppendMenu(MF_SEPARATOR,(UINT_PTR)0,TEXT(""));
		custom_base_host = custom_walk;
		host_edit_mode_context_menu_build(p_id,pt,fromkeyboard,menu,custom_walk);
	}

	{
		if (p_item->edit_mode_context_menu_test(pt,fromkeyboard)) {
			WIN32_OP_D( menu.AppendMenu(MF_SEPARATOR,(UINT_PTR)0,TEXT("")) );
			custom_base_client = custom_walk;
			p_item->edit_mode_context_menu_build(pt,fromkeyboard,menu,custom_walk);
		}
	}
	int cmd;
	
	{
		ui_element_highlight_scope s(p_item->get_wnd());
		CMyMenuSelectionReceiver receiver(p_parent,this,p_item,p_id,custom_base_host,custom_base_client);
		cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,pt.x,pt.y,receiver);
	}

	if (cmd > 0) {
		switch(cmd) {
		case ID_REPLACE:
		case ID_ADD_NEW:
			replace_dialog(p_item->get_wnd(),p_id,p_item->get_guid());
			break;
		case ID_COPY:
			api->copy(p_item);
			break;
		case ID_CUT:
			api->copy(p_item);
			p_item.release();
			host_replace_element(p_id,pfc::guid_null);
			break;
		case ID_PASTE:
			host_paste_element(p_id);
			break;
		default:
			if ((unsigned)cmd >= custom_base_client) {
				p_item->edit_mode_context_menu_command(pt,fromkeyboard,(unsigned)cmd,custom_base_client);
			} else if ((unsigned)cmd >= custom_base_host) {
				host_edit_mode_context_menu_command(p_id,pt,fromkeyboard,(unsigned)cmd,custom_base_host);
			}
			break;
		}
	}
}

void ui_element_helpers::ui_element_edit_tools::on_elem_replace(unsigned p_id, GUID const & newGuid) {
	m_replace_dialog.Detach();
	
	if ( newGuid != pfc::guid_null ) {
		host_replace_element(p_id,newGuid);
	}
}

void ui_element_helpers::ui_element_edit_tools::replace_dialog(HWND p_parent,unsigned p_id,const GUID & p_current) {
	_release_replace_dialog();

	auto ks = m_killSwitch;

	auto reply = ui_element_replace_dialog_notify::create( [=] ( GUID newGUID ) {
		if ( !*ks) {
			on_elem_replace(p_id, newGUID );
		}
	} );

	HWND dlg = ui_element_common_methods_v3::get()->replace_element_dialog_start( p_parent, p_current, reply );

	m_replace_dialog.Attach( dlg );
}

const char * ui_element_helpers::ui_element_edit_tools::description_from_menu_command(unsigned p_id) {
	using namespace HostHelperIDs;
	switch(p_id) {
	case ID_REPLACE:
		return ReplaceUIElementDescription;
	case ID_CUT:
		return CutUIElementDescription;
	case ID_COPY:
		return CopyUIElementDescription;
	case ID_PASTE:
		return PasteUIElementDescription;
	case ID_ADD_NEW:
		return AddNewUIElementDescription;
	default:
		return NULL;
	}
}

void ui_element_helpers::ui_element_edit_tools::_release_replace_dialog() {
	if (m_replace_dialog.m_hWnd != NULL) {
		m_replace_dialog.DestroyWindow();
	}
}

bool ui_element_helpers::enforce_min_max_info(CWindow p_window,ui_element_min_max_info const & p_info) {
	CRect rect;
	WIN32_OP_D( p_window.GetWindowRect(&rect) );
	t_uint32 width = (t_uint32) rect.Width();
	t_uint32 height = (t_uint32) rect.Height();
	bool changed = false;
	if (width < p_info.m_min_width) {changed = true; width = p_info.m_min_width;}
	if (width > p_info.m_max_width) {changed = true; width = p_info.m_max_width;}
	if (height < p_info.m_min_height) {changed = true; height = p_info.m_min_height;}
	if (height > p_info.m_max_height) {changed = true; height = p_info.m_max_height;}
	if (changed) {
		WIN32_OP_D( p_window.SetWindowPos(NULL,0,0,width,height,SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER) );
	}
	return changed;
}



void ui_element_helpers::handle_WM_GETMINMAXINFO(LPARAM p_lp,const ui_element_min_max_info & p_myinfo) {
	MINMAXINFO * info = reinterpret_cast<MINMAXINFO *>(p_lp);
	/*console::formatter() << "handle_WM_GETMINMAXINFO";
	console::formatter() << p_myinfo.m_min_width << ", " << p_myinfo.m_min_height;
	console::formatter() << info->ptMinTrackSize << ", " << info->ptMaxTrackSize;*/
	pfc::max_acc(info->ptMinTrackSize.x,(LONG)p_myinfo.m_min_width);
	pfc::max_acc(info->ptMinTrackSize.y,(LONG)p_myinfo.m_min_height);
	if ((LONG) p_myinfo.m_max_width >= 0) pfc::min_acc(info->ptMaxTrackSize.x, (LONG) p_myinfo.m_max_width);
	if ((LONG) p_myinfo.m_max_height >= 0) pfc::min_acc(info->ptMaxTrackSize.y, (LONG) p_myinfo.m_max_height);
	//console::formatter() << info->ptMinTrackSize << ", " << info->ptMaxTrackSize;
}

bool ui_element_helpers::ui_element_edit_tools::host_paste_element(unsigned p_id) {
	pfc::com_ptr_t<IDataObject> obj;
	if (SUCCEEDED(OleGetClipboard(obj.receive_ptr()))) {
		DWORD effect;
		ui_element_config::ptr cfg;
		if (static_api_ptr_t<ui_element_common_methods>()->parse_dataobject(obj,cfg,effect)) {
			host_replace_element(p_id, cfg);
			IDataObjectUtils::SetDataObjectDWORD(obj, RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT), effect);
			IDataObjectUtils::PasteSucceeded(obj,effect);
			if (effect == DROPEFFECT_MOVE) OleSetClipboard(NULL);
			return true;
		}
	}
	return false;
}


bool ui_element_helpers::recurse_for_elem_config(ui_element_config::ptr root, ui_element_config::ptr & out, const GUID & toFind) { 
	const GUID rootID = root->get_guid();
	if (rootID == toFind) {
		out = root; return true;
	}
	ui_element::ptr elem;
	if (!find(elem, rootID)) return false;
	ui_element_children_enumerator::ptr children;
	try {
		children = elem->enumerate_children(root);
	} catch(exception_io_data) {return false;}
	if (children.is_empty()) return false;
	const t_size childrenTotal = children->get_count();
	for(t_size walk = 0; walk < childrenTotal; ++walk) {
		if (recurse_for_elem_config(children->get_item(walk), out, toFind)) return true;
	}
	return false;
}

#endif // FOOBAR2000_TARGET_VERSION >= 79
