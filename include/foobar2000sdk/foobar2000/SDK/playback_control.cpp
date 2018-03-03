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
			titleformat_compiler::get()->compile_force(script, "[%length_ex%]");
			this->playback_format_title(NULL, temp, script, NULL, display_level_titles);
			if (temp.length() > 0) rv = parse_time(temp);
		}
	}
	return rv;
}






void playback_control::userPrev() {
	userActionHook();
	if (this->is_playing() && this->playback_can_seek() && this->playback_get_position() > 5) {
		this->playback_seek(0);
	} else {
		this->previous();
	}
}

void playback_control::userNext() {
	userActionHook();
	this->start(track_command_next);
}

void playback_control::userMute() {
	userActionHook();
	this->volume_mute_toggle();
}

void playback_control::userStop() {
	userActionHook();
	this->stop();
}

void playback_control::userPlay() {
	userActionHook();
	this->play_or_pause();
}

void playback_control::userPause() {
	userActionHook();
	nonUserPause();
}

void playback_control::nonUserPause() {
	if (this->is_playing()) {
		this->pause(true);
	}
}

void playback_control::userStart() {
	userActionHook();
	if (this->is_playing()) {
		this->pause(false);
	} else {
		this->start();
	}
}
static const double seekDelta = 30;
void playback_control::userFastForward() {
	userActionHook();
	if (!this->playback_can_seek()) {
		this->userNext(); return;
	}
	this->playback_seek_delta(seekDelta);
}

void playback_control::userRewind() {
	userActionHook();
	if (!this->playback_can_seek()) {
		this->userPrev(); return;
	}
	double p = this->playback_get_position();
	if (p < 0) return;
	if (p < seekDelta) {
		if (p < seekDelta / 3) {
			this->userPrev();
		} else {
			this->playback_seek(0);
		}
	} else {
		this->playback_seek_delta(-30);
	}
}
