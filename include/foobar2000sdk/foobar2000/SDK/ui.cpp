#include "foobar2000.h"

bool ui_drop_item_callback::g_on_drop(interface IDataObject * pDataObject)
{
	service_enum_t<ui_drop_item_callback> e;
	service_ptr_t<ui_drop_item_callback> ptr;
	if (e.first(ptr)) do {
		if (ptr->on_drop(pDataObject)) return true;
	} while(e.next(ptr));
	return false;
}

bool ui_drop_item_callback::g_is_accepted_type(interface IDataObject * pDataObject, DWORD * p_effect)
{
	service_enum_t<ui_drop_item_callback> e;
	service_ptr_t<ui_drop_item_callback> ptr;
	if (e.first(ptr)) do {
		if (ptr->is_accepted_type(pDataObject,p_effect)) return true;
	} while(e.next(ptr));
	return false;
}

bool user_interface::g_find(service_ptr_t<user_interface> & p_out,const GUID & p_guid)
{
	service_enum_t<user_interface> e;
	service_ptr_t<user_interface> ptr;
	if (e.first(ptr)) do {
		if (ptr->get_guid() == p_guid)
		{
			p_out = ptr;
			return true;
		}
	} while(e.next(ptr));
	return false;
}


// ui_edit_context.h code

void ui_edit_context::select_all() { update_selection(pfc::bit_array_true(), pfc::bit_array_true()); }
void ui_edit_context::select_none() { update_selection(pfc::bit_array_true(), pfc::bit_array_false()); }
void ui_edit_context::get_selected_items(metadb_handle_list_ref out) { pfc::bit_array_bittable mask(get_item_count()); get_selection_mask(mask); get_items(out, mask); }
void ui_edit_context::remove_selection() { pfc::bit_array_bittable mask(get_item_count()); get_selection_mask(mask); remove_items(mask); }
void ui_edit_context::crop_selection() { pfc::bit_array_bittable mask(get_item_count()); get_selection_mask(mask); remove_items(pfc::bit_array_not(mask)); }
void ui_edit_context::clear() { remove_items(pfc::bit_array_true()); }
void ui_edit_context::get_all_items(metadb_handle_list_ref out) { get_items(out, pfc::bit_array_true()); }

void ui_edit_context::get_selection_mask(pfc::bit_array_var & out) {
	const t_size count = get_item_count(); for (t_size walk = 0; walk < count; ++walk) out.set(walk, is_item_selected(walk));
}

t_size ui_edit_context::get_selection_count(t_size max) {
	t_size count = 0;
	const t_size total = get_item_count();
	for (t_size walk = 0; walk < total && count < max; ++walk) if (is_item_selected(walk)) ++count;
	return count;
}

void ui_edit_context::sort_by_format(const char * spec, bool onlySelection) {
	const t_size count = get_item_count();
	pfc::array_t<t_size> order; order.set_size(count);
	pfc::array_t<t_size> sel_map;
	if (onlySelection) {
		sel_map.set_size(count);
		t_size sel_count = 0;
		for (t_size n = 0; n<count; n++) if (is_item_selected(n)) sel_map[sel_count++] = n;
		sel_map.set_size(sel_count);
	}

	{
		metadb_handle_list temp;
		pfc::array_t<t_size> order_temp;
		if (onlySelection) {
			get_selected_items(temp);
			order_temp.set_size(count);
		} else {
			get_all_items(temp);
		}


		if (spec != NULL) {
			temp.sort_by_format_get_order(onlySelection ? order_temp.get_ptr() : order.get_ptr(), spec, 0);
		} else {
			auto api = genrand_service::get(); api->seed((unsigned)__rdtsc());
			api->generate_random_order(onlySelection ? order_temp.get_ptr() : order.get_ptr(), temp.get_count());
		}

		if (onlySelection) {
			t_size n, ptr;
			for (n = 0, ptr = 0; n<count; n++) {
				if (!is_item_selected(n)) {
					order[n] = n;
				} else {
					t_size v = order_temp[ptr++];
					order[n] = sel_map[v];
				}
			}
		}
	}

	reorder_items(order.get_ptr(), count);
}
