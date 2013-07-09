
#include "foobar2000/SDK/foobar2000.h"
#include "foobar2000/helpers/helpers.h"

#define LIBOPENMPT_ALPHA_WARNING_SEEN_AND_I_KNOW_WHAT_I_AM_DOING
#include "libopenmpt.hpp"

#include <algorithm>
#include <locale>
#include <string>
#include <vector>



// Declaration of your component's version information
// Since foobar2000 v1.0 having at least one of these in your DLL is mandatory to let the troubleshooter tell different versions of your component apart.
// Note that it is possible to declare multiple components within one DLL, but it's strongly recommended to keep only one declaration per DLL.
// As for 1.1, the version numbers are used by the component update finder to find updates; for that to work, you must have ONLY ONE declaration per DLL. If there are multiple declarations, the component is assumed to be outdated and a version number of "0" is assumed, to overwrite the component with whatever is currently on the site assuming that it comes with proper version numbers.
DECLARE_COMPONENT_VERSION("OpenMPT component","0.1","libopenmpt based module file player");



// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_openmpt.dll");



// Sample initquit implementation. See also: initquit class documentation in relevant header.

class myinitquit : public initquit {
public:
	void on_init() {
		// console::print("Sample component: on_init()");
	}
	void on_quit() {
		// console::print("Sample component: on_quit()");
	}
};

static initquit_factory_t<myinitquit> g_myinitquit_factory;



// No inheritance. Our methods get called over input framework templates. See input_singletrack_impl for descriptions of what each method does.
class input_openmpt {
public:
	void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		if ( p_reason == input_open_info_write ) {
			throw exception_io_unsupported_format(); // our input does not support retagging.
		}
		m_file = p_filehint; // p_filehint may be null, hence next line
		input_open_file_helper(m_file,p_path,p_reason,p_abort); // if m_file is null, opens file with appropriate privileges for our operation (read/write for writing tags, read-only otherwise).
		if ( m_file->get_size( p_abort ) >= (std::numeric_limits<std::size_t>::max)() ) {
			throw exception_io_unsupported_format();
		}
		std::vector<char> data( static_cast<std::size_t>( m_file->get_size( p_abort ) ) );
		m_file->read( data.data(), data.size(), p_abort );
		mod = new openmpt::module( data );
	}
	void get_info(file_info & p_info,abort_callback & p_abort) {
		p_info.set_length( mod->get_duration_seconds() );
		p_info.info_set_int( "samplerate", 48000 );
		p_info.info_set_int( "channels", 2 );
		p_info.info_set_int( "bitspersample", 32 );
		std::vector<std::string> keys = mod->get_metadata_keys();
		for ( std::vector<std::string>::iterator key = keys.begin(); key != keys.end(); ++key ) {
			p_info.meta_set( (*key).c_str(), mod->get_metadata( *key ).c_str() );
		}
	}
	t_filestats get_file_stats(abort_callback & p_abort) {
		return m_file->get_stats(p_abort);
	}
	void decode_initialize(unsigned p_flags,abort_callback & p_abort) {
		m_file->reopen(p_abort); // equivalent to seek to zero, except it also works on nonseekable streams
	}
	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {
		std::size_t count = mod->read( 48000, buffersize, left.data(), right.data() );
		if ( count == 0 ) {
			return false;
		}
		for ( std::size_t frame = 0; frame < count; frame++ ) {
			buffer[frame*2+0] = left[frame];
			buffer[frame*2+1] = right[frame];
		}
		p_chunk.set_data_32( buffer.data(), count, 2, 48000 );
		return true;
	}
	void decode_seek(double p_seconds,abort_callback & p_abort) {
		mod->set_position_seconds( p_seconds );
	}
	bool decode_can_seek() {
		return true;
	}
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) { // deals with dynamic information such as VBR bitrates
		return false;
	}
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) { // deals with dynamic information such as track changes in live streams
		return false;
	}
	void decode_on_idle(abort_callback & p_abort) {
		m_file->on_idle( p_abort );
	}
	void retag(const file_info & p_info,abort_callback & p_abort) {
		throw exception_io_unsupported_format();
	}
	static bool g_is_our_content_type(const char * p_content_type) { // match against supported mime types here
		return false;
	}
	static bool g_is_our_path(const char * p_path,const char * p_extension) {
		if ( !p_extension ) {
			return false;
		}
		std::vector<std::string> extensions = openmpt::get_supported_extensions();
		std::string ext = p_extension;
		std::transform( ext.begin(), ext.end(), ext.begin(), tolower );
		return std::find( extensions.begin(), extensions.end(), ext ) != extensions.end();
	}
public:
	service_ptr_t<file> m_file;
	static const std::size_t buffersize = 1024;
	openmpt::module * mod;
	std::vector<float> left;
	std::vector<float> right;
	std::vector<float> buffer;
	input_openmpt() : mod(0), left(buffersize), right(buffersize), buffer(2*buffersize) {}
	~input_openmpt() { delete mod; mod = 0; }
};

static input_singletrack_factory_t<input_openmpt> g_input_openmpt_factory;



// copied table from soundlib/Tables.cpp
// the foobar2000 interface is stupid demanding to declare those statically

DECLARE_FILE_TYPE("OpenMPT compatibel module files",
	"*.mod" ";"
	"*.s3m" ";"
	"*.xm" ";"
	"*.it" ";"
	"*.mptm" ";"
	"*.stm" ";"
	"*.nst" ";"
	"*.m15" ";"
	"*.stk" ";"
	"*.wow" ";"
	"*.ult" ";"
	"*.669" ";"
	"*.mtm" ";"
	"*.med" ";"
	"*.far" ";"
	"*.mdl" ";"
	"*.ams" ";"
	"*.ams" ";"
	"*.dsm" ";"
	"*.amf" ";"
	"*.amf" ";"
	"*.okt" ";"
	"*.dmf" ";"
	"*.ptm" ";"
	"*.psm" ";"
	"*.mt2" ";"
	"*.dbm" ";"
	"*.digi" ";"
	"*.imf" ";"
	"*.j2b" ";"
	"*.gdm" ";"
	"*.umx" ";"
	"*.uax" ";"
	"*.mo3" );
