/*
 * libopenmpt.h
 * ------------
 * Purpose: libopenmpt public c interface
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_H
#define LIBOPENMPT_H

#include "libopenmpt_config.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBOPENMPT_API int openmpt_is_compatible_version( uint32_t api_version );

LIBOPENMPT_API uint32_t openmpt_get_library_version(void);

LIBOPENMPT_API uint32_t openmpt_get_core_version(void);

#define OPENMPT_STRING_LIBRARY_VERSION "library_version"
#define OPENMPT_STRING_CORE_VERSION    "core_version"
#define OPENMPT_STRING_BUILD           "build"
#define OPENMPT_STRING_CREDITS         "credits"
#define OPENMPT_STRING_CONTACT         "contact"

LIBOPENMPT_API void openmpt_free_string( const char * str );

LIBOPENMPT_API const char * openmpt_get_string( const char * key );

LIBOPENMPT_API const char * openmpt_get_supported_extensions(void);

LIBOPENMPT_API int openmpt_is_extension_supported( const char * extension );

typedef size_t (*openmpt_stream_read_func)( void * stream, void * dst, size_t bytes );
typedef int (*openmpt_stream_seek_func)( void * stream, int64_t offset, int whence );
typedef int64_t (*openmpt_stream_tell_func)( void * stream );

typedef struct openmpt_stream_callbacks {
	openmpt_stream_read_func read;
	openmpt_stream_seek_func seek;
	openmpt_stream_tell_func tell;
} openmpt_stream_callbacks;

typedef void (*openmpt_log_func)( const char * message, void * user );

LIBOPENMPT_API void openmpt_log_func_default( const char * message, void * user );

LIBOPENMPT_API void openmpt_log_func_silent( const char * message, void * user );

LIBOPENMPT_API double openmpt_could_open_propability( openmpt_stream_callbacks stream_callbacks, void * stream, double effort, openmpt_log_func logfunc, void * user );

typedef struct openmpt_module openmpt_module;

LIBOPENMPT_API openmpt_module * openmpt_module_create( openmpt_stream_callbacks stream_callbacks, void * stream, openmpt_log_func logfunc, void * user );

LIBOPENMPT_API openmpt_module * openmpt_module_create_from_memory( const void * filedata, size_t filesize, openmpt_log_func logfunc, void * user );

LIBOPENMPT_API void openmpt_module_destroy( openmpt_module * mod );

#define OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL          1
#define OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT     2
#define OPENMPT_MODULE_RENDER_REPEATCOUNT                  3
#define OPENMPT_MODULE_RENDER_QUALITY_PERCENT              4
#define OPENMPT_MODULE_RENDER_MAXMIXCHANNELS               5
#define OPENMPT_MODULE_RENDER_INTERPOLATION_FILTER_LENGTH  6
#define OPENMPT_MODULE_RENDER_VOLUMERAMP_UP_MICROSECONDS   7
#define OPENMPT_MODULE_RENDER_VOLUMERAMP_DOWN_MICROSECONDS 8

#define OPENMPT_MODULE_COMMAND_NOTE         0
#define OPENMPT_MODULE_COMMAND_INSTRUMENT   1
#define OPENMPT_MODULE_COMMAND_VOLUMEEFFECT 2
#define OPENMPT_MODULE_COMMAND_EFFECT       3
#define OPENMPT_MODULE_COMMAND_VOLUME       4
#define OPENMPT_MODULE_COMMAND_PARAMETER    5

LIBOPENMPT_API int openmpt_module_get_render_param( openmpt_module * mod, int command, int32_t * value );
LIBOPENMPT_API int openmpt_module_set_render_param( openmpt_module * mod, int command, int32_t value );

LIBOPENMPT_API int openmpt_module_select_subsong( openmpt_module * mod, int32_t subsong );

LIBOPENMPT_API double openmpt_module_seek_seconds( openmpt_module * mod, double seconds );

LIBOPENMPT_API size_t openmpt_module_read_mono(   openmpt_module * mod, int32_t samplerate, size_t count, int16_t * mono );
LIBOPENMPT_API size_t openmpt_module_read_stereo( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * left, int16_t * right );
LIBOPENMPT_API size_t openmpt_module_read_quad(   openmpt_module * mod, int32_t samplerate, size_t count, int16_t * left, int16_t * right, int16_t * rear_left, int16_t * rear_right );
LIBOPENMPT_API size_t openmpt_module_read_float_mono(   openmpt_module * mod, int32_t samplerate, size_t count, float * mono );
LIBOPENMPT_API size_t openmpt_module_read_float_stereo( openmpt_module * mod, int32_t samplerate, size_t count, float * left, float * right );
LIBOPENMPT_API size_t openmpt_module_read_float_quad(   openmpt_module * mod, int32_t samplerate, size_t count, float * left, float * right, float * rear_left, float * rear_right );

LIBOPENMPT_API double openmpt_module_get_current_position_seconds( openmpt_module * mod );

LIBOPENMPT_API double openmpt_module_get_duration_seconds( openmpt_module * mod );

LIBOPENMPT_API const char * openmpt_module_get_metadata_keys( openmpt_module * mod );
LIBOPENMPT_API const char * openmpt_module_get_metadata( openmpt_module * mod, const char * key );

LIBOPENMPT_API int32_t openmpt_module_get_current_speed( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_current_tempo( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_current_order( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_current_pattern( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_current_row( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_current_playing_channels( openmpt_module * mod );

LIBOPENMPT_API int32_t openmpt_module_get_num_subsongs( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_num_channels( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_num_orders( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_num_patterns( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_num_instruments( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_num_samples( openmpt_module * mod );

LIBOPENMPT_API const char * openmpt_module_get_subsong_name( openmpt_module * mod, int32_t index );
LIBOPENMPT_API const char * openmpt_module_get_channel_name( openmpt_module * mod, int32_t index );
LIBOPENMPT_API const char * openmpt_module_get_order_name( openmpt_module * mod, int32_t index );
LIBOPENMPT_API const char * openmpt_module_get_pattern_name( openmpt_module * mod, int32_t index );
LIBOPENMPT_API const char * openmpt_module_get_instrument_name( openmpt_module * mod, int32_t index );
LIBOPENMPT_API const char * openmpt_module_get_sample_name( openmpt_module * mod, int32_t index );

LIBOPENMPT_API int32_t openmpt_module_get_order_pattern( openmpt_module * mod, int32_t order );
LIBOPENMPT_API int32_t openmpt_module_get_pattern_num_rows( openmpt_module * mod, int32_t pattern );

LIBOPENMPT_API uint8_t openmpt_module_get_pattern_row_channel_command( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, int command );

/* remember to add new functions to both C and C++ interfaces and to increase OPENMPT_API_VERSION_MINOR */

#ifdef __cplusplus
};
#endif

#endif /* LIBOPENMPT_H */
