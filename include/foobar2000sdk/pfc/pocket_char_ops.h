#pragma once

// Standalone header (no dependencies) with implementations of PFC UTF-8 & UTF-16 manipulation routines

static const uint8_t mask_tab[6] = { 0x80,0xE0,0xF0,0xF8,0xFC,0xFE };

static const uint8_t val_tab[6] = { 0,0xC0,0xE0,0xF0,0xF8,0xFC };

size_t utf8_char_len_from_header(char p_c) throw()
{
	size_t cnt = 0;
	for (;;)
	{
		if ((p_c & mask_tab[cnt]) == val_tab[cnt]) break;
		if (++cnt >= 6) return 0;
	}

	return cnt + 1;

}
size_t utf8_decode_char(const char *p_utf8, unsigned & wide) throw() {
	const uint8_t * utf8 = (const uint8_t*)p_utf8;
	const size_t max = 6;

	if (utf8[0]<0x80) {
		wide = utf8[0];
		return utf8[0]>0 ? 1 : 0;
	}
	wide = 0;

	unsigned res = 0;
	unsigned n;
	unsigned cnt = 0;
	for (;;)
	{
		if ((*utf8&mask_tab[cnt]) == val_tab[cnt]) break;
		if (++cnt >= max) return 0;
	}
	cnt++;

	if (cnt == 2 && !(*utf8 & 0x1E)) return 0;

	if (cnt == 1)
		res = *utf8;
	else
		res = (0xFF >> (cnt + 1))&*utf8;

	for (n = 1; n<cnt; n++)
	{
		if ((utf8[n] & 0xC0) != 0x80)
			return 0;
		if (!res && n == 2 && !((utf8[n] & 0x7F) >> (7 - cnt)))
			return 0;

		res = (res << 6) | (utf8[n] & 0x3F);
	}

	wide = res;

	return cnt;
}

size_t utf8_decode_char(const char *p_utf8, unsigned & wide, size_t max) throw()
{
	const uint8_t * utf8 = (const uint8_t*)p_utf8;

	if (max == 0) {
		wide = 0;
		return 0;
	}

	if (utf8[0]<0x80) {
		wide = utf8[0];
		return utf8[0]>0 ? 1 : 0;
	}
	if (max>6) max = 6;
	wide = 0;

	unsigned res = 0;
	unsigned n;
	unsigned cnt = 0;
	for (;;)
	{
		if ((*utf8&mask_tab[cnt]) == val_tab[cnt]) break;
		if (++cnt >= max) return 0;
	}
	cnt++;

	if (cnt == 2 && !(*utf8 & 0x1E)) return 0;

	if (cnt == 1)
		res = *utf8;
	else
		res = (0xFF >> (cnt + 1))&*utf8;

	for (n = 1; n<cnt; n++)
	{
		if ((utf8[n] & 0xC0) != 0x80)
			return 0;
		if (!res && n == 2 && !((utf8[n] & 0x7F) >> (7 - cnt)))
			return 0;

		res = (res << 6) | (utf8[n] & 0x3F);
	}

	wide = res;

	return cnt;
}


size_t utf8_encode_char(unsigned wide, char * target) throw()
{
	size_t count;

	if (wide < 0x80)
		count = 1;
	else if (wide < 0x800)
		count = 2;
	else if (wide < 0x10000)
		count = 3;
	else if (wide < 0x200000)
		count = 4;
	else if (wide < 0x4000000)
		count = 5;
	else if (wide <= 0x7FFFFFFF)
		count = 6;
	else
		return 0;
	//if (count>max) return 0;

	if (target == 0)
		return count;

	switch (count)
	{
	case 6:
		target[5] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0x4000000;
	case 5:
		target[4] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0x200000;
	case 4:
		target[3] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0x10000;
	case 3:
		target[2] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0x800;
	case 2:
		target[1] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0xC0;
	case 1:
		target[0] = wide & 0xFF;
	}

	return count;
}

size_t utf16_encode_char(unsigned cur_wchar, char16_t * out) throw()
{
	if (cur_wchar < 0x10000) {
		*out = (char16_t)cur_wchar; return 1;
	} else if (cur_wchar < (1 << 20)) {
		unsigned c = cur_wchar - 0x10000;
		//MSDN:
		//The first (high) surrogate is a 16-bit code value in the range U+D800 to U+DBFF. The second (low) surrogate is a 16-bit code value in the range U+DC00 to U+DFFF. Using surrogates, Unicode can support over one million characters. For more details about surrogates, refer to The Unicode Standard, version 2.0.
		out[0] = (char16_t)(0xD800 | (0x3FF & (c >> 10)));
		out[1] = (char16_t)(0xDC00 | (0x3FF & c));
		return 2;
	} else {
		*out = '?'; return 1;
	}
}

size_t utf16_decode_char(const char16_t * p_source, unsigned * p_out, size_t p_source_length) throw() {
	if (p_source_length == 0) { *p_out = 0; return 0; } else if (p_source_length == 1) {
		*p_out = p_source[0];
		return 1;
	} else {
		size_t retval = 0;
		unsigned decoded = p_source[0];
		if (decoded != 0)
		{
			retval = 1;
			if ((decoded & 0xFC00) == 0xD800)
			{
				unsigned low = p_source[1];
				if ((low & 0xFC00) == 0xDC00)
				{
					decoded = 0x10000 + (((decoded & 0x3FF) << 10) | (low & 0x3FF));
					retval = 2;
				}
			}
		}
		*p_out = decoded;
		return retval;
	}
}

unsigned utf8_get_char(const char * src)
{
	unsigned rv = 0;
	utf8_decode_char(src, rv);
	return rv;
}


size_t utf8_char_len(const char * s, size_t max) throw()
{
	unsigned dummy;
	return utf8_decode_char(s, dummy, max);
}

size_t skip_utf8_chars(const char * ptr, size_t count) throw()
{
	size_t num = 0;
	for (; count && ptr[num]; count--)
	{
		size_t d = utf8_char_len(ptr + num, (size_t)(-1));
		if (d <= 0) break;
		num += d;
	}
	return num;
}

bool is_valid_utf8(const char * param, size_t max) {
	size_t walk = 0;
	while (walk < max && param[walk] != 0) {
		size_t d;
		unsigned dummy;
		d = utf8_decode_char(param + walk, dummy, max - walk);
		if (d == 0) return false;
		walk += d;
		if (walk > max) {
			// should not get here
			return false;
		}
	}
	return true;
}
