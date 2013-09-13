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

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void (*changed_func)(void);

struct libopenmpt_settings {
	bool with_outputformat;
	int samplerate;
	int channels;
	int mastergain_millibel;
	int stereoseparation;
	int repeatcount;
	int interpolationfilterlength;
	int ramping;
	changed_func changed;
};

typedef void (*libopenmpt_settings_edit_func)( libopenmpt_settings * s, HWND parent, const char * title );

void libopenmpt_settings_edit( libopenmpt_settings * s, HWND parent, const char * title );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIBOPENMPT_SETTINGS_H
