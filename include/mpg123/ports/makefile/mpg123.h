#ifndef MPG123_MAKEFILE_H
#define MPG123_MAKEFILE_H

#include <stdlib.h>
#include <sys/types.h>

#include <stdint.h>
#include <inttypes.h>

#define MPG123_API_VERSION 46
#define MPG123_NO_CONFIGURE
#define MPG123_NO_LARGENAME
#include <stddef.h>
typedef ssize_t mpg123_ssize_t;
#include "mpg123.h.in"

#endif

