
class NOVTABLE ui_edit_context : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(ui_edit_context, service_base)
public:
	//! Called by core only.
	virtual void initialize() = 0;
	//! Called by core only. \n
	//! WARNING: you may get other methods called after shutdown() in case someone using ui_edit_context_manager has kept a reference to your service - for an example during an async operation. You should behave sanely in such case - either execute the operation if still possible or fail cleanly.
	virtual void shutdown() = 0;

	enum {
		flag_removable = 1,
		flag_reorderable = 2,
		flag_undoable = 4,
		flag_redoable = 8,
		flag_linearlist = 16,
		flag_searchable = 32,
		flag_insertable = 64,
	};

	virtual t_uint32 get_flags() = 0;

	bool can_remove() {return (get_flags() & flag_removable) != 0;}
	bool test_flags(t_uint32 flags) {return (get_flags() & flags) == flags;}
	bool can_remove_mask() {return test_flags(flag_removable | flag_linearlist);}
	bool can_reorder() {return test_flags(flag_reorderable);}
	bool can_search() {return test_flags(flag_searchable);}

	virtual void select_all() {update_selection(bit_array_true(), bit_array_true());}
	virtual void select_none() {update_selection(bit_array_true(), bit_array_false());}
	virtual void get_selected_items(metadb_handle_list_ref out) {bit_array_bittable mask(get_item_count()); get_selection_mask(mask); get_items(out, mask);}
	virtual void remove_selection() {bit_array_bittable mask(get_item_count()); get_selection_mask(mask); remove_items(mask);}
	virtual void crop_selection() {bit_array_bittable mask(get_item_count()); get_selection_mask(mask); remove_items(bit_array_not(mask));}
	virtual void clear() {remove_items(bit_array_true());}
	virtual void get_all_items(metadb_handle_list_ref out) {get_items(out, bit_array_true());}
	virtual GUID get_selection_type() = 0;

	// available if flag_linearlist is set
	virtual void get_selection_mask(pfc::bit_array_var & out) {
		const t_size count = get_item_count(); for(t_size walk = 0; walk < count; ++walk) out.set(walk, is_item_selected(walk));
	}
	virtual void update_selection(const pfc::bit_array & mask, const pfc::bit_array & newVals) = 0;
	virtual t_size get_item_count(t_size max = ~0) = 0;
	virtual metadb_handle_ptr get_item(t_size index) = 0;
	virtual void get_items(metadb_handle_list_ref out, pfc::bit_array const & mask) = 0;
	virtual bool is_item_selected(t_size item) = 0;
	virtual void remove_items(pfc::bit_array const & mask) = 0;
	virtual void reorder_items(const t_size * order, t_size count) = 0;
	virtual t_size get_selection_count(t_size max = ~0) {
		t_size count = 0;
		const t_size total = get_item_count();
		for(t_size walk = 0; walk < total && count < max; ++walk) if (is_item_selected(walk)) ++count;
		return count;
	}

	virtual void search() = 0;

	virtual void undo_backup() = 0;
	virtual void undo_restore() = 0;
	virtual void redo_restore() = 0;
	
	virtual void insert_items(t_size at, metadb_handle_list_cref items, pfc::bit_array const & selection) = 0;

	virtual t_size query_insert_mark() = 0;

	void sort_by_format(const char * spec, bool onlySelection) {
		const t_size count = get_item_count();
		pfc::array_t<t_size> order; order.set_size(count);
		pfc::array_t<t_size> sel_map;
		if (onlySelection) {
			sel_map.set_size(count);
			t_size sel_count = 0;
			for(t_size n=0;n<count;n++) if (is_item_selected(n)) sel_map[sel_count++]=n;
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
				temp.sort_by_format_get_order(onlySelection ? order_temp.get_ptr() : order.get_ptr(),spec,0);
			} else {
				static_api_ptr_t<genrand_service> api; api->seed((unsigned)__rdtsc());
				api->generate_random_order(onlySelection ? order_temp.get_ptr() : order.get_ptr(),temp.get_count());
			}

			if (onlySelection) {
				t_size n,ptr;
				for(n=0,ptr=0;n<count;n++) {
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

};

class ui_edit_context_playlist : public ui_edit_context {
	FB2K_MAKE_SERVICE_INTERFACE(ui_edit_context_playlist, ui_edit_context)
public:
	virtual t_size get_playlist_number() = 0;
};

class NOVTABLE ui_edit_context_manager : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(ui_edit_context_manager)
public:
	virtual t_uint32 set_context(ui_edit_context::ptr context) = 0;
	virtual void unset_context(t_uint32 id) = 0;
	virtual ui_edit_context::ptr get_context() = 0;
	virtual ui_edit_context::ptr create_playlist_context(t_size playlist_number) = 0;
	virtual void disable_autofallback() = 0;
	virtual t_uint32 set_context_active_playlist() = 0;
};

FOOGUIDDECL const GUID ui_edit_context::class_guid = { 0xf9ba651b, 0x52dd, 0x466f, { 0xaa, 0x77, 0xa9, 0x7a, 0x74, 0x98, 0x80, 0x7e } };
FOOGUIDDECL const GUID ui_edit_context_manager::class_guid = { 0x3807f161, 0xaa17, 0x47df, { 0x80, 0xf1, 0xe, 0xfc, 0xd2, 0x19, 0xb7, 0xa1 } };
FOOGUIDDECL const GUID ui_edit_context_playlist::class_guid = { 0x6dec364d, 0x29f2, 0x47c8, { 0xaf, 0x93, 0xbd, 0x35, 0x56, 0x3f, 0xa2, 0x25 } };
