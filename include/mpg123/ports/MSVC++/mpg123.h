/*
	mpg123_msvc: MPEG Audio Decoder library wrapper header for MS VC++ 2015 and later

	copyright 2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	copyright 2023 by the OpenMPT project
	initially written by Patrick Dehne and Thomas Orgis.
	modified for OpenMPT by Jörn 'manx' Heusipp.
*/

#ifndef MPG123_MSVC_H
#define MPG123_MSVC_H

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/types.h>

#define MPG123_API_VERSION 47
#define MPG123_NO_CONFIGURE
#define MPG123_NO_LARGENAME

#ifdef __cplusplus
extern "C" {
#endif

typedef ptrdiff_t mpg123_ssize_t;

#ifdef __cplusplus
}
#endif

#include "mpg123.h.in"

#endif /* MPG123_MSVC_H */
