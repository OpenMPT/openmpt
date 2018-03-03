#include "foobar2000.h"

int playable_location::g_compare(const playable_location & p_item1,const playable_location & p_item2) {
	int ret = metadb::path_compare(p_item1.get_path(),p_item2.get_path());
	if (ret != 0) return ret;
	return pfc::compare_t(p_item1.get_subsong(),p_item2.get_subsong());
}
int playable_location::path_compare( const char * p1, const char * p2 ) {
	return metadb::path_compare(p1, p2);
}

bool playable_location::g_equals( const playable_location & p_item1, const playable_location & p_item2) {
    return g_compare(p_item1, p_item2) == 0;
}

pfc::string_base & operator<<(pfc::string_base & p_fmt,const playable_location & p_location)
{
	p_fmt << "\"" << file_path_display(p_location.get_path()) << "\"";
	t_uint32 index = p_location.get_subsong_index();
	if (index != 0) p_fmt << " / index: " << p_location.get_subsong_index();
	return p_fmt;
}


bool playable_location::operator==(const playable_location & p_other) const {
	return metadb::path_compare(get_path(),p_other.get_path()) == 0 && get_subsong() == p_other.get_subsong();
}
bool playable_location::operator!=(const playable_location & p_other) const {
	return !(*this == p_other);
}

void playable_location::reset() {
	set_path("");set_subsong(0);
}

bool playable_location::is_empty() const { 
	return * get_path() == 0; 
}

bool playable_location::is_valid() const { 
	return !is_empty(); 
}

const char * playable_location_impl::get_path() const {
	return m_path;
}

void playable_location_impl::set_path(const char* p_path) {
	m_path=p_path;
}

t_uint32 playable_location_impl::get_subsong() const {
	return m_subsong;
}

void playable_location_impl::set_subsong(t_uint32 p_subsong) {
	m_subsong=p_subsong;
}

const playable_location_impl & playable_location_impl::operator=(const playable_location & src) {
	copy(src);return *this;
}

playable_location_impl::playable_location_impl() : m_subsong(0) {}
playable_location_impl::playable_location_impl(const char * p_path,t_uint32 p_subsong) : m_path(p_path), m_subsong(p_subsong) {}
playable_location_impl::playable_location_impl(const playable_location & src) {copy(src);}



void make_playable_location::set_path(const char*) {throw pfc::exception_not_implemented();}
void make_playable_location::set_subsong(t_uint32) {throw pfc::exception_not_implemented();}

const char * make_playable_location::get_path() const {return path;}
t_uint32 make_playable_location::get_subsong() const {return num;}

make_playable_location::make_playable_location(const char * p_path,t_uint32 p_num) : path(p_path), num(p_num) {}
