#include "foobar2000.h"

void commandline_handler_metadb_handle::on_file(const char * url) {
	metadb_handle_list handles;
	try {
		metadb_io::get()->path_to_handles_simple(url, handles);
	} catch(std::exception const & e) {
		console::complain("Path evaluation failure", e);
		return;
	}
	for(t_size walk = 0; walk < handles.get_size(); ++walk) on_file(handles[walk]);
}
