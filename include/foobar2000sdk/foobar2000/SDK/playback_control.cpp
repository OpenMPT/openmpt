#include "foobar2000.h"

static double parseFraction(const char * fraction) {
	unsigned v = 0, d = 1;
	while(pfc::char_is_numeric( *fraction) ) {
		d *= 10;
		v *= 10;
		v += (unsigned) ( *fraction - '0' );
		++fraction;
	}
	PFC_ASSERT( *fraction == 0 );
	return (double)v / (double)d;
}

static double parse_time(const char * time) {
	unsigned vTotal = 0, vCur = 0;
	for(;;) {
		char c = *time++;
		if (c == 0) return (double) (vTotal + vCur);
		else if (pfc::char_is_numeric( c ) ) {
			vCur = vCur * 10 + (unsigned)(c-'0');
		} else if (c == ':') {
			if (vCur >= 60) {PFC_ASSERT(!"Invalid input"); return 0; }
			vTotal += vCur; vCur = 0; vTotal *= 60;
		} else if (c == '.') {
			return (double) (vTotal + vCur) + parseFraction(time);
		} else {
			PFC_ASSERT(!"Invalid input"); return 0;
		}
	}
}

double playback_control::playback_get_length()
{
	double rv = 0;
	metadb_handle_ptr ptr;
	if (get_now_playing(ptr))
	{
		rv = ptr->get_length();
	}
	return rv;
}

double playback_control::playback_get_length_ex() {
	double rv = 0;
	metadb_handle_ptr ptr;
	if (get_now_playing(ptr))
	{
		rv = ptr->get_length();
		if (rv <= 0) {
			pfc::string8 temp;
			titleformat_object::ptr script;
			static_api_ptr_t<titleformat_compiler>()->compile_force(script, "[%length_ex%]");
			this->playback_format_title(NULL, temp, script, NULL, display_level_titles);
			if (temp.length() > 0) rv = parse_time(temp);
		}
	}
	return rv;
}
