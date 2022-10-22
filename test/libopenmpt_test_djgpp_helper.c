/*
 * libopenmpt_test_djgpp_helper.c
 * ------------------------------
 * Purpose: libopenmpt test suite driver
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#if defined( __DJGPP__ )
#include <crt0.h>
#endif /* __DJGPP__ */

#if defined( __DJGPP__ )
/* clang-format off */
int _crt0_startup_flags = 0
	| _CRT0_FLAG_NONMOVE_SBRK          /* force interrupt compatible allocation */
	| _CRT0_DISABLE_SBRK_ADDRESS_WRAP  /* force NT compatible allocation */
	| _CRT0_FLAG_LOCK_MEMORY           /* lock all code and data at program startup */
	| 0;
/* clang-format on */
#else
unsigned char mpt_libopenmpt_test_djgpp_helper_dummy = 0;
#endif /* __DJGPP__ */
