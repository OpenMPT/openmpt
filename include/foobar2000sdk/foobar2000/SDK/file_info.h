//! Structure containing ReplayGain scan results from some playable object, also providing various helper methods to manipulate those results.
struct replaygain_info
{
	float m_album_gain,m_track_gain;
	float m_album_peak,m_track_peak;

	enum {text_buffer_size = 16 };
	typedef char t_text_buffer[text_buffer_size];

	enum { peak_invalid = -1, gain_invalid = -1000 };

	static bool g_format_gain(float p_value,char p_buffer[text_buffer_size]);
	static bool g_format_peak(float p_value,char p_buffer[text_buffer_size]);

	inline bool format_album_gain(char p_buffer[text_buffer_size]) const {return g_format_gain(m_album_gain,p_buffer);}
	inline bool format_track_gain(char p_buffer[text_buffer_size]) const {return g_format_gain(m_track_gain,p_buffer);}
	inline bool format_album_peak(char p_buffer[text_buffer_size]) const {return g_format_peak(m_album_peak,p_buffer);}
	inline bool format_track_peak(char p_buffer[text_buffer_size]) const {return g_format_peak(m_track_peak,p_buffer);}

	void set_album_gain_text(const char * p_text,t_size p_text_len = pfc_infinite);
	void set_track_gain_text(const char * p_text,t_size p_text_len = pfc_infinite);
	void set_album_peak_text(const char * p_text,t_size p_text_len = pfc_infinite);
	void set_track_peak_text(const char * p_text,t_size p_text_len = pfc_infinite);

	static bool g_is_meta_replaygain(const char * p_name,t_size p_name_len = pfc_infinite);
	bool set_from_meta_ex(const char * p_name,t_size p_name_len,const char * p_value,t_size p_value_len);
	inline bool set_from_meta(const char * p_name,const char * p_value) {return set_from_meta_ex(p_name,pfc_infinite,p_value,pfc_infinite);}

	inline bool is_album_gain_present() const {return m_album_gain != gain_invalid;}
	inline bool is_track_gain_present() const {return m_track_gain != gain_invalid;}
	inline bool is_album_peak_present() const {return m_album_peak != peak_invalid;}
	inline bool is_track_peak_present() const {return m_track_peak != peak_invalid;}
	
	inline void remove_album_gain() {m_album_gain = gain_invalid;}
	inline void remove_track_gain() {m_track_gain = gain_invalid;}
	inline void remove_album_peak() {m_album_peak = peak_invalid;}
	inline void remove_track_peak() {m_track_peak = peak_invalid;}

	t_size	get_value_count();

	static replaygain_info g_merge(replaygain_info r1,replaygain_info r2);

	static bool g_equal(const replaygain_info & item1,const replaygain_info & item2);

	void reset();
};

class format_rg_gain {
public:
	format_rg_gain(float val) {replaygain_info::g_format_gain(val, m_buffer);}

	operator const char * () const {return m_buffer;}
private:
	replaygain_info::t_text_buffer m_buffer;
};

class format_rg_peak {
public:
	format_rg_peak(float val) {replaygain_info::g_format_peak(val, m_buffer);}

	operator const char * () const {return m_buffer;}
private:
	replaygain_info::t_text_buffer m_buffer;
};

inline bool operator==(const replaygain_info & item1,const replaygain_info & item2) {return replaygain_info::g_equal(item1,item2);}
inline bool operator!=(const replaygain_info & item1,const replaygain_info & item2) {return !replaygain_info::g_equal(item1,item2);}

static const replaygain_info replaygain_info_invalid = {replaygain_info::gain_invalid,replaygain_info::gain_invalid,replaygain_info::peak_invalid,replaygain_info::peak_invalid};


//! Main interface class for information about some playable object.
class NOVTABLE file_info {
public:
	//! Retrieves audio duration, in seconds. \n
	//! Note that the reported duration should not be assumed to be the exact length of the track -\n
	//! with many popular audio formats, exact duration is impossible to determine without performing a full decode pass;\n
	//! with other formats, the decoded data may be shorter than reported due to truncation other damage.
	virtual double		get_length() const = 0;
	//! Sets audio duration, in seconds. \n
	//! Note that the reported duration should not be assumed to be the exact length of the track -\n
	//! with many popular audio formats, exact duration is impossible to determine without performing a full decode pass;\n
	//! with other formats, the decoded data may be shorter than reported due to truncation other damage.
	virtual void		set_length(double p_length) = 0;

	//! Sets ReplayGain information.
	virtual void		set_replaygain(const replaygain_info & p_info) = 0;
	//! Retrieves ReplayGain information.
	virtual replaygain_info	get_replaygain() const = 0;

	//! Retrieves count of metadata entries.
	virtual t_size		meta_get_count() const = 0;
	//! Retrieves the name of metadata entry of specified index. Return value is a null-terminated UTF-8 encoded string.
	virtual const char*	meta_enum_name(t_size p_index) const = 0;
	//! Retrieves count of values in metadata entry of specified index. The value is always equal to or greater than 1.
	virtual t_size		meta_enum_value_count(t_size p_index) const = 0;
	//! Retrieves specified value from specified metadata entry. Return value is a null-terminated UTF-8 encoded string.
	virtual const char*	meta_enum_value(t_size p_index,t_size p_value_number) const = 0;
	//! Finds index of metadata entry of specified name. Returns infinite when not found.
	virtual t_size		meta_find_ex(const char * p_name,t_size p_name_length) const;
	//! Creates a new metadata entry of specified name with specified value. If an entry of same name already exists, it is erased. Return value is the index of newly created metadata entry.
	virtual t_size		meta_set_ex(const char * p_name,t_size p_name_length,const char * p_value,t_size p_value_length) = 0;
	//! Inserts a new value into specified metadata entry.
	virtual void		meta_insert_value_ex(t_size p_index,t_size p_value_index,const char * p_value,t_size p_value_length) = 0;
	//! Removes metadata entries according to specified bit mask.
	virtual void		meta_remove_mask(const bit_array & p_mask) = 0;
	//! Reorders metadata entries according to specified permutation.
	virtual void		meta_reorder(const t_size * p_order) = 0;
	//! Removes values according to specified bit mask from specified metadata entry. If all values are removed, entire metadata entry is removed as well.
	virtual void		meta_remove_values(t_size p_index,const bit_array & p_mask) = 0;
	//! Alters specified value in specified metadata entry.
	virtual void		meta_modify_value_ex(t_size p_index,t_size p_value_index,const char * p_value,t_size p_value_length) = 0;

	//! Retrieves number of technical info entries.
	virtual t_size		info_get_count() const = 0;
	//! Retrieves the name of specified technical info entry. Return value is a null-terminated UTF-8 encoded string.
	virtual const char*	info_enum_name(t_size p_index) const = 0;
	//! Retrieves the value of specified technical info entry. Return value is a null-terminated UTF-8 encoded string.
	virtual const char*	info_enum_value(t_size p_index) const = 0;
	//! Creates a new technical info entry with specified name and specified value. If an entry of the same name already exists, it is erased. Return value is the index of newly created entry.
	virtual t_size		info_set_ex(const char * p_name,t_size p_name_length,const char * p_value,t_size p_value_length) = 0;
	//! Removes technical info entries indicated by specified bit mask.
	virtual void		info_remove_mask(const bit_array & p_mask) = 0;
	//! Finds technical info entry of specified name. Returns index of found entry on success, infinite on failure.
	virtual t_size		info_find_ex(const char * p_name,t_size p_name_length) const;

	//! Copies entire file_info contents from specified file_info object.
	virtual void		copy(const file_info & p_source);//virtualized for performance reasons, can be faster in two-pass
	//! Copies metadata from specified file_info object.
	virtual void		copy_meta(const file_info & p_source);//virtualized for performance reasons, can be faster in two-pass
	//! Copies technical info from specified file_info object.
	virtual void		copy_info(const file_info & p_source);//virtualized for performance reasons, can be faster in two-pass

	bool			meta_exists_ex(const char * p_name,t_size p_name_length) const;
	void			meta_remove_field_ex(const char * p_name,t_size p_name_length);
	void			meta_remove_index(t_size p_index);
	void			meta_remove_all();
	void			meta_remove_value(t_size p_index,t_size p_value);
	const char *	meta_get_ex(const char * p_name,t_size p_name_length,t_size p_index) const;
	t_size			meta_get_count_by_name_ex(const char * p_name,t_size p_name_length) const;
	void			meta_add_value_ex(t_size p_index,const char * p_value,t_size p_value_length);
	t_size			meta_add_ex(const char * p_name,t_size p_name_length,const char * p_value,t_size p_value_length);
	t_size			meta_calc_total_value_count() const;
	bool			meta_format(const char * p_name,pfc::string_base & p_out, const char * separator = ", ") const;
	void			meta_format_entry(t_size index, pfc::string_base & p_out, const char * separator = ", ") const;//same as meta_format but takes index instead of meta name.
	
	
	bool			info_exists_ex(const char * p_name,t_size p_name_length) const;
	void			info_remove_index(t_size p_index);
	void			info_remove_all();
	bool			info_remove_ex(const char * p_name,t_size p_name_length);
	const char *	info_get_ex(const char * p_name,t_size p_name_length) const;

	inline t_size		meta_find(const char * p_name) const	{return meta_find_ex(p_name,pfc_infinite);}
	inline bool			meta_exists(const char * p_name) const		{return meta_exists_ex(p_name,pfc_infinite);}
	inline void			meta_remove_field(const char * p_name)		{meta_remove_field_ex(p_name,pfc_infinite);}
	inline t_size		meta_set(const char * p_name,const char * p_value)		{return meta_set_ex(p_name,pfc_infinite,p_value,pfc_infinite);}
	inline void			meta_insert_value(t_size p_index,t_size p_value_index,const char * p_value) {meta_insert_value_ex(p_index,p_value_index,p_value,pfc_infinite);}
	inline void			meta_add_value(t_size p_index,const char * p_value)	{meta_add_value_ex(p_index,p_value,pfc_infinite);}
	inline const char*	meta_get(const char * p_name,t_size p_index) const	{return meta_get_ex(p_name,pfc_infinite,p_index);}
	inline t_size		meta_get_count_by_name(const char * p_name) const		{return meta_get_count_by_name_ex(p_name,pfc_infinite);}
	inline t_size		meta_add(const char * p_name,const char * p_value)		{return meta_add_ex(p_name,pfc_infinite,p_value,pfc_infinite);}
	inline void			meta_modify_value(t_size p_index,t_size p_value_index,const char * p_value) {meta_modify_value_ex(p_index,p_value_index,p_value,pfc_infinite);}

	

	inline t_size		info_set(const char * p_name,const char * p_value)	{return info_set_ex(p_name,pfc_infinite,p_value,pfc_infinite);}
	inline t_size		info_find(const char * p_name) const				{return info_find_ex(p_name,pfc_infinite);}
	inline bool			info_exists(const char * p_name) const				{return info_exists_ex(p_name,pfc_infinite);}
	inline bool			info_remove(const char * p_name)					{return info_remove_ex(p_name,pfc_infinite);}
	inline const char *	info_get(const char * p_name) const					{return info_get_ex(p_name,pfc_infinite);}

	bool				info_set_replaygain_ex(const char * p_name,t_size p_name_len,const char * p_value,t_size p_value_len);
	inline bool			info_set_replaygain(const char * p_name,const char * p_value) {return info_set_replaygain_ex(p_name,pfc_infinite,p_value,pfc_infinite);}
	void				info_set_replaygain_auto_ex(const char * p_name,t_size p_name_len,const char * p_value,t_size p_value_len);
	inline void			info_set_replaygain_auto(const char * p_name,const char * p_value) {info_set_replaygain_auto_ex(p_name,pfc_infinite,p_value,pfc_infinite);}

	

	void		copy_meta_single(const file_info & p_source,t_size p_index);
	void		copy_info_single(const file_info & p_source,t_size p_index);
	void		copy_meta_single_by_name_ex(const file_info & p_source,const char * p_name,t_size p_name_length);
	void		copy_info_single_by_name_ex(const file_info & p_source,const char * p_name,t_size p_name_length);
	inline void	copy_meta_single_by_name(const file_info & p_source,const char * p_name) {copy_meta_single_by_name_ex(p_source,p_name,pfc_infinite);}
	inline void	copy_info_single_by_name(const file_info & p_source,const char * p_name) {copy_info_single_by_name_ex(p_source,p_name,pfc_infinite);}
	void		reset();
	void		reset_replaygain();
	void		copy_meta_single_rename_ex(const file_info & p_source,t_size p_index,const char * p_new_name,t_size p_new_name_length);
	inline void	copy_meta_single_rename(const file_info & p_source,t_size p_index,const char * p_new_name) {copy_meta_single_rename_ex(p_source,p_index,p_new_name,pfc_infinite);}
	void		overwrite_info(const file_info & p_source);
	void		overwrite_meta(const file_info & p_source);

	t_int64 info_get_int(const char * name) const;
	t_int64 info_get_length_samples() const;
	double info_get_float(const char * name) const;
	void info_set_int(const char * name,t_int64 value);
	void info_set_float(const char * name,double value,unsigned precision,bool force_sign = false,const char * unit = 0);
	void info_set_replaygain_track_gain(float value);
	void info_set_replaygain_album_gain(float value);
	void info_set_replaygain_track_peak(float value);
	void info_set_replaygain_album_peak(float value);

	inline t_int64 info_get_bitrate_vbr() const {return info_get_int("bitrate_dynamic");}
	inline void info_set_bitrate_vbr(t_int64 val) {info_set_int("bitrate_dynamic",val);}
	inline t_int64 info_get_bitrate() const {return info_get_int("bitrate");}
	inline void info_set_bitrate(t_int64 val) {info_set_int("bitrate",val);}

	void info_set_wfx_chanMask(uint32_t val);
	uint32_t info_get_wfx_chanMask() const;

	bool is_encoding_lossy() const;


	void info_calculate_bitrate(t_filesize p_filesize,double p_length);

	unsigned info_get_decoded_bps() const;//what bps the stream originally was (before converting to audio_sample), 0 if unknown

private:
	void merge(const pfc::list_base_const_t<const file_info*> & p_sources);
public:

	void _set_tag(const file_info & tag);
	void _add_tag(const file_info & otherTag);

	void merge_fallback(const file_info & fallback);

	bool are_meta_fields_identical(t_size p_index1,t_size p_index2) const;

	inline const file_info & operator=(const file_info & p_source) {copy(p_source);return *this;}

	static bool g_is_meta_equal(const file_info & p_item1,const file_info & p_item2);
	static bool g_is_meta_equal_debug(const file_info & p_item1,const file_info & p_item2);
	static bool g_is_info_equal(const file_info & p_item1,const file_info & p_item2);

	//! Unsafe - does not check whether the field already exists and will result in duplicates if it does - call only when appropriate checks have been applied externally.
	t_size	__meta_add_unsafe_ex(const char * p_name,t_size p_name_length,const char * p_value,t_size p_value_length) {return meta_set_nocheck_ex(p_name,p_name_length,p_value,p_value_length);}
	//! Unsafe - does not check whether the field already exists and will result in duplicates if it does - call only when appropriate checks have been applied externally.
	t_size	__meta_add_unsafe(const char * p_name,const char * p_value) {return meta_set_nocheck_ex(p_name,pfc_infinite,p_value,pfc_infinite);}

	//! Unsafe - does not check whether the field already exists and will result in duplicates if it does - call only when appropriate checks have been applied externally.
	t_size __info_add_unsafe_ex(const char * p_name,t_size p_name_length,const char * p_value,t_size p_value_length) {return info_set_nocheck_ex(p_name,p_name_length,p_value,p_value_length);}
	//! Unsafe - does not check whether the field already exists and will result in duplicates if it does - call only when appropriate checks have been applied externally.
	t_size __info_add_unsafe(const char * p_name,const char * p_value) {return info_set_nocheck_ex(p_name,pfc_infinite,p_value,pfc_infinite);}

	void _copy_meta_single_nocheck(const file_info & p_source,t_size p_index) {copy_meta_single_nocheck(p_source, p_index);}

	static bool g_is_valid_field_name(const char * p_name,t_size p_length = pfc_infinite);
	//typedef pfc::comparator_stricmp_ascii field_name_comparator;
	typedef pfc::string::comparatorCaseInsensitiveASCII field_name_comparator;

	static bool field_name_equals(const char * n1, const char * n2) {return field_name_comparator::compare(n1, n2) == 0;}

	void to_console() const;
	void to_formatter(pfc::string_formatter&) const;
	static bool field_is_person(const char * fieldName);
	static bool field_is_title(const char * fieldName);

	void to_stream( stream_writer * stream, abort_callback & abort ) const;
	void from_stream( stream_reader * stream, abort_callback & abort );
	void from_mem( const void * memPtr, size_t memSize);
	
	//! Returns ESTIMATED audio chunk spec from what has been put in the file_info. \n
	//! Provided for convenience. Do not rely on it for processing decoded data.
	audio_chunk::spec_t audio_chunk_spec() const; 
protected:
	file_info() {}
	~file_info() {}
	void	copy_meta_single_nocheck(const file_info & p_source,t_size p_index);
	void	copy_info_single_nocheck(const file_info & p_source,t_size p_index);
	void	copy_meta_single_by_name_nocheck_ex(const file_info & p_source,const char * p_name,t_size p_name_length);
	void	copy_info_single_by_name_nocheck_ex(const file_info & p_source,const char * p_name,t_size p_name_length);
	inline void	copy_meta_single_by_name_nocheck(const file_info & p_source,const char * p_name) {copy_meta_single_by_name_nocheck_ex(p_source,p_name,pfc_infinite);}
	inline void	copy_info_single_by_name_nocheck(const file_info & p_source,const char * p_name) {copy_info_single_by_name_nocheck_ex(p_source,p_name,pfc_infinite);}

	virtual t_size	meta_set_nocheck_ex(const char * p_name,t_size p_name_length,const char * p_value,t_size p_value_length) = 0;
	virtual t_size	info_set_nocheck_ex(const char * p_name,t_size p_name_length,const char * p_value,t_size p_value_length) = 0;
	inline t_size	meta_set_nocheck(const char * p_name,const char * p_value) {return meta_set_nocheck_ex(p_name,pfc_infinite,p_value,pfc_infinite);}
	inline t_size	info_set_nocheck(const char * p_name,const char * p_value) {return info_set_nocheck_ex(p_name,pfc_infinite,p_value,pfc_infinite);}
};
