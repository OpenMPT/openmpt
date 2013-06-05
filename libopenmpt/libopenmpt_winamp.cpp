/*
 * libopenmpt_winamp.cpp
 * ---------------------
 * Purpose: libopenmpt winamp input plugin implementation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "BuildSettings.h"

#ifndef NO_WINAMP

#include "libopenmpt_internal.h"

#include "libopenmpt.hpp"

#define LIBOPENMPT_USE_SETTINGS_DLL
#include "libopenmpt_settings.h"

#define LIBOPENMPT_WINAMP_API LIBOPENMPT_API

#define NOMINMAX
#include <windows.h>

#include "winamp/IN2.H"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

#define BPS 16

#define WINAMP_DSP_HEADROOM_FACTOR 2
#define WINAMP_BUFFER_SIZE_FRAMES  576

#define WM_WA_MPEG_EOF  (WM_USER+2)
#define WM_OPENMPT_SEEK (WM_USER+3)

#define SHORT_TITLE "in_openmpt"

static void apply_options();

class winamp_settings : public openmpt::registry_settings {
public:
	void changed() {
		apply_options();
	}
	winamp_settings() : registry_settings( SHORT_TITLE ) {}
};

static HMODULE settings_dll = NULL;

struct self_winamp_t {
	std::vector<char> filetypes_string;
	openmpt::settings * settings;
	int samplerate;
	int channels;
	std::string cached_filename;
	std::string cached_title;
	int cached_length;
	std::string cached_infotext;
	std::int64_t decode_position_frames;
	openmpt::module * mod;
	HANDLE PlayThread;
	DWORD PlayThreadID;
	bool paused;
	std::vector<std::int16_t> buffer;
	std::vector<std::int16_t> interleaved_buffer;
	self_winamp_t() : settings(nullptr) {
		filetypes_string.clear();
		std::vector<std::string> extensions = openmpt::get_supported_extensions();
		for ( std::vector<std::string>::iterator ext = extensions.begin(); ext != extensions.end(); ++ext ) {
			std::copy( (*ext).begin(), (*ext).end(), std::back_inserter( filetypes_string ) );
			filetypes_string.push_back('\0');
			std::copy( SHORT_TITLE, SHORT_TITLE + std::strlen(SHORT_TITLE), std::back_inserter( filetypes_string ) );
			filetypes_string.push_back('\0');
		}
		filetypes_string.push_back('\0');
		if ( settings_dll ) {
			settings = new winamp_settings();
		} else {
			settings = new openmpt::settings();
		}
		samplerate = settings->samplerate;
		channels = settings->channels;
		cached_filename = "";
		cached_title = "";
		cached_length = 0;
		cached_infotext = "";
		decode_position_frames = 0;
		mod = 0;
		PlayThread = 0;
		PlayThreadID = 0;
		paused = false;
		buffer.resize( WINAMP_BUFFER_SIZE_FRAMES * channels );
		interleaved_buffer.resize( WINAMP_BUFFER_SIZE_FRAMES * channels * WINAMP_DSP_HEADROOM_FACTOR );
	}
	~self_winamp_t() {
		delete settings;
		settings = nullptr;
	}
};

static self_winamp_t * self = 0;

static void apply_options() {
	if ( self->mod ) {
		self->mod->set_render_param( openmpt::module::RENDER_MASTERGAIN_MILLIBEL, self->settings->mastergain_millibel );
		self->mod->set_render_param( openmpt::module::RENDER_STEREOSEPARATION_PERCENT, self->settings->stereoseparation );
		self->mod->set_render_param( openmpt::module::RENDER_REPEATCOUNT, self->settings->repeatcount );
		self->mod->set_render_param( openmpt::module::RENDER_MAXMIXCHANNELS, self->settings->maxmixchannels );
		self->mod->set_render_param( openmpt::module::RENDER_INTERPOLATION_MODE, self->settings->interpolationmode );
		self->mod->set_render_param( openmpt::module::RENDER_VOLUMERAMP_UP_MICROSECONDS, self->settings->volrampinus );
		self->mod->set_render_param( openmpt::module::RENDER_VOLUMERAMP_DOWN_MICROSECONDS, self->settings->volrampoutus );
	}
	self->settings->save();
}

extern In_Module inmod;

static DWORD WINAPI DecodeThread( LPVOID );

static std::string generate_infotext( const std::string & filename, const openmpt::module & mod ) {
	std::ostringstream str;
	str << "filename: " << filename << std::endl;
	str << "duration: " << mod.get_duration_seconds() << "seconds" << std::endl;
	std::vector<std::string> metadatakeys = mod.get_metadata_keys();
	for ( std::vector<std::string>::iterator key = metadatakeys.begin(); key != metadatakeys.end(); ++key ) {
		str << *key << ": " << mod.get_metadata(*key) << std::endl;
	}
	return str.str();
}

static void config( HWND hwndParent ) {
	if ( settings_dll ) {
		self->settings->edit( hwndParent, SHORT_TITLE );
		apply_options();
	} else {
		MessageBox( hwndParent, "libopenmpt_settings.dll failed to load. Please check if it is in the same folder as in_openmpt.dll and that .NET framework v4.0 is installed.", SHORT_TITLE, MB_ICONERROR );
	}
}

static void about( HWND hwndParent ) {
	std::ostringstream about;
	about << SHORT_TITLE << " version " << openmpt::string::get( openmpt::string::library_version ) << " " << "(built " << openmpt::string::get( openmpt::string::build ) << ")" << std::endl;
	about << " Copyright (c) 2013 OpenMPT developers (http://openmpt.org/)" << std::endl;
	about << " OpenMPT version " << openmpt::string::get( openmpt::string::core_version ) << std::endl;
	about << std::endl;
	about << openmpt::string::get( openmpt::string::contact ) << std::endl;
	about << std::endl;
	about << "Show full credits?" << std::endl;
	if ( MessageBox( hwndParent, about.str().c_str(), SHORT_TITLE, MB_ICONINFORMATION | MB_YESNOCANCEL | MB_DEFBUTTON1 ) != IDYES ) {
		return;
	}
	std::ostringstream credits;
	credits << openmpt::string::get( openmpt::string::credits );
	MessageBox( hwndParent, credits.str().c_str(), SHORT_TITLE, MB_ICONINFORMATION );
}

static void init() {
	if ( !settings_dll ) {
		settings_dll = LoadLibrary( "libopenmpt_settings.dll" );
	}
	if ( !self ) {
		self = new self_winamp_t();
		inmod.FileExtensions = self->filetypes_string.data();
	}
}

static void quit() {
	if ( self ) {
		inmod.FileExtensions = NULL;
		delete self;
		self = 0;
	}
	if ( settings_dll ) {
		FreeLibrary( settings_dll );
		settings_dll = NULL;
	}
}

static int isourfile( char * fn ) {
	return 0;
}

static int play( char * fn ) {
	if ( !fn ) {
		return -1;
	}
	try {
		std::ifstream s( fn, std::ios::binary );
		self->mod = new openmpt::module( s );
		self->cached_filename = fn;
		self->cached_title = self->mod->get_metadata( "title" );
		self->cached_length = static_cast<int>( self->mod->get_duration_seconds() * 1000.0 );
		self->cached_infotext = generate_infotext( self->cached_filename, *self->mod );
		apply_options();
		self->samplerate = self->settings->samplerate;
		self->channels = self->settings->channels;
		int maxlatency = inmod.outMod->Open( self->samplerate, self->channels, BPS, -1, -1 );
		std::ostringstream str;
		str << maxlatency;
		inmod.SetInfo( self->mod->get_num_channels(), self->samplerate/1000, self->channels, 1 );
		inmod.SAVSAInit( maxlatency, self->samplerate );
		inmod.VSASetInfo( self->channels, self->samplerate );
		inmod.outMod->SetVolume( -666 );
		inmod.outMod->SetPan( 0 );
		self->paused = false;
		self->decode_position_frames = 0;
		self->PlayThread = CreateThread( NULL, 0, DecodeThread, NULL, 0, &self->PlayThreadID );
		return 0;
	} catch ( ... ) {
		if ( self->mod ) {
			delete self->mod;
			self->mod = 0;
		}
		return -1;
	}
}

static void pause() {
	self->paused = true;
	inmod.outMod->Pause( 1 );
}

static void unpause() {
	self->paused = false;
	inmod.outMod->Pause( 0 );
}

static int ispaused() {
	return self->paused ? 1 : 0;
}

static void stop() {
	PostThreadMessage( self->PlayThreadID, WM_QUIT, 0, 0 );
	WaitForSingleObject( self->PlayThread, INFINITE );
	CloseHandle( self->PlayThread );
	self->PlayThread = 0;
	self->PlayThreadID = 0;
	delete self->mod;
	self->mod = 0;
	inmod.outMod->Close();
	inmod.SAVSADeInit();
}

static int getlength() {
	return self->cached_length;
}

static int getoutputtime() {
	//return (int)( self->decode_position_frames * 1000 / self->mod->get_render_param( openmpt::module::RENDER_SAMPLERATE_HZ ) /* + ( inmod.outMod->GetOutputTime() - inmod.outMod->GetWrittenTime() ) */ );
	return inmod.outMod->GetOutputTime();
}

static void setoutputtime( int time_in_ms ) {
	PostThreadMessage( self->PlayThreadID, WM_OPENMPT_SEEK, 0, time_in_ms );
}

static void setvolume( int volume ) {
	inmod.outMod->SetVolume( volume );
}

static void setpan( int pan ) { 
	inmod.outMod->SetPan( pan );
}

static int infobox( char * fn, HWND hWndParent ) {
	if ( fn && fn[0] != '\0' && self->cached_filename != std::string(fn) ) {
		try {
			std::ifstream s( fn, std::ios::binary );
			openmpt::module mod( s );
			MessageBox( hWndParent, generate_infotext( fn, mod ).c_str(), SHORT_TITLE, 0 );
		} catch ( ... ) {
		}
	} else {
		MessageBox( hWndParent, self->cached_infotext.c_str(), SHORT_TITLE, 0 );
	}
	return 0;
}

static void getfileinfo( char * filename, char * title, int * length_in_ms ) {
	if ( !filename || *filename == '\0' ) {
		if ( length_in_ms ) {
			*length_in_ms = self->cached_length;
		}
		if ( title ) {
			strcpy( title, self->cached_title.c_str() );
		}
	} else {
		try {
			std::ifstream s( filename, std::ios::binary );
			openmpt::module mod( s );
			if ( length_in_ms ) {
				*length_in_ms = static_cast<int>( mod.get_duration_seconds() * 1000.0 );
			}
			if ( title ) {
				strcpy( title, mod.get_metadata("title").c_str() );
			}
		} catch ( ... ) {
		}
	}
}

static void eq_set( int on, char data[10], int preamp ) {
	return;
}

static DWORD WINAPI DecodeThread( LPVOID ) {
	MSG msg;
	PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );
	bool eof = false;
	while ( true ) {
		bool quit = false;
		while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
			if ( msg.message == WM_QUIT ) {
				quit = true;
			} else if ( msg.message == WM_OPENMPT_SEEK ) {
				double pos_seconds = self->mod->seek_seconds( msg.lParam * 0.001 );
				self->decode_position_frames = (std::int64_t)( pos_seconds * (double)self->samplerate);
				eof = false;
				inmod.outMod->Flush( (int)( pos_seconds * 1000.0 ) );
			}
		}
		if ( quit ) {
			break;
		}
		if ( eof ) {
			inmod.outMod->CanWrite(); // update output plugin state
			if ( !inmod.outMod->IsPlaying() ) {
				PostMessage( inmod.hMainWindow, WM_WA_MPEG_EOF, 0, 0 );
				return 0;
			}
			Sleep( 10 );
		} else {
			bool dsp_active = inmod.dsp_isactive() ? true : false;
			if ( inmod.outMod->CanWrite() >= (int)( WINAMP_BUFFER_SIZE_FRAMES * self->channels * sizeof( signed short ) ) * ( dsp_active ? WINAMP_DSP_HEADROOM_FACTOR : 1 ) ) {
				int frames = 0;
				switch ( self->channels ) {
				case 1:
					frames = self->mod->read( self->samplerate, WINAMP_BUFFER_SIZE_FRAMES, self->buffer.data()+0*WINAMP_BUFFER_SIZE_FRAMES );
					for ( int frame = 0; frame < frames; frame++ ) {
						self->interleaved_buffer[frame*1+0] = self->buffer[0*WINAMP_BUFFER_SIZE_FRAMES+frame];
					}
					break;
				case 2:
					frames = self->mod->read( self->samplerate, WINAMP_BUFFER_SIZE_FRAMES, self->buffer.data()+0*WINAMP_BUFFER_SIZE_FRAMES, self->buffer.data()+1*WINAMP_BUFFER_SIZE_FRAMES );
					for ( int frame = 0; frame < frames; frame++ ) {
						self->interleaved_buffer[frame*2+0] = self->buffer[0*WINAMP_BUFFER_SIZE_FRAMES+frame];
						self->interleaved_buffer[frame*2+1] = self->buffer[1*WINAMP_BUFFER_SIZE_FRAMES+frame];
					}
					break;
				case 4:
					frames = self->mod->read( self->samplerate, WINAMP_BUFFER_SIZE_FRAMES, self->buffer.data()+0*WINAMP_BUFFER_SIZE_FRAMES, self->buffer.data()+1*WINAMP_BUFFER_SIZE_FRAMES, self->buffer.data()+2*WINAMP_BUFFER_SIZE_FRAMES, self->buffer.data()+3*WINAMP_BUFFER_SIZE_FRAMES );
					for ( int frame = 0; frame < frames; frame++ ) {
						self->interleaved_buffer[frame*4+0] = self->buffer[0*WINAMP_BUFFER_SIZE_FRAMES+frame];
						self->interleaved_buffer[frame*4+1] = self->buffer[1*WINAMP_BUFFER_SIZE_FRAMES+frame];
						self->interleaved_buffer[frame*4+2] = self->buffer[2*WINAMP_BUFFER_SIZE_FRAMES+frame];
						self->interleaved_buffer[frame*4+3] = self->buffer[3*WINAMP_BUFFER_SIZE_FRAMES+frame];
					}
					break;
				}
				if ( frames == 0 ) {
					eof = true;
				} else {
					self->decode_position_frames += frames;
					std::int64_t decode_pos_ms = (self->decode_position_frames * 1000 / self->samplerate );
					inmod.SAAddPCMData( self->interleaved_buffer.data(), self->channels, BPS, (int)decode_pos_ms );
					inmod.VSAAddPCMData( self->interleaved_buffer.data(), self->channels, BPS, (int)decode_pos_ms );
					if ( dsp_active ) {
						int samples = frames * self->channels;
						samples = inmod.dsp_dosamples( self->interleaved_buffer.data(), samples, BPS, self->channels, self->samplerate );
						frames = samples / self->channels;
					}
					int bytes = frames * self->channels * sizeof( signed short );
					inmod.outMod->Write( (char*)self->interleaved_buffer.data(), bytes );
				}
			} else {
				Sleep( 10 );
			}
		}
	}
	return 0;
}

In_Module inmod = {
	IN_VER,
	openmpt::version::in_openmpt_string, // SHORT_TITLE,
	0, // hMainWindow
	0, // hDllInstance
	NULL, // filled later in Init() "mptm\0ModPlug Tracker Module (*.mptm)\0",
	1, // is_seekable
	1, // uses output
	config,
	about,
	init,
	quit,
	getfileinfo,
	infobox,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,
	getlength,
	getoutputtime,
	setoutputtime,
	setvolume,
	setpan,
	0,0,0,0,0,0,0,0,0, // vis
	0,0, // dsp
	eq_set,
	NULL, // setinfo
	0 // out_mod
};

extern "C" LIBOPENMPT_WINAMP_API In_Module * winampGetInModule2() {
	return &inmod;
}

#endif // NO_WINAMP
