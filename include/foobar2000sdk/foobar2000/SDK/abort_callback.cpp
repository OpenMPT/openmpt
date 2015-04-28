#include "foobar2000.h"

void abort_callback::check() const {
	if (is_aborting()) throw exception_aborted();
}

void abort_callback::sleep(double p_timeout_seconds) const {
	if (!sleep_ex(p_timeout_seconds)) throw exception_aborted();
}

bool abort_callback::sleep_ex(double p_timeout_seconds) const {
	// return true IF NOT SET (timeout), false if set
	return !pfc::event::g_wait_for(get_abort_event(),p_timeout_seconds);
}

bool abort_callback::waitForEvent( pfc::eventHandle_t evtHandle, double timeOut ) {
    int status = pfc::event::g_twoEventWait( this->get_abort_event(), evtHandle, timeOut );
    switch(status) {
        case 1: throw exception_aborted();
        case 2: return true;
        case 0: return false;
        default: uBugCheck();
    }
}
