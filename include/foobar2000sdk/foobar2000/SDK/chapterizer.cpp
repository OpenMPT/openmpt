#include "foobar2000.h"
#include "chapterizer.h"

void chapter_list::copy(const chapter_list & p_source)
{
	t_size n, count = p_source.get_chapter_count();
	set_chapter_count(count);
	for(n=0;n<count;n++) set_info(n,p_source.get_info(n));
	set_pregap(p_source.get_pregap());
}

#ifdef FOOBAR2000_HAVE_CHAPTERIZER

// {3F489088-6179-434e-A9DB-3A14A1B081AC}
FOOGUIDDECL const GUID chapterizer::class_guid=
{ 0x3f489088, 0x6179, 0x434e, { 0xa9, 0xdb, 0x3a, 0x14, 0xa1, 0xb0, 0x81, 0xac } };

bool chapterizer::g_find(service_ptr_t<chapterizer> & p_out,const char * p_path)
{
	service_ptr_t<chapterizer> ptr;
	service_enum_t<chapterizer> e;
	while(e.next(ptr)) {
		if (ptr->is_our_path(p_path)) {
			p_out = ptr;
			return true;
		}
	}
	return false;
}

bool chapterizer::g_is_pregap_capable(const char * p_path) {
	service_ptr_t<chapterizer> ptr;
	service_enum_t<chapterizer> e;
	while(e.next(ptr)) {
		if (ptr->supports_pregaps() && ptr->is_our_path(p_path)) {
			return true;
		}
	}
	return false;
}

#endif

