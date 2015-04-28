#include "foobar2000.h"


double metadb_handle::get_length()
{
	return this->get_info_ref()->info().get_length();
}

t_filetimestamp metadb_handle::get_filetimestamp()
{
	return get_filestats().m_timestamp;
}

t_filesize metadb_handle::get_filesize()
{
	return get_filestats().m_size;
}

bool metadb_handle::format_title_legacy(titleformat_hook * p_hook,pfc::string_base & p_out,const char * p_spec,titleformat_text_filter * p_filter)
{
	service_ptr_t<titleformat_object> script;
	if (static_api_ptr_t<titleformat_compiler>()->compile(script,p_spec)) {
		return format_title(p_hook,p_out,script,p_filter);
	} else {
		p_out.reset();
		return false;
	}
}



bool metadb_handle::g_should_reload(const t_filestats & p_old_stats,const t_filestats & p_new_stats,bool p_fresh)
{
	if (p_new_stats.m_timestamp == filetimestamp_invalid) return p_fresh;
	else if (p_fresh) return p_old_stats!= p_new_stats;
	else return p_old_stats.m_timestamp < p_new_stats.m_timestamp;
}

bool metadb_handle::should_reload(const t_filestats & p_new_stats, bool p_fresh) const
{
	if (!is_info_loaded_async()) return true;
	else return g_should_reload(get_filestats(),p_new_stats,p_fresh);
}


bool metadb_handle::get_browse_info_merged(file_info & infoMerged) const {
	bool rv = true;
	metadb_info_container::ptr info, browse;
	this->get_browse_info_ref(info, browse);
	if (info.is_valid() && browse.is_valid()) {
		infoMerged = info->info();
		infoMerged.merge_fallback( browse->info() );
	} else if (info.is_valid()) {
		infoMerged = info->info();
	} else if (browse.is_valid()) {
		infoMerged = browse->info();
	} else {
		infoMerged.reset();
		rv = false;
	}
	return rv;
}

namespace {
	class metadb_info_container_impl : public metadb_info_container {
	public:
		metadb_info_container_impl() : m_stats( filestats_invalid ), m_partial() {}
		file_info const & info() {
			return m_info;
		}
		t_filestats const & stats() {
			return m_stats;
		}
		bool isInfoPartial() {
			return m_partial;
		}

		file_info_impl m_info;
		t_filestats m_stats;
		bool m_partial;
		
	};
}

metadb_info_container::ptr metadb_handle::get_full_info_ref( abort_callback & aborter ) const {
	{
		metadb_info_container::ptr info;
		if (this->get_info_ref( info ) ) {
			if (!info->isInfoPartial()) return info;
		}
	}


	input_info_reader::ptr reader;
	input_entry::g_open_for_info_read( reader, NULL, this->get_path(), aborter );
	
	service_ptr_t< metadb_info_container_impl > obj = new service_impl_t<metadb_info_container_impl>();
	obj->m_stats = reader->get_file_stats( aborter );
	reader->get_info( this->get_subsong_index(), obj->m_info, aborter );
	return obj;
}
