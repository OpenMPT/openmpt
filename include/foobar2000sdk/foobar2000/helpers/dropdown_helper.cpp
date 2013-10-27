#include "stdafx.h"
#include "dropdown_helper.h"


void _cfg_dropdown_history_base::build_list(pfc::ptr_list_t<char> & out)
{
	pfc::string8 temp; get_state(temp);
	const char * src = temp;
	while(*src)
	{
		int ptr = 0;
		while(src[ptr] && src[ptr]!=separator) ptr++;
		if (ptr>0)
		{
			out.add_item(pfc::strdup_n(src,ptr));
			src += ptr;
		}
		while(*src==separator) src++;
	}
}

void _cfg_dropdown_history_base::parse_list(const pfc::ptr_list_t<char> & src)
{
	t_size n;
	pfc::string8_fastalloc temp;
	for(n=0;n<src.get_count();n++)
	{
		temp.add_string(src[n]);
		temp.add_char(separator);		
	}
	set_state(temp);
}

static void g_setup_dropdown_fromlist(HWND wnd,const pfc::ptr_list_t<char> & list)
{
	t_size n, m = list.get_count();
	uSendMessage(wnd,CB_RESETCONTENT,0,0);
	for(n=0;n<m;n++) {
		uSendMessageText(wnd,CB_ADDSTRING,0,list[n]);
	}
}

void _cfg_dropdown_history_base::setup_dropdown(HWND wnd)
{
	pfc::ptr_list_t<char> list;
	build_list(list);
	g_setup_dropdown_fromlist(wnd,list);
	list.free_all();
}

bool _cfg_dropdown_history_base::add_item(const char * item)
{
	if (!item || !*item) return false;
	pfc::string8 meh;
	if (strchr(item,separator))
	{
		uReplaceChar(meh,item,-1,separator,'|',false);
		item = meh;
	}
	pfc::ptr_list_t<char> list;
	build_list(list);
	unsigned n;
	bool found = false;
	for(n=0;n<list.get_count();n++)
	{
		if (!strcmp(list[n],item))
		{
			char* temp = list.remove_by_idx(n);
			list.insert_item(temp,0);
			found = true;
		}
	}

	if (!found)
	{
		while(list.get_count() > m_max) list.delete_by_idx(list.get_count()-1);
		list.insert_item(_strdup(item),0);
	}
	parse_list(list);
	list.free_all();
	return found;
}

bool _cfg_dropdown_history_base::add_item(const char *item, HWND combobox) {
	const bool state = add_item(item);
	if (state) uSendMessageText(combobox, CB_ADDSTRING, 0, item);
	return state;
}

bool _cfg_dropdown_history_base::is_empty()
{
	pfc::string8 temp; get_state(temp);
	const char * src = temp;
	while(*src)
	{
		if (*src!=separator) return false;
		src++;
	}
	return true;
}

bool _cfg_dropdown_history_base::on_context(HWND wnd,LPARAM coords) {
	try {
		int coords_x = (short)LOWORD(coords), coords_y = (short)HIWORD(coords);
		if (coords_x == -1 && coords_y == -1)
		{
			RECT asdf;
			GetWindowRect(wnd,&asdf);
			coords_x = (asdf.left + asdf.right) / 2;
			coords_y = (asdf.top + asdf.bottom) / 2;
		}
		enum {ID_ERASE_ALL = 1, ID_ERASE_ONE };
		HMENU menu = CreatePopupMenu();
		uAppendMenu(menu,MF_STRING,ID_ERASE_ALL,"Wipe history");
		{
			pfc::string8 tempvalue;
			uGetWindowText(wnd,tempvalue);
			if (!tempvalue.is_empty())
				uAppendMenu(menu,MF_STRING,ID_ERASE_ONE,"Remove this history item");
		}
		int cmd = TrackPopupMenu(menu,TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,coords_x,coords_y,0,wnd,0);
		DestroyMenu(menu);
		switch(cmd)
		{
		case ID_ERASE_ALL:
			{
				set_state("");
				pfc::string8 value;//preserve old value while wiping dropdown list
				uGetWindowText(wnd,value);
				uSendMessage(wnd,CB_RESETCONTENT,0,0);
				uSetWindowText(wnd,value);
				return true;
			}
		case ID_ERASE_ONE:
			{
				pfc::string8 value;
				uGetWindowText(wnd,value);

				pfc::ptr_list_t<char> list;
				build_list(list);
				bool found = false;
				for(t_size n=0;n<list.get_size();n++)
				{
					if (!strcmp(value,list[n]))
					{
						free(list[n]);
						list.remove_by_idx(n);
						found = true;
						break;
					}
				}
				if (found) parse_list(list);
				g_setup_dropdown_fromlist(wnd,list);
				list.free_all();
				return found;
			}
		}
	} catch(...) {}
	return false;
}
