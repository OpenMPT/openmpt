#include "foobar2000.h"

bool album_art_editor::g_get_interface(service_ptr_t<album_art_editor> & out,const char * path) {
	service_enum_t<album_art_editor> e; ptr ptr;
	pfc::string_extension ext(path);
	while(e.next(ptr)) {
		if (ptr->is_our_path(path,ext)) { out = ptr; return true; }
	}
	return false;
}

bool album_art_editor::g_is_supported_path(const char * path) {
	ptr ptr; return g_get_interface(ptr,path);
}

album_art_editor_instance_ptr album_art_editor::g_open(file_ptr p_filehint,const char * p_path,abort_callback & p_abort) {
	album_art_editor::ptr obj;
	if (!g_get_interface(obj, p_path)) throw exception_album_art_unsupported_format();
	return obj->open(p_filehint, p_path, p_abort);
}


bool album_art_extractor::g_get_interface(service_ptr_t<album_art_extractor> & out,const char * path) {
	service_enum_t<album_art_extractor> e; ptr ptr;
	pfc::string_extension ext(path);
	while(e.next(ptr)) {
		if (ptr->is_our_path(path,ext)) { out = ptr; return true; }
	}
	return false;
}

bool album_art_extractor::g_is_supported_path(const char * path) {
	ptr ptr; return g_get_interface(ptr,path);
}
album_art_extractor_instance_ptr album_art_extractor::g_open(file_ptr p_filehint,const char * p_path,abort_callback & p_abort) {
	album_art_extractor::ptr obj;
	if (!g_get_interface(obj, p_path)) throw exception_album_art_unsupported_format();
	return obj->open(p_filehint, p_path, p_abort);
}


album_art_extractor_instance_ptr album_art_extractor::g_open_allowempty(file_ptr p_filehint,const char * p_path,abort_callback & p_abort) {
	try {
		return g_open(p_filehint, p_path, p_abort);
	} catch(exception_album_art_not_found) {
		return new service_impl_t<album_art_extractor_instance_simple>();
	}
}
