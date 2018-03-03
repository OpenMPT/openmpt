#pragma once

class file_move_helper {
public:
	static bool g_on_deleted(const pfc::list_base_const_t<const char *> & p_files);

	static t_size g_filter_dead_files_sorted_make_mask(pfc::list_base_t<metadb_handle_ptr> & p_data,const pfc::list_base_const_t<const char*> & p_dead,bit_array_var & p_mask);
	static t_size g_filter_dead_files_sorted(pfc::list_base_t<metadb_handle_ptr> & p_data,const pfc::list_base_const_t<const char*> & p_dead);
	static t_size g_filter_dead_files(pfc::list_base_t<metadb_handle_ptr> & p_data,const pfc::list_base_const_t<const char*> & p_dead);
};
