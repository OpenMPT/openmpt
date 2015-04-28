#include "foobar2000.h"

void chapter_list::copy(const chapter_list & p_source)
{
	t_size n, count = p_source.get_chapter_count();
	set_chapter_count(count);
	for(n=0;n<count;n++) set_info(n,p_source.get_info(n));
	set_pregap(p_source.get_pregap());
}

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

cuesheet_format_index_time::cuesheet_format_index_time(double p_time)
{
	t_uint64 ticks = audio_math::time_to_samples(p_time,75);
	t_uint64 seconds = ticks / 75; ticks %= 75;
	t_uint64 minutes = seconds / 60; seconds %= 60;
	m_buffer << pfc::format_uint(minutes,2) << ":" << pfc::format_uint(seconds,2) << ":" << pfc::format_uint(ticks,2);
}

double cuesheet_parse_index_time_e(const char * p_string,t_size p_length)
{
	return (double) cuesheet_parse_index_time_ticks_e(p_string,p_length) / 75.0;
}

unsigned cuesheet_parse_index_time_ticks_e(const char * p_string,t_size p_length)
{
	p_length = pfc::strlen_max(p_string,p_length);
	t_size ptr = 0;
	t_size splitmarks[2];
	t_size splitptr = 0;
	for(ptr=0;ptr<p_length;ptr++)
	{
		if (p_string[ptr] == ':')
		{
			if (splitptr >= 2) throw std::runtime_error("invalid INDEX time syntax");
			splitmarks[splitptr++] = ptr;
		}
		else if (!pfc::char_is_numeric(p_string[ptr])) throw std::runtime_error("invalid INDEX time syntax");
	}
	
	t_size minutes_base = 0, minutes_length = 0, seconds_base = 0, seconds_length = 0, frames_base = 0, frames_length = 0;

	switch(splitptr)
	{
	case 0:
		frames_base = 0;
		frames_length = p_length;
		break;
	case 1:
		seconds_base = 0;
		seconds_length = splitmarks[0];
		frames_base = splitmarks[0] + 1;
		frames_length = p_length - frames_base;
		break;
	case 2:
		minutes_base = 0;
		minutes_length = splitmarks[0];
		seconds_base = splitmarks[0] + 1;
		seconds_length = splitmarks[1] - seconds_base;
		frames_base = splitmarks[1] + 1;
		frames_length = p_length - frames_base;
		break;
	}

	unsigned ret = 0;

	if (frames_length > 0) ret += pfc::atoui_ex(p_string + frames_base,frames_length);
	if (seconds_length > 0) ret += 75 * pfc::atoui_ex(p_string + seconds_base,seconds_length);
	if (minutes_length > 0) ret += 60 * 75 * pfc::atoui_ex(p_string + minutes_base,minutes_length);

	return ret;	
}

