#include "foobar2000.h"

#if FOOBAR2000_TARGET_VERSION >= 76
static void process_path_internal(const char * p_path,const service_ptr_t<file> & p_reader,playlist_loader_callback::ptr callback, abort_callback & abort,playlist_loader_callback::t_entry_type type,const t_filestats & p_stats);

namespace {
	class archive_callback_impl : public archive_callback {
	public:
		archive_callback_impl(playlist_loader_callback::ptr p_callback, abort_callback & p_abort) : m_callback(p_callback), m_abort(p_abort) {}
		bool on_entry(archive * owner,const char * p_path,const t_filestats & p_stats,const service_ptr_t<file> & p_reader)
		{
			process_path_internal(p_path,p_reader,m_callback,m_abort,playlist_loader_callback::entry_directory_enumerated,p_stats);
			return !m_abort.is_aborting();
		}
		bool is_aborting() const {return m_abort.is_aborting();}
		abort_callback_event get_abort_event() const {return m_abort.get_abort_event();}
	private:
		const playlist_loader_callback::ptr m_callback;
		abort_callback & m_abort;
	};
}

void playlist_loader::g_load_playlist_filehint(file::ptr fileHint,const char * p_path,playlist_loader_callback::ptr p_callback, abort_callback & p_abort) {
	TRACK_CALL_TEXT("playlist_loader::g_load_playlist_filehint");
	pfc::string8 filepath;

	filesystem::g_get_canonical_path(p_path,filepath);
	
	pfc::string_extension extension(filepath);

	service_ptr_t<file> l_file = fileHint;

	if (l_file.is_empty()) {
		filesystem::ptr fs;
		if (filesystem::g_get_interface(fs,filepath)) {
			if (fs->supports_content_types()) {
				fs->open(l_file,filepath,filesystem::open_mode_read,p_abort);
			}
		}
	}

	{
		service_enum_t<playlist_loader> e;

		if (l_file.is_valid()) {
			pfc::string8 content_type;
			if (l_file->get_content_type(content_type)) {
				service_ptr_t<playlist_loader> l;
				e.reset(); while(e.next(l)) {
					if (l->is_our_content_type(content_type)) {
						try {
							TRACK_CODE("playlist_loader::open",l->open(filepath,l_file,p_callback, p_abort));
							return;
						} catch(exception_io_unsupported_format) {
							l_file->reopen(p_abort);
						}
					}
				}
			}
		}

		if (extension.length()>0) {
			playlist_loader::ptr l;
			e.reset(); while(e.next(l)) {
				if (stricmp_utf8(l->get_extension(),extension) == 0) {
					if (l_file.is_empty()) filesystem::g_open_read(l_file,filepath,p_abort);
					try {
						TRACK_CODE("playlist_loader::open",l->open(filepath,l_file,p_callback,p_abort));
						return;
					} catch(exception_io_unsupported_format) {
						l_file->reopen(p_abort);
					}
				}
			}
		}
	}

	throw exception_io_unsupported_format();
}
void playlist_loader::g_load_playlist(const char * p_path,playlist_loader_callback::ptr callback, abort_callback & abort) {
	g_load_playlist_filehint(NULL,p_path,callback,abort);
}

static void index_tracks_helper(const char * p_path,const service_ptr_t<file> & p_reader,const t_filestats & p_stats,playlist_loader_callback::t_entry_type p_type,playlist_loader_callback::ptr p_callback, abort_callback & p_abort,bool & p_got_input)
{
	TRACK_CALL_TEXT("index_tracks_helper");
	if (p_reader.is_empty() && filesystem::g_is_remote_safe(p_path))
	{
		TRACK_CALL_TEXT("remote");
		metadb_handle_ptr handle;
		p_callback->handle_create(handle,make_playable_location(p_path,0));
		p_got_input = true;
		p_callback->on_entry(handle,p_type,p_stats,true);
	} else {
		TRACK_CALL_TEXT("hintable");
		service_ptr_t<input_info_reader> instance;
		input_entry::g_open_for_info_read(instance,p_reader,p_path,p_abort);

		t_filestats stats = instance->get_file_stats(p_abort);

		t_uint32 subsong,subsong_count = instance->get_subsong_count();
		for(subsong=0;subsong<subsong_count;subsong++)
		{
			TRACK_CALL_TEXT("subsong-loop");
			p_abort.check();
			metadb_handle_ptr handle;
			t_uint32 index = instance->get_subsong(subsong);
			p_callback->handle_create(handle,make_playable_location(p_path,index));

			p_got_input = true;
			if (p_callback->want_info(handle,p_type,stats,true))
			{
				file_info_impl info;
				TRACK_CODE("get_info",instance->get_info(index,info,p_abort));
				p_callback->on_entry_info(handle,p_type,stats,info,true);
			}
			else
			{
				p_callback->on_entry(handle,p_type,stats,true);
			}
		}
	}
}


static void track_indexer__g_get_tracks_wrap(const char * p_path,const service_ptr_t<file> & p_reader,const t_filestats & p_stats,playlist_loader_callback::t_entry_type p_type,playlist_loader_callback::ptr p_callback, abort_callback & p_abort) {
	bool got_input = false;
	bool fail = false;
	try {
		index_tracks_helper(p_path,p_reader,p_stats,p_type,p_callback,p_abort, got_input);
	} catch(exception_aborted) {
		throw;
	} catch(exception_io_unsupported_format) {
		fail = true;
	} catch(std::exception const & e) {
		fail = true;
		console::formatter() << "could not enumerate tracks (" << e << ") on:\n" << file_path_display(p_path);
	}
	if (fail) {
		if (!got_input && !p_abort.is_aborting()) {
			if (p_type == playlist_loader_callback::entry_user_requested)
			{
				metadb_handle_ptr handle;
				p_callback->handle_create(handle,make_playable_location(p_path,0));
				p_callback->on_entry(handle,p_type,p_stats,true);
			}
		}
	}
}


static void process_path_internal(const char * p_path,const service_ptr_t<file> & p_reader,playlist_loader_callback::ptr callback, abort_callback & abort,playlist_loader_callback::t_entry_type type,const t_filestats & p_stats)
{
	//p_path must be canonical

	abort.check();

	callback->on_progress(p_path);

	
	{
		if (p_reader.is_empty()) {
			directory_callback_impl directory_results(true);
			try {
				filesystem::g_list_directory(p_path,directory_results,abort);
				for(t_size n=0;n<directory_results.get_count();n++) {
					process_path_internal(directory_results.get_item(n),0,callback,abort,playlist_loader_callback::entry_directory_enumerated,directory_results.get_item_stats(n));
				}
				return;
			} catch(exception_aborted) {throw;}
			catch(...) {
				//do nothing, fall thru
				//fixme - catch only filesystem exceptions?
			}
		}

		bool found = false;


		{
			archive_callback_impl archive_results(callback, abort);
			service_enum_t<filesystem> e;
			service_ptr_t<filesystem> f;
			while(e.next(f)) {
				abort.check();
				service_ptr_t<archive> arch;
				if (f->service_query_t(arch)) {
					if (p_reader.is_valid()) p_reader->reopen(abort);

					try {
						TRACK_CODE("archive::archive_list",arch->archive_list(p_path,p_reader,archive_results,true));
						return;
					} catch(exception_aborted) {throw;} 
					catch(...) {}
				}
			} 
		}
	}

	

	{
		service_ptr_t<link_resolver> ptr;
		if (link_resolver::g_find(ptr,p_path))
		{
			if (p_reader.is_valid()) p_reader->reopen(abort);

			pfc::string8 temp;
			try {
				TRACK_CODE("link_resolver::resolve",ptr->resolve(p_reader,p_path,temp,abort));

				track_indexer__g_get_tracks_wrap(temp,0,filestats_invalid,playlist_loader_callback::entry_from_playlist,callback, abort);
				return;//success
			} catch(exception_aborted) {throw;}
			catch(...) {}
		}
	}

	if (callback->is_path_wanted(p_path,type)) {
		track_indexer__g_get_tracks_wrap(p_path,p_reader,p_stats,type,callback, abort);
	}
}

void playlist_loader::g_process_path(const char * p_filename,playlist_loader_callback::ptr callback, abort_callback & abort,playlist_loader_callback::t_entry_type type)
{
	TRACK_CALL_TEXT("playlist_loader::g_process_path");

	file_path_canonical filename(p_filename);

	process_path_internal(filename,0,callback,abort, type,filestats_invalid);
}

void playlist_loader::g_save_playlist(const char * p_filename,const pfc::list_base_const_t<metadb_handle_ptr> & data,abort_callback & p_abort)
{
	TRACK_CALL_TEXT("playlist_loader::g_save_playlist");
	pfc::string8 filename;
	filesystem::g_get_canonical_path(p_filename,filename);
	try {
		service_ptr_t<file> r;
		filesystem::g_open(r,filename,filesystem::open_mode_write_new,p_abort);

		pfc::string_extension ext(filename);
		
		service_enum_t<playlist_loader> e;
		service_ptr_t<playlist_loader> l;
		if (e.first(l)) do {
			if (l->can_write() && !stricmp_utf8(ext,l->get_extension())) {
				try {
					TRACK_CODE("playlist_loader::write",l->write(filename,r,data,p_abort));
					return;
				} catch(exception_io_data) {}
			}
		} while(e.next(l));
		throw exception_io_data();
	} catch(...) {
		try {filesystem::g_remove(filename,p_abort);} catch(...) {}
		throw;
	}
}


bool playlist_loader::g_process_path_ex(const char * filename,playlist_loader_callback::ptr callback, abort_callback & abort,playlist_loader_callback::t_entry_type type)
{
	try {
		g_load_playlist(filename,callback, abort);
		return true;
	} catch(exception_io_unsupported_format) {//not a playlist format
		g_process_path(filename,callback,abort,type);
		return false;
	}
}

#endif