#include "pfc.h"

namespace pfc {
//utf8 stuff
#include "pocket_char_ops.h"

#ifdef _MSC_VER
    t_size utf16_decode_char(const wchar_t * p_source,unsigned * p_out,t_size p_source_length) throw() {
        PFC_STATIC_ASSERT( sizeof(wchar_t) == sizeof(char16_t) );
        return wide_decode_char( p_source, p_out, p_source_length );
    }
    t_size utf16_encode_char(unsigned c,wchar_t * out) throw() {
        PFC_STATIC_ASSERT( sizeof(wchar_t) == sizeof(char16_t) );
        return wide_encode_char( c, out );
    }
#endif

    t_size wide_decode_char(const wchar_t * p_source,unsigned * p_out,t_size p_source_length) throw() {
        PFC_STATIC_ASSERT( sizeof( wchar_t ) == sizeof( char16_t ) || sizeof( wchar_t ) == sizeof( unsigned ) );
        if (sizeof( wchar_t ) == sizeof( char16_t ) ) {
            return utf16_decode_char( reinterpret_cast< const char16_t *>(p_source), p_out, p_source_length );
        } else {
            if (p_source_length == 0) { * p_out = 0; return 0; }
            * p_out = p_source [ 0 ];
            return 1;
        }
    }
	t_size wide_encode_char(unsigned c,wchar_t * out) throw() {
        PFC_STATIC_ASSERT( sizeof( wchar_t ) == sizeof( char16_t ) || sizeof( wchar_t ) == sizeof( unsigned ) );
        if (sizeof( wchar_t ) == sizeof( char16_t ) ) {
            return utf16_encode_char( c, reinterpret_cast< char16_t * >(out) );
        } else {
            * out = (wchar_t) c;
            return 1;
        }
    }



bool is_lower_ascii(const char * param)
{
	while(*param)
	{
		if (*param<0) return false;
		param++;
	}
	return true;
}

static bool check_end_of_string(const char * ptr)
{
	return !*ptr;
}

unsigned strcpy_utf8_truncate(const char * src,char * out,unsigned maxbytes)
{
	unsigned rv = 0 , ptr = 0;
	if (maxbytes>0)
	{	
		maxbytes--;//for null
		while(!check_end_of_string(src) && maxbytes>0)
		{
            t_size delta = utf8_char_len(src);
            if (delta>maxbytes || delta==0) break;
			maxbytes -= delta;
            do 
            {
                out[ptr++] = *(src++);
            } while(--delta);
			rv = ptr;
		}
		out[rv]=0;
	}
	return rv;
}

t_size strlen_utf8(const char * p,t_size num) throw()
{
	unsigned w;
	t_size d;
	t_size ret = 0;
	for(;num;)
	{
		d = utf8_decode_char(p,w);
		if (w==0 || d<=0) break;
		ret++;
		p+=d;
		num-=d;
	}
	return ret;
}

t_size utf8_chars_to_bytes(const char * string,t_size count) throw()
{
	t_size bytes = 0;
	while(count)
	{
		unsigned dummy;
		t_size delta = utf8_decode_char(string+bytes,dummy);
		if (delta==0) break;
		bytes += delta;
		count--;
	}
	return bytes;
}

}