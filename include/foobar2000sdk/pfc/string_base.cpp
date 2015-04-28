#include "pfc.h"

namespace pfc {

void string_receiver::add_char(t_uint32 p_char)
{
	char temp[8];
	t_size len = utf8_encode_char(p_char,temp);
	if (len>0) add_string(temp,len);
}

void string_base::skip_trailing_char(unsigned skip)
{
	const char * str = get_ptr();
	t_size ptr,trunc = 0;
	bool need_trunc = false;
	for(ptr=0;str[ptr];)
	{
		unsigned c;
		t_size delta = utf8_decode_char(str+ptr,c);
		if (delta==0) break;
		if (c==skip)
		{
			need_trunc = true;
			trunc = ptr;
		}
		else
		{
			need_trunc = false;
		}
		ptr += delta;
	}
	if (need_trunc) truncate(trunc);
}

format_time::format_time(t_uint64 p_seconds) {
	t_uint64 length = p_seconds;
	unsigned weeks,days,hours,minutes,seconds;
	
	weeks = (unsigned)( ( length / (60*60*24*7) ) );
	days = (unsigned)( ( length / (60*60*24) ) % 7 );
	hours = (unsigned) ( ( length / (60 * 60) ) % 24);
	minutes = (unsigned) ( ( length / (60 ) ) % 60 );
	seconds = (unsigned) ( ( length ) % 60 );

	if (weeks) {
		m_buffer << weeks << "wk ";
	}
	if (days || weeks) {
		m_buffer << days << "d ";
	}
	if (hours || days || weeks) {
		m_buffer << hours << ":" << format_uint(minutes,2) << ":" << format_uint(seconds,2);
	} else {
		m_buffer << minutes << ":" << format_uint(seconds,2);
	}
}

bool is_path_separator(unsigned c)
{
	return c=='\\' || c=='/' || c=='|' || c==':';
}

bool is_path_bad_char(unsigned c)
{
#ifdef _WINDOWS
	return c=='\\' || c=='/' || c=='|' || c==':' || c=='*' || c=='?' || c=='\"' || c=='>' || c=='<';
#else
	return c=='/' || c=='*' || c=='?';
#endif
}



char * strdup_n(const char * src,t_size len)
{
	len = strlen_max(src,len);
	char * ret = (char*)malloc(len+1);
	if (ret)
	{
		memcpy(ret,src,len);
		ret[len]=0;
	}
	return ret;
}

string_filename::string_filename(const char * fn)
{
	fn += pfc::scan_filename(fn);
	const char * ptr=fn,*dot=0;
	while(*ptr && *ptr!='?')
	{
		if (*ptr=='.') dot=ptr;
		ptr++;
	}

	if (dot && dot>fn) set_string(fn,dot-fn);
	else set_string(fn);
}

string_filename_ext::string_filename_ext(const char * fn)
{
	fn += pfc::scan_filename(fn);
	const char * ptr = fn;
	while(*ptr && *ptr!='?') ptr++;
	set_string(fn,ptr-fn);
}

string_extension::string_extension(const char * src)
{
	buffer[0]=0;
	const char * start = src + pfc::scan_filename(src);
	const char * end = start + strlen(start);
	const char * ptr = end-1;
	while(ptr>start && *ptr!='.')
	{
		if (*ptr=='?') end=ptr;
		ptr--;
	}

	if (ptr>=start && *ptr=='.')
	{
		ptr++;
		t_size len = end-ptr;
		if (len<PFC_TABSIZE(buffer))
		{
			memcpy(buffer,ptr,len*sizeof(char));
			buffer[len]=0;
		}
	}
}


bool has_path_bad_chars(const char * param)
{
	while(*param)
	{
		if (is_path_bad_char(*param)) return true;
		param++;
	}
	return false;
}

void float_to_string(char * out,t_size out_max,double val,unsigned precision,bool b_sign) {
	pfc::string_fixed_t<63> temp;
	t_size outptr;

	if (out_max == 0) return;
	out_max--;//for null terminator
	
	outptr = 0;	

	if (outptr == out_max) {out[outptr]=0;return;}

	if (val<0) {out[outptr++] = '-'; val = -val;}
	else if (b_sign) {out[outptr++] = '+';}

	if (outptr == out_max) {out[outptr]=0;return;}

	
	{
		double powval = pow((double)10.0,(double)precision);
		temp << (t_int64)floor(val * powval + 0.5);
		//_i64toa(blargh,temp,10);
	}
	
	const t_size temp_len = temp.length();
	if (temp_len <= precision)
	{
		out[outptr++] = '0';
		if (outptr == out_max) {out[outptr]=0;return;}
		out[outptr++] = '.';
		if (outptr == out_max) {out[outptr]=0;return;}
		t_size d;
		for(d=precision-temp_len;d;d--)
		{
			out[outptr++] = '0';
			if (outptr == out_max) {out[outptr]=0;return;}
		}
		for(d=0;d<temp_len;d++)
		{
			out[outptr++] = temp[d];
			if (outptr == out_max) {out[outptr]=0;return;}
		}
	}
	else
	{
		t_size d = temp_len;
		const char * src = temp;
		while(*src)
		{
			if (d-- == precision)
			{
				out[outptr++] = '.';
				if (outptr == out_max) {out[outptr]=0;return;}
			}
			out[outptr++] = *(src++);
			if (outptr == out_max) {out[outptr]=0;return;}
		}
	}
	out[outptr] = 0;
}


    
static double pfc_string_to_float_internal(const char * src)
{
	bool neg = false;
	t_int64 val = 0;
	int div = 0;
	bool got_dot = false;

	while(*src==' ') src++;

	if (*src=='-') {neg = true;src++;}
	else if (*src=='+') src++;
	
	while(*src)
	{
		if (*src>='0' && *src<='9')
		{
			int d = *src - '0';
			val = val * 10 + d;
			if (got_dot) div--;
			src++;
		}
		else if (*src=='.' || *src==',')
		{
			if (got_dot) break;
			got_dot = true;
			src++;
		}
		else if (*src=='E' || *src=='e')
		{
			src++;
			div += atoi(src);
			break;
		}
		else break;
	}
	if (neg) val = -val;
    return (double) val * exp_int(10, div);
}

double string_to_float(const char * src,t_size max) {
	//old function wants an oldstyle nullterminated string, and i don't currently care enough to rewrite it as it works appropriately otherwise
	char blargh[128];
	if (max > 127) max = 127;
	t_size walk;
	for(walk = 0; walk < max && src[walk]; walk++) blargh[walk] = src[walk];
	blargh[walk] = 0;
	return pfc_string_to_float_internal(blargh);
}



void string_base::convert_to_lower_ascii(const char * src,char replace)
{
	reset();
	PFC_ASSERT(replace>0);
	while(*src)
	{
		unsigned c;
		t_size delta = utf8_decode_char(src,c);
		if (delta==0) {c = replace; delta = 1;}
		else if (c>=0x80) c = replace;
		add_byte((char)c);
		src += delta;
	}
}

void convert_to_lower_ascii(const char * src,t_size max,char * out,char replace)
{
	t_size ptr = 0;
	PFC_ASSERT(replace>0);
	while(ptr<max && src[ptr])
	{
		unsigned c;
		t_size delta = utf8_decode_char(src+ptr,c,max-ptr);
		if (delta==0) {c = replace; delta = 1;}
		else if (c>=0x80) c = replace;
		*(out++) = (char)c;
		ptr += delta;
	}
	*out = 0;
}

t_size strstr_ex(const char * p_string,t_size p_string_len,const char * p_substring,t_size p_substring_len) throw()
{
	p_string_len = strlen_max(p_string,p_string_len);
	p_substring_len = strlen_max(p_substring,p_substring_len);
	t_size index = 0;
	while(index + p_substring_len <= p_string_len)
	{
		if (memcmp(p_string+index,p_substring,p_substring_len) == 0) return index;
		t_size delta = utf8_char_len(p_string+index,p_string_len - index);
		if (delta == 0) break;
		index += delta;
	}
	return ~0;
}

unsigned atoui_ex(const char * p_string,t_size p_string_len)
{
	unsigned ret = 0; t_size ptr = 0;
	while(ptr<p_string_len)
	{
		char c = p_string[ptr];
		if (! ( c >= '0' && c <= '9' ) ) break;
		ret = ret * 10 + (unsigned)( c - '0' );
		ptr++;
	}
	return ret;
}

int strcmp_nc(const char* p1, size_t n1, const char * p2, size_t n2) throw() {
	t_size idx = 0;
	for(;;)
	{
		if (idx == n1 && idx == n2) return 0;
		else if (idx == n1) return -1;//end of param1
		else if (idx == n2) return 1;//end of param2

		char c1 = p1[idx], c2 = p2[idx];
		if (c1<c2) return -1;
		else if (c1>c2) return 1;
		
		idx++;
	}
}

int strcmp_ex(const char* p1,t_size n1,const char* p2,t_size n2) throw()
{
	n1 = strlen_max(p1,n1); n2 = strlen_max(p2,n2);
	return strcmp_nc(p1, n1, p2, n2);
}

t_uint64 atoui64_ex(const char * src,t_size len) {
	len = strlen_max(src,len);
	t_uint64 ret = 0, mul = 1;
	t_size ptr = len;
	t_size start = 0;
//	start += skip_spacing(src+start,len-start);
	
	while(ptr>start)
	{
		char c = src[--ptr];
		if (c>='0' && c<='9')
		{
			ret += (c-'0') * mul;
			mul *= 10;
		}
		else
		{
			ret = 0;
			mul = 1;
		}
	}
	return ret;
}


t_int64 atoi64_ex(const char * src,t_size len)
{
	len = strlen_max(src,len);
	t_int64 ret = 0, mul = 1;
	t_size ptr = len;
	t_size start = 0;
	bool neg = false;
//	start += skip_spacing(src+start,len-start);
	if (start < len && src[start] == '-') {neg = true; start++;}
//	start += skip_spacing(src+start,len-start);
	
	while(ptr>start)
	{
		char c = src[--ptr];
		if (c>='0' && c<='9')
		{
			ret += (c-'0') * mul;
			mul *= 10;
		}
		else
		{
			ret = 0;
			mul = 1;
		}
	}
	return neg ? -ret : ret;
}

int stricmp_ascii_partial( const char * str, const char * substr) throw() {
    size_t walk = 0;
    for(;;) {
        char c1 = str[walk];
        char c2 = substr[walk];
        c1 = ascii_tolower(c1); c2 = ascii_tolower(c2);
        if (c2 == 0) return 0; // substr terminated = ret0 regardless of str content
        if (c1<c2) return -1; // ret -1 early
        else if (c1>c2) return 1; // ret 1 early
        // else c1 == c2 and c2 != 0 so c1 != 0 either
        ++walk; // go on
    }
}

int stricmp_ascii_ex(const char * const s1,t_size const len1,const char * const s2,t_size const len2) throw() {
	t_size walk1 = 0, walk2 = 0;
	for(;;) {
		char c1 = (walk1 < len1) ? s1[walk1] : 0;
		char c2 = (walk2 < len2) ? s2[walk2] : 0;
		c1 = ascii_tolower(c1); c2 = ascii_tolower(c2);
		if (c1<c2) return -1;
		else if (c1>c2) return 1;
		else if (c1 == 0) return 0;
		walk1++;
		walk2++;
	}

}
int stricmp_ascii(const char * s1,const char * s2) throw() {
	for(;;) {
		char c1 = *s1, c2 = *s2;

		if (c1 > 0 && c2 > 0) {
			c1 = ascii_tolower_lookup(c1);
			c2 = ascii_tolower_lookup(c2);
		} else {
			if (c1 == 0 && c2 == 0) return 0;
		}
		if (c1<c2) return -1;
		else if (c1>c2) return 1;
		else if (c1 == 0) return 0;

		s1++;
		s2++;
	}
}

static int naturalSortCompareInternal( const char * s1, const char * s2, bool insensitive) throw() {
    for( ;; ) {
        unsigned c1, c2;
        size_t d1 = utf8_decode_char( s1, c1 );
        size_t d2 = utf8_decode_char( s2, c2 );
        if (d1 == 0 && d2 == 0) {
            return 0;
        }
        if (char_is_numeric( c1 ) && char_is_numeric( c2 ) ) {
            // Numeric block in both strings, do natural sort magic here
            size_t l1 = 1, l2 = 1;
            while( char_is_numeric( s1[l1] ) ) ++l1;
            while( char_is_numeric( s2[l2] ) ) ++l2;
            
            size_t l = max_t(l1, l2);
            for(size_t w = 0; w < l; ++w) {
                char digit1, digit2;
                
                t_ssize off;
                
                off = w + l1 - l;
                if (off >= 0) {
                    digit1 = s1[w - l + l1];
                } else {
                    digit1 = 0;
                }
                off = w + l2 - l;
                if (off >= 0) {
                    digit2 = s2[w - l + l2];
                } else {
                    digit2 = 0;
                }
                if (digit1 < digit2) return -1;
                if (digit1 > digit2) return 1;
            }
            
            s1 += l1; s2 += l2;
            continue;
        }
        

        if (insensitive) {
            c1 = charLower( c1 );
            c2 = charLower( c2 );
        }
        if (c1 < c2) return -1;
        if (c1 > c2) return 1;
        
        s1 += d1; s2 += d2;
    }
}
int naturalSortCompare( const char * s1, const char * s2) throw() {
    int v = naturalSortCompareInternal( s1, s2, true );
    if (v) return v;
    v = naturalSortCompareInternal( s1, s2, false );
    if (v) return v;
    return strcmp(s1, s2);
}

int naturalSortCompareI( const char * s1, const char * s2) throw() {
    return naturalSortCompareInternal( s1, s2, true );
}


format_float::format_float(double p_val,unsigned p_width,unsigned p_prec)
{
	char temp[64];
	float_to_string(temp,64,p_val,p_prec,false);
	temp[63] = 0;
	t_size len = strlen(temp);
	if (len < p_width)
		m_buffer.add_chars(' ',p_width-len);
	m_buffer += temp;
}

char format_hex_char(unsigned p_val)
{
	PFC_ASSERT(p_val < 16);
	return (p_val < 10) ? p_val + '0' : p_val - 10 + 'A';
}

format_hex::format_hex(t_uint64 p_val,unsigned p_width)
{
	if (p_width > 16) p_width = 16;
	else if (p_width == 0) p_width = 1;
	char temp[16];
	unsigned n;
	for(n=0;n<16;n++)
	{
		temp[15-n] = format_hex_char((unsigned)(p_val & 0xF));
		p_val >>= 4;
	}

	for(n=0;n<16 && temp[n] == '0';n++) {}
	
	if (n > 16 - p_width) n = 16 - p_width;
	
	char * out = m_buffer;
	for(;n<16;n++)
		*(out++) = temp[n];
	*out = 0;
}

char format_hex_char_lowercase(unsigned p_val)
{
	PFC_ASSERT(p_val < 16);
	return (p_val < 10) ? p_val + '0' : p_val - 10 + 'a';
}

format_hex_lowercase::format_hex_lowercase(t_uint64 p_val,unsigned p_width)
{
	if (p_width > 16) p_width = 16;
	else if (p_width == 0) p_width = 1;
	char temp[16];
	unsigned n;
	for(n=0;n<16;n++)
	{
		temp[15-n] = format_hex_char_lowercase((unsigned)(p_val & 0xF));
		p_val >>= 4;
	}

	for(n=0;n<16 && temp[n] == '0';n++) {}
	
	if (n > 16 - p_width) n = 16 - p_width;
	
	char * out = m_buffer;
	for(;n<16;n++)
		*(out++) = temp[n];
	*out = 0;
}

format_uint::format_uint(t_uint64 val,unsigned p_width,unsigned p_base)
{
	
	enum {max_width = PFC_TABSIZE(m_buffer) - 1};

	if (p_width > max_width) p_width = max_width;
	else if (p_width == 0) p_width = 1;

	char temp[max_width];
	
	unsigned n;
	for(n=0;n<max_width;n++)
	{
		temp[max_width-1-n] = format_hex_char((unsigned)(val % p_base));
		val /= p_base;
	}

	for(n=0;n<max_width && temp[n] == '0';n++) {}
	
	if (n > max_width - p_width) n = max_width - p_width;
	
	char * out = m_buffer;

	for(;n<max_width;n++)
		*(out++) = temp[n];
	*out = 0;
}

format_fixedpoint::format_fixedpoint(t_int64 p_val,unsigned p_point)
{
	unsigned div = 1;
	for(unsigned n=0;n<p_point;n++) div *= 10;

	if (p_val < 0) {m_buffer << "-";p_val = -p_val;}

	
	m_buffer << format_int(p_val / div) << "." << format_int(p_val % div, p_point);
}

format_int::format_int(t_int64 p_val,unsigned p_width,unsigned p_base)
{
	bool neg = false;
	t_uint64 val;
	if (p_val < 0) {neg = true; val = (t_uint64)(-p_val);}
	else val = (t_uint64)p_val;
	
	enum {max_width = PFC_TABSIZE(m_buffer) - 1};

	if (p_width > max_width) p_width = max_width;
	else if (p_width == 0) p_width = 1;

	if (neg && p_width > 1) p_width --;
	
	char temp[max_width];
	
	unsigned n;
	for(n=0;n<max_width;n++)
	{
		temp[max_width-1-n] = format_hex_char((unsigned)(val % p_base));
		val /= p_base;
	}

	for(n=0;n<max_width && temp[n] == '0';n++) {}
	
	if (n > max_width - p_width) n = max_width - p_width;
	
	char * out = m_buffer;

	if (neg) *(out++) = '-';

	for(;n<max_width;n++)
		*(out++) = temp[n];
	*out = 0;
}

format_hexdump_lowercase::format_hexdump_lowercase(const void * p_buffer,t_size p_bytes,const char * p_spacing)
{
	t_size n;
	const t_uint8 * buffer = (const t_uint8*)p_buffer;
	for(n=0;n<p_bytes;n++)
	{
		if (n > 0 && p_spacing != 0) m_formatter << p_spacing;
		m_formatter << format_hex_lowercase(buffer[n],2);
	}
}

format_hexdump::format_hexdump(const void * p_buffer,t_size p_bytes,const char * p_spacing)
{
	t_size n;
	const t_uint8 * buffer = (const t_uint8*)p_buffer;
	for(n=0;n<p_bytes;n++)
	{
		if (n > 0 && p_spacing != 0) m_formatter << p_spacing;
		m_formatter << format_hex(buffer[n],2);
	}
}



string_replace_extension::string_replace_extension(const char * p_path,const char * p_ext)
{
	m_data = p_path;
	t_size dot = m_data.find_last('.');
	if (dot < m_data.scan_filename())
	{//argh
		m_data += ".";
		m_data += p_ext;
	}
	else
	{
		m_data.truncate(dot+1);
		m_data += p_ext;
	}
}

string_directory::string_directory(const char * p_path)
{
	t_size ptr = scan_filename(p_path);
	if (ptr > 1) {
		if (is_path_separator(p_path[ptr-1]) && !is_path_separator(p_path[ptr-2])) --ptr;
	}
	m_data.set_string(p_path,ptr);
}

t_size scan_filename(const char * ptr)
{
	t_size n;
	t_size _used = strlen(ptr);
	for(n=_used-1;n!=~0;n--)
	{
		if (is_path_separator(ptr[n])) return n+1;
	}
	return 0;
}



t_size string_find_first(const char * p_string,char p_tofind,t_size p_start) {
	for(t_size walk = p_start; p_string[walk]; ++walk) {
		if (p_string[walk] == p_tofind) return walk;
	}
	return ~0;
}
t_size string_find_last(const char * p_string,char p_tofind,t_size p_start) {
	return string_find_last_ex(p_string,~0,&p_tofind,1,p_start);
}
t_size string_find_first(const char * p_string,const char * p_tofind,t_size p_start) {
	return string_find_first_ex(p_string,~0,p_tofind,~0,p_start);
}
t_size string_find_last(const char * p_string,const char * p_tofind,t_size p_start) {
	return string_find_last_ex(p_string,~0,p_tofind,~0,p_start);
}

t_size string_find_first_ex(const char * p_string,t_size p_string_length,char p_tofind,t_size p_start) {
	for(t_size walk = p_start; walk < p_string_length && p_string[walk]; ++walk) {
		if (p_string[walk] == p_tofind) return walk;
	}
	return ~0;
}
t_size string_find_last_ex(const char * p_string,t_size p_string_length,char p_tofind,t_size p_start) {
	return string_find_last_ex(p_string,p_string_length,&p_tofind,1,p_start);
}
t_size string_find_first_ex(const char * p_string,t_size p_string_length,const char * p_tofind,t_size p_tofind_length,t_size p_start) {
	p_string_length = strlen_max(p_string,p_string_length); p_tofind_length = strlen_max(p_tofind,p_tofind_length);
	if (p_string_length >= p_tofind_length) {
		t_size max = p_string_length - p_tofind_length;
		for(t_size walk = p_start; walk <= max; walk++) {
			if (_strcmp_partial_ex(p_string+walk,p_string_length-walk,p_tofind,p_tofind_length) == 0) return walk;
		}
	}
	return ~0;
}
t_size string_find_last_ex(const char * p_string,t_size p_string_length,const char * p_tofind,t_size p_tofind_length,t_size p_start) {
	p_string_length = strlen_max(p_string,p_string_length); p_tofind_length = strlen_max(p_tofind,p_tofind_length);
	if (p_string_length >= p_tofind_length) {
		t_size max = min_t<t_size>(p_string_length - p_tofind_length,p_start);
		for(t_size walk = max; walk != (t_size)(-1); walk--) {
			if (_strcmp_partial_ex(p_string+walk,p_string_length-walk,p_tofind,p_tofind_length) == 0) return walk;
		}
	}
	return ~0;
}

t_size string_find_first_nc(const char * p_string,t_size p_string_length,char c,t_size p_start) {
	for(t_size walk = p_start; walk < p_string_length; walk++) {
		if (p_string[walk] == c) return walk;
	}
	return ~0;
}

t_size string_find_first_nc(const char * p_string,t_size p_string_length,const char * p_tofind,t_size p_tofind_length,t_size p_start) {
	if (p_string_length >= p_tofind_length) {
		t_size max = p_string_length - p_tofind_length;
		for(t_size walk = p_start; walk <= max; walk++) {
			if (memcmp(p_string+walk, p_tofind, p_tofind_length) == 0) return walk;
		}
	}
	return ~0;
}


bool string_is_numeric(const char * p_string,t_size p_length) throw() {
	bool retval = false;
	for(t_size walk = 0; walk < p_length && p_string[walk] != 0; walk++) {
		if (!char_is_numeric(p_string[walk])) {retval = false; break;}
		retval = true;
	}
	return retval;
}


void string_base::end_with(char p_char) {
	if (!ends_with(p_char)) add_byte(p_char);
}
bool string_base::ends_with(char c) const {
	t_size length = get_length();
	return length > 0 && get_ptr()[length-1] == c;
}

void string_base::end_with_slash() {
	end_with( io::path::getDefaultSeparator() );
}

char string_base::last_char() const {
    size_t l = this->length();
    if (l == 0) return 0;
    return this->get_ptr()[l-1];
}
void string_base::truncate_last_char() {
    size_t l = this->length();
    if (l > 0) this->truncate( l - 1 );
}
    
void string_base::truncate_number_suffix() {
    size_t l = this->length();
    const char * const p = this->get_ptr();
    while( l > 0 && char_is_numeric( p[l-1] ) ) --l;
    truncate( l );
}
    
bool is_multiline(const char * p_string,t_size p_len) {
	for(t_size n = 0; n < p_len && p_string[n]; n++) {
		switch(p_string[n]) {
		case '\r':
		case '\n':
			return true;
		}
	}
	return false;
}

static t_uint64 pow10_helper(unsigned p_extra) {
	t_uint64 ret = 1;
	for(unsigned n = 0; n < p_extra; n++ ) ret *= 10;
	return ret;
}

static uint64_t safeMulAdd(uint64_t prev, unsigned scale, uint64_t add) {
	if (add >= scale || scale == 0) throw pfc::exception_invalid_params();
	uint64_t v = prev * scale + add;
	if (v / scale != prev) throw pfc::exception_invalid_params();
	return v;
}

static size_t parseNumber(const char * in, uint64_t & outNumber) {
	size_t walk = 0;
	uint64_t total = 0;
	for (;;) {
		char c = in[walk];
		if (!pfc::char_is_numeric(c)) break;
		unsigned v = (unsigned)(c - '0');
		uint64_t newVal = total * 10 + v;
		if (newVal / 10 != total) throw pfc::exception_overflow();
		total = newVal;
		++walk;
	}
	outNumber = total;
	return walk;
}

double parse_timecode(const char * in) {
	char separator = 0;
	uint64_t seconds = 0;
	unsigned colons = 0;
	for (;;) {
		uint64_t number = 0;
		size_t digits = parseNumber(in, number);
		if (digits == 0) throw pfc::exception_invalid_params();
		in += digits;
		char nextSeparator = *in;
		switch (separator) { // *previous* separator
		case '.':
			if (nextSeparator != 0) throw pfc::exception_bug_check();
			return (double)seconds + (double)pfc::exp_int(10, -(int)digits) * number;
		case 0: // is first number in the string
			seconds = number;
			break;
		case ':':
			if (colons == 2) throw pfc::exception_invalid_params();
			++colons;
			seconds = safeMulAdd(seconds, 60, number);
			break;
		}

		if (nextSeparator == 0) {
			// end of string
			return (double)seconds;
		}

		++in;
		separator = nextSeparator;
	}
}

format_time_ex::format_time_ex(double p_seconds,unsigned p_extra) {
	t_uint64 pow10 = pow10_helper(p_extra);
	t_uint64 ticks = pfc::rint64(pow10 * p_seconds);

	m_buffer << pfc::format_time(ticks / pow10);
	if (p_extra>0) {
		m_buffer << "." << pfc::format_uint(ticks % pow10, p_extra);
	}
}

void stringToUpperAppend(string_base & out, const char * src, t_size len) {
	while(len && *src) {
		unsigned c; t_size d;
		d = utf8_decode_char(src,c,len);
		if (d==0 || d>len) break;
		out.add_char(charUpper(c));
		src+=d;
		len-=d;
	}
}
void stringToLowerAppend(string_base & out, const char * src, t_size len) {
	while(len && *src) {
		unsigned c; t_size d;
		d = utf8_decode_char(src,c,len);
		if (d==0 || d>len) break;
		out.add_char(charLower(c));
		src+=d;
		len-=d;
	}
}
int stringCompareCaseInsensitiveEx(string_part_ref s1, string_part_ref s2) {
	t_size w1 = 0, w2 = 0;
	for(;;) {
		unsigned c1, c2; t_size d1, d2;
		d1 = utf8_decode_char(s1.m_ptr + w1, c1, s1.m_len - w1);
		d2 = utf8_decode_char(s2.m_ptr + w2, c2, s2.m_len - w2);
		if (d1 == 0 && d2 == 0) return 0;
		else if (d1 == 0) return -1;
		else if (d2 == 0) return 1;
		else {
			c1 = charLower(c1); c2 = charLower(c2);
			if (c1 < c2) return -1;
			else if (c1 > c2) return 1;
		}
		w1 += d1; w2 += d2;
	}
}
int stringCompareCaseInsensitive(const char * s1, const char * s2) {
	for(;;) {
		unsigned c1, c2; t_size d1, d2;
		d1 = utf8_decode_char(s1,c1);
		d2 = utf8_decode_char(s2,c2);
		if (d1 == 0 && d2 == 0) return 0;
		else if (d1 == 0) return -1;
		else if (d2 == 0) return 1;
		else {
			c1 = charLower(c1); c2 = charLower(c2);
			if (c1 < c2) return -1;
			else if (c1 > c2) return 1;
		}
		s1 += d1; s2 += d2;
	}
}

format_file_size_short::format_file_size_short(t_uint64 size) {
	t_uint64 scale = 1;
	const char * unit = "B";
	const char * const unitTable[] = {"B","KB","MB","GB","TB"};
	for(t_size walk = 1; walk < PFC_TABSIZE(unitTable); ++walk) {
		t_uint64 next = scale * 1024;
		if (size < next) break;
		scale = next; unit = unitTable[walk];
	}
	*this << ( size  / scale );

	if (scale > 1 && length() < 3) {
		t_size digits = 3 - length();
		const t_uint64 mask = pow_int(10,digits);
		t_uint64 remaining = ( (size * mask / scale) % mask );
		while(digits > 0 && (remaining % 10) == 0) {
			remaining /= 10; --digits;
		}
		if (digits > 0) {
			*this << "." << format_uint(remaining, (t_uint32)digits);
		}
	}
	*this << " " << unit;
	m_scale = scale;
}

bool string_base::truncate_eol(t_size start)
{
	const char * ptr = get_ptr() + start;
	for(t_size n=start;*ptr;n++)
	{
		if (*ptr==10 || *ptr==13)
		{
			truncate(n);
			return true;
		}
		ptr++;
	}
	return false;
}

bool string_base::fix_eol(const char * append,t_size start)
{
	const bool rv = truncate_eol(start);
	if (rv) add_string(append);
	return rv;
}

bool string_base::limit_length(t_size length_in_chars,const char * append)
{
	bool rv = false;
	const char * base = get_ptr(), * ptr = base;
	while(length_in_chars && utf8_advance(ptr)) length_in_chars--;
	if (length_in_chars==0)
	{
		truncate(ptr-base);
		add_string(append);
		rv = true;
	}
	return rv;
}

void string_base::truncate_to_parent_path() {
	size_t at = scan_filename();
#ifdef _WIN32
	while(at > 0 && (*this)[at-1] == '\\') --at;
	if (at > 0 && (*this)[at-1] == ':' && (*this)[at] == '\\') ++at;
#else
	// Strip trailing /
	while(at > 0 && (*this)[at-1] == '/') --at;

	// Hit empty? Bring root / back to life
	if (at == 0 && (*this)[0] == '/') ++at;

	// Deal with proto://
	if (at > 0 && (*this)[at-1] == ':') {
		while((*this)[at] == '/') ++at;
	}
#endif
	this->truncate( at );
}

t_size string_base::replace_string ( const char * replace, const char * replaceWith, t_size start) {
    string_formatter temp;
    size_t srcDone = 0, walk = start;
    size_t occurances = 0;
    const char * const source = this->get_ptr();
    
    const size_t replaceLen = strlen( replace );
    for(;;) {
        const char * ptr = strstr( source + walk, replace );
        if (ptr == NULL) {
            // end
            if (srcDone == 0) {
                return 0; // string not altered
            }
            temp.add_string( source + srcDone );
            break;
        }
        ++occurances;
        walk = ptr - source;
        temp.add_string( source + srcDone, walk - srcDone );
        temp.add_string( replaceWith );
        walk += replaceLen;
        srcDone = walk;
    }
    this->set_string( temp );
    return occurances;
    
}
void urlEncodeAppendRaw(pfc::string_base & out, const char * in, t_size inSize) {
	for(t_size walk = 0; walk < inSize; ++walk) {
		const char c = in[walk];
		if (c == ' ') out.add_byte('+');
		else if (pfc::char_is_ascii_alphanumeric(c) || c == '_') out.add_byte(c);
		else out << "%" << pfc::format_hex((t_uint8)c, 2);
	}
}
void urlEncodeAppend(pfc::string_base & out, const char * in) {
	for(;;) {
		const char c = *(in++);
		if (c == 0) break;
		else if (c == ' ') out.add_byte('+');
		else if (pfc::char_is_ascii_alphanumeric(c) || c == '_') out.add_byte(c);
		else out << "%" << pfc::format_hex((t_uint8)c, 2);
	}
}
void urlEncode(pfc::string_base & out, const char * in) {
	out.reset(); urlEncodeAppend(out, in);
}

unsigned char_to_dec(char c) {
	if (c >= '0' && c <= '9') return (unsigned)(c - '0');
	else throw exception_invalid_params();
}

unsigned char_to_hex(char c) {
	if (c >= '0' && c <= '9') return (unsigned)(c - '0');
	else if (c >= 'a' && c <= 'f') return (unsigned)(c - 'a' + 10);
	else if (c >= 'A' && c <= 'F') return (unsigned)(c - 'A' + 10);
	else throw exception_invalid_params();
}


static const t_uint8 ascii_tolower_table[128] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x5B,0x5C,0x5D,0x5E,0x5F,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F};

uint32_t charLower(uint32_t param)
{
	if (param<128) {
		return ascii_tolower_table[param];
	}
#ifdef _WIN32
	else if (param<0x10000) {
		return (unsigned)CharLowerW((WCHAR*)param);
	}
#endif
	else return param;
}

uint32_t charUpper(uint32_t param)
{
	if (param<128) {
		if (param>='a' && param<='z') param += 'A' - 'a';
		return param;
	}
#ifdef _WIN32
	else if (param<0x10000) {
		return (unsigned)CharUpperW((WCHAR*)param);
	}
#endif
	else return param;
}


bool stringEqualsI_ascii(const char * p1,const char * p2) throw() {
	for(;;)
	{
		char c1 = *p1;
		char c2 = *p2;
		if (c1 > 0 && c2 > 0) {
			if (ascii_tolower_table[c1] != ascii_tolower_table[c2]) return false;
		} else {
			if (c1 == 0 && c2 == 0) return true;
			if (c1 == 0 || c2 == 0) return false;
			if (c1 != c2) return false;
		}
		++p1; ++p2;
	}
}

bool stringEqualsI_utf8(const char * p1,const char * p2) throw()
{
	for(;;)
	{
		char c1 = *p1;
		char c2 = *p2;
		if (c1 > 0 && c2 > 0) {
			if (ascii_tolower_table[c1] != ascii_tolower_table[c2]) return false;
			++p1; ++p2;
		} else {
			if (c1 == 0 && c2 == 0) return true;
			if (c1 == 0 || c2 == 0) return false;
			unsigned w1,w2; t_size d1,d2;
			d1 = utf8_decode_char(p1,w1);
			d2 = utf8_decode_char(p2,w2);
			if (d1 == 0 || d2 == 0) return false; // bad UTF-8, bail
			if (w1 != w2) {
				if (charLower(w1) != charLower(w2)) return false;
			}
			p1 += d1;
			p2 += d2;
		}
	}
}

char ascii_tolower_lookup(char c) {
	PFC_ASSERT( c >= 0);
	return (char)ascii_tolower_table[c];
}

void string_base::fix_dir_separator(char c) {
#ifdef _WIN32
    end_with(c);
#else
    end_with_slash();
#endif
}
    

    bool string_has_prefix( const char * string, const char * prefix ) {
        for(size_t w = 0; ; ++w ) {
            char c = prefix[w];
            if (c == 0) return true;
            if (string[w] != c) return false;
        }
    }
    bool string_has_prefix_i( const char * string, const char * prefix ) {
        const char * p1 = string; const char * p2 = prefix;
        for(;;) {
            unsigned w1, w2; size_t d1, d2;
            d1 = utf8_decode_char(p1, w1);
            d2 = utf8_decode_char(p2, w2);
            if (d2 == 0) return true;
            if (d1 == 0) return false;
            if (w1 != w2) {
                if (charLower(w1) != charLower(w2)) return false;
            }
            p1 += d1; p2 += d2;
        }
    }
    bool string_has_suffix( const char * string, const char * suffix ) {
        size_t len = strlen( string );
        size_t suffixLen = strlen( suffix );
        if (suffixLen > len) return false;
        size_t base = len - suffixLen;
        return memcmp( string + base, suffix, suffixLen * sizeof(char)) == 0;
    }
    bool string_has_suffix_i( const char * string, const char * suffix ) {
        for(;;) {
            if (*string == 0) return false;
            if (stringEqualsI_utf8( string, suffix )) return true;
            if (!utf8_advance(string)) return false;
        }
    }

} //namespace pfc
