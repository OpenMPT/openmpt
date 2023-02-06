
#ifndef MPG123_MAKEFILE_H
#define MPG123_MAKEFILE_H

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

typedef ssize_t mpg123_ssize_t;

#ifdef __cplusplus
}
#endif

#include "mpg123.h.in"

#endif /* MPG123_MAKEFILE_H */
