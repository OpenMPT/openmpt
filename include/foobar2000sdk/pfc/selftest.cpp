#include "pfc.h"


namespace {
    class foo {};
}

inline pfc::string_base & operator<<(pfc::string_base & p_fmt,foo p_source) {p_fmt.add_string_("FOO"); return p_fmt;}

namespace pfc {
    void selftest();
}

void pfc::selftest() //never called, testing done at compile time
{
	PFC_STATIC_ASSERT( sizeof(t_uint8) == 1 );
	PFC_STATIC_ASSERT( sizeof(t_uint16) == 2 );
	PFC_STATIC_ASSERT( sizeof(t_uint32) == 4 );
	PFC_STATIC_ASSERT( sizeof(t_uint64) == 8 );

	PFC_STATIC_ASSERT( sizeof(t_int8) == 1 );
	PFC_STATIC_ASSERT( sizeof(t_int16) == 2 );
	PFC_STATIC_ASSERT( sizeof(t_int32) == 4 );
	PFC_STATIC_ASSERT( sizeof(t_int64) == 8 );

	PFC_STATIC_ASSERT( sizeof(t_float32) == 4 );
	PFC_STATIC_ASSERT( sizeof(t_float64) == 8 );

	PFC_STATIC_ASSERT( sizeof(t_size) == sizeof(void*) );
	PFC_STATIC_ASSERT( sizeof(t_ssize) == sizeof(void*) );

	PFC_STATIC_ASSERT( sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4 );

	PFC_STATIC_ASSERT( sizeof(GUID) == 16 );

	typedef pfc::avltree_t<int> t_asdf;
	t_asdf asdf; asdf.add_item(1);
	t_asdf::iterator iter = asdf._first_var();
	t_asdf::const_iterator iter2 = asdf._first_var();
    
    PFC_string_formatter() << "foo" << 1337 << foo();
    
    pfc::list_t<int> l; l.add_item(3);

}
