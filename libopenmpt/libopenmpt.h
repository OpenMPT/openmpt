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

/*!
 * \page libopenmpt_c_overview C API
 *
 * \section error Error Handling
 *
 * - Functions with no return value in the corresponding C++ API return 0 on failure and 1 on success.
 * - Functions that return a string in the corresponding C++ API return a dynamically allocated const char *. I case of failure or memory allocation failure, a NULL pointer is returned.
 * - Functions that return integer values signal error condition by returning an invalid value (-1 in most cases, 0 in some cases).
 *
 * \section strings Strings
 *
 * - All strings returned from libopenmpt are encoded in UTF-8.
 * - All strings passed to libopenmpt should also be encoded in UTF-8.
 * Behaviour in case of invalid UTF-8 is unspecified.
 * - All strings returned from libopenmpt are dynamically allocated and must be
 * freed with openmpt_free_string(). Do NOT used C standard library free() for
 * libopenmpt strings as that would make your code invalid on windows when
 * dynamically linking against libopenmpt which itself statically links to the
 * C runtime.
 * - All strings passed to libopenmpt are copied. No ownership is assumed or
 * transferred.
 *
 * \section libopenmpt_c_detailed Detailed documentation
 *
 * \ref libopenmpt_c
 * 
 * The C API documentaion currently lacks behind the C++ API documentation.
 * In case a function is not documented here, you might want to look at the
 * \ref libopenmpt_cpp documentation. The C and C++ APIs are kept semantically
 * as close as possible.
 *
 * \section libopenmpt_c_examples Examples
 *
 * \subsection libopenmpt_c_example_file FILE*
 * \include libopenmpt_example_c.c
 * \subsection libopenmpt_c_example_inmemory in memory
 * \include libopenmpt_example_c_mem.c
 *
 */

/*! \defgroup libopenmpt_c libopenmpt C */

/*! \addtogroup libopenmpt_c
  @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Get the libopenmpt version number
 *
 * Returns the libopenmpt version number.
 * \return The value represents (major << 24 + minor << 16 + revision).
 */
LIBOPENMPT_API uint32_t openmpt_get_library_version(void);

/*! \brief Get the core version number
 *
 * Return the OpenMPT core version number.
 * \return The value represents (majormajor << 24 + major << 16 + minor << 8 + minorminor).
 */
LIBOPENMPT_API uint32_t openmpt_get_core_version(void);

/*! Return a verbose library version string from openmpt_string_get(). */
#define OPENMPT_STRING_LIBRARY_VERSION  "library_version"
/*! Return a verbose library features string from openmpt_string_get(). */
#define OPENMPT_STRING_LIBRARY_FEATURES "library_features"
/*! Return a verbose OpenMPT core version string from openmpt_string_get(). */
#define OPENMPT_STRING_CORE_VERSION     "core_version"
/*! Return information about the current build (e.g. the build date or compiler used) from openmpt_string_get(). */
#define OPENMPT_STRING_BUILD            "build"
/*! Return all contributors from openmpt_string_get(). */
#define OPENMPT_STRING_CREDITS          "credits"
/*! Return contact infromation about libopenmpt from openmpt_string_get(). */
#define OPENMPT_STRING_CONTACT         "contact"
/*! Return the libopenmpt license openmpt_string_get(). */
#define OPENMPT_STRING_LICENSE         "license"

/*! \brief Free a string returned by libopenmpt
 *
 * Frees any string that got returned by libopenmpt.
 */
LIBOPENMPT_API void openmpt_free_string( const char * str );

/*! \brief Get library related metadata.
 *
 * \param key Key to query.
 * \return A (possibly multi-line) string containing the queried information. If no information is available, the string is empty.
 * \sa OPENMPT_STRING_LIBRARY_VERSION
 * \sa OPENMPT_STRING_CORE_VERSION
 * \sa OPENMPT_STRING_BUILD
 * \sa OPENMPT_STRING_CREDITS
 * \sa OPENMPT_STRING_CONTACT
 * \sa OPENMPT_STRING_LICENSE
 */
LIBOPENMPT_API const char * openmpt_get_string( const char * key );

/*! \brief Get a list of supported file extensions
 *
 * \return The semicolon-separated list of extensions supported by this libopenmpt build. The extensions are returned lower-case without a leading dot.
 */
LIBOPENMPT_API const char * openmpt_get_supported_extensions(void);

/*! \brief Query whether a file extension is supported
 *
 * \param extension file extension to query without a leading dot. The case is ignored.
 * \return 1 if the extension is supported by libopenmpt, 0 otherwise.
 */
LIBOPENMPT_API int openmpt_is_extension_supported( const char * extension );

/*! \brief Read bytes from stream
 *
 * Read byte data from stream to dst.
 * \param stream Stream to read data from
 * \param dst Target where to copy data.
 * \param bytes Number of bytes to read.
 * \return Number of bytes actually read and written to dst.
 * \retval 0 End fo stream or error.
 */
typedef size_t (*openmpt_stream_read_func)( void * stream, void * dst, size_t bytes );

/*! \brief Seek stream position
 *
 * Seek to stream position offset at whence.
 * \param stream Stream to operate on.
 * \param offset Offset to seek to.
 * \param whence SEEK_SET, SEEK_CUR, SEEK_END. See C89 documentation.
 * \return Returns 0 on success.
 * \retval 0 Success.
 * \retval -1 Failure. Position gets not updated.
 */
typedef int (*openmpt_stream_seek_func)( void * stream, int64_t offset, int whence );

/*! \brief Tell stream position
 *
 * Tell position of stream.
 * \param stream Stream to operate on.
 * \return Current position in stream.
 * \retval -1 Failure. Position gets not updated.
 */
typedef int64_t (*openmpt_stream_tell_func)( void * stream );

/*! \brief Stream callbacks
 *
 * Stream callbacks used by libopenmpt for stream operations.
 */
typedef struct openmpt_stream_callbacks {

	/*! \brief Read callback.
	 *
	 * \sa openmpt_stream_read_func
	 */
	openmpt_stream_read_func read;

	/*! \brief Seek callback.
	 *
	 * Seek callback can be NULL if seeking is not supported.
	 * \sa openmpt_stream_seek_func
	 */
	openmpt_stream_seek_func seek;

	/*! \brief Tell callback.
	 *
	 * Tell callback can be NULL if seeking is not supported.
	 * \sa openmpt_stream_tell_func
	 */
	openmpt_stream_tell_func tell;

} openmpt_stream_callbacks;

/*! \brief Logging function
 *
 * \param message UTF-8 encoded log message.
 * \param user User context that was passed to openmpt_module_create(), openmpt_module_create_from_memory() or openmpt_could_open_propability().
 */
typedef void (*openmpt_log_func)( const char * message, void * user );

/*! \brief Default logging function
 *
 * Default logging function that logs anything to stderr.
 */
LIBOPENMPT_API void openmpt_log_func_default( const char * message, void * user );

/*! \brief Silent logging function
 *
 * Silent logging function that throws any log message away.
 */
LIBOPENMPT_API void openmpt_log_func_silent( const char * message, void * user );

/*! \brief Roughly scan the input stream to find out whether libopenmpt might be able to open it
 *
 * \param stream_callbacks Input stream callback operations.
 * \param stream Input stream to scan.
 * \param effort Effort to make when validating stream. Effort 0.0 does not even look at stream at all and effort 1.0 completely loads the file from stream. A lower effort requires less dat to be loaded but only gives a rough estimate answer.
 * \param logfunc Logging function where warning and errors are written.
 * \param user Logging function user context.
 * \return Propability between 0.0 and 1.0.
 */
LIBOPENMPT_API double openmpt_could_open_propability( openmpt_stream_callbacks stream_callbacks, void * stream, double effort, openmpt_log_func logfunc, void * user );

/*! \brief Opaque type representing a libopenmpt module
 */
typedef struct openmpt_module openmpt_module;

typedef struct openmpt_module_initial_ctl {
	const char * ctl;
	const char * value;
} openmpt_module_initial_ctl;

LIBOPENMPT_API openmpt_module * openmpt_module_create( openmpt_stream_callbacks stream_callbacks, void * stream, openmpt_log_func logfunc, void * user, const openmpt_module_initial_ctl * ctls );

LIBOPENMPT_API openmpt_module * openmpt_module_create_from_memory( const void * filedata, size_t filesize, openmpt_log_func logfunc, void * user, const openmpt_module_initial_ctl * ctls );

LIBOPENMPT_API void openmpt_module_destroy( openmpt_module * mod );

#define OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL        1
#define OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT   2
#define OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH 3
#define OPENMPT_MODULE_RENDER_VOLUMERAMPING_STRENGTH     4

#define OPENMPT_MODULE_COMMAND_NOTE         0
#define OPENMPT_MODULE_COMMAND_INSTRUMENT   1
#define OPENMPT_MODULE_COMMAND_VOLUMEEFFECT 2
#define OPENMPT_MODULE_COMMAND_EFFECT       3
#define OPENMPT_MODULE_COMMAND_VOLUME       4
#define OPENMPT_MODULE_COMMAND_PARAMETER    5

LIBOPENMPT_API int openmpt_module_select_subsong( openmpt_module * mod, int32_t subsong );
LIBOPENMPT_API int openmpt_module_set_repeat_count( openmpt_module * mod, int32_t repeat_count );
LIBOPENMPT_API int32_t openmpt_module_get_repeat_count( openmpt_module * mod );

LIBOPENMPT_API double openmpt_module_get_duration_seconds( openmpt_module * mod );

LIBOPENMPT_API double openmpt_module_set_position_seconds( openmpt_module * mod, double seconds );
LIBOPENMPT_API double openmpt_module_get_position_seconds( openmpt_module * mod );

LIBOPENMPT_API double openmpt_module_set_position_order_row( openmpt_module * mod, int32_t order, int32_t row );

LIBOPENMPT_API int openmpt_module_get_render_param( openmpt_module * mod, int param, int32_t * value );
LIBOPENMPT_API int openmpt_module_set_render_param( openmpt_module * mod, int param, int32_t value );

LIBOPENMPT_API size_t openmpt_module_read_mono(   openmpt_module * mod, int32_t samplerate, size_t count, int16_t * mono );
LIBOPENMPT_API size_t openmpt_module_read_stereo( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * left, int16_t * right );
LIBOPENMPT_API size_t openmpt_module_read_quad(   openmpt_module * mod, int32_t samplerate, size_t count, int16_t * left, int16_t * right, int16_t * rear_left, int16_t * rear_right );
LIBOPENMPT_API size_t openmpt_module_read_float_mono(   openmpt_module * mod, int32_t samplerate, size_t count, float * mono );
LIBOPENMPT_API size_t openmpt_module_read_float_stereo( openmpt_module * mod, int32_t samplerate, size_t count, float * left, float * right );
LIBOPENMPT_API size_t openmpt_module_read_float_quad(   openmpt_module * mod, int32_t samplerate, size_t count, float * left, float * right, float * rear_left, float * rear_right );
LIBOPENMPT_API size_t openmpt_module_read_interleaved_stereo( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * interleaved_stereo );
LIBOPENMPT_API size_t openmpt_module_read_interleaved_quad(   openmpt_module * mod, int32_t samplerate, size_t count, int16_t * interleaved_quad   );
LIBOPENMPT_API size_t openmpt_module_read_interleaved_float_stereo( openmpt_module * mod, int32_t samplerate, size_t count, float * interleaved_stereo );
LIBOPENMPT_API size_t openmpt_module_read_interleaved_float_quad(   openmpt_module * mod, int32_t samplerate, size_t count, float * interleaved_quad   );

LIBOPENMPT_API const char * openmpt_module_get_metadata_keys( openmpt_module * mod );
LIBOPENMPT_API const char * openmpt_module_get_metadata( openmpt_module * mod, const char * key );

LIBOPENMPT_API int32_t openmpt_module_get_current_speed( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_current_tempo( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_current_order( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_current_pattern( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_current_row( openmpt_module * mod );
LIBOPENMPT_API int32_t openmpt_module_get_current_playing_channels( openmpt_module * mod );

LIBOPENMPT_API float openmpt_module_get_current_channel_vu_mono( openmpt_module * mod, int32_t channel );
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_left( openmpt_module * mod, int32_t channel );
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_right( openmpt_module * mod, int32_t channel );
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_rear_left( openmpt_module * mod, int32_t channel );
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_rear_right( openmpt_module * mod, int32_t channel );

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

LIBOPENMPT_API const char * openmpt_module_format_pattern_row_channel_command( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, int command );
LIBOPENMPT_API const char * openmpt_module_highlight_pattern_row_channel_command( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, int command );

LIBOPENMPT_API const char * openmpt_module_format_pattern_row_channel( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, size_t width, int pad );
LIBOPENMPT_API const char * openmpt_module_highlight_pattern_row_channel( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, size_t width, int pad );

LIBOPENMPT_API const char * openmpt_module_get_ctls( openmpt_module * mod );
LIBOPENMPT_API const char * openmpt_module_ctl_get( openmpt_module * mod, const char * ctl );
LIBOPENMPT_API int openmpt_module_ctl_set( openmpt_module * mod, const char * ctl, const char * value );

/* remember to add new functions to both C and C++ interfaces and to increase OPENMPT_API_VERSION_MINOR */

#ifdef __cplusplus
}
#endif

/*!
  @}
*/

#endif /* LIBOPENMPT_H */
