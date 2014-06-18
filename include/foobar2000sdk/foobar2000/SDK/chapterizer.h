//! Interface for object storing list of chapters.
class NOVTABLE chapter_list {
public:
	//! Returns number of chapters.
	virtual t_size get_chapter_count() const = 0;
	//! Queries description of specified chapter.
	//! @param p_chapter Index of chapter to query, greater or equal zero and less than get_chapter_count() value. If p_chapter value is out of valid range, results are undefined (e.g. crash).
	//! @returns reference to file_info object describing specified chapter (length part of file_info indicates distance between beginning of this chapter and next chapter mark). Returned reference value for temporary use only, becomes invalid after any non-const operation on the chapter_list object.
	virtual const file_info & get_info(t_size p_chapter) const = 0;
	
	//! Sets number of chapters.
	virtual void set_chapter_count(t_size p_count) = 0;
	//! Modifies description of specified chapter.
	//! @param p_chapter_index Index of chapter to modify, greater or equal zero and less than get_chapter_count() value. If p_chapter value is out of valid range, results are undefined (e.g. crash).
	//! @param p_info New chapter description. Note that length part of file_info is used to calculate chapter marks.
	virtual void set_info(t_size p_chapter,const file_info & p_info) = 0;

	virtual double get_pregap() const = 0;
	virtual void set_pregap(double val) = 0;

	//! Copies contents of specified chapter_list object to this object.
	void copy(const chapter_list & p_source);
	
	inline const chapter_list & operator=(const chapter_list & p_source) {copy(p_source); return *this;}

protected:
	chapter_list() {}
	~chapter_list() {}
};

//! Implements chapter_list.
template<typename file_info_ = file_info_impl> 
class chapter_list_impl_t : public chapter_list {
public:
	chapter_list_impl_t() : m_pregap() {}
	typedef chapter_list_impl_t<file_info_> t_self;
	chapter_list_impl_t(const chapter_list & p_source) : m_pregap() {copy(p_source);}

	const t_self & operator=(const chapter_list & p_source) {copy(p_source); return *this;}

	t_size get_chapter_count() const {return m_infos.get_size();}
	const file_info & get_info(t_size p_chapter) const {return m_infos[p_chapter];}

	void set_chapter_count(t_size p_count) {m_infos.set_size(p_count);}
	void set_info(t_size p_chapter,const file_info & p_info) {m_infos[p_chapter] = p_info;}
	file_info_ & get_info_(t_size p_chapter) {return m_infos[p_chapter];}

	double get_pregap() const {return m_pregap;}
	void set_pregap(double val) {PFC_ASSERT(val >= 0); m_pregap = val;}
private:
	pfc::array_t<file_info_> m_infos;
	double m_pregap;
};

typedef chapter_list_impl_t<> chapter_list_impl;


//! This service implements chapter list editing operations for various file formats, e.g. for MP4 chapters or CD images with embedded cuesheets. Used by converter "encode single file with chapters" feature.
class NOVTABLE chapterizer : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(chapterizer);
public:
	//! Tests whether specified path is supported by this implementation.
	//! @param p_ext Extension of the file being processed.
	virtual bool is_our_path(const char * p_path) = 0;
	
	//! Writes new chapter list to specified file.
	//! @param p_path Path of file to modify.
	//! @param p_list New chapter list to write.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void set_chapters(const char * p_path,chapter_list const & p_list,abort_callback & p_abort) = 0;
	//! Retrieves chapter list from specified file.
	//! @param p_path Path of file to examine.
	//! @param p_list Object receiving chapter list.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void get_chapters(const char * p_path,chapter_list & p_list,abort_callback & p_abort) = 0;

	virtual bool supports_pregaps() = 0;

	//! Static helper, tries to find chapterizer interface that supports specified file.
	static bool g_find(service_ptr_t<chapterizer> & p_out,const char * p_path);

	static bool g_is_pregap_capable(const char * p_path);
};



unsigned cuesheet_parse_index_time_ticks_e(const char * p_string,t_size p_length);
double cuesheet_parse_index_time_e(const char * p_string,t_size p_length);

class cuesheet_format_index_time
{
public:
	cuesheet_format_index_time(double p_time);
	inline operator const char*() const {return m_buffer;}
private:
	pfc::string_formatter m_buffer;
};
