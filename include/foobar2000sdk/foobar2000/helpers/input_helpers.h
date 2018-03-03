#pragma once

#include <functional>
#include <list>

class input_helper {
public:
	input_helper();

    typedef std::function< bool ( file::ptr &, const char *, abort_callback & ) > ioFilter_t;
	typedef std::list<ioFilter_t> ioFilters_t;

	struct decodeInfo_t {
		bool m_flush_on_pause;
		bool m_can_seek;
#ifdef FOOBAR2000_MODERN
		bool m_seeking_expensive;
#endif
	};

	struct decodeOpen_t {
		decodeOpen_t() : m_from_redirect(), m_skip_hints(), m_flags() {}
		event_logger::ptr m_logger;
		bool m_from_redirect;
		bool m_skip_hints;
		unsigned m_flags;
		file::ptr m_hint;
		ioFilters_t m_ioFilters;
	};

	void open(service_ptr_t<file> p_filehint,metadb_handle_ptr p_location,unsigned p_flags,abort_callback & p_abort,bool p_from_redirect = false,bool p_skip_hints = false);
	void open(service_ptr_t<file> p_filehint,const playable_location & p_location,unsigned p_flags,abort_callback & p_abort,bool p_from_redirect = false,bool p_skip_hints = false);

	void open(const playable_location & location, abort_callback & abort, decodeOpen_t const & other);
	void open(metadb_handle_ptr location, abort_callback & abort, decodeOpen_t const & other) {this->open(location->get_location(), abort, other);}


	//! Multilevel open helpers.
	//! @returns Diagnostic/helper value: true if the decoder had to be re-opened entirely, false if the instance was reused.
	bool open_path(const char * path, abort_callback & abort, decodeOpen_t const & other);
	//! Multilevel open helpers.
	void open_decoding(t_uint32 subsong, t_uint32 flags, abort_callback & p_abort);

	bool need_file_reopen(const char * newPath) const;
	
	decodeInfo_t decode_info();

	void close();
	bool is_open();
	bool run(audio_chunk & p_chunk,abort_callback & p_abort);
	bool run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort);
	void seek(double seconds,abort_callback & p_abort);
	bool can_seek();
#ifdef FOOBAR2000_MODERN
    size_t extended_param( const GUID & type, size_t arg1, void * arg2, size_t arg2size);
#endif
	static ioFilter_t ioFilter_full_buffer(t_filesize val );
    static ioFilter_t ioFilter_block_buffer(size_t val);
    static ioFilter_t ioFilter_remote_read_ahead( size_t val );

	void on_idle(abort_callback & p_abort);
	bool get_dynamic_info(file_info & p_out,double & p_timestamp_delta);
	bool get_dynamic_info_track(file_info & p_out,double & p_timestamp_delta);
	void set_logger(event_logger::ptr ptr);
	void set_pause(bool state);
	bool flush_on_pause();

	//! Retrieves path of currently open file.
	const char * get_path() const;

	//! Retrieves info about specific subsong of currently open file.
	void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort);

	static void g_get_info(const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_from_redirect = false);
	static void g_set_info(const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_from_redirect = false);


	static bool g_mark_dead(const pfc::list_base_const_t<metadb_handle_ptr> & p_list,bit_array_var & p_mask,abort_callback & p_abort);

private:
	void fileOpenTools(service_ptr_t<file> & p_file,const char * p_path, ioFilters_t const & filters, abort_callback & p_abort);
	service_ptr_t<input_decoder> m_input;
	pfc::string8 m_path;
};

class NOVTABLE dead_item_filter : public abort_callback {
public:
	virtual void on_progress(t_size p_position,t_size p_total) = 0;

	bool run(const pfc::list_base_const_t<metadb_handle_ptr> & p_list,bit_array_var & p_mask);
};

class input_info_read_helper {
public:
	input_info_read_helper() {}
	void get_info(const playable_location & p_location,file_info & p_info,t_filestats & p_stats,abort_callback & p_abort);
	void get_info_check(const playable_location & p_location,file_info & p_info,t_filestats & p_stats,bool & p_reloaded,abort_callback & p_abort);
private:
	void open(const char * p_path,abort_callback & p_abort);

	pfc::string8 m_path;
	service_ptr_t<input_info_reader> m_input;
};



class input_helper_cue {
public:
	void open(service_ptr_t<file> p_filehint,const playable_location & p_location,unsigned p_flags,abort_callback & p_abort,double p_start,double p_length);

	void close();
	bool is_open();
	bool run(audio_chunk & p_chunk,abort_callback & p_abort);
	bool run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort);
	void seek(double seconds,abort_callback & p_abort);
	bool can_seek();
	void on_idle(abort_callback & p_abort);
	bool get_dynamic_info(file_info & p_out,double & p_timestamp_delta);
	bool get_dynamic_info_track(file_info & p_out,double & p_timestamp_delta);
	void set_logger(event_logger::ptr ptr) {m_input.set_logger(ptr);}

	const char * get_path() const;
	
	void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort);

private:
	bool _run(audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort);
	bool _m_input_run(audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort);
	input_helper m_input;
	double m_start,m_length,m_position;
	bool m_dynamic_info_trigger,m_dynamic_info_track_trigger;
};

//! openAudioData return value, see openAudioData()
struct openAudioData_t {
	file::ptr audioData; // audio data stream
	audio_chunk::spec_t audioSpec; // format description (sample rate, channel layout).
};

//! Opens the specified location as a stream of audio_samples. \n
//! Returns a file object that allows you to read the audio data stream as if it was a physical file, together with audio stream format description (sample rate, channel layout). \n
//! Please keep in mind that certain features of the returned file object are only as reliable as the underlying file format or decoder implementation allows them to be. \n
//! Reported exact file size may be either unavailable or unreliable if the file format does not let us known the exact value without decoding the whole file. \n
//! Seeking may be inaccurate with certain file formats. \n
//! In general, all file object methods will work as intended on core-supported file formats such as FLAC or WavPack. \n
//! However, if you want 100% functionality regardless of file format being worked with, mirror the content to a temp file and let go of the file object returned by openAudioData().
openAudioData_t openAudioData(playable_location const & loc, bool bSeekable, file::ptr fileHint, abort_callback & aborter);
openAudioData_t openAudioData2(playable_location const & loc, input_helper::decodeOpen_t const & openArg, abort_callback & aborter);
