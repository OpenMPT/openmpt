#pragma once

//playable_location stores location of a playable resource, currently implemented as file path and integer for indicating multiple playable "subsongs" per file
//also see: file_info.h
//for getting more info about resource referenced by a playable_location, see metadb.h

//char* strings are all UTF-8

class NOVTABLE playable_location//interface (for passing around between DLLs)
{
public:
	virtual const char * get_path() const = 0;
	virtual void set_path(const char*) = 0;
	virtual t_uint32 get_subsong() const = 0;
	virtual void set_subsong(t_uint32) = 0;
	
	void copy(const playable_location & p_other) {
		set_path(p_other.get_path());
		set_subsong(p_other.get_subsong());
	}

	static int g_compare(const playable_location & p_item1,const playable_location & p_item2);
    static bool g_equals( const playable_location & p_item1, const playable_location & p_item2);

	const playable_location & operator=(const playable_location & src) {copy(src);return *this;}	

	bool operator==(const playable_location & p_other) const;
	bool operator!=(const playable_location & p_other) const;

	void reset();
	
	inline t_uint32 get_subsong_index() const {return get_subsong();}
	inline void set_subsong_index(t_uint32 v) {set_subsong(v);}

	bool is_empty() const;
	bool is_valid() const;


	class comparator {
	public:
		static int compare(const playable_location & v1, const playable_location & v2) {return g_compare(v1,v2);}
	};
	static int path_compare( const char * p1, const char * p2 );

protected:
	playable_location() {}
	~playable_location() {}
};

typedef playable_location * pplayable_location;
typedef playable_location const * pcplayable_location;
typedef playable_location & rplayable_location;
typedef playable_location const & rcplayable_location;

class playable_location_impl : public playable_location//implementation
{
public:
	virtual const char * get_path() const;
	virtual void set_path(const char* p_path);
	virtual t_uint32 get_subsong() const;
	virtual void set_subsong(t_uint32 p_subsong);

	const playable_location_impl & operator=(const playable_location & src);

	playable_location_impl();
	playable_location_impl(const char * p_path,t_uint32 p_subsong);
	playable_location_impl(const playable_location & src);
private:
	pfc::string_simple m_path;
	t_uint32 m_subsong;
};

// usage: somefunction( make_playable_location("file://c:\blah.ogg",0) );
// only for use as a parameter to a function taking const playable_location &
class make_playable_location : public playable_location
{
	const char * path;
	t_uint32 num;
	
	virtual void set_path(const char*);
	virtual void set_subsong(t_uint32);
public:
	virtual const char * get_path() const;
	virtual t_uint32 get_subsong() const;

	make_playable_location(const char * p_path,t_uint32 p_num);
};

pfc::string_base & operator<<(pfc::string_base & p_fmt,const playable_location & p_location);
