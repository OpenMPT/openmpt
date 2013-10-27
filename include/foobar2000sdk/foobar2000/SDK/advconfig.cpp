#include "foobar2000.h"


t_uint32 advconfig_entry::get_preferences_flags_() {
	{
		advconfig_entry_string_v2::ptr ex;
		if (service_query_t(ex)) return ex->get_preferences_flags();
	}
	{
		advconfig_entry_checkbox_v2::ptr ex;
		if (service_query_t(ex)) return ex->get_preferences_flags();
	}
	return 0;
}

bool advconfig_entry_checkbox::get_default_state_() {
	{
		advconfig_entry_checkbox_v2::ptr ex;
		if (service_query_t(ex)) return ex->get_default_state();
	}

	bool backup = get_state();
	reset();
	bool rv = get_state();
	set_state(backup);
	return rv;
}

void advconfig_entry_string::get_default_state_(pfc::string_base & out) {
	{
		advconfig_entry_string_v2::ptr ex;
		if (service_query_t(ex)) {ex->get_default_state(out); return;}
	}
	pfc::string8 backup;
	get_state(backup);
	reset();
	get_state(out);
	set_state(backup);
}
