//! Implements album_art_data.
class album_art_data_impl : public album_art_data {
public:
	const void * get_ptr() const {return m_content.get_ptr();}
	t_size get_size() const {return m_content.get_size();}

	void * get_ptr() {return m_content.get_ptr();}
	void set_size(t_size p_size) {m_content.set_size(p_size);}

	//! Reads picture data from the specified stream object.
	void from_stream(stream_reader * p_stream,t_size p_bytes,abort_callback & p_abort) {
		set_size(p_bytes); p_stream->read_object(get_ptr(),p_bytes,p_abort);
	}

	//! Creates an album_art_data object from picture data contained in a memory buffer.
	static album_art_data_ptr g_create(const void * p_buffer,t_size p_bytes) {
		service_ptr_t<album_art_data_impl> instance = new service_impl_t<album_art_data_impl>();
		instance->set_size(p_bytes);
		memcpy(instance->get_ptr(),p_buffer,p_bytes);
		return instance;
	}
	//! Creates an album_art_data object from picture data contained in a stream.
	static album_art_data_ptr g_create(stream_reader * p_stream,t_size p_bytes,abort_callback & p_abort) {
		service_ptr_t<album_art_data_impl> instance = new service_impl_t<album_art_data_impl>();
		instance->from_stream(p_stream,p_bytes,p_abort);
		return instance;
	}

private:
	pfc::array_t<t_uint8> m_content;
};


//! Helper - simple implementation of album_art_extractor_instance.
class album_art_extractor_instance_simple : public album_art_extractor_instance {
public:
	void set(const GUID & p_what,album_art_data_ptr p_content) {m_content.set(p_what,p_content);}
	bool have_item(const GUID & p_what) {return m_content.have_item(p_what);}
	album_art_data_ptr query(const GUID & p_what,abort_callback & p_abort) {
		album_art_data_ptr temp;
		if (!m_content.query(p_what,temp)) throw exception_album_art_not_found();
		return temp;
	}
	bool is_empty() const {return m_content.get_count() == 0;}
	bool remove(const GUID & p_what) {
		return m_content.remove(p_what);
	}
private:
	pfc::map_t<GUID,album_art_data_ptr> m_content;
};

//! Helper implementation of album_art_extractor - reads album art from arbitrary file formats that comply with APEv2 tagging specification.
class album_art_extractor_impl_stdtags : public album_art_extractor {
public:
	//! @param exts Semicolon-separated list of file format extensions to support.
	album_art_extractor_impl_stdtags(const char * exts) {
		pfc::splitStringSimple_toList(m_extensions,';',exts);
	}

	bool is_our_path(const char * p_path,const char * p_extension) {
		return m_extensions.have_item(p_extension);
	}

	album_art_extractor_instance_ptr open(file_ptr p_filehint,const char * p_path,abort_callback & p_abort) {
		PFC_ASSERT( is_our_path(p_path, pfc::string_extension(p_path) ) );
		file_ptr l_file ( p_filehint );
		if (l_file.is_empty()) filesystem::g_open_read(l_file, p_path, p_abort);
		return static_api_ptr_t<tag_processor_album_art_utils>()->open( l_file, p_abort );
	}
private:
	pfc::avltree_t<pfc::string,pfc::string::comparatorCaseInsensitiveASCII> m_extensions;
};

//! Helper implementation of album_art_editor - edits album art from arbitrary file formats that comply with APEv2 tagging specification.
class album_art_editor_impl_stdtags : public album_art_editor {
public:
	//! @param exts Semicolon-separated list of file format extensions to support.
	album_art_editor_impl_stdtags(const char * exts) {
		pfc::splitStringSimple_toList(m_extensions,';',exts);
	}

	bool is_our_path(const char * p_path,const char * p_extension) {
		return m_extensions.have_item(p_extension);
	}

	album_art_editor_instance_ptr open(file_ptr p_filehint,const char * p_path,abort_callback & p_abort) {
		PFC_ASSERT( is_our_path(p_path, pfc::string_extension(p_path) ) );
		file_ptr l_file ( p_filehint );
		if (l_file.is_empty()) filesystem::g_open(l_file, p_path, filesystem::open_mode_write_existing, p_abort);
		return static_api_ptr_t<tag_processor_album_art_utils>()->edit( l_file, p_abort );
	}
private:
	pfc::avltree_t<pfc::string,pfc::string::comparatorCaseInsensitiveASCII> m_extensions;

};

//! Helper - a more advanced implementation of album_art_extractor_instance.
class album_art_extractor_instance_fileref : public album_art_extractor_instance {
public:
	album_art_extractor_instance_fileref(file::ptr f) : m_file(f) {}

	void set(const GUID & p_what,t_filesize p_offset, t_filesize p_size) {
		const t_fileref ref = {p_offset, p_size};
		m_data.set(p_what, ref);
		m_cache.remove(p_what);
	}
	
	bool have_item(const GUID & p_what) {
		return m_data.have_item(p_what);
	}
	
	album_art_data_ptr query(const GUID & p_what,abort_callback & p_abort) {
		album_art_data_ptr item;
		if (m_cache.query(p_what,item)) return item;
		t_fileref ref;
		if (!m_data.query(p_what, ref)) throw exception_album_art_not_found();
		m_file->seek(ref.m_offset, p_abort);
		item = album_art_data_impl::g_create(m_file.get_ptr(), pfc::downcast_guarded<t_size>(ref.m_size), p_abort);
		m_cache.set(p_what, item);
		return item;
	}
	bool is_empty() const {return m_data.get_count() == 0;}
private:
	struct t_fileref {
		t_filesize m_offset, m_size;
	};
	const file::ptr m_file;
	pfc::map_t<GUID, t_fileref> m_data;
	pfc::map_t<GUID, album_art_data::ptr> m_cache;
};

//! album_art_path_list implementation helper
class album_art_path_list_impl : public album_art_path_list {
public:
	template<typename t_in> album_art_path_list_impl(const t_in & in) {pfc::list_to_array(m_data, in);}
	const char * get_path(t_size index) const {return m_data[index];}
	t_size get_count() const {return m_data.get_size();}
private:
	pfc::array_t<pfc::string8> m_data;
};

//! album_art_path_list implementation helper
class album_art_path_list_dummy : public album_art_path_list {
public:
	const char * get_path(t_size index) const {uBugCheck();}
	t_size get_count() const {return 0;}
};
