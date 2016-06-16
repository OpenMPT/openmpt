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
 * \section libopenmpt_c_error Error Handling
 *
 * - Functions with no return value in the corresponding C++ API return 0 on
 * failure and 1 on success.
 * - Functions that return a string in the corresponding C++ API return a
 * dynamically allocated const char *. I case of failure or memory allocation
 * failure, a NULL pointer is returned.
 * - Functions that return integer values signal error condition by returning
 * an invalid value (-1 in most cases, 0 in some cases).
 *
 * \section libopenmpt_c_strings Strings
 *
 * - All strings returned from libopenmpt are encoded in UTF-8.
 * - All strings passed to libopenmpt should also be encoded in UTF-8.
 * Behaviour in case of invalid UTF-8 is unspecified.
 * - libopenmpt does not enforce or expect any particular unicode
 * normalization form.
 * - All strings returned from libopenmpt are dynamically allocated and must
 * be freed with openmpt_free_string(). Do NOT use the C standard library
 * free() for libopenmpt strings as that would make your code invalid on
 * windows when dynamically linking against libopenmpt which itself statically
 * links to the C runtime.
 * - All strings passed to libopenmpt are copied. No ownership is assumed or
 * transferred.
 *
 * \section libopenmpt_c_fileio File I/O
 *
 * libopenmpt can use 3 different strategies for file I/O.
 *
 * - openmpt_module_create_from_memory() will load the module from the provided
 * memory buffer, which will require loading all data upfront by the library
 * caller.
 * - openmpt_module_create() with a seekable stream will load the module via
 * callbacks to the stream interface. libopenmpt will not implement an
 * additional buffering layer in this case whih means the callbacks are assumed
 * to be performant even with small i/o sizes.
 * - openmpt_module_create() with an unseekable stream will load the module via
 * callbacks to the stream interface. libopempt will make an internal copy as
 * it goes along, and sometimes have to pre-cache the whole file in case it
 * needs to know the complete file size. This strategy is intended to be used
 * if the file is located on a high latency network.
 *
 * | create function                                | speed  | memory consumption |
 * | ---------------------------------------------: | :----: | :----------------: |
 * | openmpt_module_create_from_memory()            | <p style="background-color:green" >fast  </p> | <p style="background-color:yellow">medium</p> | 
 * | openmpt_module_create() with seekable stream   | <p style="background-color:red"   >slow  </p> | <p style="background-color:green" >low   </p> |
 * | openmpt_module_create() with unseekable stream | <p style="background-color:yellow">medium</p> | <p style="background-color:red"   >high  </p> |
 *
 * In all cases, the data or stream passed to the create function is no longer
 * needed after the the openmpt_module has been created and can be freed by the
 * caller.
 *
 * \section libopenmpt_c_detailed Detailed documentation
 *
 * \ref libopenmpt_c
 * 
 * In case a function is not documented here, you might want to look at the
 * \ref libopenmpt_cpp documentation. The C and C++ APIs are kept semantically
 * as close as possible.
 *
 * \section libopenmpt_c_examples Examples
 *
 * \subsection libopenmpt_c_example_unsafe Unsafe, simplified example without any error checking to get a first idea of the API
 * \include libopenmpt_example_c_unsafe.c
 * \subsection libopenmpt_c_example_file FILE*
 * \include libopenmpt_example_c.c
 * \subsection libopenmpt_c_example_inmemory in memory
 * \include libopenmpt_example_c_mem.c
 * \subsection libopenmpt_c_example_stdout reading FILE* and writing PCM data to STDOUT (usable without PortAudio)
 * \include libopenmpt_example_c_stdout.c
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

/*! Return a verbose library version string from openmpt_get_string(). \deprecated Please use \code "library_version" \endcode directly. */
#define OPENMPT_STRING_LIBRARY_VERSION  LIBOPENMPT_DEPRECATED_STRING( "library_version" )
/*! Return a verbose library features string from openmpt_get_string(). \deprecated Please use \code "library_features" \endcode directly. */
#define OPENMPT_STRING_LIBRARY_FEATURES LIBOPENMPT_DEPRECATED_STRING( "library_features" )
/*! Return a verbose OpenMPT core version string from openmpt_get_string(). \deprecated Please use \code "core_version" \endcode directly. */
#define OPENMPT_STRING_CORE_VERSION     LIBOPENMPT_DEPRECATED_STRING( "core_version" )
/*! Return information about the current build (e.g. the build date or compiler used) from openmpt_get_string(). \deprecated Please use \code "build" \endcode directly. */
#define OPENMPT_STRING_BUILD            LIBOPENMPT_DEPRECATED_STRING( "build" )
/*! Return all contributors from openmpt_get_string(). \deprecated Please use \code "credits" \endcode directly. */
#define OPENMPT_STRING_CREDITS          LIBOPENMPT_DEPRECATED_STRING( "credits" )
/*! Return contact infromation about libopenmpt from openmpt_get_string(). \deprecated Please use \code "contact" \endcode directly. */
#define OPENMPT_STRING_CONTACT          LIBOPENMPT_DEPRECATED_STRING( "contact" )
/*! Return the libopenmpt license from openmpt_get_string(). \deprecated Please use \code "license" \endcode directly. */
#define OPENMPT_STRING_LICENSE          LIBOPENMPT_DEPRECATED_STRING( "license" )

/*! \brief Free a string returned by libopenmpt
 *
 * Frees any string that got returned by libopenmpt.
 */
LIBOPENMPT_API void openmpt_free_string( const char * str );

/*! \brief Get library related metadata.
 *
 * \param key Key to query.
 *       Possible keys are:
 *        -  "library_version": verbose library version string
 *        -  "library_features": verbose library features string
 *        -  "core_version": verboseOpenMPT core version string
 *        -  "source_url": original source code URL
 *        -  "source_date": original source code date
 *        -  "build": information about the current build (e.g. the build date or compiler used)
 *        -  "build_compiler": information about the compiler used to build libopenmpt
 *        -  "credits": all contributors
 *        -  "contact": contact infromation about libopenmpt
 *        -  "license": the libopenmpt license
 *        -  "url": libopenmpt website URL
 *        -  "support_forum_url": libopenmpt support and discussions forum URL
 *        -  "bugtracker_url": libopenmpt bug and issue tracker URL
 * \return A (possibly multi-line) string containing the queried information. If no information is available, the string is empty.
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

#define OPENMPT_STREAM_SEEK_SET 0
#define OPENMPT_STREAM_SEEK_CUR 1
#define OPENMPT_STREAM_SEEK_END 2

/*! \brief Read bytes from stream
 *
 * Read bytes data from stream to dst.
 * \param stream Stream to read data from
 * \param dst Target where to copy data.
 * \param bytes Number of bytes to read.
 * \return Number of bytes actually read and written to dst.
 * \retval 0 End of stream or error.
 */
typedef size_t (*openmpt_stream_read_func)( void * stream, void * dst, size_t bytes );

/*! \brief Seek stream position
 *
 * Seek to stream position offset at whence.
 * \param stream Stream to operate on.
 * \param offset Offset to seek to.
 * \param whence OPENMPT_STREAM_SEEK_SET, OPENMPT_STREAM_SEEK_CUR, OPENMPT_STREAM_SEEK_END. See C89 documentation.
 * \return Returns 0 on success.
 * \retval 0 Success.
 * \retval -1 Failure. Position does not get updated.
 */
typedef int (*openmpt_stream_seek_func)( void * stream, int64_t offset, int whence );

/*! \brief Tell stream position
 *
 * Tell position of stream.
 * \param stream Stream to operate on.
 * \return Current position in stream.
 * \retval -1 Failure.
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
 * \param effort Effort to make when validating stream. Effort 0.0 does not even look at stream at all and effort 1.0 completely loads the file from stream. A lower effort requires less data to be loaded but only gives a rough estimate answer. Use an effort of 0.25 to only verify the header data of the module file.
 * \param logfunc Logging function where warning and errors are written.
 * \param user Logging function user context.
 * \return Probability between 0.0 and 1.0.
 * \sa openmpt_stream_callbacks
 */
LIBOPENMPT_API double openmpt_could_open_propability( openmpt_stream_callbacks stream_callbacks, void * stream, double effort, openmpt_log_func logfunc, void * user );

/*! \brief Opaque type representing a libopenmpt module
 */
typedef struct openmpt_module openmpt_module;

typedef struct openmpt_module_initial_ctl {
	const char * ctl;
	const char * value;
} openmpt_module_initial_ctl;

/*! \brief Construct an openmpt_module
 *
 * \param stream_callbacks Input stream callback operations.
 * \param stream Input stream to load the module from.
 * \param logfunc Log where any warnings or errors are printed to. The lifetime of the reference has to be as long as the lifetime of the module instance.
 * \param user User-defined data associated with this module. This value will be passed to the logging callback function (logfunc) 
 * \param ctls A map of initial ctl values, see openmpt_module_get_ctls.
 * \return A pointer to the constructed openmpt_module, or NULL on failure.
 * \remarks The input data can be discarded after an openmpt_module has been constructed succesfully.
 * \sa openmpt_stream_callbacks
 * \sa \ref libopenmpt_c_fileio
 */
LIBOPENMPT_API openmpt_module * openmpt_module_create( openmpt_stream_callbacks stream_callbacks, void * stream, openmpt_log_func logfunc, void * user, const openmpt_module_initial_ctl * ctls );

/*! \brief Construct an openmpt_module
 *
 * \param filedata Data to load the module from.
 * \param filesize Amount of data available.
 * \param logfunc Log where any warnings or errors are printed to. The lifetime of the reference has to be as long as the lifetime of the module instance.
 * \param user User-defined data associated with this module. This value will be passed to the logging callback function (logfunc) 
 * \param ctls A map of initial ctl values, see openmpt_module_get_ctls.
 * \return A pointer to the constructed openmpt_module, or NULL on failure.
 * \remarks The input data can be discarded after an openmpt_module has been constructed succesfully.
 * \sa \ref libopenmpt_c_fileio
 */
LIBOPENMPT_API openmpt_module * openmpt_module_create_from_memory( const void * filedata, size_t filesize, openmpt_log_func logfunc, void * user, const openmpt_module_initial_ctl * ctls );

/*! \brief Unload a previously created openmpt_module from memory.
 *
 * \param mod The module to unload.
 */
LIBOPENMPT_API void openmpt_module_destroy( openmpt_module * mod );

/*! \brief Master Gain
 *
 * The related value represents a relative gain in milliBel.\n
 * The default value is 0.\n
 * The supported value range is unlimited.\n
 */
#define OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL        1
/*! \brief Stereo Separation
 *
 * The related value represents the stereo separation generated by the libopenmpt mixer in percent.\n
 * The default value is 100.\n
 * The supported value range is [0,200].\n
 */
#define OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT   2
/*! \brief Interpolation Filter
 *
 * The related value represents the interpolation filter length used by the libopenmpt mixer.\n
 * The default value is 0, which indicates a recommended default value.\n
 * The supported value range is [0,inf). Values greater than the implementation limit are clamped to the maximum supported value.\n
 * Currently supported values:
 *  - 0: internal default
 *  - 1: no interpolation (zero order hold)
 *  - 2: linear interpolation
 *  - 4: cubic interpolation
 *  - 8: windowed sinc with 8 taps
 */
#define OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH 3
/*! \brief Volume Ramping Strength
 *
 * The related value represents the amount of volume ramping done by the libopenmpt mixer.\n
 * The default value is -1, which indicates a recommended default value.\n
 * The meaningful value range is [-1..10].\n
 * A value of 0 completely disables volume ramping. This might cause clicks in sound output.\n
 * Higher values imply slower/softer volume ramps.
 */
#define OPENMPT_MODULE_RENDER_VOLUMERAMPING_STRENGTH     4

/**
 * \defgroup openmpt_module_command_index Pattern cell indices
 *
 * \brief Parameter index to use with openmpt_module_get_pattern_row_channel_command, openmpt_module_format_pattern_row_channel_command and openmpt_module_highlight_pattern_row_channel_command
 * @{
 */
#define OPENMPT_MODULE_COMMAND_NOTE         0
#define OPENMPT_MODULE_COMMAND_INSTRUMENT   1
#define OPENMPT_MODULE_COMMAND_VOLUMEEFFECT 2
#define OPENMPT_MODULE_COMMAND_EFFECT       3
#define OPENMPT_MODULE_COMMAND_VOLUME       4
#define OPENMPT_MODULE_COMMAND_PARAMETER    5
/** @}*/

/*! \brief Select a subsong from a multi-song module
 *
 * \param mod The module handle to work on.
 * \param subsong Index of the subsong. -1 plays all subsongs consecutively.
 * \return 1 on success, 0 on failure.
 * \sa openmpt_module_get_num_subsongs, openmpt_module_get_subsong_names
 */
LIBOPENMPT_API int openmpt_module_select_subsong( openmpt_module * mod, int32_t subsong );
/*! \brief Set Repeat Count
 *
 * \param mod The module handle to work on.
 * \param repeat_count Repeat Count
 *   - -1: repeat forever
 *   - 0: play once, repeat zero times (the default)
 *   - n>0: play once and repeat n times after that
 * \return 1 on success, 0 on failure.
 * \sa openmpt_module_get_repeat_count
 */
LIBOPENMPT_API int openmpt_module_set_repeat_count( openmpt_module * mod, int32_t repeat_count );
/*! \brief Get Repeat Count
 *
 * \param mod The module handle to work on.
 * \return Repeat Count
 *   - -1: repeat forever
 *   - 0: play once, repeat zero times (the default)
 *   - n>0: play once and repeat n times after that
 * \sa openmpt_module_set_repeat_count
 */
LIBOPENMPT_API int32_t openmpt_module_get_repeat_count( openmpt_module * mod );

/*! \brief approximate song duration
 *
 * \param mod The module handle to work on.
 * \return Approximate duration of current subsong in seconds.
 */
LIBOPENMPT_API double openmpt_module_get_duration_seconds( openmpt_module * mod );

/*! \brief Set approximate current song position
 *
 * \param mod The module handle to work on.
 * \param seconds Seconds to seek to. If seconds is out of range, the position gets set to song start or end respectively.
 * \return Approximate new song position in seconds.
 * \sa openmpt_module_get_position_seconds
 */
LIBOPENMPT_API double openmpt_module_set_position_seconds( openmpt_module * mod, double seconds );
/*! \brief Get current song position
 *
 * \param mod The module handle to work on.
 * \return Current song position in seconds.
 * \sa openmpt_module_set_position_seconds
 */
LIBOPENMPT_API double openmpt_module_get_position_seconds( openmpt_module * mod );

/*! \brief Set approximate current song position
 *
 * If order or row are out of range, to position is not modified and the current position is returned.
 * \param mod The module handle to work on.
 * \param order Pattern order number to seek to.
 * \param row Pattern row number to seek to.
 * \return Approximate new song position in seconds.
 * \sa openmpt_module_set_position_seconds
 * \sa openmpt_module_get_position_seconds
 */
LIBOPENMPT_API double openmpt_module_set_position_order_row( openmpt_module * mod, int32_t order, int32_t row );

/*! \brief Get render parameter
 *
 * \param mod The module handle to work on.
 * \param param Parameter to query. See openmpt_module_render_param.
 * \param value Pointer to the variable that receives the current value of the parameter.
 * \return 1 on success, 0 on failure (invalid param or value is NULL).
 * \sa OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL
 * \sa OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT
 * \sa OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH
 * \sa OPENMPT_MODULE_RENDER_VOLUMERAMPING_STRENGTH
 * \sa openmpt_module_set_render_param
 */
LIBOPENMPT_API int openmpt_module_get_render_param( openmpt_module * mod, int param, int32_t * value );
/*! \brief Set render parameter
 *
 * \param mod The module handle to work on.
 * \param param Parameter to set. See openmpt_module_render_param.
 * \param value The value to set param to.
 * \return 1 on success, 0 on failure (invalid param).
 * \sa OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL
 * \sa OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT
 * \sa OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH
 * \sa OPENMPT_MODULE_RENDER_VOLUMERAMPING_STRENGTH
 * \sa openmpt_module_get_render_param
 */
LIBOPENMPT_API int openmpt_module_set_render_param( openmpt_module * mod, int param, int32_t value );

/*@{*/
/*! \brief Render audio data
 *
 * \param mod The module handle to work on.
 * \param samplerate Samplerate to render output. Should be in [8000,192000], but this is not enforced.
 * \param count Number of audio frames to render per channel.
 * \param mono Pointer to a buffer of at least count elements that receives the mono/center output.
 * \return The number of frames actually rendered.
 * \retval 0 The end of song has been reached.
 * \remarks The output buffers are only written to up to the returned number of elements.
 * \remarks You can freely switch between any of these function if you see a need to do so. libopenmpt tries to introduce as little switching annoyances as possible. Normally, you would only use a single one of these functions for rendering a particular module.
 * \remarks It is recommended to use the floating point API because of the greater dynamic range and no implied clipping.
 */
LIBOPENMPT_API size_t openmpt_module_read_mono(   openmpt_module * mod, int32_t samplerate, size_t count, int16_t * mono );
/*! \brief Render audio data
 *
 * \param mod The module handle to work on.
 * \param samplerate Samplerate to render output. Should be in [8000,192000], but this is not enforced.
 * \param count Number of audio frames to render per channel.
 * \param left Pointer to a buffer of at least count elements that receives the left output.
 * \param right Pointer to a buffer of at least count elements that receives the right output.
 * \return The number of frames actually rendered.
 * \retval 0 The end of song has been reached.
 * \remarks The output buffers are only written to up to the returned number of elements.
 * \remarks You can freely switch between any of these function if you see a need to do so. libopenmpt tries to introduce as little switching annoyances as possible. Normally, you would only use a single one of these functions for rendering a particular module.
 * \remarks It is recommended to use the floating point API because of the greater dynamic range and no implied clipping.
 */
LIBOPENMPT_API size_t openmpt_module_read_stereo( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * left, int16_t * right );
/*! \brief Render audio data
 *
 * \param mod The module handle to work on.
 * \param samplerate Samplerate to render output. Should be in [8000,192000], but this is not enforced.
 * \param count Number of audio frames to render per channel.
 * \param left Pointer to a buffer of at least count elements that receives the left output.
 * \param right Pointer to a buffer of at least count elements that receives the right output.
 * \param rear_left Pointer to a buffer of at least count elements that receives the rear left output.
 * \param rear_right Pointer to a buffer of at least count elements that receives the rear right output.
 * \return The number of frames actually rendered.
 * \retval 0 The end of song has been reached.
 * \remarks The output buffers are only written to up to the returned number of elements.
 * \remarks You can freely switch between any of these function if you see a need to do so. libopenmpt tries to introduce as little switching annoyances as possible. Normally, you would only use a single one of these functions for rendering a particular module.
 * \remarks It is recommended to use the floating point API because of the greater dynamic range and no implied clipping.
 */
LIBOPENMPT_API size_t openmpt_module_read_quad(   openmpt_module * mod, int32_t samplerate, size_t count, int16_t * left, int16_t * right, int16_t * rear_left, int16_t * rear_right );
/*! \brief Render audio data
 *
 * \param mod The module handle to work on.
 * \param samplerate Samplerate to render output. Should be in [8000,192000], but this is not enforced.
 * \param count Number of audio frames to render per channel.
 * \param mono Pointer to a buffer of at least count elements that receives the mono/center output.
 * \return The number of frames actually rendered.
 * \retval 0 The end of song has been reached.
 * \remarks The output buffers are only written to up to the returned number of elements.
 * \remarks You can freely switch between any of these function if you see a need to do so. libopenmpt tries to introduce as little switching annoyances as possible. Normally, you would only use a single one of these functions for rendering a particular module.
 * \remarks Floating point samples are in the [-1.0..1.0] nominal range. They are not clipped to that range though and thus might overshoot.
 */
LIBOPENMPT_API size_t openmpt_module_read_float_mono(   openmpt_module * mod, int32_t samplerate, size_t count, float * mono );
/*! \brief Render audio data
 *
 * \param mod The module handle to work on.
 * \param samplerate Samplerate to render output. Should be in [8000,192000], but this is not enforced.
 * \param count Number of audio frames to render per channel.
 * \param left Pointer to a buffer of at least count elements that receives the left output.
 * \param right Pointer to a buffer of at least count elements that receives the right output.
 * \return The number of frames actually rendered.
 * \retval 0 The end of song has been reached.
 * \remarks The output buffers are only written to up to the returned number of elements.
 * \remarks You can freely switch between any of these function if you see a need to do so. libopenmpt tries to introduce as little switching annoyances as possible. Normally, you would only use a single one of these functions for rendering a particular module.
 * \remarks Floating point samples are in the [-1.0..1.0] nominal range. They are not clipped to that range though and thus might overshoot.
 */
LIBOPENMPT_API size_t openmpt_module_read_float_stereo( openmpt_module * mod, int32_t samplerate, size_t count, float * left, float * right );
/*! \brief Render audio data
 *
 * \param mod The module handle to work on.
 * \param samplerate Samplerate to render output. Should be in [8000,192000], but this is not enforced.
 * \param count Number of audio frames to render per channel.
 * \param left Pointer to a buffer of at least count elements that receives the left output.
 * \param right Pointer to a buffer of at least count elements that receives the right output.
 * \param rear_left Pointer to a buffer of at least count elements that receives the rear left output.
 * \param rear_right Pointer to a buffer of at least count elements that receives the rear right output.
 * \return The number of frames actually rendered.
 * \retval 0 The end of song has been reached.
 * \remarks The output buffers are only written to up to the returned number of elements.
 * \remarks You can freely switch between any of these function if you see a need to do so. libopenmpt tries to introduce as little switching annoyances as possible. Normally, you would only use a single one of these functions for rendering a particular module.
 * \remarks Floating point samples are in the [-1.0..1.0] nominal range. They are not clipped to that range though and thus might overshoot.
 */
LIBOPENMPT_API size_t openmpt_module_read_float_quad(   openmpt_module * mod, int32_t samplerate, size_t count, float * left, float * right, float * rear_left, float * rear_right );
/*! \brief Render audio data
 *
 * \param mod The module handle to work on.
 * \param samplerate Samplerate to render output. Should be in [8000,192000], but this is not enforced.
 * \param count Number of audio frames to render per channel.
 * \param interleaved_stereo Pointer to a buffer of at least count*2 elements that receives the interleaved stereo output in the order (L,R).
 * \return The number of frames actually rendered.
 * \retval 0 The end of song has been reached.
 * \remarks The output buffers are only written to up to the returned number of elements.
 * \remarks You can freely switch between any of these function if you see a need to do so. libopenmpt tries to introduce as little switching annoyances as possible. Normally, you would only use a single one of these functions for rendering a particular module.
 * \remarks It is recommended to use the floating point API because of the greater dynamic range and no implied clipping.
 */
LIBOPENMPT_API size_t openmpt_module_read_interleaved_stereo( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * interleaved_stereo );
/*! \brief Render audio data
 *
 * \param mod The module handle to work on.
 * \param samplerate Samplerate to render output. Should be in [8000,192000], but this is not enforced.
 * \param count Number of audio frames to render per channel.
 * \param interleaved_quad Pointer to a buffer of at least count*4 elements that receives the interleaved suad surround output in the order (L,R,RL,RR).
 * \return The number of frames actually rendered.
 * \retval 0 The end of song has been reached.
 * \remarks The output buffers are only written to up to the returned number of elements.
 * \remarks You can freely switch between any of these function if you see a need to do so. libopenmpt tries to introduce as little switching annoyances as possible. Normally, you would only use a single one of these functions for rendering a particular module.
 * \remarks It is recommended to use the floating point API because of the greater dynamic range and no implied clipping.
 */
LIBOPENMPT_API size_t openmpt_module_read_interleaved_quad(   openmpt_module * mod, int32_t samplerate, size_t count, int16_t * interleaved_quad   );
/*! \brief Render audio data
 *
 * \param mod The module handle to work on.
 * \param samplerate Samplerate to render output. Should be in [8000,192000], but this is not enforced.
 * \param count Number of audio frames to render per channel.
 * \param interleaved_stereo Pointer to a buffer of at least count*2 elements that receives the interleaved stereo output in the order (L,R).
 * \return The number of frames actually rendered.
 * \retval 0 The end of song has been reached.
 * \remarks The output buffers are only written to up to the returned number of elements.
 * \remarks You can freely switch between any of these function if you see a need to do so. libopenmpt tries to introduce as little switching annoyances as possible. Normally, you would only use a single one of these functions for rendering a particular module.
 * \remarks Floating point samples are in the [-1.0..1.0] nominal range. They are not clipped to that range though and thus might overshoot.
 */
LIBOPENMPT_API size_t openmpt_module_read_interleaved_float_stereo( openmpt_module * mod, int32_t samplerate, size_t count, float * interleaved_stereo );
/*! \brief Render audio data
 *
 * \param mod The module handle to work on.
 * \param samplerate Samplerate to render output. Should be in [8000,192000], but this is not enforced.
 * \param count Number of audio frames to render per channel.
 * \param interleaved_quad Pointer to a buffer of at least count*4 elements that receives the interleaved suad surround output in the order (L,R,RL,RR).
 * \return The number of frames actually rendered.
 * \retval 0 The end of song has been reached.
 * \remarks The output buffers are only written to up to the returned number of elements.
 * \remarks You can freely switch between any of these function if you see a need to do so. libopenmpt tries to introduce as little switching annoyances as possible. Normally, you would only use a single one of these functions for rendering a particular module.
 * \remarks Floating point samples are in the [-1.0..1.0] nominal range. They are not clipped to that range though and thus might overshoot.
*/
LIBOPENMPT_API size_t openmpt_module_read_interleaved_float_quad(   openmpt_module * mod, int32_t samplerate, size_t count, float * interleaved_quad   );
/*@}*/

/*! \brief Get the list of supported metadata item keys
 *
 * \param mod The module handle to work on.
 * \return Metadata item keys supported by openmpt_module_get_metadata, as a semicolon-separated list.
 * \sa openmpt_module_get_metadata
 */
LIBOPENMPT_API const char * openmpt_module_get_metadata_keys( openmpt_module * mod );
/*! \brief Get a metadata item value
 *
 * \param mod The module handle to work on.
 * \param key Metadata item key to query. Use openmpt_module_get_metadata_keys to check for available keys.
 *          Possible keys are:
 *          - type: Module format extension (e.g. it)
 *          - type_long: Tracker name associated with the module format (e.g. Impulse Tracker)
 *          - container: Container format the module file is embedded in, if any (e.g. umx)
 *          - container_long: Full container name if the module is embedded in a container (e.g. Unreal Music)
 *          - tracker: Tracker that was (most likely) used to save the module file, if known
 *          - artist: Author of the module
 *          - title: Module title
 *          - date: Date the module was last saved, in ISO-8601 format.
 *          - message: Song message. If the song message is empty or the module format does not support song messages, a list of instrument and sample names is returned instead.
 *          - message_raw: Song message. If the song message is empty or the module format does not support song messages, an empty string is returned.
 *          - warnings: A list of warnings that were generated while loading the module.
 * \return The associated value for key.
 * \sa openmpt_module_get_metadata_keys
 */
LIBOPENMPT_API const char * openmpt_module_get_metadata( openmpt_module * mod, const char * key );

/*! \brief Get the current speed
 *
 * \param mod The module handle to work on.
 * \return The current speed in ticks per row.
 */
LIBOPENMPT_API int32_t openmpt_module_get_current_speed( openmpt_module * mod );
/*! \brief Get the current tempo
 *
 * \param mod The module handle to work on.
 * \return The current tempo in tracker units. The exact meaning of this value depends on the tempo mode being used.
 */
LIBOPENMPT_API int32_t openmpt_module_get_current_tempo( openmpt_module * mod );
/*! \brief Get the current order
 *
 * \param mod The module handle to work on.
 * \return The current order at which the module is being played back.
 */
LIBOPENMPT_API int32_t openmpt_module_get_current_order( openmpt_module * mod );
/*! \brief Get the current pattern
 *
 * \param mod The module handle to work on.
 * \return The current pattern that is being played.
 */
LIBOPENMPT_API int32_t openmpt_module_get_current_pattern( openmpt_module * mod );
/*! \brief Get the current row
 *
 * \param mod The module handle to work on.
 * \return The current row at which the current pattern is being played.
 */
LIBOPENMPT_API int32_t openmpt_module_get_current_row( openmpt_module * mod );
/*! \brief Get the current amount of playing channels.
 *
 * \param mod The module handle to work on.
 * \return The amount of sample channels that are currently being rendered.
 */
LIBOPENMPT_API int32_t openmpt_module_get_current_playing_channels( openmpt_module * mod );

/*! \brief Get an approximate indication of the channel volume.
 *
 * \param mod The module handle to work on.
 * \param channel The channel whose volume should be retrieved.
 * \return The approximate channel volume.
 * \remarks The returned value is solely based on the note velocity and does not take the actual waveform of the playing sample into account.
 */
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_mono( openmpt_module * mod, int32_t channel );
/*! \brief Get an approximate indication of the channel volume on the front-left speaker.
 *
 * \param mod The module handle to work on.
 * \param channel The channel whose volume should be retrieved.
 * \return The approximate channel volume.
 * \remarks The returned value is solely based on the note velocity and does not take the actual waveform of the playing sample into account.
 */
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_left( openmpt_module * mod, int32_t channel );
/*! \brief Get an approximate indication of the channel volume on the front-right speaker.
 *
 * \param mod The module handle to work on.
 * \param channel The channel whose volume should be retrieved.
 * \return The approximate channel volume.
 * \remarks The returned value is solely based on the note velocity and does not take the actual waveform of the playing sample into account.
 */
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_right( openmpt_module * mod, int32_t channel );
/*! \brief Get an approximate indication of the channel volume on the rear-left speaker.
 *
 * \param mod The module handle to work on.
 * \param channel The channel whose volume should be retrieved.
 * \return The approximate channel volume.
 * \remarks The returned value is solely based on the note velocity and does not take the actual waveform of the playing sample into account.
 */
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_rear_left( openmpt_module * mod, int32_t channel );
/*! \brief Get an approximate indication of the channel volume on the rear-right speaker.
 *
 * \param mod The module handle to work on.
 * \param channel The channel whose volume should be retrieved.
 * \return The approximate channel volume.
 * \remarks The returned value is solely based on the note velocity and does not take the actual waveform of the playing sample into account.
 */
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_rear_right( openmpt_module * mod, int32_t channel );

/*! \brief Get the number of subsongs
 *
 * \param mod The module handle to work on.
 * \return The number of subsongs in the module. This includes any "hidden" songs (songs that share the same sequence, but start at different order indices) and "normal" subsongs or "sequences" (if the format supports them).
 * \sa openmpt_module_get_subsong_names, openmpt_module_select_subsong
 */
LIBOPENMPT_API int32_t openmpt_module_get_num_subsongs( openmpt_module * mod );
/*! \brief Get the number of pattern channels
 *
 * \param mod The module handle to work on.
 * \return The number of pattern channels in the module. Not all channels do necessarily contain data.
 * \remarks The number of pattern channels is completely independent of the number of output channels. libopenmpt can render modules in mono, stereo or quad surround, but the choice of which of the three modes to use must not be made based on the return value of this function, which may be any positive integer amount. Only use this function for informational purposes.
 */
LIBOPENMPT_API int32_t openmpt_module_get_num_channels( openmpt_module * mod );
/*! \brief Get the number of orders
 *
 * \param mod The module handle to work on.
 * \return The number of orders in the current sequence of the module.
 */
LIBOPENMPT_API int32_t openmpt_module_get_num_orders( openmpt_module * mod );
/*! \brief Get the number of patterns
 *
 * \param mod The module handle to work on.
 * \return The number of distinct patterns in the module.
 */
LIBOPENMPT_API int32_t openmpt_module_get_num_patterns( openmpt_module * mod );
/*! \brief Get the number of instruments
 *
 * \param mod The module handle to work on.
 * \return The number of instrument slots in the module. Instruments are a layer on top of samples, and are not supported by all module formats.
 */
LIBOPENMPT_API int32_t openmpt_module_get_num_instruments( openmpt_module * mod );
/*! \brief Get the number of samples
 *
 * \param mod The module handle to work on.
 * \return The number of sample slots in the module.
 */
LIBOPENMPT_API int32_t openmpt_module_get_num_samples( openmpt_module * mod );

/*! \brief Get a subsong name
 *
 * \param mod The module handle to work on.
 * \param index The subsong whose name should be retreived
 * \return The subsong name.
 * \sa openmpt_module_get_num_subsongs, openmpt_module_select_subsong
 */
LIBOPENMPT_API const char * openmpt_module_get_subsong_name( openmpt_module * mod, int32_t index );
/*! \brief Get a channel name
 *
 * \param mod The module handle to work on.
 * \param index The channel whose name should be retreived
 * \return The channel name.
 * \sa openmpt_module_get_num_channels
 */
LIBOPENMPT_API const char * openmpt_module_get_channel_name( openmpt_module * mod, int32_t index );
/*! \brief Get an order name
 *
 * \param mod The module handle to work on.
 * \param index The order whose name should be retreived
 * \return The order name.
 * \sa openmpt_module_get_num_orders
 */
LIBOPENMPT_API const char * openmpt_module_get_order_name( openmpt_module * mod, int32_t index );
/*! \brief Get a pattern name
 *
 * \param mod The module handle to work on.
 * \param index The pattern whose name should be retreived
 * \return The pattern name.
 * \sa openmpt_module_get_num_patterns
 */
LIBOPENMPT_API const char * openmpt_module_get_pattern_name( openmpt_module * mod, int32_t index );
/*! \brief Get an instrument name
 *
 * \param mod The module handle to work on.
 * \param index The instrument whose name should be retreived
 * \return The instrument name.
 * \sa openmpt_module_get_num_instruments
 */
LIBOPENMPT_API const char * openmpt_module_get_instrument_name( openmpt_module * mod, int32_t index );
/*! \brief Get a sample name
 *
 * \param mod The module handle to work on.
 * \param index The sample whose name should be retreived
 * \return The sample name.
 * \sa openmpt_module_get_num_samples
 */
LIBOPENMPT_API const char * openmpt_module_get_sample_name( openmpt_module * mod, int32_t index );

/*! \brief Get pattern at order position
 *
 * \param mod The module handle to work on.
 * \param order The order item whose pattern index should be retrieved.
 * \return The pattern index found at the given order position of the current sequence.
 */
LIBOPENMPT_API int32_t openmpt_module_get_order_pattern( openmpt_module * mod, int32_t order );
/*! \brief Get the number of rows in a pattern
 *
 * \param mod The module handle to work on.
 * \param pattern The pattern whose row count should be retrieved.
 * \return The number of rows in the given pattern. If the pattern does not exist, 0 is returned.
 */
LIBOPENMPT_API int32_t openmpt_module_get_pattern_num_rows( openmpt_module * mod, int32_t pattern );

/*! \brief Get raw pattern content
 *
 * \param mod The module handle to work on.
 * \param pattern The pattern whose data should be retrieved.
 * \param row The row from which the data should be retrieved.
 * \param channel The channel from which the data should be retrieved.
 * \param command The cell index at which the data should be retrieved. See openmpt_module_command_index
 * \return The internal, raw pattern data at the given pattern position.
 */
LIBOPENMPT_API uint8_t openmpt_module_get_pattern_row_channel_command( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, int command );

/*! \brief Get formatted (human-readable) pattern content
 *
 * \param mod The module handle to work on.
 * \param pattern The pattern whose data should be retrieved.
 * \param row The row from which the data should be retrieved.
 * \param channel The channel from which the data should be retrieved.
 * \param command The cell index at which the data should be retrieved.
 * \return The formatted pattern data at the given pattern position. See openmpt_module_command_index
 * \sa openmpt_module_highlight_pattern_row_channel_command
 */
LIBOPENMPT_API const char * openmpt_module_format_pattern_row_channel_command( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, int command );
/*! \brief Get highlighting information for formatted pattern content
 *
 * \param mod The module handle to work on.
 * \param pattern The pattern whose data should be retrieved.
 * \param row The row from which the data should be retrieved.
 * \param channel The channel from which the data should be retrieved.
 * \param command The cell index at which the data should be retrieved. See openmpt_module_command_index
 * \return The highlighting string for the formatted pattern data as retrived by openmpt_module_get_pattern_row_channel_command at the given pattern position.
 * \remarks The returned string will map each character position of the string returned by openmpt_module_get_pattern_row_channel_command to a highlighting instruction.
 *          Possible highlighting characters are:
 *          - " " : empty/space
 *          - "." : empty/dot
 *          - "n" : generic note
 *          - "m" : special note
 *          - "i" : generic instrument
 *          - "u" : generic volume column effect
 *          - "v" : generic volume column parameter
 *          - "e" : generic effect column effect
 *          - "f" : generic effect column parameter
 * \sa openmpt_module_get_pattern_row_channel_command
 */
LIBOPENMPT_API const char * openmpt_module_highlight_pattern_row_channel_command( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, int command );

/*! \brief Get formatted (human-readable) pattern content
 *
 * \param mod The module handle to work on.
 * \param pattern The pattern whose data should be retrieved.
 * \param row The row from which the data should be retrieved.
 * \param channel The channel from which the data should be retrieved.
 * \param width The maximum number of characters the string should contain. 0 means no limit.
 * \param pad If true, the string will be resized to the exact length provided in the width parameter.
 * \return The formatted pattern data at the given pattern position.
 * \sa openmpt_module_highlight_pattern_row_channel
 */
LIBOPENMPT_API const char * openmpt_module_format_pattern_row_channel( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, size_t width, int pad );
/*! \brief Get highlighting information for formatted pattern content
 *
 * \param mod The module handle to work on.
 * \param pattern The pattern whose data should be retrieved.
 * \param row The row from which the data should be retrieved.
 * \param channel The channel from which the data should be retrieved.
 * \param width The maximum number of characters the string should contain. 0 means no limit.
 * \param pad If true, the string will be resized to the exact length provided in the width parameter.
 * \return The highlighting string for the formatted pattern data as retrived by openmpt_module_format_pattern_row_channel at the given pattern position.
 * \sa openmpt_module_format_pattern_row_channel
 */
LIBOPENMPT_API const char * openmpt_module_highlight_pattern_row_channel( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, size_t width, int pad );

/*! \brief Retrieve supported ctl keys
 *
 * \param mod The module handle to work on.
 * \return A semicolon-separated list containing all supported ctl keys.
 * \remarks Currently supported ctl values are:
 *          - load.skip_samples: Set to "1" to avoid loading samples into memory
 *          - load.skip_patterns: Set to "1" to avoid loading patterns into memory
 *          - load.skip_plugins: Set to "1" to avoid loading plugins
 *          - load.skip_subsongs_init: Set to "1" to avoid pre-initializing subsongs. Skipping results in faster module loading but slower seeking.
 *          - seek.sync_samples: Set to "1" to sync sample playback when using openmpt_module_set_position_seconds or openmpt_module_set_position_order_row.
 *          - play.tempo_factor: Set a floating point tempo factor. "1.0" is the default tempo.
 *          - play.pitch_factor: Set a floating point pitch factor. "1.0" is the default pitch.
 *          - dither: Set the dither algorithm that is used for the 16 bit versions of openmpt_module_read. Supported values are:
 *                    - 0: No dithering.
 *                    - 1: Default mode. Chosen by OpenMPT code, might change.
 *                    - 2: Rectangular, 0.5 bit depth, no noise shaping (original ModPlug Tracker).
 *                    - 3: Rectangular, 1 bit depth, simple 1st order noise shaping
 */
LIBOPENMPT_API const char * openmpt_module_get_ctls( openmpt_module * mod );
/*! \brief Get current ctl value
 *
 * \param mod The module handle to work on.
 * \param ctl The ctl key whose value should be retrieved.
 * \return The associated ctl value, or NULL on failure.
 * \sa openmpt_module_get_ctls
 */
LIBOPENMPT_API const char * openmpt_module_ctl_get( openmpt_module * mod, const char * ctl );
/*! \brief Set ctl value
 *
 * \param mod The module handle to work on.
 * \param ctl The ctl key whose value should be set.
 * \param value The value that should be set.
 * \sa openmpt_module_get_ctls
 */
LIBOPENMPT_API int openmpt_module_ctl_set( openmpt_module * mod, const char * ctl, const char * value );

/* remember to add new functions to both C and C++ interfaces and to increase OPENMPT_API_VERSION_MINOR */

#ifdef __cplusplus
}
#endif

/*!
  @}
*/

#endif /* LIBOPENMPT_H */

