#include "stdafx.h"

#include "file_move_helper.h"

bool file_move_helper::g_on_deleted(const pfc::list_base_const_t<const char *> & p_files)
{
	file_operation_callback::g_on_files_deleted(p_files);
	return true;
}

t_size file_move_helper::g_filter_dead_files_sorted_make_mask(pfc::list_base_t<metadb_handle_ptr> & p_data,const pfc::list_base_const_t<const char*> & p_dead,bit_array_var & p_mask)
{
	t_size n, m = p_data.get_count();
	t_size found = 0;
	for(n=0;n<m;n++)
	{
		t_size dummy;
		bool dead = p_dead.bsearch_t(metadb::path_compare,p_data.get_item(n)->get_path(),dummy);
		if (dead) found++;
		p_mask.set(n,dead);
	}
	return found;
}

t_size file_move_helper::g_filter_dead_files_sorted(pfc::list_base_t<metadb_handle_ptr> & p_data,const pfc::list_base_const_t<const char*> & p_dead)
{
	pfc::bit_array_bittable mask(p_data.get_count());
	t_size found = g_filter_dead_files_sorted_make_mask(p_data,p_dead,mask);
	if (found > 0) p_data.remove_mask(mask);
	return found;
}

t_size file_move_helper::g_filter_dead_files(pfc::list_base_t<metadb_handle_ptr> & p_data,const pfc::list_base_const_t<const char*> & p_dead)
{
	pfc::ptr_list_t<const char> temp;
	temp.add_items(p_dead);
	temp.sort_t(metadb::path_compare);
	return g_filter_dead_files_sorted(p_data,temp);
}

