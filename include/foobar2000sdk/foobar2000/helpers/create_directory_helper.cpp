#include "stdafx.h"

namespace create_directory_helper
{
	static void create_path_internal(const char * p_path,t_size p_base,abort_callback & p_abort) {
		pfc::string8_fastalloc temp;
		for(t_size walk = p_base; p_path[walk]; walk++) {
			if (p_path[walk] == '\\') {
				temp.set_string(p_path,walk);
				try {filesystem::g_create_directory(temp.get_ptr(),p_abort);} catch(exception_io_already_exists) {}
			}
		}
	}

	static bool is_valid_netpath_char(char p_char) {
		return pfc::char_is_ascii_alphanumeric(p_char) || p_char == '_' || p_char == '-' || p_char == '.';
	}

	static bool test_localpath(const char * p_path) {
		if (pfc::strcmp_partial(p_path,"file://") == 0) p_path += strlen("file://");
		return pfc::char_is_ascii_alpha(p_path[0]) && 
			p_path[1] == ':' &&
			p_path[2] == '\\';
	}
	static bool test_netpath(const char * p_path) {
		if (pfc::strcmp_partial(p_path,"file://") == 0) p_path += strlen("file://");
		if (*p_path != '\\') return false;
		p_path++;
		if (*p_path != '\\') return false;
		p_path++;
		if (!is_valid_netpath_char(*p_path)) return false;
		p_path++;
		while(is_valid_netpath_char(*p_path)) p_path++;
		if (*p_path != '\\') return false;
		return true;
	}

	void create_path(const char * p_path,abort_callback & p_abort) {
		if (test_localpath(p_path)) {
			t_size walk = 0;
			if (pfc::strcmp_partial(p_path,"file://") == 0) walk += strlen("file://");
			create_path_internal(p_path,walk + 3,p_abort);
		} else if (test_netpath(p_path)) {
			t_size walk = 0;
			if (pfc::strcmp_partial(p_path,"file://") == 0) walk += strlen("file://");
			while(p_path[walk] == '\\') walk++;
			while(p_path[walk] != 0 && p_path[walk] != '\\') walk++;
			while(p_path[walk] == '\\') walk++;
			while(p_path[walk] != 0 && p_path[walk] != '\\') walk++;
			while(p_path[walk] == '\\') walk++;
			create_path_internal(p_path,walk,p_abort);
		} else {
			throw exception_io("Could not create directory structure; unknown path format");
		}
	}

	static bool is_bad_dirchar(char c)
	{
		return c==' ' || c=='.';
	}

	void make_path(const char * parent,const char * filename,const char * extension,bool allow_new_dirs,pfc::string8 & out,bool really_create_dirs,abort_callback & p_abort)
	{
		out.reset();
		if (parent && *parent)
		{
			out = parent;
			out.fix_dir_separator('\\');
		}
		bool last_char_is_dir_sep = true;
		while(*filename)
		{
#ifdef WIN32
			if (allow_new_dirs && is_bad_dirchar(*filename))
			{
				const char * ptr = filename+1;
				while(is_bad_dirchar(*ptr)) ptr++;
				if (*ptr!='\\' && *ptr!='/') out.add_string(filename,ptr-filename);
				filename = ptr;
				if (*filename==0) break;
			}
#endif
			if (pfc::is_path_bad_char(*filename))
			{
				if (allow_new_dirs && (*filename=='\\' || *filename=='/'))
				{
					if (!last_char_is_dir_sep)
					{
						if (really_create_dirs) try{filesystem::g_create_directory(out,p_abort);}catch(exception_io_already_exists){}
						out.add_char('\\');
						last_char_is_dir_sep = true;
					}
				}
				else
					out.add_char('_');
			}
			else
			{
				out.add_byte(*filename);
				last_char_is_dir_sep = false;
			}
			filename++;
		}
		if (out.length()>0 && out[out.length()-1]=='\\')
		{
			out.add_string("noname");
		}
		if (extension && *extension)
		{
			out.add_char('.');
			out.add_string(extension);
		}
	}
}

pfc::string create_directory_helper::sanitize_formatted_path(pfc::stringp formatted, bool allowWC) {
	pfc::string out;
	t_size curSegBase = 0;
	for(t_size walk = 0; ; ++walk) {
		const char c = formatted[walk];
		if (c == 0 || pfc::io::path::isSeparator(c)) {
			if (curSegBase < walk) {
				pfc::string seg( formatted + curSegBase, walk - curSegBase );
				out = pfc::io::path::combine(out, pfc::io::path::validateFileName(seg, allowWC));
			}
			if (c == 0) break;
			curSegBase = walk + 1;
		}
	}
	return out;
};

void create_directory_helper::format_filename_ex(const metadb_handle_ptr & handle,titleformat_hook * p_hook,titleformat_object::ptr spec,const char * suffix, pfc::string_base & out) {
	pfc::string_formatter formatted;
	handle->format_title(p_hook,formatted,spec,&titleformat_text_filter_myimpl());
	formatted << suffix;
	out = sanitize_formatted_path(formatted).ptr();
}
void create_directory_helper::format_filename(const metadb_handle_ptr & handle,titleformat_hook * p_hook,titleformat_object::ptr spec,pfc::string_base & out) {
	format_filename_ex(handle, p_hook, spec, "", out);
}
void create_directory_helper::format_filename(const metadb_handle_ptr & handle,titleformat_hook * p_hook,const char * spec,pfc::string_base & out)
{
	service_ptr_t<titleformat_object> script;
	if (static_api_ptr_t<titleformat_compiler>()->compile(script,spec)) {
		format_filename(handle, p_hook, script, out);
	} else {
		out.reset();
	}
}

void create_directory_helper::titleformat_text_filter_myimpl::write(const GUID & p_inputType,pfc::string_receiver & p_out,const char * p_data,t_size p_dataLength) {
	if (p_inputType == titleformat_inputtypes::meta) {
		pfc::string_formatter temp;
		for(t_size walk = 0; walk < p_dataLength; ++walk) {
			char c = p_data[walk];
			if (c == 0) break;
			if (pfc::io::path::isSeparator(c)) {
				c = '-';
			}
			temp.add_byte(c);
		}
		p_out.add_string(temp);
	} else p_out.add_string(p_data,p_dataLength);
}
