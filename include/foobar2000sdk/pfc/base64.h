#pragma once

namespace pfc {
    class string_base;
	void base64_encode(pfc::string_base & out, const void * in, t_size inSize);
	void base64_encode_append(pfc::string_base & out, const void * in, t_size inSize);
	void base64_encode_from_string( pfc::string_base & out, const char * in );
	t_size base64_decode_estimate(const char * text);
	void base64_decode(const char * text, void * out);
	
	template<typename t_buffer> void base64_decode_array(t_buffer & out, const char * text) {
		PFC_STATIC_ASSERT( sizeof(out[0]) == 1 );
		out.set_size_discard( base64_decode_estimate(text) );
		base64_decode(text, out.get_ptr());
	}

	void base64_decode_to_string( pfc::string_base & out, const char * text );
}
