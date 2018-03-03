PFC_DECLARE_EXCEPTION(exception_tagging_unsupported, exception_io_data, "Tagging of this file format is not supported")

enum {
	input_flag_no_seeking					= 1 << 0,
	input_flag_no_looping					= 1 << 1,
	input_flag_playback						= 1 << 2,
	input_flag_testing_integrity			= 1 << 3,
	input_flag_allow_inaccurate_seeking		= 1 << 4,
	input_flag_no_postproc					= 1 << 5,

	input_flag_simpledecode = input_flag_no_seeking|input_flag_no_looping,
};

//! Class providing interface for retrieval of information (metadata, duration, replaygain, other tech infos) from files. Also see: file_info. \n
//! Instantiating: see input_entry.\n
//! Implementing: see input_impl.

class NOVTABLE input_info_reader : public service_base
{
public:
	//! Retrieves count of subsongs in the file. 1 for non-multisubsong-enabled inputs.
	//! Note: multi-subsong handling is disabled for remote files (see: filesystem::is_remote) for performance reasons. Remote files are always assumed to be single-subsong, with null index.
	virtual t_uint32 get_subsong_count() = 0;
	
	//! Retrieves identifier of specified subsong; this identifier is meant to be used in playable_location as well as a parameter for input_info_reader::get_info().
	//! @param p_index Index of subsong to query. Must be >=0 and < get_subsong_count().
	virtual t_uint32 get_subsong(t_uint32 p_index) = 0;
	
	//! Retrieves information about specified subsong.
	//! @param p_subsong Identifier of the subsong to query. See: input_info_reader::get_subsong(), playable_location.
	//! @param p_info file_info object to fill. Must be empty on entry.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) = 0;

	//! Retrieves file stats. Equivalent to calling get_stats() on file object.
	virtual t_filestats get_file_stats(abort_callback & p_abort) = 0;

	FB2K_MAKE_SERVICE_INTERFACE(input_info_reader,service_base);
};

//! Class providing interface for retrieval of PCM audio data from files.\n
//! Instantiating: see input_entry.\n
//! Implementing: see input_impl.

class NOVTABLE input_decoder : public input_info_reader
{
public:
	//! Prepares to decode specified subsong; resets playback position to the beginning of specified subsong. This must be called first, before any other input_decoder methods (other than those inherited from input_info_reader). \n
	//! It is legal to set initialize() more than once, with same or different subsong, to play either the same subsong again or another subsong from same file without full reopen.\n
	//! Warning: this interface inherits from input_info_reader, it is legal to call any input_info_reader methods even during decoding! Call order is not defined, other than initialize() requirement before calling other input_decoder methods.\n
	//! @param p_subsong Subsong to decode. Should always be 0 for non-multi-subsong-enabled inputs.
	//!	@param p_flags Specifies additional hints for decoding process. It can be null, or a combination of one or more following constants: \n
	//!		input_flag_no_seeking - Indicates that seek() will never be called. Can be used to avoid building potentially expensive seektables when only sequential reading is needed.\n
	//!		input_flag_no_looping - Certain input implementations can be configured to utilize looping info from file formats they process and keep playing single file forever, or keep repeating it specified number of times. This flag indicates that such features should be disabled, for e.g. ReplayGain scan or conversion.\n
	//!		input_flag_playback	- Indicates that decoding process will be used for realtime playback rather than conversion. This can be used to reconfigure features that are relevant only for conversion and take a lot of resources, such as very slow secure CDDA reading. \n
	//!		input_flag_testing_integrity - Indicates that we're testing integrity of the file. Any recoverable problems where decoding would normally continue should cause decoder to fail with exception_io_data.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort) = 0;

	//! Reads/decodes one chunk of audio data. Use false return value to signal end of file (no more data to return). Before calling run(), decoding must be initialized by initialize() call.
	//! @param p_chunk audio_chunk object receiving decoded data. Contents are valid only the method returns true.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	//! @returns true on success (new data decoded), false on EOF.
	virtual bool run(audio_chunk & p_chunk,abort_callback & p_abort) = 0;

	//! Seeks to specified time offset. Before seeking or other decoding calls, decoding must be initialized with initialize() call.
	//! @param p_seconds Time to seek to, in seconds. If p_seconds exceeds length of the object being decoded, succeed, and then return false from next run() call.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void seek(double p_seconds,abort_callback & p_abort) = 0;
	
	//! Queries whether resource being read/decoded is seekable. If p_value is set to false, all seek() calls will fail. Before calling can_seek() or other decoding calls, decoding must be initialized with initialize() call.
	virtual bool can_seek() = 0;

	//! This function is used to signal dynamic VBR bitrate, etc. Called after each run() (or not called at all if caller doesn't care about dynamic info).
	//! @param p_out Initially contains currently displayed info (either last get_dynamic_info result or current cached info), use this object to return changed info.
	//! @param p_timestamp_delta Indicates when returned info should be displayed (in seconds, relative to first sample of last decoded chunk), initially set to 0.
	//! @returns false to keep old info, or true to indicate that changes have been made to p_info and those should be displayed.
	virtual bool get_dynamic_info(file_info & p_out, double & p_timestamp_delta) = 0;

	//! This function is used to signal dynamic live stream song titles etc. Called after each run() (or not called at all if caller doesn't care about dynamic info). The difference between this and get_dynamic_info() is frequency and relevance of dynamic info changes - get_dynamic_info_track() returns new info only on track change in the stream, returning new titles etc.
	//! @param p_out Initially contains currently displayed info (either last get_dynamic_info_track result or current cached info), use this object to return changed info.
	//! @param p_timestamp_delta Indicates when returned info should be displayed (in seconds, relative to first sample of last decoded chunk), initially set to 0.
	//! @returns false to keep old info, or true to indicate that changes have been made to p_info and those should be displayed.
	virtual bool get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) = 0;

	//! Called from playback thread before sleeping.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void on_idle(abort_callback & p_abort) = 0;


	FB2K_MAKE_SERVICE_INTERFACE(input_decoder,input_info_reader);
};


class NOVTABLE input_decoder_v2 : public input_decoder {
	FB2K_MAKE_SERVICE_INTERFACE(input_decoder_v2, input_decoder)
public:

	//! OPTIONAL, throws pfc::exception_not_implemented() when not supported by this implementation.
	//! Special version of run(). Returns an audio_chunk object as well as a raw data block containing original PCM stream. This is mainly used for MD5 checks on lossless formats. \n
	//! If you set a "MD5" tech info entry in get_info(), you should make sure that run_raw() returns data stream that can be used to verify it. \n
	//! Returned raw data should be possible to cut into individual samples; size in bytes should be divisible by audio_chunk's sample count for splitting in case partial output is needed (with cuesheets etc).
	virtual bool run_raw(audio_chunk & out, mem_block_container & outRaw, abort_callback & abort) = 0;

	//! OPTIONAL, the call is ignored if this implementation doesn't support status logging. \n
	//! Mainly used to generate logs when ripping CDs etc.
	virtual void set_logger(event_logger::ptr ptr) = 0;
};

class NOVTABLE input_decoder_v3 : public input_decoder_v2 {
	FB2K_MAKE_SERVICE_INTERFACE(input_decoder_v3, input_decoder_v2);
public:
	//! OBSOLETE, functionality implemented by core.
	virtual void set_pause(bool paused) = 0;
	//! OPTIONAL, should return false in most cases; return true to force playback buffer flush on unpause. Valid only after initialize() with input_flag_playback.
	virtual bool flush_on_pause() = 0;
};

class NOVTABLE input_decoder_v4 : public input_decoder_v3 {
    FB2K_MAKE_SERVICE_INTERFACE( input_decoder_v4, input_decoder_v3 );
public:
    //! OPTIONAL, return 0 if not implemented. \n
    //! Provides means for communication of context specific data with the decoder. The decoder should do nothing and return 0 if it does not recognize the passed arguments.
    virtual size_t extended_param( const GUID & type, size_t arg1, void * arg2, size_t arg2size) = 0;
};

//! Parameter GUIDs for input_decoder_v3::extended_param().
class input_params {
public:
    //! Signals whether unnecessary seeking should be avoided with this decoder for performance reasons. \n
    //! Arguments disregarded, return value 1 or 0.
    static const GUID seeking_expensive;

	//! Tells the decoder to output at this sample rate if the decoder's sample rate is adjustable. \n
	//! Sample rate signaled in arg1.
	static const GUID set_preferred_sample_rate;
};

//! Class providing interface for writing metadata and replaygain info to files. Also see: file_info. \n
//! Instantiating: see input_entry.\n
//! Implementing: see input_impl.

class NOVTABLE input_info_writer : public input_info_reader
{
public:
	//! Tells the service to update file tags with new info for specified subsong.
	//! @param p_subsong Subsong to update. Should be always 0 for non-multisubsong-enabled inputs.
	//! @param p_info New info to write. Sometimes not all contents of p_info can be written. Caller will typically read info back after successful write, so e.g. tech infos that change with retag are properly maintained.
	//! @param p_abort abort_callback object signaling user aborting the operation. WARNING: abort_callback object is provided for consistency; if writing tags actually gets aborted, user will be likely left with corrupted file. Anything calling this should make sure that aborting is either impossible, or gives appropriate warning to the user first.
	virtual void set_info(t_uint32 p_subsong,const file_info & p_info,abort_callback & p_abort) = 0;
	
	//! Commits pending updates. In case of multisubsong inputs, set_info should queue the update and perform actual file access in commit(). Otherwise, actual writing can be done in set_info() and then Commit() can just do nothing and always succeed.
	//! @param p_abort abort_callback object signaling user aborting the operation. WARNING: abort_callback object is provided for consistency; if writing tags actually gets aborted, user will be likely left with corrupted file. Anything calling this should make sure that aborting is either impossible, or gives appropriate warning to the user first.
	virtual void commit(abort_callback & p_abort) = 0;

	FB2K_MAKE_SERVICE_INTERFACE(input_info_writer,input_info_reader);
};

class NOVTABLE input_info_writer_v2 : public input_info_writer {
public:
	//! Removes all tags from this file. Cancels any set_info() requests on this object. Does not require a commit() afterwards.
	virtual void remove_tags(abort_callback & abort) = 0;

	FB2K_MAKE_SERVICE_INTERFACE(input_info_writer_v2, input_info_writer);
};

class NOVTABLE input_entry : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(input_entry);
public:
	//! Determines whether specified content type can be handled by this input.
	//! @param p_type Content type string to test.
	virtual bool is_our_content_type(const char * p_type)=0;

	//! Determines whether specified file type can be handled by this input. This must not use any kind of file access; the result should be only based on file path / extension.
	//! @param p_full_path Full URL of file being tested.
	//! @param p_extension Extension of file being tested, provided by caller for performance reasons.
	virtual bool is_our_path(const char * p_full_path,const char * p_extension)=0;
	
	//! Opens specified resource for decoding.
	//! @param p_instance Receives new input_decoder instance if successful.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the service will handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example).
	//! @param p_path URL of resource being opened.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void open_for_decoding(service_ptr_t<input_decoder> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) = 0;

	//! Opens specified file for reading info.
	//! @param p_instance Receives new input_info_reader instance if successful.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the service will handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example).
	//! @param p_path URL of resource being opened.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void open_for_info_read(service_ptr_t<input_info_reader> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) = 0;

	//! Opens specified file for writing info.
	//! @param p_instance Receives new input_info_writer instance if successful.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the service will handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example).
	//! @param p_path URL of resource being opened.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void open_for_info_write(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) = 0;

	//! Reserved for future use. Do nothing and return until specifications are finalized.
	virtual void get_extended_data(service_ptr_t<file> p_filehint,const playable_location & p_location,const GUID & p_guid,mem_block_container & p_out,abort_callback & p_abort) = 0;

	enum {
		//! Indicates that this service implements some kind of redirector that opens another input for decoding, used to avoid circular call possibility.
		flag_redirect = 1,
		//! Indicates that multi-CPU optimizations should not be used.
		flag_parallel_reads_slow = 2,
	};
	//! See flag_* enums.
	virtual unsigned get_flags() = 0;

	inline bool is_redirect() {return (get_flags() & flag_redirect) != 0;}
	inline bool are_parallel_reads_slow() {return (get_flags() & flag_parallel_reads_slow) != 0;}

	static bool g_find_service_by_path(service_ptr_t<input_entry> & p_out,const char * p_path);
    static bool g_find_service_by_path(service_ptr_t<input_entry> & p_out,const char * p_path, const char * p_ext);
	static bool g_find_service_by_content_type(service_ptr_t<input_entry> & p_out,const char * p_content_type);
	static void g_open_for_decoding(service_ptr_t<input_decoder> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,bool p_from_redirect = false);
	static void g_open_for_info_read(service_ptr_t<input_info_reader> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,bool p_from_redirect = false);
	static void g_open_for_info_write(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,bool p_from_redirect = false);
	static void g_open_for_info_write_timeout(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,double p_timeout,bool p_from_redirect = false);
	static bool g_is_supported_path(const char * p_path);
	static bool g_find_inputs_by_content_type(pfc::list_base_t<service_ptr_t<input_entry> > & p_out, const char * p_content_type, bool p_from_redirect);
	static bool g_find_inputs_by_path(pfc::list_base_t<service_ptr_t<input_entry> > & p_out, const char * p_path, bool p_from_redirect);
	static service_ptr g_open(const GUID & whatFor, file::ptr hint, const char * path, abort_callback & aborter, bool fromRedirect = false);

	void open(service_ptr_t<input_decoder> & p_instance,service_ptr_t<file> const & p_filehint,const char * p_path,abort_callback & p_abort) {open_for_decoding(p_instance,p_filehint,p_path,p_abort);}
	void open(service_ptr_t<input_info_reader> & p_instance,service_ptr_t<file> const & p_filehint,const char * p_path,abort_callback & p_abort) {open_for_info_read(p_instance,p_filehint,p_path,p_abort);}
	void open(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> const & p_filehint,const char * p_path,abort_callback & p_abort) {open_for_info_write(p_instance,p_filehint,p_path,p_abort);}
	service_ptr open(const GUID & whatFor, file::ptr hint, const char * path, abort_callback & aborter);

	typedef pfc::list_base_const_t< input_entry::ptr > input_entry_list_t;

	static service_ptr g_open_from_list(input_entry_list_t const & list, const GUID & whatFor, file::ptr hint, const char * path, abort_callback & aborter, GUID * outGUID = nullptr);
	
};

//! \since 1.4
class input_entry_v2 : public input_entry {
	FB2K_MAKE_SERVICE_INTERFACE(input_entry_v2, input_entry);
public:
#ifdef FOOBAR2000_DESKTOP // none of this is used in fb2k mobile
	//! @returns GUID used to identify us among other deciders in the decoder priority table.
	virtual GUID get_guid() = 0;
	//! @returns Name to present to the user in the decoder priority table.
	virtual const char * get_name() = 0;
	//! @returns GUID of this decoder's preferences page (optional), null guid if there's no page to present
	virtual GUID get_preferences_guid() = 0;
	//! @returns true if the decoder should be put at the end of the list when it's first sighted, false otherwise (will be put at the beginning of the list).
	virtual bool is_low_merit() = 0;
#endif
};

#ifdef FOOBAR2000_DESKTOP
//! \since 1.4
class input_manager : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(input_manager);
public:
	virtual service_ptr open(const GUID & whatFor, file::ptr hint, const char * path, bool fromRedirect, abort_callback & aborter, GUID * outUsedEntry = nullptr) = 0;
};

//! \since 1.4
class input_stream_selector : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(input_stream_selector);
public:
	//! Returns index of stream that should be presented for this file. \n
	//! If not set by user, 0xFFFFFFFF will be returned and the default stream should be presented. \n
	//! @param guid GUID of the input asking for the stream.
	virtual uint32_t select_stream( const GUID & guid, const char * path ) = 0;
};

class input_stream_info_reader : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(input_stream_info_reader, service_base);
public:
	virtual uint32_t get_stream_count() = 0;
	virtual void get_stream_info(uint32_t index, file_info & out, abort_callback & aborter) = 0;
	virtual uint32_t get_default_stream() = 0;
};

class input_stream_info_reader_entry : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(input_stream_info_reader_entry);
public:
	virtual input_stream_info_reader::ptr open( const char * path, file::ptr fileHint, abort_callback & abort ) = 0;

	//! Return GUID of the matching input_entry.
	virtual GUID get_guid() = 0;

};
#endif
