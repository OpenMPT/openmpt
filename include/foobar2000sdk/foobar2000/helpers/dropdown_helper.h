#ifndef _DROPDOWN_HELPER_H_
#define _DROPDOWN_HELPER_H_

class _cfg_dropdown_history_base
{
	const unsigned m_max;
	void build_list(pfc::ptr_list_t<char> & out);
	void parse_list(const pfc::ptr_list_t<char> & src);
public:
	enum {separator = '\n'};
	virtual void set_state(const char * val) = 0;
	virtual void get_state(pfc::string_base & out) const = 0;
	_cfg_dropdown_history_base(unsigned p_max) : m_max(p_max) {}
	void on_init(HWND ctrl, const char * initVal) {
		add_item(initVal); setup_dropdown(ctrl); uSetWindowText(ctrl, initVal);
	}
	void setup_dropdown(HWND wnd);
	void setup_dropdown(HWND wnd,UINT id) {setup_dropdown(GetDlgItem(wnd,id));}
	bool add_item(const char * item); //returns true when the content has changed, false when not (the item was already on the list)
	bool add_item(const char * item, HWND combobox); //immediately adds the item to the combobox
	bool is_empty();
	bool on_context(HWND wnd,LPARAM coords); //returns true when the content has changed
};

class cfg_dropdown_history : public _cfg_dropdown_history_base {
public:
	cfg_dropdown_history(const GUID & p_guid,unsigned p_max = 10,const char * init_vals = "") : _cfg_dropdown_history_base(p_max), m_state(p_guid, init_vals) {}
	void set_state(const char * val) {m_state = val;}
	void get_state(pfc::string_base & out) const {out = m_state;}
private:
	cfg_string m_state;
};

class cfg_dropdown_history_mt : public _cfg_dropdown_history_base {
public:
	cfg_dropdown_history_mt(const GUID & p_guid,unsigned p_max = 10,const char * init_vals = "") : _cfg_dropdown_history_base(p_max), m_state(p_guid, init_vals) {}
	void set_state(const char * val) {m_state.set(val);}
	void get_state(pfc::string_base & out) const {m_state.get(out);}
private:
	cfg_string_mt m_state;
};

// ATL-compatible message map entry macro for installing dropdown list context menus.
#define DROPDOWN_HISTORY_HANDLER(ctrlID,var) \
	if(uMsg == WM_CONTEXTMENU) { \
		const HWND source = (HWND) wParam; \
		if (source != NULL && source == CWindow(hWnd).GetDlgItem(ctrlID)) { \
			var.on_context(source,lParam);	\
			lResult = 0;	\
			return TRUE;	\
		}	\
	}

#endif //_DROPDOWN_HELPER_H_
