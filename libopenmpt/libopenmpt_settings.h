/*
 * libopenmpt_settings.h
 * ---------------------
 * Purpose: libopenmpt plugin settings
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_SETTINGS_H
#define LIBOPENMPT_SETTINGS_H

#define LIBOPENMPT_ALPHA_WARNING_SEEN_AND_I_KNOW_WHAT_I_AM_DOING
#include "libopenmpt.h"

#if defined(LIBOPENMPT_BUILD_SETTINGS_DLL)
#define LIBOPENMPT_SETTINGS_API __declspec(dllexport)
#elif defined(LIBOPENMPT_USE_SETTINGS_DLL)
#define LIBOPENMPT_SETTINGS_API __declspec(dllimport)
#ifndef _DEBUG
#pragma comment ( lib, "bin\\libopenmpt_settings.lib" )
#else
#pragma comment ( lib, "Debug\\libopenmpt_settings.lib" )
#endif
#endif

#include <windows.h>

namespace openmpt {

#ifdef _MSC_VER
#pragma warning ( disable : 4275 ) // warning C4275: non dll-interface class 'foo' used as base for dll-interface class 'bar'
#endif

class settings {
public:
	bool with_outputformat;
	int samplerate;
	int channels;
	int mastergain;
	int stereoseparation;
	int repeatcount;
	int maxmixchannels;
	int interpolationmode;
	int volrampin;
	int volrampout;
	void load();
	void save();
	void edit( HWND parent, const char * title );
	settings( bool with_outputformat_ = true );
	virtual ~settings();
	virtual void changed();
	virtual void read_setting( const char * key, int & val );
	virtual void write_setting( const char * key, int val );
	virtual void edit_settings( HWND parent, const char * title );
};

inline void settings::load() {
	read_setting( "samplerate", samplerate );
	read_setting( "channels", channels );
	read_setting( "mastergain", mastergain );
	read_setting( "stereoseparation", stereoseparation );
	read_setting( "repeatcount", repeatcount );
	read_setting( "maxmixchannels", maxmixchannels );
	read_setting( "interpolationmode", interpolationmode );
	read_setting( "volrampin", volrampin );
	read_setting( "volrampout", volrampout );
}

inline void settings::save() {
	write_setting( "samplerate", samplerate );
	write_setting( "channels", channels );
	write_setting( "mastergain", mastergain );
	write_setting( "stereoseparation", stereoseparation );
	write_setting( "repeatcount", repeatcount );
	write_setting( "maxmixchannels", maxmixchannels );
	write_setting( "interpolationmode", interpolationmode );
	write_setting( "volrampin", volrampin );
	write_setting( "volrampout", volrampout );
}

inline void settings::edit( HWND parent, const char * title ) {
	edit_settings( parent, title );
}

inline settings::settings( bool with_outputformat_ ) : with_outputformat(with_outputformat_) {
	samplerate = 48000;
	channels = 2;
	mastergain = 0;
	stereoseparation = 100;
	repeatcount = 0;
	maxmixchannels = 256;
	interpolationmode = OPENMPT_MODULE_RENDER_INTERPOLATION_FIR_KAISER4T;
	volrampin = 16;
	volrampout = 42;
}

inline settings::~settings() {
	return;
}

inline void settings::changed() {
	return;
}

inline void settings::read_setting( const char * key, int & val ) {
	return;
}

inline void settings::write_setting( const char * key, int val ) {
	return;
}

inline void settings::edit_settings( HWND parent, const char * title ) {
	return;
}

class LIBOPENMPT_SETTINGS_API registry_settings : public settings {
private:
	const char * subkey;
public:
	registry_settings( const char * subkey_ );
	virtual ~registry_settings();
	virtual void read_setting( const char * key, int & val );
	virtual void write_setting( const char * key, int val );
	virtual void edit_settings( HWND parent, const char * title );
};

#ifdef _MSC_VER
#pragma warning ( default : 4275 ) // warning C4275: non dll-interface class 'foo' used as base for dll-interface class 'bar'
#endif

} // namespace openmpt

#endif // LIBOPENMPT_SETTINGS_H
