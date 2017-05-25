/*
 * libopenmpt_ext.h
 * ----------------
 * Purpose: libopenmpt public c interface for libopenmpt extensions
 * Notes  :
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_EXT_H
#define LIBOPENMPT_EXT_H

#include "libopenmpt_config.h"
#include "libopenmpt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct openmpt_module_ext openmpt_module_ext;

LIBOPENMPT_API openmpt_module_ext * openmpt_module_ext_create( openmpt_stream_callbacks stream_callbacks, void * stream, openmpt_log_func logfunc, void * loguser, openmpt_error_func errfunc, void * erruser, int * error, const char * * error_message, const openmpt_module_initial_ctl * ctls );

LIBOPENMPT_API openmpt_module_ext * openmpt_module_ext_create_from_memory( const void * filedata, size_t filesize, openmpt_log_func logfunc, void * loguser, openmpt_error_func errfunc, void * erruser, int * error, const char * * error_message, const openmpt_module_initial_ctl * ctls );

LIBOPENMPT_API void openmpt_module_ext_destroy( openmpt_module_ext * mod_ext );

LIBOPENMPT_API openmpt_module * openmpt_module_ext_get_module( openmpt_module_ext * mod_ext );

LIBOPENMPT_API int openmpt_module_ext_get_interface( openmpt_module_ext * mod_ext, const char * interface_id, void * interface, size_t interface_size );



#ifndef LIBOPENMPT_EXT_C_INTERFACE_PATTERN_VIS
#define LIBOPENMPT_EXT_C_INTERFACE_PATTERN_VIS "pattern_vis"
#endif

#define OPENMPT_MODULE_EXT_INTERFACE_PATTERN_VIS_EFFECT_TYPE_UNKNOWN 0
#define OPENMPT_MODULE_EXT_INTERFACE_PATTERN_VIS_EFFECT_TYPE_GENERAL 1
#define OPENMPT_MODULE_EXT_INTERFACE_PATTERN_VIS_EFFECT_TYPE_GLOBAL  2
#define OPENMPT_MODULE_EXT_INTERFACE_PATTERN_VIS_EFFECT_TYPE_VOLUME  3
#define OPENMPT_MODULE_EXT_INTERFACE_PATTERN_VIS_EFFECT_TYPE_PANNING 4
#define OPENMPT_MODULE_EXT_INTERFACE_PATTERN_VIS_EFFECT_TYPE_PITCH   5

typedef struct openmpt_module_ext_interface_pattern_vis {
	int ( * get_pattern_row_channel_volume_effect_type ) ( openmpt_module_ext * mod_ext, int32_t pattern, int32_t row, int32_t channel );
	int ( * get_pattern_row_channel_effect_type ) ( openmpt_module_ext * mod_ext, int32_t pattern, int32_t row, int32_t channel );
} openmpt_module_ext_interface_pattern_vis;



#ifndef LIBOPENMPT_EXT_C_INTERFACE_INTERACTIVE
#define LIBOPENMPT_EXT_C_INTERFACE_INTERACTIVE "interactive"
#endif

typedef struct openmpt_module_ext_interface_interactive {
	int ( * set_current_speed ) ( openmpt_module_ext * mod_ext, int32_t speed );
	int ( * set_current_tempo ) ( openmpt_module_ext * mod_ext, int32_t tempo );
	int ( * set_tempo_factor ) ( openmpt_module_ext * mod_ext, double factor );
	double ( * get_tempo_factor ) ( openmpt_module_ext * mod_ext );
	int ( * set_pitch_factor ) ( openmpt_module_ext * mod_ext, double factor );
	double ( * get_pitch_factor ) ( openmpt_module_ext * mod_ext );
	int ( * set_global_volume ) ( openmpt_module_ext * mod_ext, double volume );
	double ( * get_global_volume ) ( openmpt_module_ext * mod_ext );
	int ( * set_channel_volume ) ( openmpt_module_ext * mod_ext, int32_t channel, double volume );
	double ( * get_channel_volume ) ( openmpt_module_ext * mod_ext, int32_t channel );
	int ( * set_channel_mute_status ) ( openmpt_module_ext * mod_ext, int32_t channel, int mute );
	int ( * get_channel_mute_status ) ( openmpt_module_ext * mod_ext, int32_t channel );
	int ( * set_instrument_mute_status ) ( openmpt_module_ext * mod_ext, int32_t instrument, int mute );
	int ( * get_instrument_mute_status ) ( openmpt_module_ext * mod_ext, int32_t instrument );
	int32_t ( * play_note ) ( openmpt_module_ext * mod_ext, int32_t instrument, int32_t note, double volume, double panning );
	int ( * stop_note ) ( openmpt_module_ext * mod_ext, int32_t channel );
} openmpt_module_ext_interface_interactive;



/* add stuff here */



#ifdef __cplusplus
}
#endif

#endif /* LIBOPENMPT_EXT_H */

