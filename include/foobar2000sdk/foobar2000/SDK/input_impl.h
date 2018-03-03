enum t_input_open_reason {
	input_open_info_read,
	input_open_decode,
	input_open_info_write
};

//! Helper function for input implementation use; ensures that file is open with relevant access mode. This is typically called from input_impl::open() and such.
//! @param p_file File object pointer to process. If passed pointer is non-null, the function does nothing and always succeeds; otherwise it attempts to open the file using filesystem API methods.
//! @param p_path Path to the file.
//! @param p_reason Type of input operation requested. See: input_impl::open() parameters.
//! @param p_abort abort_callback object signaling user aborting the operation.
void input_open_file_helper(service_ptr_t<file> & p_file,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort);


//! This is a class that just declares prototypes of functions that each input needs to implement. See input_decoder / input_info_reader / input_info_writer interfaces for full descriptions of member functions. Since input implementation class is instantiated using a template, you don't need to derive from input_impl as virtual functions are not used on implementation class level. Use input_factory_t template to register input class based on input_impl.
class input_impl
{
public:
	//! Opens specified file for info read / decoding / info write. This is called only once, immediately after object creation, before any other methods, and no other methods are called if open() fails.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the implementation should handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example). Typically, an input implementation that requires file access calls input_open_file_helper() function to ensure that file is open with relevant access mode (read or read/write).
	//! @param p_path URL of resource being opened.
	//! @param p_reason Type of operation requested. Possible values are: \n
	//! - input_open_info_read - info retrieval methods are valid; \n
	//! - input_open_decode - info retrieval and decoding methods are valid; \n
	//! - input_open_info_write - info retrieval and retagging methods are valid; \n
	//! Note that info retrieval methods are valid in all cases, and they may be called at any point of decoding/retagging process. Results of info retrieval methods (other than get_subsong_count() / get_subsong()) between retag_set_info() and retag_commit() are undefined however; those should not be called during that period.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort);

	//! See: input_info_reader::get_subsong_count(). Valid after open() with any reason.
	unsigned get_subsong_count();
	//! See: input_info_reader::get_subsong(). Valid after open() with any reason.
	t_uint32 get_subsong(unsigned p_index);
	//! See: input_info_reader::get_info(). Valid after open() with any reason.
	void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort);
	//! See: input_info_reader::get_file_stats(). Valid after open() with any reason.
	t_filestats get_file_stats(abort_callback & p_abort);

	//! See: input_decoder::initialize(). Valid after open() with input_open_decode reason.
	void decode_initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort);
	//! See: input_decoder::run(). Valid after decode_initialize().
	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort);
	//! See: input_decoder::seek(). Valid after decode_initialize().
	void decode_seek(double p_seconds,abort_callback & p_abort);
	//! See: input_decoder::can_seek(). Valid after decode_initialize().
	bool decode_can_seek();
	//! See: input_decoder::get_dynamic_info(). Valid after decode_initialize().
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta);
	//! See: input_decoder::get_dynamic_info_track(). Valid after decode_initialize().
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta);
	//! See: input_decoder::on_idle(). Valid after decode_initialize().
	void decode_on_idle(abort_callback & p_abort);

	//! See: input_info_writer::set_info(). Valid after open() with input_open_info_write reason.
	void retag_set_info(t_uint32 p_subsong,const file_info & p_info,abort_callback & p_abort);
	//! See: input_info_writer::commit(). Valid after open() with input_open_info_write reason.
	void retag_commit(abort_callback & p_abort);	
	
	//! See: input_entry::is_our_content_type().
	static bool g_is_our_content_type(const char * p_content_type);
	//! See: input_entry::is_our_path().
	static bool g_is_our_path(const char * p_path,const char * p_extension);

	//! See: input_entry::get_guid().
	static GUID g_get_guid();
	//! See: input_entry::get_name().
	static const char * g_get_name();
	//! See: input_entry::get_preferences_guid().
	static GUID g_get_preferences_guid();
	//! See: input_entry::is_low_merit().
	static bool g_is_low_merit();

	//! See: input_decoder_v2::run_raw(). Relevant only when implementing input_decoder_v2. Valid after decode_initialize().
	bool decode_run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort);

	//! See: input_decoder::set_logger(). Relevant only when implementing input_decoder_v2. Valid after any open().
	void set_logger(event_logger::ptr ptr);
protected:
	input_impl() {}
	~input_impl() {}
};

//! A base class that provides stub implementations of all optional input methods. \n
//! Inherit from this and you implement input_decoder_v4 without having to provide all the methods you don't actually need.
class input_stubs {
public:
	bool decode_run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) { throw pfc::exception_not_implemented(); }
	void set_logger(event_logger::ptr ptr) {}
	void set_pause(bool paused) {}
	bool flush_on_pause() { return false; }
	size_t extended_param(const GUID & type, size_t arg1, void * arg2, size_t arg2size) { return 0; }
	static GUID g_get_preferences_guid() {return pfc::guid_null;}
	static bool g_is_low_merit() { return false; }

	//! These typedefs indicate which interfaces your class actually supports. You can override them to support non default input API interfaces without specifying input_factory parameters.
	typedef input_decoder_v4 interface_decoder_t;
	typedef input_info_reader interface_info_reader_t;
	typedef input_info_writer interface_info_writer_t;
};

//! This is a class that just declares prototypes of functions that each non-multitrack-enabled input needs to implement. See input_decoder / input_info_reader / input_info_writer interfaces for full descriptions of member functions. Since input implementation class is instantiated using a template, you don't need to derive from input_singletrack_impl as virtual functions are not used on implementation class level. Use input_singletrack_factory_t template to register input class based on input_singletrack_impl.
class input_singletrack_impl
{
public:
	//! Opens specified file for info read / decoding / info write. This is called only once, immediately after object creation, before any other methods, and no other methods are called if open() fails.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the implementation should handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example). Typically, an input implementation that requires file access calls input_open_file_helper() function to ensure that file is open with relevant access mode (read or read/write).
	//! @param p_path URL of resource being opened.
	//! @param p_reason Type of operation requested. Possible values are: \n
	//! - input_open_info_read - info retrieval methods are valid; \n
	//! - input_open_decode - info retrieval and decoding methods are valid; \n
	//! - input_open_info_write - info retrieval and retagging methods are valid; \n
	//! Note that info retrieval methods are valid in all cases, and they may be called at any point of decoding/retagging process. Results of info retrieval methods (other than get_subsong_count() / get_subsong()) between retag_set_info() and retag_commit() are undefined however; those should not be called during that period.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort);

	//! See: input_info_reader::get_info(). Valid after open() with any reason. \n
	//! Implementation warning: this is typically also called immediately after tag update and should return newly written content then.
	void get_info(file_info & p_info,abort_callback & p_abort);
	//! See: input_info_reader::get_file_stats(). Valid after open() with any reason. \n
	//! Implementation warning: this is typically also called immediately after tag update and should return new values then.
	t_filestats get_file_stats(abort_callback & p_abort);

	//! See: input_decoder::initialize(). Valid after open() with input_open_decode reason.
	void decode_initialize(unsigned p_flags,abort_callback & p_abort);
	//! See: input_decoder::run(). Valid after decode_initialize().
	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort);
	//! See: input_decoder::seek(). Valid after decode_initialize().
	void decode_seek(double p_seconds,abort_callback & p_abort);
	//! See: input_decoder::can_seek(). Valid after decode_initialize().
	bool decode_can_seek();
	//! See: input_decoder::get_dynamic_info(). Valid after decode_initialize().
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta);
	//! See: input_decoder::get_dynamic_info_track(). Valid after decode_initialize().
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta);
	//! See: input_decoder::on_idle(). Valid after decode_initialize().
	void decode_on_idle(abort_callback & p_abort);

	//! See: input_info_writer::set_info(). Note that input_info_writer::commit() call isn't forwarded because it's useless in case of non-multitrack-enabled inputs. Valid after open() with input_open_info_write reason.
	void retag(const file_info & p_info,abort_callback & p_abort);
	
	//! See: input_entry::is_our_content_type().
	static bool g_is_our_content_type(const char * p_content_type);
	//! See: input_entry::is_our_path().
	static bool g_is_our_path(const char * p_path,const char * p_extension);

protected:
	input_singletrack_impl() {}
	~input_singletrack_impl() {}
};


//! Used internally by standard input_entry implementation; do not use directly. Translates input_decoder / input_info_reader / input_info_writer calls to input_impl calls.
template<typename I, typename interface_t>
class input_impl_interface_wrapper_t : public interface_t
{
public:
	void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		m_instance.open(p_filehint,p_path,p_reason,p_abort);
	}

	// input_info_reader methods

	t_uint32 get_subsong_count() {
		return m_instance.get_subsong_count();
	}
	
	t_uint32 get_subsong(t_uint32 p_index) {
		return m_instance.get_subsong(p_index);
	}

	
	void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) {
		m_instance.get_info(p_subsong,p_info,p_abort);
	}

	t_filestats get_file_stats(abort_callback & p_abort) {
		return m_instance.get_file_stats(p_abort);
	}

	// input_decoder methods

	void initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort) {
		m_instance.decode_initialize(p_subsong,p_flags,p_abort);
#if PFC_DEBUG
		m_eof = false;
#endif
	}

	bool run(audio_chunk & p_chunk,abort_callback & p_abort) {
#if PFC_DEBUG
		PFC_ASSERT( !m_eof );
		// Complain if run()/run_raw() gets called again after having returned EOF, this means a logic error on caller's side
#endif
		bool ret = m_instance.decode_run(p_chunk,p_abort);
#if PFC_DEBUG
		if ( !ret ) m_eof = true;
#endif
		return ret;
	}

	bool run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) {
#if PFC_DEBUG
		// Complain if run()/run_raw() gets called again after having returned EOF, this means a logic error on caller's side
		PFC_ASSERT(!m_eof);
#endif
		bool ret = m_instance.decode_run_raw(p_chunk, p_raw, p_abort);
#if PFC_DEBUG
		if ( !ret ) m_eof = true;
#endif
		return ret;
	}

	void seek(double p_seconds,abort_callback & p_abort) {
		m_instance.decode_seek(p_seconds,p_abort);
#if PFC_DEBUG
		m_eof = false;
#endif
	}
	
	bool can_seek() {
		return m_instance.decode_can_seek();
	}

	bool get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {
		return m_instance.decode_get_dynamic_info(p_out,p_timestamp_delta);
	}

	bool get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) {
		return m_instance.decode_get_dynamic_info_track(p_out,p_timestamp_delta);
	}

	void on_idle(abort_callback & p_abort) {
		m_instance.decode_on_idle(p_abort);
	}

	void set_logger(event_logger::ptr ptr) {
		m_instance.set_logger(ptr);
	}

	void set_pause(bool paused) {
		// obsolete
		// m_instance.set_pause(paused);
	}

	bool flush_on_pause() {
		return m_instance.flush_on_pause();
	}

	size_t extended_param(const GUID & type, size_t arg1, void * arg2, size_t arg2size) {
		return m_instance.extended_param(type, arg1, arg2, arg2size);
	}

	// input_info_writer methods

	void set_info(t_uint32 p_subsong,const file_info & p_info,abort_callback & p_abort) {
		m_instance.retag_set_info(p_subsong,p_info,p_abort);
	}
	
	void commit(abort_callback & p_abort) {
		m_instance.retag_commit(p_abort);
	}
	void remove_tags(abort_callback & p_abort) {
		m_instance.remove_tags(p_abort);
	}

private:
	I m_instance;

#if PFC_DEBUG
	// Report illegal API calls in debug build
	bool m_eof = false;
#endif
};

template<typename input_t>
class input_forward_static_methods : public input_stubs {
public:
	static bool g_is_our_content_type(const char * p_content_type) { return input_t::g_is_our_content_type(p_content_type); }
	static bool g_is_our_path(const char * p_path, const char * p_extension) { return input_t::g_is_our_path(p_path, p_extension); }
	static GUID g_get_preferences_guid() { return input_t::g_get_preferences_guid(); }
	static GUID g_get_guid() { return input_t::g_get_guid(); }
	static const char * g_get_name() { return input_t::g_get_name(); }
	static bool g_is_low_merit() { return input_t::g_is_low_merit(); }

	typedef typename input_t::interface_decoder_t interface_decoder_t;
	typedef typename input_t::interface_info_reader_t interface_info_reader_t;
	typedef typename input_t::interface_info_writer_t interface_info_writer_t;

};

//! Helper used by input_singletrack_factory_t, do not use directly. Translates input_impl calls to input_singletrack_impl calls. 
template<typename I>
class input_wrapper_singletrack_t : public input_forward_static_methods<I>
{
public:
	input_wrapper_singletrack_t() {}

	void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		m_instance.open(p_filehint,p_path,p_reason,p_abort);
	}

	void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) {
		if (p_subsong != 0) throw exception_io_bad_subsong_index();
		m_instance.get_info(p_info,p_abort);
	}

	t_uint32 get_subsong_count() {
		return 1;
	}
	
	t_uint32 get_subsong(t_uint32 p_index) {
		PFC_ASSERT(p_index == 0);
		return 0;
	}

	t_filestats get_file_stats(abort_callback & p_abort) {
		return m_instance.get_file_stats(p_abort);
	}

	void decode_initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort) {
		if (p_subsong != 0) throw exception_io_bad_subsong_index();
		m_instance.decode_initialize(p_flags,p_abort);
	}

	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {return m_instance.decode_run(p_chunk,p_abort);}
	void decode_seek(double p_seconds,abort_callback & p_abort) {m_instance.decode_seek(p_seconds,p_abort);}
	bool decode_can_seek() {return m_instance.decode_can_seek();}
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {return m_instance.decode_get_dynamic_info(p_out,p_timestamp_delta);}
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) {return m_instance.decode_get_dynamic_info_track(p_out,p_timestamp_delta);}
	void decode_on_idle(abort_callback & p_abort) {m_instance.decode_on_idle(p_abort);}

	void retag_set_info(t_uint32 p_subsong,const file_info & p_info,abort_callback & p_abort) {
		if (p_subsong != 0) throw exception_io_bad_subsong_index();
		m_instance.retag(p_info,p_abort);
	}
	
	bool decode_run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) {
		return m_instance.decode_run_raw(p_chunk, p_raw, p_abort);
	}

	void set_logger(event_logger::ptr ptr) {m_instance.set_logger(ptr);}

	void set_pause(bool paused) {
		// m_instance.set_pause(paused);
	}
	bool flush_on_pause() {
		return m_instance.flush_on_pause();
	}
	size_t extended_param(const GUID & type, size_t arg1, void * arg2, size_t arg2size) { 
		return m_instance.extended_param(type, arg1, arg2, arg2size);
	}
	void retag_commit(abort_callback & p_abort) {}

	void remove_tags(abort_callback & p_abort) {
		m_instance.remove_tags(p_abort);
	}
private:
	I m_instance;
};

//! Helper; standard input_entry implementation. Do not instantiate this directly, use input_factory_t or one of other input_*_factory_t helpers instead.
template<typename I,unsigned t_flags, typename t_decoder = typename I::interface_decoder_t, typename t_inforeader = typename I::interface_info_reader_t, typename t_infowriter = typename I::interface_info_writer_t>
class input_entry_impl_t : public input_entry_v2
{
private:
	
	template<typename T, typename out>
	void instantiate_t(service_ptr_t<out> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort)
	{
		service_ptr_t< service_impl_t<input_impl_interface_wrapper_t<I,T> > > temp;
		temp = new service_impl_t<input_impl_interface_wrapper_t<I,T> >();
		temp->open(p_filehint,p_path,p_reason,p_abort);
		p_instance = temp.get_ptr();
	}
public:
	bool is_our_content_type(const char * p_type) {return I::g_is_our_content_type(p_type);}
	bool is_our_path(const char * p_full_path,const char * p_extension) {return I::g_is_our_path(p_full_path,p_extension);}
	
	void open_for_decoding(service_ptr_t<input_decoder> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) {
		instantiate_t<t_decoder>(p_instance,p_filehint,p_path,input_open_decode,p_abort);
	}

	void open_for_info_read(service_ptr_t<input_info_reader> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) {
		instantiate_t<t_inforeader>(p_instance,p_filehint,p_path,input_open_info_read,p_abort);
	}

	void open_for_info_write(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) {
		instantiate_t<t_infowriter>(p_instance,p_filehint,p_path,input_open_info_write,p_abort);
	}

	void get_extended_data(service_ptr_t<file> p_filehint,const playable_location & p_location,const GUID & p_guid,mem_block_container & p_out,abort_callback & p_abort) {
		p_out.reset();
	}

	unsigned get_flags() {return t_flags;}

	GUID get_guid() {
		return I::g_get_guid();
	}
	const char * get_name() {
		return I::g_get_name();
	}

	GUID get_preferences_guid() {
		return I::g_get_preferences_guid();
	}
	bool is_low_merit() {
		return I::g_is_low_merit();
	}
};


//! Stardard input factory. For reference of functions that must be supported by registered class, see input_impl.\n Usage: static input_factory_t<myinputclass> g_myinputclass_factory;\n Note that input classes can't be registered through service_factory_t template directly.
template<typename T, unsigned t_flags = 0>
class input_factory_t : public service_factory_single_t<input_entry_impl_t<T, t_flags> > {};

//! Non-multitrack-enabled input factory (helper) - hides multitrack management functions from input implementation; use this for inputs that handle file types where each physical file can contain only one user-visible playable track. For reference of functions that must be supported by registered class, see input_singletrack_impl.\n Usage: static input_singletrack_factory_t<myinputclass> g_myinputclass_factory;\n Note that input classes can't be registered through service_factory_t template directly.template<class T>
template<typename T, unsigned t_flags = 0>
class input_singletrack_factory_t : public service_factory_single_t<input_entry_impl_t<input_wrapper_singletrack_t<T>,t_flags> > {};

//! Extended version of input_factory_t, with non-default flags and supported interfaces. See: input_factory_t, input_entry::get_flags(). \n
//! This is obsolete and provided for backwards compatibility. Use interface_decoder_t + interface_info_reader_t + interface_info_writer_t typedefs in your input class to specify supported interfaces.
template<typename T, unsigned t_flags = 0, typename t_decoder = typename T::interface_decoder_t, typename t_inforeader = typename T::interface_info_reader_t, typename t_infowriter = typename T::interface_info_writer_t>
class input_factory_ex_t : public service_factory_single_t<input_entry_impl_t<T, t_flags, t_decoder, t_inforeader, t_infowriter> > {};

//! Extended version of input_singletrack_factory_t, with non-default flags and supported interfaces. See: input_singletrack_factory_t, input_entry::get_flags().
//! This is obsolete and provided for backwards compatibility. Use interface_decoder_t + interface_info_reader_t + interface_info_writer_t typedefs in your input class to specify supported interfaces.
template<typename T, unsigned t_flags = 0, typename t_decoder = typename T::interface_decoder_t, typename t_inforeader = typename T::interface_info_reader_t, typename t_infowriter = typename T::interface_info_writer_t>
class input_singletrack_factory_ex_t : public service_factory_single_t<input_entry_impl_t<input_wrapper_singletrack_t<T>, t_flags, t_decoder, t_inforeader, t_infowriter> > {};
