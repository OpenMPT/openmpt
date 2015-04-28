#include "foobar2000.h"

bool component_installation_validator::test_my_name(const char * fn) {
	const char * path = core_api::get_my_full_path();
	path += pfc::scan_filename(path);
	bool retVal = ( strcmp(path, fn) == 0 );
	PFC_ASSERT( retVal );
	return retVal;
}
bool component_installation_validator::have_other_file(const char * fn) {
	for(int retry = 0;;) {
		pfc::string_formatter path = core_api::get_my_full_path();
		path.truncate(path.scan_filename());
		path << fn;
		try {
			try {
                abort_callback_dummy aborter;
				bool v = filesystem::g_exists(path, aborter);
				PFC_ASSERT( v );
				return v;
			} catch(std::exception const & e) {
				FB2K_console_formatter() << "Component integrity check error: " << e << " (on: " << fn << ")";
				throw;
			}
		} catch(exception_io_denied) {
			continue;
		} catch(exception_io_sharing_violation) {
			continue;
		} catch(exception_io_file_corrupted) { // happens
			return false; 
		} catch(...) {
			uBugCheck();
		}
		if (++retry == 10) uBugCheck();
		Sleep(100);
	}
}
