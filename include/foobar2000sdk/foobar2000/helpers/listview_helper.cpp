#include "stdafx.h"

#ifdef _WIN32


namespace listview_helper {

	unsigned insert_item(HWND p_listview,unsigned p_index,const char * p_name,LPARAM p_param)
	{
		if (p_index == ~0) p_index = ListView_GetItemCount(p_listview);
		LVITEM item = {};

		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);

		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = p_index;
		item.lParam = p_param;
		item.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());
		
		LRESULT ret = uSendMessage(p_listview,LVM_INSERTITEM,0,(LPARAM)&item);
		if (ret < 0) return ~0;
		else return (unsigned) ret;
	}

	unsigned insert_item2(HWND p_listview, unsigned p_index, const char * col0, const char * col1, LPARAM p_param) {
		unsigned i = insert_item( p_listview, p_index, col0, p_param );
		if (i != ~0) {
			set_item_text( p_listview, i, 1, col1 );
		}
		return i;
	}

	unsigned insert_item3(HWND p_listview, unsigned p_index, const char * col0, const char * col1, const char * col2, LPARAM p_param) {
		unsigned i = insert_item( p_listview, p_index, col0, p_param );
		if (i != ~0) {
			set_item_text( p_listview, i, 1, col1 );
			set_item_text( p_listview, i, 2, col2 );
		}
		return i;
	}

	unsigned insert_column(HWND p_listview,unsigned p_index,const char * p_name,unsigned p_width_dlu)
	{
		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);

		RECT rect = {0,0,p_width_dlu,0};
		MapDialogRect(GetParent(p_listview),&rect);

		LVCOLUMN data = {};
		data.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		data.fmt = LVCFMT_LEFT;
		data.cx = rect.right;
		data.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());
		
		LRESULT ret = uSendMessage(p_listview,LVM_INSERTCOLUMN,p_index,(LPARAM)&data);
		if (ret < 0) return ~0;
		else return (unsigned) ret;
	}

	void get_item_text(HWND p_listview,unsigned p_index,unsigned p_column,pfc::string_base & p_out) {
		enum {buffer_length = 1024*64};
		pfc::array_t<TCHAR> buffer; buffer.set_size(buffer_length);
		ListView_GetItemText(p_listview,p_index,p_column,buffer.get_ptr(),buffer_length);
		p_out = pfc::stringcvt::string_utf8_from_os(buffer.get_ptr(),buffer_length);
	}

	bool set_item_text(HWND p_listview,unsigned p_index,unsigned p_column,const char * p_name)
	{
		LVITEM item = {};

		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);

		item.mask = LVIF_TEXT;
		item.iItem = p_index;
		item.iSubItem = p_column;
		item.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());
		return uSendMessage(p_listview,LVM_SETITEM,0,(LPARAM)&item) ? true : false;
	}

	bool is_item_selected(HWND p_listview,unsigned p_index)
	{
		LVITEM item = {};
		item.mask = LVIF_STATE;
		item.iItem = p_index;
		item.stateMask = LVIS_SELECTED;
		if (!uSendMessage(p_listview,LVM_GETITEM,0,(LPARAM)&item)) return false;
		return (item.state & LVIS_SELECTED) ? true : false;
	}

	void set_item_selection(HWND p_listview,unsigned p_index,bool p_state)
	{
		PFC_ASSERT( ::IsWindow(p_listview) );
		LVITEM item = {};
		item.stateMask = LVIS_SELECTED;
		item.state = p_state ? LVIS_SELECTED : 0;
		WIN32_OP_D( SendMessage(p_listview,LVM_SETITEMSTATE,(WPARAM)p_index,(LPARAM)&item) );
	}

	bool select_single_item(HWND p_listview,unsigned p_index)
	{
		LRESULT temp = SendMessage(p_listview,LVM_GETITEMCOUNT,0,0);
		if (temp < 0) return false;
		ListView_SetSelectionMark(p_listview,p_index);
		unsigned n; const unsigned m = pfc::downcast_guarded<unsigned>(temp);
		for(n=0;n<m;n++) {
			enum {mask = LVIS_FOCUSED | LVIS_SELECTED};
			ListView_SetItemState(p_listview,n,n == p_index ? mask : 0, mask);
		}
		return ensure_visible(p_listview,p_index);
	}

	bool ensure_visible(HWND p_listview,unsigned p_index)
	{
		return uSendMessage(p_listview,LVM_ENSUREVISIBLE,p_index,FALSE) ? true : false;
	}
}


void ListView_GetContextMenuPoint(HWND p_list,LPARAM p_coords,POINT & p_point,int & p_selection) {
	POINT pt = {(short)LOWORD(p_coords),(short)HIWORD(p_coords)};
	ListView_GetContextMenuPoint(p_list, pt, p_point, p_selection);
}

void ListView_GetContextMenuPoint(HWND p_list,POINT p_coords,POINT & p_point,int & p_selection) {
	if (p_coords.x == -1 && p_coords.y == -1) {
		int firstsel = ListView_GetFirstSelection(p_list);
		if (firstsel >= 0) {
			ListView_EnsureVisible(p_list, firstsel, FALSE);
			RECT rect;
			WIN32_OP_D( ListView_GetItemRect(p_list,firstsel,&rect,LVIR_BOUNDS) );
			p_point.x = (rect.left + rect.right) / 2;
			p_point.y = (rect.top + rect.bottom) / 2;
			WIN32_OP_D( ClientToScreen(p_list,&p_point) );
		} else {
			RECT rect;
			WIN32_OP_D(GetClientRect(p_list,&rect));
			p_point.x = (rect.left + rect.right) / 2;
			p_point.y = (rect.top + rect.bottom) / 2;
			WIN32_OP_D(ClientToScreen(p_list,&p_point));
		}
		p_selection = firstsel;
	} else {
		POINT pt = p_coords; // {(short)LOWORD(p_coords),(short)HIWORD(p_coords)};
		p_point = pt;
		POINT client = pt;
		WIN32_OP_D( ScreenToClient(p_list,&client) );
		LVHITTESTINFO info = {};
		info.pt = client;
		p_selection = ListView_HitTest(p_list,&info);
	}
}

#if 0
static bool ProbeColumn(HWND view, int index) {
	LVCOLUMN col = {LVCF_ORDER};
	return !! ListView_GetColumn(view, index, &col);
}
int ListView_GetColumnCount(HWND listView) {
	if (!ProbeColumn(listView, 0)) return 0;
	int hi = 1;
	for(;;) {
		if (!ProbeColumn(listView, hi)) break;
		hi <<= 1;
		if (hi <= 0) {
			PFC_ASSERT(!"Shouldn't get here!");
			return -1;
		}
	}
	int lo = hi >> 1;
	//lo is the highest known valid column, hi is the lowest known invalid, let's bsearch thru
	while(lo + 1 < hi) {
		PFC_ASSERT( lo < hi );
		const int mid = lo + (hi - lo) / 2;
		PFC_ASSERT( lo < mid && mid < hi );
		if (ProbeColumn(listView, mid)) {
			lo = mid;
		} else {
			hi = mid;
		}
	}
	return hi;
}
#else
int ListView_GetColumnCount(HWND listView) {
	HWND header = ListView_GetHeader(listView);
	PFC_ASSERT(header != NULL);
	return Header_GetItemCount(header);
}
#endif

#endif // _WIN32
