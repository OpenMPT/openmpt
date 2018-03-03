#pragma once

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

	virtual void select_all();
	virtual void select_none();
	virtual void get_selected_items(metadb_handle_list_ref out);
	virtual void remove_selection();
	virtual void crop_selection();
	virtual void clear();
	virtual void get_all_items(metadb_handle_list_ref out);
	virtual GUID get_selection_type() = 0;

	// available if flag_linearlist is set
	virtual void get_selection_mask(pfc::bit_array_var & out);

	virtual void update_selection(const pfc::bit_array & mask, const pfc::bit_array & newVals) = 0;
	virtual t_size get_item_count(t_size max = ~0) = 0;
	virtual metadb_handle_ptr get_item(t_size index) = 0;
	virtual void get_items(metadb_handle_list_ref out, pfc::bit_array const & mask) = 0;
	virtual bool is_item_selected(t_size item) = 0;
	virtual void remove_items(pfc::bit_array const & mask) = 0;
	virtual void reorder_items(const t_size * order, t_size count) = 0;
	virtual t_size get_selection_count(t_size max = ~0);

	virtual void search() = 0;

	virtual void undo_backup() = 0;
	virtual void undo_restore() = 0;
	virtual void redo_restore() = 0;
	
	virtual void insert_items(t_size at, metadb_handle_list_cref items, pfc::bit_array const & selection) = 0;

	virtual t_size query_insert_mark() = 0;

	void sort_by_format(const char * spec, bool onlySelection);
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
