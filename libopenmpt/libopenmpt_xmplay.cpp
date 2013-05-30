/*
 * libopenmpt_xmplay.cpp
 * ---------------------
 * Purpose: libopenmpt xmplay input plugin implementation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "BuildSettings.h"

#ifndef NO_XMPLAY

#include "libopenmpt_internal.h"

#include "libopenmpt.hpp"

#define LIBOPENMPT_USE_SETTINGS_DLL
#include "libopenmpt_settings.h"

#ifdef _WIN32
#ifdef LIBOPENMPT_BUILD_DLL
#define LIBOPENMPT_XMPLAY_API __declspec(dllexport)
#else
#define LIBOPENMPT_XMPLAY_API
#endif
#else
#define LIBOPENMPT_XMPLAY_API
#endif

//#define CUSTOM_OUTPUT_SETTINGS

//#define EXPERIMENTAL_VIS

#define FAST_CHECKFILE

#define USE_XMPLAY_FILE_IO

#define USE_XMPLAY_ISTREAM

#define NOMINMAX
#include <windows.h>

#include "xmplay/xmpin.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <queue>
#include <sstream>
#include <string>

#define SHORT_TITLE "xmp-openmpt"
#define SHORTER_TITLE "openmpt"

static XMPFUNC_IN * xmpfin = NULL;
static XMPFUNC_MISC * xmpfmisc = NULL;
static XMPFUNC_FILE * xmpffile = NULL;
static XMPFUNC_TEXT * xmpftext = NULL;
static XMPFUNC_STATUS * xmpfstatus = NULL;

static HMODULE settings_dll = NULL;

struct self_xmplay_t;

static self_xmplay_t * self = nullptr;

static void apply_options();

class xmplay_settings : public openmpt::registry_settings {
public:
	void changed() {
		apply_options();
	}
	xmplay_settings() : registry_settings( SHORT_TITLE ) {}
};

struct self_xmplay_t {
	std::size_t samplerate;
	std::size_t num_channels;
	openmpt::settings * settings;
	std::vector<float> buf;
	openmpt::module * mod;
	self_xmplay_t() : samplerate(48000), num_channels(2), settings(nullptr), mod(nullptr) {
		if ( settings_dll ) {
			settings = new xmplay_settings();
		} else {
			settings = new openmpt::settings();
		}
		#ifdef CUSTOM_OUTPUT_SETTINGS
			settings->with_outputformat = true;
		#else
			settings->with_outputformat = false;
		#endif
	}
	~self_xmplay_t() {
		delete settings;
		settings = nullptr;
	}
};

static void apply_options() {
	if ( self->mod ) {
		self->mod->set_render_param( openmpt::module::RENDER_MASTERGAIN_MILLIBEL, self->settings->mastergain_millibel );
		self->mod->set_render_param( openmpt::module::RENDER_STEREOSEPARATION_PERCENT, self->settings->stereoseparation );
		self->mod->set_render_param( openmpt::module::RENDER_REPEATCOUNT, self->settings->repeatcount );
		self->mod->set_render_param( openmpt::module::RENDER_MAXMIXCHANNELS, self->settings->maxmixchannels );
		self->mod->set_render_param( openmpt::module::RENDER_INTERPOLATION_MODE, self->settings->interpolationmode );
		self->mod->set_render_param( openmpt::module::RENDER_VOLUMERAMP_IN_MICROSECONDS, self->settings->volrampinus );
		self->mod->set_render_param( openmpt::module::RENDER_VOLUMERAMP_OUT_MICROSECONDS, self->settings->volrampoutus );
	}
	self->settings->save();
}

#ifdef EXPERIMENTAL_VIS
static double timeinfo_position = 0.0;
struct timeinfo {
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
	info.seconds = timeinfo_position;
	info.pattern = mod->get_current_pattern();
	info.row = mod->get_current_row();
	timeinfos.push( info );
}

static timeinfo current_timeinfo;

static timeinfo lookup_timeinfo( double seconds ) {
	timeinfo info = current_timeinfo;
#if 0
	info.seconds = timeinfo_position;
	info.pattern = mod->get_current_pattern();
	info.row = mod->get_current_row();
#endif
	while ( timeinfos.size() > 0 && timeinfos.front().seconds < seconds ) {
		info.seconds = timeinfos.front().seconds;
		info.pattern = timeinfos.front().pattern;
		info.row = timeinfos.front().row;
		timeinfos.pop();
	}
	current_timeinfo = info;
	return current_timeinfo;
}
#else
static void update_timeinfos( std::int32_t samplerate, std::int32_t count ) {
}
static void reset_timeinfos( double position = 0.0 ) {
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
	if ( MessageBox( win, about.str().c_str(), SHORT_TITLE, MB_ICONINFORMATION | MB_YESNOCANCEL | MB_DEFBUTTON1 ) != IDYES ) {
		return;
	}
	std::ostringstream credits;
	credits << openmpt::string::get( openmpt::string::credits );
	MessageBox( win, credits.str().c_str(), SHORT_TITLE, MB_ICONINFORMATION );
}

static void WINAPI openmpt_Config( HWND win ) {
	if ( settings_dll ) {
		self->settings->edit( win, SHORT_TITLE );
		apply_options();
	} else {
		MessageBox( win, "libopenmpt_settings.dll failed to load. Please check if it is in the same folder as xmp-openmpt.dll and that .NET framework v4.0 is installed.", SHORT_TITLE, MB_ICONERROR );
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
		std::memmove( base, egptr() - put_back, put_back );
		start += put_back;
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
	write_xmplay_tag( &tags[0], mod.get_metadata("title") );
	write_xmplay_tag( &tags[1], mod.get_metadata("author") );
	write_xmplay_tag( &tags[2], mod.get_metadata("xmplay-album") ); // todo, libopenmpt does not support that
	write_xmplay_tag( &tags[3], mod.get_metadata("xmplay-date") ); // todo, libopenmpt does not support that
	write_xmplay_tag( &tags[4], mod.get_metadata("xmplay-tracknumber") ); // todo, libopenmpt does not support that
	write_xmplay_tag( &tags[5], mod.get_metadata("xmplay-genre") ); // todo, libopenmpt does not support that
	write_xmplay_tag( &tags[6], mod.get_metadata("message") );
	write_xmplay_tag( &tags[7], mod.get_metadata("type") );
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
	self->settings->load();
	try {
		if ( self->mod ) {
			delete self->mod;
			self->mod = nullptr;
		}
		#ifdef USE_XMPLAY_FILE_IO
			#ifdef USE_XMPLAY_ISTREAM
				switch ( xmpffile->GetType( file ) ) {
					case XMPFILE_TYPE_MEMORY:
						self->mod = new openmpt::module( xmpffile->GetMemory( file ), xmpffile->GetSize( file ) );
						break;
					case XMPFILE_TYPE_FILE:
					case XMPFILE_TYPE_NETFILE:
					case XMPFILE_TYPE_NETSTREAM:
					default:
						{
							xmplay_istream stream( file );
							self->mod = new openmpt::module( stream );
						}
						break;
				}
			#else
				if ( xmpffile->GetType( file ) == XMPFILE_TYPE_MEMORY ) {
					self->mod = new openmpt::module( xmpffile->GetMemory( file ), xmpffile->GetSize( file ) );
				} else {
					self->mod = new openmpt::module( (read_XMPFILE( file )) );
				}
			#endif
		#else
			self->mod = new openmpt::module( std::ifstream( filename, std::ios_base::binary ) );
		#endif
		reset_timeinfos();
		apply_options();
		self->samplerate = self->settings->samplerate;
		self->num_channels = self->settings->channels;
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
	#ifdef CUSTOM_OUTPUT_SETTINGS
		form->rate = self->samplerate;
		form->chan = self->num_channels;
	#else
		if ( form->rate && form->rate > 0 ) {
			self->samplerate = form->rate;
		} else {
			form->rate = 48000;
			self->samplerate = 48000;
		}
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
		self->settings->samplerate = self->samplerate;
		self->settings->channels = self->num_channels;
	#endif
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
		write_xmplay_string( format, str.str() );
	}
	if ( length ) {
		std::ostringstream str;
		str
			<< length
			<< " - "
			<< self->mod->get_num_orders() << " orders"
			;
		write_xmplay_string( length, str.str() );
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
		<< "Format" << "\t" << self->mod->get_metadata("type") << " (" << self->mod->get_metadata("type_long") << ")" << "\r"
		<< "Channels" << "\t" << self->mod->get_num_channels() << "\r"
		<< "Orders" << "\t" << self->mod->get_num_orders() << "\r"
		<< "Patterns" << "\t" << self->mod->get_num_patterns() << "\r"
		<< "Instruments" << "\t" << self->mod->get_num_instruments() << "\r"
		<< "Samples" << "\t" << self->mod->get_num_samples() << "\r"
		<< "\r"
		<< "Tracker" << "\t" << self->mod->get_metadata("tracker") << "\r"
		<< "Player" << "\t" << "xmp-openmpt" << " version " << openmpt::string::get( openmpt::string::library_version ) << "\r"
		;
	std::string warnings = self->mod->get_metadata("warnings");
	if ( !warnings.empty() ) {
		str << "Warnings" << "\t" << string_replace( warnings, "\n", "\r\t" ) << "\r";
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
	write_xmplay_string( buf, string_replace( self->mod->get_metadata("message"), "\n", "\r" ) );
}

// Seek to a position (in granularity units)
// return the new position in seconds (-1 = failed)
static double WINAPI openmpt_SetPosition( DWORD pos ) {
	if ( !self->mod ) {
		return -1.0;
	}
	double new_position = self->mod->seek_seconds( static_cast<double>( pos ) * 0.001 );
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
	if ( !self->mod ) {
		return 0;
	}
	std::size_t frames = count / self->num_channels;
	self->buf.resize( frames * self->num_channels );
	std::size_t frames_rendered = 0;
	switch ( self->num_channels ) {
	case 1:
		{
			frames_rendered = self->mod->read( self->samplerate, frames, &(self->buf[0*frames]) );
		}
		break;
	case 2:
		{
			frames_rendered = self->mod->read( self->samplerate, frames, &(self->buf[0*frames]), &(self->buf[1*frames]) );
		}
		break;
	case 4:
		{
			frames_rendered = self->mod->read( self->samplerate, frames, &(self->buf[0*frames]), &(self->buf[1*frames]), &(self->buf[2*frames]), &(self->buf[3*frames]) );
		}
		break;
	default:
		return 0;
	}
	update_timeinfos( self->samplerate, frames );
	if ( frames_rendered == 0 ) {
		return 0;
	}
	for ( std::size_t frame = 0; frame < frames_rendered; frame++ ) {
		for ( std::size_t channel = 0; channel < self->num_channels; channel++ ) {
			*dstbuf = self->buf[channel*frames+frame];
			dstbuf++;
		}
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
			str << i << "\t" << names[i] << "\r";
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
	add_names( str, "channel", self->mod->get_channel_names() );
	add_names( str, "order", self->mod->get_order_names() );
	add_names( str, "pattern", self->mod->get_pattern_names() );
	add_names( str, "instrument", self->mod->get_instrument_names() );
	add_names( str, "sample", self->mod->get_sample_names() );
	write_xmplay_string( buf, str.str() );
}

#ifdef EXPERIMENTAL_VIS

std::vector< std::vector< std::vector < std::uint8_t > > > patterns;
DWORD viscolors[3];
HPEN vispens[3];
HBRUSH visbrushs[3];

static char nibble_to_char( std::uint8_t nibble ) {
	if ( nibble < 10 ) {
		return '0' + nibble;
	}
	return 'a' + nibble - 10;
}

static BOOL WINAPI VisOpen(DWORD colors[3]) {
	viscolors[0] = colors[0];
	viscolors[1] = colors[1];
	viscolors[2] = colors[2];
	vispens[0] = CreatePen( PS_SOLID, 1, viscolors[0] );
	vispens[1] = CreatePen( PS_SOLID, 1, viscolors[1] );
	vispens[2] = CreatePen( PS_SOLID, 1, viscolors[2] );
	visbrushs[0] = CreateSolidBrush( viscolors[0] );
	visbrushs[1] = CreateSolidBrush( viscolors[1] );
	visbrushs[2] = CreateSolidBrush( viscolors[2] );
	if ( !mod ) {
		return FALSE;
	}
	patterns.resize( mod->get_num_patterns() );
	for ( std::size_t pattern = 0; pattern < mod->get_num_patterns(); pattern++ ) {
		patterns[pattern].resize( mod->get_pattern_num_rows( pattern ) );
		for ( std::size_t row = 0; row < mod->get_pattern_num_rows( pattern ); row++ ) {
			patterns[pattern][row].resize( mod->get_num_channels() );
			for ( std::size_t channel = 0; channel < mod->get_num_channels(); channel++ ) {
				patterns[pattern][row][channel] = mod->get_pattern_row_channel_command( pattern, row, channel, openmpt::module::command_note );
			}
		}
	}
	return TRUE;
}
static void WINAPI VisClose() {
	DeleteObject( vispens[0] );
	DeleteObject( vispens[1] );
	DeleteObject( vispens[2] );
	DeleteObject( visbrushs[0] );
	DeleteObject( visbrushs[1] );
	DeleteObject( visbrushs[2] );
}
static void WINAPI VisSize(HDC dc, SIZE *size) {

}
static BOOL WINAPI VisRender(DWORD *buf, SIZE size, DWORD flags) {
	return FALSE;
}
static BOOL WINAPI VisRenderDC(HDC dc, SIZE size, DWORD flags) {
	HGDIOBJ oldpen = SelectObject( dc, vispens[1] );
	HGDIOBJ oldbrush = SelectObject( dc, visbrushs[0] );

	SetBkColor( dc, viscolors[0] );
	SetTextColor( dc, viscolors[1] );

	TEXTMETRIC tm;
	GetTextMetrics( dc, &tm );

	int top = 0;

	int channels = mod->get_num_channels();

#if 0
	int pattern = mod->get_current_pattern();
	int current_row = mod->get_current_row();
#else
	timeinfo info = lookup_timeinfo( timeinfo_position - ( (double)xmpfstatus->GetLatency() / (double)samplerate ) );
	int pattern = info.pattern;
	int current_row = info.row;
#endif

	int rows = mod->get_pattern_num_rows( pattern );

	static char buf[1<<16];
	char * dst;
	dst = buf;
	for ( std::size_t row = 0; row < rows; row++ ) {
		dst = buf;
		for ( std::size_t channel = 0; channel < channels; channel++ ) {
			*dst++ = nibble_to_char( ( patterns[pattern][row][channel] >> 4 ) &0xf );
			*dst++ = nibble_to_char( ( patterns[pattern][row][channel] >> 0 ) &0xf );
		}
		*dst++ = '\0';
		if ( row == current_row ) {
			//SelectObject( dc, vispens[2] );
			SetTextColor( dc, viscolors[2] );
		}
		RECT rect;
		rect.top = top;
		rect.left = 0;
		rect.right = size.cx;
		rect.bottom = size.cy;
		DrawText( dc, buf, strlen( buf ), &rect, DT_LEFT );
		top += tm.tmHeight;
		if ( row == current_row ) {
			//SelectObject( dc, vispens[1] );
			SetTextColor( dc, viscolors[1] );
		}
	}

	return TRUE;
}
static void WINAPI VisButton(DWORD x, DWORD y) {

}

#endif

static XMPIN xmpin = {
#ifdef USE_XMPLAY_FILE_IO
	0 |
#else
	XMPIN_FLAG_NOXMPFILE |
#endif
	0, // XMPIN_FLAG_LOOP, the xmplay looping interface is not really compatible with libopenmpt looping interface, so dont support that for now
	openmpt::version::xmp_openmpt_string,
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
	"openmpt pattern",
	VisOpen,
	VisClose,
	/*VisSize,*/NULL,
	/*VisRender,*/NULL,
	VisRenderDC,
	/*VisButton,*/NULL,
#else
	0,0,0,0,0,0,0, // no built-in vis
#endif

	NULL, // reserved2
	NULL, // GetConfig
	NULL  // SetConfig
};

static const char * xmp_openmpt_default_exts = "OpenMPT\0mptm/mptmz";

static char * file_formats;

void xmp_openmpt_on_dll_load() {
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
	settings_dll = LoadLibrary( "libopenmpt_settings.dll" );
	self = new self_xmplay_t();
}

void xmp_openmpt_on_dll_unload() {
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

#endif // NO_XMPLAY
