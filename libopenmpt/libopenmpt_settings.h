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
#endif /* __cplusplus */

typedef void (*changed_func)(void);

struct libopenmpt_settings {
	bool no_default_format;
	int samplerate;
	int channels;
	int mastergain_millibel;
	int stereoseparation;
	int repeatcount;
	int interpolationfilterlength;
	int ramping;
	int vis_allow_scroll;
	changed_func changed;
};

typedef void (*libopenmpt_settings_edit_func)( libopenmpt_settings * s, HWND parent, const char * title );

void libopenmpt_settings_edit( libopenmpt_settings * s, HWND parent, const char * title );

#ifdef __cplusplus
}
#endif /* __cplusplus */



#define LIBOPENMPT_SETTINGS_DECLARE() \
	static HMODULE settings_dll = NULL;

#define LIBOPENMPT_SETTINGS_IS_AVAILABLE() \
	settings_dll

#define LIBOPENMPT_SETTINGS_EDIT( settings, parent, title ) \
	do { \
		if ( (libopenmpt_settings_edit_func)GetProcAddress( settings_dll , "libopenmpt_settings_edit" ) ) { \
			((libopenmpt_settings_edit_func)GetProcAddress( settings_dll, "libopenmpt_settings_edit" ))( settings , parent , title ); \
		} \
	} while(0)

#define LIBOPENMPT_SETTINGS_UNAVAILABLE( parent, dll, title ) \
	MessageBox( parent , TEXT("libopenmpt_settings.dll failed to load. Please check if it is in the same folder as ") dll TEXT("."), title , MB_ICONERROR )

#define LIBOPENMPT_SETTINGS_LOAD() \
	do { \
		if ( !settings_dll ) { \
			settings_dll = LoadLibrary( TEXT("libopenmpt_settings.dll") ); \
		} \
		if ( !settings_dll ) { \
			settings_dll = LoadLibrary( TEXT("Plugins\\libopenmpt_settings.dll") ); \
		} \
	} while(0)

#define LIBOPENMPT_SETTINGS_UNLOAD() \
	do { \
		if ( settings_dll ) { \
			FreeLibrary( settings_dll ); \
			settings_dll = NULL; \
		} \
	} while(0)



#endif /* LIBOPENMPT_SETTINGS_H */
