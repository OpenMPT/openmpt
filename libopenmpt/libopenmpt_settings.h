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

namespace openmpt { namespace settings {

typedef void (*changed_func)();

struct settings {
	bool with_outputformat;
	int samplerate;
	int channels;
	int mastergain_millibel;
	int stereoseparation;
	int repeatcount;
	int interpolationfilterlength;
	int volrampinus;
	int volrampoutus;
	changed_func changed;
};

static void inline init( settings & s, bool with_outputformat = true ) {
	s.with_outputformat = with_outputformat;
	s.samplerate = 48000;
	s.channels = 2;
	s.mastergain_millibel = 0;
	s.stereoseparation = 100;
	s.repeatcount = 0;
	s.interpolationfilterlength = 8;
	s.volrampinus = 363;
	s.volrampoutus = 952;
	s.changed = 0;
}

LIBOPENMPT_SETTINGS_API void load( settings & s, const char * subkey );
LIBOPENMPT_SETTINGS_API void save( const settings & s, const char * subkey );
LIBOPENMPT_SETTINGS_API void edit( settings & s, HWND parent, const char * title );

} } // namespace openmpt::settings

#endif // LIBOPENMPT_SETTINGS_H
