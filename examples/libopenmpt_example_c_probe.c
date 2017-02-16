/*
 * libopenmpt_example_c_probe.c
 * ----------------------------
 * Purpose: libopenmpt C API probing example
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * Usage: libopenmpt_example_c_probe SOMEMODULE
 * Returns 0 on successful probing.
 * Returns 1 on failed probing.
 * Returns 2 on error.
 */

#define LIBOPENMPT_EXAMPLE_PROBE_RESULT_BINARY 1
#define LIBOPENMPT_EXAMPLE_PROBE_RESULT_FLOAT  2

#define LIBOPENMPT_EXAMPLE_PROBE_STYLE_CALLBACKS 1
#define LIBOPENMPT_EXAMPLE_PROBE_STYLE_PREFIX    2

#define LIBOPENMPT_EXAMPLE_PROBE_STYLE  LIBOPENMPT_EXAMPLE_PROBE_STYLE_PREFIX

#define LIBOPENMPT_EXAMPLE_PROBE_RESULT LIBOPENMPT_EXAMPLE_PROBE_RESULT_FLOAT

/* Use something between 1024 and 65536. 4096 is a sane default. */
#define LIBOPENMPT_EXAMPLE_PROBE_PREFIX_BYTES (4096)

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libopenmpt/libopenmpt.h>

#if ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_CALLBACKS )
#include <libopenmpt/libopenmpt_stream_callbacks_file.h>
#endif

#if ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_PREFIX )

#if defined( LIBOPENMPT_STREAM_CALLBACKS_BUFFER )
#include <libopenmpt/libopenmpt_stream_callbacks_buffer.h>
#else
#error "libopenmpt too old."
#endif

static double libopenmpt_example_could_open_probability_prefix( const void * prefix_data, size_t prefix_size_, int64_t file_size, double effort, openmpt_log_func logfunc, void * loguser, openmpt_error_func errfunc, void * erruser, int * error, const char * * error_message ) {
	double ret = 0.0;
	int64_t prefix_size = prefix_size_;
	openmpt_stream_callbacks openmpt_stream_callbacks_prefix_probe;
	openmpt_stream_buffer stream;
	memset( &openmpt_stream_callbacks_prefix_probe, 0, sizeof( openmpt_stream_callbacks ) );
	memset( &stream, 0, sizeof( openmpt_stream_buffer ) );
	if ( !prefix_data ) {
		return 0.0;
	}
	if ( prefix_size < 0 ) {
		return 0.0;
	}
	if ( file_size < 0 ) {
		return 0.0;
	}
	if ( prefix_size > file_size ) {
		prefix_size = file_size;
	}
	openmpt_stream_callbacks_prefix_probe = openmpt_stream_get_buffer_callbacks();
	openmpt_stream_buffer_init_prefix_only( &stream, prefix_data, prefix_size, file_size );
	ret = openmpt_could_open_probability2( openmpt_stream_callbacks_prefix_probe, &stream, effort, logfunc, loguser, errfunc, erruser, error, error_message );
	if ( openmpt_stream_buffer_overflowed( &stream ) ) {
		ret = 0.5;
	}
	return ret;
}

#if ( LIBOPENMPT_EXAMPLE_PROBE_RESULT == LIBOPENMPT_EXAMPLE_PROBE_RESULT_BINARY )

static int libopenmpt_example_probe_file_header_prefix( const void * prefix_data, size_t prefix_size, int64_t file_size, openmpt_log_func logfunc, void * loguser, openmpt_error_func errfunc, void * erruser, int * error, const char * * error_message ) {
	double ret = libopenmpt_example_could_open_probability_prefix( prefix_data, prefix_size, file_size, 0.25, logfunc, loguser, errfunc, erruser, error, error_message );
	if ( ret >= 0.5 ) {
		return 1;
	}
	if ( ret < 0.25 ) {
		return 0;
	}
	return 1;
}

#endif

#endif

#if ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_CALLBACKS )

#if ( LIBOPENMPT_EXAMPLE_PROBE_RESULT == LIBOPENMPT_EXAMPLE_PROBE_RESULT_BINARY )

static int libopenmpt_example_probe_file_header( openmpt_stream_callbacks stream_callbacks, void * stream, openmpt_log_func logfunc, void * loguser, openmpt_error_func errfunc, void * erruser, int * error, const char * * error_message ) {
	double ret = openmpt_could_open_probability2( stream_callbacks, stream, 0.25, logfunc, loguser, errfunc, erruser, error, error_message );
	if ( ret >= 0.5 ) {
		return 1;
	}
	if ( ret < 0.25 ) {
		return 0;
	}
	return 1;
}

#endif

#endif

static void libopenmpt_example_logfunc( const char * message, void * userdata ) {
	(void)userdata;

	if ( message ) {
		fprintf( stderr, "%s\n", message );
	}
}

#if ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_PREFIX )

typedef struct blob_t {
	size_t size;
	void * data;
} blob_t;

static void free_blob( blob_t * blob ) {
	if ( blob ) {
		if ( blob->data ) {
			free( blob->data );
			blob->data = 0;
		}
		blob->size = 0;
		free( blob );
	}
}

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
static blob_t * load_file( const wchar_t * filename ) {
#else
static blob_t * load_file( const char * filename ) {
#endif
	blob_t * result = 0;

	blob_t * blob = 0;
	FILE * file = 0;
	long tell_result = 0;

	blob = malloc( sizeof( blob_t ) );
	if ( !blob ) {
		goto fail;
	}
	memset( blob, 0, sizeof( blob_t ) );

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
	file = _wfopen( filename, L"rb" );
#else
	file = fopen( filename, "rb" );
#endif
	if ( !file ) {
		goto fail;
	}

	if ( fseek( file, 0, SEEK_END ) != 0 ) {
		goto fail;
	}

	tell_result = ftell( file );
	if ( tell_result < 0 ) {
		goto fail;
	}
	if ( (unsigned long)tell_result > SIZE_MAX ) {
		goto fail;
	}
	blob->size = (size_t)tell_result;

	if ( fseek( file, 0, SEEK_SET ) != 0 ) {
		goto fail;
	}

	blob->data = malloc( blob->size );
	if ( !blob->data ) {
		goto fail;
	}
	memset( blob->data, 0, blob->size );

	if ( fread( blob->data, 1, blob->size, file ) != blob->size ) {
		goto fail;
	}

	result = blob;
	blob = 0;
	goto cleanup;

fail:

	result = 0;

cleanup:

	if ( blob ) {
		free_blob( blob );
		blob = 0;
	}

	if ( file ) {
		fclose( file );
		file = 0;
	}

	return result;
}

#endif

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
int wmain( int argc, wchar_t * argv[] ) {
#else
int main( int argc, char * argv[] ) {
#endif

	int result = 0;
	int mod_err = OPENMPT_ERROR_OK;
#if ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_PREFIX )
	blob_t * blob = 0;
#elif ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_CALLBACKS )
	FILE * file = NULL;
#endif

#if ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_PREFIX )
	size_t prefix_size = LIBOPENMPT_EXAMPLE_PROBE_PREFIX_BYTES;
#endif
#if ( LIBOPENMPT_EXAMPLE_PROBE_RESULT == LIBOPENMPT_EXAMPLE_PROBE_RESULT_FLOAT )
	double probability = 0.0;
#endif
#if ( LIBOPENMPT_EXAMPLE_PROBE_RESULT == LIBOPENMPT_EXAMPLE_PROBE_RESULT_BINARY )
	int result_binary = 0;
#endif

	if ( argc != 2 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_probe SOMEMODULE'." );
		goto fail;
	}

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
	if ( wcslen( argv[1] ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_probe SOMEMODULE'." );
		goto fail;
	}
#else
	if ( strlen( argv[1] ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_probe SOMEMODULE'." );
		goto fail;
	}
#endif

#if ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_PREFIX )
	blob = load_file( argv[1] );
	if ( !blob ) {
		fprintf( stderr, "Error: %s\n", "load_file() failed." );
		goto fail;
	}
#elif ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_CALLBACKS )
#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
	if ( wcslen( argv[1] ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c SOMEMODULE'." );
		goto fail;
	}
	file = _wfopen( argv[1], L"rb" );
#else
	if ( strlen( argv[1] ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c SOMEMODULE'." );
		goto fail;
	}
	file = fopen( argv[1], "rb" );
#endif
	if ( !file ) {
		fprintf( stderr, "Error: %s\n", "fopen() failed." );
		goto fail;
	}
#endif

	#if ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_PREFIX )

		if ( prefix_size > blob->size ) {
			prefix_size = blob->size;
		}
		#if ( LIBOPENMPT_EXAMPLE_PROBE_RESULT == LIBOPENMPT_EXAMPLE_PROBE_RESULT_BINARY )
			result_binary = ( libopenmpt_example_probe_file_header_prefix( blob->data, prefix_size, blob->size, &libopenmpt_example_logfunc, NULL, &openmpt_error_func_default, NULL, &mod_err, NULL ) <= 0 ) ? 0 : 1;
			fprintf( stdout, "%s\n", result_binary ? "Success." : "Failure." );
			if ( result_binary ) {
				result = 0;
			} else {
				result = 1;
			}
		#elif ( LIBOPENMPT_EXAMPLE_PROBE_RESULT == LIBOPENMPT_EXAMPLE_PROBE_RESULT_FLOAT )
			probability = libopenmpt_example_could_open_probability_prefix( blob->data, prefix_size, blob->size, 0.25, &libopenmpt_example_logfunc, NULL, &openmpt_error_func_default, NULL, &mod_err, NULL );
			fprintf( stdout, "%s: %f\n", "Result", probability );
			if ( probability >= 0.5 ) {
				result = 0;
			} else {
				result = 1;
			}
		#else
			#error "LIBOPENMPT_EXAMPLE_PROBE_RESULT is wrong"
		#endif

	#elif ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_CALLBACKS )

		#if ( LIBOPENMPT_EXAMPLE_PROBE_RESULT == LIBOPENMPT_EXAMPLE_PROBE_RESULT_BINARY )
			result_binary = ( libopenmpt_example_probe_file_header( openmpt_stream_get_file_callbacks(), file, &libopenmpt_example_logfunc, NULL, &openmpt_error_func_default, NULL, &mod_err, NULL ) <= 0 ) ? 0 : 1;
			fprintf( stdout, "%s\n", result_binary ? "Success." : "Failure." );
			if ( result_binary ) {
				result = 0;
			} else {
				result = 1;
			}
		#elif ( LIBOPENMPT_EXAMPLE_PROBE_RESULT == LIBOPENMPT_EXAMPLE_PROBE_RESULT_FLOAT )
			probability = openmpt_could_open_probability( openmpt_stream_get_file_callbacks(), file, 0.25, &libopenmpt_example_logfunc, NULL, &openmpt_error_func_default, NULL, &mod_err, NULL );
			fprintf( stdout, "%s: %f\n", "Result", probability );
			if ( probability >= 0.5 ) {
				result = 0;
			} else {
				result = 1;
			}
		#else
			#error "LIBOPENMPT_EXAMPLE_PROBE_RESULT is wrong"
		#endif

	#else
		#error "LIBOPENMPT_EXAMPLE_PROBE_STYLE is wrong"
	#endif

	goto cleanup;

fail:

	result = 2;

cleanup:

#if ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_PREFIX )
	if ( blob ) {
		free_blob( blob );
		blob = 0;
	}
#endif

#if ( LIBOPENMPT_EXAMPLE_PROBE_STYLE == LIBOPENMPT_EXAMPLE_PROBE_STYLE_CALLBACKS )
	if ( file ) {
		fclose( file );
		file = 0;
	}
#endif

	return result;
}

