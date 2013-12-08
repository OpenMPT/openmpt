/*
 * xmp-openmpt.cpp
 * ---------------
 * Purpose: libopenmpt xmplay input plugin implementation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef NO_XMPLAY

#ifdef LIBOPENMPT_BUILD_DLL
#undef LIBOPENMPT_BUILD_DLL
#endif

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif // _MSC_VER

#define LIBOPENMPT_INTERACTIVE_IS_EXPERIMENTAL

#include "libopenmpt.hpp"
#include "libopenmpt_interactive.hpp"

#include "libopenmpt_settings.hpp"

#include "svn_version.h"
static const char * xmp_openmpt_string = "OpenMPT (" OPENMPT_API_VERSION_STRING "." OPENMPT_API_VERSION_STRINGIZE(OPENMPT_VERSION_REVISION) ")";

#define EXPERIMENTAL_VIS

#define FAST_CHECKFILE

#define USE_XMPLAY_FILE_IO

#define USE_XMPLAY_ISTREAM

#define NOMINMAX
#include <windows.h>

#include "xmplay/xmpin.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <sstream>
#include <string>

#include <pugixml.hpp>

#define SHORT_TITLE "xmp-openmpt"
#define SHORTER_TITLE "openmpt"

static CRITICAL_SECTION xmpopenmpt_mutex;
class xmpopenmpt_lock {
public:
	xmpopenmpt_lock() {
		EnterCriticalSection( &xmpopenmpt_mutex );
	}
	~xmpopenmpt_lock() {
		LeaveCriticalSection( &xmpopenmpt_mutex );
	}
};

static XMPFUNC_IN * xmpfin = NULL;
static XMPFUNC_MISC * xmpfmisc = NULL;
static XMPFUNC_FILE * xmpffile = NULL;
static XMPFUNC_TEXT * xmpftext = NULL;
static XMPFUNC_STATUS * xmpfstatus = NULL;

static HMODULE settings_dll = NULL;

struct self_xmplay_t;

static self_xmplay_t * self = nullptr;

static void save_options();

static void apply_and_save_options();

struct self_xmplay_t {
	std::size_t samplerate;
	std::size_t num_channels;
	openmpt::settings::settings settings;
	openmpt::interactive_module * mod;
	self_xmplay_t() : samplerate(48000), num_channels(2), settings(TEXT(SHORT_TITLE), false), mod(nullptr) {
		settings.changed = apply_and_save_options;
		settings.load();
	}
	~self_xmplay_t() {
		return;
	}
};

static std::string convert_to_native( const std::string & str ) {
	char * native_string = xmpftext->Utf8( str.c_str(), -1 );
	std::string result = native_string ? native_string : "";
	if ( native_string ) {
		xmpfmisc->Free( native_string );
		native_string = nullptr;
	}
	return result;
}

static std::string StringEncode( const std::wstring &src, UINT codepage )
{
	int required_size = WideCharToMultiByte( codepage, 0, src.c_str(), -1, nullptr, 0, nullptr, nullptr );
	if(required_size <= 0)
	{
		return std::string();
	}
	std::vector<CHAR> encoded_string( required_size );
	WideCharToMultiByte( codepage, 0, src.c_str(), -1, &encoded_string[0], encoded_string.size(), nullptr, nullptr );
	return &encoded_string[0];
}

static std::wstring StringDecode( const std::string & src, UINT codepage )
{
	int required_size = MultiByteToWideChar( codepage, 0, src.c_str(), -1, nullptr, 0 );
	if(required_size <= 0)
	{
		return std::wstring();
	}
	std::vector<WCHAR> decoded_string( required_size );
	MultiByteToWideChar( codepage, 0, src.c_str(), -1, &decoded_string[0], decoded_string.size() );
	return &decoded_string[0];
}

static void save_settings_to_map( std::map<std::string,int> & result, const openmpt::settings::settings & s ) {
	result.clear();
	result[ "Samplerate_Hz" ] = s.samplerate;
	result[ "Channels" ] = s.channels;
	result[ "MasterGain_milliBel" ] = s.mastergain_millibel;
	result[ "SeteroSeparation_Percent" ] = s.stereoseparation;
	result[ "RepeatCount" ] = s.repeatcount;
	result[ "InterpolationFilterLength" ] = s.interpolationfilterlength;
	result[ "VolumeRampingStrength" ] = s.ramping;
}

static inline void load_map_setting( const std::map<std::string,int> & map, const std::string & key, int & val ) {
	std::map<std::string,int>::const_iterator it = map.find( key );
	if ( it != map.end() ) {
		val = it->second;
	}
}

static void load_settings_from_map( openmpt::settings::settings & s, const std::map<std::string,int> & map ) {
	load_map_setting( map, "Samplerate_Hz", s.samplerate );
	load_map_setting( map, "Channels", s.channels );
	load_map_setting( map, "MasterGain_milliBel", s.mastergain_millibel );
	load_map_setting( map, "SeteroSeparation_Percent", s.stereoseparation );
	load_map_setting( map, "RepeatCount", s.repeatcount );
	load_map_setting( map, "InterpolationFilterLength", s.interpolationfilterlength );
	load_map_setting( map, "VolumeRampingStrength", s.ramping );
}

static void load_settings_from_xml( openmpt::settings::settings & s, const std::string & xml ) {
	pugi::xml_document doc;
	doc.load( xml.c_str() );
	pugi::xml_node settings_node = doc.child( "settings" );
	std::map<std::string,int> map;
	for ( pugi::xml_attribute_iterator it = settings_node.attributes_begin(); it != settings_node.attributes_end(); ++it ) {
		map[ it->name() ] = it->as_int();
	}
	load_settings_from_map( s, map );
}

static void save_settings_to_xml( std::string & xml, const openmpt::settings::settings & s ) {
	std::map<std::string,int> map;
	save_settings_to_map( map, s );
	pugi::xml_document doc;
	pugi::xml_node settings_node = doc.append_child( "settings" );
	for ( std::map<std::string,int>::const_iterator it = map.begin(); it != map.end(); ++it ) {
		settings_node.append_attribute( it->first.c_str() ).set_value( it->second );
	}
	std::ostringstream buf;
	doc.save( buf );
	xml = buf.str();
}

static void apply_options() {
	if ( self->mod ) {
		self->mod->set_repeat_count( self->settings.repeatcount );
		self->mod->set_render_param( openmpt::module::RENDER_MASTERGAIN_MILLIBEL, self->settings.mastergain_millibel );
		self->mod->set_render_param( openmpt::module::RENDER_STEREOSEPARATION_PERCENT, self->settings.stereoseparation );
		self->mod->set_render_param( openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH, self->settings.interpolationfilterlength );
		self->mod->set_render_param( openmpt::module::RENDER_VOLUMERAMPING_STRENGTH, self->settings.ramping );
	}
}

static void save_options() {
	self->settings.save();
}

static void apply_and_save_options() {
	apply_options();
	save_options();
}

static void reset_options() {
	self->settings = openmpt::settings::settings( TEXT(SHORT_TITLE), self->settings.with_outputformat );
	self->settings.changed = apply_and_save_options;
	self->settings.load();
}

// get config (return size of config data) (OPTIONAL)
static DWORD WINAPI openmpt_GetConfig( void * config ) {
	std::string xml;
	save_settings_to_xml( xml, self->settings );
	if ( config ) {
		std::memcpy( config, xml.c_str(), xml.length() + 1 );
	}
	return xml.length() + 1;
}

// apply config (OPTIONAL)
static void WINAPI openmpt_SetConfig( void * config, DWORD size ) {
	reset_options();
	if ( config ) {
		load_settings_from_xml( self->settings, std::string( (char*)config, (char*)config + size ) );
	}
}

#ifdef EXPERIMENTAL_VIS
static double timeinfo_position = 0.0;
struct timeinfo {
	bool valid;
	double seconds;
	std::int32_t pattern;
	std::int32_t row;
};
static std::queue<timeinfo> timeinfos;
static void reset_timeinfos( double position = 0.0 ) {
	while ( !timeinfos.empty() ) {
		timeinfos.pop();
	}
	timeinfo_position = position;
}
static void update_timeinfos( std::int32_t samplerate, std::int32_t count ) {
	timeinfo_position += (double)count / (double)samplerate;
	timeinfo info;
	info.valid = true;
	info.seconds = timeinfo_position;
	info.pattern = self->mod->get_current_pattern();
	info.row = self->mod->get_current_row();
	timeinfos.push( info );
}

static timeinfo current_timeinfo;

static timeinfo lookup_timeinfo( double seconds ) {
	timeinfo info = current_timeinfo;
#if 0
	info.seconds = timeinfo_position;
	info.pattern = self->mod->get_current_pattern();
	info.row = self->mod->get_current_row();
#endif
	while ( timeinfos.size() > 0 && timeinfos.front().seconds <= seconds ) {
		info = timeinfos.front();
		timeinfos.pop();
	}
	current_timeinfo = info;
	return current_timeinfo;
}

static void clear_current_timeinfo() {
	current_timeinfo = timeinfo();
}
#else
static void update_timeinfos( std::int32_t samplerate, std::int32_t count ) {
}
static void reset_timeinfos( double position = 0.0 ) {
}
static void clear_current_timeinfo() {
}
#endif

static void WINAPI openmpt_About( HWND win ) {
	std::ostringstream about;
	about << SHORT_TITLE << " version " << openmpt::string::get( openmpt::string::library_version ) << " " << "(built " << openmpt::string::get( openmpt::string::build ) << ")" << std::endl;
	about << " Copyright (c) 2013 OpenMPT developers (http://openmpt.org/)" << std::endl;
	about << " OpenMPT version " << openmpt::string::get( openmpt::string::core_version ) << std::endl;
	about << std::endl;
	about << openmpt::string::get( openmpt::string::contact ) << std::endl;
	about << std::endl;
	about << "Show full credits?" << std::endl;
	if ( MessageBox( win, StringDecode( about.str(), CP_UTF8 ).c_str(), TEXT(SHORT_TITLE), MB_ICONINFORMATION | MB_YESNOCANCEL | MB_DEFBUTTON1 ) != IDYES ) {
		return;
	}
	std::ostringstream credits;
	credits << openmpt::string::get( openmpt::string::credits );
	credits << "Additional thanks to:" << std::endl;
	credits << std::endl;
	credits << "Arseny Kapoulkine for pugixml" << std::endl;
	credits << "http://pugixml.org/" << std::endl;
	MessageBox( win, StringDecode( credits.str(), CP_UTF8 ).c_str(), TEXT(SHORT_TITLE), MB_ICONINFORMATION );
}

static void WINAPI openmpt_Config( HWND win ) {
	if ( settings_dll ) {
		if ( (libopenmpt_settings_edit_func)GetProcAddress( settings_dll, "libopenmpt_settings_edit" ) )  {
			((libopenmpt_settings_edit_func)GetProcAddress( settings_dll, "libopenmpt_settings_edit" ))( &self->settings, win, SHORT_TITLE );
		}
		apply_and_save_options();
	} else {
		MessageBox( win, TEXT("libopenmpt_settings.dll failed to load. Please check if it is in the same folder as xmp-openmpt.dll and that .NET framework v4.0 is installed."), TEXT(SHORT_TITLE), MB_ICONERROR );
	}
}

#ifdef USE_XMPLAY_FILE_IO

#ifdef USE_XMPLAY_ISTREAM 

class xmplay_streambuf : public std::streambuf {
public:
	explicit xmplay_streambuf( XMPFILE & file );
private:
	int_type underflow();
	xmplay_streambuf( const xmplay_streambuf & );
	xmplay_streambuf & operator = ( const xmplay_streambuf & );
private:
	XMPFILE & file;
	static const std::size_t put_back = 4096;
	static const std::size_t buf_size = 65536;
	std::vector<char> buffer;
}; // class xmplay_streambuf

xmplay_streambuf::xmplay_streambuf( XMPFILE & file_ ) : file(file_), buffer(buf_size) {
	char * end = &buffer.front() + buffer.size();
	setg( end, end, end );
}

std::streambuf::int_type xmplay_streambuf::underflow() {
	if ( gptr() < egptr() ) {
		return traits_type::to_int_type( *gptr() );
	}
	char * base = &buffer.front();
	char * start = base;
	if ( eback() == base ) {
		std::size_t put_back_count = std::min<std::size_t>( put_back, egptr() - base );
		std::memmove( base, egptr() - put_back_count, put_back_count );
		start += put_back_count;
	}
	std::size_t n = xmpffile->Read( file, start, buffer.size() - ( start - base ) );
	if ( n == 0 ) {
		return traits_type::eof();
	}
	setg( base, start, start + n );
	return traits_type::to_int_type( *gptr() );
}

class xmplay_istream : public std::istream {
private:
	xmplay_streambuf buf;
private:
	xmplay_istream( const xmplay_istream & );
	xmplay_istream & operator = ( const xmplay_istream & );
public:
	xmplay_istream( XMPFILE & file ) : std::istream(&buf), buf(file) {
		return;
	}
	~xmplay_istream() {
		return;
	}
}; // class xmplay_istream

#else // !USE_XMPLAY_ISTREAM

static __declspec(deprecated) std::vector<char> read_XMPFILE( XMPFILE & file ) {
	std::vector<char> data( xmpffile->GetSize( file ) );
	if ( data.size() != xmpffile->Read( file, data.data(), data.size() ) ) {
		return std::vector<char>();
	}
	return data;
}

#endif // USE_XMPLAY_ISTREAM

#endif // USE_XMPLAY_FILE_IO

static std::string string_replace( std::string str, const std::string & oldStr, const std::string & newStr ) {
	std::size_t pos = 0;
	while((pos = str.find(oldStr, pos)) != std::string::npos)
	{
		str.replace(pos, oldStr.length(), newStr);
		pos += newStr.length();
	}
	return str;
}

static void write_xmplay_string( char * dst, std::string src ) {
	// xmplay buffers are ~40kB, be conservative and truncate at 32kB-2
	if ( !dst ) {
		return;
	}
	src = src.substr( 0, (1<<15) - 2 );
	std::strcpy( dst, src.c_str() );
}

static void write_xmplay_tag( char * * tag, const std::string & value ) {
	if ( value == "" ) {
		// empty value, do not update tag
		return;
	}
	std::string old_value;
	std::string new_value;
	if ( *tag ) {
		// read old value
		old_value = *tag;
	}
	if ( old_value.length() > 0 ) {
		// add our new value to it
		new_value = old_value + "/" + value;
	} else {
		// our new value
		new_value = value;
	}
	// allocate memory
	if ( *tag ) {
		*tag = (char*)xmpfmisc->ReAlloc( *tag, new_value.length() + 1 );
	} else {
		*tag = (char*)xmpfmisc->Alloc( new_value.length() + 1 );
	}
	// set new value
	if ( *tag ) {
		std::strcpy( *tag, new_value.c_str() );
	}
}

static void write_xmplay_tags( char * tags[8], const openmpt::module & mod ) {
	write_xmplay_tag( &tags[0], convert_to_native( mod.get_metadata("title") ) );
	write_xmplay_tag( &tags[1], convert_to_native( mod.get_metadata("artist") ) );
	write_xmplay_tag( &tags[2], convert_to_native( mod.get_metadata("xmplay-album") ) ); // todo, libopenmpt does not support that
	write_xmplay_tag( &tags[3], convert_to_native( mod.get_metadata("xmplay-date") ) ); // todo, libopenmpt does not support that
	write_xmplay_tag( &tags[4], convert_to_native( mod.get_metadata("xmplay-tracknumber") ) ); // todo, libopenmpt does not support that
	write_xmplay_tag( &tags[5], convert_to_native( mod.get_metadata("xmplay-genre") ) ); // todo, libopenmpt does not support that
	write_xmplay_tag( &tags[6], convert_to_native( mod.get_metadata("message") ) );
	write_xmplay_tag( &tags[7], convert_to_native( mod.get_metadata("type") ) );
}

static void clear_xmlpay_tags( char * tags[8] ) {
	// leave tags alone
}

static void clear_xmplay_string( char * str ) {
	if ( !str ) {
		return;
	}
	str[0] = '\0';
}

static std::string sanitize_xmplay_info_string( const std::string & str ) {
	std::string result;
	for ( std::size_t i = 0; i < str.length(); ++i ) {
		const char c = str[i];
		switch ( c ) {
			case '\0':
			case '\t':
			case '\r':
			case '\n':
				break;
			default:
				result.push_back( c );
				break;
		}
	}
	return result;
}

static std::string sanitize_xmplay_multiline_string( const std::string & str ) {
	std::string result;
	for ( std::size_t i = 0; i < str.length(); ++i ) {
		const char c = str[i];
		switch ( c ) {
			case '\0':
			case '\t':
				break;
			default:
				result.push_back( c );
				break;
		}
	}
	return result;
}

// check if a file is playable by this plugin
// more thorough checks can be saved for the GetFileInfo and Open functions
static BOOL WINAPI openmpt_CheckFile( const char * filename, XMPFILE file ) {
	#ifdef FAST_CHECKFILE
		std::string fn = filename;
		std::size_t dotpos = fn.find_last_of( '.' );
		std::string ext;
		if ( dotpos != std::string::npos ) {
			ext = fn.substr( dotpos + 1 );
		}
		std::transform( ext.begin(), ext.end(), ext.begin(), tolower );
		return openmpt::is_extension_supported( ext ) ? TRUE : FALSE;
	#else // !FAST_CHECKFILE
		const double threshold = 0.1;
		#ifdef USE_XMPLAY_FILE_IO
			#ifdef USE_XMPLAY_ISTREAM
				switch ( xmpffile->GetType( file ) ) {
					case XMPFILE_TYPE_MEMORY:
						return openmpt::module::could_open_propability( xmpffile->GetMemory( file ), xmpffile->GetSize( file ) ) > threshold;
						break;
					case XMPFILE_TYPE_FILE:
					case XMPFILE_TYPE_NETFILE:
					case XMPFILE_TYPE_NETSTREAM:
					default:
						return openmpt::module::could_open_propability( (xmplay_istream( file )) ) > threshold;
						break;
				}
			#else
				if ( xmpffile->GetType( file ) == XMPFILE_TYPE_MEMORY ) {
					return openmpt::module::could_open_propability( xmpffile->GetMemory( file ), xmpffile->GetSize( file ) ) > threshold;
				} else {
					return openmpt::module::could_open_propability( (read_XMPFILE( file )) ) > threshold;
				}
			#endif
		#else
			return openmpt::module::could_open_propability( std::ifstream( filename, std::ios_base::binary ) ) > threshold;
		#endif
	#endif // FAST_CHECKFILE
}

//tags: 0=title,1=artist,2=album,3=year,4=track,5=genre,6=comment,7=filetype
static BOOL WINAPI openmpt_GetFileInfo( const char * filename, XMPFILE file, float * length, char * tags[8] ) {
	try {
		#ifdef USE_XMPLAY_FILE_IO
			#ifdef USE_XMPLAY_ISTREAM
				switch ( xmpffile->GetType( file ) ) {
					case XMPFILE_TYPE_MEMORY:
						{
							openmpt::module mod( xmpffile->GetMemory( file ), xmpffile->GetSize( file ) );
							if ( length ) *length = static_cast<float>( mod.get_duration_seconds() );
							write_xmplay_tags( tags, mod );
						}
						break;
					case XMPFILE_TYPE_FILE:
					case XMPFILE_TYPE_NETFILE:
					case XMPFILE_TYPE_NETSTREAM:
					default:
						{
							xmplay_istream stream( file );
							openmpt::module mod( stream );
							if ( length ) *length = static_cast<float>( mod.get_duration_seconds() );
							write_xmplay_tags( tags, mod );
						}
						break;
				}
			#else
				if ( xmpffile->GetType( file ) == XMPFILE_TYPE_MEMORY ) {
					openmpt::module mod( xmpffile->GetMemory( file ), xmpffile->GetSize( file ) );
					if ( length ) *length = static_cast<float>( mod.get_duration_seconds() );
					write_xmplay_tags( tags, mod );
				} else {
					openmpt::module mod( (read_XMPFILE( file )) );
					if ( length ) *length = static_cast<float>( mod.get_duration_seconds() );
					write_xmplay_tags( tags, mod );
				}
			#endif
		#else
			openmpt::module mod( std::ifstream( filename, std::ios_base::binary ) );
			if ( length ) *length = static_cast<float>( mod.get_duration_seconds() );
			write_xmplay_tags( tags, mod );
		#endif
	} catch ( ... ) {
		if ( length ) *length = 0.0f;
		clear_xmlpay_tags( tags );
		return FALSE;
	}
	return TRUE;
}

// open a file for playback
// return:  0=failed, 1=success, 2=success and XMPlay can close the file
static DWORD WINAPI openmpt_Open( const char * filename, XMPFILE file ) {
	xmpopenmpt_lock guard;
	reset_options();
	try {
		if ( self->mod ) {
			delete self->mod;
			self->mod = nullptr;
		}
		#ifdef USE_XMPLAY_FILE_IO
			#ifdef USE_XMPLAY_ISTREAM
				switch ( xmpffile->GetType( file ) ) {
					case XMPFILE_TYPE_MEMORY:
						self->mod = new openmpt::interactive_module( xmpffile->GetMemory( file ), xmpffile->GetSize( file ) );
						break;
					case XMPFILE_TYPE_FILE:
					case XMPFILE_TYPE_NETFILE:
					case XMPFILE_TYPE_NETSTREAM:
					default:
						{
							xmplay_istream stream( file );
							self->mod = new openmpt::interactive_module( stream );
						}
						break;
				}
			#else
				if ( xmpffile->GetType( file ) == XMPFILE_TYPE_MEMORY ) {
					self->mod = new openmpt::interactive_module( xmpffile->GetMemory( file ), xmpffile->GetSize( file ) );
				} else {
					self->mod = new openmpt::interactive_module( (read_XMPFILE( file )) );
				}
			#endif
		#else
			self->mod = new openmpt::interactive_module( std::ifstream( filename, std::ios_base::binary ) );
		#endif
		clear_current_timeinfo();
		reset_timeinfos();
		apply_options();
		self->samplerate = self->settings.samplerate;
		self->num_channels = self->settings.channels;
		xmpfin->SetLength( static_cast<float>( self->mod->get_duration_seconds() ), TRUE );
		return 2;
	} catch ( ... ) {
		if ( self->mod ) {
			delete self->mod;
			self->mod = nullptr;
		}
		return 0;
	}
	return 0;
}

// close the file
static void WINAPI openmpt_Close() {
	xmpopenmpt_lock guard;
	if ( self->mod ) {
		delete self->mod;
		self->mod = nullptr;
	}
}

// set the sample format (in=user chosen format, out=file format if different)
static void WINAPI openmpt_SetFormat( XMPFORMAT * form ) {
	if ( !form ) {
		return;
	}
	if ( !self->mod ) {
		form->rate = 0;
		form->chan = 0;
		form->res = 0;
		return;
	}
	if ( self->settings.samplerate != 0 ) {
		form->rate = self->samplerate;
	} else {
		if ( form->rate && form->rate > 0 ) {
			self->samplerate = form->rate;
		} else {
			form->rate = 48000;
			self->samplerate = 48000;
		}
	}
	if ( self->settings.channels != 0 ) {
		form->chan = self->num_channels;
	} else {
		if ( form->chan && form->chan > 0 ) {
			if ( form->chan > 2 ) {
				form->chan = 4;
				self->num_channels = 4;
			} else {
				self->num_channels = form->chan;
			}
		} else {
			form->chan = 2;
			self->num_channels = 2;
		}
	}
	form->res = 4; // float
}

// get the tags
// return TRUE to delay the title update when there are no tags (use UpdateTitle to request update when ready)
static BOOL WINAPI openmpt_GetTags( char * tags[8] ) {
	if ( !self->mod ) {
		clear_xmlpay_tags( tags );
		return FALSE;
	}
	write_xmplay_tags( tags, *self->mod );
	return FALSE; // TRUE would delay
}

// get the main panel info text
static void WINAPI openmpt_GetInfoText( char * format, char * length ) {
	if ( !self->mod ) {
		clear_xmplay_string( format );
		clear_xmplay_string( length );
		return;
	}
	if ( format ) {
		std::ostringstream str;
		str
			<< self->mod->get_metadata("type")
			<< " - "
			<< self->mod->get_num_channels() << " ch"
			<< " - "
			<< "(via " << SHORTER_TITLE << ")"
			;
		write_xmplay_string( format, sanitize_xmplay_info_string( str.str() ) );
	}
	if ( length ) {
		std::ostringstream str;
		str
			<< length
			<< " - "
			<< self->mod->get_num_orders() << " orders"
			;
		write_xmplay_string( length, sanitize_xmplay_info_string( str.str() ) );
	}
}

// get text for "General" info window
// separate headings and values with a tab (\t), end each line with a carriage-return (\r)
static void WINAPI openmpt_GetGeneralInfo( char * buf ) {
	if ( !self->mod ) {
		clear_xmplay_string( buf );
		return;
	}
	std::ostringstream str;
	str
		<< "\r"
		<< "Format" << "\t" << sanitize_xmplay_info_string( self->mod->get_metadata("type") ) << " (" << sanitize_xmplay_info_string( self->mod->get_metadata("type_long") ) << ")" << "\r";
	if ( !self->mod->get_metadata("container").empty() ) {
		str << "Container" << "\t"  << sanitize_xmplay_info_string( self->mod->get_metadata("container") ) << " (" << sanitize_xmplay_info_string( self->mod->get_metadata("container_long") ) << ")" << "\r";
	}
	str
		<< "Channels" << "\t" << self->mod->get_num_channels() << "\r"
		<< "Orders" << "\t" << self->mod->get_num_orders() << "\r"
		<< "Patterns" << "\t" << self->mod->get_num_patterns() << "\r"
		<< "Instruments" << "\t" << self->mod->get_num_instruments() << "\r"
		<< "Samples" << "\t" << self->mod->get_num_samples() << "\r"
		<< "\r"
		<< "Tracker" << "\t" << sanitize_xmplay_info_string( self->mod->get_metadata("tracker") ) << "\r"
		<< "Player" << "\t" << "xmp-openmpt" << " version " << openmpt::string::get( openmpt::string::library_version ) << "\r"
		;
	std::string warnings = self->mod->get_metadata("warnings");
	if ( !warnings.empty() ) {
		str << "Warnings" << "\t" << sanitize_xmplay_info_string( string_replace( warnings, "\n", "\r\t" ) ) << "\r";
	}
	str << "\r";
	write_xmplay_string( buf, str.str() );
}

// get text for "Message" info window
// separate tag names and values with a tab (\t), and end each line with a carriage-return (\r)
static void WINAPI openmpt_GetMessage( char * buf ) {
	if ( !self->mod ) {
		clear_xmplay_string( buf );
		return;
	}
	write_xmplay_string( buf, convert_to_native( sanitize_xmplay_multiline_string( string_replace( self->mod->get_metadata("message"), "\n", "\r" ) ) ) );
}

// Seek to a position (in granularity units)
// return the new position in seconds (-1 = failed)
static double WINAPI openmpt_SetPosition( DWORD pos ) {
	if ( !self->mod ) {
		return -1.0;
	}
	double new_position = self->mod->set_position_seconds( static_cast<double>( pos ) * 0.001 );
	reset_timeinfos( new_position );
	return new_position;
}

// Get the seeking granularity in seconds
static double WINAPI openmpt_GetGranularity() {
	return 0.001;
}

// get some sample data, always floating-point
// count=number of floats to write (not bytes or samples)
// return number of floats written. if it's less than requested, playback is ended...
// so wait for more if there is more to come (use CheckCancel function to check if user wants to cancel)
static DWORD WINAPI openmpt_Process( float * dstbuf, DWORD count ) {
	xmpopenmpt_lock guard;
	if ( !self->mod || self->num_channels == 0 ) {
		return 0;
	}
	update_timeinfos( self->samplerate, 0 );
	std::size_t frames = count / self->num_channels;
	std::size_t frames_to_render = frames;
	std::size_t frames_rendered = 0;
	while ( frames_to_render > 0 ) {
		std::size_t frames_chunk = std::min<std::size_t>( frames_to_render, self->samplerate / 100 ); // 100 Hz timing info update interval
		switch ( self->num_channels ) {
		case 1:
			{
				frames_chunk = self->mod->read( self->samplerate, frames_chunk, dstbuf );
			}
			break;
		case 2:
			{
				frames_chunk = self->mod->read_interleaved_stereo( self->samplerate, frames_chunk, dstbuf );
			}
			break;
		case 4:
			{
				frames_chunk = self->mod->read_interleaved_quad( self->samplerate, frames_chunk, dstbuf );
			}
			break;
		}
		dstbuf += frames_chunk * self->num_channels;
		if ( frames_chunk == 0 ) {
			break;
		}
		update_timeinfos( self->samplerate, frames_chunk );
		frames_to_render -= frames_chunk;
		frames_rendered += frames_chunk;
	}
	if ( frames_rendered == 0 ) {
		return 0;
	}
	return frames_rendered * self->num_channels;
}

static void add_names( std::ostream & str, const std::string & title, const std::vector<std::string> & names ) {
	if ( names.size() > 0 ) {
		bool valid = false;
		for ( std::size_t i = 0; i < names.size(); i++ ) {
			if ( names[i] != "" ) {
				valid = true;
			}
		}
		if ( !valid ) {
			return;
		}
		str << title << " names:" << "\r";
		for ( std::size_t i = 0; i < names.size(); i++ ) {
			str << std::setfill('0') << std::setw(2) << i << std::setw(0) << "\t" << convert_to_native( names[i] ) << "\r";
		}
		str << "\r";
	}
}

static void WINAPI openmpt_GetSamples( char * buf ) {
	if ( !self->mod ) {
		clear_xmplay_string( buf );
		return;
	}
	std::ostringstream str;
	add_names( str, "instrument", self->mod->get_instrument_names() );
	add_names( str, "sample", self->mod->get_sample_names() );
	add_names( str, "channel", self->mod->get_channel_names() );
	add_names( str, "order", self->mod->get_order_names() );
	add_names( str, "pattern", self->mod->get_pattern_names() );
	write_xmplay_string( buf, str.str() );
}

#ifdef EXPERIMENTAL_VIS

enum ColorIndex
{
	col_background = 0,
	col_unknown,
	col_text,
	col_empty,
	col_instr,
	col_vol,
	col_pitch,

	col_max
};

const struct Columns
{
	int num_chars;
	int color;
} pattern_columns[] = {
	{ 3, col_text },	// C-5
	{ 2, col_instr },	// 01
	{ 3, col_vol },		// v64
	{ 3, col_pitch },	// EFF
};

static void assure_width( std::string & str, std::size_t width ) {
	if ( str.length() == width ) {
		return;
	} else if ( str.length() < width ) {
		str += std::string( width - str.length(), ' ' );
	} else if ( str.length() > width ) {
		str = str.substr( 0, width );
	}
}

union Color
{
	struct { uint8_t r, g, b, a; };
	COLORREF dw;
};

HDC visDC;
HGDIOBJ visbitmap;

Color viscolors[col_max];
HPEN vispens[col_max];
HBRUSH visbrushs[col_max];
HFONT visfont;
static int last_pattern = -1;

static BOOL WINAPI VisOpen(DWORD colors[3]) {
	xmpopenmpt_lock guard;
	visDC = nullptr;
	visbitmap = nullptr;
	visfont = nullptr;

	viscolors[col_background].dw = colors[0];
	viscolors[col_unknown].dw = colors[1];
	viscolors[col_text].dw = colors[2];

	const int r = viscolors[col_text].r, g = viscolors[col_text].g, b = viscolors[col_text].b;
	viscolors[col_empty].r = (r + viscolors[col_background].r) / 2;
	viscolors[col_empty].g = (g + viscolors[col_background].g) / 2;
	viscolors[col_empty].b = (b + viscolors[col_background].b) / 2;
	viscolors[col_empty].a = 0;

#define MIXCOLOR(col, c1, c2, c3) { \
	viscolors[col] = viscolors[col_text]; \
	int mix = viscolors[col].c1 + 0xA0; \
	viscolors[col].c1 = mix; \
	if ( mix > 0xFF ) { \
		viscolors[col].c2 = std::max<uint8_t>( c2 - viscolors[col].c1 / 2, 0 ); \
		viscolors[col].c3 = std::max<uint8_t>( c3 - viscolors[col].c1 / 2, 0 ); \
		viscolors[col].c1 = 0xFF; \
	} }

	MIXCOLOR(col_instr, g, r, b);
	MIXCOLOR(col_vol, b, r, g);
	MIXCOLOR(col_pitch, r, g, b);
#undef MIXCOLOR

	for( int i = 0; i < col_max; ++i ) {
		vispens[i] = CreatePen( PS_SOLID, 1, viscolors[i].dw );
		visbrushs[i] = CreateSolidBrush( viscolors[i].dw );
	}

	clear_current_timeinfo();

	if ( !self->mod ) {
		return FALSE;
	}
	return TRUE;
}
static void WINAPI VisClose() {
	xmpopenmpt_lock guard;

	for( int i = 0; i < col_max; ++i ) {
		DeleteObject( vispens[i] );
		DeleteObject( visbrushs[i] );
	}

	DeleteObject( visfont );
	DeleteObject( visbitmap );
	DeleteDC( visDC );
}
static void WINAPI VisSize(HDC dc, SIZE *size) {
	xmpopenmpt_lock guard;
	last_pattern = -1;	// Force redraw
}
static BOOL WINAPI VisRender(DWORD *buf, SIZE size, DWORD flags) {
	xmpopenmpt_lock guard;
	return FALSE;
}
static BOOL WINAPI VisRenderDC(HDC dc, SIZE size, DWORD flags) {
	xmpopenmpt_lock guard;
	RECT rect;

	if( visfont == nullptr ) {
		// Force usage of a nice monospace font
		LOGFONT logfont;
		GetObject ( GetCurrentObject( dc, OBJ_FONT ), sizeof(logfont), &logfont );
		wcscpy( logfont.lfFaceName, L"Lucida Console" );
		visfont = CreateFontIndirect( &logfont );
	}
	SIZE text_size;
	SelectObject( dc, visfont );
	if ( GetTextExtentPoint32( dc, TEXT("W"), 1, &text_size ) == FALSE ) {
		return FALSE;
	}

	if(flags & XMPIN_VIS_INIT)
	{
		last_pattern = -1;
	}

	timeinfo info = lookup_timeinfo( xmpfstatus->GetTime() );

	if ( !info.valid ) {
		RECT bgrect;
		bgrect.top = 0;
		bgrect.left = 0;
		bgrect.right = size.cx;
		bgrect.bottom = size.cy;
		FillRect(dc, &bgrect, visbrushs[col_background]);
		return TRUE;
	}

	int pattern = info.pattern;
	int current_row = info.row;

	const std::size_t channels = self->mod->get_num_channels();
	const std::size_t rows = self->mod->get_pattern_num_rows( pattern );

	const std::size_t num_half_chars = std::max<std::size_t>( 2 * size.cx / text_size.cx, 8 ) - 8;
	const std::size_t num_rows = size.cy / text_size.cy;

	// Spaces between pattern components are half width, full space at channel end
	const std::size_t half_chars_per_channel = num_half_chars / channels;
	std::size_t chars_per_channel, spaces_per_channel;
	std::size_t num_cols;
	for ( num_cols = sizeof ( pattern_columns ) / sizeof ( pattern_columns[0] ); num_cols >= 1; num_cols-- ) {
		chars_per_channel = 0;
		spaces_per_channel = num_cols > 1 ? num_cols : 0;	// No extra space if we only display notes
		for ( std::size_t i = 0; i < num_cols; i++ ) {
			chars_per_channel += pattern_columns[i].num_chars;
		}

		if ( half_chars_per_channel >= chars_per_channel * 2 + spaces_per_channel + 1 || num_cols == 1 ) {
			break;
		}
	}

	int pattern_width = ((chars_per_channel * channels + 4) * text_size.cx) + (spaces_per_channel * channels + channels - (num_cols == 1 ? 1 : 2)) * (text_size.cx / 2);
	int pattern_height = rows * text_size.cy;

	if ( visDC == nullptr || last_pattern != pattern ) {
		DeleteObject( visbitmap );
		DeleteDC( visDC );

		visDC = CreateCompatibleDC( dc );
		visbitmap = CreateCompatibleBitmap( dc, pattern_width, pattern_height );
		SelectObject( visDC, visbitmap );

		SelectObject( visDC, vispens[col_unknown] );
		SelectObject( visDC, visbrushs[col_background] );

		SelectObject( visDC, visfont );

		rect.top = 0;
		rect.left = 0;
		rect.right = pattern_width;
		rect.bottom = pattern_height;
		FillRect( visDC, &rect, visbrushs[col_background] );

		SetBkColor( visDC, viscolors[col_background].dw );

		POINT pos;
		pos.y = 0;

		for ( std::size_t row = 0; row < rows; row++ ) {
			pos.x = 0;

			std::ostringstream s;
			s.imbue(std::locale::classic());
			s << std::setfill('0') << std::setw(3) << row;
			const std::string rowstr = s.str();

			SetTextColor( visDC, viscolors[1].dw );
			TextOutA( visDC, pos.x, pos.y, rowstr.c_str(), rowstr.length() );
			pos.x += 4 * text_size.cx;

			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				// "NNN IIvVV EFF"
				//std::string highlight = self->mod->highlight_pattern_row_channel( pattern, row, channel, cols_per_channel );
				//std::string chan = self->mod->format_pattern_row_channel( pattern, row, channel, cols_per_channel );

				for ( std::size_t col = 0; col < num_cols; ++col ) {
					std::string col_string;
					switch ( col ) {
						case 0:
							col_string += self->mod->format_pattern_row_channel_command( pattern, row, channel, openmpt::module::command_note );
							break;
						case 1:
							col_string += self->mod->format_pattern_row_channel_command( pattern, row, channel, openmpt::module::command_instrument );
							break;
						case 2:
							col_string += self->mod->format_pattern_row_channel_command( pattern, row, channel, openmpt::module::command_volumeffect );
							col_string += self->mod->format_pattern_row_channel_command( pattern, row, channel, openmpt::module::command_volume );
							break;
						case 3:
							col_string += self->mod->format_pattern_row_channel_command( pattern, row, channel, openmpt::module::command_effect );
							col_string += self->mod->format_pattern_row_channel_command( pattern, row, channel, openmpt::module::command_parameter );
							break;
					}
					assure_width( col_string, pattern_columns[col].num_chars );
					int color = pattern_columns[col].color;
					switch ( col_string[0] ) {
						case ' ':
						case '.':
						case '^':
						case '=':
						case '~':
							color = col_empty;
							break;
					}

					SetTextColor( visDC, viscolors[color].dw );
					TextOutA( visDC, pos.x, pos.y, col_string.c_str(), col_string.length() );
					pos.x += pattern_columns[col].num_chars * text_size.cx + text_size.cx / 2;
				}
				// Extra padding
				if ( num_cols > 1 ) {
					pos.x += text_size.cx / 2;
				}
			}

			pos.y += text_size.cy;
		}
	}

	rect.top = 0;
	rect.left = 0;
	rect.right = size.cx;
	rect.bottom = size.cy;
	FillRect( dc, &rect, visbrushs[col_background] );

	int offset_x = (size.cx - pattern_width) / 2;
	int offset_y = (size.cy - text_size.cy) / 2 - current_row * text_size.cy;
	int src_offset_x = 0;
	int src_offset_y = 0;

	if ( offset_x < 0 ) {
		src_offset_x -= offset_x;
		pattern_width = std::min<int>( pattern_width + offset_x, size.cx );
		offset_x = 0;
	}

	if ( offset_y < 0 ) {
		src_offset_y -= offset_y;
		pattern_height = std::min<int>( pattern_height + offset_y, size.cy );
		offset_y = 0;
	}

	BitBlt( dc, offset_x, offset_y, pattern_width, pattern_height, visDC, src_offset_x, src_offset_y , SRCCOPY );

	// Highlight current row
	rect.left = offset_x;
	rect.top = (size.cy - text_size.cy) / 2;
	rect.right = rect.left + pattern_width;
	rect.bottom = rect.top + text_size.cy;
	InvertRect( dc, &rect );
	
	last_pattern = pattern;

	return TRUE;
}
static void WINAPI VisButton(DWORD x, DWORD y) {
	xmpopenmpt_lock guard;
}

#endif

static XMPIN xmpin = {
#ifdef USE_XMPLAY_FILE_IO
	0 |
#else
	XMPIN_FLAG_NOXMPFILE |
#endif
	XMPIN_FLAG_CONFIG,// 0, // XMPIN_FLAG_LOOP, the xmplay looping interface is not really compatible with libopenmpt looping interface, so dont support that for now
	xmp_openmpt_string,
	NULL, // "libopenmpt\0mptm/mptmz",
	openmpt_About,
	openmpt_Config,
	openmpt_CheckFile,
	openmpt_GetFileInfo,
	openmpt_Open,
	openmpt_Close,
	NULL, // reserved
	openmpt_SetFormat,
	openmpt_GetTags,
	openmpt_GetInfoText,
	openmpt_GetGeneralInfo,
	openmpt_GetMessage,
	openmpt_SetPosition,
	openmpt_GetGranularity,
	NULL, // GetBuffering
	openmpt_Process,
	NULL, // WriteFile
	openmpt_GetSamples,
	NULL, // GetSubSongs
	NULL, // GetCues
	NULL, // GetDownloaded

#ifdef EXPERIMENTAL_VIS
	"OpenMPT Pattern Display",
	VisOpen,
	VisClose,
	VisSize,
	/*VisRender,*/NULL,
	VisRenderDC,
	/*VisButton,*/NULL,
#else
	0,0,0,0,0,0,0, // no built-in vis
#endif

	NULL, // reserved2
	openmpt_GetConfig,// NULL, // GetConfig
	openmpt_SetConfig// NULL  // SetConfig
};

static const char * xmp_openmpt_default_exts = "OpenMPT\0mptm/mptmz";

static char * file_formats;

static void xmp_openmpt_on_dll_load() {
	ZeroMemory( &xmpopenmpt_mutex, sizeof( xmpopenmpt_mutex ) );
	InitializeCriticalSection( &xmpopenmpt_mutex );
	std::vector<char> filetypes_string;
	filetypes_string.clear();
	std::vector<std::string> extensions = openmpt::get_supported_extensions();
	const char * openmpt_str = "OpenMPT";
	std::copy( openmpt_str, openmpt_str + std::strlen(openmpt_str), std::back_inserter( filetypes_string ) );
	filetypes_string.push_back('\0');
	bool first = true;
	for ( std::vector<std::string>::iterator ext = extensions.begin(); ext != extensions.end(); ++ext ) {
		if ( first ) {
			first = false;
		} else {
			filetypes_string.push_back('/');
		}
		std::copy( (*ext).begin(), (*ext).end(), std::back_inserter( filetypes_string ) );
	}
	filetypes_string.push_back('\0');
	file_formats = (char*)HeapAlloc( GetProcessHeap(), 0, filetypes_string.size() );
	if ( file_formats ) {
		std::copy( filetypes_string.data(), filetypes_string.data() + filetypes_string.size(), file_formats );
		xmpin.exts = file_formats;
	} else {
		xmpin.exts = xmp_openmpt_default_exts;
	}
	settings_dll = LoadLibrary( TEXT("libopenmpt_settings.dll") );
	self = new self_xmplay_t();
}

static void xmp_openmpt_on_dll_unload() {
	delete self;
	self = nullptr;
	if ( settings_dll ) {
		FreeLibrary( settings_dll );
		settings_dll = NULL;
	}
	if ( !xmpin.exts ) {
		return;
	}
	if ( xmpin.exts == xmp_openmpt_default_exts ) {
		xmpin.exts = NULL;
		return;
	}
	HeapFree( GetProcessHeap(), 0, (LPVOID)xmpin.exts );
	xmpin.exts = NULL;
	DeleteCriticalSection( &xmpopenmpt_mutex );
}

static XMPIN * XMPIN_GetInterface_cxx( DWORD face, InterfaceProc faceproc ) {
	if (face!=XMPIN_FACE) return NULL;
	xmpfin=(XMPFUNC_IN*)faceproc(XMPFUNC_IN_FACE);
	xmpfmisc=(XMPFUNC_MISC*)faceproc(XMPFUNC_MISC_FACE);
	xmpffile=(XMPFUNC_FILE*)faceproc(XMPFUNC_FILE_FACE);
	xmpftext=(XMPFUNC_TEXT*)faceproc(XMPFUNC_TEXT_FACE);
	xmpfstatus=(XMPFUNC_STATUS*)faceproc(XMPFUNC_STATUS_FACE);
	return &xmpin;
}

extern "C" {

// XMPLAY expects a WINAPI (which is __stdcall) function using an undecorated symbol name.
XMPIN * WINAPI XMPIN_GetInterface( DWORD face, InterfaceProc faceproc ) {
	return XMPIN_GetInterface_cxx( face, faceproc );
}
#pragma comment(linker, "/EXPORT:XMPIN_GetInterface=_XMPIN_GetInterface@8")

}; // extern "C"

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved ) {
	switch ( fdwReason ) {
	case DLL_PROCESS_ATTACH:
		xmp_openmpt_on_dll_load();
		break;
	case DLL_PROCESS_DETACH:
		xmp_openmpt_on_dll_unload();
		break;
	}
	return TRUE;
}

#endif // NO_XMPLAY
